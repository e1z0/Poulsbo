# SGX Lab Submit Manifest Op Ioctl

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 9E29F79593770BEEC5AEFD2`

## Goal

The execution-shaped lab path already had:

- `prepare`
- `arm`
- `emit`
- `launch`
- `dispatch`
- `retired`
- `report`
- the non-executing 8-dword CCB record

What it still lacked was one bounded control-side block that materialized the
current execution-shaped submit view directly in backing memory and could be
validated across:

- focused single-submit progression
- dependency-gated typed multi-submit progression
- refill cycles on a persistent submit

This batch closes that gap with a typed `manifest` block.

## What Changed

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

Typed submit-op surface:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_MANIFEST`
- userspace command:
  - `manifest <submit-selector> <offset>`

The memory-backed manifest now carries one coherent submit snapshot with:

- stage and stage count
- context and command type
- owner state and control flags
- queue base, size, packet size, packet count, and epoch
- CCB offset, read offset, and write offset
- active dependency count
- global issue/running/complete tickets
- `pds/use/target/status` GPU-VA32 words
- sync complete/pending sequence and sync flags
- owner issue/complete cookies
- config count and `last_manifest_jiffies`

The kernel now:

- validates manifest placement against:
  - the fixed control header
  - the configured queue region
  - the configured CCB region
- writes the manifest into the backing `control` object
- refreshes it together with the existing `report` block after:
  - `manifest`
  - `prepare`
  - `arm`
  - `emit`
  - `launch`
  - `issue`
  - `transition`
  - submit-shaping mutations that already invalidate later execution state
- reads it back through typed submit query and typed dump instead of exposing
  only the live in-kernel fields

Userspace coverage changed in:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)
- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)

The focused and multi-submit typed runners now configure and validate the
manifest block as part of the real progression flow instead of only checking
live query state.

## Validation

Local checks:

- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`
- `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `bash -n scripts/run-vaio-sgx-multi-submit.sh`
- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`

Guarded deploy:

- [logs/vaio-deploy/20260511-222440-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260511-222440-guarded-deploy)
- previous live override:
  - `srcversion 111E4E9C4E0999DA366432F`
  - `override_module_size=418808`
- rebuilt live override:
  - `srcversion 9E29F79593770BEEC5AEFD2`
  - `override_module_size=423020`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Canonical live bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260511-222845-sgx-submit-manifest-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-222845-sgx-submit-manifest-live/summary.log)
- typed multi-submit:
  - [logs/vaio-ioctl/20260511-222923-sgx-multi-submit-manifest-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-222923-sgx-multi-submit-manifest-live/summary.log)

Focused single-submit evidence:

- manifest configuration landed cleanly:
  - `submit_manifest_offset=0x180`
  - initial manifest state after configuration:
    - `submit_manifest_stage_name=none`
    - `submit_manifest_stage_count=0`
    - `submit_manifest_owner_state_name=queued`
    - `submit_manifest_queue_packet_count=1`
    - `submit_manifest_dependency_active_count=0`
- first-cycle manifest stage progression was validated end to end:
  - `prepared`
  - `armed`
  - `emitted`
  - `launched`
  - `dispatched`
  - `retired`
- the manifest captured the live queue and owner transitions inside that same
  first cycle:
  - after `issue`:
    - `submit_manifest_stage_name=dispatched`
    - `submit_manifest_owner_state_name=queued`
    - `submit_manifest_queue_packet_count=2`
  - after `transition running`:
    - `submit_manifest_stage_name=dispatched`
    - `submit_manifest_owner_state_name=running`
    - `submit_manifest_queue_packet_count=1`
  - after `transition complete`:
    - `submit_manifest_stage_name=retired`
    - `submit_manifest_owner_state_name=complete`
    - `submit_manifest_queue_packet_count=0`
- the manifest GPU-VA32 words matched the expanded CCB record exactly:
  - `submit_manifest_pds_gpu_va32=0x80c6f000`
  - `submit_manifest_use_gpu_va32=0x81123000`
  - `submit_manifest_target_gpu_va32=0x81127000`
  - `submit_manifest_status_gpu_va32=0x8112b000`
  - `submit_ccb_pds_gpu_va32=0x80c6f000`
  - `submit_ccb_use_gpu_va32=0x81123000`
  - `submit_ccb_target_gpu_va32=0x81127000`
  - `submit_ccb_status_gpu_va32=0x8112b000`
- focused progression still completed cleanly:
  - `submit_validate_count=1`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - final global tickets:
    - `submit_global_issue_ticket=7`
    - `submit_global_running_ticket=7`
    - `submit_global_complete_ticket=7`

Typed multi-submit evidence:

- the typed/default wrapper path completed cleanly on the new module with:
  - `coord_live_submit_count=2`
  - `coord_live_object_count=10`
  - `gtt_state_range_sgx_object_count=10`
  - `gtt_state_range_pf_bound_count=10`
- the dependency-gated submit preserved manifest dependency state before issue:
  - `submit_manifest_stage_name=prepared`
  - `submit_manifest_stage_count=1`
  - `submit_manifest_owner_state_name=queued`
  - `submit_manifest_queue_packet_count=1`
  - `submit_manifest_dependency_active_count=1`
- submit A final state:
  - `submit_prepare_count=1`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
  - final manifest:
    - `submit_manifest_stage_name=retired`
    - `submit_manifest_stage_count=3`
    - `submit_manifest_owner_state_name=complete`
    - `submit_manifest_queue_packet_count=0`
    - `submit_manifest_dependency_active_count=0`
- submit B final dependency-gated state:
  - `submit_prepare_count=1`
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
  - final manifest:
    - `submit_manifest_stage_name=retired`
    - `submit_manifest_stage_count=2`
    - `submit_manifest_owner_state_name=complete`
    - `submit_manifest_queue_packet_count=0`
    - `submit_manifest_dependency_active_count=1`
- final typed cleanup still returned:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

## Conclusion

The execution-shaped lab path now has one memory-backed control-side manifest
block that tracks and validates the current submit image through:

- `prepare`
- `arm`
- `emit`
- `launch`
- `dispatch`
- `retired`

That closes the remaining gap between the live in-kernel execution-shaped
state and one backing-memory snapshot that userspace can validate through the
typed ioctl path.

The next useful batch is above this layer: define the first more
execution-oriented SGX structure that consumes the current validated
`report + manifest + 8-dword CCB` model instead of adding more generic
inspection state.
