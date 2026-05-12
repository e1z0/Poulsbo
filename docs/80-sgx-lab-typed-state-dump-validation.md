# SGX Lab Typed State Dump Validation

Date: 2026-05-11

## Scope

Validate one end-to-end SGX lab inspection flow that does not depend on the
old PCI-device sysfs dump nodes for live state readout.

This milestone covers:

- [workloads/psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [scripts/run-vaio-sgx-ioctl-state-dump.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-state-dump.sh)

It reuses the already validated typed helpers:

- [workloads/psb_sgx_lab_object_query.c](/root/Poulsbo/workloads/psb_sgx_lab_object_query.c)
- [workloads/psb_sgx_lab_sgx_state_query.c](/root/Poulsbo/workloads/psb_sgx_lab_sgx_state_query.c)
- [workloads/psb_sgx_lab_gtt_state_query.c](/root/Poulsbo/workloads/psb_sgx_lab_gtt_state_query.c)
- [workloads/psb_sgx_lab_object_op.c](/root/Poulsbo/workloads/psb_sgx_lab_object_op.c)
- [workloads/psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)

## What Was Added

Userspace-side:

- extended [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
  with:
  - `coord`
    - runs only the coordination ioctl
    - skips the submit query cleanly
    - returns a typed coord-only dump without needing a focused or explicit
      submit selector

Runner-side:

- new direct live validator:
  - [run-vaio-sgx-ioctl-state-dump.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-state-dump.sh)
- it:
  - seeds five held GEM ranges
  - adopts five SGX lab objects
  - configures one shared typed object set
  - creates and configures two typed submits
  - advances submit A by one issue step while leaving submit B queued
  - dumps coord, object, submit, SGX, and GTT state through typed ioctls only
  - verifies persistence after the userspace holders exit
  - verifies typed cleanup returns the live counts to zero

## Hardware Validation

The live bundle is:

- [logs/vaio-ioctl/20260511-114905-sgx-state-dump-live](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live)

The run did not require a new kernel deploy because the SGX lab ABI was
unchanged. It reused the current validated VAIO module:

- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`

The typed dump log is:

- [ioctl-state-dump.log](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)

The final target state is recorded in:

- [target-health.log](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/target-health.log)
- [target-state.log](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/target-state.log)

## Observed Result

The validator created:

- `RANGE_IDS=7 9 20 21 22`
- `OBJECT_IDS=11 12 13 14 15`
- `SUBMIT_A_ID=5`
- `SUBMIT_B_ID=6`

The typed coord-only dump showed the expected live coordination state:

- [COORD_DURING](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)
- key fields:
  - `coord_focus_valid=1`
  - `coord_focused_submit_id=6`
  - `coord_live_submit_count=2`
  - `coord_live_object_count=5`
  - `coord_issue_ticket=3`
  - `coord_issue_submit_id=5`
  - `submit_selector=coord`
  - `submit_query_skipped=1`

The typed global dumps matched the staged state:

- [GTT_DURING](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)
  - `gtt_state_range_sgx_object_count=5`
  - `gtt_state_range_pf_bound_count=5`
- [SGX_DURING](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)
  - `sgx_state_ioctl_ret=0 errno=0`
  - `sgx_state_reason=ioctl-sgx-state`
  - `sgx_state_fault_count=0`
  - `sgx_state_bif_fault=0x0`

The typed focus dump resolved the second submit:

- [FOCUS_DURING](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)
  - `submit_resolved_id=6`
  - `submit_resolved_index=1`
  - `submit_issue_count=0`

The typed object index dumps matched the configured roles and classes:

- [OBJECT_INDEX_0_DURING](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)
  - `object_resolved_id=11`
  - `object_role_name=control`
  - `object_class_name=packet`
  - `object_gpu_use_name=control_ccb`
- [OBJECT_INDEX_4_DURING](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)
  - `object_resolved_id=15`
  - `object_role_name=status`
  - `object_class_name=state`
  - `object_gpu_use_name=status_block`

The typed submit index dumps matched the staged progression:

- [SUBMIT_INDEX_0_DURING](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)
  - `submit_resolved_id=5`
  - `submit_resolved_index=0`
  - `submit_issue_count=1`
  - `submit_queue_packet_count=2`
  - `submit_sync_pending_seq=0x42`
  - `submit_owner_issue_cookie=0x102`
- [SUBMIT_INDEX_1_DURING](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)
  - `submit_resolved_id=6`
  - `submit_resolved_index=1`
  - `submit_issue_count=0`
  - `submit_queue_packet_count=1`
  - `submit_sync_pending_seq=0x41`
  - `submit_owner_issue_cookie=0x101`

After all five userspace holders exited, the typed dump still resolved live
state:

- [COORD_AFTER_PROBES](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)
  - `coord_live_submit_count=2`
  - `coord_live_object_count=5`
- [OBJECT_INDEX_0_AFTER_PROBES](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)
  - `object_resolved_id=11`
- [SUBMIT_INDEX_0_AFTER_PROBES](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)
  - `submit_resolved_id=5`

Typed cleanup then returned the live counts to zero:

- [SUBMIT_CLEANUP](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)
  - `submit_result_count=2`
- [OBJECT_CLEANUP](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)
  - `object_result_count=5`
- [COORD_AFTER_CLEANUP](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
- [GTT_AFTER_CLEANUP](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

The final target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Conclusion

One end-to-end live SGX lab state dump is now validated without sysfs reads.

The typed flow now covers:

- live coordination state
- live object enumeration by index
- live submit enumeration by index
- live SGX snapshot readout
- live global GTT/MMU state
- persistence after userspace close
- typed cleanup back to zero live objects and submits

The remaining sysfs dependence is narrower again:

- legacy compatibility fallback paths
- any later SGX state-block growth that still lacks a bounded typed query
