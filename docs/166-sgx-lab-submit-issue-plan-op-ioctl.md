# SGX Lab Submit `issue-plan` Op Ioctl

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 228151D0E1E224804A6E1E6`

## Goal

The typed SGX lab submit image already had:

- `report`
- `live`
- `result`
- `manifest`
- `descriptor`
- `plan`
- the expanded non-executing 8-dword CCB record

What it still lacked was one explicit memory-backed control-side checkpoint for
the launched pre-issue packet plan itself. This batch adds that bounded
`issue-plan` block and validates it on real hardware through both the focused
single-submit path and the typed multi-submit path.

## What Changed

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

Typed submit-op surface:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE_PLAN`
- userspace command:
  - `issue-plan <submit-selector> <offset> <queued|running> <packet_count>`

The new memory-backed `issue-plan` block carries:

- stage and stage count
- valid state
- owner state
- issued packet count
- refill-from-complete state
- queue packet count and queue epoch
- queue base, size, packet size, tail, and head
- CCB read, write, and issue offsets
- sync fields and owner-cookie fields
- global issue, running, and complete tickets
- active dependency count
- CCB words
- `control`, `status`, `pds`, `use`, `target`, and `ccb status` ids and
  GPU-VA32 words
- config count and `last_issue_plan_jiffies`

The kernel now:

- validates `issue-plan` placement against the control-side layout
- writes the `issue-plan` block into the backing `control` object
- refreshes it with the other typed submit views
- exposes it through typed submit query

Userspace and runner coverage changed in:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)

## Validation Notes

The final passing runner state also corrected two concrete expectation bugs in
the focused validation path:

- the launch-time `issue-plan` view predicts the wrapped CCB write pointer, so
  `submit_issue_plan_ccb_write_offset=0x0` on the fresh focused submit
- the launch-time `issue-plan` view keeps pre-issue running and complete
  tickets, so `submit_issue_plan_global_running_ticket=0` and
  `submit_issue_plan_global_complete_ticket=0` on the fresh focused submit

Those were runner-side interpretation bugs, not kernel regressions. The final
passing state documented here includes both fixes.

## Validation

Local checks:

- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`
- `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `bash -n scripts/run-vaio-sgx-multi-submit.sh`
- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- final target health:
  - `overall=remote_usable`

Current live target state after the passing reruns:

- guarded deploy:
  - [logs/vaio-deploy/20260513-023337-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-023337-guarded-deploy)
- `driver_srcversion=228151D0E1E224804A6E1E6`
- `override_module_size=446824`
- `lightdm_active=inactive`
- `kdump_service_active=active`
- `rollback_pending_present=0`

Canonical live bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260513-023956-sgx-submit-issue-plan-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-023956-sgx-submit-issue-plan-live/summary.log)
- typed multi-submit:
  - [logs/vaio-ioctl/20260513-024024-sgx-multi-submit-issue-plan-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-024024-sgx-multi-submit-issue-plan-live/summary.log)
- exact no-redeploy `plan` regression:
  - [logs/vaio-plan-session/20260513-024621-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-024621-sgx-plan-session)

## Focused Single-Submit Evidence

- the new `issue-plan` block now becomes valid at `launch`
- at `launch`, the focused proof shows:
  - `submit_issue_plan_stage_name=launched`
  - `submit_issue_plan_valid=1`
  - `submit_issue_plan_owner_state_name=queued`
  - `submit_issue_plan_issued_packets=1`
  - `submit_issue_plan_queue_packet_count=2`
  - `submit_issue_plan_queue_epoch=0`
  - `submit_issue_plan_ccb_write_offset=0x0`
  - `submit_issue_plan_global_running_ticket=0`
  - `submit_issue_plan_global_complete_ticket=0`
- after `issue -> running -> complete`, the same block retires and clears:
  - `submit_issue_plan_stage_name=retired`
  - `submit_issue_plan_stage_count=1`
  - `submit_issue_plan_valid=0`
- the focused path still completed cleanly:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`

## Typed Multi-Submit Evidence

- submit A final:
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
  - `submit_issue_plan_stage_name=retired`
  - `submit_issue_plan_stage_count=2`
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
- final cleanup returned to zero live SGX state:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

## Result

The SGX lab submit model now has one more explicit memory-backed execution
checkpoint:

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

That still is not real SGX execution, but it gives the next kernel batch one
more validated execution-shaped control-side view to consume. The next
meaningful step is still the first more execution-oriented SGX kernel
structure built on top of this validated
`queue + ccb + sync + owner + report + live + result + manifest + descriptor + plan + issue-plan`
model.
