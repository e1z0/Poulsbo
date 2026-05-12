# SGX Lab Submit Sync Ioctl Validation

This note records the next bounded SGX lab submit-op ioctl extension:
sync-block setup is now typed and hardware-validated on the VAIO through a
`sync` submit-op verb.

## Context

[docs/67-sgx-lab-submit-status-op-ioctl.md](/root/Poulsbo/docs/67-sgx-lab-submit-status-op-ioctl.md)
closed typed status-block setup, but the next submit-side state block still
depended on PCI-device sysfs text:

- `sync submit_id offset complete_seq pending_seq flags`

That was the next narrow block because the existing `owner` validation already
depends on live sync-block contents.

## Ioctl Extension

The shared lab header
[psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
now also defines:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_SYNC`

The submit-op payload now also carries:

- `sync_offset`
- `sync_complete_seq`
- `sync_pending_seq`
- `sync_flags`

The kernel implementation lives in
[psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
and reuses the existing helper:

- `poulsbo_sgx_configure_submit_sync()`

The typed userspace helper is still:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)

The live target runner is still:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)

## Validation Notes

The clean live validation bundle is:

- [logs/vaio-ioctl/20260511-091910-sgx-submit-sync-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-091910-sgx-submit-sync-op-live)

That guarded deploy brought the VAIO up on:

- override module `srcversion`: `15BB28412789D8525D2C02A`
- override module size: `376916`
- rollback backup size: `375860`

## What Was Validated

The runner:

1. prepared five PF-bound SGX lab objects with typed roles, classes, and GPU uses
2. created one PF-context submit through the submit-op ioctl
3. attached the five submit slots through the typed submit-op path
4. configured `kick`, `ccb`, `queue`, and `status` through the typed submit-op path
5. configured `sync` through the submit-op ioctl using focused-submit lookup:
   - `offset=0x20`
   - `complete_seq=0x40`
   - `pending_seq=0x41`
   - `flags=0x1`
6. queried the typed submit state to confirm those fields landed
7. completed the existing `owner -> validate -> issue -> transition -> drop` path

## Observed Result

The saved log is:

- [ioctl-submit-op.log](/root/Poulsbo/logs/vaio-ioctl/20260511-091910-sgx-submit-sync-op-live/ioctl-submit-op.log)

Key results:

- typed `sync` through focus succeeded:
  - `submit_op_name=sync`
  - `submit_selector=focus`
  - `submit_op_ret=0`
  - `submit_sync_offset=0x20`
  - `submit_sync_complete_seq=0x40`
  - `submit_sync_pending_seq=0x41`
  - `submit_sync_flags=0x1`
- the typed query after `sync` showed the same live submit state:
  - `submit_sync_offset=0x20`
  - `submit_sync_complete_seq=0x40`
  - `submit_sync_pending_seq=0x41`
  - `submit_sync_flags=0x1`
- the existing validated submit progression still worked on top of the new
  typed `sync` path:
  - `submit_validate_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - final `submit_owner_state_name=complete`
  - final `submit_global_complete_ticket=1`
- post-drop typed queries still failed as expected:
  - `QUERY_FOCUS_AFTER_DROP_RC=3`
  - `QUERY_BY_ID_AFTER_DROP_RC=3`

The kernel log matched the same progression:

- `lab submit sync submit_id=1 offset=0x20 read_offset=0x0 write_offset=0x20 complete_seq=0x40 pending_seq=0x41 flags=0x1 wrapped=0 epoch=0 global_issue_ticket=0`
- `lab submit validate submit_id=1 context=pf status=0 count=1`
- `lab submit issue submit_id=1 ... global_issue_ticket=1`
- `lab submit transition submit_id=1 state=running ... global_running_ticket=1`
- `lab submit transition submit_id=1 state=complete ... global_complete_ticket=1`

## Stability Result

The final saved target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=15BB28412789D8525D2C02A`
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
- `queue`
- `status`
- `sync`
- `validate`
- `issue`
- `transition`
- `drop`

The remaining sysfs dependence inside the current SGX lab flow is narrower:

- `owner`
- later dependency and broader block setup
