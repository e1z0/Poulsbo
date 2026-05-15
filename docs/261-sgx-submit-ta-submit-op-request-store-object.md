# SGX Submit TA Submit-Op Request Store Object

## Goal

Lift the remaining parsed sysfs submit-store seam above
`sgx_ta_submit_op_request_sysfs` so the parsed `issue` / `transition`
sysfs store path routes through one selected top `request_store` object
without rejoining the ioctl-side public path.

## What Changed

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_store`
- Added one new shared execute helper:
  - `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_store()`
- Removed the now-unused thin wrapper helpers:
  - `poulsbo_sgx_issue_submit()`
  - `poulsbo_sgx_transition_submit()`

## Shared Execute Boundary

The shared live execute boundary now changes only for the parsed sysfs
submit-store seam:

- the parsed `submit_store()` `issue` path now uses
  `sgx_ta_submit_op_request_store`
- the parsed `submit_store()` `transition` path now uses
  `sgx_ta_submit_op_request_store`
- the parsed sysfs submit-store seam intentionally now sits above
  `sgx_ta_submit_op_request_sysfs`
- the ioctl `ISSUE` / `TRANSITION` switch intentionally stays on
  `sgx_ta_submit_op_request_origin`

## Top Object

The new top object keeps together:

- the existing `sgx_ta_submit_op_request_sysfs`
- the copied submit-op request image:
  - `request_op`
- the copied submit-op response image:
  - `response_op`
- the copied final effect-visible request/work outcome:
  - `outcome`
- the narrowed top request-store boundary:
  - `active`
  - `dependency_fields_present`
  - `compatibility_request_store`
  - `issue_request_store`
  - `transition_request_store`
  - `resolved_submit_id`
  - `request_store_op`

## Validation

Guarded deploy plus exact recovery wrapper completed successfully in:

- `logs/vaio-plan/20260515-202729-sgx-ta-submit-op-request-store-deploy`
- final live override:
  - `driver_srcversion=888811328462B2C6F18202B`
  - `override_module_size=511356`
  - `overall=remote_usable`
  - `lightdm_active=inactive`
  - validated deploy bundle:
    - `kdump_service_active=active`

The exact no-redeploy `plan` proof on that module still passed:

- `logs/vaio-plan-session/20260515-202918-sgx-plan-session`
- `session_outcome=completed`
- `plan_session_mode=recovery`
- `selected_phase_profile=complete`
- `final_driver_srcversion=888811328462B2C6F18202B`

Focused direct hardware validation passed and is now saved in:

- `logs/vaio-ioctl/20260515-203033-sgx-submit-ta-submit-op-request-store-live/summary.log`
- final focused proof:
  - `submit_resolved_id=4`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`

Typed multi-submit hardware validation also passed and is now saved in:

- `logs/vaio-ioctl/20260515-203122-sgx-multi-submit-ta-submit-op-request-store-live/summary.log`
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
- `source_timestamp=20260515-202918`

## Result

The parsed sysfs submit-store seam now routes through one selected top
`request_store` object while preserving the intentionally split public
surfaces underneath it:

- `request_store`
- `request_sysfs`
- `request_command`
- `request_origin`
- `request_entrypoint`
- `request_alias`
- `request_ioctl`
- `request_public`
- `request_adapter`
- `request_bridge`
- `request_relay`
- `request_terminal`
