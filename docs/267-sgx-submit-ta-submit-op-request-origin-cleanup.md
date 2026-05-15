## Goal

Remove the redundant public caller-origin layer so the parsed sysfs
`issue` / `transition` submit-store path and the ioctl `ISSUE` /
`TRANSITION` path both route directly through the shared
`request_entrypoint` object.

## What changed

Updated:

- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`

Removed the dead wrapper layer:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_origin`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_origin()`

Kept the shared top public caller seam on:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_entrypoint`

## Shared execute boundary

After this cleanup:

- the parsed `submit_store()` `issue` path now uses
  `sgx_ta_submit_op_request_entrypoint`
- the parsed `submit_store()` `transition` path now uses
  `sgx_ta_submit_op_request_entrypoint`
- the ioctl `ISSUE` path now uses
  `sgx_ta_submit_op_request_entrypoint`
- the ioctl `TRANSITION` path now uses
  `sgx_ta_submit_op_request_entrypoint`

The parsed sysfs submit-store seam and the ioctl public caller seam now both
sit directly on `request_entrypoint`.

## Result

The top request split is now one layer smaller:

- `request_entrypoint`
- `request_alias`
- `request_ioctl`
- `request_bridge`
- `request_terminal`
- `request_relay`

## Validation

Guarded deploy completed successfully in:

- `logs/vaio-plan/20260515-233741-sgx-ta-submit-op-request-origin-cleanup-deploy`
- final live override:
  - `driver_srcversion=DC33584BE74BE0302ABDD06`
  - `override_module_size=509368`
  - `overall=remote_usable`
  - `lightdm_active=inactive`
  - `kdump_service_active=active`

The exact no-redeploy `plan` proof passed in:

- `logs/vaio-plan-session/20260515-233937-sgx-plan-session`
- `session_outcome=completed`
- `plan_session_mode=recovery`
- `selected_phase_profile=complete`
- `final_driver_srcversion=DC33584BE74BE0302ABDD06`

Focused direct hardware validation passed in:

- `logs/vaio-ioctl/20260515-234051-sgx-submit-ta-submit-op-request-origin-cleanup-live/summary.log`
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

Typed multi-submit validation also passed in:

- `logs/vaio-ioctl/20260515-234148-sgx-multi-submit-ta-submit-op-request-origin-cleanup-live/summary.log`
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
- `source_timestamp=20260515-233937`
