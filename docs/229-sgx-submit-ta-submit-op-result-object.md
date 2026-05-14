# SGX Submit TA Submit-Op Result Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 87C32C9AB200EAC73718164`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_submit_op_route`

That layer unified the outer selected-route seam, but the shared wrappers still
reached back through nested route state to fetch the final effect-visible
request/work result before returning to the caller. This batch lifts that last
top result seam into one explicit `sgx_ta_submit_op_result` object above
`sgx_ta_submit_op_route`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_result`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_submit_op_result_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_submit_op_result_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_result_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_result()`

The new `sgx_ta_submit_op_result` object now keeps together:

- the existing `sgx_ta_submit_op_route`
- the copied caller-visible submit-op request image:
  - `request_op`
- the copied caller-visible submit-op response image:
  - `response_op`
- the copied final effect-visible request/work outcome:
  - `outcome`
- the remaining top result boundary:
  - `active`
  - `dependency_fields_present`
  - `use_focus_request`
  - `focus_valid`
  - `focus_resolved`
  - `issue_request`
  - `transition_request`
  - `issue_entry`
  - `transition_entry`
  - `issue_call`
  - `transition_call`
  - `issue_entrypoint`
  - `transition_entrypoint`
  - `issue_route`
  - `transition_route`
  - `issue_result`
  - `transition_result`
  - `refresh_views`
  - `refilled_from_complete`
  - `issue_advanced`
  - `running_advanced`
  - `complete_advanced`
  - `active_running_changed`
  - `selected_wrapped`
  - `submit_id`
  - `requested_submit_id`
  - `selected_submit_id`
  - `resolved_submit_id`
  - `ioctl_op`
  - `entry_op`
  - `call_op`
  - `entrypoint_op`
  - `route_op`
  - `result_op`
  - `op_kind`
  - `owner_state`
  - `packet_count`
  - `selected_read_offset`
  - `selected_write_offset`
  - `selected_ccb_offset`
  - `selected_outstanding_packets`
  - `selected_seq`
  - `selected_cookie`
  - `epoch`
  - `global_issue_ticket`
  - `global_running_ticket`
  - `global_complete_ticket`
  - `next_issue_count`
  - `next_transition_count`
  - `next_state`

That moves the shared live SGX top seam one level higher again:

- submit-op ioctl path: `sgx_ta_submit_op_result`
- `issue_submit()` compatibility path: `sgx_ta_submit_op_result`
- `transition_submit()` compatibility path: `sgx_ta_submit_op_result`

The old submit-op-route execute helper is now gone. The shared wrappers now
build, validate, apply, and return one top result object whose nested route
layer still owns the outer selected-route seam, while the new top layer owns
the copied request/response images, copied final `outcome`, and direct
`response_op.resolved_submit_id` return path.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy wrapper:
  [logs/vaio-plan/20260514-210407-sgx-ta-submit-op-result-deploy](/root/Poulsbo/logs/vaio-plan/20260514-210407-sgx-ta-submit-op-result-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-210559-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-210559-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-210711-sgx-submit-ta-submit-op-result-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-210711-sgx-submit-ta-submit-op-result-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-210754-sgx-multi-submit-ta-submit-op-result-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-210754-sgx-multi-submit-ta-submit-op-result-live/summary.log)

Final live module:

- `driver_srcversion=87C32C9AB200EAC73718164`
- `override_module_size=497412`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=87C32C9AB200EAC73718164`
- focused typed submit stayed terminally clean:
  - `submit_resolved_id=4`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- typed multi-submit also stayed clean:
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
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`

## Conclusion

`sgx_ta_submit_op_result` is now the validated live top submit-op result
object above `sgx_ta_submit_op_route`. The final module survives guarded
reboot, exact `complete`, focused submit, and typed multi-submit on hardware,
while keeping the VAIO headless and remotely usable.
