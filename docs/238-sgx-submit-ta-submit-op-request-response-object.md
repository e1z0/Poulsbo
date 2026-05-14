# SGX Submit TA Submit-Op Request Response Object

Date: 2026-05-15
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion FAB900381E110736AB21FFD`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_result`

That layer unified the copied request/response/outcome seam, the outer
request-entry seam, the outer request-call seam, the outer
issue-versus-transition request-dispatch seam, and the outer request-result
seam, but the final outer request-response seam still sat open-coded in
`poulsbo_sgx_issue_submit_op_request()` and
`poulsbo_sgx_transition_submit_op_request()`. This batch lifts that seam into
one explicit `sgx_ta_submit_op_request_response` object above
`sgx_ta_submit_op_request_result`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_response`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_submit_op_request_response_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_response_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_response_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_response()`

The new `sgx_ta_submit_op_request_response` object now keeps together:

- the existing `sgx_ta_submit_op_request_result`
- the copied caller-visible submit-op request image:
  - `request_op`
- the copied caller-visible submit-op response image:
  - `response_op`
- the copied final effect-visible request/work outcome:
  - `outcome`
- the remaining top request-response boundary:
  - `active`
  - `dependency_fields_present`
  - `issue_request_response`
  - `transition_request_response`
  - `resolved_submit_id`
  - `request_response_op`

That moves the shared live SGX top seam one level higher again:

- submit-op ioctl path: `sgx_ta_submit_op_request_response`
- `issue_submit()` compatibility path: `sgx_ta_submit_op_request_response`
- `transition_submit()` compatibility path: `sgx_ta_submit_op_request_response`

The old submit-op-request-result execute helper is now gone. The shared
wrappers now build, validate, apply, and return one top request-response
object whose nested request-result layer still owns the copied
request/response/outcome seam plus the lower outer request seams, while the
new top layer owns the final request-response seam itself.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy wrapper:
  [logs/vaio-plan/20260515-012406-sgx-plan-guarded-deploy](/root/Poulsbo/logs/vaio-plan/20260515-012406-sgx-plan-guarded-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260515-012613-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260515-012613-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260515-012724-sgx-submit-ta-submit-op-request-response-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260515-012724-sgx-submit-ta-submit-op-request-response-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260515-012818-sgx-multi-submit-ta-submit-op-request-response-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260515-012818-sgx-multi-submit-ta-submit-op-request-response-live/summary.log)

Final live module:

- `driver_srcversion=FAB900381E110736AB21FFD`
- `override_module_size=503968`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` recovery proof passed on the deployed module:
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=FAB900381E110736AB21FFD`
- focused typed submit stayed terminally clean:
  - `submit_resolved_id=5`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- typed multi-submit also stayed clean:
  - `SUBMIT_A_ID=7`
  - `SUBMIT_B_ID=8`
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
    - `dependency[1]_observed_complete_ticket=5`

## Conclusion

`sgx_ta_submit_op_request_response` is now the validated live top submit-op
request-response object above `sgx_ta_submit_op_request_result`. The final
module survives guarded reboot, exact `complete`, focused submit, and typed
multi-submit on hardware, while keeping the VAIO headless and remotely usable.
