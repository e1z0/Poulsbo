# SGX Submit TA Request Work-Entry Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion C35518A5768B47A82D11718`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_request_execution`

That layer unified the copied public submit-op request image with the selected
request-side execution summary, but the live `issue` and `transition` entry
points still had to log their final state through the nested
`execution->work_packet` seam.

This batch lifts that seam into one explicit request-work-entry object above
`sgx_ta_request_execution`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_request_work_entry`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_request_work_entry_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_request_work_entry_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_request_work_entry_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_request_work_entry()`

The new `sgx_ta_request_work_entry` object now keeps together:

- the existing `sgx_ta_request_execution`
- the selected request-side public boundary:
  - `dependency_fields_present`
  - `use_focus_request`
  - `focus_resolved`
  - `issue_request`
  - `transition_request`
  - `submit_id`
  - `requested_submit_id`
  - `resolved_submit_id`
  - `ioctl_op`
  - `op_kind`
  - `owner_state`
  - `packet_count`
- the selected request-side execution summary:
  - `packet_active`
  - `refilled_from_complete`
  - `refresh_views`
  - `increment_issue_count`
  - `increment_transition_count`
  - `update_last_epoch_jiffies`
  - `update_last_issue_jiffies`
  - `update_last_transition_jiffies`
  - `issue_advanced`
  - `running_advanced`
  - `complete_advanced`
  - `active_running_changed`
  - `selected_read_offset`
  - `selected_write_offset`
  - `selected_ccb_offset`
  - `selected_outstanding_packets`
  - `selected_wrapped`
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

- `issue_submit()` compatibility path: `sgx_ta_request_work_entry`
- `transition_submit()` compatibility path: `sgx_ta_request_work_entry`
- submit-op ioctl path: `sgx_ta_request_work_entry`

The old standalone `poulsbo_sgx_execute_submit_sgx_ta_request_execution()`
helper is removed. The live execute path is now owned entirely by the new
request-work-entry helper.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Hardware proof set:

- guarded deploy:
  [logs/vaio-deploy/20260514-135443-sgx-ta-request-work-entry-deploy](/root/Poulsbo/logs/vaio-deploy/20260514-135443-sgx-ta-request-work-entry-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-135631-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-135631-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-135734-sgx-submit-ta-request-work-entry-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-135734-sgx-submit-ta-request-work-entry-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-135734-sgx-multi-submit-ta-request-work-entry-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-135734-sgx-multi-submit-ta-request-work-entry-live/summary.log)

Final live module:

- `driver_srcversion=C35518A5768B47A82D11718`
- `override_module_size=484732`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=C35518A5768B47A82D11718`
- focused typed submit stayed terminally clean:
  - `submit_resolved_id=2`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- typed multi-submit also stayed clean:
  - submit A final:
    - `submit_resolved_id=3`
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_resolved_id=4`
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

`sgx_ta_request_work_entry` is now the validated live top request/work-entry
object above `sgx_ta_request_execution`. The new module survives guarded
reboot, exact `complete`, focused submit, and typed multi-submit on hardware,
while keeping the VAIO headless and remotely usable.
