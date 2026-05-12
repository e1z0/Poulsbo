# SGX Lab Submit Owner Ioctl Validation

This note records the next bounded SGX lab submit-op ioctl extension:
owner-block setup is now typed and hardware-validated on the VAIO through an
`owner` submit-op verb.

## Context

[docs/68-sgx-lab-submit-sync-op-ioctl.md](/root/Poulsbo/docs/68-sgx-lab-submit-sync-op-ioctl.md)
closed typed sync-block setup, but the next submit-side state block still
depended on PCI-device sysfs text:

- `owner submit_id offset state issue_cookie complete_cookie`

That was the last narrow block in the current core submit setup because the
later dependency and broader state-block work is structurally separate from the
basic `kick -> ccb -> queue -> status -> sync -> owner` path.

## Ioctl Extension

The shared lab header
[psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
now also defines:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_OWNER`

The submit-op payload now also carries:

- `owner_offset`
- `owner_state`
- `owner_issue_cookie`
- `owner_complete_cookie`

The kernel implementation lives in
[psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
and reuses the existing helper:

- `poulsbo_sgx_configure_submit_owner()`

The typed userspace helper is still:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)

The live target runner is still:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)

## Validation Notes

The clean live validation bundle is:

- [logs/vaio-ioctl/20260511-092344-sgx-submit-owner-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-092344-sgx-submit-owner-op-live)

That guarded deploy brought the VAIO up on:

- override module `srcversion`: `E8B0AF3BC985C1317C25433`
- override module size: `377912`
- rollback backup size: `376916`

## What Was Validated

The runner:

1. prepared five PF-bound SGX lab objects with typed roles, classes, and GPU uses
2. created one PF-context submit through the submit-op ioctl
3. attached the five submit slots through the typed submit-op path
4. configured `kick`, `ccb`, `queue`, `status`, and `sync` through the typed submit-op path
5. configured `owner` through the submit-op ioctl using focused-submit lookup:
   - `offset=0x40`
   - `state=queued`
   - `issue_cookie=0x101`
   - `complete_cookie=0x100`
6. queried the typed submit state to confirm those fields landed
7. completed the existing `validate -> issue -> transition -> drop` path

## Observed Result

The saved log is:

- [ioctl-submit-op.log](/root/Poulsbo/logs/vaio-ioctl/20260511-092344-sgx-submit-owner-op-live/ioctl-submit-op.log)

Key results:

- typed `owner` through focus succeeded:
  - `submit_op_name=owner`
  - `submit_selector=focus`
  - `submit_op_ret=0`
  - `submit_owner_offset=0x40`
  - `submit_owner_state_name=queued`
  - `submit_owner_issue_cookie=0x101`
  - `submit_owner_complete_cookie=0x100`
- the typed query after `owner` showed the same live submit state:
  - `submit_owner_offset=0x40`
  - `submit_owner_state_name=queued`
  - `submit_owner_issue_cookie=0x101`
  - `submit_owner_complete_cookie=0x100`
- the existing validated submit progression still worked on top of the new
  typed `owner` path:
  - `submit_validate_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - final `submit_owner_state_name=complete`
  - final `submit_global_complete_ticket=1`
- post-drop typed queries still failed as expected:
  - `QUERY_FOCUS_AFTER_DROP_RC=3`
  - `QUERY_BY_ID_AFTER_DROP_RC=3`

The kernel log matched the same progression:

- `lab submit owner submit_id=1 offset=0x40 state=queued issue_cookie=0x101 complete_cookie=0x100 outstanding=0x1 epoch=0 global_complete_ticket=0 global_running_ticket=0`
- `lab submit validate submit_id=1 context=pf status=0 count=1`
- `lab submit issue submit_id=1 ... issue_cookie=0x102`
- `lab submit transition submit_id=1 state=running ... complete_cookie=0x101`
- `lab submit transition submit_id=1 state=complete ... complete_cookie=0x102 global_complete_ticket=1`

## Stability Result

The final saved target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=E8B0AF3BC985C1317C25433`
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
- `owner`
- `validate`
- `issue`
- `transition`
- `drop`

That means the current core SGX lab submit setup no longer depends on PCI
device sysfs for the basic single-submit path. The remaining sysfs dependence
is now in later dependency and broader state-block setup only.
