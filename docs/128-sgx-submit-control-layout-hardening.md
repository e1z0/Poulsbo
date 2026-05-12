# SGX Submit Control-Layout Hardening

Date: 2026-05-12

## Problem

The new execution-shaped submit model now writes several control-side regions
into the same backing object:

- queue header
- queue payload
- CCB record
- manifest block
- descriptor block
- plan block

The kernel had overlap checks, but they were inconsistent and spread across
multiple paths:

- `manifest` configuration did not reject overlap with an already-configured
  `descriptor` block
- `queue` and `ccb` configuration could accept control layouts that only
  failed later in the full submit validator
- `refresh_submit_views_locked()` would immediately rewrite control-side
  blocks, so “it will fail later during validate” was not a safe contract

That meant a bad control layout could be materialized in backing memory before
the stronger end-to-end submit validator ever ran.

## Change

Kernel-side hardening landed in:

- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`

The new shared control-layout path now:

- computes active control-side regions for:
  - queue
  - CCB
  - manifest
  - descriptor
  - plan
- rejects overlap with the reserved control header
- rejects illegal queue/CCB placement
- rejects cross-region overlap between:
  - queue and manifest/descriptor/plan
  - CCB and manifest/descriptor/plan
  - manifest and descriptor/plan
  - descriptor and plan
- validates object bounds for every active control-side region

That shared validation is now used in three places:

1. the central submit validator
2. `refresh_submit_views_locked()` before any control-side refresh write
3. the direct mutating control-side setup paths:
   - `kick`
   - `ccb`
   - `queue`
   - `manifest`
   - `descriptor`
   - `plan`

The CCB path is also hardened one step further now: it requires packet config
first, so it cannot treat an uninitialized `ccb_offset` as a writable layout.

## Result

This closes the specific `manifest`/`descriptor` hole and, more importantly,
turns control-layout safety into one shared kernel rule instead of several
partially overlapping local checks.

That is the right shape for the next `plan` recovery attempt: if the target
comes back, the next run will fail at one bounded kernel validation point
instead of allowing different control-side writers to diverge.

## Local Verification

Passed locally:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

The build now succeeds cleanly again. The only remaining compiler warning in
this area is the existing large frame in the full submit validator, not the
new setter-side hardening.

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
