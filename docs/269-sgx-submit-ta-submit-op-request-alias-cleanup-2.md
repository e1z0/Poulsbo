## Goal

Remove the redundant public compatibility wrapper layer so the parsed sysfs
`issue` / `transition` path routes directly through the shared
`request_bridge` object while preserving the existing sysfs-side trace output.

## What changed

Updated:

- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`

Removed the dead compatibility wrapper layer:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_alias`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_alias()`

Kept the live top request path split on:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_bridge`
- `psb_sgx_lab_submit_sgx_ta_submit_op_request_ioctl`

## Shared execute boundary

After this cleanup:

- the parsed `submit_store()` `issue` path now uses
  `sgx_ta_submit_op_request_bridge`
- the parsed `submit_store()` `transition` path now uses
  `sgx_ta_submit_op_request_bridge`
- the ioctl `ISSUE` path still uses
  `sgx_ta_submit_op_request_ioctl`
- the ioctl `TRANSITION` path still uses
  `sgx_ta_submit_op_request_ioctl`

The parsed sysfs submit-store seam now sits directly on `request_bridge`,
while the ioctl public caller seam still sits directly on `request_ioctl`.

## Result

The top request split is now one layer smaller:

- `request_bridge`
- `request_ioctl`
- `request_terminal`
- `request_relay`

## Validation

Guarded deploy plus recovery completed successfully in:

- `logs/vaio-plan/20260516-000927-sgx-ta-submit-op-request-alias-cleanup-2-deploy`
- final live override:
  - `driver_srcversion=C2F79F87DD5B96B4419D95A`
  - `override_module_size=509052`
  - `overall=remote_usable`
  - `lightdm_active=inactive`
  - `kdump_service_active=active`

The exact no-redeploy `plan` proof passed in:

- `logs/vaio-plan-session/20260516-001447-sgx-plan-session`
- `session_outcome=completed`
- `plan_session_mode=recovery`
- `selected_phase_profile=complete`
- `final_driver_srcversion=C2F79F87DD5B96B4419D95A`

Focused direct hardware validation passed in:

- `logs/vaio-ioctl/20260516-001211-sgx-submit-ta-submit-op-request-alias-cleanup-2-live/summary.log`
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

Typed multi-submit validation also passed in:

- `logs/vaio-ioctl/20260516-001249-sgx-multi-submit-ta-submit-op-request-alias-cleanup-2-live/summary.log`
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
- `source_timestamp=20260516-001447`
