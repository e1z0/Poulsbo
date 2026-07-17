/*
 * wsegl_win.c - WSEGL with real X11 WINDOW support for the SGX535, ZERO-COPY.
 *
 * Window drawables use the PVR2D FLIP CHAIN. The chain's buffers are the
 * gma500dc DC swap-chain buffers: GTT-pinned scanout buffers whose pages the
 * SGX MMU also maps into the GL context. GL renders straight into the current
 * back buffer (GetDrawableParameters hands EGL that buffer's device+CPU addr),
 * and eglSwapBuffers -> SwapDrawable -> PVR2DPresentFlip page-flips the display
 * to it. NO CPU copy, NO XPutImage. An UNMODIFIED GLES2 app
 * (eglCreateWindowSurface + eglSwapBuffers) now presents by page flip.
 *
 * The pbuffer path is unaffected (it never enters CreateWindowDrawable), and
 * InitialiseDisplay still succeeds unconditionally with lazy ensure_pvr2d().
 */
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* ---- reconstructed PVR2D v1 ABI (SGX DDK 1.x) ---- */
typedef void *PVR2DCONTEXTHANDLE;
typedef void *PVR2DFLIPCHAINHANDLE;
typedef long PVR2DERROR;
#define PVR2D_OK 0
#define PVR2D_MAX_DEVICE_NAME 20
typedef struct { unsigned long ulDevID; char szDeviceName[PVR2D_MAX_DEVICE_NAME]; } PVR2DDEVICEINFO;
typedef struct {
	void         *pBase;          /* CPU virtual address                 */
	unsigned long ui32MemSize;
	unsigned long ui32DevAddr;    /* SGX device virtual address          */
	unsigned long ulFlags;
	void         *hPrivateData;
	void         *hPrivateMapData;
} PVR2DMEMINFO;
#define PVR2D_MEM_WRITECOMBINE 0x00000004UL

/*
 * PVR2DFORMAT enum from the SGX DDK 1.x pvr2d.h. ARGB8888 == 4.
 * ON-TARGET RISK: verify this ordinal matches libEMGD2d's PVR2DFORMAT enum
 * (it is stable across DDK 1.x, but the closed lib is authoritative).
 */
typedef int PVR2DFORMAT;
#define PVR2D_ARGB8888 4

/* Max buffers GetFlipChainBuffers may return (DDK 1.x cap). */
#define PVR2D_MAX_FLIP_BUFFERS 8

extern int        PVR2DEnumerateDevices(PVR2DDEVICEINFO *);
extern PVR2DERROR PVR2DCreateDeviceContext(unsigned long, PVR2DCONTEXTHANDLE *, unsigned long);
extern PVR2DERROR PVR2DMemAlloc(PVR2DCONTEXTHANDLE, unsigned long, unsigned long, unsigned long, PVR2DMEMINFO **);
extern PVR2DERROR PVR2DMemFree(PVR2DCONTEXTHANDLE, PVR2DMEMINFO *);

/*
 * ---- PVR2D flip-chain API (exports present in libEMGD2d / emgd_drv.so) ----
 * Canonical SGX DDK 1.x pvr2d.h signatures:
 *
 *   PVR2DERROR PVR2DCreateFlipChain(PVR2DCONTEXTHANDLE hContext,
 *       unsigned long ulFlags, unsigned long ulNumBuffers,
 *       unsigned long ulWidth, unsigned long ulHeight, PVR2DFORMAT eFormat,
 *       long *plStride, unsigned long *pulFlipChainID,
 *       PVR2DFLIPCHAINHANDLE *phFlipChain);
 *   PVR2DERROR PVR2DGetFlipChainBuffers(PVR2DCONTEXTHANDLE hContext,
 *       PVR2DFLIPCHAINHANDLE hFlipChain, unsigned long *pnBuffers,
 *       PVR2DMEMINFO *psMemInfo[]);
 *   PVR2DERROR PVR2DPresentFlip(PVR2DCONTEXTHANDLE hContext,
 *       PVR2DFLIPCHAINHANDLE hFlipChain, PVR2DMEMINFO *psMemInfo, long lRenderID);
 *   PVR2DERROR PVR2DDestroyFlipChain(PVR2DCONTEXTHANDLE hContext,
 *       PVR2DFLIPCHAINHANDLE hFlipChain);
 */
extern PVR2DERROR PVR2DCreateFlipChain(PVR2DCONTEXTHANDLE, unsigned long,
	unsigned long, unsigned long, unsigned long, PVR2DFORMAT,
	long *, unsigned long *, PVR2DFLIPCHAINHANDLE *);
extern PVR2DERROR PVR2DGetFlipChainBuffers(PVR2DCONTEXTHANDLE,
	PVR2DFLIPCHAINHANDLE, unsigned long *, PVR2DMEMINFO *[]);
extern PVR2DERROR PVR2DPresentFlip(PVR2DCONTEXTHANDLE, PVR2DFLIPCHAINHANDLE,
	PVR2DMEMINFO *, long);
extern PVR2DERROR PVR2DDestroyFlipChain(PVR2DCONTEXTHANDLE, PVR2DFLIPCHAINHANDLE);

/* ---- WSEGL v1 ABI (12-fn table, confirmed from the closed lib) ---- */
typedef void *WSEGLDisplayHandle; typedef void *WSEGLDrawableHandle;
typedef void *NativeDisplayType; typedef void *NativeWindowType; typedef void *NativePixmapType;
typedef enum { WSEGL_SUCCESS=0, WSEGL_CANNOT_INITIALISE, WSEGL_BAD_NATIVE_DISPLAY,
	WSEGL_BAD_NATIVE_WINDOW, WSEGL_BAD_NATIVE_PIXMAP, WSEGL_BAD_CONFIG,
	WSEGL_BAD_DRAWABLE, WSEGL_BAD_MATCH, WSEGL_OUT_OF_MEMORY } WSEGL_Error;
typedef enum { WSEGL_NO_CAPS=0, WSEGL_CAP_MIN_SWAP_INTERVAL, WSEGL_CAP_MAX_SWAP_INTERVAL,
	WSEGL_CAP_WINDOWS_USE_HW_SYNC, WSEGL_CAP_PIXMAPS_USE_HW_SYNC } WSEGLCapsType;
typedef struct { WSEGLCapsType eCapsType; unsigned long ui32CapsValue; } WSEGLCaps;
typedef enum { WSEGL_PIXELFORMAT_565=0, WSEGL_PIXELFORMAT_4444, WSEGL_PIXELFORMAT_8888,
	WSEGL_PIXELFORMAT_1555 } WSEGLPixelFormat;
typedef struct { unsigned long ui32DrawableType; WSEGLPixelFormat ePixelFormat;
	unsigned long ulNativeRenderable, ulFrameBufferLevel, ulNativeVisualID;
	void *hNativeVisual; unsigned long eTransparentType, ulTransparentColor, ulFramebufferTarget;
} WSEGLConfig;
typedef struct { unsigned long ui32Width, ui32Height, ui32Stride; WSEGLPixelFormat ePixelFormat;
	void *pvLinearAddress; unsigned long ui32HWAddress, ui32Flags; } WSEGLDrawableParams;
typedef unsigned long WSEGLRotationAngle;
#define WSEGL_DRAWABLE_WINDOW 1
#define WSEGL_DRAWABLE_PIXMAP 2

static const WSEGLCaps gCaps[] = {
	{WSEGL_CAP_MIN_SWAP_INTERVAL,0},{WSEGL_CAP_MAX_SWAP_INTERVAL,1},{WSEGL_NO_CAPS,0}};
static WSEGLConfig gCfg[] = {
	{WSEGL_DRAWABLE_WINDOW|WSEGL_DRAWABLE_PIXMAP, WSEGL_PIXELFORMAT_8888,0,0,0,(void*)0,0,0,0},
	{0,0,0,0,0,(void*)0,0,0,0}};

typedef struct { Display *dpy; PVR2DCONTEXTHANDLE ctx; } WDISP;
typedef struct {
	WDISP *d; Window win; unsigned w, h;
	PVR2DFLIPCHAINHANDLE flip;      /* the PVR2D flip chain               */
	PVR2DMEMINFO *buf[PVR2D_MAX_FLIP_BUFFERS]; /* chain buffers            */
	unsigned      nbuf;             /* buffers actually in the chain      */
	unsigned      back;             /* index GL renders into              */
	unsigned      front;            /* index currently displayed          */
	unsigned long stride_px;        /* stride in PIXELS (proven fix)      */
} WDRAW;

static WSEGL_Error IsDisplayValid(NativeDisplayType d){ (void)d; return WSEGL_SUCCESS; }

/* Lazily create the PVR2D device context (needs the SGX services up, which is
 * guaranteed by the time a window is created). Returns 0 on success. */
#include <stdio.h>
static int ensure_pvr2d(WDISP *d)
{
	PVR2DDEVICEINFO *arr; int n; PVR2DERROR e;
	if (d->ctx) return 0;
	n = PVR2DEnumerateDevices(0);            /* NULL -> device count */
	fprintf(stderr,"WSEGLWIN: EnumerateDevices count=%d\n", n);
	if (n < 1) return -1;
	arr = calloc(n, sizeof(*arr));
	if (!arr) return -1;
	e = PVR2DEnumerateDevices(arr);
	fprintf(stderr,"WSEGLWIN: EnumerateDevices fill=%ld id=%lu name=%s\n",(long)e,arr[0].ulDevID,arr[0].szDeviceName);
	if (e < 0) { free(arr); return -1; }
	e = PVR2DCreateDeviceContext(arr[0].ulDevID, &d->ctx, 0);
	fprintf(stderr,"WSEGLWIN: CreateDeviceContext=%ld ctx=%p\n",(long)e,(void*)d->ctx);
	if (e != PVR2D_OK) { free(arr); return -1; }
	free(arr);
	return 0;
}

static WSEGL_Error InitialiseDisplay(NativeDisplayType nd, WSEGLDisplayHandle *ph,
				     const WSEGLCaps **caps, WSEGLConfig **cfg)
{
	WDISP *d = calloc(1,sizeof(*d));
	if(!d) return WSEGL_OUT_OF_MEMORY;
	d->dpy = (Display*)nd;                 /* X Display from the app; no DRI2 */
	*ph = d; *caps = gCaps; *cfg = gCfg;   /* succeed unconditionally (pbuffer works too) */
	return WSEGL_SUCCESS;
}
static WSEGL_Error CloseDisplay(WSEGLDisplayHandle h){ free(h); return WSEGL_SUCCESS; }

static WSEGL_Error CreateWindowDrawable(WSEGLDisplayHandle h, WSEGLConfig *c,
	WSEGLDrawableHandle *pd, NativeWindowType nw, WSEGLRotationAngle *r)
{
	WDISP *d = h; WDRAW *w = calloc(1,sizeof(*w));
	Window root; int x,y; unsigned bw,dep;
	long stride = 0; unsigned long chain_id = 0, nbuf = 0;
	PVR2DERROR e;
	(void)c;(void)r;
	if(!w) return WSEGL_OUT_OF_MEMORY;
	w->d = d; w->win = (Window)(unsigned long)nw;
	if(ensure_pvr2d(d) != 0){ fprintf(stderr,"WSEGLWIN: ensure_pvr2d failed\n"); free(w); return WSEGL_CANNOT_INITIALISE; }
	if(!XGetGeometry(d->dpy, w->win, &root,&x,&y,&w->w,&w->h,&bw,&dep)){ free(w); return WSEGL_BAD_NATIVE_WINDOW; }

	/* Create a 2-buffer flip chain sized to the window. ulFlags=0 requests a
	 * normal (non-OEM) flip-able chain, so gma500dc CreateDCSwapChain takes
	 * the flip path and allocates GTT scanout buffers. */
	e = PVR2DCreateFlipChain(d->ctx, 0 /*flags*/, 2 /*num*/,
		w->w, w->h, PVR2D_ARGB8888, &stride, &chain_id, &w->flip);
	fprintf(stderr,"WSEGLWIN: CreateFlipChain(%ux%u)=%ld id=%lu stride=%ld flip=%p\n",
		w->w, w->h, (long)e, chain_id, stride, (void*)w->flip);
	if(e != PVR2D_OK){ free(w); return WSEGL_OUT_OF_MEMORY; }

	nbuf = PVR2D_MAX_FLIP_BUFFERS;
	e = PVR2DGetFlipChainBuffers(d->ctx, w->flip, &nbuf, w->buf);
	fprintf(stderr,"WSEGLWIN: GetFlipChainBuffers=%ld nbuf=%lu b0=%p dev0=0x%lx\n",
		(long)e, nbuf, (void*)w->buf[0], w->buf[0]?w->buf[0]->ui32DevAddr:0);
	if(e != PVR2D_OK || nbuf < 2){
		PVR2DDestroyFlipChain(d->ctx, w->flip);
		free(w);
		return WSEGL_OUT_OF_MEMORY;
	}
	if(nbuf > PVR2D_MAX_FLIP_BUFFERS) nbuf = PVR2D_MAX_FLIP_BUFFERS;
	w->nbuf = (unsigned)nbuf;
	/* Stride reported by PVR2D is in BYTES; EGL wants PIXELS (proven fix). */
	w->stride_px = (stride > 0) ? (unsigned long)stride / 4 : w->w;
	w->back  = 0;                    /* GL renders into buffer 0 first     */
	w->front = w->nbuf - 1;          /* nothing shown yet; differs from back */
	*pd = w;
	return WSEGL_SUCCESS;
}
static WSEGL_Error CreatePixmapDrawable(WSEGLDisplayHandle h, WSEGLConfig *c,
	WSEGLDrawableHandle *pd, NativePixmapType p, WSEGLRotationAngle *r)
{ (void)h;(void)c;(void)p;(void)r; *pd=(WSEGLDrawableHandle)0x3; return WSEGL_SUCCESS; }

static WSEGL_Error DeleteDrawable(WSEGLDrawableHandle dh)
{
	WDRAW *w = dh;
	if(w && w->d){
		if(w->flip) PVR2DDestroyFlipChain(w->d->ctx, w->flip);
		free(w);
	}
	return WSEGL_SUCCESS;
}

static WSEGL_Error SwapDrawable(WSEGLDrawableHandle dh, unsigned long x)
{
	WDRAW *w = dh; (void)x;
	if(!w || !w->flip || !w->d || w->nbuf < 2) return WSEGL_SUCCESS;
	/* ZERO-COPY present: page-flip the display to the just-rendered back
	 * buffer. No CPU copy, no XPutImage. */
	{
		PVR2DERROR e = PVR2DPresentFlip(w->d->ctx, w->flip,
			w->buf[w->back], 0 /*lRenderID*/);
		if(e!=PVR2D_OK) fprintf(stderr,"WSEGLWIN: PresentFlip err=%ld\n",(long)e);
	}
	/* The rendered buffer is now the front; advance the back index. */
	w->front = w->back;
	w->back  = (w->back + 1) % w->nbuf;
	return WSEGL_SUCCESS;
}
static WSEGL_Error SwapControlInterval(WSEGLDrawableHandle d, unsigned long x){ (void)d;(void)x; return WSEGL_SUCCESS; }
static WSEGL_Error WaitNative(WSEGLDrawableHandle d, unsigned long x){ (void)d;(void)x; return WSEGL_SUCCESS; }
static WSEGL_Error CopyFromDrawable(WSEGLDrawableHandle d, NativePixmapType p){ (void)d;(void)p; return WSEGL_SUCCESS; }
static WSEGL_Error CopyFromPBuffer(void*a,unsigned long w,unsigned long h,unsigned long s,
	WSEGLPixelFormat f,NativePixmapType p){ (void)a;(void)w;(void)h;(void)s;(void)f;(void)p; return WSEGL_SUCCESS; }

static WSEGL_Error GetDrawableParameters(WSEGLDrawableHandle dh,
	WSEGLDrawableParams *src, WSEGLDrawableParams *rnd)
{
	WDRAW *w = dh;
	WSEGLDrawableParams ps, pr;   /* source (front) and render (back)      */
	memset(&ps,0,sizeof(ps)); memset(&pr,0,sizeof(pr));
	if(w && w->flip && w->nbuf >= 2){
		PVR2DMEMINFO *back  = w->buf[w->back];
		PVR2DMEMINFO *front = w->buf[w->front];
		/* render target = current BACK buffer (where GL draws) */
		pr.ui32Width=w->w; pr.ui32Height=w->h; pr.ui32Stride=w->stride_px;
		pr.ePixelFormat=WSEGL_PIXELFORMAT_8888;
		pr.pvLinearAddress=back->pBase; pr.ui32HWAddress=back->ui32DevAddr;
		/* source = current FRONT buffer (what is displayed) */
		ps = pr;
		ps.pvLinearAddress=front->pBase; ps.ui32HWAddress=front->ui32DevAddr;
	}
	if(src)*src=ps; if(rnd)*rnd=pr;
	return WSEGL_SUCCESS;
}

typedef struct {
	unsigned long ver;
	WSEGL_Error (*IsDisplayValid)(NativeDisplayType);
	WSEGL_Error (*InitialiseDisplay)(NativeDisplayType, WSEGLDisplayHandle*, const WSEGLCaps**, WSEGLConfig**);
	WSEGL_Error (*CloseDisplay)(WSEGLDisplayHandle);
	WSEGL_Error (*CreateWindowDrawable)(WSEGLDisplayHandle, WSEGLConfig*, WSEGLDrawableHandle*, NativeWindowType, WSEGLRotationAngle*);
	WSEGL_Error (*CreatePixmapDrawable)(WSEGLDisplayHandle, WSEGLConfig*, WSEGLDrawableHandle*, NativePixmapType, WSEGLRotationAngle*);
	WSEGL_Error (*DeleteDrawable)(WSEGLDrawableHandle);
	WSEGL_Error (*SwapDrawable)(WSEGLDrawableHandle, unsigned long);
	WSEGL_Error (*SwapControlInterval)(WSEGLDrawableHandle, unsigned long);
	WSEGL_Error (*WaitNative)(WSEGLDrawableHandle, unsigned long);
	WSEGL_Error (*CopyFromDrawable)(WSEGLDrawableHandle, NativePixmapType);
	WSEGL_Error (*CopyFromPBuffer)(void*, unsigned long, unsigned long, unsigned long, WSEGLPixelFormat, NativePixmapType);
	WSEGL_Error (*GetDrawableParameters)(WSEGLDrawableHandle, WSEGLDrawableParams*, WSEGLDrawableParams*);
} WSEGL_FunctionTable;

static const WSEGL_FunctionTable gT = { 1,
	IsDisplayValid, InitialiseDisplay, CloseDisplay, CreateWindowDrawable, CreatePixmapDrawable,
	DeleteDrawable, SwapDrawable, SwapControlInterval, WaitNative, CopyFromDrawable,
	CopyFromPBuffer, GetDrawableParameters };

const WSEGL_FunctionTable *WSEGL_GetFunctionTablePointer(void){ return &gT; }
