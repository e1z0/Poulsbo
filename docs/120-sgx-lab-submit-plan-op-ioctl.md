# SGX Lab Submit `plan` Op Ioctl

## Summary

This batch adds the first control-side `plan` block on top of the existing
execution-shaped SGX lab submit model. The `plan` block is memory-backed,
typed, and configured through the submit-op ioctl path. It snapshots one
coherent execution plan selected from:

- `live`
- `launch`
- `dispatch`
- `retired`

The block now records:

- stage and stage count
- selected source
- context, command type, and owner state
- issued and completed packet counts
- dependency-active count and refill marker
- queue packet count and queue epoch
- queue layout and CCB offsets
- sync and owner offsets plus sync or owner values
- global issue, running, and complete tickets
- linked `report`, `manifest`, and `descriptor` offsets
- CCB service or cache or data words
- `control`, `pds`, `use`, `target`, and `status` GPU-VA32 words

The typed query and dump paths now expose the full `submit_plan_*` state, and
the focused plus multi-submit runners now configure and assert the `plan`
block through progression.

## Code

Kernel-side:

- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`

Userspace and runner-side:

- `workloads/psb_sgx_lab_submit_op.c`
- `workloads/psb_sgx_lab_query.c`
- `workloads/psb_sgx_lab_dump.c`
- `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `scripts/run-vaio-sgx-multi-submit.sh`

## Local Verification

The full local batch is build-clean:

- `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `bash -n scripts/run-vaio-sgx-multi-submit.sh`
- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`
- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`

The kernel module build completed with the existing large-frame warning in
`poulsbo_sgx_validate_submit_locked()`, but no new compile failure.

## Target Deployment

On May 11, 2026, the guarded deploy completed successfully and the VAIO came
back remotely usable with:

- `driver_srcversion=3D21987EBD495B4E78FBBA7`
- `override_module_size=433996`
- `rollback_pending_present=0`

## Hardware Validation Blocker

The first focused hardware validation attempt did not complete.

After the new override was loaded, `./scripts/run-vaio-sgx-ioctl-submit-op.sh`
was started against `10.1.11.218`. The target then dropped out of service and
never returned during the bounded health watch.

Observed on May 11, 2026:

- `2026-05-11T23:37:34+03:00`: `overall=ssh_port_down`
- `2026-05-11T23:37:54+03:00`: `overall=ssh_port_down`
- `2026-05-11T23:38:13+03:00`: `overall=ssh_port_down`
- `2026-05-11T23:38:33+03:00`: `overall=ssh_port_down`
- `2026-05-11T23:38:52+03:00`: `overall=ssh_port_down`
- `2026-05-11T23:39:11+03:00`: `overall=ssh_port_down`

At that point the local focused runner was killed because it was only hanging on
the dead target.

So this batch is in the following state:

- implemented
- locally verified
- deployed once
- not yet hardware-validated end to end

## Next Step

Recover the VAIO from the console, then rerun:

```bash
./scripts/run-vaio-sgx-ioctl-submit-op.sh
./scripts/run-vaio-sgx-multi-submit-typed.sh
```

If the machine is still on the failing override and does not come back by
itself, remove the override and reboot from the console:

```bash
rm -f /lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko
depmod -a 5.4.0-150-generic
reboot
```
