# SGX Submit TA Request Work-Invocation Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion FABCE60FE28BC5F9B747826`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_request_work_run`

That layer unified the copied submit-op request image and the exact top
request/execution result, but the live execute helper still accepted the raw
`(resolved_submit_id, op, dependency_fields_present)` tuple directly. This
batch lifts that remaining top request-selection boundary into one explicit
request-work-invocation object above `sgx_ta_request_work_run`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_request_work_invocation`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_request_work_invocation_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_request_work_invocation_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_request_work_invocation_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_request_work_invocation()`

The new `sgx_ta_request_work_invocation` object now keeps together:

- the existing `sgx_ta_request_work_run`
- the copied public submit-op request image:
  - `op`
- the remaining top request-selection boundary:
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

- `issue_submit()` compatibility path: `sgx_ta_request_work_invocation`
- `transition_submit()` compatibility path:
  `sgx_ta_request_work_invocation`
- submit-op ioctl path: `sgx_ta_request_work_invocation`

The old standalone `poulsbo_sgx_execute_submit_sgx_ta_request_work_run()`
helper is removed. The live execute path is now owned entirely by the new
request-work-invocation helper.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy:
  [logs/vaio-deploy/20260514-170003-sgx-ta-request-work-invocation-rerun-deploy](/root/Poulsbo/logs/vaio-deploy/20260514-170003-sgx-ta-request-work-invocation-rerun-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-170152-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-170152-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-170256-sgx-submit-ta-request-work-invocation-live-rerun/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-170256-sgx-submit-ta-request-work-invocation-live-rerun/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-170256-sgx-multi-submit-ta-request-work-invocation-live-rerun/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-170256-sgx-multi-submit-ta-request-work-invocation-live-rerun/summary.log)

Regression caught and fixed during bring-up:

- first guarded deploy:
  [logs/vaio-deploy/20260514-165414-sgx-ta-request-work-invocation-deploy](/root/Poulsbo/logs/vaio-deploy/20260514-165414-sgx-ta-request-work-invocation-deploy)
- first focused proof:
  [logs/vaio-ioctl/20260514-165701-sgx-submit-ta-request-work-invocation-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-165701-sgx-submit-ta-request-work-invocation-live/summary.log)
- first multi proof:
  [logs/vaio-ioctl/20260514-165702-sgx-multi-submit-ta-request-work-invocation-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-165702-sgx-multi-submit-ta-request-work-invocation-live/summary.log)

The initial draft incorrectly flattened nested nonzero returns in
`poulsbo_sgx_build_submit_sgx_ta_request_work_invocation_locked()`, which
turned the expected blocked-transition `-EAGAIN` into `-EINVAL` for the
dependent multi-submit path. The final committed module preserves the nested
return code again and passes the full proof set.

Final live module:

- `driver_srcversion=FABCE60FE28BC5F9B747826`
- `override_module_size=488560`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=FABCE60FE28BC5F9B747826`
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

`sgx_ta_request_work_invocation` is now the validated live top request/work
invocation object above `sgx_ta_request_work_run`. The final module survives
guarded reboot, exact `complete`, focused submit, and typed multi-submit on
hardware, while keeping the VAIO headless and remotely usable.
