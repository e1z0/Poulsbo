# SGX Submit TA Request Work-Operation Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 112AF92C8FB3968C2AC9AA5`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_request_work_request`

That layer unified the copied public request image and request-side public
boundary, but the live execute path still logged and returned its final
effect-visible state through the nested `work_record`.

This batch lifts that last seam into one explicit request-work-operation object
above `sgx_ta_request_work_request`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_request_work_operation`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_request_work_operation_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_request_work_operation_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_request_work_operation_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_request_work_operation()`

The new `sgx_ta_request_work_operation` object now keeps together:

- the existing `sgx_ta_request_work_request`
- the copied public submit-op request image:
  - `op`
- the exact effect-visible top request/work state:
  - `active`
  - `dependency_fields_present`
  - `use_focus_request`
  - `focus_resolved`
  - `issue_request`
  - `transition_request`
  - `refresh_views`
  - `refilled_from_complete`
  - `issue_advanced`
  - `running_advanced`
  - `complete_advanced`
  - `active_running_changed`
  - `selected_wrapped`
  - `submit_id`
  - `requested_submit_id`
  - `resolved_submit_id`
  - `ioctl_op`
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

- `issue_submit()` compatibility path: `sgx_ta_request_work_operation`
- `transition_submit()` compatibility path: `sgx_ta_request_work_operation`
- submit-op ioctl path: `sgx_ta_request_work_operation`

The old standalone `poulsbo_sgx_execute_submit_sgx_ta_request_work_request()`
helper is removed. The live execute path is now owned entirely by the new
request-work-operation helper.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy:
  [logs/vaio-deploy/20260514-152607-sgx-ta-request-work-operation-deploy](/root/Poulsbo/logs/vaio-deploy/20260514-152607-sgx-ta-request-work-operation-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-152753-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-152753-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-152853-sgx-submit-ta-request-work-operation-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-152853-sgx-submit-ta-request-work-operation-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-152932-sgx-multi-submit-ta-request-work-operation-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-152932-sgx-multi-submit-ta-request-work-operation-live/summary.log)

Final live module:

- `driver_srcversion=112AF92C8FB3968C2AC9AA5`
- `override_module_size=487680`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=112AF92C8FB3968C2AC9AA5`
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

`sgx_ta_request_work_operation` is now the validated live top request/work
operation object above `sgx_ta_request_work_request`. The new module survives
guarded reboot, exact `complete`, focused submit, and typed multi-submit on
hardware, while keeping the VAIO headless and remotely usable.
