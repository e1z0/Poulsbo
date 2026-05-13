# SGX Submit TA Queue Stream Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion D814CEE2FD7F50C9ECE148D`

## Goal

The validated top submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_scheduler_entry`

That layer unified the full before/after driver-global scheduler state with the
full before/after queue-command summary, but the selected packet-side queue
image was still buried lower in the SGX path:

- the chosen `queue header + ccb record + sync + owner` stream image was still
  implicit inside `issue_image` or `transition_image`
- the top mutation wrappers still consumed queue-side stream fields indirectly
- there was still no explicit top object that owned both the scheduler-entry
  summary and the selected queue-command stream payload

This batch lifts that seam into one explicit queue-stream object above
`sgx_ta_scheduler_entry`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal top object:

- `psb_sgx_lab_submit_sgx_ta_queue_stream`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_queue_stream_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_queue_stream_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_queue_stream_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_queue_stream()`

The new `sgx_ta_queue_stream` object now keeps together:

- the existing `sgx_ta_scheduler_entry`
- the selected stream kind:
  - `issue`
  - `transition`
- the selected packet-side queue stream payload:
  - `queue_header`
  - `ccb_record`
  - `sync`
  - `owner`
- the selected stream bounds:
  - `queue_region_end`
  - `sync_region_end`
  - `owner_region_end`
- the selected stream slot offset:
  - `stream_ccb_offset`

That moves the live SGX top seam one level higher again:

- validator: `sgx_ta_queue_stream`
- `issue_submit()` compatibility path: `sgx_ta_queue_stream`
- `transition_submit()` compatibility path: `sgx_ta_queue_stream`
- submit-op ioctl path: `sgx_ta_queue_stream`

The first live draft over-constrained the queue-stream validator and rejected a
real focused `issue` submit at `launch -> issue` with `-EINVAL`. The repair was
to validate only the fields that the selected stream image actually owns, while
leaving the full scheduler/global-state proof with the nested
`sgx_ta_scheduler_entry` validator.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical proof set:

- repaired guarded deploy:
  [logs/vaio-deploy/20260513-225919-sgx-ta-queue-stream-rerun-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-225919-sgx-ta-queue-stream-rerun-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-230104-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-230104-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-230207-sgx-submit-ta-queue-stream-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-230207-sgx-submit-ta-queue-stream-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-230247-sgx-multi-submit-ta-queue-stream-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-230247-sgx-multi-submit-ta-queue-stream-live/summary.log)

Final live module:

- `driver_srcversion=D814CEE2FD7F50C9ECE148D`
- `override_module_size=477908`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=D814CEE2FD7F50C9ECE148D`
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

The SGX-shaped kernel-internal runtime stack now has thirty-four validated
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

This is still lab-mode execution, not real SGX rendering, but the shared top
layer now owns both the full before/after scheduler-entry image and the
selected queue-command stream payload consumed by the live mutation wrappers,
so the top submit-op path finally carries the actual `queue header + ccb record
+ sync + owner` image instead of stopping at the scheduler-entry summary above
that stream.

The next useful step is the first explicit SGX microkernel-facing request
packet, scheduler work packet, or queue-stream packet above
`sgx_ta_queue_stream`.
