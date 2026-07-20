/*
 * gma500blt_dri2.c - DRI2 for the GMA500 open DDX, providing HW-accelerated
 * desktop OpenGL/GLX via the closed PVR DRI2 driver (emgd_dri.so).
 *
 * Buffer model (the crux): emgd maps each DRI2 buffer with PVR2DMemMap(), so the
 * buffer MUST be a PVR-services object, not a plain gma500 GEM buffer (that
 * failed with PVR2DERROR_MAPPING_FAILED).  So the DDX itself is a PVR2D client:
 * it PVR2DMemAllocs each DRI2 buffer and PVR2DMemExports it, handing the client
 * the export handle as the DRI2 buffer "name".  emgd's PVR2DMemMap(name) then
 * maps the same SGX allocation -> renders into it.  DRI2CopyRegion presents by
 * copying the PVR2D buffer to the target (the buffers carry a CPU mapping).
 *
 * The DDX links libEMGD2d (PVR2D) and needs PVR services up (srvinit) before the
 * first buffer is created; PVR2D init is lazy on first CreateBuffer.
 */
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <xorg-server.h>
#include <xf86.h>
#include <xf86drm.h>
#include <dri2.h>
#include <fb.h>
#include "gma500blt.h"

/* ---- PVR2D v1 ABI (SGX DDK 1.x), matching features/emgd-wsegl/wsegl_win.c ---- */
typedef void *PVR2DCONTEXTHANDLE;
typedef void *PVR2DHANDLE;
typedef long  PVR2DERROR;
#define PVR2D_OK 0
#define PVR2D_MAX_DEVICE_NAME 20
typedef struct { unsigned long ulDevID; char szDeviceName[PVR2D_MAX_DEVICE_NAME]; } PVR2DDEVICEINFO;
typedef struct {
	void         *pBase;		/* CPU virtual address                 */
	unsigned long ui32MemSize;
	unsigned long ui32DevAddr;	/* SGX device virtual address          */
	unsigned long ulFlags;
	void         *hPrivateData;
	void         *hPrivateMapData;
} PVR2DMEMINFO;
#define PVR2D_MEM_WRITECOMBINE 0x00000004UL

extern int        PVR2DEnumerateDevices(PVR2DDEVICEINFO *);
extern PVR2DERROR PVR2DCreateDeviceContext(unsigned long, PVR2DCONTEXTHANDLE *, unsigned long);
extern PVR2DERROR PVR2DMemAlloc(PVR2DCONTEXTHANDLE, unsigned long, unsigned long, unsigned long, PVR2DMEMINFO **);
extern PVR2DERROR PVR2DMemFree(PVR2DCONTEXTHANDLE, PVR2DMEMINFO *);
extern PVR2DERROR PVR2DMemExport(PVR2DCONTEXTHANDLE, unsigned long, PVR2DMEMINFO *, PVR2DHANDLE *);

/* PVR2D 2D-blit (SGX 2D engine) — used to present without a slow uncached CPU
 * copy of write-combine memory. Struct is the SGX DDK 1.x PVR2DBLTINFO. */
#define PVR2D_ARGB8888   4
#define PVR2DROPcopy     0xCC		/* ROP3 SRCCOPY */
typedef struct {
	unsigned long  CopyCode;
	unsigned long  Colour;
	unsigned long  ColourKey;
	unsigned char  GlobalAlphaValue;
	unsigned char  AlphaBlendingFunc;
	unsigned long  BlitFlags;
	PVR2DMEMINFO  *pDstMemInfo;
	unsigned long  DstOffset;
	long           DstStride;
	unsigned long  DstFormat;
	long           DstSurfWidth, DstSurfHeight;
	long           DstX, DstY, DSizeX, DSizeY;
	PVR2DMEMINFO  *pSrcMemInfo;
	unsigned long  SrcOffset;
	long           SrcStride;
	unsigned long  SrcFormat;
	long           SrcSurfWidth, SrcSurfHeight;
	long           SrcX, SrcY, SizeX, SizeY;
	PVR2DMEMINFO  *pMaskMemInfo;
	unsigned long  MaskOffset;
	long           MaskStride;
	long           MaskSurfWidth, MaskSurfHeight;
	long           MaskX, MaskY;
} PVR2DBLTINFO;
extern PVR2DERROR PVR2DBlt(PVR2DCONTEXTHANDLE, PVR2DBLTINFO *);
extern PVR2DERROR PVR2DQueryBlitsComplete(PVR2DCONTEXTHANDLE, PVR2DMEMINFO *, unsigned int);

/* Per-DRI2-buffer private: the backing PVR2D allocation + its export handle. */
typedef struct {
	PVR2DMEMINFO *mem;	/* PVR2D allocation (pBase = CPU VA)            */
	unsigned long name;	/* PVR2D export handle handed to the client    */
	unsigned int  size;
	unsigned int  pitch;
	unsigned int  attachment;	/* cache key: the RETURNED attachment  */
	int           w, h;		/* cache key: drawable dimensions      */
	int           cpp;
	int           refcnt;
	int           is_tfp;		/* texture_from_pixmap: name is a pixmap's
					   own export (not owned here); don't
					   free/cache the backing PVR2D memory */
} GmaDri2Buf;

static PVR2DCONTEXTHANDLE g_pvr2d;	/* the DDX's PVR2D device context */

/*
 * DRI2 buffer reuse cache.  The X server's DRI2 core destroys + recreates a
 * drawable's buffers on every SwapBuffers, so without this each frame would
 * re-run PVR2DMemAlloc + PVR2DMemExport + descriptor write (services round-trips)
 * and hand the client a new name (triggering GEM flink/auth X round-trips).  On
 * DestroyBuffer we retain the PVR2D allocation on a free list keyed by
 * (attachment, w, h); CreateBuffer reuses a match instead of allocating, giving
 * stable buffers/names across frames.  Buffers are freed for real only when the
 * list overflows (or the PVR2D context is torn down).
 */
#define GMA_DRI2_CACHE_MAX 32
static GmaDri2Buf *g_bufcache[GMA_DRI2_CACHE_MAX];
static int         g_bufcache_n;

static GmaDri2Buf *gma_dri2_cache_get(unsigned int attach, int w, int h)
{
	int i;
	for (i = 0; i < g_bufcache_n; i++) {
		GmaDri2Buf *p = g_bufcache[i];
		if (p->attachment == attach && p->w == w && p->h == h) {
			g_bufcache[i] = g_bufcache[--g_bufcache_n];	/* unlink */
			return p;
		}
	}
	return NULL;
}

static void gma_dri2_cache_put(GmaDri2Buf *p)
{
	if (g_bufcache_n < GMA_DRI2_CACHE_MAX) {
		g_bufcache[g_bufcache_n++] = p;
		return;
	}
	if (g_pvr2d && p->mem)		/* cache full: free for real */
		PVR2DMemFree(g_pvr2d, p->mem);
	free(p);
}

/* Lazily create the DDX's PVR2D context (PVR services must be up by now).
 * Non-static: the pixmap allocator and EXA Solid/Copy share this context. */
PVR2DCONTEXTHANDLE gma500blt_pvr2d_ctx(ScrnInfoPtr scrn)
{
	PVR2DDEVICEINFO *arr; int n; PVR2DERROR e;

	if (g_pvr2d)
		return g_pvr2d;
	n = PVR2DEnumerateDevices(0);
	if (n < 1) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR, "DRI2: no PVR2D device (%d)\n", n);
		return NULL;
	}
	arr = calloc(n, sizeof(*arr));
	if (!arr || PVR2DEnumerateDevices(arr) < 0) { free(arr); return NULL; }
	e = PVR2DCreateDeviceContext(arr[0].ulDevID, &g_pvr2d, 0);
	free(arr);
	if (e != PVR2D_OK) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR, "DRI2: PVR2DCreateDeviceContext=%ld\n", (long)e);
		return NULL;
	}
	xf86DrvMsg(scrn->scrnIndex, X_INFO, "DRI2: PVR2D context up (%s)\n", arr ? "" : "");
	return g_pvr2d;
}

/* DRI2 CreateBuffer: allocate the buffer through PVR2D and export it, so the
 * client (emgd) can PVR2DMemMap the same SGX allocation. */
static DRI2Buffer2Ptr
gma_dri2_create_buffer(DrawablePtr draw, unsigned int attachment,
		       unsigned int format)
{
	ScrnInfoPtr scrn = xf86ScreenToScrn(draw->pScreen);
	DRI2Buffer2Ptr buf;
	GmaDri2Buf *priv;
	PVR2DCONTEXTHANDLE ctx = gma500blt_pvr2d_ctx(scrn);
	PVR2DHANDLE hexport = NULL;
	PVR2DERROR e;
	int cpp = (format ? format : draw->bitsPerPixel) / 8;
	unsigned int pitch, size;
	unsigned int ret_attach = attachment;

	/*
	 * emgd_dri.so requests a "special" buffer with the 0x80 bit OR'd into the
	 * attachment (its getBuffers wrapper does `attach | 0x80`, e.g. back|0x80 =
	 * 0x81). Its buffer-matching loop, however, only recognises 0/1/7 (front/
	 * back/fake-front) and 0x90 — where 0x90 is a metadata buffer whose returned
	 * `name`'s low 2 bits carry drawable flags. emgd's own DDX answers the
	 * 0x80-flagged request with attachment 0x90; a generic DRI2 server that
	 * echoes 0x81 leaves it unmatched, so emgd never slots its render buffers
	 * and later NULL-derefs. Mirror emgd's DDX: map any 0x80-flagged request to
	 * 0x90 and hand back name&3 == 0 (no rotation/extra flags).
	 */
	if (attachment & 0x80)
		ret_attach = 0x90;

	if (cpp < 1) cpp = 4;
	if (!ctx) return NULL;

	/*
	 * texture_from_pixmap fast path (the fix for black composited windows).
	 * When compiz redirects a window it binds that window's backing pixmap as
	 * a GL texture; emgd then requests the pixmap drawable's FRONT buffer
	 * (front-left, optionally |0x80).  RE of emgd_dri.so shows PVRDRIInitialise-
	 * Buffer only textures a returned buffer whose attachment == 0 (front):
	 * 0x1c480 PVR2DMemMaps that buffer's `name` ([drawable+0x30]) as the sampled
	 * surface.  Our normal path hands back a fresh (empty) render buffer, so
	 * every composited window textured black.  Instead hand back the PIXMAP's
	 * own PVR2D export as an attachment-0 buffer, so the texture samples the
	 * real window content.  Only redirect/offscreen pixmaps carry pvr2d_mem;
	 * the screen/root pixmap does not, so normal on-screen GL windows fall
	 * through to the render path below and are unaffected.
	 */
	if ((attachment & 0x7f) == 0) {			/* front-left (maybe |0x80) */
		PixmapPtr pp = (draw->type == DRAWABLE_WINDOW)
			? draw->pScreen->GetWindowPixmap((WindowPtr)draw)
			: (draw->type == DRAWABLE_PIXMAP ? (PixmapPtr)draw : NULL);
		GMA500BLTPixmapPriv *pxp = pp ? exaGetPixmapDriverPrivate(pp) : NULL;

		if (pxp && pxp->pvr2d_mem) {
			if (!pxp->pvr2d_export) {
				PVR2DHANDLE ph = NULL;
				if (PVR2DMemExport(ctx, 0,
						   (PVR2DMEMINFO *)pxp->pvr2d_mem,
						   &ph) == PVR2D_OK)
					pxp->pvr2d_export = (unsigned long)ph;
			}
			if (pxp->pvr2d_export) {
				/*
				 * Opaque-window alpha fix (the wobbly white/magenta bug).
				 * Depth-24 windows have no alpha channel, so X leaves the
				 * pixmap's pad byte 0.  compiz draws TRANSFORMED (wobbly)
				 * windows with premultiplied-alpha blending (GL_ONE,
				 * GL_ONE_MINUS_SRC_ALPHA); a sampled alpha of 0 degenerates
				 * that blend to ADDITIVE (win_rgb + dst_rgb), so an opaque
				 * maroon window over the blue desktop composites to magenta
				 * and bright content to white -- exactly the wobbly glitch.
				 * (Confirmed on HW via glReadPixels: alpha0+premult-blend =>
				 * 128,0,255; alpha255 => 128,0,0.  emgd's RGB-format TFP does
				 * not force alpha=1 either.)  Force the pad byte to 0xFF for
				 * opaque (non-32-bpp-alpha) pixmaps so the sampled alpha is
				 * 1.0 and the blend is a correct source-over.  Untransformed
				 * windows draw without blend and are unaffected; 32-bit ARGB
				 * windows keep their real alpha.
				 */
				if (pp->drawable.depth != 32 && pxp->cpu_ptr) {
					unsigned char *base = pxp->cpu_ptr;
					int ph = pp->drawable.height;
					int pw = pp->drawable.width;
					int yy, xx;
					for (yy = 0; yy < ph; yy++) {
						unsigned char *row = base + (size_t)yy * pxp->pitch;
						for (xx = 0; xx < pw; xx++)
							row[xx * 4 + 3] = 0xFF;
					}
				}

				priv = calloc(1, sizeof *priv);
				if (!priv) return NULL;
				priv->is_tfp = 1;
				priv->name   = pxp->pvr2d_export;
				priv->pitch  = pxp->pitch;
				priv->cpp    = cpp;
				priv->w      = draw->width;
				priv->h      = draw->height;
				priv->refcnt = 1;

				buf = calloc(1, sizeof *buf);
				if (!buf) { free(priv); return NULL; }
				buf->attachment    = 0;	/* FRONT: emgd textures this */
				buf->name          = pxp->pvr2d_export;
				buf->pitch         = pxp->pitch;
				buf->cpp           = cpp;
				buf->flags         = 0;
				buf->format        = format;
				buf->driverPrivate = priv;
				return buf;
			}
		}
	}

	pitch = (draw->width * cpp + 7) & ~7u;		/* small alignment */
	size  = pitch * draw->height;

	/*
	 * Reuse a cached (previously-freed) buffer of the same shape; otherwise
	 * allocate + export a fresh one and zero it (PVR2DMemAlloc doesn't clear).
	 */
	priv = gma_dri2_cache_get(ret_attach, draw->width, draw->height);
	if (!priv) {
		priv = calloc(1, sizeof *priv);
		if (!priv) return NULL;
		e = PVR2DMemAlloc(ctx, size, 0, PVR2D_MEM_WRITECOMBINE, &priv->mem);
		if (e != PVR2D_OK || !priv->mem) {
			xf86DrvMsg(scrn->scrnIndex, X_ERROR, "DRI2: PVR2DMemAlloc=%ld\n", (long)e);
			free(priv); return NULL;
		}
		if (priv->mem->pBase)
			memset(priv->mem->pBase, 0, size);
		e = PVR2DMemExport(ctx, 0, priv->mem, &hexport);
		if (e != PVR2D_OK) {
			xf86DrvMsg(scrn->scrnIndex, X_ERROR, "DRI2: PVR2DMemExport=%ld\n", (long)e);
			PVR2DMemFree(ctx, priv->mem); free(priv); return NULL;
		}
		priv->name       = (unsigned long) hexport;
		priv->size       = size;
		priv->pitch      = pitch;
		priv->cpp        = cpp;
		priv->attachment = ret_attach;
		priv->w          = draw->width;
		priv->h          = draw->height;
	}
	priv->refcnt = 1;

	/*
	 * (Re)write emgd's descriptor {type=1, handle x3, count=1} at the front of
	 * the buffer.  emgd's 0x1c3b0 reads dword0 as type and dword1..3 as PVR2D
	 * handles it PVR2DMemMaps into drawable+0x24[] (dword4=count); an empty
	 * descriptor => NULL deref.  emgd's 0x1c480 PVRSRVMemSet-zeroes the buffer
	 * while processing it, wiping the descriptor, so it MUST be rewritten on
	 * every CreateBuffer -- including cache reuses -- not just fresh allocation.
	 */
	/*
	 * texture_from_pixmap: if this drawable is backed by a PVR2D pixmap (a
	 * redirected compiz window / a pixmap), point emgd's descriptor at the
	 * PIXMAP's own SGX memory so the bound texture samples the real window
	 * content, not this fresh (empty) render buffer.  Normal GL windows are
	 * backed by the (non-PVR2D) screen pixmap, so they keep the render path.
	 * The export handle is cached on the pixmap to avoid a per-frame leak.
	 */
	{
		uint32_t tex = (uint32_t)priv->name;	/* default: this buffer */
		PixmapPtr pp = (draw->type == DRAWABLE_WINDOW)
			? draw->pScreen->GetWindowPixmap((WindowPtr)draw)
			: (draw->type == DRAWABLE_PIXMAP ? (PixmapPtr)draw : NULL);
		GMA500BLTPixmapPriv *pxp = pp ? exaGetPixmapDriverPrivate(pp) : NULL;

		if (pxp && pxp->pvr2d_mem) {
			if (!pxp->pvr2d_export) {
				PVR2DHANDLE ph = NULL;
				if (PVR2DMemExport(ctx, 0,
						   (PVR2DMEMINFO *)pxp->pvr2d_mem,
						   &ph) == PVR2D_OK)
					pxp->pvr2d_export = (unsigned long)ph;
			}
			if (pxp->pvr2d_export)
				tex = (uint32_t)pxp->pvr2d_export;
		}
		if (priv->mem->pBase) {
			uint32_t *d = priv->mem->pBase;
			d[0] = 1;
			d[1] = tex;
			d[2] = tex;
			d[3] = tex;
			d[4] = 1;
		}
	}

	buf = calloc(1, sizeof *buf);
	if (!buf) { gma_dri2_cache_put(priv); return NULL; }
	buf->attachment    = ret_attach;
	/* For emgd's 0x90 metadata buffer only name&3 is read (drawable flags);
	 * clear the low bits so emgd sees flags==0. Render buffers keep the real
	 * PVR2D export handle so emgd's PVR2DMemMap(name) resolves them. */
	buf->name          = (ret_attach == 0x90) ? (priv->name & ~3u) : priv->name;
	buf->pitch         = priv->pitch;
	buf->cpp           = priv->cpp;
	buf->flags         = 0;
	buf->format        = format;
	buf->driverPrivate = priv;
	return buf;
}

static void gma_dri2_destroy_buffer(DrawablePtr draw, DRI2Buffer2Ptr buf)
{
	GmaDri2Buf *priv;
	if (!buf) return;
	priv = buf->driverPrivate;
	if (priv && priv->is_tfp) {		/* pixmap owns the memory: just drop */
		free(priv);
		free(buf);
		return;
	}
	if (priv && --priv->refcnt <= 0)
		gma_dri2_cache_put(priv);	/* retain the PVR2D alloc for reuse */
	free(buf);
}

/* Present: copy the client's rendered (source/back) buffer to the on-screen KMS
 * scanout.  The DRI2 "front" buffer is an off-screen PVR2D allocation, so the
 * DRI2 core's back->front blit never reaches the panel; we present here instead.
 * While the SGX microkernel is up the 2D-BLT engine cannot be used (it wedges
 * the core -- see 2d-sgx-bif-conflict), so this is a CPU copy of write-combine
 * memory (uncached read, slower than a device blit, but it displays). */
static void
gma_dri2_copy_region(DrawablePtr draw, RegionPtr region,
		     DRI2BufferPtr dst, DRI2BufferPtr src)
{
	GmaDri2Buf *sd = src->driverPrivate;
	GMA500BLTPtr b = GMA500BLT_PRIV(xf86ScreenToScrn(draw->pScreen));
	unsigned char *scanout = b->drmmode.front.cpu_ptr;
	unsigned char *sbase   = (sd && sd->mem) ? sd->mem->pBase : NULL;
	int scr_pitch = b->drmmode.front.pitch;
	int scr_w = b->drmmode.front.width, scr_h = b->drmmode.front.height;
	int win_x = draw->x, win_y = draw->y;
	const int cpp = 4;
	BoxPtr box;
	int nbox, i, y;

	(void)dst;
	if (!scanout || !sbase)
		return;

	/*
	 * Present ONLY the damaged rectangles.  emgd exposes GLX_MESA_copy_sub_
	 * buffer, so a GL compositor (compiz) does PARTIAL redraws: it renders just
	 * the damage region into the back buffer and leaves the rest of that buffer
	 * stale/black, then presents with `region` = the damage.  Copying the whole
	 * back buffer would paint black over everything the frame didn't touch --
	 * which made idle composited windows go black, and moving flicker black
	 * squares (the damage rects surrounded by never-drawn buffer).  Copying only
	 * `region` preserves the undamaged scanout, which already holds the correct
	 * previous frame.  Coords: `region` is drawable-relative and so is the
	 * source buffer; the scanout destination adds the drawable origin.  Every
	 * rect is clipped to the scanout so an off-screen/oversized drawable can
	 * never write past the front buffer.
	 */
	if (!region || !RegionNotEmpty(region))
		return;
	nbox = RegionNumRects(region);
	box  = RegionRects(region);

	for (i = 0; i < nbox; i++, box++) {
		int rx1 = box->x1, ry1 = box->y1, rx2 = box->x2, ry2 = box->y2;

		for (y = ry1; y < ry2; y++) {
			int sy = win_y + y;		/* scanout row  */
			int sx = win_x + rx1;		/* scanout col  */
			int copy_w = rx2 - rx1;
			unsigned char *srcrow = sbase + (size_t)y * src->pitch
					      + (size_t)rx1 * cpp;

			if (sy < 0 || sy >= scr_h)
				continue;
			if (sx < 0) {			/* clip left edge */
				srcrow += (size_t)(-sx) * cpp;
				copy_w += sx;
				sx = 0;
			}
			if (sx + copy_w > scr_w)	/* clip right edge */
				copy_w = scr_w - sx;
			if (copy_w <= 0)
				continue;
			memcpy(scanout + (size_t)sy * scr_pitch + (size_t)sx * cpp,
			       srcrow, (size_t)copy_w * cpp);
		}
	}
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
	info.driverName    = "emgd";
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
		   "DRI2: enabled, driver \"emgd\", device %s (PVR2D buffers)\n",
		   info.deviceName ? info.deviceName : "?");
	return TRUE;
}
