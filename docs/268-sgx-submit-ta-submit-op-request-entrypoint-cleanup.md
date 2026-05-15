## Goal

Remove the redundant public caller selector layer so the parsed sysfs
`issue` / `transition` path routes directly through `request_alias` and the
ioctl `ISSUE` / `TRANSITION` path routes directly through `request_ioctl`.

## What changed

Updated:

- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`

Removed the dead selector layer:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_entrypoint`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_entrypoint()`

Kept the live top public path split on:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_alias`
- `psb_sgx_lab_submit_sgx_ta_submit_op_request_ioctl`

## Shared execute boundary

After this cleanup:

- the parsed `submit_store()` `issue` path now uses
  `sgx_ta_submit_op_request_alias`
- the parsed `submit_store()` `transition` path now uses
  `sgx_ta_submit_op_request_alias`
- the ioctl `ISSUE` path now uses
  `sgx_ta_submit_op_request_ioctl`
- the ioctl `TRANSITION` path now uses
  `sgx_ta_submit_op_request_ioctl`

The compatibility-side public caller seam now sits directly on
`request_alias`, and the ioctl-side public caller seam now sits directly on
`request_ioctl`.

## Result

The top request split is now one layer smaller:

- `request_alias`
- `request_ioctl`
- `request_bridge`
- `request_terminal`
- `request_relay`

## Validation

Guarded deploy plus recovery completed successfully in:

- `logs/vaio-plan/20260515-235518-sgx-ta-submit-op-request-entrypoint-cleanup-deploy`
- final live override:
  - `driver_srcversion=260DE5BEDE54552051085A3`
  - `override_module_size=509004`
  - `overall=remote_usable`
  - `lightdm_active=inactive`
  - `kdump_service_active=active`

The exact no-redeploy `plan` proof passed in:

- `logs/vaio-plan-session/20260515-235649-sgx-plan-session`
- `session_outcome=completed`
- `plan_session_mode=recovery`
- `selected_phase_profile=complete`
- `final_driver_srcversion=260DE5BEDE54552051085A3`

Focused direct hardware validation passed in:

- `logs/vaio-ioctl/20260515-235820-sgx-submit-ta-submit-op-request-entrypoint-cleanup-live/summary.log`
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

Typed multi-submit validation also passed in:

- `logs/vaio-ioctl/20260515-235912-sgx-multi-submit-ta-submit-op-request-entrypoint-cleanup-live/summary.log`
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
    - `dependency[1]_observed_complete_ticket=6`

The shared current-boundary resolver therefore still holds terminal:

- `resolved_profile=complete`
- `resolved_reason=latest_phase_evidence_terminal`
- `source_timestamp=20260515-235649`
