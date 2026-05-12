# SGX Lab Submit Dependency Slot Ioctl Validation

This note records the next bounded SGX lab submit-op ioctl extension:
indexed dependency-slot setup and slot clear are now typed and
hardware-validated on the VAIO through the submit-op ioctl surface.

## Context

[docs/69-sgx-lab-submit-owner-op-ioctl.md](/root/Poulsbo/docs/69-sgx-lab-submit-owner-op-ioctl.md)
closed the core single-submit setup path. The next remaining submit-side lab
state still depended on PCI-device sysfs text in the multi-submit runners:

- `depend_slot submit_id slot dependency_submit_id`
- `depticket_slot submit_id slot required_ticket`
- `depstate_slot submit_id slot offset`
- `depclear_slot submit_id slot`

Those were the last write-side dependency-slot controls outside the bounded
submit-op ioctl surface.

## Ioctl Extension

The shared lab header
[psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
now also defines:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_DEPEND_SLOT`
- `DRM_PSB_SGX_LAB_SUBMIT_OP_DEPTICKET_SLOT`
- `DRM_PSB_SGX_LAB_SUBMIT_OP_DEPSTATE_SLOT`
- `DRM_PSB_SGX_LAB_SUBMIT_OP_DEPCLEAR_SLOT`

The submit-op payload now also carries:

- `dependency_slot`
- `dependency_submit_id`
- `dependency_ticket`
- `dependency_offset`

The kernel implementation lives in
[psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
and reuses the existing helpers:

- `poulsbo_sgx_configure_submit_dependency_slot_user()`
- `poulsbo_sgx_configure_submit_dependency_ticket_slot()`
- `poulsbo_sgx_configure_submit_dependency_state_slot_user()`
- `poulsbo_sgx_clear_submit_dependency_slot_user()`

The typed userspace helper is still:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)

The live target validation reused the existing multi-submit runner:

- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
- [run-vaio-sgx-dependency-slot-clear.sh](/root/Poulsbo/scripts/run-vaio-sgx-dependency-slot-clear.sh)

The runner now supports:

- `USE_IOCTL_DEPENDENCY_SETUP=1`

That stages `psb_sgx_lab_submit_op` onto the target and switches dependency
setup from sysfs writes to typed submit-op calls while keeping the same
multi-submit scenario logic.

## Validation Notes

The clean live validation bundle is:

- [logs/vaio-ioctl/20260511-093732-sgx-dependency-slot-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-093732-sgx-dependency-slot-op-live)

That guarded deploy brought the VAIO up on:

- override module `srcversion`: `D06E0CB8A26EC70E1FAF340`
- override module size: `380244`
- rollback backup size: `377912`

The saved bundle shows:

- `DEPENDENCY_SETUP=ioctl`
- `DEPENDENCY_INTERFACE=indexed`

## What Was Validated

The runner:

1. guarded-deployed the new module and confirmed the VAIO returned to `overall=remote_usable`
2. prepared five PF-context submits and their PF-bound SGX lab objects
3. configured submit B dependency slots `1..4` through typed submit-op calls
4. configured slot-specific required tickets through typed submit-op calls
5. configured slot-specific dependency state-block offsets through typed submit-op calls
6. validated that submit B stayed dependency-gated until the expected source submits completed
7. cleared dependency slot `4` through the typed submit-op path
8. validated that submit B completed without submit E and that submit E later completed without repopulating slot `4`

## Observed Result

The saved log is:

- [run.log](/root/Poulsbo/logs/vaio-ioctl/20260511-093732-sgx-dependency-slot-op-live/run.log)

Key results:

- the typed dependency-slot path was active for the whole run:
  - `DEPENDENCY_SETUP=ioctl`
  - `DEPENDENCY_INTERFACE=indexed`
- slot `4` setup landed through the typed path before clear:
  - `SUBMIT_B_AFTER_DEPTICKET4_SETUP`
  - `SUBMIT_B_AFTER_DEPSTATE4_SETUP`
  - live submit B state showed:
    - `dependency4_submit_id=5`
    - `dependency4_offset=0x100`
    - `dependency4_required_ticket=4`
    - `dependency4_flags=0x0b`
- the typed clear path removed slot `4` live:
  - `SUBMIT_B_AFTER_DEPCLEAR4`
  - live submit B state then showed:
    - `dependency4_submit_id=0`
    - `dependency4_offset=0x0`
    - `dependency4_required_ticket=0`
    - `dependency4_flags=0x0`
    - `dependency4_config_count=0`
- submit B still completed cleanly after the clear:
  - final `submit id=2` state showed:
    - `validate_count=22`
    - `issue_count=1`
    - `transition_count=2`
    - `owner_state=complete`
    - `global_issue_ticket=4`
    - `global_running_ticket=4`
    - `global_complete_ticket=4`
- submit E later completed independently and did not repopulate slot `4`:
  - `COORD_STATE_AFTER_SUBMIT_E_POST_CLEAR`
  - final coordination counters advanced to:
    - `issue_ticket_counter=5`
    - `running_ticket_counter=5`
    - `complete_ticket_counter=5`
  - submit B still showed:
    - `dependency4_submit_id=0`
    - `dependency4_config_count=0`

The post-run target state remained clean:

- [health-after.log](/root/Poulsbo/logs/vaio-ioctl/20260511-093732-sgx-dependency-slot-op-live/health-after.log)
  - `overall=remote_usable`
- [state-after.log](/root/Poulsbo/logs/vaio-ioctl/20260511-093732-sgx-dependency-slot-op-live/state-after.log)
  - `driver_srcversion=D06E0CB8A26EC70E1FAF340`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`
- [sgx-state-after.log](/root/Poulsbo/logs/vaio-ioctl/20260511-093732-sgx-dependency-slot-op-live/sgx-state-after.log)
  - `fault_count=0`
  - `bif_fault=0x000000`

## Conclusion

The bounded SGX lab submit-op ioctl surface now also covers the validated
dependency-slot setup path:

- `depend-slot`
- `depticket-slot`
- `depstate-slot`
- `depclear-slot`

That means the remaining sysfs dependence is no longer missing dependency-slot
mutation coverage. The remaining gap is now higher-level multi-submit
orchestration convenience and any later dependency or state-block work that we
have not yet moved onto the typed lab ioctl surface.
