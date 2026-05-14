# SGX Submit TA Request Execution Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 8E3405DC7D157295EE2A78D`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_request_work_packet`

That layer unified the selected request-side packet summary plus the
request-side progression result, but the live `issue` and `transition` entry
points still had to carry their public submit-op request image beside the new
top object.

This batch lifts that last seam into one explicit request-execution object
above `sgx_ta_request_work_packet`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_request_execution`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_request_execution_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_request_execution_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_request_execution_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_request_execution()`

The new `sgx_ta_request_execution` object now keeps together:

- the existing `sgx_ta_request_work_packet`
- the copied public submit-op image:
  - `op`
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

That moves the shared live SGX top seam one level higher again:

- `issue_submit()` compatibility path: `sgx_ta_request_execution`
- `transition_submit()` compatibility path: `sgx_ta_request_execution`
- submit-op ioctl path: `sgx_ta_request_execution`

The old standalone `poulsbo_sgx_execute_submit_sgx_ta_request_work_packet()`
helper is gone. The live execute path is now owned entirely by the new
request-execution helper.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Hardware proof set:

- guarded deploy:
  [logs/vaio-deploy/20260514-120212-sgx-ta-request-execution-deploy](/root/Poulsbo/logs/vaio-deploy/20260514-120212-sgx-ta-request-execution-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-120405-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-120405-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-120508-sgx-submit-ta-request-execution-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-120508-sgx-submit-ta-request-execution-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-120508-sgx-multi-submit-ta-request-execution-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-120508-sgx-multi-submit-ta-request-execution-live/summary.log)

Final live module:

- `driver_srcversion=8E3405DC7D157295EE2A78D`
- `override_module_size=482316`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=8E3405DC7D157295EE2A78D`
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

`sgx_ta_request_execution` is now the validated live top request/execution
object above `sgx_ta_request_work_packet`. The new module survives guarded
reboot, exact `complete`, focused submit, and typed multi-submit on hardware,
while keeping the VAIO headless and remotely usable.
