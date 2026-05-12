# SGX Dependency Slot Clear

This note records the new indexed dependency-slot clear path and the original
target-side blocker that stopped hardware validation in that step.

Hardware validation was completed later and is documented in
[docs/54-sgx-dependency-slot-clear-validation.md](/root/Poulsbo/docs/54-sgx-dependency-slot-clear-validation.md).

## Why This Step Exists

[docs/45-sgx-fourth-indexed-dependency-slot.md](/root/Poulsbo/docs/45-sgx-fourth-indexed-dependency-slot.md)
proved that the indexed interface can gate one dependent submit on four
concrete source submits.

The next useful question is not “can we add slot 5?” It is:

- can one indexed slot be cleared and reused without rebuilding the whole
  submit model?

That is the first real step toward a broader dependency-array style interface.

## Implemented Local Changes

Kernel-side:

- added `depclear_slot <submit_id> <slot>` to the SGX submit sysfs parser in
  [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
- implemented a real clear path that:
  - zeroes the status-object dependency block for the selected slot
  - clears the slot's source submit, offset, required/observed snapshot state,
    flags, config count, and timestamps
  - clears the slot's required-ticket source
  - refreshes the remaining active dependency slots so shared-ticket fallback
    semantics stay coherent
- updated the SGX submit help text blocks to advertise `depclear_slot`

Helper-side:

- extended
  [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
  with:
  - `CLEAR_DEPENDENCY4_AFTER_D=1`
  - `RUN_SUBMIT_E_AFTER_CLEAR=1`
- added the dedicated wrapper:
  - [scripts/run-vaio-sgx-dependency-slot-clear.sh](/root/Poulsbo/scripts/run-vaio-sgx-dependency-slot-clear.sh)

The intended hardware flow is:

1. gate submit B on slots `1..4`
2. run source submits A, C, and D
3. verify B is still blocked only because slot `4` remains active
4. run `depclear_slot B 4`
5. verify B can now run without waiting for E
6. optionally run E afterward and verify slot `4` stays detached

## Local Validation

Local checks passed:

- `bash -n scripts/run-vaio-sgx-multi-submit.sh`
- `bash -n scripts/run-vaio-sgx-dependency-slot-clear.sh`
- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- local Ubuntu 5.4 module build

The current local module build is:

- `build/devnull-vaio-gma500-src/gma500_gfx.ko`
- `srcversion 2B42B80E462B9A3C0BAAC9B`

## Hardware Validation Blocker

The new module was deployed to `10.1.11.218` with the guarded reboot flow, but
hardware validation did not complete.

Current observed target state after the reboot:

- `ping_status=ok`
- `tcp_22_status=closed`
- `ssh_pubkey_status=connection_refused`
- `overall=ssh_port_down`

That state persisted across five repeated health polls, so this does not look
like the short normal reboot window from the earlier validated steps.

Because of that, the new `depclear_slot` flow is **implemented locally but not
hardware-validated yet**.

## Recovery

Use the VAIO console to remove the override module and reboot:

```bash
rm -f /lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko
depmod -a 5.4.0-150-generic
reboot
```

After the machine returns to:

- `overall=remote_usable`

the next command to run is:

```bash
./scripts/run-vaio-sgx-dependency-slot-clear.sh
```

## Next Step

The next useful step is not more local coding. It is:

- recover the VAIO
- rerun the clear-slot validation
- confirm that `depclear_slot 4` lets submit B proceed before E, and that a
  later run of E does not repopulate `dependency4_*`
