# SGX Submit CMDTA Kick Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion C1CC8033D0035B46726BAB0`

## Goal

The shared kernel-internal submit model already had two normalized execution
layers:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`

That still left one gap above the packet: there was no bounded TA-specific
object that derived the actual command-kick device addresses from the packet
image itself. This batch adds that next internal SGX-shaped layer without
changing userspace uAPI.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal helper:

- `poulsbo_sgx_gpu_va32_add_offset()`

New internal structure:

- `struct psb_sgx_lab_submit_cmdta_kick`

New internal builder:

- `poulsbo_sgx_build_submit_cmdta_kick_locked()`

The new `cmdta_kick` object wraps one `exec_packet` and derives the TA-style
device addresses that sit above it:

- queue GPU VA
- CCB GPU VA
- sync GPU VA
- owner GPU VA
- TA status GPU VA
- 3D status GPU VA

For launch-time use, it can also carry the predicted post-issue
`issue_plan` image in the same object.

The new flow is now used in two places:

- `poulsbo_sgx_build_submit_issue_plan_view_locked()` now builds one
  launch-source `cmdta_kick` and serializes `issue-plan` from that object
  instead of juggling a raw launch packet plus a separate local plan overlay
- `poulsbo_sgx_validate_submit_locked()` now prebuilds:
  - one live-source `cmdta_kick`
  - and, when applicable, one launch-source `cmdta_kick` with the
    post-issue plan

That means the central validator now has one explicit internal TA-specific
object above `exec_packet`, and the typed `issue-plan` view is now fed by the
same object.

No userspace ioctl or query surface changed in this batch.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy:

- [logs/vaio-deploy/20260513-032841-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-032841-guarded-deploy)

Live target after deploy:

- `driver_srcversion=C1CC8033D0035B46726BAB0`
- `override_module_size=447532`
- `lightdm_active=inactive`
- `kdump_service_active=active`
- `overall=remote_usable`

Canonical live bundles:

- exact no-redeploy `plan` regression proof:
  - [logs/vaio-plan-session/20260513-033102-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-033102-sgx-plan-session)
- focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-033202-sgx-submit-cmdta-kick-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-033202-sgx-submit-cmdta-kick-live/summary.log)
- typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-033241-sgx-multi-submit-cmdta-kick-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-033241-sgx-multi-submit-cmdta-kick-live/summary.log)

## Focused Evidence

- the exact `complete` regression proof stayed terminally clean on the new
  module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=C1CC8033D0035B46726BAB0`
- focused typed submit still completed cleanly:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- the execution-shaped views stayed coherent through final retire:
  - `submit_live_stage=6`
  - `submit_result_stage=6`
  - `submit_manifest_stage=6`
  - `submit_descriptor_stage=6`
  - `submit_plan_stage=6`
  - `submit_issue_plan_offset=0x4a0`
  - `submit_issue_plan_stage_name=retired`
  - `submit_issue_plan_stage_count=1`
  - `submit_issue_plan_config_count=1`
  - `submit_owner_state_name=complete`
  - `submit_dependency_active_count=0`
- final cleanup returned to zero live SGX state:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

## Typed Multi-Submit Evidence

- `SUBMIT_A_ID=3`
- `SUBMIT_B_ID=4`
- submit A final:
  - `submit_prepare_count=1`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
  - `submit_live_stage_name=retired`
  - `submit_result_stage_name=retired`
  - `submit_manifest_stage_name=retired`
  - `submit_descriptor_stage_name=retired`
  - `submit_plan_stage_name=retired`
  - `submit_issue_plan_stage_name=retired`
  - `submit_issue_plan_stage_count=3`
- submit B final:
  - `submit_prepare_count=1`
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
  - `submit_live_stage_name=retired`
  - `submit_result_stage_name=retired`
  - `submit_manifest_stage_name=retired`
  - `submit_descriptor_stage_name=retired`
  - `submit_plan_stage_name=retired`
  - `submit_issue_plan_stage_name=retired`
  - `submit_issue_plan_stage_count=2`
- the dependent submit still proved the intended live path in dmesg:
  - `lab submit emit submit_id=4 ... dependency_active_count=1`
  - `lab submit launch submit_id=4 ... dependency_active_count=1`
  - `lab submit issue submit_id=4 ... refill=1`
- final cleanup returned to zero live SGX state:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

## Result

The shared execution-shaped kernel model now has three explicit internal
layers:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`

That is still not real SGX execution, but it is the first TA-specific
execution-oriented object above `exec_packet`, and it is already validated
through guarded deploy, exact `complete`, focused single-submit, and typed
multi-submit hardware proof. The next useful step is the first real SGX
execution object that consumes this `cmdta_kick` layer instead of adding more
read-side structure.
