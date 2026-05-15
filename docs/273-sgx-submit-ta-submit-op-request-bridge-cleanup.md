## Goal

Remove the redundant parsed-sysfs wrapper so both parsed sysfs and ioctl
`ISSUE` / `TRANSITION` submit-entry paths now target `request_portal`
directly.

## What changed

Updated:

- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`

Removed the dead wrapper layer:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_bridge`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_bridge()`

The shared top request path is now unified:

- parsed sysfs `issue` path -> `sgx_ta_submit_op_request_portal`
- parsed sysfs `transition` path -> `sgx_ta_submit_op_request_portal`
- ioctl `ISSUE` path -> `sgx_ta_submit_op_request_portal`
- ioctl `TRANSITION` path -> `sgx_ta_submit_op_request_portal`

That leaves the live top request boundary on:

- `request_portal`

## Validation

Guarded deploy plus recovery completed successfully in:

- `logs/vaio-plan/20260516-013112-sgx-ta-submit-op-request-bridge-cleanup-deploy`
- final live override:
  - `driver_srcversion=855E58C0139EEF34A51A14C`
  - `override_module_size=507512`
  - `overall=remote_usable`
  - `lightdm_active=inactive`
  - validated deploy bundle:
    - `kdump_service_active=active`

The exact no-redeploy `plan` proof on that module still passed:

- `logs/vaio-plan-session/20260516-013303-sgx-plan-session`
- `session_outcome=completed`
- `plan_session_mode=recovery`
- `selected_phase_profile=complete`
- `final_driver_srcversion=855E58C0139EEF34A51A14C`

Focused direct hardware validation passed and is saved in:

- `logs/vaio-ioctl/20260516-013303-sgx-submit-ta-submit-op-request-bridge-cleanup-live/summary.log`
- final focused proof:
  - `submit_resolved_id=3`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`

Typed multi-submit hardware validation also passed and is saved in:

- `logs/vaio-ioctl/20260516-013303-sgx-multi-submit-ta-submit-op-request-bridge-cleanup-live/summary.log`
- final typed run proved:
  - `SUBMIT_A_ID=1`
  - `SUBMIT_B_ID=2`
  - submit A final:
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=3`

The shared current-boundary resolver therefore still holds terminal:

- `resolved_profile=complete`
- `resolved_reason=latest_phase_evidence_terminal`
- `source_timestamp=20260516-013303`
