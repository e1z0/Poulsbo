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
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <drm/drm_vblank.h>
#include "psb_drv.h"		/* pulls in gtt.h, power.h, REG_*/IS_PSB, dev_priv */
#include "psb_intel_reg.h"	/* PIPEACONF / PIPEASTAT / vblank status bits */
#include "gma500_flip.h"

/*
 * flip_vsync - when non-zero, a zero-copy page-flip present completes only at
 * the next vblank instead of immediately.  This throttles GL to the panel
 * refresh and presents tear-free -- WITHOUT busy-waiting: gma500_flip_to() just
 * arms the new scanout base (the display latches it at the next frame start),
 * and the DC flip command is completed ASYNCHRONOUSLY from the vblank interrupt
 * (gma500_flip_defer_complete -> gma500_flip_vblank_notify -> a workqueue), so
 * the SGX MISR workqueue is never blocked and throughput stays at the refresh
 * rate rather than collapsing.  Default 0 = present as soon as the buffer is
 * armed (lowest latency, a fast full-screen app may tear).  Runtime-writable:
 *   /sys/module/gma500_gfx/parameters/flip_vsync
 */
static int flip_vsync;
module_param(flip_vsync, int, 0644);
MODULE_PARM_DESC(flip_vsync,
	"Throttle presents to ~refresh + reduce tearing via a deferred, "
	"non-MISR-blocking completion (0=off/max-rate [default], 1=on/throttled)");

int gma500_flip_vsync_on(void)
{
	return flip_vsync;
}
EXPORT_SYMBOL(gma500_flip_vsync_on);

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
 * Deferred (refresh-throttled) flip completion.
 *
 * gma500dc arms the flip (gma500_flip_to writes the scanout base, which the
 * display hardware latches at the next frame start -- so the flip itself is
 * tear-free) then, in vsync mode, hands us the DC command's completion via
 * gma500_flip_defer_complete() instead of completing it inline.  We complete it
 * one refresh period later from a delayed workqueue, so eglSwapBuffers throttles
 * to the panel refresh (~60 Hz) instead of spinning as fast as the SGX can
 * render -- WITHOUT blocking the SGX MISR (the completion runs on its own
 * worker) and without touching the display vblank IRQ (which is not reliably
 * usable for the fbcon CRTC here, and whose enable/disable churn destabilised
 * the box).  A timer is phase-free relative to vblank but caps the present rate
 * and, combined with the hardware's vblank-latched base, gives smooth
 * tear-reduced output.  Single flip in flight (2-buffer chain), so one slot.
 */
#define GMA500_FLIP_REFRESH_MS 16	/* ~60 Hz present cap */

static struct gma_flip_defer {
	spinlock_t          lock;
	void              (*cb)(void *);
	void               *data;
	bool                valid;
	bool                inited;
	struct delayed_work work;
} g_flip_defer;

/* Complete the pending flip (once); the delayed worker's sole job. */
static void gma500_flip_complete_work(struct work_struct *w)
{
	void (*cb)(void *) = NULL;
	void *data = NULL;
	unsigned long f;

	spin_lock_irqsave(&g_flip_defer.lock, f);
	if (g_flip_defer.valid) {
		cb   = g_flip_defer.cb;
		data = g_flip_defer.data;
		g_flip_defer.valid = false;
	}
	spin_unlock_irqrestore(&g_flip_defer.lock, f);

	if (cb)
		cb(data);			/* pfnPVRSRVCmdComplete (process ctx) */
}

/* One-time init of the deferred-completion state.  Called from
 * psb_driver_load() once gma500_drm_dev is published. */
void gma500_flip_init(void)
{
	if (g_flip_defer.inited)
		return;
	spin_lock_init(&g_flip_defer.lock);
	INIT_DELAYED_WORK(&g_flip_defer.work, gma500_flip_complete_work);
	g_flip_defer.inited = true;
}
EXPORT_SYMBOL(gma500_flip_init);

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

	/*
	 * Arming the base is all we do here; the display latches it at the next
	 * frame start (tear-free).  In vsync mode gma500dc defers the DC command
	 * completion to that vblank via gma500_flip_defer_complete() so the
	 * present throttles to the refresh rate without blocking the MISR.
	 */
	gma_power_end(dev);
	return 0;
}
EXPORT_SYMBOL(gma500_flip_to);

/**
 * gma500_flip_defer_complete - complete @cb(@data) ~one refresh period later
 * @cb:   completion callback (e.g. the DC's pfnPVRSRVCmdComplete wrapper)
 * @data: opaque argument passed to @cb
 *
 * Schedules @cb to run, in process context, ~one refresh period from now, so
 * eglSwapBuffers throttles to the panel refresh instead of spinning.  Returns 0
 * if deferred (caller must NOT complete it itself) or -errno if it could not be
 * (services not up, or one already pending) -- caller then completes inline.
 * Does not block the SGX MISR and does not touch the display vblank IRQ.
 */
int gma500_flip_defer_complete(void (*cb)(void *), void *data)
{
	unsigned long f;

	if (!cb || !g_flip_defer.inited)
		return -ENODEV;

	spin_lock_irqsave(&g_flip_defer.lock, f);
	if (g_flip_defer.valid) {		/* a flip is already pending */
		spin_unlock_irqrestore(&g_flip_defer.lock, f);
		return -EBUSY;
	}
	g_flip_defer.cb    = cb;
	g_flip_defer.data  = data;
	g_flip_defer.valid = true;
	spin_unlock_irqrestore(&g_flip_defer.lock, f);

	mod_delayed_work(system_wq, &g_flip_defer.work,
			 msecs_to_jiffies(GMA500_FLIP_REFRESH_MS));
	return 0;
}
EXPORT_SYMBOL(gma500_flip_defer_complete);

/**
 * gma500_flip_vblank_disable - flush any pending deferred completion.
 *
 * Called on swap-chain teardown to complete/cancel an in-flight deferred flip
 * so no completion callback fires into a torn-down swap chain.
 */
void gma500_flip_vblank_disable(void)
{
	unsigned long f;

	if (!g_flip_defer.inited)
		return;

	spin_lock_irqsave(&g_flip_defer.lock, f);
	g_flip_defer.valid = false;
	spin_unlock_irqrestore(&g_flip_defer.lock, f);

	cancel_delayed_work_sync(&g_flip_defer.work);
}
EXPORT_SYMBOL(gma500_flip_vblank_disable);
