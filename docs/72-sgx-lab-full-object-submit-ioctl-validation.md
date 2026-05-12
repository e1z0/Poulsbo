# SGX Lab Full Object And Submit Ioctl Validation

Date: 2026-05-11

## Scope

Validate the existing four-lane dependency-slot-clear scenario with all three
mutation surfaces on bounded ioctls:

- `USE_IOCTL_OBJECT_SETUP=1`
- `USE_IOCTL_SUBMIT_SETUP=1`
- `USE_IOCTL_DEPENDENCY_SETUP=1`

This moves the core object lifecycle/config path, the submit lifecycle/setup
path, and the indexed dependency-slot mutation path off direct sysfs writes for
the real multi-submit VAIO scenario.

## Files Changed

- [workloads/psb_sgx_lab_object_op.c](/root/Poulsbo/workloads/psb_sgx_lab_object_op.c)
- [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)

## Runner Issues Found And Fixed

Two real issues showed up while moving the object path into the same scenario:

1. `psb_sgx_lab_object_op.c` still used libdrm's `drmIoctl()`
   - switched it to raw `ioctl()` to keep custom lab-object behavior aligned
     with the already-fixed submit-op tool

2. `run-vaio-sgx-multi-submit.sh` treated `object_resolved_id=0` as success
   during fallback range scanning
   - some non-adoptable candidate ranges returned a zero resolved object ID
   - the runner was accepting those zeros into the object map
   - that eventually poisoned later object-role and bind setup
   - the fix was to reject zero resolved IDs in `object_adopt()` and in the
     mapped-object validation pass

I also added `REMOTE_SHELL_TRACE=1` support to the runner so future target-side
runner failures can be debugged with remote `set -x` without patching the file
again.

## Full Hardware Validation

Clean live run:

- [logs/vaio-ioctl/20260511-101953-sgx-full-object-submit-ioctl-live](/root/Poulsbo/logs/vaio-ioctl/20260511-101953-sgx-full-object-submit-ioctl-live)

Validated run mode:

- `SUBMIT_SETUP=ioctl`
- `OBJECT_SETUP=ioctl`
- `DEPENDENCY_SETUP=ioctl`
- `DEPENDENCY_INTERFACE=indexed`

Key results:

- the full object set for the multi-submit scenario was adopted and configured
  through the object-op ioctl path
- the same scenario still cleared dependency slot `4` live on submit B:
  - `SUBMIT_B_AFTER_DEPCLEAR4`
  - `dependency4_submit_id=0`
  - `dependency4_offset=0x0`
  - `dependency4_required_ticket=0`
  - `dependency4_flags=0x0`
  - `dependency4_config_count=0`
- submit B still completed cleanly after that clear
- submit E later completed independently and did not repopulate slot `4`
- coordination after the post-clear submit E run reached:
  - `issue_ticket_counter=20`
  - `running_ticket_counter=20`
  - `complete_ticket_counter=20`
  - `live_submit_count=5`
- final cleanup completed cleanly:
  - `OBJECTS_AFTER_DROP` reported `objects_shown=0 objects_total=0`
  - `COORD_STATE_AFTER_DROP` reported `live_submit_count=0`
  - `focused_submit_valid=0`

Saved post-run state:

- [health-after.log](/root/Poulsbo/logs/vaio-ioctl/20260511-101953-sgx-full-object-submit-ioctl-live/health-after.log)
- [state-after.log](/root/Poulsbo/logs/vaio-ioctl/20260511-101953-sgx-full-object-submit-ioctl-live/state-after.log)
- [sgx-state-after.log](/root/Poulsbo/logs/vaio-ioctl/20260511-101953-sgx-full-object-submit-ioctl-live/sgx-state-after.log)

Those confirm:

- `overall=remote_usable`
- module `srcversion D06E0CB8A26EC70E1FAF340`
- `rollback_pending_present=0`
- `rollback_backup_present=1`
- `fault_count=0`
- `bif_fault=0x000000`

## Conclusion

The full multi-submit dependency-slot-clear scenario is now validated on real
hardware with:

- object setup on the object-op ioctl path
- submit setup/progression on the submit-op ioctl path
- indexed dependency-slot setup/clear on the submit-op ioctl path

That means the remaining sysfs dependence in this lab flow is no longer the
core object or submit mutation path. It is now mainly:

- human-readable text dumps
- stale cleanup convenience
- range discovery convenience
- any later lab-only state growth that has not yet been moved onto bounded
  ioctl surfaces
