# SGX Submit TA Request Work-Action Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion F2C027054B485A0EB632D40`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_request_work_state`

That layer unified the exact post-apply top state image, but the live execute
helper still accepted raw `submit_op` request fields and request-resolution
state beside the top SGX object. This batch lifts that last caller-visible
submit-op seam into one explicit request-work-action object above
`sgx_ta_request_work_state`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_request_work_action`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_request_work_action_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_request_work_action_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_request_work_action_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_request_work_action()`

The new `sgx_ta_request_work_action` object now keeps together:

- the existing `sgx_ta_request_work_state`
- the copied public submit-op request image:
  - `op`
- the remaining request-resolution boundary:
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

That moves the shared live SGX top seam one level higher again:

- `issue_submit()` compatibility path: `sgx_ta_request_work_action`
- `transition_submit()` compatibility path: `sgx_ta_request_work_action`
- submit-op ioctl path: `sgx_ta_request_work_action`

The old standalone `poulsbo_sgx_execute_submit_sgx_ta_request_work_state()`
helper is removed. The live execute path is now owned entirely by the new
request-work-action helper.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy:
  [logs/vaio-deploy/20260514-160925-sgx-ta-request-work-action-deploy](/root/Poulsbo/logs/vaio-deploy/20260514-160925-sgx-ta-request-work-action-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-161114-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-161114-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-161217-sgx-submit-ta-request-work-action-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-161217-sgx-submit-ta-request-work-action-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-161217-sgx-multi-submit-ta-request-work-action-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-161217-sgx-multi-submit-ta-request-work-action-live/summary.log)

Final live module:

- `driver_srcversion=F2C027054B485A0EB632D40`
- `override_module_size=489416`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=F2C027054B485A0EB632D40`
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

`sgx_ta_request_work_action` is now the validated live top request/work action
object above `sgx_ta_request_work_state`. The new module survives guarded
reboot, exact `complete`, focused submit, and typed multi-submit on hardware,
while keeping the VAIO headless and remotely usable.
