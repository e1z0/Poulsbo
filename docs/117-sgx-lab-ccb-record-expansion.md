# SGX Lab CCB Record Expansion

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 111E4E9C4E0999DA366432F`

## Goal

The execution-shaped submit path already had:

- `prepare`
- `arm`
- `emit`
- `launch`
- `dispatch`
- `retired`
- `report`

What it still lacked was one more SGX-shaped control-block step: the old
fixed CCB payload was still effectively a minimal four-dword generic record.
This batch expands that into a bounded non-executing 8-dword record before
any real SGX submit attempt.

## What Changed

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

The current memory-backed CCB record is now:

- service address
- cache control
- data0
- data1
- `pds_gpu_va32`
- `use_gpu_va32`
- `target_gpu_va32`
- `status_gpu_va32`

This batch updates the whole execution-shaped submit path coherently:

- `ccb` configuration now reserves and validates a 32-byte record instead of
  the old 16-byte payload
- `prepare` now writes the full 8-dword record into the backing `control`
  object
- queue-packet materialization now uses the same 8-dword record format
- `issue` and `transition` now preserve and rewrite the same expanded record
  instead of falling back to the earlier four-word shape
- typed submit query and typed dump now read back the new memory-backed
  GPU-VA32 words:
  - `submit_ccb_pds_gpu_va32`
  - `submit_ccb_use_gpu_va32`
  - `submit_ccb_target_gpu_va32`
  - `submit_ccb_status_gpu_va32`

Userspace coverage changed in:

- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)
- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)

The focused submit runner now explicitly checks that the memory-backed CCB
GPU-VA32 words match the attached live object GPU VAs after `prepare`.

## Validation

Local checks:

- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`
- `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `bash -n scripts/run-vaio-sgx-multi-submit.sh`
- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`

Guarded deploy:

- [logs/vaio-deploy/20260511-214857-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260511-214857-guarded-deploy)
- previous live override:
  - `srcversion E774FBBBD966270FEA6C67B`
  - `override_module_size=416736`
- rebuilt live override:
  - `srcversion 111E4E9C4E0999DA366432F`
  - `override_module_size=418808`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Canonical live bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260511-215431-sgx-submit-ccb-record-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-215431-sgx-submit-ccb-record-live/summary.log)
- typed multi-submit:
  - [logs/vaio-ioctl/20260511-215509-sgx-multi-submit-ccb-record-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-215509-sgx-multi-submit-ccb-record-live/summary.log)

Focused single-submit evidence:

- the memory-backed CCB record now resolves the attached object GPU VAs:
  - `submit_ccb_pds_gpu_va32=0x81123000`
  - `submit_ccb_use_gpu_va32=0x81127000`
  - `submit_ccb_target_gpu_va32=0x8112b000`
  - `submit_ccb_status_gpu_va32=0x8112f000`
- those match the live attached object fields exactly:
  - `submit_pds_gpu_va=0x81123000`
  - `submit_use_gpu_va=0x81127000`
  - `submit_target_gpu_va=0x8112b000`
  - `submit_status_gpu_va=0x8112f000`
- the focused path still completed cleanly through progression:
  - `submit_prepare_count=1`
  - `submit_validate_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- final typed cleanup still returned:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

Typed multi-submit evidence:

- submit A now carries one coherent expanded record through refill cycles:
  - final `submit_ccb_pds_gpu_va32=0x8112b000`
  - final `submit_ccb_use_gpu_va32=0x8112f000`
  - final `submit_ccb_target_gpu_va32=0x81133000`
  - final `submit_ccb_status_gpu_va32=0x81137000`
  - final progression counts:
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
- submit B now carries the same expanded record through the dependency-gated
  path:
  - final `submit_ccb_pds_gpu_va32=0x8113f000`
  - final `submit_ccb_use_gpu_va32=0x81143000`
  - final `submit_ccb_target_gpu_va32=0x81147000`
  - final `submit_ccb_status_gpu_va32=0x8114b000`
  - final progression counts:
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
  - emitted state still proves dependency activity:
    - `submit_emit_dependency_active_count=1`
- final typed cleanup still returned:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

## Conclusion

The lab submit path now materializes a more SGX-shaped non-executing CCB
record in backing memory instead of stopping at a fixed four-dword generic
payload.

That closes the immediate TODO around payload shape before any real execution
attempt. The next useful batch is above this layer: decide which additional
header or queue semantics beyond the current validated 8-dword record should
become explicit before the first real SGX submit experiment.
