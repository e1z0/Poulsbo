# SGX Submit TA Scheduler Entry Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 4B13FC7FC62A3A9F89B82A8`

## Goal

The validated top submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_queue_command`

That layer unified the full before/after queue-command image, but the
driver-global scheduler coordination seam was still implicit above it:

- the previous global submit-id state stayed open-coded in `dev_priv`
- the next global submit-id state stayed buried in `request->next_state`
- there was still no single typed top object that owned the full before/after
  scheduler-entry image consumed by the live mutation paths

This batch lifts that seam into one explicit scheduler-entry object above
`sgx_ta_queue_command`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal top object:

- `psb_sgx_lab_submit_sgx_ta_scheduler_entry`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_scheduler_entry_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_scheduler_entry_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_scheduler_entry_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_scheduler_entry()`

The new `sgx_ta_scheduler_entry` object now keeps together:

- the existing `sgx_ta_queue_command`
- the full previous driver-global scheduler state:
  - `issue_ticket`
  - `running_ticket`
  - `complete_ticket`
  - `issue_submit_id`
  - `running_submit_id`
  - `complete_submit_id`
  - `active_running_valid`
  - `active_running_submit_id`
- the full next driver-global scheduler state:
  - `issue_ticket`
  - `running_ticket`
  - `complete_ticket`
  - `issue_submit_id`
  - `running_submit_id`
  - `complete_submit_id`
  - `active_running_valid`
  - `active_running_submit_id`
- the derived scheduler advance flags:
  - `issue_advanced`
  - `running_advanced`
  - `complete_advanced`
  - `active_running_changed`
  - `issue_submit_changed`
  - `running_submit_changed`
  - `complete_submit_changed`

That moves the live SGX top seam one level higher again:

- validator: `sgx_ta_scheduler_entry`
- `issue_submit()` compatibility path: `sgx_ta_scheduler_entry`
- `transition_submit()` compatibility path: `sgx_ta_scheduler_entry`
- submit-op ioctl path: `sgx_ta_scheduler_entry`

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-223430-sgx-ta-scheduler-entry-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-223430-sgx-ta-scheduler-entry-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-223620-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-223620-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-223718-sgx-submit-ta-scheduler-entry-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-223718-sgx-submit-ta-scheduler-entry-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-223759-sgx-multi-submit-ta-scheduler-entry-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-223759-sgx-multi-submit-ta-scheduler-entry-live/summary.log)

Final live module:

- `driver_srcversion=4B13FC7FC62A3A9F89B82A8`
- `override_module_size=475088`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=4B13FC7FC62A3A9F89B82A8`
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

The SGX-shaped kernel-internal runtime stack now has thirty-three validated
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

This is still lab-mode execution, not real SGX rendering, but the shared top
layer now owns the full before/after driver-global scheduler entry image that
the live mutation wrappers implicitly consume, so both compatibility
entrypoints and the public submit-op path now meet at one typed scheduler
entry instead of rebuilding the queue-command image separately from the global
submit-id and active-running coordination image.

The next useful step is the first explicit SGX microkernel-facing request
packet, scheduler work packet, or queue-command stream object above
`sgx_ta_scheduler_entry`.
