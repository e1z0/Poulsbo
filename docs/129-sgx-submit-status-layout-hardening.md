# SGX Submit Status-Layout Hardening

Date: 2026-05-12

## Problem

The blocked `plan` work already pushed the lab submit model into a denser
status-side layout in the backing status object:

- TA status block
- 3D status block
- sync block
- owner block
- report block
- dependency-state blocks

The kernel still validated those regions through several partially overlapping
local checks:

- `status` configuration did not reject overlap with an already-configured
  `owner` block or dependency-state block
- `sync` and `owner` configuration each checked only part of the full status
  layout
- dependency refresh and report refresh validated only their own narrow write
  window instead of one shared status-layout contract

That meant the control side was getting a single hardened rule while the
status side could still drift across setters, refresh paths, and the central
submit validator.

## Change

Kernel-side hardening landed in:

- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`

The new shared status-layout path now:

- computes active status-side regions for:
  - TA status
  - 3D status
  - sync
  - owner
  - report
  - all dependency-state slots
- rejects overlap between:
  - TA and 3D status
  - sync and TA/3D
  - owner and sync/TA/3D
  - report and sync/owner/TA/3D
  - dependency-state slots and sync/owner/report/TA/3D
  - dependency-state slots and each other
- validates object bounds for every active status-side region

That shared validation is now used in three places:

1. the central submit validator
2. `refresh_submit_report_locked()` and dependency refresh before any
   status-side writeback
3. the direct mutating status-side setup paths:
   - `status`
   - `sync`
   - `owner`
   - `report`
   - dependency-state slot configuration

Like the control-side hardening batch, the setter paths use temporary field
overrides plus restore instead of whole-submit stack copies.

## Result

The status side now follows the same rule as the control side: one shared
kernel layout contract, checked before refresh writes and before direct typed
mutation succeeds.

That closes the concrete missing-owner and missing-dependency overlap holes on
`status`, `sync`, and `owner`, and it makes the next `plan` recovery attempt
less likely to fail through silent status-object self-corruption.

## Local Verification

Passed locally:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

The module build is clean again after adding the missing forward declarations
for the dependency-slot pointer helpers used by the new shared validator.

The only remaining compiler warning in this area is still the large stack
frame in the full submit validator, not a new setter-side or refresh-side
regression from this batch.

## Hardware Status

There is no new VAIO validation result for this batch because the target is
still down:

- `ping_status=fail`
- `tcp_22_status=closed`
- `ssh_pubkey_status=no_route`
- `overall=ssh_port_down`

## Next Step

After console recovery, use the existing operator entrypoint:

```bash
./scripts/run-vaio-sgx-plan-session.sh
```
