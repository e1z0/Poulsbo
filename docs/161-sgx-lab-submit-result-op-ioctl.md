# SGX Lab Submit `result` Op Ioctl

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion EAD330CBA159A4483448EBF`

## Goal

The execution-shaped lab path already had the memory-backed:

- `report` block
- `manifest` block
- `descriptor` block
- `plan` block
- expanded non-executing 8-dword CCB record

What it still lacked was one typed status-side block that snapshots the
current execution result in backing memory in a form that can be compared
directly across:

- live submit state
- post-`issue` dispatch state
- final retired state

This batch closes that gap with a bounded `result` block.

## What Changed

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

Typed submit-op surface:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_RESULT`
- userspace command:
  - `result <submit-selector> <offset>`

The memory-backed result block now carries:

- stage and stage count
- selected source:
  - `none`
  - `live`
  - `dispatch`
  - `retired`
- context and command type
- owner state
- issued packet count
- completed packet count
- active dependency count
- queue packet count and queue epoch
- sync fields and owner-cookie fields
- global issue/running/complete tickets
- control flags and status-layout fields
- `control` and `status` object IDs and GPU-VA32 words
- linked `report`, `manifest`, `descriptor`, and `plan` offsets
- config count and `last_result_jiffies`

The kernel now:

- validates result placement against the status-side layout
- writes the result into the backing `status` object
- refreshes it together with the other typed submit views
- exposes it through typed submit query and typed dump

Userspace and runner coverage changed in:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)
- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)

## Validation Follow-Ups

The final passing batch also included two important runner-side fixes:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
  now treats `result` as a selector-using op, so `result focus <offset>` no
  longer drops the focused-submit selector
- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
  now attaches the typed objects before `kick`, matching the focused path, and
  now uses the corrected refill-cycle `plan` expectations

Those were validation-surface bugs, not kernel panics, and they are part of
the final passing state documented here.

## Validation

Local checks:

- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`
- `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `bash -n scripts/run-vaio-sgx-multi-submit.sh`
- `bash -n scripts/run-vaio-sgx-multi-submit-typed.sh`
- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- final target health:
  - `overall=remote_usable`

Current live target state after the passing reruns:

- guarded deploy:
  - [logs/vaio-deploy/20260512-234450-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260512-234450-guarded-deploy)
- `driver_srcversion=EAD330CBA159A4483448EBF`
- `override_module_size=434112`
- `lightdm_active=inactive`
- `kdump_service_active=active`
- `rollback_pending_present=0`

Canonical live bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260513-000332-sgx-submit-result-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-000332-sgx-submit-result-live/summary.log)
- typed multi-submit:
  - [logs/vaio-ioctl/20260513-000411-sgx-multi-submit-result-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-000411-sgx-multi-submit-result-live/summary.log)
- exact no-redeploy `plan` regression:
  - [logs/vaio-plan-session/20260512-235844-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-235844-sgx-plan-session)

## Focused Single-Submit Evidence

- the `result` block now progresses in backing memory through:
  - `prepared`
  - `armed`
  - `emitted`
  - `launched`
  - `dispatched`
  - `retired`
- final focused result block:
  - `submit_result_offset=0x180`
  - `submit_result_stage_name=retired`
  - `submit_result_stage_count=1`
  - `submit_result_source_name=retired`
  - `submit_result_owner_state_name=complete`
  - `submit_result_issued_packets=1`
  - `submit_result_completed_packets=2`
  - `submit_result_dependency_active_count=0`
  - `submit_result_queue_packet_count=0`
  - `submit_result_queue_epoch=0`
  - `submit_result_control_gpu_va32=0x807bf000`
  - `submit_result_status_gpu_va32=0x807cf000`
  - `submit_result_report_offset=0x120`
  - `submit_result_manifest_offset=0x180`
  - `submit_result_descriptor_offset=0x200`
  - `submit_result_plan_offset=0x300`
  - `submit_result_config_count=1`
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

- `SUBMIT_A_ID=23`
- `SUBMIT_B_ID=24`
- submit A final:
  - `submit_result_stage_name=retired`
  - `submit_result_stage_count=3`
  - `submit_result_source_name=retired`
  - `submit_result_owner_state_name=complete`
  - `submit_result_dependency_active_count=0`
  - `submit_result_queue_epoch=2`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
- submit B final:
  - `submit_result_stage_name=retired`
  - `submit_result_stage_count=2`
  - `submit_result_source_name=retired`
  - `submit_result_owner_state_name=complete`
  - `submit_result_dependency_active_count=1`
  - `submit_result_queue_epoch=1`
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

The SGX lab submit model now has one more bounded memory-backed layer that
agrees with the live state:

- in-kernel live submit state
- memory-backed `report`
- memory-backed `result`
- memory-backed `manifest`
- memory-backed `descriptor`
- memory-backed `plan`
- expanded 8-dword CCB record

That still is not real SGX execution, but it gives the next kernel batch a
better execution-shaped status surface to consume. The next meaningful step is
no longer another exact single-submit rerun; it is the first more
execution-oriented SGX kernel structure built on top of this validated
`result + report + manifest + descriptor + plan + CCB` model.
