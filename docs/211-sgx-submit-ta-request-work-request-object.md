# SGX Submit TA Request Work-Request Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion EF5B87D1980AA1E469E8723`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_request_work_record`

That layer unified the exact live work record, but the live execute path still
carried the copied public submit-op request image beside that top object.

This batch lifts that seam into one explicit request-work-request object above
`sgx_ta_request_work_record`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_request_work_request`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_request_work_request_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_request_work_request_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_request_work_request_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_request_work_request()`

The new `sgx_ta_request_work_request` object now keeps together:

- the existing `sgx_ta_request_work_record`
- the copied public submit-op request image:
  - `op`
- the selected request-side public boundary:
  - `active`
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

- `issue_submit()` compatibility path: `sgx_ta_request_work_request`
- `transition_submit()` compatibility path: `sgx_ta_request_work_request`
- submit-op ioctl path: `sgx_ta_request_work_request`

The old standalone `poulsbo_sgx_execute_submit_sgx_ta_request_work_record()`
helper is removed. The live execute path is now owned entirely by the new
request-work-request helper.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Hardware proof set:

- guarded deploy:
  [logs/vaio-deploy/20260514-151223-sgx-ta-request-work-request-deploy](/root/Poulsbo/logs/vaio-deploy/20260514-151223-sgx-ta-request-work-request-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-151409-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-151409-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-151513-sgx-submit-ta-request-work-request-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-151513-sgx-submit-ta-request-work-request-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-151513-sgx-multi-submit-ta-request-work-request-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-151513-sgx-multi-submit-ta-request-work-request-live/summary.log)

Final live module:

- `driver_srcversion=EF5B87D1980AA1E469E8723`
- `override_module_size=487144`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=EF5B87D1980AA1E469E8723`
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
  - `SUBMIT_A_ID=2`
  - `SUBMIT_B_ID=3`
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
    - `dependency[1]_observed_complete_ticket=4`

## Conclusion

`sgx_ta_request_work_request` is now the validated live top request/work
request object above `sgx_ta_request_work_record`. The new module survives
guarded reboot, exact `complete`, focused submit, and typed multi-submit on
hardware, while keeping the VAIO headless and remotely usable.
