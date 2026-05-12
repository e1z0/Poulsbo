# SGX Submit Exec Record Refactor

## Summary

The kernel now has one internal execution-shaped SGX submit record in
`third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`:

- `enum psb_sgx_lab_exec_source`
- `struct psb_sgx_lab_submit_exec_record`
- `poulsbo_sgx_build_submit_exec_record_locked()`

That record is now the common source for the memory-backed typed submit views:

- `manifest`
- `descriptor`
- `plan`
- `result`

This is the first kernel-internal structure in the lab path that represents the
submit's execution-facing state directly instead of rebuilding the same launch /
dispatch / retired / live picture separately inside each view builder.

## Code Changes

- Added shared execution-source selection:
  - `poulsbo_sgx_submit_plan_exec_source_locked()`
  - `poulsbo_sgx_submit_result_exec_source_locked()`
- Added shared GPU-VA resolution helper:
  - `poulsbo_sgx_resolve_object_gpu_va32_locked()`
- Refactored:
  - `poulsbo_sgx_build_submit_manifest_locked()`
  - `poulsbo_sgx_build_submit_descriptor_locked()`
  - `poulsbo_sgx_build_submit_plan_locked()`
  - `poulsbo_sgx_build_submit_result_locked()`

The main effect is that the SGX lab memory-backed submit views now consume one
shared internal execution record instead of each re-encoding the same source
state independently.

## Validation

Guarded deploy:

- [logs/vaio-deploy/20260513-002930-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-002930-guarded-deploy)
- live override after reboot:
  - `driver_srcversion=59347D903B1ABC5E00105D3`
  - `override_module_size=434080`

Exact regression:

- [logs/vaio-plan-session/20260513-003119-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-003119-sgx-plan-session)
- `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete`
- result:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=59347D903B1ABC5E00105D3`

Focused typed submit validation:

- [logs/vaio-ioctl/20260513-003228-sgx-submit-exec-record-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-003228-sgx-submit-exec-record-live/summary.log)
- final focused result state:
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

- [logs/vaio-ioctl/20260513-003228-sgx-multi-submit-exec-record-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-003228-sgx-multi-submit-exec-record-live/summary.log)
- final multi-submit result state:
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
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

Final target state:

- `overall=remote_usable`
- `lightdm_active=inactive`
- `driver_srcversion=59347D903B1ABC5E00105D3`

## Conclusion

The lab path now has a real kernel-internal execution record that sits between
raw submit state and the typed backing-memory views. The next step is to keep
moving that record toward a real SGX execution model instead of adding more
parallel view-specific state.
