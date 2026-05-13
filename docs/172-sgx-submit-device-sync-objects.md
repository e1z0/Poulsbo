# SGX Submit Device Sync Objects

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 41BF53B53E121CEEB0F3594`

## Goal

The internal `psb_sgx_lab_submit_sgx_cmdta_shared` object already made the
`CMDTA_SHARED`-style SGX control/status topology explicit, but its sync side
was still too collapsed: one coarse `sgx_sync` shape stood in for distinct
device-sync records, there was no explicit hardware destination-sync list, and
the validator still had to carry a full `ccb_kick` scratch object on stack.

This batch keeps the same six-layer execution-shaped stack, but normalizes the
shared SGX sync side into explicit device-sync objects and keeps the validator
below the stack-frame warning threshold again.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal helper structures:

- `struct psb_sgx_lab_submit_sgx_device_sync_object`
- `struct psb_sgx_lab_submit_sgx_hwdevice_sync_list`
- `struct psb_sgx_lab_submit_sgx_ta3d_dependency`

Updated shared object:

- `struct psb_sgx_lab_submit_sgx_cmdta_shared`

New helper routines:

- `poulsbo_sgx_init_submit_sgx_device_sync_object()`
- `poulsbo_sgx_submit_sgx_device_sync_object_is_zero()`
- `poulsbo_sgx_submit_sgx_device_sync_object_equal()`

The `sgx_cmdta_shared` layer now carries explicit SGX-shaped sync records for:

- `ta_tq_sync`
- `threed_tq_sync`
- `ta3d_dependency_sync`
- `hwdevice_sync_list`

That means the shared builder now derives:

- read and write pending values separately
- read and write complete GPU VA32 addresses separately
- the explicit destination-sync list count and payload
- the 3D dependency sync object independently of the dependency metadata tuple

The validator now checks those objects directly:

- zero destination-sync state means zero TQ sync objects and zero destination
  sync-list contents
- nonzero destination-sync state means the first hardware destination-sync
  object must match the normalized TA TQ sync object
- zero 3D source-sync state means zero TA/3D dependency sync object
- nonzero 3D source-sync state means the dependency sync write-complete GPU VA
  must exist

The large validator frame is also gone again: the temporary `ccb_kick` scratch
used by `poulsbo_sgx_validate_submit_sgx_cmdta_shared_locked()` now lives on
heap instead of the function stack, so this batch does not reintroduce the old
`-Wframe-larger-than=1024` regression.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy:

- [logs/vaio-deploy/20260513-052151-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-052151-guarded-deploy)

Live target after deploy:

- `driver_srcversion=41BF53B53E121CEEB0F3594`
- `override_module_size=449960`
- `lightdm_active=inactive`
- `kdump_service_active=active`
- `overall=remote_usable`

Canonical live bundles:

- exact no-redeploy `plan` regression proof:
  - [logs/vaio-plan-session/20260513-052334-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-052334-sgx-plan-session)
- focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-052451-sgx-submit-device-sync-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-052451-sgx-submit-device-sync-live/summary.log)
- typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-052529-sgx-multi-submit-device-sync-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-052529-sgx-multi-submit-device-sync-live/summary.log)

## Focused Evidence

- the exact `complete` regression proof stayed terminally clean on the new
  module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=41BF53B53E121CEEB0F3594`
- focused typed submit still completed cleanly:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- final focused execution-shaped views stayed coherent:
  - `submit_live_stage_name=retired`
  - `submit_result_stage_name=retired`
  - `submit_manifest_stage_name=retired`
  - `submit_descriptor_stage_name=retired`
  - `submit_plan_stage_name=retired`
  - `submit_issue_plan_stage_name=retired`
  - `submit_issue_plan_dependency_active_count=0`
  - final cleanup:
    - `submit_dependency_active_count=0`
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`

## Typed Multi-Submit Evidence

- submit A still finished terminally clean on the new module:
  - `SUBMIT_A_ID=3`
  - `submit_prepare_count=1`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
  - `submit_issue_plan_stage_name=retired`
  - `submit_dependency_active_count=0`
- submit B retained explicit dependency state through the normalized shared
  views and still retired cleanly:
  - `SUBMIT_B_ID=4`
  - `submit_prepare_count=1`
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
  - `submit_result_dependency_active_count=1`
  - `submit_manifest_dependency_active_count=1`
  - `submit_descriptor_dependency_active_count=1`
  - `submit_plan_dependency_active_count=1`
  - `submit_dependency_active_count=1`
  - `submit_issue_plan_stage_name=retired`
- final cleanup returned to zero live SGX state:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

## Result

The execution-shaped kernel stack is still the same six-layer line:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`
- `psb_sgx_lab_submit_ccb_kick`
- `psb_sgx_lab_submit_sgx_ccb_kick`
- `psb_sgx_lab_submit_sgx_cmdta_shared`

What changed in this batch is that the top shared layer now models SGX-style
device sync objects and the hardware destination-sync list explicitly instead
of flattening them into one coarser sync record. That makes the sync side of
the shared object closer to mirrored Services4 `CMDTA_SHARED`/device-sync
shapes without changing the already-validated external submit behavior. The
next useful step is still the first real SGX execution object above
`sgx_cmdta_shared`, not another refactor-only pass over the same exact submit
ladder.
