# SGX Lab Submit Query Index Ioctl Validation

Date: 2026-05-11

## Scope

Extend the bounded typed submit query so live SGX lab submit inspection no
longer depends only on explicit submit IDs or focused-submit lookup.

This milestone covers:

- [third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
- [workloads/psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [scripts/run-vaio-sgx-ioctl-submit-query.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-query.sh)

## What Was Added

Kernel-side:

- extended bounded submit-query flag:
  - `DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_INDEX`
- extended typed submit-query fields:
  - `query_index`
  - `resolved_index`
  - `total_submit_count`

The ioctl now covers submit lookup by:

- explicit submit ID
- focused submit
- live submit index

Userspace-side:

- extended 32-bit helper:
  - [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- it now supports:
  - `<submit_id>`
  - `focus`
  - `index <query_index>`
- it now prints:
  - `submit_query_index`
  - `submit_resolved_index`
  - `submit_total_count`

Runner-side:

- new direct live validator:
  - [run-vaio-sgx-ioctl-submit-query.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-query.sh)
- it:
  - seeds five held GEM ranges
  - adopts five SGX lab objects
  - configures one shared typed object set
  - creates two typed submits
  - advances submit A by one issue step while leaving submit B queued
  - queries submit A by ID, submit B by ID, submit B by focus, and both
    submits by live index
  - re-queries both submits after the userspace holders exit
  - verifies clean `ENOENT` after typed submit and object cleanup

## Hardware Validation

The live bundle is:

- [logs/vaio-ioctl/20260511-113812-sgx-submit-query-live](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live)

The target state for this run was:

- kernel: `5.4.0-150-generic`
- override module path:
  `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- override module `srcversion`:
  `D43951B174F84BE870230AB`
- override module size:
  `385384`

The run used the guarded deploy path first:

- [deploy.log](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/deploy.log)

and then the new live validator:

- [ioctl-submit-query.log](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)

Final health and module state are recorded in:

- [target-health.log](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/target-health.log)
- [target-state.log](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/target-state.log)

## Observed Result

The validator created:

- `RANGE_IDS=7 9 15 16 17`
- `OBJECT_IDS=6 7 8 9 10`
- `SUBMIT_A_ID=3`
- `SUBMIT_B_ID=4`

Submit A was then advanced one issue step:

- [SUBMIT_A_QUERY_AFTER_ISSUE](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)
- key fields:
  - `submit_resolved_id=3`
  - `submit_resolved_index=0`
  - `submit_total_count=1`
  - `submit_issue_count=1`
  - `submit_queue_packet_count=2`
  - `submit_sync_pending_seq=0x42`
  - `submit_owner_issue_cookie=0x102`
  - `submit_global_issue_ticket=2`

After submit B was created and focused, the same explicit-ID query for submit A
showed the expected live-list shape:

- [SUBMIT_A_QUERY_WITH_B_PRESENT](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)
- key fields:
  - `submit_resolved_id=3`
  - `submit_resolved_index=0`
  - `submit_total_count=2`
  - `submit_issue_count=1`

Submit B stayed queued:

- [SUBMIT_B_QUERY_BASE](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)
- key fields:
  - `submit_resolved_id=4`
  - `submit_resolved_index=1`
  - `submit_total_count=2`
  - `submit_issue_count=0`
  - `submit_queue_packet_count=1`
  - `submit_sync_pending_seq=0x41`
  - `submit_owner_issue_cookie=0x101`
  - `submit_global_issue_ticket=0`

The focused query matched submit B:

- [FOCUS_QUERY_BASE](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)
- key fields:
  - `submit_resolved_id=4`
  - `submit_resolved_index=1`

The new index-based queries matched the same two submits:

- [SUBMIT_QUERY_INDEX_A](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)
  - `submit_query_index=0`
  - `submit_resolved_id=3`
  - `submit_resolved_index=0`
  - `submit_total_count=2`
  - `submit_issue_count=1`
- [SUBMIT_QUERY_INDEX_B](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)
  - `submit_query_index=1`
  - `submit_resolved_id=4`
  - `submit_resolved_index=1`
  - `submit_total_count=2`
  - `submit_issue_count=0`

After all five userspace holders exited, the state still persisted:

- [SUBMIT_QUERY_A_AFTER_PROBES](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)
- [SUBMIT_QUERY_B_AFTER_PROBES](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)
- [SUBMIT_QUERY_INDEX_A_AFTER_PROBES](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)
- [SUBMIT_QUERY_INDEX_B_AFTER_PROBES](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)

Typed cleanup then removed both submits and all five adopted objects:

- [SUBMIT_CLEANUP](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)
  - `submit_result_count=2`
- [OBJECT_CLEANUP](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)
  - `object_result_count=5`

After cleanup, all remaining submit query selectors failed cleanly:

- [SUBMIT_QUERY_A_AFTER_CLEANUP_RC=3](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)
- [SUBMIT_QUERY_B_AFTER_CLEANUP_RC=3](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)
- [SUBMIT_QUERY_INDEX_0_AFTER_CLEANUP_RC=3](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)
- [FOCUS_QUERY_AFTER_CLEANUP_RC=3](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live/ioctl-submit-query.log)

Each returned:

- `submit_ioctl_ret=-1 errno=2`

The final target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Conclusion

The bounded typed submit query is now validated for:

- explicit submit-ID lookup
- focused-submit lookup
- live submit-index lookup
- persistence after userspace close
- clean `ENOENT` after typed cleanup

The remaining submit-side sysfs dependence is narrower again:

- human-readable text dumps
- any later submit-list or broader state-block growth that still lacks a
  bounded typed query
