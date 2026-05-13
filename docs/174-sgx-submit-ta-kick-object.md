# SGX Submit TA Kick Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion B0D06254613C58FF8D2BBF3`

## Goal

The internal SGX-shaped submit stack already had:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`
- `psb_sgx_lab_submit_ccb_kick`
- `psb_sgx_lab_submit_sgx_ccb_kick`
- `psb_sgx_lab_submit_sgx_cmdta_shared`
- `psb_sgx_lab_submit_sgx_cmdta`

That still left one execution-side gap above the bounded `CMDTA` payload:
there was no explicit host-side TA kick object that wrapped the SGX-style
command words together with the live GPU VA bindings for the queued TA
command image. This batch adds that next layer:

- `psb_sgx_lab_submit_sgx_ta_kick`

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal objects:

- `struct psb_sgx_lab_submit_sgx_command`
- `struct psb_sgx_lab_submit_sgx_ta_kick`

New internal builder:

- `poulsbo_sgx_build_submit_sgx_ta_kick_locked()`

The new `sgx_ta_kick` object consumes one normalized `ccb_kick` and embeds the
current `sgx_cmdta` payload. It materializes the host-side SGX-shaped command
view the current lab model can actually prove today:

- SGX command words:
  - `service_address`
  - `cache_control`
  - `data0`
  - `data1`
- live GPU bindings:
  - `control_gpu_va32`
  - `ccb_gpu_va32`
  - `cmdta_gpu_va32`
  - `queue_gpu_va32`
  - `sync_gpu_va32`
  - `owner_gpu_va32`
  - `ta_status_gpu_va32`
  - `status_3d_gpu_va32`

The central submit validator now consumes `sgx_ta_kick` instead of validating
only the lower `sgx_cmdta` layer. That means the live validation step now
checks:

- SGX command words against the normalized live `exec_packet`
- GPU VA bindings against the normalized live `cmdta_kick`
- the same bounded `CMDTA` sync-object payload as before through the embedded
  `sgx_cmdta`
- the launch-time `issue-plan` path through the new TA-kick builder as well

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy:

- [logs/vaio-deploy/20260513-055407-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-055407-guarded-deploy)

Live target after deploy:

- `driver_srcversion=B0D06254613C58FF8D2BBF3`
- `override_module_size=450652`
- `lightdm_active=inactive`
- `kdump_service_active=active`
- `overall=remote_usable`

Canonical live bundles:

- exact no-redeploy `plan` regression proof:
  - [logs/vaio-plan-session/20260513-055558-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-055558-sgx-plan-session)
- focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-055708-sgx-submit-ta-kick-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-055708-sgx-submit-ta-kick-live/summary.log)
- typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-055746-sgx-multi-submit-ta-kick-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-055746-sgx-multi-submit-ta-kick-live/summary.log)

## Focused Evidence

- the exact `complete` regression proof stayed terminally clean on the new
  module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=B0D06254613C58FF8D2BBF3`
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
  - `submit_issue_plan_stage_name=retired`
  - `submit_dependency_active_count=0`
- submit B retained dependency state through the normalized TA-kick line and
  still retired cleanly:
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
  - `submit_issue_plan_stage_name=retired`
- final cleanup returned to zero live SGX state:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

## Result

The SGX-shaped kernel-internal execution stack now has eight validated layers:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`
- `psb_sgx_lab_submit_ccb_kick`
- `psb_sgx_lab_submit_sgx_ccb_kick`
- `psb_sgx_lab_submit_sgx_cmdta_shared`
- `psb_sgx_lab_submit_sgx_cmdta`
- `psb_sgx_lab_submit_sgx_ta_kick`

This is still lab-model execution, not real SGX rendering, but it is the
first host-side TA-kick object above the bounded `CMDTA` payload. It makes the
SGX command words and live queue/sync/status GPU bindings explicit as one
validated internal object on hardware. The next useful step is the next real
microkernel-facing SGX control object above this TA-kick layer, not another
rerun of the already-clean exact `complete` ladder.
