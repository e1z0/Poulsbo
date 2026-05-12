# SGX Lab Dump JSON Validation

Date: 2026-05-11

## Scope

Validate stable JSON output for the already-proven standalone typed SGX lab
dump tool.

This milestone covers:

- [workloads/psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)
- [scripts/run-vaio-sgx-ioctl-state-dump.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-state-dump.sh)

## What Was Added

Userspace-side:

- extended
  [psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)
- it now supports:
  - `cached`
  - `capture`
  - `json`
  - `capture-json`
- the JSON path emits one bounded typed snapshot with:
  - `coord`
  - `gtt_state`
  - `sgx_state`
  - `objects[]`
  - `submits[]`

Runner-side:

- the live validator
  [run-vaio-sgx-ioctl-state-dump.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-state-dump.sh)
  now validates both:
  - text dump output
  - JSON dump output

## Hardware Validation

The live bundle is:

- [logs/vaio-ioctl/20260511-120359-sgx-dump-json-live](/root/Poulsbo/logs/vaio-ioctl/20260511-120359-sgx-dump-json-live)

No kernel deploy was needed for this step. The VAIO stayed on:

- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`

The relevant live log is:

- [ioctl-state-dump.log](/root/Poulsbo/logs/vaio-ioctl/20260511-120359-sgx-dump-json-live/ioctl-state-dump.log)

## Observed Result

The runner seeded:

- `RANGE_IDS=7 9 30 31 32`
- `OBJECT_IDS=21 22 23 24 25`
- `SUBMIT_A_ID=9`
- `SUBMIT_B_ID=10`

The text dump still matched the staged live state, and the new JSON dump did
too.

During the staged state:

- [DUMP_DURING](/root/Poulsbo/logs/vaio-ioctl/20260511-120359-sgx-dump-json-live/ioctl-state-dump.log)
- [DUMP_JSON_DURING](/root/Poulsbo/logs/vaio-ioctl/20260511-120359-sgx-dump-json-live/ioctl-state-dump.log)

The JSON dump reported the same key facts as the text dump:

- coordination:
  - `"live_submit_count": 2`
  - `"live_object_count": 5`
  - `"focused_submit_id": 10`
  - `"issue_ticket": 5`
- GTT/MMU:
  - `"range_sgx_object_count": 5`
  - `"range_pf_bound_count": 5`
  - `"mmu_insert_pages_count": 52`
  - `"mmu_remove_pages_count": 40`
- SGX:
  - `"reason": "ioctl-sgx-state"`
  - `"fault_count": 0`
  - `"bif_fault": 0`
- typed object dump:
  - `"role_name": "control"`
  - `"gpu_use_name": "control_ccb"`
  - `"role_name": "status"`
  - `"gpu_use_name": "status_block"`
- typed submit dump:
  - submit `9`:
    - `"issue_count": 1`
    - `"queue_packet_count": 2`
    - `"sync_pending_seq": 66`
  - submit `10`:
    - `"issue_count": 0`
    - `"queue_packet_count": 1`
    - `"sync_pending_seq": 65`

After typed cleanup:

- [DUMP_AFTER_CLEANUP](/root/Poulsbo/logs/vaio-ioctl/20260511-120359-sgx-dump-json-live/ioctl-state-dump.log)
- [DUMP_JSON_AFTER_CLEANUP](/root/Poulsbo/logs/vaio-ioctl/20260511-120359-sgx-dump-json-live/ioctl-state-dump.log)

Both dump modes reported the expected cleared live state:

- `coord_live_submit_count=0`
- `coord_live_object_count=0`
- `gtt_state_range_sgx_object_count=0`
- `"live_submit_count": 0`
- `"live_object_count": 0`
- `"range_sgx_object_count": 0`

The final target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Conclusion

The standalone typed SGX lab dump tool now has two validated stable output
shapes:

- human-readable text
- structured JSON

That means the no-sysfs SGX lab inspection path is now usable both for direct
manual inspection and for later machine-checked validation without scraping
kernel sysfs text dumps.
