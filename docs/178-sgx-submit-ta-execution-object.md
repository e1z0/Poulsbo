# SGX Submit TA Execution Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 311989DA7E14F05C171ED79`

## Goal

The SGX-shaped submit stack already had an explicit TA submission object:

- `psb_sgx_lab_submit_sgx_ta_submission`

That still left one awkward split in the central validator:

- the live submission path was validated as one built object
- the launch-time future `issue-plan` snapshot was rebuilt and checked through
  a separate narrow branch

This batch adds one explicit execution object above `sgx_ta_submission`:

- `psb_sgx_lab_submit_sgx_ta_execution`

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal object:

- `struct psb_sgx_lab_submit_sgx_ta_execution`

New internal builder:

- `poulsbo_sgx_build_submit_sgx_ta_execution_locked()`

The new `sgx_ta_execution` object carries:

- one live `ccb_kick`
- one live `sgx_ta_submission`
- one optional launch-time `ccb_kick`
- one optional launch-time `sgx_ta_submission`

The first draft was too strict on live hardware. It reused the full
live-submission coherence helper for the launch snapshot, which reintroduced
the old `launch -> issue` regression:

- guarded deploy:
  [logs/vaio-deploy/20260513-111124-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-111124-guarded-deploy)
- exact proof:
  [logs/vaio-plan-session/20260513-111340-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-111340-sgx-plan-session)
- failure:
  - `submit_op_name=issue`
  - `submit_op_ret=-1 errno=22`

The fix was to keep the unified execution object, but validate the launch
snapshot only on launch-stable fields:

- queued TA command wiring
- queue-header placement
- copied future `issue-plan` state

The live-only submission coherence checks remain on the live snapshot. That
preserves the higher-level object model without falsely treating the future
launch image as if it were the current live submission image.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

Live progression:

- first guarded deploy of the new execution object:
  - [logs/vaio-deploy/20260513-111124-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-111124-guarded-deploy)
  - live override `srcversion 9574D599C4FD8B682D93F32`
  - exact proof failed at `issue` in
    [logs/vaio-plan-session/20260513-111340-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-111340-sgx-plan-session)
- final guarded deploy after narrowing launch-side validation:
  - [logs/vaio-deploy/20260513-120442-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-120442-guarded-deploy)
  - live override:
    - `driver_srcversion=311989DA7E14F05C171ED79`
    - `override_module_size=454160`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`

Canonical live bundles on the repaired module:

- exact no-redeploy `plan` regression proof:
  - [logs/vaio-plan-session/20260513-120641-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-120641-sgx-plan-session)
- focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-120825-sgx-submit-ta-execution-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-120825-sgx-submit-ta-execution-live/summary.log)
- typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-120927-sgx-multi-submit-ta-execution-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-120927-sgx-multi-submit-ta-execution-live/summary.log)

## Focused Evidence

- the exact `complete` proof is clean again on the repaired module:
  - `selected_phase_profile=complete`
  - `selected_outcome=completed`
  - `final_driver_srcversion=311989DA7E14F05C171ED79`
- focused typed submit still completed cleanly:
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

- submit A still retired terminally clean:
  - `SUBMIT_A_ID=3`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
  - `submit_dependency_active_count=0`
- submit B still carried dependency state through the execution object and
  retired cleanly:
  - `SUBMIT_B_ID=4`
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
  - `submit_dependency_active_count=1`

## Result

The SGX-shaped kernel-internal execution stack now has twelve validated
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

This is still lab-model execution, not real SGX rendering, but the validator
now has one explicit object that spans both the live TA submission snapshot
and the optional launch-time future issue snapshot. The next useful step is
the first more microkernel-facing SGX execution object above this
`sgx_ta_execution` layer.
