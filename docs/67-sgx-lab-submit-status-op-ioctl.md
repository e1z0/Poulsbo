# SGX Lab Submit Status Ioctl Validation

This note records the next bounded SGX lab submit-op ioctl extension:
status-block setup is now typed and hardware-validated on the VAIO through a
`status` submit-op verb.

## Context

[docs/66-sgx-lab-submit-queue-op-ioctl.md](/root/Poulsbo/docs/66-sgx-lab-submit-queue-op-ioctl.md)
closed typed queue setup, but the next submit-side state block still depended
on PCI-device sysfs text:

- `status submit_id ta_offset 3d_offset stride seed`

That was the next narrow block because the existing `sync` and `owner`
validation already depend on live status-object contents.

## Ioctl Extension

The shared lab header
[psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
now also defines:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_STATUS`

The submit-op payload now also carries:

- `status_ta_offset`
- `status_3d_offset`
- `status_stride`
- `status_value_seed`

The typed submit query now also exposes those live fields for readback.

The kernel implementation lives in
[psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
and reuses the existing helper:

- `poulsbo_sgx_configure_submit_status()`

The typed userspace helpers are:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)

The live target runner is still:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)

## Validation Notes

The clean live validation bundle is:

- [logs/vaio-ioctl/20260511-091155-sgx-submit-status-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-091155-sgx-submit-status-op-live)

That guarded deploy brought the VAIO up on:

- override module `srcversion`: `1A0C93B58FDC70B06D841C6`
- override module size: `375860`
- rollback backup size: `375144`

## What Was Validated

The runner:

1. prepared five PF-bound SGX lab objects with typed roles, classes, and GPU uses
2. created one PF-context submit through the submit-op ioctl
3. attached the five submit slots through the typed submit-op path
4. configured `kick`, `ccb`, and `queue` through the typed submit-op path
5. configured `status` through the submit-op ioctl using focused-submit lookup:
   - `ta_offset=0x80`
   - `3d_offset=0xc0`
   - `stride=0x4`
   - `seed=0x5a500000`
6. queried the typed submit state to confirm those fields landed
7. completed the existing `sync -> owner -> validate -> issue -> transition ->
   drop` path

## Observed Result

The saved log is:

- [ioctl-submit-op.log](/root/Poulsbo/logs/vaio-ioctl/20260511-091155-sgx-submit-status-op-live/ioctl-submit-op.log)

Key results:

- typed `status` through focus succeeded:
  - `submit_op_name=status`
  - `submit_selector=focus`
  - `submit_op_ret=0`
  - `submit_status_ta_offset=0x80`
  - `submit_status_3d_offset=0xc0`
  - `submit_status_stride=0x4`
  - `submit_status_value_seed=0x5a500000`
- the typed query after `status` showed the same live submit state:
  - `submit_status_ta_offset=0x80`
  - `submit_status_3d_offset=0xc0`
  - `submit_status_stride=0x4`
  - `submit_status_value_seed=0x5a500000`
  - `submit_status_object_id=5`
- the existing validated submit progression still worked on top of the new
  typed `status` path:
  - `submit_validate_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - final `submit_owner_state_name=complete`
  - final `submit_global_complete_ticket=1`
- post-drop typed queries still failed as expected:
  - `QUERY_FOCUS_AFTER_DROP_RC=3`
  - `QUERY_BY_ID_AFTER_DROP_RC=3`

The kernel log matched the same progression:

- `lab submit status submit_id=1 ta_offset=0x80 3d_offset=0xc0 stride=0x4 seed=0x5a500000 ta_count=1 3d_count=1`
- `lab submit validate submit_id=1 context=pf status=0 count=1`
- `lab submit issue submit_id=1 ... global_issue_ticket=1`
- `lab submit transition submit_id=1 state=running ... global_running_ticket=1`
- `lab submit transition submit_id=1 state=complete ... global_complete_ticket=1`

## Stability Result

The final saved target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=1A0C93B58FDC70B06D841C6`
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
- `validate`
- `issue`
- `transition`
- `drop`

The remaining sysfs dependence inside the current SGX lab flow is narrower:

- `sync`
- `owner`
- later dependency and broader block setup
