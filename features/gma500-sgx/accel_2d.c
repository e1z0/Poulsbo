// SPDX-License-Identifier: GPL-2.0-only
/**************************************************************************
 * Copyright (c) 2007-2011, Intel Corporation.
 * All Rights Reserved.
 *
 * Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 * develop this driver.
 *
 **************************************************************************/

#include <linux/console.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/tty.h>

#include <drm/drm.h>
#include <drm/drm_crtc.h>
#include <drm/drm_fourcc.h>

#include "framebuffer.h"
#include "psb_drv.h"
#include "psb_reg.h"
#include "mmu.h"

static void psbfb_log_copyarea(struct drm_device *dev, const char *tag,
			       const struct fb_info *info,
			       const struct fb_copyarea *a,
			       unsigned int depth)
{
	dev_info_ratelimited(dev->dev,
			     "2D copyarea %s: sx=%u sy=%u dx=%u dy=%u w=%u h=%u depth=%u fb_flags=%#x\n",
			     tag, a->sx, a->sy, a->dx, a->dy, a->width,
			     a->height, depth, info->flags);
}

static void psbfb_log_imageblit(struct drm_device *dev, const char *tag,
				const struct fb_info *info,
				const struct fb_image *image,
				unsigned int fb_depth)
{
	dev_info_ratelimited(dev->dev,
			     "2D imageblit %s: dx=%u dy=%u w=%u h=%u image_depth=%u fb_depth=%u fg=%#x bg=%#x fb_flags=%#x\n",
			     tag, image->dx, image->dy, image->width,
			     image->height, image->depth, fb_depth,
			     image->fg_color, image->bg_color, info->flags);
}

static void psbfb_log_fillrect(struct drm_device *dev, const char *tag,
			       const struct fb_info *info,
			       const struct fb_fillrect *rect,
			       unsigned int fb_depth)
{
	dev_info_ratelimited(dev->dev,
			     "2D fillrect %s: dx=%u dy=%u w=%u h=%u color=%#x rop=%u fb_depth=%u fb_flags=%#x\n",
			     tag, rect->dx, rect->dy, rect->width,
			     rect->height, rect->color, rect->rop,
			     fb_depth, info->flags);
}

#define PSB_2D_ROP3_PATINVERT 0x5A5A

static u32 psbfb_resolve_pseudo_palette(const struct fb_info *info,
					const struct drm_framebuffer *fb,
					u32 color)
{
	if (!fb)
		return color;

	if (fb->format->depth <= 8)
		return color;

	if (!info->pseudo_palette || color >= 16)
		return color;

	switch (fb->format->cpp[0]) {
	case 2:
		return ((const u16 *)info->pseudo_palette)[color];
	case 4:
		return ((const u32 *)info->pseudo_palette)[color];
	default:
		return color;
	}
}

static bool psbfb_stage_mono_image(struct fb_info *info,
				   struct drm_framebuffer *fb,
				   struct gtt_range *backing,
				   const struct fb_image *image,
				   u32 scratch_x, u32 scratch_y,
				   u32 src_x, u32 width)
{
	struct psb_fbdev *fbdev = info->par;
	struct psb_framebuffer *psbfb = &fbdev->pfb;
	struct drm_device *dev = psbfb->base.dev;
	struct drm_psb_private *dev_priv = dev->dev_private;
	u8 *dst_base;
	const u8 *src = (const u8 *)image->data;
	u32 stride = fb->pitches[0];
	u32 cpp = fb->format->cpp[0];
	u32 fg;
	u32 bg;
	u32 row;
	u32 bytes_per_row;

	if (cpp != 2 && cpp != 4)
		return false;

	fg = psbfb_resolve_pseudo_palette(info, fb, image->fg_color);
	bg = psbfb_resolve_pseudo_palette(info, fb, image->bg_color);
	bytes_per_row = DIV_ROUND_UP(image->width, 8);
	dst_base = dev_priv->vram_addr + backing->offset +
		   scratch_y * stride + scratch_x * cpp;

	for (row = 0; row < image->height; row++) {
		const u8 *src_row = src + row * bytes_per_row;
		u8 *dst_row = dst_base + row * stride;
		u32 x;

		for (x = 0; x < width; x++) {
			u32 image_x = src_x + x;
			bool set = src_row[image_x >> 3] &
				   BIT(7 - (image_x & 7));

			if (cpp == 2)
				((u16 *)dst_row)[x] = set ? (u16)fg : (u16)bg;
			else
				((u32 *)dst_row)[x] = set ? fg : bg;
		}
	}

	wmb();
	return true;
}

static char *psbfb_lab_next_token(char **cursor)
{
	char *token;

	if (!cursor || !*cursor)
		return NULL;

	do {
		token = strsep(cursor, " \t");
	} while (token && !*token);

	return token;
}

static int psbfb_lab_next_u32(char **cursor, u32 *value)
{
	char *token = psbfb_lab_next_token(cursor);

	if (!token)
		return -EINVAL;

	return kstrtou32(token, 0, value);
}

static int psbfb_lab_sync_locked(struct fb_info *info)
{
	if (info->fbops && info->fbops->fb_sync)
		return info->fbops->fb_sync(info);
	return 0;
}

static void psbfb_lab_set_mono_bit(u8 *data, u32 stride, u32 x, u32 y)
{
	data[y * stride + (x >> 3)] |= BIT(7 - (x & 7));
}

static int psbfb_lab_build_mono_image(u8 *data, u32 width, u32 height,
				      const char *pattern)
{
	u32 stride = DIV_ROUND_UP(width, 8);
	u32 x;
	u32 y;

	memset(data, 0, stride * height);

	if (!pattern || !strcmp(pattern, "checker")) {
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				if ((((x >> 4) ^ (y >> 4)) & 1) == 0)
					psbfb_lab_set_mono_bit(data, stride, x, y);
			}
		}
		return 0;
	}

	if (!strcmp(pattern, "vertical")) {
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				if (((x >> 4) & 1) == 0)
					psbfb_lab_set_mono_bit(data, stride, x, y);
			}
		}
		return 0;
	}

	if (!strcmp(pattern, "horizontal")) {
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				if (((y >> 4) & 1) == 0)
					psbfb_lab_set_mono_bit(data, stride, x, y);
			}
		}
		return 0;
	}

	if (!strcmp(pattern, "diagonal")) {
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				if ((((x + y) >> 4) & 1) == 0)
					psbfb_lab_set_mono_bit(data, stride, x, y);
			}
		}
		return 0;
	}

	if (!strcmp(pattern, "solid")) {
		memset(data, 0xff, stride * height);
		return 0;
	}

	return -EINVAL;
}

int psbfb_run_lab_command(struct drm_device *dev, const char *buf, size_t count)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_fbdev *fbdev = dev_priv->fbdev;
	struct drm_framebuffer *fb;
	struct fb_info *info;
	struct fb_fillrect rect = { 0 };
	struct fb_copyarea area = { 0 };
	struct fb_image image = { 0 };
	char *cmdline;
	char *cursor;
	char *cmd;
	char *token;
	char snapshot_reason[PSB_SGX_SNAPSHOT_REASON_LEN];
	u8 *mono_data = NULL;
	int ret = 0;

	if (!fbdev)
		return -ENODEV;

	info = fbdev->pfb.fbdev;
	fb = fbdev->psb_fb_helper.fb;
	if (!info || !fb || !info->fbops)
		return -ENODEV;

	cmdline = kstrndup(buf, count, GFP_KERNEL);
	if (!cmdline)
		return -ENOMEM;

	cursor = strim(cmdline);
	if (!*cursor) {
		ret = -EINVAL;
		goto out_free;
	}

	cmd = psbfb_lab_next_token(&cursor);
	if (!cmd) {
		ret = -EINVAL;
		goto out_free;
	}
	scnprintf(snapshot_reason, sizeof(snapshot_reason), "lab:%s", cmd);

	console_lock();
	lock_fb_info(info);

	if (!strcmp(cmd, "fill")) {
		u32 color;

		ret = psbfb_lab_next_u32(&cursor, &rect.dx);
		if (ret)
			goto out_unlock;
		ret = psbfb_lab_next_u32(&cursor, &rect.dy);
		if (ret)
			goto out_unlock;
		ret = psbfb_lab_next_u32(&cursor, &rect.width);
		if (ret)
			goto out_unlock;
		ret = psbfb_lab_next_u32(&cursor, &rect.height);
		if (ret)
			goto out_unlock;
		ret = psbfb_lab_next_u32(&cursor, &color);
		if (ret)
			goto out_unlock;

		token = psbfb_lab_next_token(&cursor);
		if (!token || !strcmp(token, "copy"))
			rect.rop = ROP_COPY;
		else if (!strcmp(token, "xor"))
			rect.rop = ROP_XOR;
		else {
			ret = -EINVAL;
			goto out_unlock;
		}

		rect.color = color;
		dev_info(dev->dev,
			 "lab_2d fill: dx=%u dy=%u w=%u h=%u color=%#x rop=%u\n",
			 rect.dx, rect.dy, rect.width, rect.height,
			 rect.color, rect.rop);
		info->fbops->fb_fillrect(info, &rect);
		ret = psbfb_lab_sync_locked(info);
	} else if (!strcmp(cmd, "clear")) {
		u32 color;

		ret = psbfb_lab_next_u32(&cursor, &color);
		if (ret)
			goto out_unlock;

		rect.dx = 0;
		rect.dy = 0;
		rect.width = fb->width;
		rect.height = fb->height;
		rect.color = color;
		rect.rop = ROP_COPY;

		dev_info(dev->dev,
			 "lab_2d clear: w=%u h=%u color=%#x\n",
			 rect.width, rect.height, rect.color);
		info->fbops->fb_fillrect(info, &rect);
		ret = psbfb_lab_sync_locked(info);
	} else if (!strcmp(cmd, "copy")) {
		ret = psbfb_lab_next_u32(&cursor, &area.sx);
		if (ret)
			goto out_unlock;
		ret = psbfb_lab_next_u32(&cursor, &area.sy);
		if (ret)
			goto out_unlock;
		ret = psbfb_lab_next_u32(&cursor, &area.dx);
		if (ret)
			goto out_unlock;
		ret = psbfb_lab_next_u32(&cursor, &area.dy);
		if (ret)
			goto out_unlock;
		ret = psbfb_lab_next_u32(&cursor, &area.width);
		if (ret)
			goto out_unlock;
		ret = psbfb_lab_next_u32(&cursor, &area.height);
		if (ret)
			goto out_unlock;

		dev_info(dev->dev,
			 "lab_2d copy: sx=%u sy=%u dx=%u dy=%u w=%u h=%u\n",
			 area.sx, area.sy, area.dx, area.dy,
			 area.width, area.height);
		info->fbops->fb_copyarea(info, &area);
		ret = psbfb_lab_sync_locked(info);
	} else if (!strcmp(cmd, "mono")) {
		u32 fg;
		u32 bg;
		u32 stride;

		ret = psbfb_lab_next_u32(&cursor, &image.dx);
		if (ret)
			goto out_unlock;
		ret = psbfb_lab_next_u32(&cursor, &image.dy);
		if (ret)
			goto out_unlock;
		ret = psbfb_lab_next_u32(&cursor, &image.width);
		if (ret)
			goto out_unlock;
		ret = psbfb_lab_next_u32(&cursor, &image.height);
		if (ret)
			goto out_unlock;
		ret = psbfb_lab_next_u32(&cursor, &fg);
		if (ret)
			goto out_unlock;
		ret = psbfb_lab_next_u32(&cursor, &bg);
		if (ret)
			goto out_unlock;

		token = psbfb_lab_next_token(&cursor);
		if (!token)
			token = "checker";

		stride = DIV_ROUND_UP(image.width, 8);
		mono_data = kcalloc(stride, image.height, GFP_KERNEL);
		if (!mono_data) {
			ret = -ENOMEM;
			goto out_unlock;
		}

		ret = psbfb_lab_build_mono_image(mono_data, image.width,
						 image.height, token);
		if (ret)
			goto out_unlock;

		image.depth = 1;
		image.fg_color = fg;
		image.bg_color = bg;
		image.data = mono_data;

		dev_info(dev->dev,
			 "lab_2d mono: dx=%u dy=%u w=%u h=%u fg=%#x bg=%#x pattern=%s\n",
			 image.dx, image.dy, image.width, image.height,
			 image.fg_color, image.bg_color, token);
		info->fbops->fb_imageblit(info, &image);
		ret = psbfb_lab_sync_locked(info);
	} else if (!strcmp(cmd, "sync")) {
		dev_info(dev->dev, "lab_2d sync\n");
		ret = psbfb_lab_sync_locked(info);
	} else {
		ret = -EINVAL;
	}

out_unlock:
	unlock_fb_info(info);
	console_unlock();
	if (!ret)
		psb_sgx_capture_state(dev, snapshot_reason, true);
out_free:
	kfree(mono_data);
	kfree(cmdline);
	return ret;
}

/**
 *	psb_spank		-	reset the 2D engine
 *	@dev_priv: our PSB DRM device
 *
 *	Soft reset the graphics engine and then reload the necessary registers.
 *	We use this at initialisation time but it will become relevant for
 *	accelerated X later
 *
 *	Must be atomic-safe: every caller that can race the live FIFO holds
 *	lock_2d with IRQs off (psb_2d_wait_available() here, the
 *	DRM_PSB_BLT_SUBMIT reset path in blt_submit.c), hence mdelay()
 *	rather than msleep().
 */
void psb_spank(struct drm_psb_private *dev_priv)
{
	dev_warn(dev_priv->dev->dev, "2D engine reset\n");
	PSB_WSGX32(_PSB_CS_RESET_BIF_RESET | _PSB_CS_RESET_DPM_RESET |
		_PSB_CS_RESET_TA_RESET | _PSB_CS_RESET_USE_RESET |
		_PSB_CS_RESET_ISP_RESET | _PSB_CS_RESET_TSP_RESET |
		_PSB_CS_RESET_TWOD_RESET, PSB_CR_SOFT_RESET);
	PSB_RSGX32(PSB_CR_SOFT_RESET);

	mdelay(1);

	PSB_WSGX32(0, PSB_CR_SOFT_RESET);
	wmb();
	PSB_WSGX32(PSB_RSGX32(PSB_CR_BIF_CTRL) | _PSB_CB_CTRL_CLEAR_FAULT,
		   PSB_CR_BIF_CTRL);
	wmb();
	(void) PSB_RSGX32(PSB_CR_BIF_CTRL);

	mdelay(1);
	PSB_WSGX32(PSB_RSGX32(PSB_CR_BIF_CTRL) & ~_PSB_CB_CTRL_CLEAR_FAULT,
		   PSB_CR_BIF_CTRL);
	(void) PSB_RSGX32(PSB_CR_BIF_CTRL);
	PSB_WSGX32(dev_priv->gtt.gatt_start, PSB_CR_BIF_TWOD_REQ_BASE);
}

/**
 *	psb2_2d_wait_available	-	wait for FIFO room
 *	@dev_priv: our DRM device
 *	@size: size (in dwords) of the command we want to issue
 *
 *	Wait until there is room to load the FIFO with our data. If the
 *	device is not responding then reset it
 */
static int psb_2d_wait_available(struct drm_psb_private *dev_priv,
			  unsigned size)
{
	uint32_t avail = PSB_RSGX32(PSB_CR_2D_SOCIF);
	unsigned long t = jiffies + HZ;

	while (avail < size) {
		avail = PSB_RSGX32(PSB_CR_2D_SOCIF);
		if (time_after(jiffies, t)) {
			dev_err(dev_priv->dev->dev,
				"2D wait timeout: need=%u avail=%#08x blit_status=%#08x\n",
				size, avail, PSB_RSGX32(PSB_CR_2D_BLIT_STATUS));
			psb_spank(dev_priv);
			return -EIO;
		}
	}
	return 0;
}

/**
 *	psb_2d_submit		-	submit a 2D command
 *	@dev_priv: our DRM device
 *	@cmdbuf: command to issue
 *	@size: length (in dwords)
 *
 *	Issue one or more 2D commands to the accelerator. This needs to be
 *	serialized later when we add the GEM interfaces for acceleration
 */
static int psbfb_2d_submit(struct drm_psb_private *dev_priv, uint32_t *cmdbuf,
								unsigned size)
{
	int ret = 0;
	int i;
	unsigned submit_size;
	unsigned long flags;

	spin_lock_irqsave(&dev_priv->lock_2d, flags);
	while (size > 0) {
		submit_size = (size < 0x60) ? size : 0x60;
		size -= submit_size;
		ret = psb_2d_wait_available(dev_priv, submit_size);
		if (ret)
			break;

		submit_size <<= 2;

		for (i = 0; i < submit_size; i += 4)
			PSB_WSGX32(*cmdbuf++, PSB_SGX_2D_SLAVE_PORT + i);

		(void)PSB_RSGX32(PSB_SGX_2D_SLAVE_PORT + i - 4);
	}
	spin_unlock_irqrestore(&dev_priv->lock_2d, flags);
	return ret;
}

/**
 *	psb_2d_submit_user	-	submit a validated user 2D command
 *	@dev_priv: our DRM device
 *	@cmdbuf: command to issue
 *	@size: length (in dwords)
 *
 *	Entry point for the DRM_PSB_BLT_SUBMIT ioctl (blt_submit.c).
 *	Mirrors psbfb_2d_submit() above (same lock_2d serialization
 *	against the live fbcon acceleration, same <= 0x60 dword chunking)
 *	with one deliberate difference: on a FIFO-space timeout it simply
 *	returns -EIO instead of going through psb_2d_wait_available(),
 *	whose psb_spank() call would delay and printk to the fbcon while
 *	lock_2d is held with IRQs off.  The ioctl caller detects the hang
 *	and resets the engine from process context instead.
 *
 *	The FIFO-room wait runs before each chunk is written, and ioctl
 *	streams are capped at DRM_PSB_BLT_MAX_DWORDS == 0x60 (a single
 *	chunk), so a timeout here means no dwords were written: a bail
 *	can never leave a truncated stream in the FIFO.
 */
int psb_2d_submit_user(struct drm_psb_private *dev_priv, uint32_t *cmdbuf,
		       unsigned size)
{
	unsigned long flags;
	int ret = 0;
	int i;
	unsigned submit_size;

	/*
	 * NOTE: 2D/SGX BIF coexistence is NOT solved here -- see the
	 * 2d-sgx-bif-conflict memory.  While SGX is initialised, the shared BIF
	 * routes the 2D requestor through the SGX MMU and this direct-MMIO path
	 * wedges the core, so the DDX runs 2D on the CPU (software-2D mode) and
	 * never calls this.  With SGX off, this path works (blt-ioctl 12/12).
	 */
	spin_lock_irqsave(&dev_priv->lock_2d, flags);
	while (size > 0) {
		unsigned long stop = jiffies + HZ;

		submit_size = (size < 0x60) ? size : 0x60;

		/* Bounded wait for FIFO room: never spank under lock_2d */
		while (PSB_RSGX32(PSB_CR_2D_SOCIF) < submit_size) {
			if (time_after(jiffies, stop)) {
				ret = -EIO;
				goto out_unlock;
			}
			cpu_relax();
		}

		size -= submit_size;
		submit_size <<= 2;

		for (i = 0; i < submit_size; i += 4)
			PSB_WSGX32(*cmdbuf++, PSB_SGX_2D_SLAVE_PORT + i);

		(void)PSB_RSGX32(PSB_SGX_2D_SLAVE_PORT + i - 4);
	}
out_unlock:
	spin_unlock_irqrestore(&dev_priv->lock_2d, flags);
	return ret;
}


/**
 *	psb_accel_2d_copy_direction	-	compute blit order
 *	@xdir: X direction of move
 *	@ydir: Y direction of move
 *
 *	Compute the correct order setings to ensure that an overlapping blit
 *	correctly copies all the pixels.
 */
static u32 psb_accel_2d_copy_direction(int xdir, int ydir)
{
	if (xdir < 0)
		return (ydir < 0) ? PSB_2D_COPYORDER_BR2TL :
						PSB_2D_COPYORDER_TR2BL;
	else
		return (ydir < 0) ? PSB_2D_COPYORDER_BL2TR :
						PSB_2D_COPYORDER_TL2BR;
}

/**
 *	psb_accel_2d_copy		-	accelerated 2D copy
 *	@dev_priv: our DRM device
 *	@src_offset in bytes
 *	@src_stride in bytes
 *	@src_format psb 2D format defines
 *	@dst_offset in bytes
 *	@dst_stride in bytes
 *	@dst_format psb 2D format defines
 *	@src_x offset in pixels
 *	@src_y offset in pixels
 *	@dst_x offset in pixels
 *	@dst_y offset in pixels
 *	@size_x of the copied area
 *	@size_y of the copied area
 *
 *	Format and issue a 2D accelerated copy command.
 */
static int psb_accel_2d_copy(struct drm_psb_private *dev_priv,
			     uint32_t src_offset, uint32_t src_stride,
			     uint32_t src_format, uint32_t dst_offset,
			     uint32_t dst_stride, uint32_t dst_format,
			     uint16_t src_x, uint16_t src_y,
			     uint16_t dst_x, uint16_t dst_y,
			     uint16_t size_x, uint16_t size_y)
{
	uint32_t blit_cmd;
	uint32_t buffer[10];
	uint32_t *buf;
	uint32_t direction;

	buf = buffer;

	direction =
	    psb_accel_2d_copy_direction(src_x - dst_x, src_y - dst_y);

	if (direction == PSB_2D_COPYORDER_BR2TL ||
	    direction == PSB_2D_COPYORDER_TR2BL) {
		src_x += size_x - 1;
		dst_x += size_x - 1;
	}
	if (direction == PSB_2D_COPYORDER_BR2TL ||
	    direction == PSB_2D_COPYORDER_BL2TR) {
		src_y += size_y - 1;
		dst_y += size_y - 1;
	}

	blit_cmd =
	    PSB_2D_BLIT_BH |
	    PSB_2D_ROT_NONE |
	    PSB_2D_DSTCK_DISABLE |
	    PSB_2D_SRCCK_DISABLE |
	    PSB_2D_USE_PAT | PSB_2D_ROP3_SRCCOPY | direction;

	*buf++ = PSB_2D_FENCE_BH;
	*buf++ =
	    PSB_2D_DST_SURF_BH | dst_format | (dst_stride <<
					       PSB_2D_DST_STRIDE_SHIFT);
	*buf++ = dst_offset;
	*buf++ =
	    PSB_2D_SRC_SURF_BH | src_format | (src_stride <<
					       PSB_2D_SRC_STRIDE_SHIFT);
	*buf++ = src_offset;
	*buf++ =
	    PSB_2D_SRC_OFF_BH | (src_x << PSB_2D_SRCOFF_XSTART_SHIFT) |
	    (src_y << PSB_2D_SRCOFF_YSTART_SHIFT);
	*buf++ = blit_cmd;
	*buf++ =
	    (dst_x << PSB_2D_DST_XSTART_SHIFT) | (dst_y <<
						  PSB_2D_DST_YSTART_SHIFT);
	*buf++ =
	    (size_x << PSB_2D_DST_XSIZE_SHIFT) | (size_y <<
						  PSB_2D_DST_YSIZE_SHIFT);
	*buf++ = PSB_2D_FLUSH_BH;

	return psbfb_2d_submit(dev_priv, buffer, buf - buffer);
}

static int psb_accel_2d_fill(struct drm_psb_private *dev_priv,
			     u32 dst_offset, u32 dst_stride, u32 dst_format,
			     u32 fill_color, u16 dst_x, u16 dst_y,
			     u16 size_x, u16 size_y, u16 rop3)
{
	u32 buffer[8];
	u32 *buf = buffer;
	u32 blit_cmd;

	blit_cmd = PSB_2D_BLIT_BH |
		   PSB_2D_ROT_NONE |
		   PSB_2D_DSTCK_DISABLE |
		   PSB_2D_SRCCK_DISABLE |
		   PSB_2D_USE_FILL | rop3;

	*buf++ = PSB_2D_FENCE_BH;
	*buf++ = PSB_2D_DST_SURF_BH | dst_format |
		 (dst_stride << PSB_2D_DST_STRIDE_SHIFT);
	*buf++ = dst_offset;
	*buf++ = blit_cmd;
	*buf++ = fill_color;
	*buf++ = (dst_x << PSB_2D_DST_XSTART_SHIFT) |
		 (dst_y << PSB_2D_DST_YSTART_SHIFT);
	*buf++ = (size_x << PSB_2D_DST_XSIZE_SHIFT) |
		 (size_y << PSB_2D_DST_YSIZE_SHIFT);
	*buf++ = PSB_2D_FLUSH_BH;

	return psbfb_2d_submit(dev_priv, buffer, buf - buffer);
}

/**
 *	psbfb_copyarea_accel	-	copyarea acceleration for /dev/fb
 *	@info: our framebuffer
 *	@a: copyarea parameters from the framebuffer core
 *
 *	Perform a 2D copy via the accelerator
 */
static void psbfb_copyarea_accel(struct fb_info *info,
				 const struct fb_copyarea *a)
{
	struct psb_fbdev *fbdev = info->par;
	struct psb_framebuffer *psbfb = &fbdev->pfb;
	struct drm_device *dev = psbfb->base.dev;
	struct drm_framebuffer *fb = fbdev->psb_fb_helper.fb;
	struct drm_psb_private *dev_priv = dev->dev_private;
	uint32_t offset;
	uint32_t stride;
	uint32_t src_format;
	uint32_t dst_format;

	if (!fb)
		return;

	offset = to_gtt_range(fb->obj[0])->offset;
	stride = fb->pitches[0];

	switch (fb->format->depth) {
	case 8:
		src_format = PSB_2D_SRC_332RGB;
		dst_format = PSB_2D_DST_332RGB;
		break;
	case 15:
		src_format = PSB_2D_SRC_555RGB;
		dst_format = PSB_2D_DST_555RGB;
		break;
	case 16:
		src_format = PSB_2D_SRC_565RGB;
		dst_format = PSB_2D_DST_565RGB;
		break;
	case 24:
	case 32:
		/* this is wrong but since we don't do blending its okay */
		src_format = PSB_2D_SRC_8888ARGB;
		dst_format = PSB_2D_DST_8888ARGB;
		break;
	default:
		/* software fallback */
		psbfb_log_copyarea(dev, "fallback-depth", info, a,
				   fb->format->depth);
		drm_fb_helper_cfb_copyarea(info, a);
		return;
	}

	if (!gma_power_begin(dev, false)) {
		psbfb_log_copyarea(dev, "fallback-power", info, a,
				   fb->format->depth);
		drm_fb_helper_cfb_copyarea(info, a);
		return;
	}
	psbfb_log_copyarea(dev, "submit", info, a, fb->format->depth);
	psb_accel_2d_copy(dev_priv,
			  offset, stride, src_format,
			  offset, stride, dst_format,
			  a->sx, a->sy, a->dx, a->dy, a->width, a->height);
	gma_power_end(dev);
}

/**
 *	psbfb_copyarea	-	2D copy interface
 *	@info: our framebuffer
 *	@region: region to copy
 *
 *	Copy an area of the framebuffer console either by the accelerator
 *	or directly using the cfb helpers according to the request
 */
void psbfb_copyarea(struct fb_info *info,
			   const struct fb_copyarea *region)
{
	if (unlikely(info->state != FBINFO_STATE_RUNNING))
		return;

	/* Avoid the 8 pixel erratum */
	if (region->width == 8 || region->height == 8 ||
		(info->flags & FBINFO_HWACCEL_DISABLED)) {
		struct psb_fbdev *fbdev = info->par;
		struct psb_framebuffer *psbfb = &fbdev->pfb;
		struct drm_device *dev = psbfb->base.dev;
		unsigned int depth = 0;

		if (fbdev->psb_fb_helper.fb)
			depth = fbdev->psb_fb_helper.fb->format->depth;
		psbfb_log_copyarea(dev, "fallback-guard", info, region, depth);
		return drm_fb_helper_cfb_copyarea(info, region);
	}

	psbfb_copyarea_accel(info, region);
}

void psbfb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
	struct psb_fbdev *fbdev = info->par;
	struct psb_framebuffer *psbfb = &fbdev->pfb;
	struct drm_device *dev = psbfb->base.dev;
	struct drm_framebuffer *fb = fbdev->psb_fb_helper.fb;
	struct drm_psb_private *dev_priv = dev->dev_private;
	u32 offset;
	u32 stride;
	u32 dst_format;
	u32 rop3;
	u32 fill_color;

	if (unlikely(info->state != FBINFO_STATE_RUNNING))
		return;

	if (!fb) {
		drm_fb_helper_cfb_fillrect(info, rect);
		return;
	}

	if (!rect->width || !rect->height)
		return;

	if (rect->dx + rect->width > fb->width ||
	    rect->dy + rect->height > fb->height) {
		psbfb_log_fillrect(dev, "fallback-bounds", info, rect,
				   fb->format->depth);
		return drm_fb_helper_cfb_fillrect(info, rect);
	}

	if (rect->width == 8 || rect->height == 8 ||
	    (info->flags & FBINFO_HWACCEL_DISABLED)) {
		psbfb_log_fillrect(dev, "fallback-guard", info, rect,
				   fb->format->depth);
		return drm_fb_helper_cfb_fillrect(info, rect);
	}

	switch (fb->format->depth) {
	case 15:
		dst_format = PSB_2D_DST_555RGB;
		break;
	case 16:
		dst_format = PSB_2D_DST_565RGB;
		break;
	case 24:
	case 32:
		dst_format = PSB_2D_DST_8888ARGB;
		break;
	default:
		psbfb_log_fillrect(dev, "fallback-format", info, rect,
				   fb->format->depth);
		return drm_fb_helper_cfb_fillrect(info, rect);
	}

	switch (rect->rop) {
	case ROP_COPY:
		rop3 = PSB_2D_ROP3_PATCOPY;
		break;
	case ROP_XOR:
		rop3 = PSB_2D_ROP3_PATINVERT;
		break;
	default:
		psbfb_log_fillrect(dev, "fallback-rop", info, rect,
				   fb->format->depth);
		return drm_fb_helper_cfb_fillrect(info, rect);
	}

	offset = to_gtt_range(fb->obj[0])->offset;
	stride = fb->pitches[0];
	fill_color = psbfb_resolve_pseudo_palette(info, fb, rect->color);

	if (!gma_power_begin(dev, false)) {
		psbfb_log_fillrect(dev, "fallback-power", info, rect,
				   fb->format->depth);
		return drm_fb_helper_cfb_fillrect(info, rect);
	}

	psbfb_log_fillrect(dev, "submit", info, rect, fb->format->depth);
	psb_accel_2d_fill(dev_priv, offset, stride, dst_format, fill_color,
			  rect->dx, rect->dy, rect->width, rect->height, rop3);
	gma_power_end(dev);
}

void psbfb_imageblit(struct fb_info *info, const struct fb_image *image)
{
	struct psb_fbdev *fbdev = info->par;
	struct psb_framebuffer *psbfb = &fbdev->pfb;
	struct drm_device *dev = psbfb->base.dev;
	struct drm_framebuffer *fb = fbdev->psb_fb_helper.fb;
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct gtt_range *backing;
	u32 offset;
	u32 stride;
	u32 pitch_pixels;
	u32 scratch_width;
	u32 scratch_x;
	u32 scratch_y = 0;
	u32 src_format;
	u32 dst_format;

	if (unlikely(info->state != FBINFO_STATE_RUNNING))
		return;

	if (!fb || !image || !image->data)
		return drm_fb_helper_cfb_imageblit(info, image);

	if (image->depth != 1) {
		psbfb_log_imageblit(dev, "fallback-depth", info, image,
				    fb->format->depth);
		return drm_fb_helper_cfb_imageblit(info, image);
	}

	backing = to_gtt_range(fb->obj[0]);
	offset = backing->offset;
	stride = fb->pitches[0];
	pitch_pixels = stride / fb->format->cpp[0];
	scratch_x = fb->width;
	scratch_width = pitch_pixels - scratch_x;

	if (fb->format->cpp[0] != 2 && fb->format->cpp[0] != 4) {
		psbfb_log_imageblit(dev, "fallback-cpp", info, image,
				    fb->format->depth);
		return drm_fb_helper_cfb_imageblit(info, image);
	}

	if (pitch_pixels <= scratch_x ||
	    image->height > fb->height ||
	    image->dx + image->width > fb->width ||
	    image->dy + image->height > fb->height) {
		psbfb_log_imageblit(dev, "fallback-bounds", info, image,
				    fb->format->depth);
		return drm_fb_helper_cfb_imageblit(info, image);
	}

	switch (fb->format->depth) {
	case 15:
		src_format = PSB_2D_SRC_555RGB;
		dst_format = PSB_2D_DST_555RGB;
		break;
	case 16:
		src_format = PSB_2D_SRC_565RGB;
		dst_format = PSB_2D_DST_565RGB;
		break;
	case 24:
	case 32:
		src_format = PSB_2D_SRC_8888ARGB;
		dst_format = PSB_2D_DST_8888ARGB;
		break;
	default:
		psbfb_log_imageblit(dev, "fallback-format", info, image,
				    fb->format->depth);
		return drm_fb_helper_cfb_imageblit(info, image);
	}

	if (!gma_power_begin(dev, false)) {
		psbfb_log_imageblit(dev, "fallback-power", info, image,
				    fb->format->depth);
		return drm_fb_helper_cfb_imageblit(info, image);
	}

	psbfb_log_imageblit(dev,
			    image->width > scratch_width ? "submit-chunked" :
			    "submit",
			    info, image, fb->format->depth);

	{
		u32 submitted = 0;

		while (submitted < image->width) {
			u32 chunk = min(scratch_width, image->width - submitted);

			if (!psbfb_stage_mono_image(info, fb, backing, image,
						     scratch_x, scratch_y,
						     submitted, chunk)) {
				gma_power_end(dev);
				psbfb_log_imageblit(dev, "fallback-stage",
						    info, image,
						    fb->format->depth);
				return drm_fb_helper_cfb_imageblit(info, image);
			}

			psb_accel_2d_copy(dev_priv, offset, stride, src_format,
					  offset, stride, dst_format,
					  scratch_x, scratch_y,
					  image->dx + submitted, image->dy,
					  chunk, image->height);
			submitted += chunk;
		}
	}

	gma_power_end(dev);
}

/**
 *	psbfb_sync	-	synchronize 2D
 *	@info: our framebuffer
 *
 *	Wait for the 2D engine to quiesce so that we can do CPU
 *	access to the framebuffer again
 */
int psbfb_sync(struct fb_info *info)
{
	struct psb_fbdev *fbdev = info->par;
	struct psb_framebuffer *psbfb = &fbdev->pfb;
	struct drm_device *dev = psbfb->base.dev;
	struct drm_psb_private *dev_priv = dev->dev_private;
	unsigned long _end = jiffies + HZ;
	int busy = 0;
	unsigned long flags;

	spin_lock_irqsave(&dev_priv->lock_2d, flags);
	/*
	 * First idle the 2D engine.
	 */

	if ((PSB_RSGX32(PSB_CR_2D_SOCIF) == _PSB_C2_SOCIF_EMPTY) &&
	    ((PSB_RSGX32(PSB_CR_2D_BLIT_STATUS) & _PSB_C2B_STATUS_BUSY) == 0))
		goto out;

	do {
		busy = (PSB_RSGX32(PSB_CR_2D_SOCIF) != _PSB_C2_SOCIF_EMPTY);
		cpu_relax();
	} while (busy && !time_after_eq(jiffies, _end));

	if (busy)
		busy = (PSB_RSGX32(PSB_CR_2D_SOCIF) != _PSB_C2_SOCIF_EMPTY);
	if (busy)
		goto out;

	do {
		busy = ((PSB_RSGX32(PSB_CR_2D_BLIT_STATUS) &
						_PSB_C2B_STATUS_BUSY) != 0);
		cpu_relax();
	} while (busy && !time_after_eq(jiffies, _end));
	if (busy)
		busy = ((PSB_RSGX32(PSB_CR_2D_BLIT_STATUS) &
					_PSB_C2B_STATUS_BUSY) != 0);

out:
	spin_unlock_irqrestore(&dev_priv->lock_2d, flags);
	return (busy) ? -EBUSY : 0;
}
