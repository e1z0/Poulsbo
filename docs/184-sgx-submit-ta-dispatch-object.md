# SGX Submit TA Dispatch Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion AA6BEDA312078692D47D0A8`

## Goal

The SGX-shaped submit validator already had an explicit top scheduling object:

- `psb_sgx_lab_submit_sgx_ta_schedule`

That still left one important live/writeback split. The `issue_submit()` path
was still consuming typed control state, but then building the launch queue,
CCB, sync, and owner writeback image through open-coded local state:

- the validator stopped at `sgx_ta_schedule`
- the live issue path still rebuilt the final launch image inline
- transition progression was typed, issue writeback was not

This batch adds one higher object:

- `psb_sgx_lab_submit_sgx_ta_dispatch`

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal objects:

- `struct psb_sgx_lab_submit_sgx_issue_image`
- `struct psb_sgx_lab_submit_sgx_ta_dispatch`

New internal builders/helpers:

- `poulsbo_sgx_fill_submit_sgx_ta_schedule_from_control_locked()`
- `poulsbo_sgx_build_submit_sgx_ta_dispatch_locked()`
- `poulsbo_sgx_submit_sgx_ta_dispatch_issue_status_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_dispatch_locked()`
- `poulsbo_sgx_validate_submit_sgx_ta_dispatch_locked()`

The new `sgx_ta_dispatch` object builds above `sgx_ta_schedule` and makes the
launch-side dispatch image explicit:

- the final queue-header image for the launch snapshot
- the repeated CCB record image used for the issue slots
- the sync image that advances pending state
- the owner image that advances issue ownership
- the launch queue/sync/owner region bounds
- the start and end write offsets for the issued packets

The live `issue_submit()` path now consumes that typed dispatch image instead
of rebuilding queue/sync/owner writeback inline, and the top validator now
moves from `sgx_ta_schedule` to `sgx_ta_dispatch`.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical live proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-162106-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-162106-guarded-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-162255-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-162255-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-162355-sgx-submit-ta-dispatch-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-162355-sgx-submit-ta-dispatch-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-162434-sgx-multi-submit-ta-dispatch-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-162434-sgx-multi-submit-ta-dispatch-live/summary.log)

Final live module:

- `driver_srcversion=AA6BEDA312078692D47D0A8`
- `override_module_size=462560`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed again:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=AA6BEDA312078692D47D0A8`
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

- submit B still remains blocked-but-valid before dependency release:
  - `SUBMIT_B_STILL_IDLE_BEFORE_DEPENDENCY_RELEASE`
  - `submit_issue_count=0`
  - `submit_transition_count=0`
  - `submit_dependency_active_count=1`
  - `dependency[1]_required_ticket=3`
  - `dependency[1]_observed_complete_ticket=0`
- submit B later observes dependency completion and still remains valid:
  - `SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_A`
  - `submit_dependency_active_count=1`
  - `dependency[1]_required_ticket=3`
  - `dependency[1]_observed_complete_ticket=5`
- final typed multi-submit state retired cleanly:
  - `SUBMIT_A_ID=3`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - `SUBMIT_B_ID=4`
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

The SGX-shaped kernel-internal runtime stack now has eighteen validated
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

This is still lab-mode execution, not real SGX rendering, but the launch-side
issue writeback image is now a shared SGX-shaped object instead of separate
live-path bookkeeping.

The next useful step is the next more microkernel-facing SGX submit or
dispatch object above this repaired `sgx_ta_dispatch` layer.
