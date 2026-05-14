# SGX Submit TA Request Work-Submit-Op Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion C582E9B3D60213B4F150F67`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_request_work_invocation`

That layer unified the copied submit-op request image and the remaining live
request-selection fields, but the outer `psb_ioctl_sgx_lab_submit_op()` path
still resolved focus/direct submit selection and returned the resolved submit id
outside the shared SGX object chain. This batch lifts that remaining
submit-op selection boundary into one explicit request-work-submit-op object
above `sgx_ta_request_work_invocation`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_request_work_submit_op`

New shared helpers:

- `poulsbo_sgx_resolve_submit_op_submit_id_locked()`
- `poulsbo_sgx_build_submit_sgx_ta_request_work_submit_op_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_request_work_submit_op_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_request_work_submit_op_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_request_work_submit_op()`

The new `sgx_ta_request_work_submit_op` object now keeps together:

- the existing `sgx_ta_request_work_invocation`
- the copied public submit-op request image:
  - `op`
- the remaining top submit-op selection boundary:
  - `active`
  - `dependency_fields_present`
  - `use_focus_request`
  - `focus_valid`
  - `focus_resolved`
  - `issue_request`
  - `transition_request`
  - `submit_id`
  - `requested_submit_id`
  - `selected_submit_id`
  - `resolved_submit_id`
  - `ioctl_op`
  - `op_kind`

That moves the shared live SGX top seam one level higher again:

- submit-op ioctl path: `sgx_ta_request_work_submit_op`
- `issue_submit()` compatibility path: `sgx_ta_request_work_submit_op`
- `transition_submit()` compatibility path: `sgx_ta_request_work_submit_op`

The old standalone
`poulsbo_sgx_execute_submit_sgx_ta_request_work_invocation()` helper is now
removed. The live execute path is now owned entirely by the new
request-work-submit-op helper, including focus/direct submit selection and the
resolved submit-id return path.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy:
  [logs/vaio-deploy/20260514-174605-sgx-ta-request-work-submit-op-deploy](/root/Poulsbo/logs/vaio-deploy/20260514-174605-sgx-ta-request-work-submit-op-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-174749-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-174749-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-174851-sgx-submit-ta-request-work-submit-op-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-174851-sgx-submit-ta-request-work-submit-op-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-174930-sgx-multi-submit-ta-request-work-submit-op-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-174930-sgx-multi-submit-ta-request-work-submit-op-live/summary.log)

Final live module:

- `driver_srcversion=C582E9B3D60213B4F150F67`
- `override_module_size=493780`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=C582E9B3D60213B4F150F67`
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

`sgx_ta_request_work_submit_op` is now the validated live top submit-op object
above `sgx_ta_request_work_invocation`. The final module survives guarded
reboot, exact `complete`, focused submit, and typed multi-submit on hardware,
while keeping the VAIO headless and remotely usable.
