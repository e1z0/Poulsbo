# SGX Lab Range Query GPU-VA Validation

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion C69057A8C6926335A87C2A9`

## Goal

Extend the bounded range-query path so it can resolve a live `gtt_range` by GPU
VA instead of only by:

- live match index
- explicit `range_id`

That gives the SGX lab path a direct way to map future MMU or SGX-side GPU
addresses back onto the current range/object model.

## Changes

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

Added:

- `DRM_PSB_SGX_LAB_QUERY_RANGE_USE_GPU_VA`
- `query_gpu_va`
- `query_gpu_va_offset`

`QUERY_RANGE` now accepts:

- `index <query_index> [gem]`
- `id <range_id> [gem]`
- `gpuva <gpu_va> [gem]`

and resolves the containing range when the requested GPU VA falls within:

- `range_gpu_va <= query_gpu_va < range_gpu_va + range_len`

Userspace:

- [psb_sgx_lab_range_query.c](/root/Poulsbo/workloads/psb_sgx_lab_range_query.c)

It now supports:

- `gpuva <value> [gem]`

and prints both:

- `range_query_gpu_va`
- `range_query_gpu_va_offset`

Validation runner:

- [run-vaio-sgx-ioctl-range-query.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-range-query.sh)

It now:

- captures a baseline live GEM-range set before holder probes
- enumerates the live GEM-range set while probes are active
- prefers a fresh probe-created range when one exists
- falls back to a persistent baseline range when the target already has no
  fresh candidate
- validates lookup by:
  - `range_id`
  - GPU VA inside the resolved range
- adopts the selected range into an SGX lab object
- validates that the same GPU VA reflects `sgx_object_id` ownership
- validates that the same ownership survives userspace holder exit
- validates that after object drop, the same GPU VA no longer reports SGX
  ownership

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `bash -n scripts/run-vaio-sgx-ioctl-range-query.sh`
- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy:

- new live module:
  - `driver_srcversion=C69057A8C6926335A87C2A9`
  - `override_module_size=385832`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Canonical live bundle:

- [logs/vaio-ioctl/20260511-150817-sgx-range-query-gpuva-live](/root/Poulsbo/logs/vaio-ioctl/20260511-150817-sgx-range-query-gpuva-live)

Saved result:

- `session_kind=sgx_range_query_gpuva_live`
- `session_outcome=completed`
- `finished_at=2026-05-11T15:08:48+03:00`

Target state from that bundle:

- `driver_srcversion=C69057A8C6926335A87C2A9`
- `override_module_size=385832`
- `overall=remote_usable`

Key live evidence:

- baseline GEM-range set before holder probes:
  - `BASELINE_RANGE_IDS=5 7 8 9 16 17 18`
- no fresh probe-created GEM range appeared on this warm target, so the runner
  intentionally fell back to:
  - `FRESH_RANGE_ID=5`
  - `FRESH_RANGE_IS_NEW=0`
- explicit `range_id` lookup resolved:
  - `range_resolved_id=5`
  - `range_gpu_va=0x807bf000`
  - `range_len=0x004b0000`
- GPU-VA lookup inside that range:
  - `range_query_gpu_va=0x807c0000`
  - `range_query_gpu_va_offset=0x00001000`
  - `range_resolved_id=5`
  - `range_sgx_object_id=0`
- typed object adopt:
  - `object_resolved_id=2`
- GPU-VA lookup after adopt:
  - `range_resolved_id=5`
  - `range_sgx_object_id=2`
- GPU-VA lookup after userspace holder exit:
  - `range_resolved_id=5`
  - `range_sgx_object_id=2`
- GPU-VA lookup after object drop:
  - `RANGE_QUERY_BY_GPUVA_AFTER_DROP_RC=0`
  - `range_resolved_id=5`
  - `range_sgx_object_id=0`

That final `RC=0` is expected in this canonical run because the selected range
was a persistent baseline GEM range, not a fresh probe-created one. The
validated behavior is still the important one for future fault work: the same
GPU VA cleanly transitions through SGX ownership and back out of it.

## Result

The bounded range-query path can now answer:

- "Which live range owns this GPU VA?"
- "Is that range currently attached to an SGX lab object?"

That closes a practical inspection gap for future MMU or SGX fault work
without inventing a new control surface.
