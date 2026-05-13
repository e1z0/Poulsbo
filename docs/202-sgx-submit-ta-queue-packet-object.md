# SGX Submit TA Queue Packet Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 6CB410CA76314429A21CE15`

## Goal

The validated top submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_queue_stream`

That layer unified the selected `queue header + ccb record + sync + owner`
payload with the full before/after scheduler-entry image, but the live top path
still reached back into nested queue-command and scheduler state to understand
what the selected packet was supposed to do.

This batch lifts that seam into one explicit queue-packet object above
`sgx_ta_queue_stream`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal top object:

- `psb_sgx_lab_submit_sgx_ta_queue_packet`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_queue_packet_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_queue_packet_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_queue_packet_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_queue_packet()`

The new `sgx_ta_queue_packet` object now keeps together:

- the existing `sgx_ta_queue_stream`
- the selected packet-side observable queue/sync summary:
  - `queue_read_offset`
  - `queue_write_offset`
  - `sync_read_offset`
  - `sync_write_offset`
  - `ccb_offset`
  - `queue_packet_count`
  - `queue_tail_packet_count`
  - `queue_head_packet_count`
  - `sync_complete_seq`
  - `sync_pending_seq`
  - `sync_flags`
  - `owner_issue_cookie`
  - `owner_complete_cookie`
  - `epoch`
- the selected packet envelope:
  - `queue_base_offset`
  - `queue_size`
  - `queue_packet_size`
- the selected queue-command/scheduler effect flags:
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
  - `scheduler_issue_advanced`
  - `scheduler_running_advanced`
  - `scheduler_complete_advanced`
- the selected next global ticket vector:
  - `global_issue_ticket`
  - `global_running_ticket`
  - `global_complete_ticket`

That moves the live SGX top seam one level higher again:

- validator: `sgx_ta_queue_packet`
- `issue_submit()` compatibility path: `sgx_ta_queue_packet`
- `transition_submit()` compatibility path: `sgx_ta_queue_packet`
- submit-op ioctl path: `sgx_ta_queue_packet`

## Failure And Repair

The first live draft over-constrained the new packet validator and rejected a
real focused `issue` submit at `launch -> issue` with `-EINVAL`.

The bad assumption was that the selected queue-stream payload owned the
post-apply global ticket vector. It does not. The repair was to keep the packet
object's `global_*_ticket` fields tied to the nested scheduler next-state while
dropping the cross-check against the selected queue-stream `sync/owner` payload.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

First-draft evidence:

- guarded deploy:
  [logs/vaio-deploy/20260513-231923-sgx-ta-queue-packet-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-231923-sgx-ta-queue-packet-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-232119-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-232119-sgx-plan-session)
- failing focused submit:
  [logs/vaio-ioctl/20260513-232311-sgx-submit-ta-queue-packet-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-232311-sgx-submit-ta-queue-packet-live/summary.log)
  - `submit issue failed`
  - `submit_op_ret=-1 errno=22`

Canonical repaired proof set:

- repaired guarded deploy:
  [logs/vaio-deploy/20260513-232500-sgx-ta-queue-packet-rerun-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-232500-sgx-ta-queue-packet-rerun-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-232651-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-232651-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-232750-sgx-submit-ta-queue-packet-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-232750-sgx-submit-ta-queue-packet-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-232833-sgx-multi-submit-ta-queue-packet-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-232833-sgx-multi-submit-ta-queue-packet-live/summary.log)

Final live module:

- `driver_srcversion=6CB410CA76314429A21CE15`
- `override_module_size=478876`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the repaired module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=6CB410CA76314429A21CE15`
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

The SGX-shaped kernel-internal runtime stack now has thirty-five validated
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

This is still lab-mode execution, not real SGX rendering, but the shared top
layer now owns the selected queue-command stream payload and the selected
packet-effect vector together, so the live submit-op path no longer reaches
back into nested queue-command or scheduler state to interpret what the chosen
packet is supposed to do.

The next useful step is the first explicit SGX microkernel-facing request
packet, scheduler work packet, or queue packet sequence above
`sgx_ta_queue_packet`.
