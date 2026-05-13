# SGX Submit SGX CCB Kick Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 529917AB9D340930B19FF28`

## Goal

The shared kernel-internal submit model already had four normalized
execution-shaped layers:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`
- `psb_sgx_lab_submit_ccb_kick`

That still left one SGX-shaped gap above the explicit CCB kick image: there was
no bounded object that carried the kick-level sync topology, first-kick/resume
state, and dependency snapshot fields implied by the live submit state itself.
This batch adds that next internal layer and fixes the first real regression it
exposed in the typed dependent-submit path.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

Runner follow-up:

- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)

New internal structure:

- `struct psb_sgx_lab_submit_sgx_ccb_kick`

New internal builder:

- `poulsbo_sgx_build_submit_sgx_ccb_kick_locked()`

New validation helper:

- `poulsbo_sgx_validate_submit_sgx_ccb_kick_locked()`

The new `sgx_ccb_kick` object wraps one `ccb_kick` and derives:

- destination sync-object count
- TA and 3D status-value counts
- first-kick-or-resume state
- CCB packet offset
- TA source/destination sync counts
- 3D source sync count
- TA dependency presence
- dependency submit id plus required and observed complete tickets
- dependency flags
- TA and 3D sync GPU-VA32 words

The status-side validator now consumes one explicit live `sgx_ccb_kick` instead
of only the raw `ccb_kick` image, and the central submit validator now
prebuilds both the live-source and launch-source `sgx_ccb_kick` objects as part
of the same execution-shaped path.

## Dependency-Source Fix

The first typed multi-submit rerun on this batch exposed a real regression:

- the dependent submit failed at `prepare`
- the failure came back as `submit_op_ret=-1 errno=22`

Root cause:

- the first draft treated a ticket-only TA dependency as an active 3D source
  sync before `depend-slot` and `depstate` had populated the dependency
  snapshot source submit id and required ticket
- the new `sgx_ccb_kick` self-check then correctly rejected that incomplete
  dependency image

The builder now only materializes the 3D source-sync side when the dependency
snapshot has a real source submit id and required ticket. That keeps early
`prepare` valid while still enforcing that an actually materialized dependency
must have a real source.

The runner follow-up in
[run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
now also prints the captured submit-op payload on failure, so later kernel-side
regressions do not disappear behind a silent wrapper failure.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`
- `bash -n scripts/run-vaio-sgx-multi-submit.sh`

Guarded deploy:

- [logs/vaio-deploy/20260513-042457-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-042457-guarded-deploy)

Live target after deploy:

- `driver_srcversion=529917AB9D340930B19FF28`
- `override_module_size=449016`
- `lightdm_active=inactive`
- `kdump_service_active=active`
- `overall=remote_usable`

Canonical live bundles:

- exact no-redeploy `plan` regression proof:
  - [logs/vaio-plan-session/20260513-042638-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-042638-sgx-plan-session)
- focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-042800-sgx-submit-sgx-ccb-kick-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-042800-sgx-submit-sgx-ccb-kick-live/summary.log)
- typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-042841-sgx-multi-submit-sgx-ccb-kick-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-042841-sgx-multi-submit-sgx-ccb-kick-live/summary.log)

## Focused Evidence

- the exact `complete` regression proof stayed terminally clean on the new
  module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=529917AB9D340930B19FF28`
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
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`

## Typed Multi-Submit Evidence

- `SUBMIT_A_ID=3`
- `SUBMIT_B_ID=4`
- the dependent submit now no longer dies at `prepare`; it reaches final
  `retired` with dependency state intact through the normalized views:
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
  - `submit_live_dependency_active_count=1`
  - `submit_result_dependency_active_count=1`
  - `submit_manifest_dependency_active_count=1`
  - `submit_descriptor_dependency_active_count=1`
  - `submit_plan_dependency_active_count=1`
  - `dependency[1]_submit_id=3`
  - `dependency[1]_required_ticket=3`
  - `dependency[1]_observed_complete_ticket=5`
  - `dependency[1]_flags=0xf`
- final cleanup returned to zero live SGX state:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

## Result

The shared execution-shaped kernel model now has five explicit internal layers:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`
- `psb_sgx_lab_submit_ccb_kick`
- `psb_sgx_lab_submit_sgx_ccb_kick`

That is still not real SGX execution, but it is the first layer that carries
explicit SGX-kick sync and dependency-snapshot semantics above the raw CCB kick
image, and it is already validated through guarded deploy, exact `complete`,
focused single-submit, and typed multi-submit hardware proof. The next useful
step is the first real SGX execution object that consumes this `sgx_ccb_kick`
layer instead of extending more read-side state.
