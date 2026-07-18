/*
 * gma500blt_dri2.c - DRI2 support for the GMA500 open DDX, so the closed PVR
 * DRI2 driver (emgd_dri.so) provides hardware-accelerated desktop OpenGL / GLX.
 *
 * Architecture (see memory glx-dri2-effort):
 *   - We advertise DRI2 driverName "emgd"; Mesa's libGL then loads
 *     /usr/lib/<triplet>/dri/emgd_dri.so as the client GL driver.  emgd_dri.so
 *     renders via libEMGDOGL on the SGX (reaching pvrsrvkm through the gma500
 *     PVR-DRM bridge on this same card0 fd) into buffers it maps by GEM flink
 *     name.
 *   - DRI2 buffers here are plain gma500 GEM dumb buffers (CREATE_DUMB); we hand
 *     the client their global flink name.  The client imports them (PVR2DMemMap
 *     of the flink'd handle) and renders into them.
 *   - DRI2CopyRegion blits the client's back buffer into the window's front
 *     buffer using the SGX 2D BLT engine (the same GMA_BLT_SUBMIT path EXA uses).
 *
 * This is the server (DDX) half of GLX; the DRI2 extension itself is built into
 * the Xorg 1.20 server (DRI2ScreenInit).
 */
#include <sys/ioctl.h>
#include <errno.h>
#include <xorg-server.h>
#include <xf86.h>
#include <xf86drm.h>
#include <dri2.h>
#include <fb.h>
#include "gma500blt.h"
#include "gma_blt_drm.h"

/* PSB 2D-core opcodes (must match gma500blt_exa.c / the proven blt-ioctl). */
#define P2D_FENCE_BH	0x70000000u
#define P2D_BLIT_BH	0x80000000u
#define P2D_DST_SURF_BH	0xA0000000u
#define P2D_SRC_SURF_BH	0xC0000000u
#define P2D_FLUSH_BH	0xF0000000u
#define P2D_USE_PAT	0x00000010u
#define P2D_ROP3_SRCCOPY 0x0000CCCCu
#define P2D_FMT_8888	0x00060000u	/* ARGB8888 surface format */

/* Per-DRI2-buffer private: the backing gma500 GEM dumb buffer. */
typedef struct {
	uint32_t handle;	/* GEM handle (this DDX's fd)          */
	uint32_t name;		/* GEM global (flink) name for clients */
	uint32_t pitch;		/* bytes                               */
	uint32_t size;
	int      refcnt;
} GmaDri2Buf;

static uint32_t gma_gem_create(int fd, int w, int h, int cpp,
			       uint32_t *pitch, uint32_t *size)
{
	struct drm_mode_create_dumb c;
	memset(&c, 0, sizeof c);
	c.width = w; c.height = h; c.bpp = cpp * 8;
	if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &c) != 0)
		return 0;
	*pitch = c.pitch; *size = c.size;
	return c.handle;
}

static uint32_t gma_gem_flink(int fd, uint32_t handle)
{
	struct drm_gem_flink f;
	memset(&f, 0, sizeof f);
	f.handle = handle;
	if (drmIoctl(fd, DRM_IOCTL_GEM_FLINK, &f) != 0)
		return 0;
	return f.name;
}

static void gma_gem_close(int fd, uint32_t handle)
{
	struct drm_gem_close cl;
	memset(&cl, 0, sizeof cl);
	cl.handle = handle;
	drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &cl);
}

/* DRI2 CreateBuffer: allocate a GEM buffer for the requested attachment and
 * hand back its flink name so emgd_dri.so can map it. */
static DRI2Buffer2Ptr
gma_dri2_create_buffer(DrawablePtr draw, unsigned int attachment,
		       unsigned int format)
{
	ScreenPtr pScreen = draw->pScreen;
	ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
	GMA500BLTPtr b = GMA500BLT_PRIV(scrn);
	DRI2Buffer2Ptr buf;
	GmaDri2Buf *priv;
	int cpp = (format ? format : draw->depth + 7) / 8;
	if (cpp < 1) cpp = 4;

	buf  = calloc(1, sizeof *buf);
	priv = calloc(1, sizeof *priv);
	if (!buf || !priv) { free(buf); free(priv); return NULL; }

	priv->handle = gma_gem_create(b->drm_fd, draw->width, draw->height, cpp,
				      &priv->pitch, &priv->size);
	if (!priv->handle) { free(buf); free(priv); return NULL; }
	priv->name = gma_gem_flink(b->drm_fd, priv->handle);
	priv->refcnt = 1;

	buf->attachment  = attachment;
	buf->name        = priv->name;
	buf->pitch       = priv->pitch;
	buf->cpp         = cpp;
	buf->flags       = 0;
	buf->format      = format;
	buf->driverPrivate = priv;
	return buf;
}

static void gma_dri2_destroy_buffer(DrawablePtr draw, DRI2Buffer2Ptr buf)
{
	ScrnInfoPtr scrn;
	GMA500BLTPtr b;
	GmaDri2Buf *priv;
	if (!buf) return;
	priv = buf->driverPrivate;
	if (priv && --priv->refcnt <= 0) {
		if (draw) {
			scrn = xf86ScreenToScrn(draw->pScreen);
			b = GMA500BLT_PRIV(scrn);
			gma_gem_close(b->drm_fd, priv->handle);
		}
		free(priv);
	}
	free(buf);
}

/* Blit a full drawable-sized region from src GEM buffer to dst GEM buffer via
 * the SGX 2D BLT engine (GMA_BLT_SUBMIT), used for DRI2CopyRegion. */
static void gma_dri2_blit(int fd, GmaDri2Buf *dst, GmaDri2Buf *src,
			  int w, int h)
{
	uint32_t cs[10];
	struct drm_psb_blt_reloc r[2];
	struct drm_psb_blt_submit s;

	/* Full-surface src-copy, byte-identical stream shape to EXA gma_Copy():
	 * FENCE, SRC_SURF(addr@2), DST_SURF(addr@4), BLIT, srcXY, dstXY, wh, FLUSH */
	cs[0] = P2D_FENCE_BH;
	cs[1] = P2D_SRC_SURF_BH | P2D_FMT_8888 | (src->pitch & 0x7fff);
	cs[2] = 0;					/* src addr (reloc 0)  */
	cs[3] = P2D_DST_SURF_BH | P2D_FMT_8888 | (dst->pitch & 0x7fff);
	cs[4] = 0;					/* dst addr (reloc 1)  */
	cs[5] = P2D_BLIT_BH | P2D_USE_PAT | P2D_ROP3_SRCCOPY;
	cs[6] = 0;					/* srcX,srcY = 0,0     */
	cs[7] = 0;					/* dstX,dstY = 0,0     */
	cs[8] = ((uint32_t)w << 12) | (uint32_t)h;	/* width,height        */
	cs[9] = P2D_FLUSH_BH;

	memset(r, 0, sizeof r);
	r[0].offset_dw = 2; r[0].handle = src->handle; r[0].delta = 0;
	r[1].offset_dw = 4; r[1].handle = dst->handle; r[1].delta = 0;

	memset(&s, 0, sizeof s);
	s.size        = sizeof s;
	s.flags       = DRM_PSB_BLT_SUBMIT_WAIT;
	s.cmd_ptr     = (uintptr_t) cs;
	s.relocs_ptr  = (uintptr_t) r;
	s.cmd_dwords  = 10;
	s.nr_relocs   = 2;
	(void) ioctl(fd, DRM_IOCTL_PSB_BLT_SUBMIT, &s);
}

static void
gma_dri2_copy_region(DrawablePtr draw, RegionPtr region,
		     DRI2BufferPtr dst, DRI2BufferPtr src)
{
	ScrnInfoPtr scrn = xf86ScreenToScrn(draw->pScreen);
	GMA500BLTPtr b = GMA500BLT_PRIV(scrn);
	GmaDri2Buf *sd = src->driverPrivate, *dd = dst->driverPrivate;

	/* Synchronous blit only.  Do NOT call DRI2SwapComplete here: with no
	 * ScheduleSwap, DRI2 drives swaps through CopyRegion, and DRI2SwapComplete
	 * re-enters DRI2CopyRegion -> this function -> infinite recursion. */
	if (sd && dd)
		gma_dri2_blit(b->drm_fd, dd, sd, draw->width, draw->height);
}

Bool gma500blt_dri2_screen_init(ScreenPtr pScreen)
{
	ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
	GMA500BLTPtr b = GMA500BLT_PRIV(scrn);
	DRI2InfoRec info;
	const char *dev;

	memset(&info, 0, sizeof info);
	info.version       = 4;
	info.fd            = b->drm_fd;
	info.driverName    = "emgd";		/* -> loads emgd_dri.so       */
	dev = drmGetDeviceNameFromFd2(b->drm_fd);
	info.deviceName    = dev ? dev : drmGetDeviceNameFromFd(b->drm_fd);
	info.CreateBuffer  = gma_dri2_create_buffer;
	info.DestroyBuffer = gma_dri2_destroy_buffer;
	info.CopyRegion    = gma_dri2_copy_region;

	if (!DRI2ScreenInit(pScreen, &info)) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR, "DRI2ScreenInit failed\n");
		return FALSE;
	}
	xf86DrvMsg(scrn->scrnIndex, X_INFO,
		   "DRI2: enabled, driver \"emgd\", device %s\n",
		   info.deviceName ? info.deviceName : "?");
	return TRUE;
}
