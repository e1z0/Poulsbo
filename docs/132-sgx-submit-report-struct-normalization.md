# SGX Submit `report` Struct Normalization

Date: 2026-05-12

## Problem

The local `plan` recovery candidate already had:

- shared control-layout hardening
- shared status-layout hardening
- a split central submit validator
- refresh preflight for `manifest`, `descriptor`, and `plan`

But `report` was still the odd submit view out.

Unlike the other memory-backed submit blocks, it still used:

- field-by-field refresh writes
- field-by-field status-side readback validation
- no structured prebuilt image in the shared refresh preflight

That meant the blocked `plan` candidate still had one asymmetric path in the
middle of the now-structured backing-image model.

## Change

Kernel-side normalization landed in:

- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`

The `report` block now has the same shape as the other memory-backed submit
views:

- `struct psb_sgx_lab_report_words`
- `poulsbo_sgx_build_submit_report_locked()`
- `poulsbo_sgx_write_submit_report_locked()`
- `poulsbo_sgx_read_submit_report_locked()`

The shared refresh preflight now also builds `report` before the first write,
alongside:

- `manifest`
- `descriptor`
- `plan`

The refresh path now writes `report` through the structured helper instead of
recomputing and storing each field inline, and the status-side validator now:

1. reads the live `report` block into one structured image
2. rebuilds the expected `report` block from the live submit state
3. compares them as one bounded backing image

## Result

The blocked `plan` recovery candidate is now more internally consistent:

- all four memory-backed submit views now use structured backing-image helpers:
  - `report`
  - `manifest`
  - `descriptor`
  - `plan`
- refresh preflight now covers every memory-backed submit view that the refresh
  pass can rewrite
- status-side readback no longer treats `report` as a special field-by-field
  case

This does not add new SGX behavior by itself. It closes one more asymmetry in
the execution-shaped submit model so the first post-recovery `plan` rerun is
more likely to fail at one bounded comparison point instead of through a stale
or half-normalized `report` path.

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

After console recovery, keep using the existing operator entrypoint:

```bash
./scripts/run-vaio-sgx-plan-session.sh
```
