# SGX Submit TA Submit-Op Request Endpoint Object

Date: 2026-05-15
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 0844E58E37B9982B2D11C3F`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_frontend`

That layer unified the copied request/response/outcome seam plus the final
caller-facing request wrapper boundary, but the outer request entrypoints still
open-coded the last shared execute / resolve / free seam in
`poulsbo_sgx_issue_submit_op_request()` and
`poulsbo_sgx_transition_submit_op_request()`. This batch lifts that seam into
one explicit `sgx_ta_submit_op_request_endpoint` object above
`sgx_ta_submit_op_request_frontend`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_endpoint`

New shared helper:

- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_endpoint()`

The new `sgx_ta_submit_op_request_endpoint` object now keeps together:

- the existing `sgx_ta_submit_op_request_frontend`
- the copied caller-visible submit-op request image:
  - `request_op`
- the copied caller-visible submit-op response image:
  - `response_op`
- the copied final effect-visible request/work outcome:
  - `outcome`
- the remaining top request-endpoint boundary:
  - `active`
  - `dependency_fields_present`
  - `issue_request_endpoint`
  - `transition_request_endpoint`
  - `resolved_submit_id`
  - `request_endpoint_op`

That moves the shared live SGX top seam one level higher again:

- submit-op ioctl path: `sgx_ta_submit_op_request_endpoint`
- `issue_submit()` compatibility path: `sgx_ta_submit_op_request_endpoint`
- `transition_submit()` compatibility path:
  `sgx_ta_submit_op_request_endpoint`

The older `sgx_ta_submit_op_request_frontend` helper stays as the nested
validated apply path, but the outer wrappers now consume and free one top
request-endpoint object whose nested request-frontend layer still owns the
lower outer request seams while the new top layer owns the final shared
execute / resolve / free boundary itself.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy wrapper:
  [logs/vaio-plan/20260515-035735-sgx-ta-submit-op-request-endpoint-deploy](/root/Poulsbo/logs/vaio-plan/20260515-035735-sgx-ta-submit-op-request-endpoint-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260515-035937-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260515-035937-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260515-040327-sgx-submit-ta-submit-op-request-endpoint-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260515-040327-sgx-submit-ta-submit-op-request-endpoint-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260515-040327-sgx-multi-submit-ta-submit-op-request-endpoint-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260515-040327-sgx-multi-submit-ta-submit-op-request-endpoint-live/summary.log)

Final live module:

- `driver_srcversion=0844E58E37B9982B2D11C3F`
- `override_module_size=506656`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` recovery proof passed on the deployed module:
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=0844E58E37B9982B2D11C3F`
- focused typed submit stayed terminally clean:
  - `submit_resolved_id=11`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- typed multi-submit also stayed clean:
  - `SUBMIT_A_ID=9`
  - `SUBMIT_B_ID=10`
  - submit A final:
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=10`

## Conclusion

`sgx_ta_submit_op_request_endpoint` is now the validated live top submit-op
request-endpoint object above `sgx_ta_submit_op_request_frontend`. The final
module survives guarded reboot, exact `complete`, focused submit, and typed
multi-submit on hardware, while keeping the VAIO headless and remotely usable.
