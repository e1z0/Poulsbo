# SGX Submit TA Scheduler Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 2F3CD7EC0198A4A88B2798E`

## Goal

The validated SGX-shaped submit stack already reached the public live
issue/transition request boundary through:

- `psb_sgx_lab_submit_sgx_ta_op`

That still left one meaningful top-side execution seam open-coded:

- `sgx_ta_commit` still applied driver-global `issue/running/complete`
  coordination state directly into `dev_priv`
- the shared top layer did not yet own the live scheduler image that sits above
  the submit-local queue/CCB/sync/owner state transition

This batch adds that explicit scheduler layer above `sgx_ta_op`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal top object:

- `psb_sgx_lab_submit_sgx_ta_scheduler`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_scheduler_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_scheduler_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_scheduler()`
- `poulsbo_sgx_validate_submit_sgx_ta_scheduler_locked()`

The new `sgx_ta_scheduler` object now keeps together:

- the existing `sgx_ta_op`
- the final driver-global issue/running/complete coordination image
- the active-running owner image
- the flags that say which coordination edges actually advanced

That lets the top live SGX path move one level higher:

- validator: `sgx_ta_scheduler`
- `issue_submit()`: `sgx_ta_scheduler`
- `transition_submit()`: `sgx_ta_scheduler`

This batch also removes one remaining split in the live apply path:

- `sgx_ta_commit` no longer writes global issue/running/complete coordination
  directly into `dev_priv`
- the top scheduler object now owns that apply-side image explicitly

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical live proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-192012-sgx-ta-scheduler-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-192012-sgx-ta-scheduler-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-192157-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-192157-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-192257-sgx-submit-ta-scheduler-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-192257-sgx-submit-ta-scheduler-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-192342-sgx-multi-submit-ta-scheduler-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-192342-sgx-multi-submit-ta-scheduler-live/summary.log)

Final live module:

- `driver_srcversion=2F3CD7EC0198A4A88B2798E`
- `override_module_size=465728`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed again:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=2F3CD7EC0198A4A88B2798E`
- focused typed submit stayed terminally clean:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - `submit_dependency_active_count=0`
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`

## Typed Multi-Submit Evidence

- final typed multi-submit state retired cleanly:
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_dependency_active_count=1`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`

## Result

The SGX-shaped kernel-internal runtime stack now has twenty-four validated
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

This is still lab-mode execution, not real SGX rendering, but the shared top
layer now owns another real live boundary: the driver-global scheduler image
that advances issue/running/complete coordination above the submit-local apply
image.

The next useful step is the first explicit SGX sequence or microkernel-facing
work queue object above `sgx_ta_scheduler`.
