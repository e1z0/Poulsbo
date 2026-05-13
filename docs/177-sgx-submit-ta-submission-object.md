# SGX Submit TA Submission Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 329853678DD9A992924BB5C`

## Goal

The internal SGX-shaped submit stack already had:

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

That still left the control-backed submit views validated as separate image
checks above the queued TA command line. This batch adds one higher explicit
execution object:

- `psb_sgx_lab_submit_sgx_ta_submission`

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal object:

- `struct psb_sgx_lab_submit_sgx_ta_submission`

New internal builder:

- `poulsbo_sgx_build_submit_sgx_ta_submission_locked()`

The new `sgx_ta_submission` object consumes one normalized submit kick and
embeds:

- the validated `sgx_queued_ta_command`
- the typed `live` image
- the typed `manifest` image
- the typed `descriptor` image
- the typed `plan` image
- the optional launch-time typed `issue-plan` image

The control-side image validators were also widened so they can validate
against already-built expected typed images instead of always rebuilding their
own temporary words. The central validator now consumes `sgx_ta_submission`,
so one checked object spans the queued TA command and the control-backed
memory images that represent the same submission.

Two invariants from the first draft had to be relaxed after live testing:

- `issue-plan` is a launch-time future snapshot, not always a projection of the
  current live queue state
- `plan` is source-switched by stage, so `launch`, `dispatch`, and `retired`
  use their own plan-source views instead of always mirroring current live

That means the final `sgx_ta_submission` validator compares those views on
stable command and object identity fields, not on transient live queue offsets
that intentionally diverge by stage.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy and exact-proof progression:

- first guarded deploy:
  - [logs/vaio-deploy/20260513-075745-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-075745-guarded-deploy)
  - live override `srcversion 0AD6D850315762A2E4F6E45`
  - exact proof failed at `issue` in
    [logs/vaio-plan-session/20260513-080115-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-080115-sgx-plan-session)
    with `submit_op_ret=-1 errno=22`
- second guarded deploy after relaxing the `issue-plan` cross-check:
  - [logs/vaio-deploy/20260513-080356-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-080356-guarded-deploy)
  - live override `srcversion 9172ED422A56E8DA614C79D`
  - exact proof still failed at `issue` in
    [logs/vaio-plan-session/20260513-080540-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-080540-sgx-plan-session)
- final guarded deploy after relaxing the stage-switched `plan` cross-check:
  - [logs/vaio-deploy/20260513-080856-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-080856-guarded-deploy)
  - live override:
    - `driver_srcversion=329853678DD9A992924BB5C`
    - `override_module_size=453508`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`

Canonical live bundles on the final fixed module:

- exact no-redeploy `plan` regression proof:
  - [logs/vaio-plan-session/20260513-081039-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-081039-sgx-plan-session)
- focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-081614-sgx-submit-ta-submission-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-081614-sgx-submit-ta-submission-live/summary.log)
- typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-081700-sgx-multi-submit-ta-submission-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-081700-sgx-multi-submit-ta-submission-live/summary.log)

## Focused Evidence

- the exact `complete` regression proof stayed terminally clean on the final
  module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=329853678DD9A992924BB5C`
- focused typed submit still completed cleanly:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - final cleanup:
    - `submit_dependency_active_count=0`
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`

## Typed Multi-Submit Evidence

- submit A still finished terminally clean on the final module:
  - `SUBMIT_A_ID=6`
  - `submit_prepare_count=1`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
  - `submit_dependency_active_count=0`
- submit B retained dependency state through the normalized submission line and
  still retired cleanly:
  - `SUBMIT_B_ID=7`
  - `submit_prepare_count=1`
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
  - `submit_dependency_active_count=1`
- final cleanup returned to zero live SGX state:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

## Result

The SGX-shaped kernel-internal execution stack now has eleven validated
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

This is still lab-model execution, not real SGX rendering, but it is now the
first explicit submission object above the queued TA-command layer. It keeps
the queued command image and the control-backed `live/manifest/descriptor/plan`
views coherent as one validated SGX-shaped execution object on hardware. The
next useful step is the next microkernel-facing SGX execution object above
this `sgx_ta_submission` layer.
