# SGX Submit Refresh Preflight

Date: 2026-05-12

## Problem

The local `plan` recovery candidate already had:

- control-layout hardening
- status-layout hardening
- a split central submit validator

But the backing-image refresh path still updated submit views one at a time:

1. `report`
2. `manifest`
3. `descriptor`
4. `plan`

That meant a late failure in `manifest`, `descriptor`, or `plan` could still
leave a partially refreshed submit image even though the kernel had already
proven the general layout was valid.

For the blocked `plan` work, that was still too permissive.

## Change

Kernel-side refresh hardening landed in:

- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`

The new shared refresh path now does one explicit preflight before the first
write:

- finds and reuses the live control object once
- finds and reuses the live status object once
- validates the full control-side layout once
- validates the full status-side layout once
- prebuilds the memory-backed views that are about to be written:
  - manifest
  - descriptor
  - plan

Only after all of that succeeds does `refresh_submit_views_locked()` write:

1. `report`
2. `manifest`
3. `descriptor`
4. `plan`

The per-view refresh helpers now consume the preflighted objects and prebuilt
images instead of each doing their own lookup/build/validate sequence.

## Result

The local `plan` recovery candidate is now stricter in one more useful way:

- later submit-view refresh failures no longer happen after earlier blocks were
  already rewritten in the same refresh pass

This does not make the refresh path literally transactional, but it does make
the common failure mode much narrower: validation and view construction now
fail before the first write instead of halfway through the write sequence.

## Local Verification

Passed locally:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

The module still builds cleanly after the refresh refactor.

## Hardware Status

There is still no new VAIO validation result for this batch because the target
remains down:

- `ping_status=fail`
- `tcp_22_status=closed`
- `ssh_pubkey_status=no_route`
- `overall=ssh_port_down`

## Next Step

After console recovery, use the existing operator entrypoint:

```bash
./scripts/run-vaio-sgx-plan-session.sh
```
