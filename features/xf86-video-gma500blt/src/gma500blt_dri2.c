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

/* Per-DRI2-buffer private: the backing PVR2D allocation + its export handle. */
typedef struct {
	PVR2DMEMINFO *mem;	/* PVR2D allocation (pBase = CPU VA)            */
	unsigned long name;	/* PVR2D export handle handed to the client    */
	unsigned int  size;
	int           refcnt;
} GmaDri2Buf;

static PVR2DCONTEXTHANDLE g_pvr2d;	/* the DDX's PVR2D device context */

/* Lazily create the DDX's PVR2D context (PVR services must be up by now). */
static PVR2DCONTEXTHANDLE gma_dri2_pvr2d(ScrnInfoPtr scrn)
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
	PVR2DCONTEXTHANDLE ctx = gma_dri2_pvr2d(scrn);
	PVR2DHANDLE hexport = NULL;
	PVR2DERROR e;
	int cpp = (format ? format : draw->bitsPerPixel) / 8;
	unsigned int pitch, size;

	if (cpp < 1) cpp = 4;
	if (!ctx) return NULL;

	pitch = (draw->width * cpp + 7) & ~7u;		/* small alignment */
	size  = pitch * draw->height;

	buf  = calloc(1, sizeof *buf);
	priv = calloc(1, sizeof *priv);
	if (!buf || !priv) { free(buf); free(priv); return NULL; }

	e = PVR2DMemAlloc(ctx, size, 0, PVR2D_MEM_WRITECOMBINE, &priv->mem);
	if (e != PVR2D_OK || !priv->mem) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR, "DRI2: PVR2DMemAlloc=%ld\n", (long)e);
		free(buf); free(priv); return NULL;
	}
	e = PVR2DMemExport(ctx, 0, priv->mem, &hexport);
	if (e != PVR2D_OK) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR, "DRI2: PVR2DMemExport=%ld\n", (long)e);
		PVR2DMemFree(ctx, priv->mem); free(buf); free(priv); return NULL;
	}
	priv->name   = (unsigned long) hexport;
	priv->size   = size;
	priv->refcnt = 1;

	buf->attachment    = attachment;
	buf->name          = priv->name;	/* PVR2D export handle */
	buf->pitch         = pitch;
	buf->cpp           = cpp;
	buf->flags         = 0;
	buf->format        = format;
	buf->driverPrivate = priv;

	xf86DrvMsg(scrn->scrnIndex, X_INFO,
		   "DRI2 CreateBuffer: attach=%#x(%u) format=%#x %dx%d cpp=%d pitch=%u devaddr=%#lx name=%#lx\n",
		   attachment, attachment, format, draw->width, draw->height, cpp, pitch,
		   priv->mem->ui32DevAddr, priv->name);
	return buf;
}

static void gma_dri2_destroy_buffer(DrawablePtr draw, DRI2Buffer2Ptr buf)
{
	GmaDri2Buf *priv;
	if (!buf) return;
	priv = buf->driverPrivate;
	if (priv && --priv->refcnt <= 0) {
		if (g_pvr2d && priv->mem)
			PVR2DMemFree(g_pvr2d, priv->mem);
		free(priv);
	}
	free(buf);
}

/* Present: copy the source PVR2D buffer to the destination (both carry a CPU
 * mapping via PVR2DMEMINFO.pBase).  A CPU copy for now; the SGX 2D BLT path can
 * replace it once buffers expose GEM handles. */
static void
gma_dri2_copy_region(DrawablePtr draw, RegionPtr region,
		     DRI2BufferPtr dst, DRI2BufferPtr src)
{
	GmaDri2Buf *sd = src->driverPrivate, *dd = dst->driverPrivate;

	if (sd && dd && sd->mem && dd->mem && sd->mem->pBase && dd->mem->pBase) {
		unsigned int n = sd->size < dd->size ? sd->size : dd->size;
		memcpy(dd->mem->pBase, sd->mem->pBase, n);
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
