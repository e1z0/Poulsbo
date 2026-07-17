// SPDX-License-Identifier: GPL-2.0-only
/*
 * gma500_flip.c - Zero-copy page-flip scanout helpers for the SGX535 GL
 *                 swap chain on Poulsbo (GMA500).
 *
 * These helpers are EXPORT_SYMBOL'd so the out-of-tree display-class module
 * (gma500dc.ko) can:
 *   - allocate GTT-pinned, scanout-capable buffers that the SGX renders into
 *     (gma500_flip_alloc / gma500_flip_free), and
 *   - flip the primary CRTC scanout address to a just-rendered buffer with no
 *     CPU copy (gma500_flip_to).
 *
 * The buffers are ordinary gma500 gtt_range GEM ranges:
 *   - psb_gtt_pin() pins them into the GTT (so the display can scan them out)
 *     AND maps their pages into the gma500 default MMU page directory.
 *   - Their struct page ** array is handed (via gma500dc) to pvrsrvkm, which
 *     maps the same physical pages into the SGX GL rendering context.  So the
 *     SGX renders into the very pages the display scans out => zero copy.
 *
 * The flip itself is a single register write to the pipe-0 display surface
 * base register (DSPABASE, 0x70184 on Poulsbo).  See gma_pipe_set_base() in
 * gma_display.c: for IS_PSB() parts the scanout base is programmed as
 *     REG_WRITE(map->base, offset + start)
 * where start == gtt_range->offset.  gma500_flip_to() replicates just that
 * base write (offset == 0, full-surface flip) for pipe 0, wrapped in the same
 * gma_power_begin()/gma_power_end() sequencing gma_pipe_set_base() uses.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/vmalloc.h>

#include <drm/drm_device.h>

#include <linux/delay.h>
#include "psb_drv.h"		/* pulls in gtt.h, power.h, REG_*/IS_PSB, dev_priv */
#include "psb_intel_reg.h"	/* PIPEACONF / PIPEASTAT / vblank status bits */
#include "gma500_flip.h"

/*
 * flip_vsync - when non-zero, gma500_flip_to() waits for the next start-of-
 * vblank on pipe 0 after arming the new scanout base, so a page-flip present
 * (eglSwapBuffers) does not return until the buffer is actually being scanned
 * out.  This throttles rendering to the panel refresh (~60 Hz) and eliminates
 * tearing, at the cost of the per-frame vblank wait.  Default 0 = off (lowest
 * latency; a fast full-screen app may tear).  Runtime-writable via sysfs:
 *   /sys/module/gma500_gfx/parameters/flip_vsync
 */
static int flip_vsync;
module_param(flip_vsync, int, 0644);
MODULE_PARM_DESC(flip_vsync,
	"Wait for vblank on each zero-copy page-flip present "
	"(0=off/low-latency [default], 1=on/anti-tearing)");

/*
 * gma500_flip_active_pipe - return the pipe currently scanning out (its
 * PIPExCONF ENABLE bit set), or -1 if none.  On the Sony Vaio the LVDS panel is
 * driven by pipe B (plane B), not pipe A, so a fixed pipe-0 flip writes a
 * disabled plane and is invisible.  Scanning the regmap makes the flip target
 * whichever pipe actually owns the panel.
 */
static int gma500_flip_active_pipe(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	unsigned int pipe;

	for (pipe = 0; pipe < dev_priv->num_pipe; pipe++) {
		const struct psb_offset *m = &dev_priv->regmap[pipe];

		if (!m->conf)
			continue;
		if (REG_READ(m->conf) & PIPEACONF_ENABLE)
			return (int)pipe;
	}
	return -1;
}

/*
 * gma500_flip_wait_vblank - bounded wait for the next frame boundary on @map's
 * pipe.  Polls the per-pipe frame counter (PIPExFRAMEPIXEL bits [31:24] carry
 * the frame-count low byte, incremented once per scanned-out frame) and returns
 * when it changes.  This is used instead of the PIPExSTAT vblank status bit,
 * which stays latched on this hardware unless the vblank IRQ is enabled and so
 * cannot be polled for an edge.  The frame counter runs whenever the pipe is
 * clocked, independent of interrupt state.  Hard-bounded to ~20 ms (>> one
 * 60 Hz frame) so a stalled pipe can never hang the flip / MISR workqueue.
 * Caller must hold a gma_power_begin() reference (registers are live).
 */
static void gma500_flip_wait_vblank(struct drm_device *dev, const struct psb_offset *map)
{
	u32 frame_reg, start;
	int waited_us;

	if (!map->conf || !(REG_READ(map->conf) & PIPEACONF_ENABLE))
		return;

	/* PIPExFRAMEPIXEL sits at (pipe MMIO base)+0x44; map->conf is
	 * (pipe base)+0x08, so mask to the pipe base and add 0x44. */
	frame_reg = (map->conf & ~0xFFFu) + 0x44;

	start = REG_READ(frame_reg) >> PIPE_FRAME_LOW_SHIFT;
	for (waited_us = 0; waited_us < 25000; waited_us += 300) {
		if ((REG_READ(frame_reg) >> PIPE_FRAME_LOW_SHIFT) != start)
			return;
		usleep_range(200, 350);
	}
}

/*
 * The bound gma500 drm_device.  Published by psb_driver_load() (see psb_drv.c)
 * so the exported helpers can find the device without a drm_crtc in hand.
 */
struct drm_device *gma500_drm_dev;

/**
 * gma500_get_drm - return the bound gma500 DRM device (or NULL if not probed)
 */
struct drm_device *gma500_get_drm(void)
{
	return gma500_drm_dev;
}
EXPORT_SYMBOL(gma500_get_drm);

/**
 * gma500_flip_alloc - allocate a GTT-pinned, scanout-capable render buffer
 * @width:  buffer width in pixels
 * @height: buffer height in pixels
 * @stride_out: (out) byte stride actually used (ALIGN(width*4, 64))
 * @out:    (out) filled flipbuf descriptor
 *
 * Allocates a non-stolen gtt_range via the kernel-internal allocator (NOT the
 * GEM ioctl path), pins it into the GTT, and exposes its backing pages + GTT
 * offset so the caller can (a) hand the pages to pvrsrvkm for SGX MMU mapping
 * and (b) flip scanout to gtt_offset.  Returns 0 on success, negative errno.
 */
int gma500_flip_alloc(u32 width, u32 height, u32 *stride_out,
		      struct gma_flipbuf *out)
{
	struct drm_device *dev = gma500_get_drm();
	struct gtt_range *gt;
	u32 stride, size;
	int ret;

	if (!dev)
		return -ENODEV;
	if (!out || !width || !height)
		return -EINVAL;

	stride = ALIGN(width * 4, 64);
	size = stride * height;

	/*
	 * backed=0 -> GEM-style range backed by shmem pages (not stolen RAM),
	 * so psb_gtt_pin() allocates real struct pages we can also map into the
	 * SGX.  align to PAGE_SIZE.
	 */
	gt = psb_gtt_alloc_range(dev, size, "sgx-flip", 0, PAGE_SIZE);
	if (!gt)
		return -ENOSPC;

	/* GEM bookkeeping so the range behaves like other gma500 objects. */
	ret = drm_gem_object_init(dev, &gt->gem, size);
	if (ret) {
		psb_gtt_free_range(dev, gt);
		return ret;
	}
	/* 32-bit device: keep the backing pages in the DMA32 zone. */
	mapping_set_gfp_mask(gt->gem.filp->f_mapping, GFP_KERNEL | __GFP_DMA32);

	/* Pin: attaches pages, inserts GTT entries, maps into the gma500 MMU. */
	ret = psb_gtt_pin(gt);
	if (ret < 0) {
		drm_gem_object_release(&gt->gem);
		psb_gtt_free_range(dev, gt);
		return ret;
	}

	memset(out, 0, sizeof(*out));
	out->gt = gt;
	out->gtt_offset = gt->offset;
	out->pages = gt->pages;
	out->npage = gt->npage;

	/* Optional CPU view of the buffer (kernel VA) for debug / clears. */
	out->cpu = vmap(gt->pages, gt->npage, VM_MAP, PAGE_KERNEL);
	/* vmap failure is non-fatal: CPU access is optional, flipping is not. */

	if (stride_out)
		*stride_out = stride;

	return 0;
}
EXPORT_SYMBOL(gma500_flip_alloc);

/**
 * gma500_flip_free - unpin + free a flipbuf allocated by gma500_flip_alloc
 */
void gma500_flip_free(struct gma_flipbuf *fb)
{
	struct drm_device *dev = gma500_get_drm();

	if (!fb || !fb->gt)
		return;

	if (fb->cpu) {
		vunmap(fb->cpu);
		fb->cpu = NULL;
	}

	psb_gtt_unpin(fb->gt);
	drm_gem_object_release(&fb->gt->gem);
	if (dev)
		psb_gtt_free_range(dev, fb->gt);
	fb->gt = NULL;
	fb->pages = NULL;
	fb->npage = 0;
	fb->gtt_offset = 0;
}
EXPORT_SYMBOL(gma500_flip_free);

/**
 * gma500_flip_to - point the active-pipe scanout at @gtt_offset
 * @gtt_offset: GTT byte offset of the buffer to display (gma_flipbuf.gtt_offset)
 *
 * This is the zero-copy present: a single MMIO write to the display surface
 * base register of whichever pipe currently drives the panel (pipe B on the
 * Vaio).  Replicates gma_pipe_set_base()'s base-address write using that pipe's
 * regmap entry and the same power sequencing.  Returns 0 on success, negative
 * errno on failure.
 *
 * NOTE: this changes only the scanout base.  Stride / pixel format / plane
 * size must already have been programmed by a prior modeset (they do not
 * change across a flip of equally-sized ARGB8888 buffers).
 */
int gma500_flip_to(u32 gtt_offset)
{
	struct drm_device *dev = gma500_get_drm();
	struct drm_psb_private *dev_priv;
	const struct psb_offset *map;

	int pipe;

	if (!dev)
		return -ENODEV;
	dev_priv = dev->dev_private;
	if (!dev_priv)
		return -ENODEV;

	if (!gma_power_begin(dev, true))
		return -EIO;

	/* Flip whichever pipe actually drives the panel (pipe B on the Vaio). */
	pipe = gma500_flip_active_pipe(dev);
	if (pipe < 0) {
		gma_power_end(dev);
		return -ENODEV;
	}
	map = &dev_priv->regmap[pipe];

	/*
	 * Match gma_pipe_set_base(): on Poulsbo (IS_PSB) the linear-offset
	 * register IS the scanout base, so write the absolute GTT offset there.
	 * On the (non-PSB) surface-register parts, write surf = base address and
	 * keep the linear offset at 0.  Read back to post the write.
	 */
	if (IS_PSB(dev)) {
		REG_WRITE(map->base, gtt_offset);
		REG_READ(map->base);
	} else {
		REG_WRITE(map->base, 0);
		REG_READ(map->base);
		REG_WRITE(map->surf, gtt_offset);
		REG_READ(map->surf);
	}

	/* Optional vsync: block until the new base is being scanned out. */
	if (flip_vsync)
		gma500_flip_wait_vblank(dev, map);

	gma_power_end(dev);
	return 0;
}
EXPORT_SYMBOL(gma500_flip_to);
