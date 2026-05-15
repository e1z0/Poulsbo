# SGX Submit TA Submit-Op Request Entrypoint Object

## Goal

Lift the remaining duplicated public caller seam above
`sgx_ta_submit_op_request_alias` and `sgx_ta_submit_op_request_ioctl` so both
the compatibility wrappers and the ioctl `ISSUE` / `TRANSITION` switch route
through one selected top `request_entrypoint` object.

## What Changed

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_entrypoint`
- Added one new shared execute helper:
  - `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_entrypoint()`

## Shared Execute Boundary

The shared live execute boundary now changes only for the last duplicated
public caller seam:

- `issue_submit()` compatibility flow now uses
  `sgx_ta_submit_op_request_entrypoint`
- `transition_submit()` compatibility flow now uses
  `sgx_ta_submit_op_request_entrypoint`
- `psb_ioctl_sgx_lab_submit_op()` `ISSUE` path now uses
  `sgx_ta_submit_op_request_entrypoint`
- `psb_ioctl_sgx_lab_submit_op()` `TRANSITION` path now uses
  `sgx_ta_submit_op_request_entrypoint`
- the compatibility-side public path intentionally stays represented through
  `sgx_ta_submit_op_request_alias`
- the ioctl-side public path intentionally stays represented through
  `sgx_ta_submit_op_request_ioctl`

## Top Object

The new top object keeps together:

- the existing `sgx_ta_submit_op_request_alias`
- the existing `sgx_ta_submit_op_request_ioctl`
- the copied submit-op request image:
  - `request_op`
- the copied submit-op response image:
  - `response_op`
- the copied final effect-visible request/work outcome:
  - `outcome`
- the narrowed top request-entrypoint boundary:
  - `active`
  - `dependency_fields_present`
  - `compatibility_request_entrypoint`
  - `ioctl_request_entrypoint`
  - `issue_request_entrypoint`
  - `transition_request_entrypoint`
  - `resolved_submit_id`
  - `request_entrypoint_op`

## Validation

Guarded deploy plus exact recovery wrapper completed successfully in:

- `logs/vaio-plan/20260515-154713-sgx-plan-guarded-deploy`
- final live override:
  - `driver_srcversion=45DC6E0FCCD779A67680873`
  - `override_module_size=510160`
  - `overall=remote_usable`
  - `lightdm_active=inactive`
  - validated deploy bundle:
    - `kdump_service_active=active`

The exact no-redeploy `plan` proof on that module still passed:

- `logs/vaio-plan-session/20260515-154921-sgx-plan-session`
- `session_outcome=completed`
- `plan_session_mode=recovery`
- `selected_phase_profile=complete`
- `final_driver_srcversion=45DC6E0FCCD779A67680873`

Focused direct hardware validation passed and is now saved in:

- `logs/vaio-ioctl/20260515-155029-sgx-submit-ta-submit-op-request-entrypoint-live/summary.log`
- final focused proof:
  - `submit_resolved_id=5`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`

Typed multi-submit hardware validation also passed and is now saved in:

- `logs/vaio-ioctl/20260515-155120-sgx-multi-submit-ta-submit-op-request-entrypoint-live/summary.log`
- final typed run proved:
  - `SUBMIT_A_ID=7`
  - `SUBMIT_B_ID=8`
  - submit A final:
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`

The shared current-boundary resolver therefore still holds terminal:

- `resolved_profile=complete`
- `resolved_reason=latest_phase_evidence_terminal`
- `source_timestamp=20260515-154921`

## Result

The last duplicated public caller seam now routes through one selected top
`request_entrypoint` object while preserving the intentionally split public
surfaces:

- `request_entrypoint`
- `request_alias`
- `request_ioctl`
- `request_public`
- `request_adapter`
- `request_bridge`
- `request_relay`
- `request_terminal`
