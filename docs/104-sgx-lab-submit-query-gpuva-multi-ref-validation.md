# SGX Lab Submit Query GPU-VA Multi-Ref Validation

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion 11920D5153C981A7E8A264A`

## Goal

The earlier direct GPU-VA submit-query milestones could already resolve one
matching live submit and return:

- submit ID
- slot
- object ID
- range ID and type
- object role/class/GPU use/expected context
- in-range offset

One practical ambiguity remained: when more than one live submit references the
same adopted object in the same slot, the direct `QUERY_SUBMIT gpuva` path
could not say whether the resolved submit was unique or just the first match.

This follow-up extends the same bounded submit-query result so one direct
GPU-VA lookup now also returns:

- how many live submits currently match that VA

That lets the same typed read-side path prove a live handoff from one submit to
another without first going through `QUERY_RANGE`.

## Changes

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

Added to `drm_psb_sgx_lab_submit_query`:

- `matching_submit_count`

Implementation changes:

- the GPU-VA submit resolver now keeps the first matching submit as the
  resolved submit
- it continues scanning the live submit list for the same GPU VA
- additional matches increment `matching_submit_count`
- explicit-ID, focused-submit, and submit-index queries set
  `matching_submit_count=1` on success

Userspace:

- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)

It now prints:

- `submit_matching_submit_count`

Validation runners:

- [run-vaio-sgx-ioctl-submit-query-gpuva.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh)
- [run-vaio-sgx-ioctl-submit-query-gpuva-multi-ref.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-query-gpuva-multi-ref.sh)

The base runner now supports `SHARED_CONTROL_SUBMITS=2`. In that mode it:

- creates a second PF-context submit
- attaches the same control object to the second submit's `control` slot
- proves the shared GPU VA first resolves submit A with
  `matching_submit_count=2`
- drops submit A and proves the same GPU VA then resolves submit B with
  `matching_submit_count=1`
- drops submit B and proves final `ENOENT`

The wrapper locks that scenario on as the canonical multi-ref validation path.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
- `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva-multi-ref.sh`
- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy:

- live module:
  - `driver_srcversion=11920D5153C981A7E8A264A`
  - `override_module_size=387336`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`
  - `rollback_backup_size=387144`

Canonical live bundle:

- [logs/vaio-ioctl/20260511-162454-sgx-submit-query-gpuva-multi-ref-live](/root/Poulsbo/logs/vaio-ioctl/20260511-162454-sgx-submit-query-gpuva-multi-ref-live)

Saved result:

- `session_kind=sgx_submit_query_gpuva_multi_ref_validation`
- `session_outcome=completed`
- `finished_at=2026-05-11T16:25:34+03:00`

Target state from that bundle:

- `overall=remote_usable`
- `driver_srcversion=11920D5153C981A7E8A264A`
- `override_module_size=387336`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

Canonical live evidence:

- live staged ranges:
  - `RANGE_IDS=7 9 10 11 12`
- live adopted objects:
  - `OBJECT_IDS=1 2 3 4 5`
- live submits:
  - `SUBMIT_ID=1`
  - `SECOND_SUBMIT_ID=2`
- canonical GPU VA:
  - `CONTROL_QUERY_GPU_VA=0x807c0000`
  - `CONTROL_QUERY_OFFSET=0x1000`

Direct submit-query result on that same VA:

- while both live submits exist:
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
  - `submit_matching_submit_count=2`
- after userspace holder exit:
  - submit A still resolved first
  - `submit_matching_submit_count=2`
- after typed submit-A drop:
  - the same GPU VA resolved submit B instead:
    - `submit_resolved_id=2`
    - `submit_resolved_slot_name=control`
    - `submit_resolved_object_id=1`
    - `submit_resolved_range_id=7`
    - `submit_matching_submit_count=1`
- after typed submit-B drop:
  - `SUBMIT_QUERY_GPUVA_AFTER_DROP_RC=3`
  - `submit_ioctl_ret=-1 errno=2`
- after typed object cleanup:
  - `object_result_count=5`

## Result

One bounded `QUERY_SUBMIT gpuva` call can now distinguish between:

- a unique live submit reference
- multiple live submit matches on the same GPU VA
- no remaining live submit at all

That closes the remaining ambiguity in the direct GPU-VA-to-submit read path.
The canonical shared-object handoff is now hardware-validated as:

- `matching_submit_count=2`
- `matching_submit_count=1`
- `ENOENT`
