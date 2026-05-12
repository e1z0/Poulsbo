# SGX Lab Range Query GPU-VA Submit-Ref Validation

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion AEB8DB73A3D6849EE3A59FA`

## Goal

The earlier GPU-VA range-query milestones could answer:

- which live `gtt_range` contains this GPU VA
- whether that range currently belongs to an SGX lab object
- what semantic role/class/use/context that object currently carries

The next missing question for future SGX fault and submission triage was:

- is that adopted object currently attached to a live submit
- if so, which submit sees it first
- in which submit slot

This follow-up extends the same bounded GPU-VA query path so one lookup can
answer that directly without a second submit query.

## Changes

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

Added to `drm_psb_sgx_lab_range_query`:

- `sgx_submit_ref_count`
- `sgx_first_submit_id`
- `sgx_first_submit_slot`

Implementation changes:

- `QUERY_RANGE` now scans the live SGX lab submit list when
  `range_sgx_object_id` is non-zero
- the typed result now reports:
  - how many live submits currently reference that adopted object
  - the first matching submit ID
  - the first matching submit slot
- the internal lock order for the range-query path is now:
  - `sgx_lab_submit_mutex`
  - `sgx_lab_object_mutex`
  - `gtt_mutex`

That keeps range-query submit scanning aligned with the existing submit-query
lock ordering instead of inventing a parallel path.

Userspace:

- [psb_sgx_lab_range_query.c](/root/Poulsbo/workloads/psb_sgx_lab_range_query.c)

It now prints:

- `range_sgx_submit_ref_count`
- `range_sgx_first_submit_id`
- `range_sgx_first_submit_slot`
- `range_sgx_first_submit_slot_name`

Validation runner:

- [run-vaio-sgx-ioctl-range-query.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-range-query.sh)

It now:

- stages the typed submit-op helper as well as the range and object helpers
- runs typed submit and object cleanup before probing
- adopts one live GEM range and sets:
  - `role=control`
  - `class=packet`
  - `gpu_use=control_ccb`
  - `context=pf`
- creates one typed submit
- attaches that adopted object in the `control` slot
- verifies through the same GPU-VA lookup that:
  - the object still resolves correctly
  - `submit_ref_count=1`
  - the first submit is the new live submit
  - the first slot is `control`
- verifies that the same submit-reference state survives userspace holder exit
- drops the submit and proves the same GPU VA still reports object metadata but
  no longer reports any submit reference
- drops the object and proves the same GPU VA returns to
  `generic/generic/generic/auto` with zero submit references

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `bash -n scripts/run-vaio-sgx-ioctl-range-query.sh`
- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy:

- new live module:
  - `driver_srcversion=AEB8DB73A3D6849EE3A59FA`
  - `override_module_size=386328`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`
  - `rollback_backup_size=385984`

Canonical live bundle:

- [logs/vaio-ioctl/20260511-152849-sgx-range-query-gpuva-submit-ref-live](/root/Poulsbo/logs/vaio-ioctl/20260511-152849-sgx-range-query-gpuva-submit-ref-live)

Saved result:

- `session_kind=sgx_range_query_gpuva_submit_ref_validation`
- `session_outcome=completed`
- `finished_at=2026-05-11T15:30:02+03:00`

Target state from that bundle:

- `driver_srcversion=AEB8DB73A3D6849EE3A59FA`
- `override_module_size=386328`
- `overall=remote_usable`

Key live evidence:

- baseline GEM-range set before holder probes:
  - `BASELINE_RANGE_IDS=7 8 9 13 14 15`
- the warm target again had no fresh probe-created GEM range, so the runner
  intentionally fell back to:
  - `FRESH_RANGE_ID=7`
  - `FRESH_RANGE_IS_NEW=0`
- canonical live VA:
  - `range_query_gpu_va=0x807c0000`
  - `range_query_gpu_va_offset=0x00001000`
  - `range_resolved_id=7`
- before adopt:
  - `range_sgx_object_id=0`
  - `range_sgx_submit_ref_count=0`
  - `range_sgx_first_submit_id=0`
  - `range_sgx_first_submit_slot_name=none`
- after typed object adopt plus metadata setup:
  - `object_resolved_id=2`
  - `range_sgx_object_id=2`
  - `range_sgx_object_role_name=control`
  - `range_sgx_object_class_name=packet`
  - `range_sgx_object_gpu_use_name=control_ccb`
  - `range_sgx_object_expected_context_name=pf`
  - `range_sgx_submit_ref_count=0`
  - `range_sgx_first_submit_id=0`
  - `range_sgx_first_submit_slot_name=none`
- after typed submit create plus `control` attach:
  - `submit_resolved_id=2`
  - `range_sgx_object_id=2`
  - `range_sgx_submit_ref_count=1`
  - `range_sgx_first_submit_id=2`
  - `range_sgx_first_submit_slot_name=control`
- after userspace holder exit:
  - `range_sgx_object_id=2`
  - `range_sgx_submit_ref_count=1`
  - `range_sgx_first_submit_id=2`
  - `range_sgx_first_submit_slot_name=control`
- after typed submit drop:
  - `range_sgx_object_id=2`
  - `range_sgx_object_role_name=control`
  - `range_sgx_object_class_name=packet`
  - `range_sgx_object_gpu_use_name=control_ccb`
  - `range_sgx_object_expected_context_name=pf`
  - `range_sgx_submit_ref_count=0`
  - `range_sgx_first_submit_id=0`
  - `range_sgx_first_submit_slot_name=none`
- after typed object drop:
  - `RANGE_QUERY_BY_GPUVA_AFTER_DROP_RC=0`
  - `range_sgx_object_id=0`
  - `range_sgx_object_role_name=generic`
  - `range_sgx_object_class_name=generic`
  - `range_sgx_object_gpu_use_name=generic`
  - `range_sgx_object_expected_context_name=auto`
  - `range_sgx_submit_ref_count=0`
  - `range_sgx_first_submit_id=0`
  - `range_sgx_first_submit_slot_name=none`

As with the earlier GPU-VA bundles, this canonical run stayed on a persistent
baseline GEM range rather than a fresh probe-created one. So the final
post-drop GPU-VA query still resolved the underlying range, but both the SGX
object semantics and the SGX submit-reference state were correctly gone.

## Result

One typed GPU-VA lookup can now answer the full live ownership chain:

- which range contains this GPU VA
- whether that range belongs to an SGX lab object
- what semantic role/class/use/context that object carries
- whether that adopted object is attached to a live submit
- which submit sees it first
- in which slot

That gives future SGX-side or MMU-side fault work a direct path from one GPU VA
to both object semantics and current submit ownership without falling back to
text dumps or a second query family.
