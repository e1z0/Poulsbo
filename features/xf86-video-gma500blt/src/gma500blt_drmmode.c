/*
 * gma500blt_drmmode.c - libdrm KMS modeset layer for the GMA500 BLT DDX.
 *
 * Implements xf86 CRTC/output objects over the in-kernel gma500 KMS driver.
 * Minimal but functional: enumerate CRTCs + connectors, expose their modes to
 * RandR, and scan out from a single dumb-buffer front buffer via drmModeSetCrtc.
 *
 * Modeled on xf86-video-modesetting's drmmode_display.c (Xorg 1.20, video ABI
 * 24). No hardware cursor (software cursor via miDCInitialize), no rotation
 * shadow, no page flipping.
 *
 * Spots marked ON-TARGET need validation against real Xorg 1.20 on the Vaio.
 */
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "xorg-server.h"
#include "xf86.h"
#include "xf86Crtc.h"
#include "xf86str.h"
#include "xf86DDC.h"		/* xf86InterpretEDID */
#include "xf86drm.h"
#include "xf86drmMode.h"

#include "gma500blt.h"
#include "gma500blt_drmmode.h"

/* ------------------------------------------------------------------ */
/* mode conversion helpers                                            */
/* ------------------------------------------------------------------ */

static void drmmode_ConvertFromKMode(ScrnInfoPtr pScrn, drmModeModeInfoPtr kmode,
				     DisplayModePtr mode)
{
	memset(mode, 0, sizeof(DisplayModeRec));
	mode->status = MODE_OK;

	mode->Clock = kmode->clock;

	mode->HDisplay   = kmode->hdisplay;
	mode->HSyncStart = kmode->hsync_start;
	mode->HSyncEnd   = kmode->hsync_end;
	mode->HTotal     = kmode->htotal;
	mode->HSkew      = kmode->hskew;

	mode->VDisplay   = kmode->vdisplay;
	mode->VSyncStart = kmode->vsync_start;
	mode->VSyncEnd   = kmode->vsync_end;
	mode->VTotal     = kmode->vtotal;
	mode->VScan      = kmode->vscan;

	mode->Flags   = kmode->flags;
	mode->name    = strdup(kmode->name);

	if (kmode->type & DRM_MODE_TYPE_DRIVER)
		mode->type = M_T_DRIVER;
	if (kmode->type & DRM_MODE_TYPE_PREFERRED)
		mode->type |= M_T_PREFERRED;

	xf86SetModeCrtc(mode, pScrn->adjustFlags);
}

static void drmmode_ConvertToKMode(drmModeModeInfoPtr kmode, DisplayModePtr mode)
{
	memset(kmode, 0, sizeof(*kmode));

	kmode->clock       = mode->Clock;
	kmode->hdisplay    = mode->HDisplay;
	kmode->hsync_start = mode->HSyncStart;
	kmode->hsync_end   = mode->HSyncEnd;
	kmode->htotal      = mode->HTotal;
	kmode->hskew       = mode->HSkew;

	kmode->vdisplay    = mode->VDisplay;
	kmode->vsync_start = mode->VSyncStart;
	kmode->vsync_end   = mode->VSyncEnd;
	kmode->vtotal      = mode->VTotal;
	kmode->vscan       = mode->VScan;

	kmode->flags       = mode->Flags;
	if (mode->name)
		strncpy(kmode->name, mode->name, DRM_DISPLAY_MODE_LEN - 1);
	kmode->name[DRM_DISPLAY_MODE_LEN - 1] = '\0';
}

/* ------------------------------------------------------------------ */
/* scanout front buffer                                              */
/* ------------------------------------------------------------------ */

void drmmode_free_front_bo(drmmode_ptr drmmode)
{
	drmmode_bo *bo = &drmmode->front;

	if (bo->cpu_ptr && bo->mapped)
		munmap(bo->cpu_ptr, bo->size);
	if (bo->fb_id)
		drmModeRmFB(drmmode->fd, bo->fb_id);
	if (bo->handle) {
		struct drm_mode_destroy_dumb d;
		memset(&d, 0, sizeof(d));
		d.handle = bo->handle;
		drmIoctl(drmmode->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &d);
	}
	memset(bo, 0, sizeof(*bo));
}

Bool drmmode_create_front_bo(ScrnInfoPtr pScrn, drmmode_ptr drmmode,
			     int width, int height, int bpp, int depth)
{
	struct drm_mode_create_dumb create;
	struct drm_mode_map_dumb map;
	drmmode_bo *bo = &drmmode->front;

	drmmode_free_front_bo(drmmode);

	memset(&create, 0, sizeof(create));
	create.width  = width;
	create.height = height;
	create.bpp    = bpp;
	if (drmIoctl(drmmode->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "CREATE_DUMB scanout failed: %s\n", strerror(errno));
		return FALSE;
	}
	bo->handle = create.handle;
	bo->pitch  = create.pitch;
	bo->size   = create.size;
	bo->width  = width;
	bo->height = height;

	if (drmModeAddFB(drmmode->fd, width, height, depth, bpp,
			 create.pitch, create.handle, &bo->fb_id)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "drmModeAddFB failed: %s\n", strerror(errno));
		goto err;
	}

	memset(&map, 0, sizeof(map));
	map.handle = create.handle;
	if (drmIoctl(drmmode->fd, DRM_IOCTL_MODE_MAP_DUMB, &map)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "MAP_DUMB scanout failed: %s\n", strerror(errno));
		goto err;
	}
	bo->cpu_ptr = mmap(NULL, create.size, PROT_READ | PROT_WRITE,
			   MAP_SHARED, drmmode->fd, map.offset);
	if (bo->cpu_ptr == MAP_FAILED) {
		bo->cpu_ptr = NULL;
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "scanout mmap failed: %s\n", strerror(errno));
		goto err;
	}
	bo->mapped = 1;
	memset(bo->cpu_ptr, 0, create.size);	/* start black */

	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "scanout front buffer %dx%d pitch %u fb_id %u\n",
		   width, height, bo->pitch, bo->fb_id);
	return TRUE;

err:
	drmmode_free_front_bo(drmmode);
	return FALSE;
}

/* ------------------------------------------------------------------ */
/* CRTC funcs                                                        */
/* ------------------------------------------------------------------ */

static void drmmode_crtc_dpms(xf86CrtcPtr crtc, int mode)
{
	(void)crtc;
	(void)mode;
	/* DPMS is driven at the output level below; nothing to do here. */
}

Bool drmmode_set_mode_major(xf86CrtcPtr crtc, DisplayModePtr mode,
			    Rotation rotation, int x, int y)
{
	ScrnInfoPtr pScrn = crtc->scrn;
	xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
	drmmode_crtc_private_ptr dc = crtc->driver_private;
	drmmode_ptr drmmode = dc->drmmode;
	uint32_t *connectors;
	int nconn = 0, i, ret;
	drmModeModeInfo kmode;
	Bool ok = FALSE;

	connectors = calloc(config->num_output, sizeof(uint32_t));
	if (!connectors)
		return FALSE;

	for (i = 0; i < config->num_output; i++) {
		xf86OutputPtr output = config->output[i];
		drmmode_output_private_ptr op;

		if (output->crtc != crtc)
			continue;
		op = output->driver_private;
		connectors[nconn++] = op->output_id;
	}

	if (nconn == 0) {
		/* No output on this CRTC -> disable it. */
		ret = drmModeSetCrtc(drmmode->fd, dc->crtc_id, 0, 0, 0,
				     NULL, 0, NULL);
		ok = (ret == 0);
		goto out;
	}

	drmmode_ConvertToKMode(&kmode, mode);
	ret = drmModeSetCrtc(drmmode->fd, dc->crtc_id, drmmode->front.fb_id,
			     x, y, connectors, nconn, &kmode);
	if (ret) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "drmModeSetCrtc(crtc %u fb %u) failed: %s\n",
			   dc->crtc_id, drmmode->front.fb_id, strerror(errno));
		ok = FALSE;
	} else {
		crtc->mode     = *mode;
		crtc->x        = x;
		crtc->y        = y;
		crtc->rotation = rotation;
		ok = TRUE;
	}

out:
	free(connectors);
	return ok;
}

static void drmmode_crtc_gamma_set(xf86CrtcPtr crtc, CARD16 *red, CARD16 *green,
				   CARD16 *blue, int size)
{
	drmmode_crtc_private_ptr dc = crtc->driver_private;

	/* Best-effort; the gma500 KMS driver may ignore this. ON-TARGET. */
	drmModeCrtcSetGamma(dc->drmmode->fd, dc->crtc_id, size, red, green, blue);
}

static void drmmode_crtc_destroy(xf86CrtcPtr crtc)
{
	drmmode_crtc_private_ptr dc = crtc->driver_private;

	if (dc) {
		if (dc->mode_crtc)
			drmModeFreeCrtc(dc->mode_crtc);
		free(dc);
	}
	crtc->driver_private = NULL;
}

static const xf86CrtcFuncsRec drmmode_crtc_funcs = {
	.dpms           = drmmode_crtc_dpms,
	.set_mode_major = drmmode_set_mode_major,
	.gamma_set      = drmmode_crtc_gamma_set,
	.destroy        = drmmode_crtc_destroy,
	/* No hardware cursor / shadow: software cursor via miDCInitialize. */
};

static void drmmode_crtc_init(ScrnInfoPtr pScrn, drmmode_ptr drmmode, int num)
{
	xf86CrtcPtr crtc;
	drmmode_crtc_private_ptr dc;

	crtc = xf86CrtcCreate(pScrn, &drmmode_crtc_funcs);
	if (!crtc)
		return;

	dc = calloc(1, sizeof(*dc));
	if (!dc) {
		xf86CrtcDestroy(crtc);
		return;
	}
	dc->drmmode  = drmmode;
	dc->index    = num;
	dc->crtc_id  = drmmode->mode_res->crtcs[num];
	dc->mode_crtc = drmModeGetCrtc(drmmode->fd, dc->crtc_id);
	crtc->driver_private = dc;
}

/* ------------------------------------------------------------------ */
/* Output funcs                                                      */
/* ------------------------------------------------------------------ */

static const char * const output_type_names[] = {
	"Unknown", "VGA", "DVI-I", "DVI-D", "DVI-A", "Composite",
	"SVIDEO", "LVDS", "Component", "DIN", "DP", "HDMI-A",
	"HDMI-B", "TV", "eDP", "Virtual", "DSI", "DPI",
};

static void drmmode_output_dpms(xf86OutputPtr output, int mode)
{
	(void)output;
	(void)mode;
	/* Panel/connector DPMS via the DPMS connector property is optional and
	 * left as a no-op here; the mode-off path (RandR disable) turns the
	 * CRTC off via drmModeSetCrtc. ON-TARGET if backlight blanking wanted. */
}

static xf86OutputStatus drmmode_output_detect(xf86OutputPtr output)
{
	drmmode_output_private_ptr op = output->driver_private;
	drmmode_ptr drmmode = op->drmmode;
	drmModeConnectorPtr koutput;

	/* Refresh connector state (hotplug). */
	koutput = drmModeGetConnector(drmmode->fd, op->output_id);
	if (koutput) {
		if (op->mode_output)
			drmModeFreeConnector(op->mode_output);
		op->mode_output = koutput;
	}
	if (!op->mode_output)
		return XF86OutputStatusUnknown;

	switch (op->mode_output->connection) {
	case DRM_MODE_CONNECTED:
		return XF86OutputStatusConnected;
	case DRM_MODE_DISCONNECTED:
		return XF86OutputStatusDisconnected;
	default:
		return XF86OutputStatusUnknown;
	}
}

static Bool drmmode_output_mode_valid(xf86OutputPtr output, DisplayModePtr mode)
{
	(void)output;
	(void)mode;
	return MODE_OK;
}

/* Pull the EDID blob (if any) and hand it to RandR. */
static void drmmode_output_attach_edid(xf86OutputPtr output)
{
	drmmode_output_private_ptr op = output->driver_private;
	drmmode_ptr drmmode = op->drmmode;
	drmModeConnectorPtr koutput = op->mode_output;
	drmModePropertyBlobPtr edid_blob = NULL;
	xf86MonPtr mon = NULL;
	int i;

	for (i = 0; koutput && i < koutput->count_props; i++) {
		drmModePropertyPtr prop =
			drmModeGetProperty(drmmode->fd, koutput->props[i]);
		if (!prop)
			continue;
		if ((prop->flags & DRM_MODE_PROP_BLOB) &&
		    !strcmp(prop->name, "EDID")) {
			if (edid_blob)
				drmModeFreePropertyBlob(edid_blob);
			edid_blob = drmModeGetPropertyBlob(drmmode->fd,
							   koutput->prop_values[i]);
		}
		drmModeFreeProperty(prop);
	}

	if (edid_blob) {
		mon = xf86InterpretEDID(output->scrn->scrnIndex, edid_blob->data);
		drmModeFreePropertyBlob(edid_blob);
	}
	xf86OutputSetEDID(output, mon);
}

static DisplayModePtr drmmode_output_get_modes(xf86OutputPtr output)
{
	drmmode_output_private_ptr op = output->driver_private;
	drmModeConnectorPtr koutput = op->mode_output;
	DisplayModePtr modes = NULL;
	int i;

	drmmode_output_attach_edid(output);

	if (!koutput)
		return NULL;

	for (i = 0; i < koutput->count_modes; i++) {
		DisplayModePtr mode = xnfalloc(sizeof(DisplayModeRec));

		drmmode_ConvertFromKMode(output->scrn, &koutput->modes[i], mode);
		modes = xf86ModesAdd(modes, mode);
	}
	return modes;
}

static void drmmode_output_destroy(xf86OutputPtr output)
{
	drmmode_output_private_ptr op = output->driver_private;

	if (op) {
		if (op->mode_encoder)
			drmModeFreeEncoder(op->mode_encoder);
		if (op->mode_output)
			drmModeFreeConnector(op->mode_output);
		free(op);
	}
	output->driver_private = NULL;
}

static const xf86OutputFuncsRec drmmode_output_funcs = {
	.dpms       = drmmode_output_dpms,
	.detect     = drmmode_output_detect,
	.mode_valid = drmmode_output_mode_valid,
	.get_modes  = drmmode_output_get_modes,
	.destroy    = drmmode_output_destroy,
};

static void drmmode_output_init(ScrnInfoPtr pScrn, drmmode_ptr drmmode, int num)
{
	xf86OutputPtr output;
	drmModeConnectorPtr koutput;
	drmModeEncoderPtr kencoder = NULL;
	drmmode_output_private_ptr op;
	const char *tname;
	char name[32];

	koutput = drmModeGetConnector(drmmode->fd,
				      drmmode->mode_res->connectors[num]);
	if (!koutput)
		return;

	if (koutput->count_encoders > 0)
		kencoder = drmModeGetEncoder(drmmode->fd, koutput->encoders[0]);

	if (koutput->connector_type <
	    (int)(sizeof(output_type_names) / sizeof(output_type_names[0])))
		tname = output_type_names[koutput->connector_type];
	else
		tname = "Unknown";
	snprintf(name, sizeof(name), "%s-%d", tname,
		 koutput->connector_type_id);

	output = xf86OutputCreate(pScrn, &drmmode_output_funcs, name);
	if (!output) {
		if (kencoder)
			drmModeFreeEncoder(kencoder);
		drmModeFreeConnector(koutput);
		return;
	}

	op = calloc(1, sizeof(*op));
	if (!op) {
		xf86OutputDestroy(output);
		if (kencoder)
			drmModeFreeEncoder(kencoder);
		drmModeFreeConnector(koutput);
		return;
	}
	op->drmmode      = drmmode;
	op->output_id    = drmmode->mode_res->connectors[num];
	op->mode_output  = koutput;
	op->mode_encoder = kencoder;
	output->driver_private = op;

	output->possible_crtcs  = kencoder ? kencoder->possible_crtcs : ~0u;
	output->possible_clones = kencoder ? kencoder->possible_clones : 0;
	output->interlaceAllowed = TRUE;
	output->subpixel_order   = SubPixelUnknown;
}

/* ------------------------------------------------------------------ */
/* CRTC config (RandR resize)                                        */
/* ------------------------------------------------------------------ */

static Bool drmmode_xf86crtc_resize(ScrnInfoPtr pScrn, int width, int height)
{
	GMA500BLTPtr b = GMA500BLT_PRIV(pScrn);
	drmmode_ptr drmmode = &b->drmmode;
	ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
	PixmapPtr root;
	int i;

	if (width == pScrn->virtualX && height == pScrn->virtualY)
		return TRUE;

	/* Reallocate the scanout front buffer at the new size, then re-point
	 * the root pixmap at it. CRTCs are re-programmed by the RandR caller
	 * (which calls set_mode_major, picking up the new front.fb_id).
	 *
	 * ON-TARGET: exercises the resize path; the initial preferred-mode
	 * bring-up does not depend on it. */
	if (!drmmode_create_front_bo(pScrn, drmmode, width, height,
				     pScrn->bitsPerPixel, pScrn->depth))
		return FALSE;

	pScrn->virtualX     = width;
	pScrn->virtualY     = height;
	pScrn->displayWidth = drmmode->front.pitch / drmmode->cpp;

	if (pScreen) {
		root = pScreen->GetScreenPixmap(pScreen);
		if (root)
			pScreen->ModifyPixmapHeader(root, width, height, -1, -1,
						    drmmode->front.pitch,
						    drmmode->front.cpu_ptr);
	}

	/* Disable CRTCs whose mode no longer fits; RandR re-enables valid ones. */
	{
		xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
		for (i = 0; i < config->num_crtc; i++) {
			xf86CrtcPtr crtc = config->crtc[i];
			if (!crtc->enabled)
				continue;
			if (crtc->mode.HDisplay > width ||
			    crtc->mode.VDisplay > height)
				drmmode_set_mode_major(crtc, &crtc->mode,
						       crtc->rotation,
						       crtc->x, crtc->y);
		}
	}
	return TRUE;
}

static const xf86CrtcConfigFuncsRec drmmode_xf86crtc_config_funcs = {
	.resize = drmmode_xf86crtc_resize,
};

/* ------------------------------------------------------------------ */
/* pre-init / teardown                                               */
/* ------------------------------------------------------------------ */

Bool drmmode_pre_init(ScrnInfoPtr pScrn, drmmode_ptr drmmode, int cpp)
{
	int i;
	int max_w, max_h;

	drmmode->cpp = cpp;

	drmmode->mode_res = drmModeGetResources(drmmode->fd);
	if (!drmmode->mode_res) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "drmModeGetResources failed: %s\n", strerror(errno));
		return FALSE;
	}

	xf86CrtcConfigInit(pScrn, &drmmode_xf86crtc_config_funcs);

	/* The SGX 2D BLIT core uses 12-bit (0..4095) coordinate fields; clamp
	 * the RandR virtual-size range so EXA/scanout stay within it. */
	max_w = drmmode->mode_res->max_width;
	max_h = drmmode->mode_res->max_height;
	if (max_w <= 0 || max_w > 2048)
		max_w = 2048;
	if (max_h <= 0 || max_h > 2048)
		max_h = 2048;
	xf86CrtcSetSizeRange(pScrn, 320, 200, max_w, max_h);

	for (i = 0; i < drmmode->mode_res->count_crtcs; i++)
		drmmode_crtc_init(pScrn, drmmode, i);

	for (i = 0; i < drmmode->mode_res->count_connectors; i++)
		drmmode_output_init(pScrn, drmmode, i);

	/* Pick CRTC<->output assignment + the initial virtual size/mode. */
	if (!xf86InitialConfiguration(pScrn, TRUE)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "xf86InitialConfiguration failed\n");
		return FALSE;
	}
	return TRUE;
}

void drmmode_close(ScrnInfoPtr pScrn, drmmode_ptr drmmode)
{
	(void)pScrn;
	drmmode_free_front_bo(drmmode);
	if (drmmode->mode_res) {
		drmModeFreeResources(drmmode->mode_res);
		drmmode->mode_res = NULL;
	}
	/* xf86 CRTC/output objects are freed by the server via the .destroy
	 * hooks when the screen is torn down. */
}
