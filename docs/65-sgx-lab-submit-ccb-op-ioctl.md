# SGX Lab Submit CCB Ioctl Validation

This note records the next bounded SGX lab submit-op ioctl extension:
CCB setup is now typed and hardware-validated on the VAIO through a `ccb`
submit-op verb.

## Context

[docs/64-sgx-lab-submit-kick-op-ioctl.md](/root/Poulsbo/docs/64-sgx-lab-submit-kick-op-ioctl.md)
closed typed packet setup, but the next submit-side block still depended on
PCI-device sysfs text:

- `ccb submit_id write_offset read_offset service cache data0 data1`

That was the next narrowest block because `queue` validation already depends on
live CCB state.

## Ioctl Extension

The shared lab header
[psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
now also defines:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_CCB`

The submit-op payload now also carries:

- `ccb_write_offset`
- `ccb_read_offset`
- `ccb_service_address`
- `ccb_cache_control`
- `ccb_data0`
- `ccb_data1`

The kernel implementation lives in
[psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
and reuses the existing helper:

- `poulsbo_sgx_configure_submit_ccb()`

The typed userspace helper is still:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)

The live target runner is still:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)

## Validation Notes

The clean live validation bundle is:

- [logs/vaio-ioctl/20260511-085320-sgx-submit-ccb-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-085320-sgx-submit-ccb-op-live)

That guarded deploy brought the VAIO up on:

- override module `srcversion`: `27FE36C613637398E2288FE`
- override module size: `374484`
- rollback backup size: `373572`

## What Was Validated

The runner:

1. prepared five PF-bound SGX lab objects with typed roles, classes, and GPU uses
2. created one PF-context submit through the submit-op ioctl
3. attached the five submit slots through the typed submit-op path
4. configured `kick` through the typed submit-op path
5. configured `ccb` through the submit-op ioctl using focused-submit lookup:
   - `write_offset=0x20`
   - `read_offset=0x0`
   - `service=0x1000`
   - `cache=0x0`
   - `data0=0xc0de0001`
   - `data1=0xc0de0002`
6. queried the typed submit state to confirm those fields landed
7. completed the existing `queue -> status -> sync -> owner -> validate ->
   issue -> transition -> drop` path

## Observed Result

The saved log is:

- [ioctl-submit-op.log](/root/Poulsbo/logs/vaio-ioctl/20260511-085320-sgx-submit-ccb-op-live/ioctl-submit-op.log)

Key results:

- typed `ccb` through focus succeeded:
  - `submit_op_name=ccb`
  - `submit_selector=focus`
  - `submit_op_ret=0`
  - `submit_ccb_write_offset=0x20`
  - `submit_ccb_read_offset=0x0`
  - `submit_ccb_service_address=0x1000`
  - `submit_ccb_cache_control=0x0`
  - `submit_ccb_data0=0xc0de0001`
  - `submit_ccb_data1=0xc0de0002`
- the typed query after `ccb` showed the same live submit state:
  - `submit_ccb_write_offset=0x20`
  - `submit_ccb_read_offset=0x0`
  - `submit_ccb_service_address=0x1000`
  - `submit_ccb_cache_control=0x0`
  - `submit_ccb_data0=0xc0de0001`
  - `submit_ccb_data1=0xc0de0002`
  - `submit_status_object_id=5`
- the existing validated submit progression still worked on top of the new
  typed `ccb` path:
  - `submit_validate_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - final `submit_owner_state_name=complete`
  - final `submit_global_complete_ticket=1`
- post-drop typed queries still failed as expected:
  - `QUERY_FOCUS_AFTER_DROP_RC=3`
  - `QUERY_BY_ID_AFTER_DROP_RC=3`

The kernel log matched the same progression:

- `lab submit ccb submit_id=1 write_offset=0x20 read_offset=0x0 service=0x1000 cache=0x0 data0=0xc0de0001 data1=0xc0de0002`
- `lab submit validate submit_id=1 context=pf status=0 count=1`
- `lab submit issue submit_id=1 ... global_issue_ticket=1`
- `lab submit transition submit_id=1 state=running ... global_running_ticket=1`
- `lab submit transition submit_id=1 state=complete ... global_complete_ticket=1`

## Stability Result

The final saved target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=27FE36C613637398E2288FE`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Conclusion

The bounded SGX lab submit-op ioctl surface now covers the validated
submit-side mutation path:

- `create`
- `focus`
- `attach`
- `detach`
- `kick`
- `ccb`
- `validate`
- `issue`
- `transition`
- `drop`

The remaining sysfs dependence inside the current SGX lab flow is narrower:

- `queue`
- `status`
- `sync`
- `owner`
- later dependency and broader block setup
