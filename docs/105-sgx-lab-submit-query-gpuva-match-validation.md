# SGX Lab Submit Query GPU-VA Match Validation

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion 0F9AE3730BF455D57D74E17`

## Goal

The previous direct GPU-VA submit-query follow-up added
`matching_submit_count`, which was enough to tell whether the resolved submit
was unique or just the first match.

One practical debugging gap still remained: with two live submits attached to
the same adopted object, the caller could see that there were two matches, but
could not explicitly ask for match `0` or match `1` while both submits were
still alive.

This follow-up extends the same bounded `QUERY_SUBMIT gpuva` path so the caller
can select a concrete GPU-VA match ordinal directly.

## Changes

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

Added to the submit-query UAPI:

- new flag:
  - `DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_GPU_VA_MATCH_INDEX`
- new request field:
  - `query_match_index`
- new result field:
  - `resolved_match_index`

Implementation changes:

- `QUERY_SUBMIT gpuva` still counts all matching submits through
  `matching_submit_count`
- when the new match-index flag is present, the resolver now selects the
  specific ordinal among those GPU-VA matches instead of always taking the
  first one
- if the requested match ordinal no longer exists, the query now fails cleanly
  with `-ENOENT`

Userspace:

- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)

It now supports:

- `gpuva-match <gpu_va> <match_index>`

It now prints:

- `submit_query_match_index`
- `submit_resolved_match_index`

Validation runners:

- [run-vaio-sgx-ioctl-submit-query-gpuva.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh)
- [run-vaio-sgx-ioctl-submit-query-gpuva-match.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-query-gpuva-match.sh)

The base runner now validates explicit match-index resolution when
`SHARED_CONTROL_SUBMITS=2`, and the new wrapper fixes that scenario as the
canonical live path.

## Validation

Local checks:

- `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
- `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva-match.sh`
- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`
- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`

Guarded deploy:

- live module:
  - `driver_srcversion=0F9AE3730BF455D57D74E17`
  - `override_module_size=387528`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`
  - `rollback_backup_size=387336`

Canonical live bundle:

- [logs/vaio-ioctl/20260511-163739-sgx-submit-query-gpuva-match-live](/root/Poulsbo/logs/vaio-ioctl/20260511-163739-sgx-submit-query-gpuva-match-live)

Saved result:

- `session_kind=sgx_submit_query_gpuva_match_validation`
- `session_outcome=completed`
- `finished_at=2026-05-11T16:39:46+03:00`

Target state from that bundle:

- `overall=remote_usable`
- `driver_srcversion=0F9AE3730BF455D57D74E17`
- `override_module_size=387528`
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
  - plain `gpuva` still resolves the first match:
    - `submit_resolved_id=1`
    - `submit_matching_submit_count=2`
    - `submit_resolved_match_index=0`
  - explicit `gpuva-match 0` resolves submit A:
    - `submit_query_match_index=0`
    - `submit_resolved_id=1`
    - `submit_matching_submit_count=2`
    - `submit_resolved_match_index=0`
  - explicit `gpuva-match 1` resolves submit B:
    - `submit_query_match_index=1`
    - `submit_resolved_id=2`
    - `submit_matching_submit_count=2`
    - `submit_resolved_match_index=1`
- after userspace holder exit:
  - `gpuva-match 1` still resolves submit B cleanly:
    - `submit_query_match_index=1`
    - `submit_resolved_id=2`
    - `submit_matching_submit_count=2`
    - `submit_resolved_match_index=1`
- after typed submit-A drop:
  - plain `gpuva` and `gpuva-match 0` both resolve submit B:
    - `submit_resolved_id=2`
    - `submit_matching_submit_count=1`
    - `submit_resolved_match_index=0`
  - `gpuva-match 1` now fails cleanly:
    - `SUBMIT_QUERY_GPUVA_MATCH1_AFTER_PRIMARY_DROP_RC=3`
    - `submit_ioctl_ret=-1 errno=2`
- after typed submit-B drop:
  - the plain `gpuva` lookup also fails cleanly:
    - `SUBMIT_QUERY_GPUVA_AFTER_DROP_RC=3`
    - `submit_ioctl_ret=-1 errno=2`
- after typed object cleanup:
  - `object_result_count=5`

## Result

The bounded direct GPU-VA submit-query path can now answer both questions:

- how many live submits match this VA
- which concrete submit is match `N`

The canonical shared-object handoff is now hardware-validated as:

- `gpuva-match 0 -> submit A`
- `gpuva-match 1 -> submit B`
- after submit-A drop: `gpuva-match 0 -> submit B`
- after submit-A drop: `gpuva-match 1 -> ENOENT`
