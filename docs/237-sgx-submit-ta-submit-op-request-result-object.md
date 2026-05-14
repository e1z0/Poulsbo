# SGX Submit TA Submit-Op Request Result Object

Date: 2026-05-15
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion C1B63173499BF0FAF42F76C`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_dispatch`

That layer unified the copied request/response/outcome seam, the outer
request-entry seam, the outer request-call seam, and the outer
issue-versus-transition request-dispatch seam, but the final outer
request-result seam still sat open-coded in
`poulsbo_sgx_issue_submit_op_request()` and
`poulsbo_sgx_transition_submit_op_request()`. This batch lifts that seam into
one explicit `sgx_ta_submit_op_request_result` object above
`sgx_ta_submit_op_request_dispatch`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_result`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_submit_op_request_result_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_result_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_result_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_result()`

The new `sgx_ta_submit_op_request_result` object now keeps together:

- the existing `sgx_ta_submit_op_request_dispatch`
- the copied caller-visible submit-op request image:
  - `request_op`
- the copied caller-visible submit-op response image:
  - `response_op`
- the copied final effect-visible request/work outcome:
  - `outcome`
- the remaining top request-result boundary:
  - `active`
  - `dependency_fields_present`
  - `issue_request_result`
  - `transition_request_result`
  - `resolved_submit_id`
  - `request_result_op`

That moves the shared live SGX top seam one level higher again:

- submit-op ioctl path: `sgx_ta_submit_op_request_result`
- `issue_submit()` compatibility path: `sgx_ta_submit_op_request_result`
- `transition_submit()` compatibility path: `sgx_ta_submit_op_request_result`

The old submit-op-request-dispatch execute helper is now gone. The shared
wrappers now build, validate, apply, and return one top request-result object
whose nested request-dispatch layer still owns the copied request/response/
outcome seam plus the lower outer request seams, while the new top layer owns
the final request-result seam itself.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy wrapper:
  [logs/vaio-plan/20260515-010522-sgx-plan-guarded-deploy](/root/Poulsbo/logs/vaio-plan/20260515-010522-sgx-plan-guarded-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260515-010803-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260515-010803-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260515-010919-sgx-submit-ta-submit-op-request-result-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260515-010919-sgx-submit-ta-submit-op-request-result-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260515-011009-sgx-multi-submit-ta-submit-op-request-result-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260515-011009-sgx-multi-submit-ta-submit-op-request-result-live/summary.log)

Final live module:

- `driver_srcversion=C1B63173499BF0FAF42F76C`
- `override_module_size=503676`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` recovery proof passed on the deployed module:
  - `selected_phase_profile=complete`
  - `session_outcome=no_bad_prefix_found`
  - `bisect_outcome=all_prefixes_healthy`
  - `final_driver_srcversion=C1B63173499BF0FAF42F76C`
- focused typed submit stayed terminally clean:
  - `submit_resolved_id=6`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- typed multi-submit also stayed clean:
  - `SUBMIT_A_ID=8`
  - `SUBMIT_B_ID=9`
  - submit A final:
    - `submit_prepare_count=1`
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_prepare_count=1`
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=6`

## Conclusion

`sgx_ta_submit_op_request_result` is now the validated live top submit-op
request-result object above `sgx_ta_submit_op_request_dispatch`. The final
module survives guarded reboot, exact `complete`, focused submit, and typed
multi-submit on hardware, while keeping the VAIO headless and remotely usable.
