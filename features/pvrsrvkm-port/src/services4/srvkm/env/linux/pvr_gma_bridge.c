/*
 * pvr_gma_bridge.c - exported PVR-over-DRM bridge entry points for the gma500
 * coexistence integration (kernel 5.4 port).
 *
 * The closed EMGD userspace (libemgdsrv_um / WSEGL) opens the DRM node by busid
 * "PCI:00:02:00" -- which is gma500's card0 -- and issues the PVRSRV bridge over
 * DRM command ioctls (DRM_COMMAND_BASE + 0x12 SRVKM, +0x15 IsMaster, +0x16
 * Unpriv). gma500 hosts those ioctls and forwards them here; these wrappers run
 * the SAME transport-agnostic core the char-device path uses (BridgedDispatchKM,
 * PVRSRVProcessConnect/Disconnect), so no second DRM/PCI driver is registered
 * (gma500 owns 8086:8108). Per-process state is keyed by PID, so we don't touch
 * drm_file->driver_priv (which belongs to gma500). Built without the
 * PVR_SECURE_* paths, matching this port's ABI-safe option set.
 */
#include <linux/module.h>
#include <linux/mutex.h>
#include <drm/drm_device.h>
#include <drm/drm_file.h>

#include "img_defs.h"
#include "services.h"
#include "srvkm.h"
#include "pvr_bridge.h"
#include "perproc.h"
#include "pvr_debug.h"
#include "sgx_bridge.h"
#include "bridged_pvr_bridge.h"
#include "handle.h"

extern struct mutex gPVRSRVLock;

/* Create the per-process services connection for the opening client. Mirrors the
 * DRM PVRSRVOpen() minus the secure-export file-private-data bookkeeping. */
int gma_pvr_srvkm_open(struct drm_device *dev, struct drm_file *file)
{
	PVR_UNREFERENCED_PARAMETER(dev);
	PVR_UNREFERENCED_PARAMETER(file);

	/*
	 * Best-effort: this hook runs for EVERY open of gma500's DRM node,
	 * including internal fbcon/KMS opens. We must NOT fail the DRM open (that
	 * would break the display), so we pre-establish the PVR connection if we
	 * can but always return success. The real client connection is guaranteed
	 * by the PVRSRV_BRIDGE_CONNECT_SERVICES dispatch (PVRSRVPerProcessData
	 * creates the per-process area on demand) regardless of this call.
	 */
	mutex_lock(&gPVRSRVLock);
	(void)PVRSRVProcessConnect(OSGetCurrentProcessIDKM());
	mutex_unlock(&gPVRSRVLock);
	return 0;
}
EXPORT_SYMBOL(gma_pvr_srvkm_open);

/* Tear down the per-process connection. Mirrors DRM PVRSRVRelease(). */
void gma_pvr_srvkm_release(struct drm_device *dev, struct drm_file *file)
{
	PVR_UNREFERENCED_PARAMETER(dev);
	PVR_UNREFERENCED_PARAMETER(file);

	mutex_lock(&gPVRSRVLock);
	PVRSRVProcessDisconnect(OSGetCurrentProcessIDKM());
	mutex_unlock(&gPVRSRVLock);
}
EXPORT_SYMBOL(gma_pvr_srvkm_release);

/*
 * The main bridge dispatch. DRM core has already copied the ioctl argument into
 * kernel space, so 'arg' is a kernel PVRSRV_BRIDGE_PACKAGE. Mirrors the
 * SUPPORT_DRI_DRM PVRSRV_BridgeDispatchKM() (non-secure, non-test build).
 */
int gma_pvr_srvkm_dispatch(struct drm_device *dev, void *arg,
			   struct drm_file *file)
{
	PVRSRV_BRIDGE_PACKAGE *psBridgePackageKM = (PVRSRV_BRIDGE_PACKAGE *)arg;
	IMG_UINT32 cmd;
	IMG_UINT32 ui32PID = OSGetCurrentProcessIDKM();
	PVRSRV_PER_PROCESS_DATA *psPerProc;
	int err = -EFAULT;

	PVR_UNREFERENCED_PARAMETER(dev);
	PVR_UNREFERENCED_PARAMETER(file);

	if (psBridgePackageKM == IMG_NULL)
		return -EFAULT;

	mutex_lock(&gPVRSRVLock);

	cmd = psBridgePackageKM->ui32BridgeID;

	if (cmd != PVRSRV_BRIDGE_CONNECT_SERVICES) {
		PVRSRV_ERROR eError =
			PVRSRVLookupHandle(KERNEL_HANDLE_BASE,
					   (IMG_PVOID *)&psPerProc,
					   psBridgePackageKM->hKernelServices,
					   PVRSRV_HANDLE_TYPE_PERPROC_DATA);
		if (eError != PVRSRV_OK) {
			printk(KERN_INFO "PVRSGX-BR: cmd=%u PID=%u LookupHandle(%p) FAILED err=%d -> EFAULT\n",
			       cmd, ui32PID, psBridgePackageKM->hKernelServices, eError);
			goto unlock_and_return;
		}
		if (psPerProc->ui32PID != ui32PID) {
			printk(KERN_INFO "PVRSGX-BR: cmd=%u PID mismatch %u vs perproc %u -> EFAULT\n",
			       cmd, ui32PID, psPerProc->ui32PID);
			goto unlock_and_return;
		}
	} else {
		psPerProc = PVRSRVPerProcessData(ui32PID);
		if (psPerProc == IMG_NULL) {
			printk(KERN_INFO "PVRSGX-BR: CONNECT PID=%u perproc create FAILED -> EFAULT\n", ui32PID);
			goto unlock_and_return;
		}
	}

	psBridgePackageKM->ui32BridgeID =
		PVRSRV_GET_BRIDGE_ID(psBridgePackageKM->ui32BridgeID);

	err = BridgedDispatchKM(psPerProc, psBridgePackageKM);
	printk(KERN_INFO "PVRSGX-BR: cmd=%u PID=%u BridgedDispatchKM ret=%d\n", cmd, ui32PID, err);

unlock_and_return:
	mutex_unlock(&gPVRSRVLock);
	return err;
}
EXPORT_SYMBOL(gma_pvr_srvkm_dispatch);

/* Auxiliary DRM commands. IsMaster mirrors PVRDRMIsMaster (userspace probes
 * which fd is the DRM master); Unpriv handles the small unprivileged command
 * set. Kept minimal for the initial bring-up. */
int gma_pvr_is_master(struct drm_device *dev, void *arg, struct drm_file *file)
{
	PVR_UNREFERENCED_PARAMETER(dev);
	PVR_UNREFERENCED_PARAMETER(arg);
	PVR_UNREFERENCED_PARAMETER(file);
	return 0;
}
EXPORT_SYMBOL(gma_pvr_is_master);
