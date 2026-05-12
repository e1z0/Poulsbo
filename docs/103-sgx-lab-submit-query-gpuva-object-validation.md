# SGX Lab Submit Query GPU-VA Object Validation

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion 19426C7ED791DCAAD913C11`

## Goal

The earlier direct GPU-VA submit-query milestones could already resolve:

- submit ID
- submit slot
- adopted object ID
- backing range ID and type
- in-range offset

One last practical gap remained: one `QUERY_SUBMIT gpuva` call still could not
answer the semantic meaning of the matched object without a second typed object
or range query.

This follow-up extends the same bounded submit-query result so one direct
GPU-VA lookup now also returns the matched object’s:

- role
- class
- GPU use
- expected context

## Changes

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

Added to `drm_psb_sgx_lab_submit_query`:

- `resolved_object_role`
- `resolved_object_class`
- `resolved_object_gpu_use`
- `resolved_object_expected_context`

Implementation changes:

- once `QUERY_SUBMIT gpuva` resolves the matching object ID, the submit-query
  fill path now also resolves that object under the existing object lock and
  copies its semantic metadata into the typed result

Userspace:

- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)

It now prints:

- `submit_resolved_object_role`
- `submit_resolved_object_role_name`
- `submit_resolved_object_class`
- `submit_resolved_object_class_name`
- `submit_resolved_object_gpu_use`
- `submit_resolved_object_gpu_use_name`
- `submit_resolved_object_expected_context`
- `submit_resolved_object_expected_context_name`

Validation runner:

- [run-vaio-sgx-ioctl-submit-query-gpuva.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh)

It now also asserts, on the same control-object GPU VA:

- `role=control`
- `class=packet`
- `gpu_use=control_ccb`
- `expected_context=pf`

both before and after the userspace holders exit.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy:

- live module:
  - `driver_srcversion=19426C7ED791DCAAD913C11`
  - `override_module_size=387144`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`
  - `rollback_backup_size=387080`

Canonical live bundle:

- [logs/vaio-ioctl/20260511-161543-sgx-submit-query-gpuva-object-live](/root/Poulsbo/logs/vaio-ioctl/20260511-161543-sgx-submit-query-gpuva-object-live)

Saved result:

- `session_kind=sgx_submit_query_gpuva_object_validation`
- `session_outcome=completed`
- `finished_at=2026-05-11T16:16:23+03:00`

Target state from that bundle:

- `overall=remote_usable`
- `driver_srcversion=19426C7ED791DCAAD913C11`
- `override_module_size=387144`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

Canonical live evidence:

- live staged ranges:
  - `RANGE_IDS=7 9 10 11 12`
- live adopted objects:
  - `OBJECT_IDS=1 2 3 4 5`
- live submit:
  - `SUBMIT_ID=1`
- canonical GPU VA:
  - `CONTROL_QUERY_GPU_VA=0x807c0000`
  - `CONTROL_QUERY_OFFSET=0x1000`

Direct submit-query result on that same VA:

- while the live submit exists:
  - `submit_resolved_id=1`
  - `submit_resolved_slot_name=control`
  - `submit_resolved_object_id=1`
  - `submit_resolved_range_id=7`
  - `submit_resolved_range_type_name=gem`
  - `submit_resolved_object_role_name=control`
  - `submit_resolved_object_class_name=packet`
  - `submit_resolved_object_gpu_use_name=control_ccb`
  - `submit_resolved_object_expected_context_name=pf`
  - `submit_query_gpu_va_offset=0x00001000`
- after userspace holder exit:
  - the same `submit / slot / object / range / role / class / gpu_use /
    context / offset` tuple still resolved cleanly
- after typed submit drop:
  - `SUBMIT_QUERY_GPUVA_AFTER_DROP_RC=3`
  - `submit_ioctl_ret=-1 errno=2`
- after typed object cleanup:
  - `object_result_count=5`

## Result

One bounded `QUERY_SUBMIT gpuva` call now returns the full direct ownership and
semantic tuple for the matching VA:

- submit ID
- slot
- object ID
- range ID
- range type
- object role
- object class
- object GPU use
- object expected context
- in-range offset

That closes the remaining practical gap for direct GPU-VA-to-submit inspection
without needing a follow-up object or range query.
