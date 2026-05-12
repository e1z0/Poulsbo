# SGX Lab Lifecycle Ioctl Validation

This note records the first hardware validation of bounded SGX lab submit
lifecycle control through the submit-op ioctl path.

## Context

[docs/59-sgx-lab-transition-op-ioctl.md](/root/Poulsbo/docs/59-sgx-lab-transition-op-ioctl.md)
closed the progression gap for an already existing submit by validating:

- `validate`
- `issue`
- `transition`

That still left one practical dependency on PCI-device sysfs text for the
submit lifecycle itself:

- `create`
- `focus`
- `drop`

This milestone moves those bounded lifecycle actions into the same typed
submit-op ioctl path, while still leaving object adoption, object attachment,
queue setup, and state-block configuration on the existing lab sysfs path.

## Ioctl Surface

The shared header
[psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
now also defines:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_CREATE`
- `DRM_PSB_SGX_LAB_SUBMIT_OP_FOCUS`
- `DRM_PSB_SGX_LAB_SUBMIT_OP_FOCUS_CLEAR`
- `DRM_PSB_SGX_LAB_SUBMIT_OP_DROP`

The kernel implementation lives in
[psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
and extends the bounded lab submit-op path to:

- create one submit in a requested context
- focus one submit by explicit ID
- clear the focused submit
- drop one submit by explicit ID or focus

The typed userspace probe is still:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)

The live target runner is still:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)

## Validation Notes

The first live attempt exposed a userspace parser bug in
[psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c):
`create` was parsed in the early argument stage but still fell through to the
generic `invalid op` path later. That first bundle is kept as a development
artifact:

- [logs/vaio-ioctl/20260511-075831-sgx-lifecycle-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-075831-sgx-lifecycle-op-live)

The final hardware validation after fixing that parser bug is:

- [logs/vaio-ioctl/20260511-080143-sgx-lifecycle-op-live-retry](/root/Poulsbo/logs/vaio-ioctl/20260511-080143-sgx-lifecycle-op-live-retry)

The target state for the successful retry was:

- kernel: `5.4.0-150-generic`
- override module path:
  `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- override module `srcversion`:
  `9F374F3350ECBF29F9A0DB0`

## What Was Validated

The runner still used the existing sysfs lab path for object adoption,
attachment, queue setup, and state-block setup, but used only typed ioctls for
submit lifecycle and progression:

1. `create pf`
2. `focus <submit_id>`
3. `validate focus`
4. `issue <submit_id> queued 1`
5. `transition focus running 1`
6. `transition <submit_id> complete 1`
7. `drop focus`

That covers:

- context-aware submit creation
- explicit-ID focus selection
- focus-based and explicit-ID progression
- submit teardown through the same typed path
- post-drop negative queries by both focus and explicit ID

## Observed Result

The saved log is:

- [ioctl-lifecycle-op.log](/root/Poulsbo/logs/vaio-ioctl/20260511-080143-sgx-lifecycle-op-live-retry/ioctl-lifecycle-op.log)

Key results:

- create through the ioctl succeeded:
  - `submit_op_name=create`
  - `submit_op_ret=0`
  - `submit_resolved_id=1`
  - `submit_context_name=pf`
- focus through explicit submit ID succeeded:
  - `submit_op_name=focus`
  - `submit_selector=1`
  - `submit_op_ret=0`
- base focused state after create and focus showed:
  - `focused_submit_id=1`
  - `coord_focus_valid=1`
  - `coord_focused_submit_id=1`
  - `coord_live_submit_count=1`
- validate through focus still succeeded:
  - `submit_op_name=validate`
  - `submit_op_ret=0`
  - `submit_validate_count=1`
- issue through explicit submit ID still succeeded:
  - `submit_op_name=issue`
  - `submit_op_ret=0`
  - `submit_issue_count=1`
  - `submit_global_issue_ticket=1`
  - `coord_issue_ticket=1`
- transition through focus into `running` succeeded:
  - `submit_op_name=transition`
  - `submit_selector=focus`
  - `submit_op_ret=0`
  - `submit_owner_state_name=running`
  - `submit_transition_count=1`
  - `submit_global_running_ticket=1`
  - `coord_running_ticket=1`
  - `coord_active_running_valid=1`
- transition through explicit submit ID into `complete` succeeded:
  - `submit_op_name=transition`
  - `submit_selector=1`
  - `submit_op_ret=0`
  - `submit_owner_state_name=complete`
  - `submit_transition_count=2`
  - `submit_global_complete_ticket=1`
  - `coord_complete_ticket=1`
  - `coord_active_running_valid=0`
- drop through focus succeeded:
  - `submit_op_name=drop`
  - `submit_selector=focus`
  - `submit_op_ret=0`
  - `submit_resolved_id=1`
- the post-drop state matched full submit teardown:
  - `focused_submit_valid=0`
  - `focused_submit_id=0`
  - `live_submit_count=0`
- post-drop typed query failure behaved as expected:
  - `QUERY_FOCUS_AFTER_DROP_RC=3`
  - `QUERY_BY_ID_AFTER_DROP_RC=3`
  - `submit_ioctl_ret=-1 errno=2`
- the kernel log confirmed the same lifecycle:
  - `lab submit create submit_id=1 context=pf`
  - `lab submit validate submit_id=1`
  - `lab submit issue submit_id=1 ... global_issue_ticket=1`
  - `lab submit transition submit_id=1 state=running ... global_running_ticket=1`
  - `lab submit transition submit_id=1 state=complete ... global_complete_ticket=1`
  - `lab submit drop submit_id=1 context=pf`

## Stability Result

The final saved target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=9F374F3350ECBF29F9A0DB0`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Conclusion

The bounded SGX lab submit-op ioctl path now covers the full validated
single-submit lifecycle:

- `create`
- `focus`
- `validate`
- `issue`
- `transition`
- `drop`

The remaining sysfs dependence is narrower still:

- object adoption
- object attachment
- queue and block setup
- broader lab-state configuration
