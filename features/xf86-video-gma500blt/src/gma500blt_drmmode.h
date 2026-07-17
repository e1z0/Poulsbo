/*
 * gma500blt_drmmode.h - libdrm KMS modeset layer for the GMA500 BLT DDX.
 *
 * xf86 CRTC/output objects backed by the in-kernel gma500 KMS driver
 * (drmModeGetResources / drmModeSetCrtc / dumb-buffer scanout). Modeled on
 * xf86-video-modesetting's drmmode_display.c for Xorg 1.20 (video ABI 24),
 * trimmed to one CRTC + the connected output.
 */
#ifndef _GMA500BLT_DRMMODE_H_
#define _GMA500BLT_DRMMODE_H_

#include <stdint.h>
#include <xf86.h>
#include <xf86Crtc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

/* A dumb buffer object used as the KMS scanout front buffer. */
typedef struct {
	uint32_t handle;	/* GEM handle */
	uint32_t pitch;		/* bytes */
	uint32_t size;		/* bytes */
	uint32_t fb_id;		/* drmModeAddFB id, 0 if none */
	int      width;
	int      height;
	void    *cpu_ptr;	/* mmap of the bo */
	int      mapped;
} drmmode_bo;

/* Per-screen KMS state. Embedded in GMA500BLTRec. */
typedef struct {
	int		fd;		/* DRM fd (owned by the driver rec) */
	int		cpp;		/* bytes per pixel */
	drmModeResPtr	mode_res;	/* cached KMS resources */
	drmmode_bo	front;		/* scanout front buffer (root pixmap) */
} drmmode_rec, *drmmode_ptr;

/* Per-CRTC private (xf86CrtcPtr->driver_private). */
typedef struct {
	drmmode_ptr	drmmode;
	uint32_t	crtc_id;
	int		index;
	drmModeCrtcPtr	mode_crtc;	/* saved KMS CRTC (for restore) */
} drmmode_crtc_private_rec, *drmmode_crtc_private_ptr;

/* Per-output private (xf86OutputPtr->driver_private). */
typedef struct {
	drmmode_ptr		drmmode;
	uint32_t		output_id;	/* connector id */
	drmModeConnectorPtr	mode_output;
	drmModeEncoderPtr	mode_encoder;
} drmmode_output_private_rec, *drmmode_output_private_ptr;

/* --- entry points used by gma500blt_driver.c --- */

/* Build the xf86 CRTC config from KMS resources and run the initial layout. */
Bool drmmode_pre_init(ScrnInfoPtr pScrn, drmmode_ptr drmmode, int cpp);

/* Create/replace the scanout front buffer (dumb bo + AddFB + mmap). */
Bool drmmode_create_front_bo(ScrnInfoPtr pScrn, drmmode_ptr drmmode,
			     int width, int height, int bpp, int depth);
void drmmode_free_front_bo(drmmode_ptr drmmode);

/* Program a CRTC with a mode against the scanout fb. Exposed so AdjustFrame
 * can re-pan without going through the full RandR path. */
Bool drmmode_set_mode_major(xf86CrtcPtr crtc, DisplayModePtr mode,
			    Rotation rotation, int x, int y);

/* Tear down CRTC/output libdrm objects + cached resources. */
void drmmode_close(ScrnInfoPtr pScrn, drmmode_ptr drmmode);

#endif /* _GMA500BLT_DRMMODE_H_ */
