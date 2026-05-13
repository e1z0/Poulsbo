# SGX Submit TA Timeline Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion A1A66310C19EE4235C25ED2`

## Goal

The SGX-shaped submit validator already had an explicit live-plus-future object:

- `psb_sgx_lab_submit_sgx_ta_execution`

That still left one real semantic gap. The runtime snapshots for:

- launch
- dispatch
- retired

were only validated indirectly through raw submit fields and the stage-switched
typed views. This batch adds one higher SGX-shaped runtime object:

- `psb_sgx_lab_submit_sgx_ta_timeline`

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal object:

- `struct psb_sgx_lab_submit_sgx_ta_timeline`

New internal builders:

- `poulsbo_sgx_build_submit_sgx_queued_ta_command_from_source_locked()`
- `poulsbo_sgx_build_submit_sgx_ta_timeline_locked()`

New validation split:

- the TA command, queued-command, and queued-command `issue-plan` checks are
  now shared helpers instead of duplicated between the full submission and
  launch-only branches
- the top validator now builds one `sgx_ta_timeline` object above
  `sgx_ta_execution`

The first draft over-constrained the running multi-submit path. It assumed a
valid `dispatch` snapshot must still have a preserved historical `launch`
snapshot and `launch_consumed=1`. Real hardware disproved that on the running
submit: `dispatch_valid=1`, but `launch_valid=0` and `launch_consumed=0`.

The fix was to keep the new timeline object, but only compare launch-to-dispatch
identity when a launch snapshot is actually present. Dispatch and retired
snapshots now stand on their own validated queued-command images, with
dispatch-to-retired identity still checked when both exist.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

First live draft:

- guarded deploy:
  [logs/vaio-deploy/20260513-123521-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-123521-guarded-deploy)
- live module:
  - `driver_srcversion=82C2FB699E0C6937AB04CC2`
  - `override_module_size=455932`
- exact proof:
  [logs/vaio-plan-session/20260513-123726-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-123726-sgx-plan-session)
  still passed terminal `complete`
- focused typed proof:
  [logs/vaio-ioctl/20260513-123846-sgx-submit-ta-timeline-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-123846-sgx-submit-ta-timeline-live/summary.log)
  still passed
- typed multi-submit exposed the bad invariant:
  [logs/vaio-ioctl/20260513-123933-sgx-multi-submit-ta-timeline-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-123933-sgx-multi-submit-ta-timeline-live/summary.log)
  - failure at `SUBMIT_A_AFTER_RUNNING_VALIDATE`
  - `submit_op_name=validate`
  - `submit_op_ret=-1 errno=22`

Repaired live draft:

- guarded deploy:
  [logs/vaio-deploy/20260513-124147-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-124147-guarded-deploy)
- final live module:
  - `driver_srcversion=A1A66310C19EE4235C25ED2`
  - `override_module_size=455932`
  - `overall=remote_usable`
  - `lightdm_active=inactive`
  - `kdump_service_active=active`

Canonical live proof set on the repaired module:

- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-124338-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-124338-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-124444-sgx-submit-ta-timeline-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-124444-sgx-submit-ta-timeline-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-124523-sgx-multi-submit-ta-timeline-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-124523-sgx-multi-submit-ta-timeline-live/summary.log)

## Focused Evidence

- the exact `complete` proof passed again:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=A1A66310C19EE4235C25ED2`
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
  - `submit_global_issue_ticket=2`
  - `submit_global_running_ticket=2`
  - `submit_global_complete_ticket=2`

## Typed Multi-Submit Evidence

- submit A retired cleanly through the repaired timeline path:
  - `SUBMIT_A_ID=3`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - `submit_dependency_active_count=0`
- submit B also retired cleanly while still carrying dependency state:
  - `SUBMIT_B_ID=4`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - `submit_dependency_active_count=1`

## Result

The SGX-shaped kernel-internal runtime stack now has thirteen validated layers:

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

This is still lab-mode execution, not real SGX rendering, but the validator
now has one explicit runtime object that spans:

- the live submission snapshot
- the optional future launch snapshot
- the historical launch/dispatch/retired queued-command snapshots
- the runtime issue/transition/ticket state around them

The next useful step is the first more microkernel-facing SGX execution object
above this repaired `sgx_ta_timeline` layer.
