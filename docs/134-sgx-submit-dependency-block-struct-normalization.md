# SGX Submit Dependency-Block Struct Normalization

Date: 2026-05-12

## Problem

The blocked `plan` recovery candidate already had:

- structured `report`, `manifest`, `descriptor`, and `plan` views
- structured fixed queue/status blocks:
  - queue header
  - CCB record
  - sync
  - owner

But dependency-state slots were still one remaining special case.

Their lifecycle still used open-coded scalar logic across:

- snapshot construction
- on-object writes
- status-side validation
- clear/reset

That left the dependency path as the last status-side submit block that did not
share the same structured helper model as the rest of the execution-shaped
submit record.

## Change

Kernel-side normalization landed in:

- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`

Dependency slots now use one structured 4-dword image:

- `struct psb_sgx_lab_dependency_words`

And one shared helper layer now covers:

- dynamic slot build from live dependency state
- readback from the status object
- writeback to the status object
- storing the mirrored snapshot into the in-memory submit state
- loading the mirrored snapshot back out of the submit state

That helper layer is now used across:

- dependency-block refresh
- status-side dependency-block validation
- dependency-slot clear

## Result

The blocked `plan` recovery candidate now has one consistent structured helper
model across every bounded status-side submit block:

- sync
- owner
- report
- dependency slots

The dependency path no longer has separate ad hoc write, validate, and clear
logic. The validator now compares live dependency-state blocks through the same
`read + expected-image + memcmp` pattern used for the other structured submit
blocks.

## Local Verification

Passed locally:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

## Hardware Status

There is still no new VAIO validation result for this batch because the target
remains down:

- `ping_status=fail`
- `tcp_22_status=closed`
- `ssh_pubkey_status=no_route`
- `overall=ssh_port_down`

## Next Step

After console recovery, keep using the same operator entrypoint:

```bash
./scripts/run-vaio-sgx-plan-session.sh
```
