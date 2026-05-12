# SGX Lab Multi-Submit Submit-Op Validation

Date: 2026-05-11

## Scope

Validate the existing multi-submit dependency-slot-clear scenario with both:

- `USE_IOCTL_SUBMIT_SETUP=1`
- `USE_IOCTL_DEPENDENCY_SETUP=1`

This keeps object adoption and object-role setup on the existing lab path, but
moves the actual submit creation, submit setup, submit progression, and
dependency-slot mutation work onto the bounded submit-op ioctl surface.

## Files Changed

- [workloads/psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
- [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)

## Root Cause Found During Bring-Up

The first ioctl-based multi-submit attempt stalled on the negative dependency
gate path:

- `/root/poulsbo-workloads/psb_sgx_lab_submit_op /dev/dri/card0 transition 7 running 0`

The issue was userspace, not the target kernel or the VAIO:

- `psb_sgx_lab_submit_op.c` used libdrm's `drmIoctl()`
- libdrm retries `EAGAIN`
- this lab submit ioctl intentionally uses `-EAGAIN` to report dependency-gated
  `running` transitions

That meant the tool kept resubmitting the same ioctl instead of returning the
expected error to the runner.

The fix was to use raw `ioctl()` for `DRM_IOCTL_PSB_SGX_LAB_SUBMIT_OP` so
`errno=11` reaches the runner unchanged.

I also fixed one runner bookkeeping bug in the new `USE_IOCTL_SUBMIT_SETUP=1`
path: it was not creating the saved `after_submit_a_create_submits.txt`
snapshot that the final summary block prints.

## Direct Smoke Test

After rebuilding only the 32-bit workloads and copying the updated
`psb_sgx_lab_submit_op` binary to the VAIO, the previously stuck direct command
returned immediately:

- `submit_op_name=transition`
- `submit_selector=7`
- `submit_op_ret=-1`
- `errno=11`
- exit code `1`

That proved the retry loop was in libdrm userspace, not in the kernel submit
path.

## Full Hardware Validation

Clean live run:

- [logs/vaio-ioctl/20260511-100431-sgx-multi-submit-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-100431-sgx-multi-submit-op-live)

Validated state:

- `SUBMIT_SETUP=ioctl`
- `DEPENDENCY_SETUP=ioctl`
- `DEPENDENCY_INTERFACE=indexed`

Key results from the run:

- submit IDs were allocated through the typed submit-op ioctl path
- dependency-gated `running` checks now fail cleanly with `errno=11`
- running-owner contention still fails cleanly with `errno=16`
- slot `4` was cleared live through the ioctl path:
  - `SUBMIT_B_AFTER_DEPCLEAR4`
  - `dependency4_submit_id=0`
  - `dependency4_offset=0x0`
  - `dependency4_required_ticket=0`
  - `dependency4_flags=0x0`
  - `dependency4_config_count=0`
- submit B still completed cleanly after that clear
- submit E later completed independently and did not repopulate slot `4`
- the coordination state after that post-clear submit E run was:
  - `issue_ticket_counter=15`
  - `running_ticket_counter=15`
  - `complete_ticket_counter=15`
  - `live_submit_count=5`
- final cleanup completed cleanly:
  - `OBJECTS_AFTER_DROP` reported `objects_shown=0 objects_total=0`
  - `COORD_STATE_AFTER_DROP` reported `live_submit_count=0`
  - `focused_submit_valid=0`

Saved post-run target state:

- [health-after.log](/root/Poulsbo/logs/vaio-ioctl/20260511-100431-sgx-multi-submit-op-live/health-after.log)
- [state-after.log](/root/Poulsbo/logs/vaio-ioctl/20260511-100431-sgx-multi-submit-op-live/state-after.log)
- [sgx-state-after.log](/root/Poulsbo/logs/vaio-ioctl/20260511-100431-sgx-multi-submit-op-live/sgx-state-after.log)

Those confirm:

- `overall=remote_usable`
- module `srcversion D06E0CB8A26EC70E1FAF340`
- `rollback_pending_present=0`
- `rollback_backup_present=1`
- `fault_count=0`
- `bif_fault=0x000000`

## Conclusion

The multi-submit dependency-slot-clear scenario is now validated on real
hardware with the typed submit-op ioctl path active for both:

- submit lifecycle and progression setup
- indexed dependency-slot setup and clear

That means the remaining sysfs dependence is no longer the core multi-submit
submit path. It is now mainly:

- object-adoption and object-role setup convenience
- sysfs text dumps for human-readable inspection
- any future dependency or broader state-block expansion that has not yet been
  moved onto bounded lab ioctls
