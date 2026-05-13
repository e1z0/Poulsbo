# SGX Submit Exec Packet Structure

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion A154282637C389A3032C8D9`

## Goal

The shared kernel-internal submit model already had one normalized execution
record:

- `psb_sgx_lab_submit_exec_record`

That was enough to unify the read-side typed submit views, but it still left
the fixed queue/sync/owner/CCB image builders rebuilding packet-shaped state
piecemeal. This batch adds the next internal SGX-shaped layer above the record:
one explicit execution packet structure that can feed both the fixed blocks and
the typed control-backed views.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal structures:

- `struct psb_sgx_lab_submit_ccb_command`
- `struct psb_sgx_lab_submit_ta_shared`
- `struct psb_sgx_lab_submit_exec_packet`

New internal flow:

- `poulsbo_sgx_build_submit_exec_record_locked()`
  -> `poulsbo_sgx_fill_submit_exec_packet_from_record_locked()`
  -> `poulsbo_sgx_build_submit_exec_packet_locked()`

The new packet now carries one SGX-shaped execution image with:

- stage, stage count, source, context, and command type
- issued/completed/refilled packet state
- queue geometry plus computed head/tail packet counts
- CCB offsets and 4-word command payload
- control/status object ids and GPU-VA32 words
- shared TA-side control, status, sync, owner, ticket, and dependency fields

That packet is now the common builder input for:

- `queue header`
- `ccb record`
- `sync`
- `owner`
- `report`
- `live`
- `result`
- `manifest`
- `descriptor`
- `plan`
- `issue-plan`

The `issue-plan` builder also now uses the packet consistently:

- stage and stage count come from the `live` packet source
- the base launched packet image comes from the `launch` packet source
- predicted post-issue fields are then overlaid from the existing issue-plan
  helper

No userspace uAPI changed in this batch. The new structure is kernel-internal
only.

## Runner Follow-Up

The kernel change exposed two real runner-side expectation bugs in
[run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh):

- `submit_issue_plan_dependency_active_count` stays `0` before `launch`
- for the dependent submit, it becomes `1` at `launch`
- after `issue` and at final `retired` snapshots, it is back to `0`

The typed multi-submit runner now matches that hardware-observed behavior.
The intermediate typed reruns that exposed those two mismatches are:

- [logs/vaio-ioctl/20260513-030955-sgx-multi-submit-exec-packet-live-typed/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-030955-sgx-multi-submit-exec-packet-live-typed/summary.log)
- [logs/vaio-ioctl/20260513-031242-sgx-multi-submit-exec-packet-live-typed-rerun/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-031242-sgx-multi-submit-exec-packet-live-typed-rerun/summary.log)

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`
- `bash -n scripts/run-vaio-sgx-multi-submit.sh`

Guarded deploy:

- [logs/vaio-deploy/20260513-030640-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-030640-guarded-deploy)

Live target after deploy:

- `driver_srcversion=A154282637C389A3032C8D9`
- `override_module_size=447160`
- `lightdm_active=inactive`
- `kdump_service_active=active`
- `overall=remote_usable`

Canonical live bundles:

- exact no-redeploy `plan` regression proof:
  - [logs/vaio-plan-session/20260513-030828-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-030828-sgx-plan-session)
- focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-030828-sgx-submit-exec-packet-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-030828-sgx-submit-exec-packet-live/summary.log)
- typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-031429-sgx-multi-submit-exec-packet-live-typed-rerun2/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-031429-sgx-multi-submit-exec-packet-live-typed-rerun2/summary.log)

## Focused Evidence

- the exact `complete` regression proof stayed terminally clean on the new
  module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=A154282637C389A3032C8D9`
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
  - `submit_live_stage_count=1`
  - `submit_result_stage_name=retired`
  - `submit_result_stage_count=1`
  - `submit_issue_plan_stage_name=retired`
  - `submit_issue_plan_stage_count=1`
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`

## Typed Multi-Submit Evidence

- submit A final:
  - `submit_live_stage_name=retired`
  - `submit_live_stage_count=3`
  - `submit_result_stage_name=retired`
  - `submit_result_stage_count=3`
  - `submit_issue_plan_stage_name=retired`
  - `submit_issue_plan_stage_count=3`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
- submit B final:
  - `submit_live_stage_name=retired`
  - `submit_live_stage_count=2`
  - `submit_result_stage_name=retired`
  - `submit_result_stage_count=2`
  - `submit_issue_plan_stage_name=retired`
  - `submit_issue_plan_stage_count=2`
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
- the dependent typed path now proves the finer issue-plan behavior:
  - submit-level `live/result/manifest/descriptor/plan` dependency-active state
    is `1` while submit B is still gated
  - `issue-plan` dependency-active state is:
    - `0` before `launch`
    - `1` at `launch`
    - `0` again once the launched packet is consumed and later retired
- final cleanup returned to zero live SGX state:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

## Result

The shared execution-shaped kernel model now has two explicit internal layers:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`

That is still not real SGX execution, but it is the first more
execution-oriented kernel structure above the record-only model, and it now
drives the fixed queue/CCB/sync/owner blocks and the typed submit views through
one common packet image. The next useful step is to keep pushing this packet
model toward the first truly execution-oriented SGX kernel object instead of
adding more read-side plumbing.
