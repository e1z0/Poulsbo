# SGX Submit TA Request Packet Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 3FA57ECCFB162117FD52520`

## Goal

The validated top submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_queue_packet`

That layer unified the selected queue packet payload, the selected
queue/sync/owner summary, and the selected packet-effect vector, but the live
top path still had to reason separately about the public request boundary:
which submit was being targeted, whether the operation was an `issue` or a
`transition`, and which selected read/write/CCB/ticket fields should apply for
that request.

This batch lifts that seam into one explicit request-packet object above
`sgx_ta_queue_packet`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal top object:

- `psb_sgx_lab_submit_sgx_ta_request_packet`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_request_packet_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_request_packet_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_request_packet_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_request_packet()`

The new `sgx_ta_request_packet` object now keeps together:

- the existing `sgx_ta_queue_packet`
- the public request boundary:
  - `requested_submit_id`
  - `resolved_submit_id`
  - `ioctl_op`
  - `op_kind`
  - `issue_request`
  - `transition_request`
  - `packet_active`
  - `refilled_from_complete`
- the selected request-side observable summary:
  - `selected_read_offset`
  - `selected_write_offset`
  - `selected_ccb_offset`
  - `selected_outstanding_packets`
  - `selected_wrapped`
  - `selected_seq`
  - `selected_cookie`
  - `epoch`
- the selected next global ticket vector:
  - `global_issue_ticket`
  - `global_running_ticket`
  - `global_complete_ticket`

That moves the live SGX top seam one level higher again:

- validator: `sgx_ta_request_packet`
- `issue_submit()` compatibility path: `sgx_ta_request_packet`
- `transition_submit()` compatibility path: `sgx_ta_request_packet`
- submit-op ioctl path: `sgx_ta_request_packet`

## Failure And Repair

The first live draft preserved the focused single-submit path, but it broke the
dependency-gated typed multi-submit path during the blocked `running`
transition.

The bug was in the new request-packet builder: it collapsed the lower-layer
`sgx_ta_queue_packet` return code to generic `-EINVAL`. That destroyed the
intended dependency-gated `-EAGAIN` result coming from the lower scheduler
gate and made the live wrapper treat a blocked dependent submit as a structural
failure.

The repair was to preserve and return the nested builder's real `ret` value
instead of rewriting every non-zero return to `-EINVAL`.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

First-draft evidence:

- guarded deploy:
  [logs/vaio-deploy/20260513-234413-sgx-ta-request-packet-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-234413-sgx-ta-request-packet-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-234558-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-234558-sgx-plan-session)
- the first typed multi-submit live rerun failed at the blocked dependent
  `running` transition because the request-packet builder rewrote the lower
  dependency gate result:
  - `submit_op_name=transition`
  - `submit_owner_state_name=running`
  - `submit_op_ret=-1 errno=22`
  - `submit_resolved_id=0`

Canonical repaired proof set:

- repaired guarded deploy:
  [logs/vaio-deploy/20260513-235006-sgx-ta-request-packet-rerun-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-235006-sgx-ta-request-packet-rerun-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-235155-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-235155-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-235251-sgx-submit-ta-request-packet-rerun-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-235251-sgx-submit-ta-request-packet-rerun-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-235347-sgx-multi-submit-ta-request-packet-rerun-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-235347-sgx-multi-submit-ta-request-packet-rerun-live/summary.log)

Final live module:

- `driver_srcversion=3FA57ECCFB162117FD52520`
- `override_module_size=480168`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the repaired module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=3FA57ECCFB162117FD52520`
- focused typed submit stayed terminally clean:
  - `submit_resolved_id=8`
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
    - `submit_resolved_id=9`
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_resolved_id=10`
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=17`

## Result

The SGX-shaped kernel-internal runtime stack now has thirty-six validated
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

This is still lab-mode execution, not real SGX rendering, but the shared top
layer now owns the public request boundary plus the selected packet-side
read/write/CCB/ticket summary together, so the live submit-op path no longer
has to flatten that seam back across nested queue-packet and request state.

The next useful step is the first explicit SGX microkernel-facing scheduler
work packet, request-sequence packet, or queue packet sequence above
`sgx_ta_request_packet`.
