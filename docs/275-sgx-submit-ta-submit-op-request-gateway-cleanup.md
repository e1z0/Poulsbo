## Goal

Remove the redundant shared top request wrapper so both parsed sysfs and ioctl
`ISSUE` / `TRANSITION` submit-entry paths now target `request_endpoint`
directly.

## What changed

Updated:

- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`

Removed the dead wrapper layer:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_gateway`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_gateway()`

The shared top request path is now unified:

- parsed sysfs `issue` path -> `sgx_ta_submit_op_request_endpoint`
- parsed sysfs `transition` path -> `sgx_ta_submit_op_request_endpoint`
- ioctl `ISSUE` path -> `sgx_ta_submit_op_request_endpoint`
- ioctl `TRANSITION` path -> `sgx_ta_submit_op_request_endpoint`

That leaves the live top request boundary on:

- `request_endpoint`

## Validation

Guarded deploy plus recovery completed successfully in:

- `logs/vaio-plan/20260516-015443-sgx-ta-submit-op-request-gateway-cleanup-deploy`
- final live override:
  - `driver_srcversion=08C202815096288E5AE44D6`
  - `override_module_size=506392`
  - `overall=remote_usable`
  - `lightdm_active=inactive`
  - validated deploy bundle:
    - `kdump_service_active=active`

The exact no-redeploy `plan` proof on that module still passed:

- `logs/vaio-plan-session/20260516-015719-sgx-plan-session`
- `session_outcome=completed`
- `selected_child_phase_profile=complete`
- `selected_child_phase_selection=explicit`
- `selected_child_phases=complete`
- `final_driver_srcversion=08C202815096288E5AE44D6`

Focused direct hardware validation passed and is saved in:

- `logs/vaio-ioctl/20260516-015719-sgx-submit-ta-submit-op-request-gateway-cleanup-live/summary.log`
- final focused proof:
  - `submit_resolved_id=1`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`

Typed multi-submit hardware validation also passed and is saved in:

- `logs/vaio-ioctl/20260516-015719-sgx-multi-submit-ta-submit-op-request-gateway-cleanup-live/summary.log`
- final typed run proved:
  - `SUBMIT_A_ID=2`
  - `SUBMIT_B_ID=3`
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
- `source_timestamp=20260516-015719`
