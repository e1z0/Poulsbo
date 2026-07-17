/*
 * gma500blt_driver.c - xf86 DDX driver for the GMA500/Poulsbo BLT EXA driver.
 *
 * KMS display comes from the in-kernel gma500 driver (this DDX opens its DRM
 * node and drives modeset via libdrm, see gma500blt_drmmode.c); 2D accel is
 * EXA over the SGX 2D BLIT core (gma500blt_exa.c) via the GMA_BLT_SUBMIT ioctl.
 *
 * Pixmap model:
 *   - Offscreen EXA pixmaps are their own GEM dumb buffers (gma_create_pixmap).
 *   - The root/screen pixmap is the KMS scanout front buffer (drmmode.front):
 *     fb rendering + EXA both target it; scanout is a drmModeAddFB of the same
 *     bo. This lets EXA accelerate onto the screen AND display correctly.
 *   - Composite/AA -> EXA software fallback (no PrepareComposite).
 *
 * Target: Xorg 1.20, video driver ABI 24 (modern PreInit/ScreenInit sigs).
 * Spots marked ON-TARGET need validation on the Vaio (no X test loop here).
 */
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "xorg-server.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Module.h"
#include <pciaccess.h>
#include "xf86Pci.h"
#include "xf86Crtc.h"
#include "fb.h"
#include "micmap.h"
#include "mipointer.h"
#include "exa.h"
#include "xf86drm.h"

#include "gma500blt.h"
#include "gma500blt_drmmode.h"

#define GMA_DRM_NAME		"gma500"	/* in-kernel KMS driver name */
#define GMA_VERSION		1000
#define GMA_NAME		"gma500blt"
#define GMA_DRIVER_NAME		"gma500blt"

#define PCI_VENDOR_INTEL	0x8086
#define PCI_CHIP_POULSBO	0x8108		/* 8086:8108 @ 0000:00:02.0 */

/* ---- forward decls ---- */
static Bool gma_pre_init(ScrnInfoPtr pScrn, int flags);
static Bool gma_screen_init(ScreenPtr pScreen, int argc, char **argv);
static Bool gma_close_screen(ScreenPtr pScreen);
static Bool gma_enter_vt(ScrnInfoPtr pScrn);
static void gma_leave_vt(ScrnInfoPtr pScrn);
static Bool gma_switch_mode(ScrnInfoPtr pScrn, DisplayModePtr mode);
static void gma_adjust_frame(ScrnInfoPtr pScrn, int x, int y);
static void gma_free_screen(ScrnInfoPtr pScrn);
static ModeStatus gma_valid_mode(ScrnInfoPtr pScrn, DisplayModePtr mode,
				 Bool verbose, int flags);

/* ================================================================== */
/* EXA: GEM dumb-buffer pixmap backing                               */
/* ================================================================== */

static void *gma_create_pixmap(ScreenPtr pScreen, int w, int h, int depth,
			       int usage, int bpp, int *new_pitch)
{
	GMA500BLTPtr b = GMA500BLT_PRIV(xf86ScreenToScrn(pScreen));
	GMA500BLTPixmapPriv *priv = calloc(1, sizeof(*priv));
	struct drm_mode_create_dumb c;

	(void)depth;
	(void)usage;
	if (!priv)
		return NULL;
	if (w == 0 || h == 0)			/* scratch/empty pixmap */
		return priv;

	memset(&c, 0, sizeof(c));
	c.width = w;
	c.height = h;
	c.bpp = bpp;
	if (drmIoctl(b->drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &c) != 0) {
		free(priv);
		return NULL;
	}
	priv->handle = c.handle;
	priv->pitch  = c.pitch;
	priv->size   = c.size;
	if (new_pitch)
		*new_pitch = c.pitch;
	return priv;
}

static void gma_destroy_pixmap(ScreenPtr pScreen, void *driverPriv)
{
	GMA500BLTPtr b = GMA500BLT_PRIV(xf86ScreenToScrn(pScreen));
	GMA500BLTPixmapPriv *priv = driverPriv;

	if (!priv)
		return;
	if (priv->is_front) {		/* root: drmmode owns this bo/mapping */
		free(priv);
		return;
	}
	if (priv->bo_mapped && priv->cpu_ptr)
		munmap(priv->cpu_ptr, priv->size);
	if (priv->handle) {
		struct drm_mode_destroy_dumb d;
		memset(&d, 0, sizeof(d));
		d.handle = priv->handle;
		drmIoctl(b->drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &d);
	}
	free(priv);
}

/*
 * EXA needs to know which pixmaps live in "driver memory" so it drives
 * PrepareSolid/Copy/Access on them. Offscreen dumb pixmaps and the scanout-
 * backed root pixmap both qualify.
 *
 * ON-TARGET RISK: the root pixmap has no EXA driver-private (fb created it
 * before EXA), yet we report it offscreen so the 2D BLT accelerates on-screen.
 * gma_pixmap_handle()/gma_prepare_access() special-case it, but if EXA is seen
 * to dereference a NULL driverPriv for the root on this xserver build, the safe
 * fallback is to `return FALSE` here for the root (screen stays software, only
 * offscreen<->offscreen blits accelerate). Verify on the Vaio.
 */
static Bool gma_pixmap_is_offscreen(PixmapPtr pPix)
{
	GMA500BLTPixmapPriv *priv = exaGetPixmapDriverPrivate(pPix);

	/*
	 * Only pixmaps that carry their own dumb bo (offscreen EXA pixmaps) are
	 * "offscreen" for acceleration. The root/screen pixmap has NO EXA
	 * driver-private (fb created it before EXA); reporting it offscreen made
	 * EXA dereference that NULL private during setup -> segfault. Treat the
	 * root as not-offscreen: it renders via fb/software, while offscreen dumb
	 * pixmaps still accelerate Solid/Copy. (On-target-hardened.)
	 */
	if (priv && priv->handle)
		return TRUE;
	return FALSE;
}

/* CPU access for software fallback / Composite: mmap the dumb BO. */
static Bool gma_prepare_access(PixmapPtr pPix, int index)
{
	GMA500BLTPtr b = GMA500BLT_GET(pPix);
	ScreenPtr pScreen = pPix->drawable.pScreen;
	GMA500BLTPixmapPriv *priv = exaGetPixmapDriverPrivate(pPix);
	struct drm_mode_map_dumb m;

	(void)index;

	/* Root/screen pixmap: backed by the persistent scanout mmap. */
	if ((!priv || !priv->handle)) {
		if (pPix == pScreen->GetScreenPixmap(pScreen) &&
		    b->drmmode.front.cpu_ptr) {
			pPix->devPrivate.ptr = b->drmmode.front.cpu_ptr;
			return TRUE;
		}
		return FALSE;
	}

	if (!priv->cpu_ptr) {
		memset(&m, 0, sizeof(m));
		m.handle = priv->handle;
		if (drmIoctl(b->drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &m) != 0)
			return FALSE;
		priv->cpu_ptr = mmap(NULL, priv->size, PROT_READ | PROT_WRITE,
				     MAP_SHARED, b->drm_fd, m.offset);
		if (priv->cpu_ptr == MAP_FAILED) {
			priv->cpu_ptr = NULL;
			return FALSE;
		}
		priv->bo_mapped = 1;
	}
	pPix->devPrivate.ptr = priv->cpu_ptr;
	return TRUE;
}

static void gma_finish_access(PixmapPtr pPix, int index)
{
	(void)index;
	pPix->devPrivate.ptr = NULL;		/* keep the map cached for reuse */
}

static Bool gma_exa_init(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	GMA500BLTPtr b = GMA500BLT_PRIV(pScrn);
	ExaDriverPtr exa = exaDriverAlloc();

	if (!exa)
		return FALSE;

	exa->exa_major = EXA_VERSION_MAJOR;
	exa->exa_minor = EXA_VERSION_MINOR;
	exa->pixmapOffsetAlign = 4;
	exa->pixmapPitchAlign  = 64;		/* ADDFB2 / dumb pitch is 64-aligned */
	exa->maxX = 2048;			/* 2D core 12-bit coords */
	exa->maxY = 2048;
	exa->flags = EXA_OFFSCREEN_PIXMAPS | EXA_HANDLES_PIXMAPS |
		     EXA_SUPPORTS_PREPARE_AUX;

	/* EXA_HANDLES_PIXMAPS: driver owns pixmap storage (GEM dumb bos). */
	exa->CreatePixmap2     = gma_create_pixmap;
	exa->DestroyPixmap     = gma_destroy_pixmap;
	exa->PixmapIsOffscreen = gma_pixmap_is_offscreen;
	exa->PrepareAccess     = gma_prepare_access;
	exa->FinishAccess      = gma_finish_access;
	exa->WaitMarker        = (void *)NoopDDA;

	gma500blt_exa_init_accel(exa);		/* Solid/Copy over the 2D core */

	if (!exaDriverInit(pScreen, exa)) {
		free(exa);
		return FALSE;
	}
	b->exa = exa;
	return TRUE;
}

/* ================================================================== */
/* Screen resources: attach the scanout bo to the root pixmap        */
/* ================================================================== */

static Bool gma_create_screen_resources(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	GMA500BLTPtr b = GMA500BLT_PRIV(pScrn);
	PixmapPtr root;
	Bool ret;

	pScreen->CreateScreenResources = b->CreateScreenResources;
	ret = pScreen->CreateScreenResources(pScreen);
	pScreen->CreateScreenResources = gma_create_screen_resources;
	if (!ret)
		return FALSE;

	/* Point the root pixmap at the scanout front buffer memory. */
	root = pScreen->GetScreenPixmap(pScreen);
	if (!pScreen->ModifyPixmapHeader(root, pScrn->virtualX, pScrn->virtualY,
					 -1, -1, b->drmmode.front.pitch,
					 b->drmmode.front.cpu_ptr))
		return FALSE;

	/*
	 * Root pixmap acceleration: DELIBERATELY LEFT SOFTWARE. Binding the root
	 * to the scanout bo and accelerating on-screen Solid/Copy via the SGX 2D
	 * BLT engine is correct but *slower* than software here: the kernel BLT
	 * submit is fully synchronous per op (pin -> submit -> wait-for-idle ->
	 * unpin), ~330ms/op measured, vs ~14000 small fills/sec in software. So we
	 * bind the scanout memory to the root (below) for CPU/fb rendering only and
	 * leave gma_pixmap_is_offscreen() returning FALSE for it. The EXA BLT path
	 * stays wired for offscreen pixmaps; making on-screen accel a win needs an
	 * async/fenced BLT submit (deferred unpin) in the kernel -- a separate task.
	 */
	{
		GMA500BLTPixmapPriv *rp = exaGetPixmapDriverPrivate(root);
		if (rp) {			/* fb/CPU access to the scanout, no accel handle */
			rp->cpu_ptr   = b->drmmode.front.cpu_ptr;
			rp->bo_mapped = 1;
			rp->is_front  = 1;
		}
	}

	/* Everything (front buffer, root pixmap, CRTC config) is ready: set
	 * the CRTC(s) to their desired modes. */
	if (!xf86SetDesiredModes(pScrn))
		return FALSE;

	return TRUE;
}

/* ================================================================== */
/* Screen init / close                                               */
/* ================================================================== */

static Bool gma_close_screen(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	GMA500BLTPtr b = GMA500BLT_PRIV(pScrn);

	if (pScrn->vtSema)
		gma_leave_vt(pScrn);
	pScrn->vtSema = FALSE;

	if (b->exa) {
		exaDriverFini(pScreen);
		free(b->exa);
		b->exa = NULL;
	}
	drmmode_free_front_bo(&b->drmmode);

	pScreen->CreateScreenResources = b->CreateScreenResources;
	pScreen->CloseScreen = b->CloseScreen;
	return (*pScreen->CloseScreen)(pScreen);
}

static Bool gma_screen_init(ScreenPtr pScreen, int argc, char **argv)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	GMA500BLTPtr b = GMA500BLT_PRIV(pScrn);
	VisualPtr visual;

	(void)argc;
	(void)argv;

	/* Scanout front buffer sized to the virtual screen; this is the root
	 * pixmap's backing store and the KMS scanout source. */
	if (!drmmode_create_front_bo(pScrn, &b->drmmode, pScrn->virtualX,
				     pScrn->virtualY, pScrn->bitsPerPixel,
				     pScrn->depth))
		return FALSE;
	pScrn->displayWidth = b->drmmode.front.pitch / b->cpp;

	miClearVisualTypes();
	if (!miSetVisualTypes(pScrn->depth, miGetDefaultVisualMask(pScrn->depth),
			      pScrn->rgbBits, pScrn->defaultVisual))
		return FALSE;
	if (!miSetPixmapDepths())
		return FALSE;

	if (!fbScreenInit(pScreen, b->drmmode.front.cpu_ptr,
			  pScrn->virtualX, pScrn->virtualY,
			  pScrn->xDpi, pScrn->yDpi, pScrn->displayWidth,
			  pScrn->bitsPerPixel))
		return FALSE;

	/* Fix up the RGB visual masks for TrueColor/DirectColor. */
	if (pScrn->bitsPerPixel > 8) {
		visual = pScreen->visuals + pScreen->numVisuals;
		while (--visual >= pScreen->visuals) {
			if ((visual->class | DynamicClass) == DirectColor) {
				visual->offsetRed   = pScrn->offset.red;
				visual->offsetGreen = pScrn->offset.green;
				visual->offsetBlue  = pScrn->offset.blue;
				visual->redMask     = pScrn->mask.red;
				visual->greenMask   = pScrn->mask.green;
				visual->blueMask    = pScrn->mask.blue;
			}
		}
	}

	fbPictureInit(pScreen, NULL, 0);

	xf86SetBlackWhitePixels(pScreen);

	if (!gma_exa_init(pScreen)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "EXA init failed\n");
		return FALSE;
	}

	xf86SetBackingStore(pScreen);
	xf86SetSilkenMouse(pScreen);
	miDCInitialize(pScreen, xf86GetPointerScreenFuncs());	/* SW cursor */

	/* Wrap CreateScreenResources to bind the root pixmap to the scanout
	 * bo and program the initial modes once resources exist. */
	b->CreateScreenResources = pScreen->CreateScreenResources;
	pScreen->CreateScreenResources = gma_create_screen_resources;

	if (!miCreateDefColormap(pScreen))
		return FALSE;

	/* Screen-saver/blank hook. Without this pScreen->SaveScreen is NULL and
	 * dixSaveScreens() calls through it at startup -> segfault. */
	pScreen->SaveScreen = xf86SaveScreen;

	/* Hook up RandR/CRTC. */
	if (!xf86CrtcScreenInit(pScreen))
		return FALSE;

	if (!xf86DPMSInit(pScreen, xf86DPMSSet, 0))
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "DPMS init failed\n");

	pScrn->vtSema = TRUE;

	b->CloseScreen = pScreen->CloseScreen;
	pScreen->CloseScreen = gma_close_screen;

	return TRUE;
}

/* ================================================================== */
/* VT / mode entry points (dispatched via the xf86 CRTC helpers)     */
/* ================================================================== */

static Bool gma_enter_vt(ScrnInfoPtr pScrn)
{
	GMA500BLTPtr b = GMA500BLT_PRIV(pScrn);

	/* Become DRM master and re-assert the modes. ON-TARGET: under logind
	 * the server may already own the master fd; this classic drmSetMaster
	 * path assumes we opened the node ourselves (root on a free VT). */
	if (drmSetMaster(b->drm_fd) != 0)
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			   "drmSetMaster failed (continuing)\n");

	pScrn->vtSema = TRUE;
	if (!xf86SetDesiredModes(pScrn))
		return FALSE;
	return TRUE;
}

static void gma_leave_vt(ScrnInfoPtr pScrn)
{
	GMA500BLTPtr b = GMA500BLT_PRIV(pScrn);
	xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
	int i;

	for (i = 0; i < config->num_crtc; i++) {
		xf86CrtcPtr crtc = config->crtc[i];
		drmmode_crtc_private_ptr dc = crtc->driver_private;

		/* Turn the CRTC off so the kernel/fbcon can take back the panel. */
		drmModeSetCrtc(b->drm_fd, dc->crtc_id, 0, 0, 0, NULL, 0, NULL);
	}
	drmDropMaster(b->drm_fd);
	pScrn->vtSema = FALSE;
}

static Bool gma_switch_mode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	return xf86SetSingleMode(pScrn, mode, RR_Rotate_0);
}

static void gma_adjust_frame(ScrnInfoPtr pScrn, int x, int y)
{
	xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
	xf86OutputPtr output = config->output[config->compat_output];
	xf86CrtcPtr crtc = output->crtc;

	if (crtc && crtc->enabled)
		drmmode_set_mode_major(crtc, &crtc->mode, crtc->rotation, x, y);
}

static ModeStatus gma_valid_mode(ScrnInfoPtr pScrn, DisplayModePtr mode,
				 Bool verbose, int flags)
{
	(void)pScrn;
	(void)mode;
	(void)verbose;
	(void)flags;
	return MODE_OK;
}

/* ================================================================== */
/* PreInit                                                           */
/* ================================================================== */

static Bool gma_pre_init(ScrnInfoPtr pScrn, int flags)
{
	GMA500BLTPtr b;
	rgb default_weight = { 0, 0, 0 };
	Gamma zeros = { 0.0, 0.0, 0.0 };

	if (flags & PROBE_DETECT)
		return FALSE;
	if (pScrn->numEntities != 1)
		return FALSE;

	if (!(pScrn->driverPrivate = calloc(1, sizeof(GMA500BLTRec))))
		return FALSE;
	b = GMA500BLT_PRIV(pScrn);
	b->pEnt = xf86GetEntityInfo(pScrn->entityList[0]);

	pScrn->monitor = pScrn->confScreen->monitor;
	pScrn->chipset = (char *)GMA_NAME;

	/* Open the in-kernel gma500 KMS node. ON-TARGET: drmOpen matches by
	 * driver name; if multiple DRM nodes exist, prefer a BusID match. */
	b->drm_fd = drmOpen(GMA_DRM_NAME, NULL);
	if (b->drm_fd < 0) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "cannot open DRM node for '%s'\n", GMA_DRM_NAME);
		return FALSE;
	}
	b->drmmode.fd = b->drm_fd;

	if (!xf86SetDepthBpp(pScrn, 24, 0, 0, Support32bppFb))
		return FALSE;
	xf86PrintDepthBpp(pScrn);
	if (pScrn->depth > 24) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "depth %d not supported\n", pScrn->depth);
		return FALSE;
	}
	if (!xf86SetWeight(pScrn, default_weight, default_weight))
		return FALSE;
	if (!xf86SetDefaultVisual(pScrn, -1))
		return FALSE;

	pScrn->progClock = TRUE;
	pScrn->rgbBits   = 8;
	b->cpp = (pScrn->bitsPerPixel + 7) / 8;
	b->drmmode.cpp = b->cpp;

	xf86CollectOptions(pScrn, NULL);

	/* Build the xf86 CRTC config from KMS resources + initial layout. */
	if (!drmmode_pre_init(pScrn, &b->drmmode, b->cpp))
		return FALSE;

	if (pScrn->virtualX == 0 || pScrn->virtualY == 0) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "no usable mode found from KMS connectors\n");
		return FALSE;
	}
	pScrn->displayWidth = pScrn->virtualX;	/* refined in ScreenInit */
	pScrn->currentMode  = pScrn->modes;	/* may be NULL for CRTC-based */

	xf86SetGamma(pScrn, zeros);
	xf86SetDpi(pScrn, 0, 0);

	if (!xf86LoadSubModule(pScrn, "fb"))
		return FALSE;
	if (!xf86LoadSubModule(pScrn, "exa"))
		return FALSE;

	return TRUE;
}

static void gma_free_screen(ScrnInfoPtr pScrn)
{
	GMA500BLTPtr b = GMA500BLT_PRIV(pScrn);

	if (!b)
		return;
	drmmode_close(pScrn, &b->drmmode);
	if (b->pEnt)
		free(b->pEnt);
	if (b->drm_fd >= 0)
		drmClose(b->drm_fd);
	free(b);
	pScrn->driverPrivate = NULL;
}

/* ================================================================== */
/* Probe / Identify / options                                        */
/* ================================================================== */

static const OptionInfoRec gma_options[] = {
	{ -1, NULL, OPTV_NONE, { 0 }, FALSE },
};

static const OptionInfoRec *gma_available_options(int chipid, int busid)
{
	(void)chipid;
	(void)busid;
	return gma_options;
}

static SymTabRec gma_chipsets[] = {
	{ PCI_CHIP_POULSBO, "gma500blt (Intel GMA500/Poulsbo SGX535)" },
	{ -1, NULL },
};

/* resources = NULL: KMS driver claims no legacy VGA/IO resources. */
static PciChipsets gma_pci_chipsets[] = {
	{ PCI_CHIP_POULSBO, PCI_CHIP_POULSBO, NULL },
	{ -1, -1, NULL },
};

/* PCI ids for autoloading via supported_devices. */
static const struct pci_id_match gma_device_match[] = {
	{ PCI_VENDOR_INTEL, PCI_CHIP_POULSBO, PCI_MATCH_ANY, PCI_MATCH_ANY,
	  0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0 },
};

static void gma_identify(int flags)
{
	(void)flags;
	xf86PrintChipsets(GMA_NAME,
			  "Driver for Intel GMA500/Poulsbo (SGX535 2D BLT EXA)",
			  gma_chipsets);
}

static Bool gma_probe(DriverPtr drv, int flags)
{
	GDevPtr *dev_sections;
	int *used_chips;
	int num_dev_sections, num_used, i;
	Bool found = FALSE;

	if (flags & PROBE_DETECT)
		return FALSE;

	num_dev_sections = xf86MatchDevice(GMA_NAME, &dev_sections);
	if (num_dev_sections <= 0)
		return FALSE;

	num_used = xf86MatchPciInstances(GMA_NAME, PCI_VENDOR_INTEL,
					 gma_chipsets, gma_pci_chipsets,
					 dev_sections, num_dev_sections,
					 drv, &used_chips);
	free(dev_sections);
	if (num_used <= 0)
		return FALSE;

	for (i = 0; i < num_used; i++) {
		ScrnInfoPtr pScrn =
			xf86ConfigPciEntity(NULL, 0, used_chips[i],
					    gma_pci_chipsets, NULL, NULL,
					    NULL, NULL, NULL);
		if (!pScrn)
			continue;
		found = TRUE;

		pScrn->driverVersion = GMA_VERSION;
		pScrn->driverName    = (char *)GMA_DRIVER_NAME;
		pScrn->name          = (char *)GMA_NAME;
		pScrn->Probe         = gma_probe;
		pScrn->PreInit       = gma_pre_init;
		pScrn->ScreenInit    = gma_screen_init;
		pScrn->SwitchMode    = gma_switch_mode;
		pScrn->AdjustFrame   = gma_adjust_frame;
		pScrn->EnterVT       = gma_enter_vt;
		pScrn->LeaveVT       = gma_leave_vt;
		pScrn->FreeScreen    = gma_free_screen;
		pScrn->ValidMode     = gma_valid_mode;
	}
	free(used_chips);
	return found;
}

/* KMS-only: tell the server we do not need console/VGA HW access. */
static Bool gma_driver_func(ScrnInfoPtr pScrn, xorgDriverFuncOp op, void *data)
{
	xorgHWFlags *flag;

	(void)pScrn;
	switch (op) {
	case GET_REQUIRED_HW_INTERFACES:
		flag = (xorgHWFlags *)data;
		*flag = HW_SKIP_CONSOLE;
		return TRUE;
	default:
		return FALSE;
	}
}

/* ================================================================== */
/* DriverRec + module descriptor                                     */
/* ================================================================== */

_X_EXPORT DriverRec gma500blt = {
	.driverVersion    = GMA_VERSION,
	.driverName       = (char *)GMA_DRIVER_NAME,
	.Identify         = gma_identify,
	.Probe            = gma_probe,
	.AvailableOptions = gma_available_options,
	.module           = NULL,
	.refCount         = 0,
	.driverFunc       = gma_driver_func,
	.supported_devices = gma_device_match,
	.PciProbe         = NULL,	/* classic Probe path (see gma_probe) */
};

static MODULESETUPPROTO(gma_setup);

static XF86ModuleVersionInfo gma_version_rec = {
	GMA_NAME,
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	1, 0, 0,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{ 0, 0, 0, 0 },
};

_X_EXPORT XF86ModuleData gma500bltModuleData = {
	&gma_version_rec,
	gma_setup,
	NULL,
};

/* MODULESETUPPROTO expands to: void *gma_setup(void *, void *, int *, int *)
 * (the legacy `pointer` typedef was removed in Xorg 1.20). */
static void *gma_setup(void *module, void *opts, int *errmaj, int *errmin)
{
	static Bool setup_done = FALSE;

	(void)opts;
	(void)errmin;
	if (!setup_done) {
		setup_done = TRUE;
		xf86AddDriver(&gma500blt, module, HaveDriverFuncs);
		return (void *)1;
	}
	if (errmaj)
		*errmaj = LDR_ONCEONLY;
	return NULL;
}
