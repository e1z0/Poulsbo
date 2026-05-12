# SGX Lab Submit-Op Ioctl Validation

This note records the first bounded write-side SGX lab ioctl surface and its
live validation on the Sony VAIO VGN-P11Z target.

## Context

[docs/57-sgx-lab-ioctl-query.md](/root/Poulsbo/docs/57-sgx-lab-ioctl-query.md)
closed the read-side gap by adding typed `QUERY_SUBMIT` and `QUERY_COORD`
ioctls for existing SGX lab state.

The next narrower question was whether the lab path still had to depend
entirely on PCI-device sysfs text for submit mutation. Instead of moving every
operation at once, this milestone adds one bounded write-side ioctl around two
already validated internal helpers:

- `validate`
- `issue`

Setup, attachment, queue configuration, and teardown still stay on the existing
sysfs path.

## Ioctl Surface

The shared header
[psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
now also defines:

- `DRM_IOCTL_PSB_SGX_LAB_SUBMIT_OP`
- `DRM_PSB_SGX_LAB_SUBMIT_OP_VALIDATE`
- `DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE`
- `DRM_PSB_SGX_LAB_SUBMIT_OP_USE_FOCUS`

The kernel implementation lives in
[psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
and intentionally keeps the scope tight:

- resolve an existing submit by explicit ID or focus
- run `poulsbo_sgx_validate_submit()`
- run `poulsbo_sgx_issue_submit()`
- return the resolved submit ID to userspace

The typed userspace probe is:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)

The live target runner is:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)

## Hardware Validation

The live bundle is:

- [logs/vaio-ioctl/20260511-074144-sgx-submit-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-074144-sgx-submit-op-live)

The target state for this run was:

- kernel: `5.4.0-150-generic`
- override module path:
  `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- override module `srcversion`:
  `7ADE14FECA75AEE334A3D5C`

The runner:

1. copied `drm_probe`, `psb_sgx_lab_query`, and `psb_sgx_lab_submit_op` to the
   VAIO
2. created five live dumb GEM ranges
3. adopted them as SGX lab objects
4. created and configured one PF submit through the existing sysfs control path
5. queried the baseline typed state before any write-side ioctl
6. ran `validate` through the new ioctl using focused-submit lookup
7. re-queried the updated state
8. ran `issue` through the new ioctl using the explicit submit ID
9. re-queried submit and coordination state again

## Observed Result

The saved log is:

- [ioctl-submit-op.log](/root/Poulsbo/logs/vaio-ioctl/20260511-074144-sgx-submit-op-live/ioctl-submit-op.log)

Key results:

- base submit state before the write-side ioctl path showed:
  - `validate_count=0`
  - `last_validate_status=-22`
  - `issue_count=0`
  - `global_issue_ticket=0`
- the validate op through focus succeeded:
  - `submit_op_name=validate`
  - `submit_selector=focus`
  - `submit_op_ret=0`
  - `submit_resolved_id=1`
- after that validate ioctl, the live submit state moved to:
  - `validate_count=1`
  - `last_validate_status=0`
- the typed query ioctl matched that validated state:
  - `submit_validate_count=1`
  - `submit_last_validate_status=0`
- the issue op through explicit ID succeeded:
  - `submit_op_name=issue`
  - `submit_selector=1`
  - `submit_op_ret=0`
  - `submit_resolved_id=1`
  - `submit_owner_state_name=queued`
  - `submit_packet_count=1`
- after that issue ioctl, the live submit state moved to:
  - `queue_packet_count=2`
  - `sync_pending_seq=0x42`
  - `owner_issue_cookie=0x102`
  - `global_issue_ticket=1`
  - `issue_count=1`
- the typed query ioctl matched that updated state:
  - `submit_issue_count=1`
  - `submit_queue_packet_count=2`
  - `submit_sync_pending_seq=0x42`
  - `submit_owner_issue_cookie=0x102`
  - `submit_global_issue_ticket=1`
- the coordination ioctl matched the global state too:
  - `coord_issue_ticket=1`
  - `coord_issue_submit_id=1`

## Stability Result

The final saved target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=7ADE14FECA75AEE334A3D5C`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

The bundle also preserves the guarded deploy log that brought this module onto
the VAIO:

- [deploy.log](/root/Poulsbo/logs/vaio-ioctl/20260511-074144-sgx-submit-op-live/deploy.log)

## Conclusion

The SGX lab path now has both:

- bounded typed query ioctls
- bounded typed write-side ioctls for `validate` and `issue`

The remaining interface question is narrower than before:

- keep broader setup and mutation on sysfs, or
- move more already validated submit operations into similarly bounded lab
  ioctls one step at a time
