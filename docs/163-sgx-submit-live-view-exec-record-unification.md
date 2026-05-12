# SGX Submit Live-View Exec-Record Unification

## Summary

The shared kernel-internal execution-shaped submit record in
`third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c` now drives the
rest of the typed live submit memory model, not just the newer
memory-backed views.

After this batch, the shared execution record feeds:

- `queue header`
- `ccb record`
- `sync`
- `owner`
- `report`
- `manifest`
- `descriptor`
- `plan`
- `result`

This means the full typed SGX lab submit image is now built around one common
execution-facing source of truth instead of parallel per-view reconstructions.

## Code Changes

- Extended `struct psb_sgx_lab_submit_exec_record` with shared live/report
  counters:
  - `prepare_count`
  - `arm_count`
  - `emit_count`
  - `launch_count`
  - `dispatch_count`
  - `retired_count`
- Updated `poulsbo_sgx_build_submit_exec_record_locked()` so those counters are
  available across live, launch, dispatch, and retired sources.
- Rewired these builders to consume the shared execution record:
  - `poulsbo_sgx_build_submit_queue_header_locked()`
  - `poulsbo_sgx_build_submit_ccb_record_locked()`
  - `poulsbo_sgx_build_submit_sync_locked()`
  - `poulsbo_sgx_build_submit_owner_locked()`
  - `poulsbo_sgx_build_submit_report_locked()`

## Regression And Fix

The first live candidate from this batch exposed a real pre-queue regression.

Exact regression bundle:

- [logs/vaio-plan-session/20260513-004821-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-004821-sgx-plan-session)
- live override after that guarded deploy:
  - `driver_srcversion=A91AC3D851E88D083C04C9F`
- failing exact phase:
  - `selected_failed_phase=complete`
  - `selected_failed_reason=phase_failed`
  - `selected_failed_step=submit_op_ccb_focus`
  - `selected_failed_rc=1`

Cause:

- `poulsbo_sgx_build_submit_queue_header_locked()` started assuming queue
  geometry was already configured and tried to compute live queue progression
  even in the legal pre-queue `ccb` path.

Fix:

- `queue header` and `sync` builders now keep a valid pre-queue fallback:
  - skip queue progression until queue geometry exists
  - preserve zero packet geometry before `queue`
  - keep the pre-queue `submit_op ccb` path legal

## Validation

Final direct guarded deploy:

- [logs/vaio-deploy/20260513-005139-guarded-deploy-direct](/root/Poulsbo/logs/vaio-deploy/20260513-005139-guarded-deploy-direct)
- final live override after reboot:
  - `driver_srcversion=21D34B366D429B395AD45DC`
  - `override_module_size=434960`

Exact terminal rerun:

- [logs/vaio-plan-session/20260513-005336-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-005336-sgx-plan-session)
- `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete`
- result:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=21D34B366D429B395AD45DC`

Focused typed submit validation:

- [logs/vaio-ioctl/20260513-005719-sgx-submit-exec-record-full-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-005719-sgx-submit-exec-record-full-live/summary.log)
- final focused state:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - `submit_result_stage_name=retired`
  - `submit_result_source_name=retired`
  - `submit_result_owner_state_name=complete`

Typed multi-submit validation:

- [logs/vaio-ioctl/20260513-005759-sgx-multi-submit-exec-record-full-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-005759-sgx-multi-submit-exec-record-full-live/summary.log)
- final multi-submit state:
  - `SUBMIT_A_ID=6`
  - `SUBMIT_B_ID=7`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_result_stage_name=retired`
    - `submit_result_source_name=retired`
    - `submit_result_owner_state_name=complete`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_result_stage_name=retired`
    - `submit_result_source_name=retired`
    - `submit_result_owner_state_name=complete`
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`

Final exact boundary state:

- `resolved_profile=complete`
- `resolved_reason=latest_phase_evidence_terminal`
- `source_timestamp=20260513-005336`
- `source_row_module_srcversion=21D34B366D429B395AD45DC`

Final target state:

- `overall=remote_usable`
- `lightdm_active=inactive`
- `driver_srcversion=21D34B366D429B395AD45DC`

## Conclusion

The shared execution record now backs the whole typed SGX lab submit image,
including the older fixed queue/sync/owner/report blocks. The first candidate
proved the pre-queue `ccb` path still mattered; the final fix restored that
path and kept the full exact `complete`, focused submit, and typed multi-submit
hardware validation clean on the new module.
