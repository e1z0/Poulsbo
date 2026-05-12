# SGX Lab Range Query GPU-VA Metadata Validation

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion 1DAAA2E3085BB97D83AA3D7`

## Goal

The earlier GPU-VA range-query milestone could answer:

- which live `gtt_range` contains this GPU VA
- whether that range currently has an `sgx_object_id`

The missing piece was object semantics. For future MMU or SGX fault work, the
useful question is usually not only "who owns this VA?" but also "what kind of
SGX object is it?"

This follow-up extends the same bounded range-query path so one GPU-VA lookup
also returns the live object's:

- role
- class
- intended GPU use
- expected context

## Changes

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

Added to `drm_psb_sgx_lab_range_query`:

- `sgx_object_role`
- `sgx_object_class`
- `sgx_object_gpu_use`
- `sgx_object_expected_context`

`QUERY_RANGE` now resolves those fields from the live SGX lab object list when
`range_sgx_object_id` is non-zero.

Userspace:

- [psb_sgx_lab_range_query.c](/root/Poulsbo/workloads/psb_sgx_lab_range_query.c)

It now prints:

- `range_sgx_object_role`
- `range_sgx_object_role_name`
- `range_sgx_object_class`
- `range_sgx_object_class_name`
- `range_sgx_object_gpu_use`
- `range_sgx_object_gpu_use_name`
- `range_sgx_object_expected_context`
- `range_sgx_object_expected_context_name`

Validation runner:

- [run-vaio-sgx-ioctl-range-query.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-range-query.sh)

It now:

- adopts the selected range into an SGX lab object
- mutates that object to:
  - `role=control`
  - `class=packet`
  - `gpu_use=control_ccb`
  - `context=pf`
- verifies those exact semantics through the GPU-VA query path
- verifies they survive userspace holder exit
- verifies they return to `generic/generic/generic/auto` after object drop

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `bash -n scripts/run-vaio-sgx-ioctl-range-query.sh`
- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy:

- new live module:
  - `driver_srcversion=1DAAA2E3085BB97D83AA3D7`
  - `override_module_size=385984`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Canonical live bundle:

- [logs/vaio-ioctl/20260511-151741-sgx-range-query-gpuva-metadata-live](/root/Poulsbo/logs/vaio-ioctl/20260511-151741-sgx-range-query-gpuva-metadata-live)

Saved result:

- `session_kind=sgx_range_query_gpuva_metadata_live`
- `session_outcome=completed`
- `finished_at=2026-05-11T15:18:13+03:00`

Target state from that bundle:

- `driver_srcversion=1DAAA2E3085BB97D83AA3D7`
- `override_module_size=385984`
- `overall=remote_usable`

Key live evidence:

- baseline GEM-range set before holder probes:
  - `BASELINE_RANGE_IDS=6 7 8 9 10 11 12`
- the warm target again had no fresh probe-created GEM range, so the runner
  intentionally fell back to:
  - `FRESH_RANGE_ID=6`
  - `FRESH_RANGE_IS_NEW=0`
- canonical live VA:
  - `range_query_gpu_va=0x807c0000`
  - `range_query_gpu_va_offset=0x00001000`
  - `range_resolved_id=6`
- before adopt:
  - `range_sgx_object_id=0`
  - `range_sgx_object_role_name=generic`
  - `range_sgx_object_class_name=generic`
  - `range_sgx_object_gpu_use_name=generic`
  - `range_sgx_object_expected_context_name=auto`
- after typed object adopt plus metadata setup:
  - `object_resolved_id=1`
  - `range_sgx_object_id=1`
  - `range_sgx_object_role_name=control`
  - `range_sgx_object_class_name=packet`
  - `range_sgx_object_gpu_use_name=control_ccb`
  - `range_sgx_object_expected_context_name=pf`
- after userspace holder exit:
  - `range_sgx_object_id=1`
  - `range_sgx_object_role_name=control`
  - `range_sgx_object_class_name=packet`
  - `range_sgx_object_gpu_use_name=control_ccb`
  - `range_sgx_object_expected_context_name=pf`
- after typed object drop:
  - `RANGE_QUERY_BY_GPUVA_AFTER_DROP_RC=0`
  - `range_sgx_object_id=0`
  - `range_sgx_object_role_name=generic`
  - `range_sgx_object_class_name=generic`
  - `range_sgx_object_gpu_use_name=generic`
  - `range_sgx_object_expected_context_name=auto`

Like the earlier GPU-VA bundle, this canonical run stayed on a persistent
baseline range rather than a fresh probe-created one. So the final post-drop
GPU-VA query still resolved the range, but the SGX ownership and SGX object
semantics were correctly gone.

## Result

One typed GPU-VA lookup now answers all of the practical lab questions at
once:

- which range contains this GPU VA
- whether that range currently belongs to an SGX lab object
- what semantic role/class/use/context that object currently carries

That makes the GPU-VA query path directly useful for future MMU or SGX fault
triage without needing a second object query.
