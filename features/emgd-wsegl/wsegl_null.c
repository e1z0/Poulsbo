/*
 * wsegl_null.c - a minimal null WSEGL for offscreen (pbuffer) GLES2 on the SGX.
 *
 * libEMGDegl dlopens the WSEGL (OPK_DEFAULT="libemgdPVR2D_DRIWSEGL.so") and
 * dlsym's WSEGL_GetFunctionTablePointer -> a 12-function WSEGL v1 table
 * (confirmed from the closed lib: version + 12 fn ptrs). The stock DRIWSEGL
 * requires an X DRI2 provider; for a pbuffer we don't need one. This null WSEGL
 * makes eglInitialize succeed (InitialiseDisplay returns caps + a config, no
 * DRI2), letting a pbuffer surface render entirely on the SGX. Window/pixmap
 * paths are stubbed (we only exercise pbuffers).
 */
typedef void *WSEGLDisplayHandle;
typedef void *WSEGLDrawableHandle;
typedef void *NativeDisplayType;
typedef void *NativeWindowType;
typedef void *NativePixmapType;

typedef enum {
	WSEGL_SUCCESS = 0,
	WSEGL_CANNOT_INITIALISE,
	WSEGL_BAD_NATIVE_DISPLAY,
	WSEGL_BAD_NATIVE_WINDOW,
	WSEGL_BAD_NATIVE_PIXMAP,
	WSEGL_BAD_CONFIG,
	WSEGL_BAD_DRAWABLE,
	WSEGL_BAD_MATCH,
	WSEGL_OUT_OF_MEMORY
} WSEGL_Error;

typedef enum {
	WSEGL_NO_CAPS = 0,
	WSEGL_CAP_MIN_SWAP_INTERVAL,
	WSEGL_CAP_MAX_SWAP_INTERVAL,
	WSEGL_CAP_WINDOWS_USE_HW_SYNC,
	WSEGL_CAP_PIXMAPS_USE_HW_SYNC
} WSEGLCapsType;

typedef struct { WSEGLCapsType eCapsType; unsigned long ui32CapsValue; } WSEGLCaps;

typedef enum {
	WSEGL_PIXELFORMAT_565 = 0,
	WSEGL_PIXELFORMAT_4444,
	WSEGL_PIXELFORMAT_8888,
	WSEGL_PIXELFORMAT_1555
} WSEGLPixelFormat;

typedef enum { WSEGL_FALSE = 0, WSEGL_TRUE = 1 } WSEGL_Bool;

typedef struct {
	unsigned long    ui32DrawableType;
	WSEGLPixelFormat ePixelFormat;
	unsigned long    ulNativeRenderable;
	unsigned long    ulFrameBufferLevel;
	unsigned long    ulNativeVisualID;
	void            *hNativeVisual;
	unsigned long    eTransparentType;
	unsigned long    ulTransparentColor;
	unsigned long    ulFramebufferTarget;
} WSEGLConfig;

typedef struct {
	unsigned long    ui32Width;
	unsigned long    ui32Height;
	unsigned long    ui32Stride;
	WSEGLPixelFormat ePixelFormat;
	void            *pvLinearAddress;
	unsigned long    ui32HWAddress;
	unsigned long    ui32Flags;
} WSEGLDrawableParams;

typedef unsigned long WSEGLRotationAngle;

#define WSEGL_DRAWABLE_WINDOW 1
#define WSEGL_DRAWABLE_PIXMAP 2

/* Static caps + a single 8888 config (window|pixmap; the PVR EGL adds pbuffer). */
static const WSEGLCaps gasCaps[] = {
	{ WSEGL_CAP_MIN_SWAP_INTERVAL, 0 },
	{ WSEGL_CAP_MAX_SWAP_INTERVAL, 1 },
	{ WSEGL_NO_CAPS, 0 }
};

static WSEGLConfig gasConfigs[] = {
	{ WSEGL_DRAWABLE_WINDOW | WSEGL_DRAWABLE_PIXMAP, WSEGL_PIXELFORMAT_8888,
	  0, 0, 0, (void *)0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, (void *)0, 0, 0, 0 }   /* terminator */
};

static WSEGL_Error IsDisplayValid(NativeDisplayType d) { (void)d; return WSEGL_SUCCESS; }

static WSEGL_Error InitialiseDisplay(NativeDisplayType d, WSEGLDisplayHandle *ph,
				     const WSEGLCaps **ppCaps, WSEGLConfig **ppCfg)
{
	(void)d;
	*ph = (WSEGLDisplayHandle)0x1;   /* opaque non-NULL display handle */
	*ppCaps = gasCaps;
	*ppCfg = gasConfigs;
	return WSEGL_SUCCESS;
}

static WSEGL_Error CloseDisplay(WSEGLDisplayHandle h) { (void)h; return WSEGL_SUCCESS; }
static WSEGL_Error CreateWindowDrawable(WSEGLDisplayHandle h, WSEGLConfig *c,
	WSEGLDrawableHandle *pd, NativeWindowType w, WSEGLRotationAngle *r)
{ (void)h;(void)c;(void)w;(void)r; *pd=(WSEGLDrawableHandle)0x2; return WSEGL_SUCCESS; }
static WSEGL_Error CreatePixmapDrawable(WSEGLDisplayHandle h, WSEGLConfig *c,
	WSEGLDrawableHandle *pd, NativePixmapType p, WSEGLRotationAngle *r)
{ (void)h;(void)c;(void)p;(void)r; *pd=(WSEGLDrawableHandle)0x3; return WSEGL_SUCCESS; }
static WSEGL_Error DeleteDrawable(WSEGLDrawableHandle d) { (void)d; return WSEGL_SUCCESS; }
static WSEGL_Error SwapDrawable(WSEGLDrawableHandle d, unsigned long x) { (void)d;(void)x; return WSEGL_SUCCESS; }
static WSEGL_Error SwapControlInterval(WSEGLDrawableHandle d, unsigned long x) { (void)d;(void)x; return WSEGL_SUCCESS; }
static WSEGL_Error WaitNative(WSEGLDrawableHandle d, unsigned long x) { (void)d;(void)x; return WSEGL_SUCCESS; }
static WSEGL_Error CopyFromDrawable(WSEGLDrawableHandle d, NativePixmapType p) { (void)d;(void)p; return WSEGL_SUCCESS; }
static WSEGL_Error CopyFromPBuffer(void *a, unsigned long w, unsigned long h,
	unsigned long s, WSEGLPixelFormat f, NativePixmapType p)
{ (void)a;(void)w;(void)h;(void)s;(void)f;(void)p; return WSEGL_SUCCESS; }
static WSEGL_Error GetDrawableParameters(WSEGLDrawableHandle d,
	WSEGLDrawableParams *src, WSEGLDrawableParams *rnd)
{ (void)d;(void)src;(void)rnd; return WSEGL_SUCCESS; }

typedef struct {
	unsigned long ui32WSEGLVersion;
	WSEGL_Error (*pfnWSEGL_IsDisplayValid)(NativeDisplayType);
	WSEGL_Error (*pfnWSEGL_InitialiseDisplay)(NativeDisplayType, WSEGLDisplayHandle *, const WSEGLCaps **, WSEGLConfig **);
	WSEGL_Error (*pfnWSEGL_CloseDisplay)(WSEGLDisplayHandle);
	WSEGL_Error (*pfnWSEGL_CreateWindowDrawable)(WSEGLDisplayHandle, WSEGLConfig *, WSEGLDrawableHandle *, NativeWindowType, WSEGLRotationAngle *);
	WSEGL_Error (*pfnWSEGL_CreatePixmapDrawable)(WSEGLDisplayHandle, WSEGLConfig *, WSEGLDrawableHandle *, NativePixmapType, WSEGLRotationAngle *);
	WSEGL_Error (*pfnWSEGL_DeleteDrawable)(WSEGLDrawableHandle);
	WSEGL_Error (*pfnWSEGL_SwapDrawable)(WSEGLDrawableHandle, unsigned long);
	WSEGL_Error (*pfnWSEGL_SwapControlInterval)(WSEGLDrawableHandle, unsigned long);
	WSEGL_Error (*pfnWSEGL_WaitNative)(WSEGLDrawableHandle, unsigned long);
	WSEGL_Error (*pfnWSEGL_CopyFromDrawable)(WSEGLDrawableHandle, NativePixmapType);
	WSEGL_Error (*pfnWSEGL_CopyFromPBuffer)(void *, unsigned long, unsigned long, unsigned long, WSEGLPixelFormat, NativePixmapType);
	WSEGL_Error (*pfnWSEGL_GetDrawableParameters)(WSEGLDrawableHandle, WSEGLDrawableParams *, WSEGLDrawableParams *);
} WSEGL_FunctionTable;

static const WSEGL_FunctionTable gsFuncTable = {
	1,
	IsDisplayValid, InitialiseDisplay, CloseDisplay,
	CreateWindowDrawable, CreatePixmapDrawable, DeleteDrawable,
	SwapDrawable, SwapControlInterval, WaitNative,
	CopyFromDrawable, CopyFromPBuffer, GetDrawableParameters
};

const WSEGL_FunctionTable *WSEGL_GetFunctionTablePointer(void)
{
	return &gsFuncTable;
}
