# SGX Lab Transition Ioctl Validation

This note records the first hardware validation of `transition` through the
bounded SGX lab submit-op ioctl path.

## Context

[docs/58-sgx-lab-submit-op-ioctl.md](/root/Poulsbo/docs/58-sgx-lab-submit-op-ioctl.md)
validated the first write-side SGX lab ioctl surface for:

- `validate`
- `issue`

The next bounded extension was to cover the already validated internal
transition helper too, without moving setup, object attachment, or broader
submit mutation out of the existing sysfs path.

The updated userspace path still uses:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)

but the write-side ioctl now also accepts:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION`

## Hardware Validation

The live bundle is:

- [logs/vaio-ioctl/20260511-074937-sgx-transition-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-074937-sgx-transition-op-live)

The target state for this run was:

- kernel: `5.4.0-150-generic`
- override module path:
  `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- override module `srcversion`:
  `19A66F470763F1CC9968B27`

## What Was Validated

The runner kept setup and teardown on the existing sysfs path, then used the
typed write-side ioctl path for the progression itself:

1. `validate focus`
2. `issue <submit_id> queued 1`
3. `transition focus running 1`
4. `transition <submit_id> complete 1`

That covers both selector modes:

- focused-submit lookup
- explicit submit ID

and both transition states:

- `running`
- `complete`

## Observed Result

The saved log is:

- [ioctl-transition-op.log](/root/Poulsbo/logs/vaio-ioctl/20260511-074937-sgx-transition-op-live/ioctl-transition-op.log)

Key results:

- the validate and issue parts still succeeded first:
  - `submit_op_name=validate`
  - `submit_op_ret=0`
  - `submit_op_name=issue`
  - `submit_op_ret=0`
- the running transition through focused-submit lookup succeeded:
  - `submit_op_name=transition`
  - `submit_selector=focus`
  - `submit_op_ret=0`
  - `submit_owner_state_name=running`
- after that running transition, typed query matched the expected state:
  - `submit_transition_count=1`
  - `submit_owner_state_name=running`
  - `submit_ccb_offset=0x40`
  - `submit_ccb_read_offset=0x20`
  - `submit_queue_packet_count=1`
  - `submit_sync_complete_seq=0x41`
  - `submit_owner_complete_cookie=0x101`
  - `submit_global_running_ticket=1`
  - `coord_running_ticket=1`
  - `coord_running_submit_id=1`
  - `coord_active_running_valid=1`
- the complete transition through explicit submit ID succeeded:
  - `submit_op_name=transition`
  - `submit_selector=1`
  - `submit_op_ret=0`
  - `submit_owner_state_name=complete`
- after that complete transition, typed query matched the expected final state:
  - `submit_transition_count=2`
  - `submit_owner_state_name=complete`
  - `submit_queue_packet_count=0`
  - `submit_sync_complete_seq=0x42`
  - `submit_owner_complete_cookie=0x102`
  - `submit_global_complete_ticket=1`
  - `coord_running_ticket=1`
  - `coord_complete_ticket=1`
  - `coord_complete_submit_id=1`
  - `coord_active_running_valid=0`
- the kernel log confirms the same progression:
  - `lab submit transition submit_id=1 state=running ... global_running_ticket=1`
  - `lab submit transition submit_id=1 state=complete ... global_complete_ticket=1`

## Stability Result

The final saved target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=19A66F470763F1CC9968B27`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Conclusion

The bounded write-side SGX lab ioctl surface now covers the full validated
single-submit progression:

- `validate`
- `issue`
- `transition`

The remaining interface question is now narrower still:

- which other already validated submit mutations, if any, should move out of
  sysfs into similarly bounded lab ioctls
