/*
 * gma500blt_exa.c - EXA 2D acceleration for GMA500/Poulsbo (SGX 2D BLIT core)
 *
 * Accelerates X11 Solid (fill) and Copy via the kernel GMA_BLT_SUBMIT ioctl,
 * which drives the SGX 2D BLIT engine's slave port. Pixmaps are GEM dumb
 * buffers; the 2D command stream carries 0-filled address slots that the
 * kernel relocates from GEM handles (out-of-band relocs), so no GPU VAs ever
 * leave userspace. Composite/blend/AA fall back to software (the 2D core has
 * no blend/mask here) until the GL/glamor path (Layer C) exists.
 */
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include "xorg-server.h"
#include "xf86.h"
#include "exa.h"
#include "gma500blt.h"		/* GMA500BLTPtr, pixmap->handle helpers */
#include "gma_blt_drm.h"	/* DRM_IOCTL_PSB_BLT_SUBMIT, reloc, stream limits */

/* SGX 2D-core command-stream opcodes (psb_reg.h / the proven blt-ioctl). */
#define P2D_FENCE_BH	0x70000000u
#define P2D_BLIT_BH	0x80000000u
#define P2D_DST_SURF_BH	0xA0000000u
#define P2D_SRC_SURF_BH	0xC0000000u
#define P2D_FLUSH_BH	0xF0000000u
#define P2D_USE_FILL	0x00000000u
#define P2D_USE_PAT	0x00000010u	/* copy uses the pattern/source path */
#define P2D_ROP3_PATCOPY 0x0000F0F0u	/* S       (SRCCOPY for copy)         */
#define P2D_ROP3_SRCCOPY 0x0000CCCCu

/* X11 GXcopy is the only ROP we accelerate; others -> software. */
#define GMA_ACCEL_ALU_COPY	3	/* GXcopy */

/* cpp -> DST/SRC surface format field (matches PSB_2D_DST_* formats). */
static uint32_t gma_2d_format(int depth, int bpp)
{
	switch (bpp) {
	case 32: return 0x00060000u;	/* 8888 ARGB */
	case 16: return (depth == 15) ? 0x00040000u	/* 1555 */
				      : 0x00050000u;	/* 565  */
	case 8:  return 0x00030000u;	/* 332  */
	default: return 0xffffffffu;	/* unsupported */
	}
}

/*
 * Per-op state stashed between Prepare* and the Solid/Copy calls.
 * The definition of struct gma_accel_state now lives in gma500blt.h (a single
 * authoritative definition; defining the same tag here too is a C redefinition
 * error once the header is included). The field set is unchanged, so the accel
 * logic below is byte-for-byte the proven-on-hardware code.
 */

/* Submit an 8-dword fill or 10-dword copy stream with its relocs. */
static Bool gma_blt_submit(int fd, const uint32_t *cs, int dwords,
			   const struct drm_psb_blt_reloc *relocs, int nrelocs)
{
	struct drm_psb_blt_submit s;

	memset(&s, 0, sizeof(s));
	s.size       = sizeof(s);
	s.flags      = DRM_PSB_BLT_SUBMIT_WAIT;
	s.cmd_ptr    = (uintptr_t)cs;
	s.relocs_ptr = (uintptr_t)relocs;
	s.cmd_dwords = dwords;
	s.nr_relocs  = nrelocs;
	return ioctl(fd, DRM_IOCTL_PSB_BLT_SUBMIT, &s) == 0;
}

/* ---------------- EXA Solid (fill) ---------------- */

static Bool gma_PrepareSolid(PixmapPtr pDst, int alu, Pixel planemask, Pixel fg)
{
	GMA500BLTPtr b = GMA500BLT_GET(pDst);
	struct gma_accel_state *st = &b->accel;
	uint32_t fmt = gma_2d_format(pDst->drawable.depth, pDst->drawable.bitsPerPixel);

	if (alu != GMA_ACCEL_ALU_COPY || !GMA_PLANEMASK_FULL(planemask, pDst))
		return FALSE;			/* only GXcopy, full planemask */
	if (fmt == 0xffffffffu)
		return FALSE;
	if (gma_pixmap_stride(pDst) > 0x7fff)	/* DST_SURF stride field */
		return FALSE;

	st->fill_colour    = fg;
	st->rop_fmt        = P2D_BLIT_BH | P2D_USE_FILL | P2D_ROP3_PATCOPY;
	st->dst_fmt_stride = P2D_DST_SURF_BH | fmt | gma_pixmap_stride(pDst);
	return TRUE;
}

/*
 * CPU fill of a rectangle -- used for sub-minimum rects that would wedge the
 * 2D engine (GMA_BLT_MIN_DIM erratum).  Writes straight into the pixmap's
 * backing bo (the same memory the engine would target), so it is immediately
 * consistent with later accelerated ops on the same pixmap.
 */
static void gma_solid_sw(PixmapPtr pDst, int x1, int y1, int x2, int y2,
			 uint32_t colour)
{
	int cpp   = pDst->drawable.bitsPerPixel >> 3;
	int pitch = (int)exaGetPixmapPitch(pDst);
	unsigned char *base;
	int x, y, w = x2 - x1;

	if (!gma_prepare_access(pDst, EXA_PREPARE_DEST))
		return;
	base = pDst->devPrivate.ptr;
	if (base) {
		for (y = y1; y < y2; y++) {
			unsigned char *row = base + (size_t)y * pitch +
					     (size_t)x1 * cpp;
			if (cpp == 4) {
				uint32_t *p = (uint32_t *)row;
				for (x = 0; x < w; x++)
					p[x] = colour;
			} else if (cpp == 2) {
				uint16_t *p = (uint16_t *)row;
				for (x = 0; x < w; x++)
					p[x] = (uint16_t)colour;
			} else {
				memset(row, (int)(colour & 0xff), (size_t)w);
			}
		}
	}
	gma_finish_access(pDst, EXA_PREPARE_DEST);
}

static void gma_Solid(PixmapPtr pDst, int x1, int y1, int x2, int y2)
{
	GMA500BLTPtr b = GMA500BLT_GET(pDst);
	struct gma_accel_state *st = &b->accel;
	uint32_t cs[8];
	struct drm_psb_blt_reloc r;

	/*
	 * Software-2D mode: the 2D-BLT engine hangs the graphics core while the
	 * SGX microkernel is up (the GL/DRI2 path initialises it), so never
	 * issue a blit -- do every fill on the CPU.  Revert to the sub-8px
	 * guard once 2D/SGX coexistence lands (see 2d-sgx-bif-conflict).
	 */
	if (1) {
		gma_solid_sw(pDst, x1, y1, x2, y2, st->fill_colour);
		return;
	}

	cs[0] = P2D_FENCE_BH;
	cs[1] = st->dst_fmt_stride;
	cs[2] = 0;					/* dst addr (reloc slot 2) */
	cs[3] = st->rop_fmt;
	cs[4] = st->fill_colour;
	cs[5] = ((uint32_t)x1 << 12) | (uint32_t)y1;
	cs[6] = ((uint32_t)(x2 - x1) << 12) | (uint32_t)(y2 - y1);
	cs[7] = P2D_FLUSH_BH;

	memset(&r, 0, sizeof(r));
	r.offset_dw = 2;
	r.handle    = gma_pixmap_handle(pDst);
	r.delta     = 0;
	gma_blt_submit(b->drm_fd, cs, 8, &r, 1);
}

static void gma_DoneSolid(PixmapPtr pDst) { (void)pDst; }

/* ---------------- EXA Copy (screen-to-screen blit) ---------------- */

static Bool gma_PrepareCopy(PixmapPtr pSrc, PixmapPtr pDst, int xdir, int ydir,
			    int alu, Pixel planemask)
{
	GMA500BLTPtr b = GMA500BLT_GET(pDst);
	struct gma_accel_state *st = &b->accel;
	uint32_t dfmt = gma_2d_format(pDst->drawable.depth, pDst->drawable.bitsPerPixel);
	uint32_t sfmt = gma_2d_format(pSrc->drawable.depth, pSrc->drawable.bitsPerPixel);

	if (alu != GMA_ACCEL_ALU_COPY || !GMA_PLANEMASK_FULL(planemask, pDst))
		return FALSE;
	if (dfmt == 0xffffffffu || sfmt == 0xffffffffu)
		return FALSE;
	if (gma_pixmap_stride(pDst) > 0x7fff || gma_pixmap_stride(pSrc) > 0x7fff)
		return FALSE;

	st->rop_fmt        = P2D_BLIT_BH | P2D_USE_PAT | P2D_ROP3_SRCCOPY;
	st->dst_fmt_stride = P2D_DST_SURF_BH | dfmt | gma_pixmap_stride(pDst);
	st->src_fmt_stride = P2D_SRC_SURF_BH | sfmt | gma_pixmap_stride(pSrc);
	st->src_handle     = gma_pixmap_handle(pSrc);
	st->copy_src       = pSrc;		/* for the tiny-rect CPU fallback */
	st->xdir = xdir; st->ydir = ydir;
	return TRUE;
}

/*
 * CPU copy of a rectangle -- tiny-rect erratum fallback for gma_Copy.  Honours
 * the EXA copy direction (ydir for vertically-overlapping same-pixmap blits)
 * and uses memmove per row so horizontal overlap is safe too.
 */
static void gma_copy_sw(PixmapPtr pDst, PixmapPtr pSrc, int srcX, int srcY,
			int dstX, int dstY, int w, int h, int ydir)
{
	int cpp    = pDst->drawable.bitsPerPixel >> 3;
	int dpitch = (int)exaGetPixmapPitch(pDst);
	int spitch = (int)exaGetPixmapPitch(pSrc);
	unsigned char *sbase, *dbase;
	int i;

	if (!gma_prepare_access(pSrc, EXA_PREPARE_SRC))
		return;
	sbase = pSrc->devPrivate.ptr;
	if (!gma_prepare_access(pDst, EXA_PREPARE_DEST)) {
		gma_finish_access(pSrc, EXA_PREPARE_SRC);
		return;
	}
	dbase = pDst->devPrivate.ptr;
	if (sbase && dbase) {
		size_t rowbytes = (size_t)w * cpp;
		for (i = 0; i < h; i++) {
			int y = (ydir < 0) ? (h - 1 - i) : i;
			unsigned char *s = sbase + (size_t)(srcY + y) * spitch +
					   (size_t)srcX * cpp;
			unsigned char *d = dbase + (size_t)(dstY + y) * dpitch +
					   (size_t)dstX * cpp;
			memmove(d, s, rowbytes);
		}
	}
	gma_finish_access(pDst, EXA_PREPARE_DEST);
	gma_finish_access(pSrc, EXA_PREPARE_SRC);
}

static void gma_Copy(PixmapPtr pDst, int srcX, int srcY, int dstX, int dstY,
		     int w, int h)
{
	GMA500BLTPtr b = GMA500BLT_GET(pDst);
	struct gma_accel_state *st = &b->accel;
	uint32_t cs[10];
	struct drm_psb_blt_reloc r[2];

	/* Software-2D mode (see gma_Solid): never touch the 2D-BLT engine while
	 * SGX is up -- copy on the CPU. */
	if (1) {
		gma_copy_sw(pDst, st->copy_src, srcX, srcY, dstX, dstY, w, h,
			    st->ydir);
		return;
	}

	/* SRC_SURF (addr) + DST_SURF (addr) + BLIT(src copy) + coords + FLUSH */
	cs[0] = P2D_FENCE_BH;
	cs[1] = st->src_fmt_stride;
	cs[2] = 0;				/* src addr (reloc 0)  */
	cs[3] = st->dst_fmt_stride;
	cs[4] = 0;				/* dst addr (reloc 1)  */
	cs[5] = st->rop_fmt;
	cs[6] = ((uint32_t)srcX << 12) | (uint32_t)srcY;
	cs[7] = ((uint32_t)dstX << 12) | (uint32_t)dstY;
	cs[8] = ((uint32_t)w << 12) | (uint32_t)h;
	cs[9] = P2D_FLUSH_BH;

	memset(r, 0, sizeof(r));
	r[0].offset_dw = 2; r[0].handle = st->src_handle;      r[0].delta = 0;
	r[1].offset_dw = 4; r[1].handle = gma_pixmap_handle(pDst); r[1].delta = 0;
	gma_blt_submit(b->drm_fd, cs, 10, r, 2);
}

static void gma_DoneCopy(PixmapPtr pDst) { (void)pDst; }

/*
 * Register the accel hooks into an ExaDriverRec. CPU-access (PrepareAccess/
 * FinishAccess), pixmap alloc, and Composite (software fallback) are wired by
 * the driver scaffolding (gma500blt_driver.c); this file provides the 2D-core
 * Solid/Copy that give the actual GPU acceleration.
 */
void gma500blt_exa_init_accel(ExaDriverPtr exa)
{
	exa->PrepareSolid = gma_PrepareSolid;
	exa->Solid        = gma_Solid;
	exa->DoneSolid    = gma_DoneSolid;
	exa->PrepareCopy  = gma_PrepareCopy;
	exa->Copy         = gma_Copy;
	exa->DoneCopy     = gma_DoneCopy;
	/* No PrepareComposite -> EXA falls back to software for Render/AA. */
}
