# SGX Submit TA Queue Item Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 6D6E13A5AC5A07DA2DE5EEC`

## Goal

The validated SGX-shaped submit stack already had a shared top submit-op
request object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request`

That layer unified the copied public submit-op packet and the resolved-submit
seam, but the next scheduler-facing seam was still open-coded in the live
mutation wrappers:

- `issue_submit()` still reached deep into nested issue-image state for
  queue/ticket logging
- `transition_submit()` still reached deep into nested commit-state fields for
  queue/ticket logging
- there was no single typed top object that owned the request plus the queue,
  ticket, and cookie effect summary that the wrappers actually report

This batch lifts that scheduler-facing seam into one explicit top object above
`sgx_ta_submit_op_request`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal top object:

- `psb_sgx_lab_submit_sgx_ta_queue_item`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_queue_item_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_queue_item_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_queue_item_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_queue_item()`

The new `sgx_ta_queue_item` object now keeps together:

- the existing `sgx_ta_submit_op_request`
- the top request split:
  - `issue_request`
  - `transition_request`
- the resolved queue effect summary:
  - `new_read_offset`
  - `new_write_offset`
  - `new_ccb_offset`
  - `outstanding_packets`
  - `wrapped`
  - `pending_seq`
  - `complete_seq`
  - `issue_cookie`
  - `complete_cookie`
  - `epoch`
- the global scheduler ticket summary:
  - `issue_ticket`
  - `running_ticket`
  - `complete_ticket`

That moves the live SGX top seam one level higher again:

- validator: `sgx_ta_queue_item`
- `issue_submit()` compatibility path: `sgx_ta_queue_item`
- `transition_submit()` compatibility path: `sgx_ta_queue_item`
- submit-op ioctl path: `sgx_ta_queue_item`

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

During the first local draft, the queue-item builder pulled an issue-side
`ccb_offset` from the wrong scheduler-state image. The local rebuild caught
that immediately, and the fix was to source the issue-side CCB position from
the issue image instead of the scheduler snapshot.

Canonical proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-220428-sgx-ta-queue-item-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-220428-sgx-ta-queue-item-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-220620-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-220620-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-220721-sgx-submit-ta-queue-item-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-220721-sgx-submit-ta-queue-item-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-220800-sgx-multi-submit-ta-queue-item-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-220800-sgx-multi-submit-ta-queue-item-live/summary.log)

Final live module:

- `driver_srcversion=6D6E13A5AC5A07DA2DE5EEC`
- `override_module_size=473084`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=6D6E13A5AC5A07DA2DE5EEC`
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

The SGX-shaped kernel-internal runtime stack now has thirty-one validated
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

This is still lab-mode execution, not real SGX rendering, but the shared top
layer now owns the queue/ticket effect summary that the live mutation wrappers
actually consume, so both compatibility entrypoints and the public submit-op
path now meet at one typed scheduler-facing queue item instead of drilling
through the whole lower object chain for those effect fields.

The next useful step is the first explicit SGX microkernel-facing request
packet, scheduler work entry, or real queue command object above
`sgx_ta_queue_item`.
