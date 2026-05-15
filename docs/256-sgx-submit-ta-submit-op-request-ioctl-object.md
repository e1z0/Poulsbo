# SGX Submit TA Submit-Op Request Ioctl Object

## Goal

Lift the final `ISSUE` / `TRANSITION` ioctl switch seam above
`psb_sgx_lab_submit_sgx_ta_submit_op_request_public` without disturbing the
already validated compatibility-side split that still ends at
`psb_sgx_lab_submit_sgx_ta_submit_op_request_alias`.

## What Changed

- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_ioctl`
- Added one shared execute helper:
  - `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_ioctl()`
- Kept the narrowed split introduced by the previous batches:
  - `request_alias` still owns the public compatibility wrapper seam
  - `request_public` still owns the public ioctl caller seam
  - `request_adapter` still owns the outer compatibility caller seam
  - `request_bridge` still owns the copied outer wrapper seam
  - `request_relay` still owns the copied ioctl-side return seam
  - `request_terminal` still owns the final ioctl-side caller seam
  - `request_ioctl` now owns the final ioctl `ISSUE` / `TRANSITION` switch
    seam in `psb_ioctl_sgx_lab_submit_op()`
- The new top object keeps together:
  - the existing `sgx_ta_submit_op_request_public`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the narrowed top request-ioctl boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_request_ioctl`
    - `transition_request_ioctl`
    - `resolved_submit_id`
    - `request_ioctl_op`

## Validation

- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260515-152659-sgx-plan-guarded-deploy`
  - final live override:
    - `driver_srcversion=0F1335BC6DA0C617739215E`
    - `override_module_size=509800`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - validated deploy bundle:
      - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module passed:
  - `logs/vaio-plan-session/20260515-152843-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=0F1335BC6DA0C617739215E`
- Focused direct hardware validation passed:
  - `logs/vaio-ioctl/20260515-153003-sgx-submit-ta-submit-op-request-ioctl-live/summary.log`
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
- Typed multi-submit hardware validation also passed:
  - `logs/vaio-ioctl/20260515-153045-sgx-multi-submit-ta-submit-op-request-ioctl-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=6`
    - `SUBMIT_B_ID=7`
    - submit A final:
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`

## Conclusion

The final ioctl `ISSUE` / `TRANSITION` switch seam is now routed through one
shared top `request_ioctl` object, while the compatibility-side public seam
remains on `request_alias`. The live validated top septet is now:

- `request_alias`
- `request_ioctl`
- `request_public`
- `request_adapter`
- `request_bridge`
- `request_relay`
- `request_terminal`
