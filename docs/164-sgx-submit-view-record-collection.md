# SGX Submit View-Record Collection

## Summary

The shared SGX lab submit live-view model in
`third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c` now has one
explicit collection layer for the three execution-shaped records that the
typed submit image cares about:

- `live`
- `plan`
- `result`

This batch adds `struct psb_sgx_lab_submit_view_records` plus
`poulsbo_sgx_collect_submit_view_records_locked()`, then routes refresh and
image validation through that collected record set instead of rebuilding each
typed view independently.

## Code Changes

- Added one shared collected-record container:
  - `struct psb_sgx_lab_submit_view_records`
- Added one shared collector:
  - `poulsbo_sgx_collect_submit_view_records_locked()`
- The collector now:
  - builds `live` once
  - resolves `plan_source` from stage
  - resolves `result_source` from stage
  - reuses `live` when `plan` or `result` already point at the same source
- Rewired these builders to consume the collected records:
  - `poulsbo_sgx_build_submit_queue_header_locked()`
  - `poulsbo_sgx_build_submit_ccb_record_locked()`
  - `poulsbo_sgx_build_submit_sync_locked()`
  - `poulsbo_sgx_build_submit_owner_locked()`
  - `poulsbo_sgx_build_submit_report_locked()`
  - `poulsbo_sgx_build_submit_manifest_locked()`
  - `poulsbo_sgx_build_submit_descriptor_locked()`
  - `poulsbo_sgx_build_submit_plan_locked()`
  - `poulsbo_sgx_build_submit_result_locked()`
- `poulsbo_sgx_preflight_submit_views_locked()` now collects the typed view
  records once per pass when any of:
  - `report`
  - `manifest`
  - `descriptor`
  - `plan`
  - `result`
  is configured
- Split the control-side and status-side backing-image validation back into
  smaller per-image helpers so the shared wrappers stay used and the validator
  path stays warning-clean.

## Validation

Guarded deploy:

- [logs/vaio-deploy/20260513-012143-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-012143-guarded-deploy)
- final live override after reboot:
  - `driver_srcversion=7E90BF7EA4740AF68A6B588`
  - `override_module_size=438196`
  - `overall=remote_usable`
  - `lightdm_active=inactive`
  - `kdump_service_active=active`

Exact terminal rerun:

- [logs/vaio-plan-session/20260513-012335-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-012335-sgx-plan-session)
- `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete`
- result:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=7E90BF7EA4740AF68A6B588`

Focused typed submit validation:

- [logs/vaio-ioctl/20260513-012435-sgx-submit-view-records-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-012435-sgx-submit-view-records-live/summary.log)
- final focused state:
  - `SUBMIT_ID=4`
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
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

Typed multi-submit validation:

- [logs/vaio-ioctl/20260513-012435-sgx-multi-submit-view-records-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-012435-sgx-multi-submit-view-records-live/summary.log)
- final multi-submit state:
  - `SUBMIT_A_ID=2`
  - `SUBMIT_B_ID=3`
  - submit A final:
    - `submit_prepare_count=1`
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
    - `submit_prepare_count=1`
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
- `source_timestamp=20260513-012335`
- `source_row_module_srcversion=7E90BF7EA4740AF68A6B588`

## Conclusion

The typed SGX lab submit image no longer decides `live`, `plan`, and `result`
source records ad hoc in each builder. It now collects that view state once
per pass, uses it across refresh and validation, and stays clean on the
current headless, panic-safe VAIO through exact `complete`, focused typed
submit validation, and typed multi-submit validation.
