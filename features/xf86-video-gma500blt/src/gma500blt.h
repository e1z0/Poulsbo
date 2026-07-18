/*
 * gma500blt.h - private state + helpers for the GMA500 BLT EXA DDX.
 *
 * KMS/modeset comes from the in-kernel gma500 driver; this DDX opens its DRM
 * node and drives modeset via libdrm (see gma500blt_drmmode.c). 2D accel is
 * EXA over the SGX 2D BLIT core (gma500blt_exa.c). EXA pixmaps are GEM dumb
 * buffers; the screen/root pixmap is the KMS scanout front buffer.
 */
#ifndef _GMA500BLT_H_
#define _GMA500BLT_H_

#include <stdint.h>
#include <xf86.h>
#include <xf86Crtc.h>
#include <exa.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "gma500blt_drmmode.h"	/* drmmode_rec / drmmode_bo */

/*
 * Per-op accel state stashed between EXA Prepare* and the Solid/Copy calls.
 *
 * NOTE: this struct used to be (re)defined in gma500blt_exa.c as well. Two
 * definitions of the same tag in one translation unit is a hard C error, so
 * the single authoritative definition now lives here and gma500blt_exa.c just
 * uses it. The field set is byte-identical to the accel core's prior local
 * copy, so the proven Solid/Copy logic is unchanged.
 */
struct gma_accel_state {
	uint32_t fill_colour;
	uint32_t rop_fmt;		/* BLIT header ROP+use bits */
	uint32_t dst_fmt_stride;	/* DST_SURF format|pitch dword */
	uint32_t src_fmt_stride;	/* SRC_SURF format|pitch dword (copy) */
	uint32_t src_handle;		/* source GEM handle (copy) */
	int      xdir, ydir;		/* copy direction (overlap handling) */
};

/* Per-pixmap driver private: the GEM dumb-buffer backing an EXA pixmap. */
typedef struct {
	uint32_t handle;	/* GEM handle */
	uint32_t pitch;		/* bytes */
	uint32_t size;
	void    *cpu_ptr;	/* mmap for PrepareAccess/software fallback */
	int      bo_mapped;
	int      is_front;	/* root pixmap bound to the scanout bo: do not
				   DESTROY_DUMB/munmap it (drmmode owns it) */
} GMA500BLTPixmapPriv;

/* Per-screen driver private. */
typedef struct {
	int		drm_fd;		/* == drmmode.fd (exa core reads this) */
	int		cpp;		/* bytes per pixel of the framebuffer  */
	EntityInfoPtr	pEnt;
	drmmode_rec	drmmode;	/* KMS CRTC/output state + scanout bo   */
	ExaDriverPtr	exa;
	OptionInfoPtr	options;
	CreateScreenResourcesProcPtr	CreateScreenResources;
	CloseScreenProcPtr		CloseScreen;
	struct gma_accel_state		accel;
} GMA500BLTRec, *GMA500BLTPtr;

#define GMA500BLT_PRIV(scrn) ((GMA500BLTPtr)((scrn)->driverPrivate))

/* EXA passes PixmapPtr; recover the screen priv + the pixmap's GEM handle. */
#define GMA500BLT_GET(pPix) \
	GMA500BLT_PRIV(xf86ScreenToScrn((pPix)->drawable.pScreen))

/*
 * The GEM handle backing a pixmap. Offscreen EXA pixmaps carry their own dumb
 * bo in the driver-private. The root/screen pixmap has no EXA driver-private
 * (fb created it before EXA), so it is backed by the KMS scanout front buffer.
 */
static inline uint32_t gma_pixmap_handle(PixmapPtr pPix)
{
	GMA500BLTPixmapPriv *p = exaGetPixmapDriverPrivate(pPix);
	ScreenPtr pScreen;

	if (p && p->handle)
		return p->handle;
	pScreen = pPix->drawable.pScreen;
	if (pPix == pScreen->GetScreenPixmap(pScreen))
		return GMA500BLT_GET(pPix)->drmmode.front.handle;
	return 0;
}

static inline uint32_t gma_pixmap_stride(PixmapPtr pPix)
{
	return (uint32_t)exaGetPixmapPitch(pPix);
}

/* Full planemask test for the pixmap's depth (accel only supports full). */
#define GMA_PLANEMASK_FULL(pm, pPix) \
	(((pm) & ((1ull << (pPix)->drawable.depth) - 1)) == \
	 ((1ull << (pPix)->drawable.depth) - 1))

/* gma500blt_exa.c */
void gma500blt_exa_init_accel(ExaDriverPtr exa);

/* gma500blt_dri2.c - DRI2/GLX (loads the closed emgd_dri.so client driver). */
Bool gma500blt_dri2_screen_init(ScreenPtr pScreen);

#endif /* _GMA500BLT_H_ */
