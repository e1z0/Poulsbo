# SGX Lab Dump Tool Validation

Date: 2026-05-11

## Scope

Validate a standalone userspace dump tool for the already-proven typed
no-sysfs SGX lab inspection path.

This milestone covers:

- [workloads/psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)
- [scripts/run-vaio-sgx-ioctl-state-dump.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-state-dump.sh)

## What Was Added

Userspace-side:

- new 32-bit helper:
  - [psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)
- it:
  - opens `/dev/dri/card0` directly
  - supports:
    - `cached`
    - `capture`
  - runs only the bounded query ioctls
  - dumps:
    - typed coord state
    - typed GTT/MMU state
    - typed SGX state
    - typed object state by index
    - typed submit state by index

Runner-side:

- the live no-sysfs validator
  [run-vaio-sgx-ioctl-state-dump.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-state-dump.sh)
  now stages and validates the dump tool in addition to the lower-level point
  queries

## Hardware Validation

The live bundle is:

- [logs/vaio-ioctl/20260511-114905-sgx-state-dump-live](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live)

This stayed on the current validated VAIO module:

- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`

The relevant live log is:

- [ioctl-state-dump.log](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)

## Observed Result

The runner seeded:

- `RANGE_IDS=7 9 25 26 27`
- `OBJECT_IDS=16 17 18 19 20`
- `SUBMIT_A_ID=7`
- `SUBMIT_B_ID=8`

The standalone dump tool ran in capture mode during the staged state:

- [DUMP_DURING](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)

Key fields matched the expected live state:

- coordination:
  - `coord_live_submit_count=2`
  - `coord_live_object_count=5`
  - `coord_focus_valid=1`
  - `coord_focused_submit_id=8`
  - `coord_issue_ticket=4`
  - `coord_issue_submit_id=7`
- GTT/MMU:
  - `gtt_state_range_total_count=12`
  - `gtt_state_range_gem_count=9`
  - `gtt_state_range_sgx_object_count=5`
  - `gtt_state_range_pf_bound_count=5`
- SGX:
  - `sgx_state_reason=ioctl-sgx-state`
  - `sgx_state_fault_count=0`
  - `sgx_state_bif_fault=0x0`
- typed object dump:
  - `object[0]_role_name=control`
  - `object[4]_role_name=status`
  - `object[4]_gpu_use_name=status_block`
- typed submit dump:
  - `submit[0]_id=7`
  - `submit[0]_issue_count=1`
  - `submit[0]_queue_packet_count=2`
  - `submit[0]_sync_pending_seq=0x42`
  - `submit[1]_id=8`
  - `submit[1]_issue_count=0`
  - `submit[1]_queue_packet_count=1`
  - `submit[1]_sync_pending_seq=0x41`

After typed cleanup, the same dump tool ran again in cached mode:

- [DUMP_AFTER_CLEANUP](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live/ioctl-state-dump.log)

It reported the expected cleared live state:

- `coord_live_submit_count=0`
- `coord_live_object_count=0`
- `gtt_state_range_sgx_object_count=0`
- `gtt_state_range_pf_bound_count=0`
- `sgx_state_fault_count=0`

The final target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Conclusion

The typed no-sysfs SGX lab dump path now has a standalone userspace tool.

That means one human-readable end-to-end SGX lab snapshot no longer depends on
PCI-device sysfs reads or on shell-side composition alone.
