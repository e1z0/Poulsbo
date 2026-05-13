# SGX Submit TA Job Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion A0E6A4B613B1CF22BFAB9DC`

## Goal

The validated SGX-shaped submit stack already had a shared top work-queue
object:

- `psb_sgx_lab_submit_sgx_ta_work_queue`

That layer unified the typed request boundary and the pre-apply scheduler
snapshot, but one higher execution-side seam still remained open-coded:

- the submit contract itself:
  - context
  - cmd type
  - control flags
  - bound object ids
  - status value counts
  - dependency state
- the top dependency/gate summary that says whether the request can enter
  running safely

were still spread between the callers and the nested work-queue builder.

This batch adds that explicit job layer above `sgx_ta_work_queue`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal top object:

- `psb_sgx_lab_submit_sgx_ta_job`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_job_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_job_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_job()`
- `poulsbo_sgx_validate_submit_sgx_ta_job_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_job_locked()`

The new `sgx_ta_job` object now keeps together:

- the existing `sgx_ta_work_queue`
- the top submit contract:
  - `submit_id`
  - `context`
  - `cmd_type`
  - `ctrl_flags`
  - `control_object_id`
  - `pds_object_id`
  - `use_object_id`
  - `target_object_id`
  - `status_object_id`
  - `num_ta_status_vals`
  - `num_3d_status_vals`
  - `ta_dependency`
- the copied dependency-state array
- the top gate summary:
  - `active_dependency_count`
  - `running_slot_busy`
  - `can_enter_running`

That moves the top live SGX path one level higher again:

- validator: `sgx_ta_job`
- `issue_submit()`: `sgx_ta_job`
- `transition_submit()`: `sgx_ta_job`

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical live proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-201803-sgx-ta-job-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-201803-sgx-ta-job-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-202002-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-202002-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-202558-sgx-submit-ta-job-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-202558-sgx-submit-ta-job-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-202643-sgx-multi-submit-ta-job-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-202643-sgx-multi-submit-ta-job-live/summary.log)

Final live module:

- `driver_srcversion=A0E6A4B613B1CF22BFAB9DC`
- `override_module_size=469344`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed again:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=A0E6A4B613B1CF22BFAB9DC`
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

## Typed Multi-Submit Evidence

- final typed multi-submit state retired cleanly:
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
    - `dependency[1]_observed_complete_ticket=11`

## Result

The SGX-shaped kernel-internal runtime stack now has twenty-seven validated
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

This is still lab-mode execution, not real SGX rendering, but the shared top
layer now owns the full typed submit contract and dependency summary above the
request boundary, scheduler snapshot, and live operation sequence.

The next useful step is the first explicit SGX request, scheduler-facing
work item, or microkernel-facing queue object above `sgx_ta_job`.
