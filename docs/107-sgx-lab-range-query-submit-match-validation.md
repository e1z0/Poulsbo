# SGX Lab Range Query Submit-Match Validation

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion 933E555C1A18400A2CF86AE`

## Goal

The earlier GPU-VA range-query milestones could answer:

- which live `gtt_range` contains this GPU VA
- whether that range belongs to an SGX lab object
- what semantic role/class/use/context that object carries
- how many live submits reference that adopted object
- which submit sees it first

The next missing question for shared-object debugging was:

- which specific submit owns match `0`, `1`, or later for this same GPU VA

This follow-up extends the same bounded `QUERY_RANGE` path so range-side GPU-VA
inspection can select one live submit-reference ordinal directly, instead of
forcing callers to switch over to the submit-query family.

## Changes

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

Added to `drm_psb_sgx_lab_range_query`:

- `DRM_PSB_SGX_LAB_QUERY_RANGE_USE_SUBMIT_MATCH_INDEX`
- `query_submit_match_index`
- `sgx_resolved_submit_found`
- `sgx_resolved_submit_id`
- `sgx_resolved_submit_slot`
- `sgx_resolved_submit_match_index`

Implementation changes:

- `QUERY_RANGE gpuva` can now optionally select one submit-reference ordinal for
  the adopted object that owns the requested GPU VA
- the typed result still reports:
  - total submit-reference count
  - first matching submit ID and slot
- and now also reports:
  - whether a requested ordinal resolved
  - which submit ID matched that ordinal
  - which slot matched that ordinal
  - which ordinal actually resolved

This batch also fixes two real kernel bugs in the range-query path:

- submit-list scanning is now done while `sgx_lab_submit_mutex` is actually
  held
- the ioctl exit path no longer tries to unlock `sgx_lab_submit_mutex` when it
  was never locked

The range-query lock order is now consistently:

- `sgx_lab_submit_mutex`
- `sgx_lab_object_mutex`
- `gtt_mutex`

Userspace:

- [psb_sgx_lab_range_query.c](/root/Poulsbo/workloads/psb_sgx_lab_range_query.c)

It now supports:

- `gpuva-submit <gpu_va> <submit_match_index> [gem]`

and prints:

- `range_query_submit_match_index`
- `range_sgx_resolved_submit_found`
- `range_sgx_resolved_submit_id`
- `range_sgx_resolved_submit_slot`
- `range_sgx_resolved_submit_slot_name`
- `range_sgx_resolved_submit_match_index`

Validation runners:

- [run-vaio-sgx-ioctl-range-query.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-range-query.sh)
- [run-vaio-sgx-ioctl-range-query-submit-match.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-range-query-submit-match.sh)

The base runner now supports:

- `SHARED_CONTROL_SUBMITS=2`

In that mode it:

- adopts one live GEM range as one SGX lab object
- configures it as `control / packet / control_ccb / pf`
- creates two submits
- attaches the same object in the `control` slot of both submits
- verifies plain `gpuva` range-query behavior while both submits exist
- verifies:
  - `gpuva-submit <va> 0`
  - `gpuva-submit <va> 1`
- verifies the same behavior after userspace holder exit
- drops the primary submit and verifies:
  - plain `gpuva` now reports one remaining submit-reference
  - `gpuva-submit <va> 0` resolves the remaining submit
  - `gpuva-submit <va> 1` no longer resolves a selected submit
- drops the final submit and object and verifies the path returns to zero

The wrapper fixes that mode on as the canonical shared-object live validation.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `bash -n scripts/run-vaio-sgx-ioctl-range-query.sh`
- `bash -n scripts/run-vaio-sgx-ioctl-range-query-submit-match.sh`
- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy:

- new live module:
  - `driver_srcversion=933E555C1A18400A2CF86AE`
  - `override_module_size=387664`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`
  - `rollback_backup_size=387528`

Canonical live bundle:

- [logs/vaio-ioctl/20260511-165702-sgx-range-query-submit-match-live](/root/Poulsbo/logs/vaio-ioctl/20260511-165702-sgx-range-query-submit-match-live)

Saved result:

- `session_kind=sgx_range_query_submit_match_validation`
- `session_outcome=completed`
- `finished_at=2026-05-11T16:59:01+03:00`

Target state from that bundle:

- `driver_srcversion=933E555C1A18400A2CF86AE`
- `override_module_size=387664`
- `overall=remote_usable`

Key live evidence:

- warm-target baseline GEM-range set before holder probes:
  - `BASELINE_RANGE_IDS=7 8 9 10 11 12`
- the canonical live run again reused a persistent baseline GEM range:
  - `FRESH_RANGE_ID=7`
- canonical live VA:
  - `range_query_gpu_va=0x807c0000`
  - `range_query_gpu_va_offset=0x00001000`
  - `range_resolved_id=7`

While submit `1` and submit `2` both attach the same control object:

- plain GPU-VA range query:
  - `range_sgx_submit_ref_count=2`
  - `range_sgx_first_submit_id=1`
  - `range_sgx_first_submit_slot_name=control`
  - `range_sgx_resolved_submit_found=0`
- `gpuva-submit 0`:
  - `range_sgx_submit_ref_count=2`
  - `range_sgx_resolved_submit_found=1`
  - `range_sgx_resolved_submit_id=1`
  - `range_sgx_resolved_submit_slot_name=control`
  - `range_sgx_resolved_submit_match_index=0`
- `gpuva-submit 1`:
  - `range_sgx_submit_ref_count=2`
  - `range_sgx_resolved_submit_found=1`
  - `range_sgx_resolved_submit_id=2`
  - `range_sgx_resolved_submit_slot_name=control`
  - `range_sgx_resolved_submit_match_index=1`

After userspace holder exit:

- plain GPU-VA range query still showed:
  - `range_sgx_submit_ref_count=2`
- `gpuva-submit 1` still resolved:
  - `range_sgx_resolved_submit_id=2`
  - `range_sgx_resolved_submit_match_index=1`

After the primary submit drops:

- plain GPU-VA range query moved to:
  - `range_sgx_submit_ref_count=1`
  - `range_sgx_first_submit_id=2`
  - `range_sgx_first_submit_slot_name=control`
- `gpuva-submit 0` now resolved:
  - `range_sgx_resolved_submit_id=2`
  - `range_sgx_resolved_submit_match_index=0`
- `gpuva-submit 1` no longer resolved a selected submit:
  - `range_sgx_submit_ref_count=1`
  - `range_sgx_resolved_submit_found=0`
  - `range_sgx_resolved_submit_id=0`

After the final submit drop:

- `range_sgx_submit_ref_count=0`
- `range_sgx_first_submit_id=0`
- `range_sgx_first_submit_slot_name=none`

After the final object drop:

- `RANGE_QUERY_BY_GPUVA_AFTER_DROP_RC=0`
- `range_sgx_object_id=0`
- `range_sgx_submit_ref_count=0`
- `range_sgx_resolved_submit_found=0`

As with the earlier GPU-VA bundles, this canonical run stayed on a persistent
baseline GEM range rather than a fresh probe-created one. So the final
post-drop GPU-VA query still resolved the underlying baseline range, but the
SGX object and SGX submit-reference state were both correctly gone.

## Result

The bounded range-query path can now answer the full shared-object question
directly from one GPU VA:

- which range owns this VA
- which SGX object semantics it currently carries
- how many submits reference that object
- which submit matches ordinal `0`, `1`, or later

That gives the repo a range-side counterpart to the earlier direct
submit-query `gpuva-match` path and closes another practical inspection gap
before any real SGX execution work begins.
