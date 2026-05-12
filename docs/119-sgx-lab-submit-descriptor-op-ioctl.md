# SGX Lab Submit Descriptor Op Ioctl

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 9E540439CC3313278F803B0`

## Goal

The execution-shaped lab path already had:

- `prepare`
- `arm`
- `emit`
- `launch`
- `dispatch`
- `retired`
- the memory-backed `report` block
- the memory-backed `manifest` block
- the expanded non-executing 8-dword CCB record

What it still lacked was one bounded control-side block that directly
materialized the current execution-shaped submit record in backing memory in a
form that was closer to a real SGX submission descriptor than the higher-level
`report` or `manifest` views.

This batch closes that gap with a typed `descriptor` block.

## What Changed

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

Typed submit-op surface:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_DESCRIPTOR`
- userspace command:
  - `descriptor <submit-selector> <offset>`

The memory-backed descriptor now carries:

- stage and stage count
- context and command type
- owner state
- issued packet count
- completed packet count
- active dependency count
- queue packet count and queue epoch
- control flags
- queue base, size, and packet size
- `ccb`, `sync`, `owner`, `report`, and `manifest` offsets
- global issue/running/complete tickets
- report flags
- TA/3D status offsets, stride, and seed
- `control/pds/use/target/status` GPU-VA32 words
- config count and `last_descriptor_jiffies`

The kernel now:

- validates descriptor placement against:
  - the fixed control header
  - the configured queue region
  - the configured CCB region
  - the configured manifest region
- writes the descriptor into the backing `control` object
- refreshes it together with `report` and `manifest` after:
  - `descriptor`
  - `prepare`
  - `arm`
  - `emit`
  - `launch`
  - `issue`
  - `transition`
- reads it back through typed submit query and typed dump

Userspace and runner coverage changed in:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)
- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)

The focused runner also now prints the captured descriptor-op output on failure.

## Selector Bug Fix

The first focused validation run exposed a userspace bug, not a kernel one:

- `descriptor focus 0x200` returned `errno=2`
- the captured output showed:
  - `submit_selector=focus`
  - `submit_flags=0x0`
  - `submit_resolved_id=0`

Root cause:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
  did not list `DRM_PSB_SGX_LAB_SUBMIT_OP_DESCRIPTOR` in
  `submit_op_uses_selector()`

Fix:

- add `DRM_PSB_SGX_LAB_SUBMIT_OP_DESCRIPTOR` to the selector-using op table
- rebuild the i386 workloads
- rerun the live validation on the already-deployed kernel module

No second kernel deploy was needed after that userspace fix.

## Validation

Local checks:

- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`
- `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- final target health:
  - `overall=remote_usable`

Live target state after the successful reruns:

- `driver_srcversion=9E540439CC3313278F803B0`
- `override_module_size=427328`
- `rollback_pending_present=0`

Canonical live bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260511-230546-sgx-submit-descriptor-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-230546-sgx-submit-descriptor-live/summary.log)
- typed multi-submit:
  - [logs/vaio-ioctl/20260511-230625-sgx-multi-submit-descriptor-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-230625-sgx-multi-submit-descriptor-live/summary.log)

Focused single-submit evidence:

- `SUBMIT_ID=4`
- the descriptor now progresses end to end in backing memory:
  - `none`
  - `prepared`
  - `armed`
  - `emitted`
  - `launched`
  - `dispatched`
  - `retired`
- final descriptor block:
  - `submit_descriptor_offset=0x200`
  - `submit_descriptor_stage_name=retired`
  - `submit_descriptor_config_count=1`
- the expanded CCB and descriptor GPU-VA32 words matched the attached objects:
  - `submit_ccb_pds_gpu_va32=0x81123000`
  - `submit_ccb_use_gpu_va32=0x81127000`
  - `submit_ccb_target_gpu_va32=0x8112b000`
  - `submit_ccb_status_gpu_va32=0x8112f000`
  - `submit_descriptor_control_gpu_va32=0x80c6f000`
  - `submit_descriptor_pds_gpu_va32=0x81123000`
  - `submit_descriptor_use_gpu_va32=0x81127000`
  - `submit_descriptor_target_gpu_va32=0x8112b000`
  - `submit_descriptor_status_gpu_va32=0x8112f000`
- the focused path still completed cleanly:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`

Typed multi-submit evidence:

- `SUBMIT_SETUP=ioctl`
- `OBJECT_SETUP=ioctl`
- `RANGE_DISCOVERY=ioctl`
- `DEPENDENCY_SETUP=ioctl`
- `VERIFICATION_MODE=ioctl`
- `SUBMIT_A_ID=6`
- `SUBMIT_B_ID=7`
- the descriptor stayed active through dependency gating:
  - `submit_descriptor_dependency_active_count=1`
  - `submit_emit_dependency_active_count=1`
- submit A final:
  - `submit_descriptor_stage_name=retired`
  - `submit_descriptor_config_count=1`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
- submit B final:
  - `submit_descriptor_stage_name=retired`
  - `submit_descriptor_config_count=1`
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

The lab path now has four execution-shaped layers that agree with each other:

- in-kernel live submit state
- the memory-backed `report` block
- the memory-backed `manifest` block
- the memory-backed `descriptor` block
- the expanded 8-dword CCB record

That is still not real SGX execution, but it is a much better foundation for
the next kernel batch: the first execution-oriented SGX structure that consumes
this validated descriptor-manifest-CCB model instead of only validating it.
