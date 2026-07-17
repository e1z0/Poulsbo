/*
 * pvr_gma_bridge.h - gma500 <-> pvrsrvkm PVR-over-DRM bridge glue.
 *
 * The closed EMGD userspace opens gma500's DRM node (by busid PCI:00:02:00) and
 * issues the PVRSRV bridge over DRM command ioctls at the reserved offsets the
 * EMGD driver historically used (emgd_drm.h). We host those ioctls in gma500 and
 * forward to functions exported by pvrsrvkm (pvr_gma_bridge.c). Command numbers,
 * encodings and the arg struct size MUST match what the closed libs send:
 *   0x12 SRVKM    DRM_IOW (BASE+0x12, PVRSRV_BRIDGE_PACKAGE[28 bytes])
 *   0x15 ISMASTER DRM_IO  (BASE+0x15)
 *   0x16 UNPRIV   DRM_IOWR(BASE+0x16, unsigned long)
 */
#ifndef _PVR_GMA_BRIDGE_H_
#define _PVR_GMA_BRIDGE_H_

#include <drm/drm_ioctl.h>

/* Reserved command offsets (relative to DRM_COMMAND_BASE), matching EMGD's
 * DRM_PVR_RESERVED1/4/5 in drm/include/emgd_shared.h. */
#define DRM_GMA_PVR_SRVKM	0x12
#define DRM_GMA_PVR_ISMASTER	0x15
#define DRM_GMA_PVR_UNPRIV	0x16

/* The bridge argument the userspace passes to the SRVKM command. Layout must be
 * byte-identical to pvrsrvkm's PVRSRV_BRIDGE_PACKAGE (services4/include/pvr_bridge.h);
 * gma500 only needs its SIZE for the ioctl encoding, never its contents. */
struct gma_pvr_bridge_package {
	u32 ui32BridgeID;
	u32 ui32Size;
	void *pvParamIn;
	u32 ui32InBufferSize;
	void *pvParamOut;
	u32 ui32OutBufferSize;
	void *hKernelServices;
};

#define DRM_IOCTL_GMA_PVR_SRVKM \
	DRM_IOW(DRM_COMMAND_BASE + DRM_GMA_PVR_SRVKM, struct gma_pvr_bridge_package)
#define DRM_IOCTL_GMA_PVR_ISMASTER \
	DRM_IO(DRM_COMMAND_BASE + DRM_GMA_PVR_ISMASTER)
#define DRM_IOCTL_GMA_PVR_UNPRIV \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_GMA_PVR_UNPRIV, unsigned long)

/* Entry points exported by pvrsrvkm (services4/srvkm/env/linux/pvr_gma_bridge.c). */
extern int gma_pvr_srvkm_open(struct drm_device *dev, struct drm_file *file);
extern void gma_pvr_srvkm_release(struct drm_device *dev, struct drm_file *file);
extern int gma_pvr_srvkm_dispatch(struct drm_device *dev, void *arg,
				  struct drm_file *file);
extern int gma_pvr_is_master(struct drm_device *dev, void *arg,
			     struct drm_file *file);
/* Returns 1 if vm_pgoff is NOT a PVR area (caller should GEM-mmap); else it
 * services the PVR mmap and returns 0 or a negative errno. */
extern int gma_pvr_try_mmap(struct file *filp, struct vm_area_struct *vma);

#endif /* _PVR_GMA_BRIDGE_H_ */
