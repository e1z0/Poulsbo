# SGX Submit TA Request Sequence Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 92CB8FBC111248DC82C2825`

## Goal

The validated top submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_request_packet`

That layer unified the public request boundary plus the selected packet-side
read/write/CCB/ticket summary, but the live top path still had to pull the
request-level execution progression back out of lower nested state:

- request-side `issue_active` versus `transition_active`
- refresh and counter update intent
- next issue / transition counts
- next scheduler ticket and submit-id state

This batch lifts that seam into one explicit request-sequence object above
`sgx_ta_request_packet`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal top object:

- `psb_sgx_lab_submit_sgx_ta_request_sequence`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_request_sequence_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_request_sequence_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_request_sequence_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_request_sequence()`

The new `sgx_ta_request_sequence` object now keeps together:

- the existing `sgx_ta_request_packet`
- the request-side execution-progress flags:
  - `issue_active`
  - `transition_active`
  - `refresh_views`
  - `increment_issue_count`
  - `increment_transition_count`
  - `update_last_epoch_jiffies`
  - `update_last_issue_jiffies`
  - `update_last_transition_jiffies`
- the request-side scheduler progress summary:
  - `issue_advanced`
  - `running_advanced`
  - `complete_advanced`
  - `active_running_changed`
  - `next_issue_count`
  - `next_transition_count`
  - `next_state`

That moves the live SGX top seam one level higher again:

- validator: `sgx_ta_request_sequence`
- `issue_submit()` compatibility path: `sgx_ta_request_sequence`
- `transition_submit()` compatibility path: `sgx_ta_request_sequence`
- submit-op ioctl path: `sgx_ta_request_sequence`

The live kernel traces now also emit those top-level sequence effects directly:

- `issue` logs now include:
  - `refresh_views`
  - `next_issue_count`
  - `issue_advanced`
  - `next_issue_ticket`
  - `next_issue_submit_id`
- `transition` logs now include:
  - `refresh_views`
  - `next_transition_count`
  - `running_advanced`
  - `complete_advanced`
  - `next_running_ticket`
  - `next_complete_ticket`
  - `active_running_changed`

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Hardware proof set:

- guarded deploy:
  [logs/vaio-deploy/20260514-001931-sgx-ta-request-sequence-deploy](/root/Poulsbo/logs/vaio-deploy/20260514-001931-sgx-ta-request-sequence-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-002126-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-002126-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-002243-sgx-submit-ta-request-sequence-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-002243-sgx-submit-ta-request-sequence-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-002324-sgx-multi-submit-ta-request-sequence-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-002324-sgx-multi-submit-ta-request-sequence-live/summary.log)

Final live module:

- `driver_srcversion=92CB8FBC111248DC82C2825`
- `override_module_size=481200`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=92CB8FBC111248DC82C2825`
- focused typed submit stayed terminally clean:
  - `submit_resolved_id=2`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`

## Typed Multi-Submit Evidence

- final typed multi-submit state also retired cleanly:
  - submit A final:
    - `submit_resolved_id=3`
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_resolved_id=4`
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`

## Result

The SGX-shaped kernel-internal runtime stack now has thirty-seven validated
layers:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`
- `psb_sgx_lab_submit_ccb_kick`
- `psb_sgx_lab_submit_sgx_ccb_kick`
- `psb_sgx_lab_submit_sgx_cmdta_shared`
- `psb_sgx_lab_submit_sgx_cmdta`
- `psb_sgx_lab_submit_sgx_ta_kick`
- `psb_sgx_lab_submit_sgx_ta_command`
- `psb_sgx_lab_submit_sgx_queued_ta_command`
- `psb_sgx_lab_submit_sgx_ta_submission`
- `psb_sgx_lab_submit_sgx_ta_execution`
- `psb_sgx_lab_submit_sgx_ta_timeline`
- `psb_sgx_lab_submit_sgx_ta_runtime`
- `psb_sgx_lab_submit_sgx_ta_gate`
- `psb_sgx_lab_submit_sgx_ta_control`
- `psb_sgx_lab_submit_sgx_ta_schedule`
- `psb_sgx_lab_submit_sgx_ta_dispatch`
- `psb_sgx_lab_submit_sgx_ta_path`
- `psb_sgx_lab_submit_sgx_ta_commit`
- `psb_sgx_lab_submit_sgx_ta_event`
- `psb_sgx_lab_submit_sgx_ta_work_item`
- `psb_sgx_lab_submit_sgx_ta_op`
- `psb_sgx_lab_submit_sgx_ta_scheduler`
- `psb_sgx_lab_submit_sgx_ta_sequence`
- `psb_sgx_lab_submit_sgx_ta_work_queue`
- `psb_sgx_lab_submit_sgx_ta_job`
- `psb_sgx_lab_submit_sgx_ta_request`
- `psb_sgx_lab_submit_sgx_ta_ioctl_request`
- `psb_sgx_lab_submit_sgx_ta_submit_op_request`
- `psb_sgx_lab_submit_sgx_ta_queue_item`
- `psb_sgx_lab_submit_sgx_ta_queue_command`
- `psb_sgx_lab_submit_sgx_ta_scheduler_entry`
- `psb_sgx_lab_submit_sgx_ta_queue_stream`
- `psb_sgx_lab_submit_sgx_ta_queue_packet`
- `psb_sgx_lab_submit_sgx_ta_request_packet`
- `psb_sgx_lab_submit_sgx_ta_request_sequence`

This is still lab-mode execution, not real SGX rendering, but the shared top
layer now owns the request-level refresh/count/ticket progression together
with the public request boundary and selected packet-side summary, so the live
submit-op path no longer has to reconstruct that sequencing seam from nested
`request`, `sequence`, and `scheduler` state.

The next useful step is the first explicit SGX microkernel-facing request
work packet, scheduler work entry, or queue-sequencing object above
`sgx_ta_request_sequence`.
