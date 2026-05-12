# SGX Lab SGX-State Query Ioctl Validation

Date: 2026-05-11

## Scope

Add a bounded typed SGX snapshot query ioctl so the live SGX state path no
longer depends entirely on `poulsbo_sgx_state` text parsing.

This milestone covers:

- [third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
- [workloads/Makefile](/root/Poulsbo/workloads/Makefile)
- [workloads/psb_sgx_lab_sgx_state_query.c](/root/Poulsbo/workloads/psb_sgx_lab_sgx_state_query.c)
- [scripts/run-vaio-sgx-ioctl-sgx-state.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-sgx-state.sh)

## What Was Added

Kernel-side:

- new bounded ioctl query:
  - `DRM_IOCTL_PSB_SGX_LAB_QUERY_SGX_STATE`
- new optional capture flag:
  - `DRM_PSB_SGX_LAB_QUERY_SGX_STATE_CAPTURE`
- new typed SGX snapshot structure:
  - `drm_psb_sgx_lab_sgx_state_query`

The ioctl mirrors the current kernel-owned SGX snapshot state, including:

- `reason`
- `sequence`
- `captured_jiffies`
- `capture_failed`
- `irq_count`
- `fault_count`
- MMU and GATT snapshot fields
- SGX core and event registers
- BIF fault and directory-bank state
- 2D request base and blit status

Userspace-side:

- new 32-bit helper:
  - [psb_sgx_lab_sgx_state_query.c](/root/Poulsbo/workloads/psb_sgx_lab_sgx_state_query.c)
- it supports:
  - `cached`
  - `capture`
- it prints the typed SGX snapshot fields returned by the ioctl

Runner-side:

- new direct live validator:
  - [run-vaio-sgx-ioctl-sgx-state.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-sgx-state.sh)
- it stages the new helper to the VAIO
- it compares:
  - ioctl-captured snapshot
  - ioctl-cached snapshot
  - sysfs-captured snapshot
- it does that both before and after the validated 2D lab sequence

## Local Build And Deploy

Rebuilt locally with:

- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy result:

- `driver_srcversion=85342D8F2786B99DF520799`
- `override_module_size=382696`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Hardware Validation

The live bundle is:

- [logs/vaio-ioctl/20260511-112522-sgx-state-query-live](/root/Poulsbo/logs/vaio-ioctl/20260511-112522-sgx-state-query-live)

The target remained clean after the run:

- `overall=remote_usable`
- `driver_srcversion=85342D8F2786B99DF520799`
- `override_module_size=382696`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Observed Result

Before the 2D sequence:

- `IOCTL_SGX_STATE_CAPTURE_BEFORE` succeeded with:
  - `sgx_state_ioctl_ret=0 errno=0`
  - `sgx_state_reason=ioctl-sgx-state`
  - `sgx_state_sequence=1`
  - `sgx_state_irq_count=0`
  - `sgx_state_fault_count=0`
  - `sgx_state_bif_fault=0x0`
  - `sgx_state_twod_blit_status=0x0`
- `IOCTL_SGX_STATE_CACHED_BEFORE` returned the same snapshot:
  - `sgx_state_reason=ioctl-sgx-state`
  - `sgx_state_sequence=1`
- `SYSFS_SGX_STATE_BEFORE` then advanced the kernel snapshot to:
  - `reason=poulsbo_sgx_state`
  - `sequence=2`
- `IOCTL_SGX_STATE_AFTER_SYSFS_BEFORE` matched that sysfs-triggered snapshot:
  - `sgx_state_reason=poulsbo_sgx_state`
  - `sgx_state_sequence=2`

After the validated 2D lab sequence:

- `IOCTL_SGX_STATE_CAPTURE_AFTER` succeeded with:
  - `sgx_state_reason=ioctl-sgx-state`
  - `sgx_state_sequence=14`
  - `sgx_state_irq_count=7`
  - `sgx_state_fault_count=0`
  - `sgx_state_bif_fault=0x0`
  - `sgx_state_twod_blit_status=0xe`
- `IOCTL_SGX_STATE_CACHED_AFTER` returned the same snapshot:
  - `sgx_state_sequence=14`
  - `sgx_state_irq_count=7`
  - `sgx_state_twod_blit_status=0xe`
- `SYSFS_SGX_STATE_AFTER` then advanced the kernel snapshot to:
  - `reason=poulsbo_sgx_state`
  - `sequence=15`
  - `irq_count=7`
  - `fault_count=0`
  - `2d_blit_status=0x00000e`
- `IOCTL_SGX_STATE_AFTER_SYSFS_AFTER` matched that sysfs-triggered snapshot:
  - `sgx_state_reason=poulsbo_sgx_state`
  - `sgx_state_sequence=15`
  - `sgx_state_irq_count=7`
  - `sgx_state_fault_count=0`
  - `sgx_state_twod_blit_status=0xe`

The saved kernel log tail in the same bundle shows the expected 2D activity:

- `2D fillrect submit`
- `2D copyarea submit`
- `2D imageblit submit-chunked`
- repeated `SGX irq ... [TWOD_COMPLETE]`

## Conclusion

The live SGX snapshot path is no longer sysfs-only.

The new bounded SGX-state query ioctl is now validated on the VAIO in both:

- direct capture mode
- cached-read mode after an ioctl-triggered capture and after a sysfs-triggered
  capture

That means the remaining read-side sysfs dependence is narrower again:

- human-readable GTT dumps
- stale cleanup convenience
- any future broader state growth that still lacks a bounded typed query
