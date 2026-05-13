# SGX Submit CCB Kick Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 8B053004C83E74C5824A815`

## Goal

The shared kernel-internal SGX submit model already had three normalized
execution-shaped layers:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`

That still left one gap above the TA-style kick object: there was no bounded
internal object that carried the exact status-word updates implied by the live
CCB kick image itself. This batch adds that next internal layer without
changing the userspace ioctl surface.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal structures:

- `struct psb_sgx_lab_submit_status_update`
- `struct psb_sgx_lab_submit_ccb_kick`

New internal builders:

- `poulsbo_sgx_build_submit_status_updates_locked()`
- `poulsbo_sgx_build_submit_ccb_kick_locked()`

New validation helper:

- `poulsbo_sgx_validate_submit_ccb_kick_locked()`

The new `ccb_kick` object wraps one `cmdta_kick` and derives:

- the CCB packet offset
- whether the packet carries a TA dependency
- the TA status update set
- the 3D status update set
- the exact expected status values and GPU VA32 addresses for each update slot

The status-side validator no longer rebuilds TA and 3D status expectations from
raw submit offsets inline. It now validates the backing status object through
one explicit live-source `ccb_kick`. When a launch-time issue-plan is present,
the central validator also prebuilds the launch-source `ccb_kick` as part of
the same SGX-shaped execution path.

No userspace ioctl or typed query format changed in this batch.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy:

- [logs/vaio-deploy/20260513-034651-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-034651-guarded-deploy)

Live target after deploy:

- `driver_srcversion=8B053004C83E74C5824A815`
- `override_module_size=448296`
- `lightdm_active=inactive`
- `kdump_service_active=active`
- `overall=remote_usable`

Canonical live bundles:

- exact no-redeploy `plan` regression proof:
  - [logs/vaio-plan-session/20260513-034951-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-034951-sgx-plan-session)
- focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-035055-sgx-submit-ccb-kick-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-035055-sgx-submit-ccb-kick-live/summary.log)
- typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-035151-sgx-multi-submit-ccb-kick-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-035151-sgx-multi-submit-ccb-kick-live/summary.log)

## Focused Evidence

- the exact `complete` regression proof stayed terminally clean on the new
  module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=8B053004C83E74C5824A815`
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
  - `submit_plan_dependency_active_count=0`
  - `submit_descriptor_dependency_active_count=0`
  - `submit_manifest_dependency_active_count=0`
  - `submit_result_dependency_active_count=0`
  - `submit_dependency_active_count=0`
- final cleanup returned to zero live SGX state:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

## Typed Multi-Submit Evidence

- the exact proof stayed clean after focused validation and before multi-submit:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-034951`
- the dependent submit still carried dependency state through the normalized
  views:
  - `submit_result_dependency_active_count=1`
  - `submit_manifest_dependency_active_count=1`
  - `submit_descriptor_dependency_active_count=1`
  - `submit_plan_dependency_active_count=1`
  - `submit_dependency_active_count=1`
  - `dependency[1]_submit_id=3`
  - `dependency[1]_required_ticket=3`
  - `dependency[1]_observed_complete_ticket=5`
  - `dependency[1]_flags=0xf`
- final cleanup returned to zero live SGX state:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

## Result

The shared execution-shaped kernel model now has four explicit internal layers:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`
- `psb_sgx_lab_submit_ccb_kick`

That is still not real SGX execution, but it is the first internal object that
turns the TA-style kick image into explicit status-update expectations, and it
is already validated through guarded deploy, exact `complete`, focused
single-submit, and typed multi-submit hardware proof. The next useful step is
the first real SGX execution object that consumes this `ccb_kick` layer instead
of extending more read-side state.
