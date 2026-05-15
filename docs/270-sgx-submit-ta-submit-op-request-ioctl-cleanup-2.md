## Goal

Remove the redundant ioctl public caller wrapper so the ioctl `ISSUE` /
`TRANSITION` path targets `request_terminal` directly while the parsed sysfs
submit-store seam stays on `request_bridge`.

## What changed

Updated:

- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`

Removed the dead wrapper layer:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_ioctl`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_ioctl()`

The shared top request split is now one layer smaller:

- parsed sysfs `issue` path -> `sgx_ta_submit_op_request_bridge`
- parsed sysfs `transition` path -> `sgx_ta_submit_op_request_bridge`
- ioctl `ISSUE` path -> `sgx_ta_submit_op_request_terminal`
- ioctl `TRANSITION` path -> `sgx_ta_submit_op_request_terminal`

That leaves the live top request split on:

- `request_bridge`
- `request_terminal`
- `request_relay`

## Validation

Guarded deploy plus recovery completed successfully in:

- `logs/vaio-plan/20260516-002431-sgx-ta-submit-op-request-ioctl-cleanup-2-deploy`
- final live override:
  - `driver_srcversion=C14AE9E3B5EE8296E20BA22`
  - `override_module_size=508816`
  - `overall=remote_usable`
  - `lightdm_active=inactive`
  - validated deploy bundle:
    - `kdump_service_active=active`

The exact no-redeploy `plan` proof on that module still passed:

- `logs/vaio-plan-session/20260516-005023-sgx-plan-session`
- `session_outcome=completed`
- `plan_session_mode=recovery`
- `selected_phase_profile=complete`
- `final_driver_srcversion=C14AE9E3B5EE8296E20BA22`

Focused direct hardware validation passed and is saved in:

- `logs/vaio-ioctl/20260516-004807-sgx-submit-ta-submit-op-request-ioctl-cleanup-2-live/summary.log`
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

Typed multi-submit hardware validation also passed and is saved in:

- `logs/vaio-ioctl/20260516-004847-sgx-multi-submit-ta-submit-op-request-ioctl-cleanup-2-live/summary.log`
- final typed run proved:
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
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
- `source_timestamp=20260516-005023`
