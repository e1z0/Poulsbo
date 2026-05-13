# SGX Submit TA Runtime Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 440FDB3F77708F25340DA25`

## Goal

The SGX-shaped submit validator already had an explicit per-submit runtime
history object:

- `psb_sgx_lab_submit_sgx_ta_timeline`

That still left one semantic gap above the per-submit view. Global SGX
coordination and dependency state were still being checked through scattered
raw submit fields:

- global issue/running/complete tickets
- active running submit tracking
- unresolved dependency snapshots
- live sync/owner outstanding counts

This batch adds one higher object:

- `psb_sgx_lab_submit_sgx_ta_runtime`

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal objects:

- `struct psb_sgx_lab_submit_sgx_dependency_state`
- `struct psb_sgx_lab_submit_sgx_coordination`
- `struct psb_sgx_lab_submit_sgx_ta_runtime`

New internal builder:

- `poulsbo_sgx_build_submit_sgx_ta_runtime_locked()`

Validation split:

- `poulsbo_sgx_validate_built_submit_sgx_ta_timeline_locked()`
- `poulsbo_sgx_validate_submit_sgx_ta_runtime_locked()`

The first draft over-constrained unresolved dependency state. It treated an
unsatisfied dependency as structurally invalid during `prepare`, even though
this lab path allows a blocked dependency to exist before launch and issue
consume it. Real hardware disproved that on the second typed submit:

- submit B kicked with `ta_dependency=3`
- submit B was still valid at `prepare`
- the dependency source and depstate materialization were intentionally not yet
  complete

The fix kept the runtime object but stopped treating
`observed_complete_ticket < required_ticket` as a validator failure. The launch
and issue path still own the actual dependency gate.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

First live draft:

- guarded deploy:
  [logs/vaio-deploy/20260513-125935-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-125935-guarded-deploy)
- live module:
  - `driver_srcversion=BC32B83F0BFBD10FB1D0645`
  - `override_module_size=456232`
- exact proof:
  [logs/vaio-plan-session/20260513-130122-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-130122-sgx-plan-session)
  still passed terminal `complete`
- focused typed proof:
  [logs/vaio-ioctl/20260513-130233-sgx-submit-ta-runtime-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-130233-sgx-submit-ta-runtime-live/summary.log)
  still passed
- typed multi-submit exposed the bad invariant:
  [logs/vaio-ioctl/20260513-130314-sgx-multi-submit-ta-runtime-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-130314-sgx-multi-submit-ta-runtime-live/summary.log)
  - failure at `submit_prepare 6`
  - `submit_op_name=prepare`
  - `submit_op_ret=-1 errno=22`
- trace capture of the same failing point:
  [logs/vaio-ioctl/20260513-130855-sgx-multi-submit-ta-runtime-trace/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-130855-sgx-multi-submit-ta-runtime-trace/summary.log)

Repaired live draft:

- guarded deploy:
  [logs/vaio-deploy/20260513-131259-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-131259-guarded-deploy)
- final live module:
  - `driver_srcversion=440FDB3F77708F25340DA25`
  - `override_module_size=456232`
  - `overall=remote_usable`
  - `lightdm_active=inactive`
  - `kdump_service_active=active`

Canonical live proof set on the repaired module:

- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-131445-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-131445-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-131554-sgx-submit-ta-runtime-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-131554-sgx-submit-ta-runtime-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-131635-sgx-multi-submit-ta-runtime-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-131635-sgx-multi-submit-ta-runtime-live/summary.log)

## Focused Evidence

- the exact `complete` proof passed again:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=440FDB3F77708F25340DA25`
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

- submit B now survives the earlier broken runtime point:
  - `SUBMIT_B_AFTER_DEPSTATE_SETUP`
  - `submit_prepare_count=1`
  - `submit_dependency_active_count=1`
  - `dependency[1]_required_ticket=3`
  - `dependency[1]_observed_complete_ticket=0`
- submit B still remains blocked-but-valid before dependency release:
  - `SUBMIT_B_STILL_IDLE_BEFORE_DEPENDENCY_RELEASE`
  - `submit_issue_count=0`
  - `submit_transition_count=0`
  - `submit_dependency_active_count=1`
- submit B later observes dependency completion and still remains valid:
  - `SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_A`
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

The SGX-shaped kernel-internal runtime stack now has fourteen validated layers:

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

This is still lab-mode execution, not real SGX rendering, but the validator
now has one explicit top runtime object that spans:

- the per-submit execution timeline
- global issue/running/complete coordination
- sync and owner outstanding counts
- unresolved and later-resolved dependency snapshots

The next useful step is the first more microkernel-facing SGX control object
above this repaired `sgx_ta_runtime` layer.
