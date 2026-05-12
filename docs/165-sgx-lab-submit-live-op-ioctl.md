# SGX Typed Submit `live` Block

## Summary

The typed SGX lab submit image now has a first-class control-backed `live`
block. This batch adds one explicit serialized `live` image to the control
object, exposes it through the typed submit query ioctl, and validates that
the recorded execution-facing state matches the live submit model through both
the focused single-submit path and the typed multi-submit path.

## Code Changes

- Added one new typed submit configure op:
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_LIVE`
- Extended the kernel submit query with typed `live` fields covering:
  - stage and per-stage counters
  - owner, queue, sync, and ticket state
  - control/status object ids and GPU VAs
  - CCB words
  - `live_config_count`
  - `last_live_jiffies`
- Added one new serialized control-backed image:
  - `struct psb_sgx_lab_live_words`
- Added shared kernel helpers for the `live` block:
  - build
  - read
  - write
  - refresh
  - image validation
- Extended control-layout collection and validation so `live` participates in:
  - overlap rejection
  - object access validation
  - refresh preflight
  - backing-image validation
- Wired the new block into:
  - the submit-op ioctl switch
  - typed submit query filling
  - the focused submit-op runner
  - the typed multi-submit runner
  - the userspace typed query and submit-op tools

## Validation

Guarded deploy:

- [logs/vaio-deploy/20260513-015019-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-015019-guarded-deploy)
- final live override after reboot:
  - `driver_srcversion=864D7160A07B237AB1A9D36`
  - `override_module_size=442596`
  - `overall=remote_usable`
  - `lightdm_active=inactive`
  - `kdump_service_active=active`

Exact terminal rerun:

- [logs/vaio-plan-session/20260513-015211-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-015211-sgx-plan-session)
- `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete`
- result:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=864D7160A07B237AB1A9D36`

Focused typed submit validation:

- [logs/vaio-ioctl/20260513-015552-sgx-submit-live-op-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-015552-sgx-submit-live-op-live/summary.log)
- final focused `live` state:
  - `SUBMIT_ID=5`
  - `submit_live_stage_name=retired`
  - `submit_live_stage_count=1`
  - `submit_live_prepare_count=1`
  - `submit_live_arm_count=1`
  - `submit_live_emit_count=1`
  - `submit_live_launch_count=1`
  - `submit_live_dispatch_count=1`
  - `submit_live_retired_count=1`
  - `submit_live_owner_state_name=complete`
  - `submit_live_dependency_active_count=0`
  - `submit_live_queue_packet_count=0`
  - `submit_live_queue_epoch=0`
  - `submit_live_control_gpu_va32=0x807bf000`
  - `submit_live_status_gpu_va32=0x807cf000`
  - `submit_live_pds_gpu_va32=0x807c3000`
  - `submit_live_use_gpu_va32=0x807c7000`
  - `submit_live_target_gpu_va32=0x807cb000`
  - `submit_live_ccb_status_gpu_va32=0x807cf000`
  - `submit_live_config_count=1`
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`

Typed multi-submit validation:

- [logs/vaio-ioctl/20260513-015629-sgx-multi-submit-live-op-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-015629-sgx-multi-submit-live-op-live/summary.log)
- final multi-submit ids:
  - `SUBMIT_A_ID=6`
  - `SUBMIT_B_ID=7`
- submit A final `live` state:
  - `submit_live_stage_name=retired`
  - `submit_live_stage_count=3`
  - `submit_live_prepare_count=1`
  - `submit_live_arm_count=3`
  - `submit_live_emit_count=3`
  - `submit_live_launch_count=3`
  - `submit_live_dispatch_count=3`
  - `submit_live_retired_count=3`
  - `submit_live_owner_state_name=complete`
  - `submit_live_dependency_active_count=0`
  - `submit_live_queue_packet_count=0`
  - `submit_live_queue_epoch=2`
  - `submit_live_control_gpu_va32=0x807bf000`
  - `submit_live_status_gpu_va32=0x807cf000`
  - `submit_live_pds_gpu_va32=0x807c3000`
  - `submit_live_use_gpu_va32=0x807c7000`
  - `submit_live_target_gpu_va32=0x807cb000`
  - `submit_live_ccb_status_gpu_va32=0x807cf000`
  - `submit_live_config_count=1`
- submit B final `live` state:
  - `submit_live_stage_name=retired`
  - `submit_live_stage_count=2`
  - `submit_live_prepare_count=1`
  - `submit_live_arm_count=2`
  - `submit_live_emit_count=2`
  - `submit_live_launch_count=2`
  - `submit_live_dispatch_count=2`
  - `submit_live_retired_count=2`
  - `submit_live_owner_state_name=complete`
  - `submit_live_dependency_active_count=1`
  - `submit_live_queue_packet_count=0`
  - `submit_live_queue_epoch=1`
  - `submit_live_control_gpu_va32=0x807d3000`
  - `submit_live_status_gpu_va32=0x807e3000`
  - `submit_live_pds_gpu_va32=0x807d7000`
  - `submit_live_use_gpu_va32=0x807db000`
  - `submit_live_target_gpu_va32=0x807df000`
  - `submit_live_ccb_status_gpu_va32=0x807e3000`
  - `submit_live_config_count=1`
- final cleanup:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

Final exact boundary state:

- `resolved_profile=complete`
- `resolved_reason=latest_phase_evidence_terminal`
- `source_timestamp=20260513-015211`
- `source_row_module_srcversion=864D7160A07B237AB1A9D36`

## Conclusion

The typed SGX lab submit image now has an explicit control-backed `live`
execution block instead of treating that state as implicit read-side context.
That block is live-validated on the current headless, panic-safe VAIO through
exact `complete`, focused typed submit validation, and typed multi-submit
validation, so the next useful work is above this view layer: the first more
execution-oriented SGX kernel structure that consumes the now-validated
`queue + ccb + sync + owner + report + live + result + manifest + descriptor
+ plan` model.
