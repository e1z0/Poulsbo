# SGX Lab Cleanup Op Ioctl Validation

Date: 2026-05-11

## Scope

Add bounded typed cleanup ops to the existing SGX lab object-op and submit-op
ioctl surfaces so the ioctl-based preflight path no longer has to depend on
sysfs scrape-and-drop loops for stale submits, focus state, and adopted lab
objects.

This milestone covers:

- [third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
- [workloads/psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
- [workloads/psb_sgx_lab_object_op.c](/root/Poulsbo/workloads/psb_sgx_lab_object_op.c)
- [scripts/run-vaio-sgx-ioctl-cleanup.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-cleanup.sh)
- [scripts/run-vaio-sgx-ioctl-object-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-object-op.sh)
- [scripts/run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
- [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)

## What Was Added

Kernel-side:

- new typed submit-op verb:
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_CLEANUP`
- new typed object-op verb:
  - `DRM_PSB_SGX_LAB_OBJECT_OP_CLEANUP`
- new typed result field on both op structs:
  - `result_count`

The new cleanup ops use the kernel-owned SGX lab lists directly:

- submit cleanup:
  - drops all live lab submits
  - clears focused-submit state
  - clears any active running-owner state through the existing drop-all path
- object cleanup:
  - drops all live lab objects
  - reuses the existing object drop path, including PF unbind when needed

Userspace-side:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
  now supports:
  - `cleanup`
- [psb_sgx_lab_object_op.c](/root/Poulsbo/workloads/psb_sgx_lab_object_op.c)
  now supports:
  - `cleanup`
- both now print:
  - `submit_result_count`
  - `object_result_count`

Runner-side:

- new focused live validator:
  - [run-vaio-sgx-ioctl-cleanup.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-cleanup.sh)
- it:
  - stages `drm_probe`, `psb_sgx_lab_range_query`, `psb_sgx_lab_submit_op`,
    and `psb_sgx_lab_object_op`
  - seeds two held GEM ranges
  - adopts two objects from those ranges
  - PF-binds one object
  - creates two submits and focuses one
  - runs typed submit cleanup and typed object cleanup
  - verifies the post-cleanup state through sysfs and typed range-query readback
- the existing ioctl-based runners now prefer typed cleanup first and only fall
  back to the older sysfs drop loops for compatibility:
  - `run-vaio-sgx-ioctl-object-op.sh`
  - `run-vaio-sgx-ioctl-submit-op.sh`
  - `run-vaio-sgx-multi-submit.sh`

## Local Build And Deploy

Rebuilt locally with:

- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy result:

- `driver_srcversion=C142911E7A8FF2E7821CAE0`
- `override_module_size=384424`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Hardware Validation

The live bundle is:

- [logs/vaio-ioctl/20260511-111529-sgx-cleanup-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-111529-sgx-cleanup-op-live)

The target remained clean after the run:

- `overall=remote_usable`
- `driver_srcversion=C142911E7A8FF2E7821CAE0`
- `override_module_size=384424`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

Integrated runner follow-up:

- [logs/vaio-ioctl/20260511-111946-sgx-multi-submit-cleanup-rerun](/root/Poulsbo/logs/vaio-ioctl/20260511-111946-sgx-multi-submit-cleanup-rerun)
- that rerun proved the updated multi-submit helper actually took:
  - `STALE_SUBMIT_CLEANUP=ioctl`
  - `STALE_OBJECT_CLEANUP=ioctl`
- and still finished with:
  - `overall=remote_usable`

## Observed Result

The validator seeded stale state first:

- picked two live mmapping GEM ranges:
  - `RANGE_ID_A=5`
  - `RANGE_ID_B=7`
- adopted two objects:
  - object `1` on range `5`
  - object `2` on range `7`
- configured object `1` further:
  - `context=pf`
  - `gpu_use=control_ccb`
  - `pf_bind_count=1`
- created two submits and focused the first one

Before cleanup:

- `COORD_BEFORE_CLEANUP` showed:
  - `focused_submit_valid=1`
  - `focused_submit_id=1`
  - `live_submit_count=2`
- `SUBMITS_BEFORE_CLEANUP` still showed live lab submit state
- `OBJECTS_BEFORE_CLEANUP` showed:
  - `objects_shown=2`
  - object `1` still PF-bound
  - object `2` still adopted

Typed submit cleanup:

- `SUBMIT_CLEANUP` succeeded with:
  - `submit_op_name=cleanup`
  - `submit_op_ret=0 errno=0`
  - `submit_result_count=2`
- `COORD_AFTER_SUBMIT_CLEANUP` showed:
  - `focused_submit_valid=0`
  - `focused_submit_id=0`
  - `live_submit_count=0`
- `SUBMITS_AFTER_SUBMIT_CLEANUP` showed:
  - `submits_shown=0`
  - `submits_total=0`

Typed object cleanup:

- `OBJECT_CLEANUP` succeeded with:
  - `object_op_name=cleanup`
  - `object_op_ret=0 errno=0`
  - `object_result_count=2`
- `OBJECTS_AFTER_OBJECT_CLEANUP` showed:
  - `objects_shown=0`
  - `objects_total=0`

Post-cleanup range ownership and bind state were also clean:

- `GTT_AFTER_OBJECT_CLEANUP` showed:
  - range `5` with:
    - `sgx_object_id=0`
    - `pf_bind_count=0`
    - `pf_bind_total=1`
    - `pf_unbind_total=1`
  - range `7` with:
    - `sgx_object_id=0`
    - `pf_bind_count=0`
- direct typed range-query readback matched that:
  - `RANGE_A_AFTER_CLEANUP`:
    - `range_sgx_object_id=0`
    - `range_pf_bind_count=0`
    - `range_pf_bind_total=1`
    - `range_pf_unbind_total=1`
  - `RANGE_B_AFTER_CLEANUP`:
    - `range_sgx_object_id=0`
    - `range_pf_bind_count=0`

## Conclusion

The ioctl-based stale cleanup path is no longer dependent on sysfs scrape-and-
drop loops.

The new bounded cleanup ops are now validated on the VAIO for both:

- stale submit plus focused-submit cleanup
- stale adopted-object plus PF-bound-object cleanup

The updated multi-submit runner is also now confirmed to take the typed cleanup
branch on real hardware before its main scenario, not only in local syntax
tests.

That narrows the remaining sysfs dependence again:

- human-readable inspection dumps
- legacy compatibility fallback paths in older runners
- later dependency or state-block growth that still lacks a bounded ioctl
