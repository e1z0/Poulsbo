# SGX Submit CMDTA Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 2DA0FEA89EBE73F96BE4053`

## Goal

The internal SGX-shaped submit stack already had:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`
- `psb_sgx_lab_submit_ccb_kick`
- `psb_sgx_lab_submit_sgx_ccb_kick`
- `psb_sgx_lab_submit_sgx_cmdta_shared`

That still left one obvious gap relative to the mirrored Services4
`SGXMKIF_CMDTA_SHARED` shape: the current top layer had explicit sync records
and counts, but not a real CMDTA-level object that materialized the bounded
TA-destination and 3D-source sync-object arrays as one validated internal
payload.

This batch adds that next layer:

- `psb_sgx_lab_submit_sgx_cmdta`

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal object:

- `struct psb_sgx_lab_submit_sgx_cmdta`

New internal builder:

- `poulsbo_sgx_build_submit_sgx_cmdta_locked()`

The new `sgx_cmdta` object embeds the current `sgx_cmdta_shared` layer and
materializes the bounded generalized-sync arrays the current lab model can
actually support today:

- `ta_dst_syncs`
- `threed_src_syncs`

The current model still has no TA source sync objects, so the new builder
explicitly rejects nonzero `num_ta_src_syncs` instead of silently pretending
to support them. The current bounded object therefore stays honest to the live
lab behavior:

- `num_ta_src_syncs == 0`
- `num_ta_dst_syncs <= 1`
- `num_3d_src_syncs <= 1`

The submit validator now consumes `psb_sgx_lab_submit_sgx_cmdta` instead of
validating only the lower `sgx_cmdta_shared` layer. That means the live
validation step now checks:

- the explicit TA-destination sync-object payload against the normalized
  `ta_tq_sync` and hardware destination-sync list
- the explicit 3D-source sync-object payload against the normalized
  `ta3d_dependency_sync`
- the same status-side image checks as before through the embedded shared
  object

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy:

- [logs/vaio-deploy/20260513-053804-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-053804-guarded-deploy)

Live target after deploy:

- `driver_srcversion=2DA0FEA89EBE73F96BE4053`
- `override_module_size=450568`
- `lightdm_active=inactive`
- `kdump_service_active=active`
- `overall=remote_usable`

Canonical live bundles:

- exact no-redeploy `plan` regression proof:
  - [logs/vaio-plan-session/20260513-053956-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-053956-sgx-plan-session)
- focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-054056-sgx-submit-cmdta-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-054056-sgx-submit-cmdta-live/summary.log)
- typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-054133-sgx-multi-submit-cmdta-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-054133-sgx-multi-submit-cmdta-live/summary.log)

## Focused Evidence

- the exact `complete` regression proof stayed terminally clean on the new
  module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=2DA0FEA89EBE73F96BE4053`
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
- submit B retained explicit dependency state through the normalized CMDTA
  views and still retired cleanly:
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

The SGX-shaped kernel-internal execution stack now has seven validated layers:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`
- `psb_sgx_lab_submit_ccb_kick`
- `psb_sgx_lab_submit_sgx_ccb_kick`
- `psb_sgx_lab_submit_sgx_cmdta_shared`
- `psb_sgx_lab_submit_sgx_cmdta`

This is still lab-model execution, not real SGX rendering, but it is the
first layer that materializes a bounded CMDTA-level sync-object payload above
the shared sync/status topology. The next useful step is still a real SGX
execution object above `sgx_cmdta`, not another no-code rerun of the same
exact submit ladder.
