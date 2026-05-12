# SGX Lab Submit Query GPU-VA Validation

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion CA6AF1E225E6F5B90E66C7D`

## Goal

The earlier GPU-VA range-query milestones could answer:

- which live `gtt_range` contains this GPU VA
- whether that range belongs to an SGX lab object
- what semantic role/class/use/context that object carries
- whether that adopted object is referenced by any live submit

The next missing read-side step was the inverse lookup:

- start from one GPU VA and resolve the live SGX submit directly
- report which submit slot matched that VA
- report which adopted SGX object in that submit owns that VA
- keep the same answer stable after userspace closes
- fail cleanly once the submit itself is dropped

This milestone extends the bounded `QUERY_SUBMIT` path to do that directly,
instead of requiring a separate range query first.

## Changes

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

Added to `drm_psb_sgx_lab_submit_query`:

- `DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_GPU_VA`
- `query_gpu_va`
- `resolved_slot`
- `resolved_object_id`
- `query_gpu_va_offset`

Implementation changes:

- `QUERY_SUBMIT` selectors are now mutually exclusive across:
  - explicit submit ID
  - focused submit
  - live submit index
  - GPU VA
- `QUERY_SUBMIT` can now scan the live submit list and resolve the first submit
  whose attached object range contains the requested GPU VA
- the typed result now reports:
  - which slot matched
  - which adopted object matched
  - the in-range GPU-VA offset used for that match
- the live slot search currently checks:
  - `control`
  - `pds`
  - `use`
  - `target`
  - `status`

Userspace:

- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)

It now supports:

- `gpuva <gpu_va>`

and prints:

- `submit_query_gpu_va`
- `submit_resolved_slot`
- `submit_resolved_slot_name`
- `submit_resolved_object_id`
- `submit_query_gpu_va_offset`

Validation runner:

- [run-vaio-sgx-ioctl-submit-query-gpuva.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh)

It now:

- stages:
  - `drm_probe`
  - `psb_sgx_lab_range_query`
  - `psb_sgx_lab_object_op`
  - `psb_sgx_lab_submit_op`
  - `psb_sgx_lab_query`
- seeds five held GEM-backed ranges
- adopts them as five SGX lab objects
- assigns:
  - `control / packet / control_ccb / pf`
  - `pds / code / pds_code / pf`
  - `use / code / use_code / pf`
  - `target / surface / render_target / pf`
  - `status / state / status_block / pf`
- creates one PF-context submit
- attaches those five objects to the five submit slots
- resolves one control-object GPU VA through the typed range-query helper
- proves the new typed submit query resolves that same GPU VA back to:
  - the live submit
  - the `control` slot
  - the control object ID
  - the correct in-range offset
- proves the same answer survives userspace holder exit
- drops the submit and proves the same query then fails cleanly with `ENOENT`
- cleans up the adopted objects

This runner intentionally uses an attached-but-unvalidated submit. That is
enough for the GPU-VA ownership question here, and it avoids requiring the
later kick/queue/status/sync/owner block setup just to prove read-side submit
resolution.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy:

- live module:
  - `driver_srcversion=CA6AF1E225E6F5B90E66C7D`
  - `override_module_size=386888`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`
  - `rollback_backup_size=386328`

Canonical live bundle:

- [logs/vaio-ioctl/20260511-155105-sgx-submit-query-gpuva-live](/root/Poulsbo/logs/vaio-ioctl/20260511-155105-sgx-submit-query-gpuva-live)

Saved result:

- `session_kind=sgx_submit_query_gpuva_validation`
- `session_outcome=completed`
- `finished_at=2026-05-11T15:51:05+03:00`

Target state from that bundle:

- `overall=remote_usable`
- `driver_srcversion=CA6AF1E225E6F5B90E66C7D`
- `override_module_size=386888`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

Canonical live evidence:

- live staged ranges:
  - `RANGE_IDS=5 7 9 40 41`
- live adopted objects:
  - `OBJECT_IDS=31 32 33 34 35`
- live submit:
  - `SUBMIT_ID=7`
- canonical GPU VA:
  - `CONTROL_QUERY_GPU_VA=0x807c0000`
  - `CONTROL_QUERY_OFFSET=0x1000`

GPU-VA submit-query transition on that same VA:

- while the live submit exists:
  - `submit_selector=gpuva`
  - `submit_query_gpu_va=0x807c0000`
  - `submit_ioctl_ret=0 errno=0`
  - `submit_resolved_id=7`
  - `submit_resolved_slot_name=control`
  - `submit_resolved_object_id=31`
  - `submit_query_gpu_va_offset=0x00001000`
  - `submit_control_object_id=31`
  - `submit_control_gpu_va=0x807bf000`
  - `submit_validate_count=0`
  - `submit_issue_count=0`
  - `submit_transition_count=0`
- after userspace holder exit:
  - `submit_ioctl_ret=0 errno=0`
  - `submit_resolved_id=7`
  - `submit_resolved_slot_name=control`
  - `submit_resolved_object_id=31`
  - `submit_query_gpu_va_offset=0x00001000`
- after typed submit drop:
  - `coord_live_submit_count=0`
  - `submit_ioctl_ret=-1 errno=2`
- after typed object cleanup:
  - `object_result_count=5`
  - `submit_result_count=0`

## Result

The bounded submit-query path can now start from one GPU VA and resolve live
submit ownership directly, including:

- the matching submit ID
- the matching submit slot
- the matching adopted object ID
- the in-range GPU-VA offset used for the resolution

That answer stays stable after userspace closes and disappears cleanly after
submit drop. Together with the earlier GPU-VA range-query work, the lab now
has both directions of typed lookup:

- GPU VA -> range -> object -> first submit reference
- GPU VA -> resolved live submit -> resolved slot -> resolved object
