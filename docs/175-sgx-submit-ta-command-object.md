# SGX Submit TA Command Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 61850CF09DE3C6531E06127`

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

That still left one gap above the host-side TA kick: there was no single
kernel-internal object that tied the validated TA-kick control view back to
the actual queued CCB command image. This batch adds that next layer:

- `psb_sgx_lab_submit_sgx_ta_command`

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal object:

- `struct psb_sgx_lab_submit_sgx_ta_command`

New internal builder:

- `poulsbo_sgx_build_submit_sgx_ta_command_locked()`

The new `sgx_ta_command` object consumes one normalized `ccb_kick`, embeds the
current `sgx_ta_kick`, and carries the typed CCB record words that represent
the queued TA command image. That means one validated internal object now
covers:

- `service_address`
- `cache_control`
- `data0`
- `data1`
- `pds_gpu_va32`
- `use_gpu_va32`
- `target_gpu_va32`
- `status_gpu_va32`

The central submit validator now consumes `sgx_ta_command` instead of stopping
at `sgx_ta_kick`. That makes the command words, the queued CCB payload, and
the live TA-kick bindings coherent as one checked SGX-shaped execution object.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy:

- [logs/vaio-deploy/20260513-061050-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-061050-guarded-deploy)

Live target after deploy:

- `driver_srcversion=61850CF09DE3C6531E06127`
- `override_module_size=451152`
- `lightdm_active=inactive`
- `kdump_service_active=active`
- `overall=remote_usable`

Canonical live bundles:

- exact no-redeploy `plan` regression proof:
  - [logs/vaio-plan-session/20260513-061236-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-061236-sgx-plan-session)
- focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-061337-sgx-submit-ta-command-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-061337-sgx-submit-ta-command-live/summary.log)
- typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-061416-sgx-multi-submit-ta-command-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-061416-sgx-multi-submit-ta-command-live/summary.log)

## Focused Evidence

- the exact `complete` regression proof stayed terminally clean on the new
  module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=61850CF09DE3C6531E06127`
- focused typed submit still completed cleanly:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- final focused execution-shaped views stayed coherent:
  - `submit_live_stage_name=retired`
  - `submit_result_stage_name=retired`
  - `submit_manifest_stage_name=retired`
  - `submit_descriptor_stage_name=retired`
  - `submit_plan_stage_name=retired`
  - `submit_issue_plan_stage_name=retired`
  - `submit_issue_plan_dependency_active_count=0`
  - final cleanup:
    - `submit_dependency_active_count=0`
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`

## Typed Multi-Submit Evidence

- submit A still finished terminally clean on the new module:
  - `SUBMIT_A_ID=3`
  - `submit_prepare_count=1`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
  - `submit_dependency_active_count=0`
- submit B retained dependency state through the normalized TA-command line
  and still retired cleanly:
  - `SUBMIT_B_ID=4`
  - `submit_prepare_count=1`
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
  - `submit_result_dependency_active_count=1`
  - `submit_manifest_dependency_active_count=1`
  - `submit_descriptor_dependency_active_count=1`
  - `submit_plan_dependency_active_count=1`
  - `submit_dependency_active_count=1`
- final cleanup returned to zero live SGX state:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

## Result

The SGX-shaped kernel-internal execution stack now has nine validated layers:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`
- `psb_sgx_lab_submit_ccb_kick`
- `psb_sgx_lab_submit_sgx_ccb_kick`
- `psb_sgx_lab_submit_sgx_cmdta_shared`
- `psb_sgx_lab_submit_sgx_cmdta`
- `psb_sgx_lab_submit_sgx_ta_kick`
- `psb_sgx_lab_submit_sgx_ta_command`

This is still lab-model execution, not real SGX rendering, but it is now the
first full queued TA command object above the TA-kick layer. It keeps the
validated command words, the typed CCB payload, and the live SGX bindings in
one explicit object on hardware. The next useful step is the next
microkernel-facing SGX execution object above this `sgx_ta_command` layer.
