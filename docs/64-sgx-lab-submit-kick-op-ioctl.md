# SGX Lab Submit Kick Ioctl Validation

This note records the next bounded SGX lab submit-op ioctl extension:
submit packet setup is now typed and hardware-validated on the VAIO through a
`kick` submit-op verb.

## Context

[docs/63-sgx-lab-submit-attach-op-ioctl.md](/root/Poulsbo/docs/63-sgx-lab-submit-attach-op-ioctl.md)
closed typed submit-slot attachment and detachment, but the next SGX-shaped
submit field group still depended on PCI-device sysfs text:

- `kick submit_id cmd_type ccb_offset ctrl_flags num_ta_status num_3d_status ta_dependency`

That was the smallest remaining submit-setup block and the cleanest next step
before touching `ccb`, `queue`, `status`, `sync`, or `owner`.

## Ioctl Extension

The shared lab header
[psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
now also defines:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_KICK`

The submit-op payload now also carries:

- `cmd_type`
- `ccb_offset`
- `ctrl_flags`
- `num_ta_status_vals`
- `num_3d_status_vals`
- `ta_dependency`

The kernel implementation lives in
[psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
and reuses the existing submit helper:

- `poulsbo_sgx_configure_submit_kick()`

The typed userspace helper is still:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)

The live target runner is still:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)

## Validation Notes

The clean live validation bundle is:

- [logs/vaio-ioctl/20260511-084613-sgx-submit-kick-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-084613-sgx-submit-kick-op-live)

That guarded deploy brought the VAIO up on:

- override module `srcversion`: `C9FFE7995A1210A0D10781A`
- override module size: `373572`
- rollback backup size: `372736`

## What Was Validated

The runner:

1. prepared five PF-bound SGX lab objects with typed roles, classes, and GPU uses
2. created one PF-context submit through the submit-op ioctl
3. attached the five submit slots through the typed submit-op path
4. detached and reattached `status` once to preserve the previous milestone
5. configured `kick` through the submit-op ioctl using focused-submit lookup:
   - `cmd_type=ta`
   - `ccb_offset=0x20`
   - `ctrl_flags=0x1`
   - `num_ta_status_vals=1`
   - `num_3d_status_vals=1`
   - `ta_dependency=0`
6. queried the typed submit state to confirm those fields landed
7. completed the existing `ccb -> queue -> status -> sync -> owner ->
   validate -> issue -> transition -> drop` path

## Observed Result

The saved log is:

- [ioctl-submit-op.log](/root/Poulsbo/logs/vaio-ioctl/20260511-084613-sgx-submit-kick-op-live/ioctl-submit-op.log)

Key results:

- typed kick through focus succeeded:
  - `submit_op_name=kick`
  - `submit_selector=focus`
  - `submit_op_ret=0`
  - `submit_cmd_type_name=ta`
  - `submit_ccb_offset=0x20`
  - `submit_ctrl_flags=0x1`
  - `submit_num_ta_status_vals=1`
  - `submit_num_3d_status_vals=1`
  - `submit_ta_dependency=0`
- the typed query after kick showed the same live submit state:
  - `submit_cmd_type_name=ta`
  - `submit_ccb_offset=0x20`
  - `submit_ctrl_flags=0x1`
  - `submit_num_ta_status_vals=1`
  - `submit_num_3d_status_vals=1`
  - `submit_ta_dependency=0`
  - `submit_status_object_id=5`
- the existing validated submit progression still worked on top of the new
  typed kick path:
  - `submit_validate_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - final `submit_owner_state_name=complete`
  - final `submit_global_complete_ticket=1`
- post-drop typed queries still failed as expected:
  - `QUERY_FOCUS_AFTER_DROP_RC=3`
  - `QUERY_BY_ID_AFTER_DROP_RC=3`

The kernel log matched the same progression:

- `lab submit kick submit_id=1 cmd=ta ccb_offset=0x20 ctrl_flags=0x1 ta_status=1 3d_status=1 ta_dependency=0`
- `lab submit validate submit_id=1 context=pf status=0 count=1`
- `lab submit issue submit_id=1 ... global_issue_ticket=1`
- `lab submit transition submit_id=1 state=running ... global_running_ticket=1`
- `lab submit transition submit_id=1 state=complete ... global_complete_ticket=1`

## Stability Result

The final saved target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=C9FFE7995A1210A0D10781A`
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
- `validate`
- `issue`
- `transition`
- `drop`

The remaining sysfs dependence inside the current SGX lab flow is narrower:

- `ccb`
- `queue`
- `status`
- `sync`
- `owner`
- later dependency and broader block setup
