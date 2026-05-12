# SGX Lab Submit Queue Ioctl Validation

This note records the next bounded SGX lab submit-op ioctl extension:
queue setup is now typed and hardware-validated on the VAIO through a `queue`
submit-op verb.

## Context

[docs/65-sgx-lab-submit-ccb-op-ioctl.md](/root/Poulsbo/docs/65-sgx-lab-submit-ccb-op-ioctl.md)
closed typed CCB setup, but the next submit-side block still depended on
PCI-device sysfs text:

- `queue submit_id base_offset size packet_size packet_count`

That was the next narrowest block because `status`, `sync`, and `owner`
validation already depend on live queue state.

## Ioctl Extension

The shared lab header
[psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
now also defines:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_QUEUE`

The submit-op payload now also carries:

- `queue_base_offset`
- `queue_size`
- `queue_packet_size`
- `queue_packet_count`

The kernel implementation lives in
[psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
and reuses the existing helper:

- `poulsbo_sgx_configure_submit_queue()`

The typed userspace helper is still:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)

The live target runner is still:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)

## Validation Notes

The clean live validation bundle is:

- [logs/vaio-ioctl/20260511-090033-sgx-submit-queue-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-090033-sgx-submit-queue-op-live)

That guarded deploy brought the VAIO up on:

- override module `srcversion`: `61D418B9197433168D7124F`
- override module size: `375144`
- rollback backup size: `374484`

## What Was Validated

The runner:

1. prepared five PF-bound SGX lab objects with typed roles, classes, and GPU uses
2. created one PF-context submit through the submit-op ioctl
3. attached the five submit slots through the typed submit-op path
4. configured `kick` and `ccb` through the typed submit-op path
5. configured `queue` through the submit-op ioctl using focused-submit lookup:
   - `base_offset=0x20`
   - `size=0x40`
   - `packet_size=0x20`
   - `packet_count=1`
6. queried the typed submit state to confirm those fields landed
7. completed the existing `status -> sync -> owner -> validate -> issue ->
   transition -> drop` path

## Observed Result

The saved log is:

- [ioctl-submit-op.log](/root/Poulsbo/logs/vaio-ioctl/20260511-090033-sgx-submit-queue-op-live/ioctl-submit-op.log)

Key results:

- typed `queue` through focus succeeded:
  - `submit_op_name=queue`
  - `submit_selector=focus`
  - `submit_op_ret=0`
  - `submit_queue_base_offset=0x20`
  - `submit_queue_size=0x40`
  - `submit_queue_packet_size=0x20`
  - `submit_queue_packet_count=1`
- the typed query after `queue` showed the same live submit state:
  - `submit_queue_base_offset=0x20`
  - `submit_queue_size=0x40`
  - `submit_queue_packet_size=0x20`
  - `submit_queue_packet_count=1`
  - `submit_status_object_id=5`
- the existing validated submit progression still worked on top of the new
  typed `queue` path:
  - `submit_validate_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - final `submit_owner_state_name=complete`
  - final `submit_global_complete_ticket=1`
- post-drop typed queries still failed as expected:
  - `QUERY_FOCUS_AFTER_DROP_RC=3`
  - `QUERY_BY_ID_AFTER_DROP_RC=3`

The kernel log matched the same progression:

- `lab submit queue submit_id=1 base_offset=0x20 size=0x40 packet_size=0x20 packet_count=1 read_offset=0x0 write_offset=0x20 tail_packets=1 head_packets=0 wrapped=0`
- `lab submit validate submit_id=1 context=pf status=0 count=1`
- `lab submit issue submit_id=1 ... global_issue_ticket=1`
- `lab submit transition submit_id=1 state=running ... global_running_ticket=1`
- `lab submit transition submit_id=1 state=complete ... global_complete_ticket=1`

## Stability Result

The final saved target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=61D418B9197433168D7124F`
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
- `validate`
- `issue`
- `transition`
- `drop`

The remaining sysfs dependence inside the current SGX lab flow is narrower:

- `status`
- `sync`
- `owner`
- later dependency and broader block setup
