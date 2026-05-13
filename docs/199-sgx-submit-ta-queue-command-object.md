# SGX Submit TA Queue Command Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 9D9E9D6EADBAAE4FD768742`

## Goal

The validated top submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_queue_item`

That layer unified the resolved submit-op request with the queue/ticket effect
summary that the live wrappers printed, but it still only exposed a partial
post-apply view:

- issue-side completion fields were implicit
- the previous queue/ticket state stayed open-coded in the wrappers
- there was still no single typed top object that owned the full before/after
  queue command image

This batch lifts that seam into one explicit queue-command object above
`sgx_ta_queue_item`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal top object:

- `psb_sgx_lab_submit_sgx_ta_queue_command`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_queue_command_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_queue_command_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_queue_command_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_queue_command()`

The new `sgx_ta_queue_command` object now keeps together:

- the existing `sgx_ta_queue_item`
- the full pre-apply queue state:
  - `prev_read_offset`
  - `prev_write_offset`
  - `prev_ccb_offset`
  - `prev_outstanding_packets`
  - `prev_wrapped`
  - `prev_pending_seq`
  - `prev_complete_seq`
  - `prev_issue_cookie`
  - `prev_complete_cookie`
  - `prev_epoch`
  - `prev_issue_ticket`
  - `prev_running_ticket`
  - `prev_complete_ticket`
- the full post-apply queue state:
  - `next_read_offset`
  - `next_write_offset`
  - `next_ccb_offset`
  - `next_outstanding_packets`
  - `next_wrapped`
  - `next_pending_seq`
  - `next_complete_seq`
  - `next_issue_cookie`
  - `next_complete_cookie`
  - `next_epoch`
  - `next_issue_ticket`
  - `next_running_ticket`
  - `next_complete_ticket`
- derived queue-command effect flags:
  - `read_advanced`
  - `write_advanced`
  - `ccb_advanced`
  - `outstanding_changed`
  - `pending_advanced`
  - `complete_advanced`
  - `issue_cookie_advanced`
  - `complete_cookie_advanced`
  - `epoch_advanced`
  - `issue_ticket_advanced`
  - `running_ticket_advanced`
  - `complete_ticket_advanced`

That moves the live SGX top seam one level higher again:

- validator: `sgx_ta_queue_command`
- `issue_submit()` compatibility path: `sgx_ta_queue_command`
- `transition_submit()` compatibility path: `sgx_ta_queue_command`
- submit-op ioctl path: `sgx_ta_queue_command`

During the first local draft, the queue-command builder pulled the driver-wide
ticket counters from the wrong field names in `drm_psb_private`. The local
rebuild caught that immediately, and the fix was to use the actual global
ticket counters:

- `sgx_lab_global_issue_ticket`
- `sgx_lab_global_running_ticket`
- `sgx_lab_global_complete_ticket`

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-222055-sgx-ta-queue-command-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-222055-sgx-ta-queue-command-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-222252-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-222252-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-222359-sgx-submit-ta-queue-command-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-222359-sgx-submit-ta-queue-command-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-222447-sgx-multi-submit-ta-queue-command-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-222447-sgx-multi-submit-ta-queue-command-live/summary.log)

Final live module:

- `driver_srcversion=9D9E9D6EADBAAE4FD768742`
- `override_module_size=474248`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=9D9E9D6EADBAAE4FD768742`
- focused typed submit stayed terminally clean:
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
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
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

The SGX-shaped kernel-internal runtime stack now has thirty-two validated
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

This is still lab-mode execution, not real SGX rendering, but the shared top
layer now owns the full before/after queue command image that the live
mutation wrappers actually consume, so both compatibility entrypoints and the
public submit-op path now meet at one typed queue command instead of rebuilding
the previous queue state and the post-apply queue/ticket state separately.

The next useful step is the first explicit SGX microkernel-facing request
packet, scheduler work entry, or real queue command stream object above
`sgx_ta_queue_command`.
