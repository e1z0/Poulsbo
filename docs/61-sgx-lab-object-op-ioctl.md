# SGX Lab Object-Op Ioctl Validation

This note records the first hardware validation of a bounded SGX lab object
mutation ioctl path.

## Context

[docs/60-sgx-lab-lifecycle-op-ioctl.md](/root/Poulsbo/docs/60-sgx-lab-lifecycle-op-ioctl.md)
closed the full single-submit lifecycle on the typed submit-op ioctl path.

That still left the SGX object side dependent on PCI-device sysfs text for:

- `adopt`
- `gpu_use`
- `context`
- `pf_bind`
- `pf_unbind`
- `drop`

This milestone moves those bounded object-side mutations into a typed ioctl
without claiming any broader render uAPI.

## Ioctl Surface

The shared header
[psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
now also defines:

- `DRM_IOCTL_PSB_SGX_LAB_OBJECT_OP`
- `DRM_PSB_SGX_LAB_OBJECT_OP_ADOPT`
- `DRM_PSB_SGX_LAB_OBJECT_OP_DROP`
- `DRM_PSB_SGX_LAB_OBJECT_OP_PF_BIND`
- `DRM_PSB_SGX_LAB_OBJECT_OP_PF_UNBIND`
- `DRM_PSB_SGX_LAB_OBJECT_OP_GPU_USE`
- `DRM_PSB_SGX_LAB_OBJECT_OP_CONTEXT`

The kernel implementation lives in
[psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
and reuses the existing lab helpers for:

- range adoption
- object `gpu_use` updates
- object expected-context updates
- PF bind and unbind
- object drop

The typed userspace helper is:

- [psb_sgx_lab_object_op.c](/root/Poulsbo/workloads/psb_sgx_lab_object_op.c)

The live target runner is:

- [run-vaio-sgx-ioctl-object-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-object-op.sh)

## Validation Notes

The new module was guarded-deployed to the VAIO first, producing:

- [logs/vaio-ioctl/20260511-081603-sgx-object-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-081603-sgx-object-op-live)

That deploy brought the target up on:

- override module `srcversion`: `95EEBEFCF0AA1E3328782CE`

Two local wrapper fixes happened before the final clean bundle:

- the first post-deploy run used a non-executable local script
- the next run validated the hardware path but ended with a stray trailing
  `EOF` in the local wrapper

The final clean validation bundle is:

- [logs/vaio-ioctl/20260511-081924-sgx-object-op-live-retry2](/root/Poulsbo/logs/vaio-ioctl/20260511-081924-sgx-object-op-live-retry2)

## What Was Validated

The runner:

1. held one live dumb GEM range open through `drm_probe`
2. adopted that range through the new object-op ioctl
3. changed the object `gpu_use` to `control_ccb`
4. changed the object expected context to `pf`
5. PF-bound the object
6. PF-unbound the object
7. let the userspace probe exit
8. verified the kernel-owned object still existed
9. dropped the object through the same ioctl path

## Observed Result

The saved log is:

- [ioctl-object-op.log](/root/Poulsbo/logs/vaio-ioctl/20260511-081924-sgx-object-op-live-retry2/ioctl-object-op.log)

Key results:

- adopt through the ioctl succeeded:
  - `object_op_name=adopt`
  - `object_op_ret=0`
  - `object_resolved_id=2`
  - `object_range_id=11`
- the adopted object started in the expected default state:
  - `gpu_use=generic`
  - `expected_context=auto`
  - `pf_bind_count=0`
- metadata updates through the ioctl succeeded:
  - `object_op_name=gpu_use`
  - `object_gpu_use_name=control_ccb`
  - `object_op_name=context`
  - `object_context_name=pf`
- after those metadata updates, the object view showed:
  - `gpu_use=control_ccb`
  - `expected_context=pf`
- PF bind through the ioctl succeeded:
  - `object_op_name=pf_bind`
  - `object_op_ret=0`
  - `pf_bind_count=1`
  - `pf_bind_total=1`
  - range `11` stayed linked through `sgx_object_id=2`
  - MMU counters moved from:
    - `flush=7 -> 8`
    - `insert_pages=5 -> 6`
- PF unbind through the ioctl succeeded:
  - `object_op_name=pf_unbind`
  - `object_op_ret=0`
  - `pf_bind_count=0`
  - `pf_unbind_total=1`
  - MMU counters moved from:
    - `flush=8 -> 9`
    - `remove_pages=2 -> 3`
- after the userspace probe exited, the kernel-owned object still existed with:
  - `object id=2`
  - `gpu_use=control_ccb`
  - `expected_context=pf`
  - `pf_bind_total=1`
  - `pf_unbind_total=1`
- drop through the ioctl succeeded:
  - `object_op_name=drop`
  - `object_op_ret=0`
  - `object_resolved_id=2`
  - `objects_shown=0`
  - MMU counters moved from:
    - `flush=9 -> 10`
    - `remove_pages=3 -> 4`
- the kernel log confirmed the same object lifecycle:
  - `lab object adopt object_id=2 range_id=11`
  - `lab object gpu_use object_id=2 gpu_use=control_ccb`
  - `lab object context object_id=2 context=pf`
  - `lab pf_bind id=11 ... total=1`
  - `lab pf_unbind id=11 ... total=1`
  - `lab object drop object_id=2 range_id=11`

## Stability Result

The final saved target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=95EEBEFCF0AA1E3328782CE`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Conclusion

The bounded SGX lab ioctl surface now covers:

- object-side mutation:
  - `adopt`
  - `gpu_use`
  - `context`
  - `pf_bind`
  - `pf_unbind`
  - `drop`
- submit-side mutation:
  - `create`
  - `focus`
  - `validate`
  - `issue`
  - `transition`
  - `drop`

The remaining sysfs dependence is narrower again:

- object `role` and `class`
- submit attachment
- packet, queue, sync, and block setup
