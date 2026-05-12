# SGX Lab Submit Attach/Detach Ioctl Validation

This note records the next bounded SGX lab submit-op ioctl extension:
submit-slot object attachment is now typed and hardware-validated on the VAIO.

## Context

[docs/60-sgx-lab-lifecycle-op-ioctl.md](/root/Poulsbo/docs/60-sgx-lab-lifecycle-op-ioctl.md)
closed the full validated single-submit lifecycle on the typed submit-op path,
and [docs/62-sgx-lab-object-role-class-ioctl.md](/root/Poulsbo/docs/62-sgx-lab-object-role-class-ioctl.md)
closed the remaining object-side metadata gap.

That still left submit-slot wiring on PCI-device sysfs text:

- `attach submit_id slot object_id`
- `detach submit_id slot`

This milestone moves those bounded slot mutations into the typed submit-op
ioctl without claiming anything broader about the eventual render uAPI.

## Ioctl Extension

The shared lab header
[psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
now also defines:

- submit slots:
  - `CONTROL`
  - `PDS`
  - `USE`
  - `TARGET`
  - `STATUS`
- new submit-op verbs:
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_ATTACH`
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_DETACH`

The kernel implementation lives in
[psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
and reuses the existing submit/object helpers for:

- typed slot-name resolution
- slot-to-role validation
- submit attachment
- submit detachment

The typed userspace helper is still:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)

The live target runner is still:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)

## Validation Notes

The clean live validation bundle is:

- [logs/vaio-ioctl/20260511-083705-sgx-submit-attach-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-083705-sgx-submit-attach-op-live)

That guarded deploy brought the VAIO up on:

- override module `srcversion`: `AADEFBD92B8B3E07E7907A0`
- override module size: `372736`
- rollback backup size: `371980`

## What Was Validated

The runner:

1. adopted five live GEM ranges and configured them as:
   - `control/packet/control_ccb`
   - `pds/code/pds_code`
   - `use/code/use_code`
   - `target/surface/render_target`
   - `status/state/status_block`
2. PF-bound those five objects
3. created one PF-context submit through the submit-op ioctl
4. focused that submit through the same ioctl
5. attached:
   - `control`
   - `pds`
   - `use`
   - `target`
   by explicit submit ID
6. attached `status` through focused-submit lookup
7. queried the typed submit state to confirm all five object IDs resolved
8. detached `status` through focused-submit lookup
9. queried the typed submit state to confirm `status_object_id=0`
10. reattached `status` by explicit submit ID
11. finished the existing validated `kick -> ccb -> queue -> status -> sync -> owner -> validate -> issue -> transition -> drop` path

## Observed Result

The saved log is:

- [ioctl-submit-op.log](/root/Poulsbo/logs/vaio-ioctl/20260511-083705-sgx-submit-attach-op-live/ioctl-submit-op.log)

Key results:

- create through the submit-op ioctl still succeeded:
  - `submit_resolved_id=1`
  - `submit_context_name=pf`
- typed slot attachment succeeded:
  - `attach 1 control 1`
  - `attach 1 pds 2`
  - `attach 1 use 3`
  - `attach 1 target 4`
  - `attach focus status 5`
- the typed query after attachment showed:
  - `submit_control_object_id=1`
  - `submit_pds_object_id=2`
  - `submit_use_object_id=3`
  - `submit_target_object_id=4`
  - `submit_status_object_id=5`
  - resolved GPU VAs for all five slots
- typed detach through focus succeeded:
  - `submit_op_name=detach`
  - `submit_slot_name=status`
  - `submit_resolved_id=1`
- the typed query after detach showed:
  - `submit_status_object_id=0`
  - the other four slot IDs remained intact
- typed reattach succeeded:
  - `submit_op_name=attach`
  - `submit_slot_name=status`
  - `submit_object_id=5`
- the typed query after reattach showed:
  - `submit_status_object_id=5`
- the existing validated submit progression still worked on top of the new
  typed attachment path:
  - `validate_count=1`
  - `issue_count=1`
  - `transition_count=2`
  - `global_issue_ticket=1`
  - `global_running_ticket=1`
  - `global_complete_ticket=1`
  - final `owner_state=complete`
- post-drop typed queries still failed as expected:
  - `QUERY_FOCUS_AFTER_DROP_RC=3`
  - `QUERY_BY_ID_AFTER_DROP_RC=3`

The kernel log matched the same attachment lifecycle:

- `lab submit attach submit_id=1 slot=control object_id=1`
- `lab submit attach submit_id=1 slot=pds object_id=2`
- `lab submit attach submit_id=1 slot=use object_id=3`
- `lab submit attach submit_id=1 slot=target object_id=4`
- `lab submit attach submit_id=1 slot=status object_id=5`
- `lab submit detach submit_id=1 slot=status`
- `lab submit attach submit_id=1 slot=status object_id=5`

## Stability Result

The final saved target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=AADEFBD92B8B3E07E7907A0`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Conclusion

The bounded SGX lab submit-op ioctl surface now covers the validated
submit-side mutation path:

- `create`
- `focus`
- `attach`
- `detach`
- `validate`
- `issue`
- `transition`
- `drop`

The remaining sysfs dependence inside the current SGX lab flow is now narrower:

- `kick`
- `ccb`
- `queue`
- `status`
- `sync`
- `owner`
- later dependency and broader block setup
