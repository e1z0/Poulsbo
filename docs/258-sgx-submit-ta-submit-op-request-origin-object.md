# SGX Submit TA Submit-Op Request Origin Object

## Goal

Lift the final public caller-origin seam above
`sgx_ta_submit_op_request_entrypoint` so both the compatibility wrappers and
the ioctl `ISSUE` / `TRANSITION` switch route through one selected top
`request_origin` object.

## What Changed

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_origin`
- Added one new shared execute helper:
  - `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_origin()`

## Shared Execute Boundary

The shared live execute boundary now changes only for the final public
caller-origin seam:

- `issue_submit()` compatibility flow now uses
  `sgx_ta_submit_op_request_origin`
- `transition_submit()` compatibility flow now uses
  `sgx_ta_submit_op_request_origin`
- `psb_ioctl_sgx_lab_submit_op()` `ISSUE` path now uses
  `sgx_ta_submit_op_request_origin`
- `psb_ioctl_sgx_lab_submit_op()` `TRANSITION` path now uses
  `sgx_ta_submit_op_request_origin`
- the selected public caller seam intentionally stays represented through
  `sgx_ta_submit_op_request_entrypoint`
- the compatibility-side public path intentionally stays represented through
  `sgx_ta_submit_op_request_alias`
- the ioctl-side public path intentionally stays represented through
  `sgx_ta_submit_op_request_ioctl`

## Top Object

The new top object keeps together:

- the existing `sgx_ta_submit_op_request_entrypoint`
- the copied submit-op request image:
  - `request_op`
- the copied submit-op response image:
  - `response_op`
- the copied final effect-visible request/work outcome:
  - `outcome`
- the narrowed top request-origin boundary:
  - `active`
  - `dependency_fields_present`
  - `compatibility_request_origin`
  - `ioctl_request_origin`
  - `issue_request_origin`
  - `transition_request_origin`
  - `resolved_submit_id`
  - `request_origin_op`

## Validation

Guarded deploy plus exact recovery wrapper completed successfully in:

- `logs/vaio-plan/20260515-162849-sgx-ta-submit-op-request-origin-deploy`
- final live override:
  - `driver_srcversion=0969D38024C38FBFE3A9564`
  - `override_module_size=510232`
  - `overall=remote_usable`
  - `lightdm_active=inactive`
  - validated deploy bundle:
    - `kdump_service_active=active`

The exact no-redeploy `plan` proof on that module still passed:

- `logs/vaio-plan-session/20260515-163354-sgx-plan-session`
- `session_outcome=completed`
- `plan_session_mode=recovery`
- `selected_phase_profile=complete`
- `final_driver_srcversion=0969D38024C38FBFE3A9564`

Focused direct hardware validation passed and is now saved in:

- `logs/vaio-ioctl/20260515-163105-sgx-submit-ta-submit-op-request-origin-live/summary.log`
- final focused proof:
  - `submit_resolved_id=2`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`

Typed multi-submit hardware validation also passed and is now saved in:

- `logs/vaio-ioctl/20260515-163147-sgx-multi-submit-ta-submit-op-request-origin-live/summary.log`
- final typed run proved:
  - `SUBMIT_A_ID=4`
  - `SUBMIT_B_ID=5`
  - submit A final:
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=4`

The shared current-boundary resolver therefore still holds terminal:

- `resolved_profile=complete`
- `resolved_reason=latest_phase_evidence_terminal`
- `source_timestamp=20260515-163354`

## Result

The final public caller-origin seam now routes through one selected top
`request_origin` object while preserving the intentionally split public
surfaces underneath it:

- `request_origin`
- `request_entrypoint`
- `request_alias`
- `request_ioctl`
- `request_public`
- `request_adapter`
- `request_bridge`
- `request_relay`
- `request_terminal`
