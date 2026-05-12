# SGX Lab Query Tools Raw Ioctl Validation

Date: 2026-05-11

## Scope

Move the remaining typed SGX lab query helpers off libdrm retry semantics and
add a direct VAIO validation wrapper for the live range-query path.

This milestone covers:

- [workloads/psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [workloads/psb_sgx_lab_range_query.c](/root/Poulsbo/workloads/psb_sgx_lab_range_query.c)
- [scripts/run-vaio-sgx-ioctl-range-query.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-range-query.sh)

## What Changed

Userspace-side:

- `psb_sgx_lab_query` now uses raw `ioctl()` for:
  - `DRM_IOCTL_PSB_SGX_LAB_QUERY_COORD`
  - `DRM_IOCTL_PSB_SGX_LAB_QUERY_SUBMIT`
- `psb_sgx_lab_range_query` now uses raw `ioctl()` for:
  - `DRM_IOCTL_PSB_SGX_LAB_QUERY_RANGE`

That keeps the read-side lab tools aligned with the already-converted
object-op and submit-op helpers instead of inheriting libdrm's `drmIoctl()`
retry behavior.

Validation-side:

- new direct live runner:
  - [run-vaio-sgx-ioctl-range-query.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-range-query.sh)
- it stages:
  - `drm_probe`
  - `psb_sgx_lab_range_query`
- it creates live dumb GEM ranges on the VAIO, captures before/during
  `poulsbo_gtt_state`, enumerates typed GEM ranges by index, then re-queries
  the first live GEM range by explicit `range_id`

## Local Build

Rebuilt locally with:

- `./scripts/docker-build-workloads-i386.sh`

No kernel-tree files changed in this milestone. The live target kept the
existing validated override module:

- `driver_srcversion=8D7D3CAE32683F70FE739B7`
- `override_module_size=381292`

## Hardware Validation

Submit and coord typed query rerun:

- [logs/vaio-ioctl/20260511-110011-sgx-query-raw-ioctl-live](/root/Poulsbo/logs/vaio-ioctl/20260511-110011-sgx-query-raw-ioctl-live)

Direct typed range-query run:

- [logs/vaio-ioctl/20260511-110639-sgx-range-query-direct-live-clean](/root/Poulsbo/logs/vaio-ioctl/20260511-110639-sgx-range-query-direct-live-clean)

Both finished with:

- `overall=remote_usable`
- `driver_srcversion=8D7D3CAE32683F70FE739B7`
- `override_module_size=381292`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Observed Result

### Raw submit and coord query helper

In the rerun:

- `IOCTL_QUERY_BY_ID_BASE` succeeded with:
  - `coord_ioctl_ret=0 errno=0`
  - `submit_ioctl_ret=0 errno=0`
  - `submit_resolved_id=6`
  - `submit_context_name=pf`
  - `submit_cmd_type_name=ta`
  - `submit_owner_state_name=queued`
  - `submit_validate_count=1`
  - `submit_issue_count=0`
- `IOCTL_QUERY_FOCUS_BASE` resolved the same submit through focus:
  - `submit_resolved_id=6`
- after one issue step, `IOCTL_QUERY_BY_ID_AFTER_ISSUE` still succeeded and
  matched the updated state:
  - `submit_validate_count=2`
  - `submit_issue_count=1`
  - `submit_global_issue_ticket=6`
  - `coord_issue_ticket=6`
  - `coord_issue_submit_id=6`

That shows the typed submit/coord query helper still matches live sysfs state
after the raw-`ioctl()` conversion.

### Direct typed range-query helper

In the direct range-query run:

- `RANGE_QUERY_INDEX_0` through `RANGE_QUERY_INDEX_6` all succeeded with:
  - `range_ioctl_ret=0 errno=0`
  - `range_type_name=gem`
- the run reported:
  - `RANGE_QUERY_COUNT=7`
- the first four GEM ranges matched the long-lived baseline GEM set:
  - `range_resolved_id=6`
  - `range_resolved_id=7`
  - `range_resolved_id=8`
  - `range_resolved_id=9`
- the later three GEM ranges matched the three new held probe allocations:
  - `range_resolved_id=43`
  - `range_resolved_id=44`
  - `range_resolved_id=45`
- the explicit ID query matched the first indexed result:
  - `RANGE_QUERY_BY_ID`
  - `range_resolved_id=6`
  - `range_resolved_index=0`
- during the held-probe window the live typed queries reported:
  - `range_total_count=10`
  - `range_matching_count=7`

The helper therefore works both as an indexed filtered live-range enumerator
and as an explicit-ID typed range query against the same current kernel state.

## Bug Found And Fixed

The first draft of `run-vaio-sgx-ioctl-range-query.sh` used an unbound remote
`hold_seconds` variable in its wait loop. That bug did not invalidate the typed
range-query logic itself, but it polluted the first live bundle. The clean
bundle above is the post-fix run and is the one to treat as canonical.

## Conclusion

The remaining custom SGX lab query helpers no longer depend on libdrm retry
behavior:

- `psb_sgx_lab_query`
- `psb_sgx_lab_range_query`

The read-side typed query path is now validated directly on the VAIO through:

- typed submit and coordination query rerun
- direct typed range enumeration and explicit-ID range lookup

The remaining lab-interface work is no longer “should the query helpers stay on
libdrm?”. That audit is now closed for the custom SGX lab tools.
