# SGX Submit Cleanup Deadlock Fix

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Last known good override before this batch: `gma500_gfx` `srcversion 801B0633670F92DA0659014`
Current candidate module built locally: `gma500_gfx` `srcversion BB080942FE121033007E2EC`

## Status Update

This document records the original deadlock analysis and first failed guarded
deploy attempt.

The follow-up is now complete: the same candidate override
`BB080942FE121033007E2EC` was later redeployed successfully and validated on
hardware through both the focused typed submit path and the safe typed/default
multi-submit wrapper path. The current close-out is:

- [docs/115-sgx-deadlock-fix-candidate-validation.md](/root/Poulsbo/docs/115-sgx-deadlock-fix-candidate-validation.md)

## What Changed

This batch fixed the concrete kernel deadlock behind the stuck typed submit
cleanup path and hardened the main mutation runners against concurrent use.

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
  no longer takes `sgx_lab_submit_mutex` twice in
  `poulsbo_sgx_submits_show()`
- the submit-cleanup log string now says `during cleanup` instead of
  `during unload`

Runner-side:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
- [run-vaio-sgx-ioctl-object-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-object-op.sh)
- [run-vaio-sgx-ioctl-cleanup.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-cleanup.sh)
- [run-vaio-sgx-lab-reset.sh](/root/Poulsbo/scripts/run-vaio-sgx-lab-reset.sh)

These now take a shared remote lock with `flock(1)` at
`/run/poulsbo-sgx-lab.lock` by default, so one VAIO-side SGX lab mutation
session cannot overlap another by accident.

## Root Cause

The stuck cleanup was not caused by the new retired-state query fields.

Remote inspection on the VAIO showed three D-state tasks:

- `cat /sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`
- two `psb_sgx_lab_submit_op /dev/dri/card0 cleanup` processes

The submit-dump reader was stuck in `poulsbo_sgx_submits_show`, and both typed
cleanup tasks were blocked in `poulsbo_sgx_drop_all_submits_count`.

That matches the actual bug in the driver: `poulsbo_sgx_submits_show()` took
`sgx_lab_submit_mutex` twice and only unlocked it once, so one sysfs read could
deadlock itself while permanently holding the submit mutex. Every later submit
op that needed the same mutex then blocked behind it.

## Verification

Local verification passed:

- `bash -n` on the touched runners
- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- local VAIO-target module rebuild through
  [docker-build-vaio-gma500.sh](/root/Poulsbo/scripts/docker-build-vaio-gma500.sh)

## Deployment Outcome

I deployed the candidate module with the guarded path:

- [deploy-vaio-gma500-guarded.sh](/root/Poulsbo/scripts/deploy-vaio-gma500-guarded.sh)

The preflight target state was healthy:

- `overall=remote_usable`
- loaded override `srcversion 801B0633670F92DA0659014`

The new module copied successfully and reboot was issued, but the VAIO did not
return to remote usability within the guarded timeout. Final observed state:

- `overall=ssh_port_down`
- later degraded to `ping_status=fail`
- rollback did not recover the machine remotely, which implies the system did
  not get far enough for the target-side rollback service to restore the
  previous override automatically

So the deadlock fix is in the tree and built, but hardware revalidation of the
submit path is currently blocked by target recovery.

## Follow-Up On The Recovered Previous Override

The target later came back on the restored previous override:

- `driver_srcversion=801B0633670F92DA0659014`
- `override_module_size=408352`
- `rollback_pending_present=0`

With that recovered state, the focused typed submit path was re-run
sequentially and saved successfully at:

- [logs/vaio-ioctl/20260511-201657-sgx-submit-op-retired-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-201657-sgx-submit-op-retired-live/summary.log)

That bundle proves the retired execution snapshot on real hardware with the
recovered previous override:

- `SUBMIT_ID=2`
- `submit_prepare_count=1`
- `submit_arm_count=1`
- `submit_emit_count=1`
- `submit_launch_count=1`
- `submit_dispatch_count=1`
- `submit_retired_count=1`
- `submit_retired_owner_state_name=complete`
- `submit_retired_completed_packets=2`
- `submit_retired_global_complete_ticket=2`
- final zero cleanup:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

One direct rerun mistake also re-exposed why the old override still needs the
kernel deadlock fix: launching the raw legacy
`run-vaio-sgx-multi-submit.sh` entrypoint on that restored module triggered
another D-state `cat .../poulsbo_sgx_submits` reader, because the legacy path
still uses the old sysfs submit dump. A plain reboot was then issued to clear
that stale state. The next multi-submit rerun on recovery should use the typed
wrapper path, not the direct legacy entrypoint.

After that reboot, the typed/default multi-submit wrapper was rerun cleanly and
saved at:

- [logs/vaio-ioctl/20260511-203149-sgx-multi-submit-default-retired-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-203149-sgx-multi-submit-default-retired-live/summary.log)

That run completed on the restored previous override with the safe wrapper
stack:

- `STALE_SUBMIT_CLEANUP=ioctl`
- `STALE_OBJECT_CLEANUP=ioctl`
- typed/default multi-submit path, not the raw legacy entrypoint
- target remained healthy afterwards:
  - `overall=remote_usable`
  - `driver_srcversion=801B0633670F92DA0659014`
  - `rollback_pending_present=0`

So the currently closed gap is runner-side: the repo now has one saved focused
typed rerun and one saved typed/default multi-submit rerun on the recovered old
override. The still-open gap is candidate-module validation of the kernel
deadlock fix itself.

## Next Steps

Recover the VAIO on the console, then rerun the focused and multi-submit
validators sequentially.

Console recovery:

```bash
rm -f /lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko
depmod -a 5.4.0-150-generic
reboot
```

Once SSH is back:

```bash
./scripts/run-vaio-sgx-ioctl-submit-op.sh
./scripts/run-vaio-sgx-multi-submit-default.sh
```

After that, the repo still needs one final follow-up for this batch:

- confirm the deadlock is gone under real hardware cleanup on the fixed
  candidate module
- keep the validation strictly sequential now that the runners serialize on the
  target
