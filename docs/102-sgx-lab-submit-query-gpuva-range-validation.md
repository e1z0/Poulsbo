# SGX Lab Submit Query GPU-VA Range Validation

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion 9864A6D5176BBB851C0B51E`

## Goal

The earlier direct GPU-VA submit-query milestone could answer:

- which live submit owns this GPU VA
- which submit slot matched
- which adopted SGX object matched
- what the in-range GPU-VA offset was

One practical gap remained: that direct submit lookup still did not return the
backing `gtt_range` identity. A second range query was still needed to answer:

- which live range backs the matching object for this submit-side GPU VA
- whether that backing range is GEM, fb, cursor, or another range type

This follow-up extends the bounded `QUERY_SUBMIT gpuva` path so one typed query
now resolves:

- submit
- slot
- object
- backing range ID
- backing range type
- in-range offset

## Changes

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

Added to `drm_psb_sgx_lab_submit_query`:

- `resolved_range_id`
- `resolved_range_type`

Implementation changes:

- the internal GPU-VA submit resolver now also returns:
  - `gtt_range.debug_id`
  - lab-visible range type via `poulsbo_sgx_lab_range_type()`
- the typed submit-query fill path now preserves those fields in the returned
  submit query result

Userspace:

- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)

It now prints:

- `submit_resolved_range_id`
- `submit_resolved_range_type`
- `submit_resolved_range_type_name`

Validation runner:

- [run-vaio-sgx-ioctl-submit-query-gpuva.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh)

It now additionally asserts, for the live control-object GPU VA:

- the resolved range ID matches the control object’s backing range
- the resolved range type is `gem`

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy:

- live module:
  - `driver_srcversion=9864A6D5176BBB851C0B51E`
  - `override_module_size=387080`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`
  - `rollback_backup_size=386888`

Canonical live bundle:

- [logs/vaio-ioctl/20260511-160747-sgx-submit-query-gpuva-range-live](/root/Poulsbo/logs/vaio-ioctl/20260511-160747-sgx-submit-query-gpuva-range-live)

Saved result:

- `session_kind=sgx_submit_query_gpuva_range_validation`
- `session_outcome=completed`
- `finished_at=2026-05-11T16:08:27+03:00`

Target state from that bundle:

- `overall=remote_usable`
- `driver_srcversion=9864A6D5176BBB851C0B51E`
- `override_module_size=387080`
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
  - `submit_resolved_range_type=1`
  - `submit_resolved_range_type_name=gem`
  - `submit_query_gpu_va_offset=0x00001000`
- after userspace holder exit:
  - the same `submit / slot / object / range / type / offset` tuple still
    resolved cleanly
- after typed submit drop:
  - `SUBMIT_QUERY_GPUVA_AFTER_DROP_RC=3`
  - `submit_ioctl_ret=-1 errno=2`
- after typed object cleanup:
  - `object_result_count=5`

As with the earlier focused GPU-VA submit-query run, this milestone still uses
an attached-but-unvalidated submit. That is sufficient for ownership lookup,
and it keeps the direct read-side range correlation separate from later queue
and state-block setup.

## Result

One bounded `QUERY_SUBMIT gpuva` call now returns the full direct ownership
tuple for the matching VA:

- submit ID
- slot
- object ID
- range ID
- range type
- in-range offset

That closes the last practical read-side gap for direct GPU-VA-to-submit
inspection without needing a second typed range query.
