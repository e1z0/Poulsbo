# SGX Submit TA Submit-Op Request Portal Object

Date: 2026-05-15
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 53FC91A14B3946D5652FA82`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_gateway`

That layer unified the copied request/response/outcome seam plus the final
shared outer request-wrapper boundary, but the submit-op ioctl path still
open-coded the last issue-versus-transition request dispatch seam above the
outer request wrappers. This batch lifts that seam into one explicit
`sgx_ta_submit_op_request_portal` object above
`sgx_ta_submit_op_request_gateway`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_portal`

New shared helper:

- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_portal()`

The new `sgx_ta_submit_op_request_portal` object now keeps together:

- the existing `sgx_ta_submit_op_request_gateway`
- the copied caller-visible submit-op request image:
  - `request_op`
- the copied caller-visible submit-op response image:
  - `response_op`
- the copied final effect-visible request/work outcome:
  - `outcome`
- the remaining top request-portal boundary:
  - `active`
  - `dependency_fields_present`
  - `issue_request_portal`
  - `transition_request_portal`
  - `resolved_submit_id`
  - `request_portal_op`

That moves the shared live SGX top seam one level higher again:

- submit-op ioctl path: `sgx_ta_submit_op_request_portal`
- `issue_submit()` compatibility path: `sgx_ta_submit_op_request_portal`
- `transition_submit()` compatibility path:
  `sgx_ta_submit_op_request_portal`

The older `sgx_ta_submit_op_request_gateway` helper stays as the nested
validated apply path, but the outer wrappers and the submit-op ioctl path now
consume and free one top request-portal object whose nested request-gateway
layer still owns the lower outer request seams while the new top layer owns
the final caller-facing issue-versus-transition request dispatch boundary.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy wrapper:
  [logs/vaio-plan/20260515-042846-sgx-ta-submit-op-request-portal-deploy](/root/Poulsbo/logs/vaio-plan/20260515-042846-sgx-ta-submit-op-request-portal-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260515-043038-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260515-043038-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260515-043158-sgx-submit-ta-submit-op-request-portal-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260515-043158-sgx-submit-ta-submit-op-request-portal-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260515-043158-sgx-multi-submit-ta-submit-op-request-portal-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260515-043158-sgx-multi-submit-ta-submit-op-request-portal-live/summary.log)

Final live module:

- `driver_srcversion=53FC91A14B3946D5652FA82`
- `override_module_size=507688`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` recovery proof passed on the deployed module:
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=53FC91A14B3946D5652FA82`
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
  - `SUBMIT_A_ID=6`
  - `SUBMIT_B_ID=7`
  - submit A final:
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`

## Conclusion

`sgx_ta_submit_op_request_portal` is now the validated live top submit-op
request-portal object above `sgx_ta_submit_op_request_gateway`. The final
module survives guarded reboot, exact `complete`, focused submit, and typed
multi-submit on hardware, while keeping the VAIO headless and remotely usable.
