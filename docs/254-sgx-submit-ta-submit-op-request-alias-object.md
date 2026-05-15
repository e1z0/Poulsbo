# SGX Submit TA Submit-Op Request Alias Object

## Goal

Lift the final public compatibility wrapper seam above
`psb_sgx_lab_submit_sgx_ta_submit_op_request_adapter` without rejoining the
already validated split between the compatibility side and the ioctl side of
the submit-op path.

## What Changed

- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_alias`
- Added one shared execute helper:
  - `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_alias()`
- Kept the narrowed split introduced by the previous batches:
  - `request_terminal` still owns the final ioctl-side caller seam
  - `request_relay` still owns the copied ioctl-side return seam
  - `request_bridge` still owns the copied outer wrapper seam
  - `request_adapter` still owns the outer compatibility caller seam
  - `request_alias` now owns the final public compatibility wrapper seam
- The new top object keeps together:
  - the existing `sgx_ta_submit_op_request_adapter`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the narrowed top request-alias boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_request_alias`
    - `transition_request_alias`
    - `resolved_submit_id`
    - `request_alias_op`

## Validation

- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260515-144948-sgx-ta-submit-op-request-alias-deploy`
  - final live override:
    - `driver_srcversion=BDCD208257504490F3641F3`
    - `override_module_size=509260`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module passed:
  - `logs/vaio-plan-session/20260515-145147-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=BDCD208257504490F3641F3`
- Focused direct hardware validation passed:
  - `logs/vaio-ioctl/20260515-145147-sgx-submit-ta-submit-op-request-alias-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=7`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed:
  - `logs/vaio-ioctl/20260515-145147-sgx-multi-submit-ta-submit-op-request-alias-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=5`
    - `SUBMIT_B_ID=6`
    - submit A final:
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=4`

## Conclusion

The final public compatibility wrapper seam is now routed through one shared
top `request_alias` object, while the compatibility caller seam remains on
`request_adapter` and the ioctl-side split remains on `request_terminal` and
`request_relay`. The live validated top quintet is now:

- `request_alias`
- `request_adapter`
- `request_bridge`
- `request_relay`
- `request_terminal`
