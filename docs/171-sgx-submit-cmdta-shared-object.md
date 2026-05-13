# SGX Submit CMDTA Shared Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 040B3A07BC156A1AAEE99D6`

## Goal

The shared kernel-internal submit model already had five normalized
execution-shaped layers:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`
- `psb_sgx_lab_submit_ccb_kick`
- `psb_sgx_lab_submit_sgx_ccb_kick`

That still left one SGX microkernel-shaped gap above the live CCB kick image:
there was no explicit object for the `CMDTA_SHARED`-style control-side sync and
status topology that the status-side validator could consume directly. This
batch adds that next internal layer and finishes the validator refactor so the
large stack-frame warning is gone again.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal structures:

- `struct psb_sgx_lab_submit_sgx_sync`
- `struct psb_sgx_lab_submit_sgx_ta3d_dependency`
- `struct psb_sgx_lab_submit_sgx_cmdta_shared`

New internal builder:

- `poulsbo_sgx_build_submit_sgx_cmdta_shared_locked()`

New validation helper:

- `poulsbo_sgx_validate_submit_sgx_cmdta_shared_locked()`

The new `sgx_cmdta_shared` object wraps one `ccb_kick`, rebuilds the SGX kick
side internally, and carries:

- kick pointer back to the normalized `ccb_kick`
- control flags
- dst sync-object count
- TA and 3D status-value counts
- first-kick-or-resume state
- CCB packet offset
- TA dependency presence
- TA and 3D TQ sync words
- TA source and destination sync counts
- 3D source-sync count
- TA/3D dependency snapshot fields
- shared pointers to the normalized TA and 3D status-update arrays

The status-side validator now consumes one explicit live
`psb_sgx_lab_submit_sgx_cmdta_shared` object instead of carrying a separate
live `sgx_ccb_kick` plus raw status-update loops. That keeps the new layer
real, and it collapses the validator frame back under the earlier
`-Wframe-larger-than=1024` threshold by moving the SGX kick rebuild into the
builder instead of keeping both SGX layers live in the validator.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy:

- [logs/vaio-deploy/20260513-045436-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-045436-guarded-deploy)

Live target after deploy:

- `driver_srcversion=040B3A07BC156A1AAEE99D6`
- `override_module_size=449672`
- `lightdm_active=inactive`
- `kdump_service_active=active`
- `overall=remote_usable`

Canonical live bundles:

- exact no-redeploy `plan` regression proof:
  - [logs/vaio-plan-session/20260513-045622-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-045622-sgx-plan-session)
- focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-045728-sgx-submit-cmdta-shared-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-045728-sgx-submit-cmdta-shared-live/summary.log)
- typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-045811-sgx-multi-submit-cmdta-shared-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-045811-sgx-multi-submit-cmdta-shared-live/summary.log)

## Focused Evidence

- the exact `complete` regression proof stayed terminally clean on the new
  module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=040B3A07BC156A1AAEE99D6`
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
  - `submit_issue_plan_stage_count=1`
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`

## Typed Multi-Submit Evidence

- submit B retained explicit dependency state through the normalized shared
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

The shared execution-shaped kernel model now has six explicit internal layers:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`
- `psb_sgx_lab_submit_ccb_kick`
- `psb_sgx_lab_submit_sgx_ccb_kick`
- `psb_sgx_lab_submit_sgx_cmdta_shared`

That is still not real SGX execution, but it is the first layer that makes the
`CMDTA_SHARED`-style sync and status topology explicit above the SGX CCB kick
image, and it is already validated through guarded deploy, exact `complete`,
focused single-submit, and typed multi-submit hardware proof. The next useful
step is the first real SGX execution object that consumes this
`sgx_cmdta_shared` layer instead of extending more validation-only SGX state.
