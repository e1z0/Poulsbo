# SGX Submit TA Submit-Op Request Ioctl Cleanup

## Goal

Remove the now-redundant public ioctl-only request layer so the live
`ISSUE` / `TRANSITION` submit-op ioctl path still routes through the selected
top `request_ioctl` object, but now sits directly on `request_terminal`
instead of paying one extra execute hop through `request_public`.

## What Changed

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Removed the dead intermediate layer:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_public`
  - `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_public()`
- Kept the live ioctl submit-op top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_ioctl`

## Shared Execute Boundary

The shared live execute boundary now changes only by one cleanup:

- the ioctl `ISSUE` path still uses `sgx_ta_submit_op_request_ioctl`
- the ioctl `TRANSITION` path still uses `sgx_ta_submit_op_request_ioctl`
- the public ioctl submit-op seam now sits directly above
  `sgx_ta_submit_op_request_terminal`
- the parsed sysfs submit-store seam intentionally still stays on
  `sgx_ta_submit_op_request_store`

## Result

The top request split is one layer smaller and still preserves the same
intentional public separation:

- `request_store`
- `request_command`
- `request_origin`
- `request_entrypoint`
- `request_alias`
- `request_ioctl`
- `request_adapter`
- `request_bridge`
- `request_terminal`
- `request_relay`

## Validation

Guarded deploy completed successfully in:

- `logs/vaio-plan/20260515-210431-sgx-ta-submit-op-request-ioctl-cleanup-deploy`
- final live override:
  - `driver_srcversion=9C46B6BC078AC87963DFEF7`
  - `override_module_size=510740`
  - `overall=remote_usable`
  - `lightdm_active=inactive`
  - validated deploy bundle:
    - `kdump_service_active=active`

The exact no-redeploy `plan` proof on that module still passed:

- `logs/vaio-plan-session/20260515-210758-sgx-plan-session`
- `session_outcome=completed`
- `plan_session_mode=recovery`
- `selected_phase_profile=complete`
- `final_driver_srcversion=9C46B6BC078AC87963DFEF7`

Focused direct hardware validation passed and is now saved in:

- `logs/vaio-ioctl/20260515-210938-sgx-submit-ta-submit-op-request-ioctl-cleanup-live/summary.log`
- final focused proof:
  - `submit_resolved_id=6`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`

Typed multi-submit hardware validation also passed and is now saved in:

- `logs/vaio-ioctl/20260515-211024-sgx-multi-submit-ta-submit-op-request-ioctl-cleanup-live/summary.log`
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
- `source_timestamp=20260515-210758`
