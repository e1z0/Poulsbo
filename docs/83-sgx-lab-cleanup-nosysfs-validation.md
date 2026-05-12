# SGX Lab Cleanup No-Sysfs Validation

Date: 2026-05-11

## Scope

Validate that the focused SGX lab cleanup runner no longer needs PCI-device
sysfs reads to prove submit cleanup, focused-submit cleanup, object cleanup,
and PF-unbind cleanup.

This milestone covers:

- [scripts/run-vaio-sgx-ioctl-cleanup.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-cleanup.sh)

## What Changed

The cleanup validator now stages and uses only typed lab helpers for its
verification path:

- [workloads/psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [workloads/psb_sgx_lab_object_query.c](/root/Poulsbo/workloads/psb_sgx_lab_object_query.c)
- [workloads/psb_sgx_lab_range_query.c](/root/Poulsbo/workloads/psb_sgx_lab_range_query.c)
- [workloads/psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)
- [workloads/psb_sgx_lab_object_op.c](/root/Poulsbo/workloads/psb_sgx_lab_object_op.c)
- [workloads/psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)

Instead of reading:

- `poulsbo_sgx_coord_state`
- `poulsbo_sgx_submits`
- `poulsbo_sgx_objects`
- `poulsbo_gtt_state`

the runner now verifies state through:

- typed coord query
- typed submit-index query
- typed object-ID query
- typed range-ID query
- standalone typed dump snapshots before and after cleanup

## Hardware Validation

The live bundle is:

- [logs/vaio-ioctl/20260511-122500-sgx-cleanup-nosysfs-live](/root/Poulsbo/logs/vaio-ioctl/20260511-122500-sgx-cleanup-nosysfs-live)

The relevant live log is:

- [ioctl-cleanup.log](/root/Poulsbo/logs/vaio-ioctl/20260511-122500-sgx-cleanup-nosysfs-live/ioctl-cleanup.log)

No kernel deploy was needed for this step. The VAIO stayed on:

- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`

## Observed Result

The runner seeded two live objects and two live submits:

- `RANGE_ID_A=7`
- `RANGE_ID_B=9`
- `OBJECT_A_ID=28`
- `OBJECT_B_ID=29`
- `SUBMIT_A_ID=13`
- `SUBMIT_B_ID=14`

Before cleanup, the typed verification path reported the expected live state:

- typed coord query:
  - `coord_focus_valid=1`
  - `coord_focused_submit_id=13`
  - `coord_live_submit_count=2`
  - `coord_live_object_count=2`
- typed submit-index queries:
  - index `0` resolved submit `13`
  - index `1` resolved submit `14`
- typed object-ID queries:
  - object `28` resolved on range `7`
  - object `29` resolved on range `9`
- typed dump snapshot:
  - `coord_live_submit_count=2`
  - `coord_live_object_count=2`
  - `gtt_state_range_sgx_object_count=2`
  - `gtt_state_range_pf_bound_count=1`

After typed submit cleanup, the verification path no longer used
`poulsbo_sgx_coord_state` or `poulsbo_sgx_submits`:

- typed coord query showed:
  - `coord_focus_valid=0`
  - `coord_focused_submit_id=0`
  - `coord_live_submit_count=0`
  - `coord_live_object_count=2`
- focused-submit query failed cleanly with:
  - `submit_ioctl_ret=-1 errno=2`
- submit-index query for index `0` failed cleanly with:
  - `submit_ioctl_ret=-1 errno=2`

After typed object cleanup, the verification path no longer used
`poulsbo_sgx_objects` or `poulsbo_gtt_state`:

- typed object-ID queries for objects `28` and `29` failed cleanly with:
  - `object_ioctl_ret=-1 errno=2`
- the standalone typed dump snapshot showed:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

The final typed range-ID checks also stayed clean:

- range `7`:
  - `range_sgx_object_id=0`
  - `range_pf_bind_count=0`
  - nonzero lifetime totals remained visible:
    - `range_pf_bind_total=7`
    - `range_pf_unbind_total=7`
- range `9`:
  - `range_sgx_object_id=0`
  - `range_pf_bind_count=0`

The nonzero totals are expected. They are lifetime counters, not
per-validation counters.

The final target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Conclusion

The focused cleanup validator no longer depends on sysfs text reads for its
verification path.

That narrows the remaining sysfs dependence again:

- legacy compatibility fallbacks in older runners
- older non-ioctl SGX lab wrappers kept for bring-up history
- any later dependency or state-block growth that still lacks a bounded typed
  read path
