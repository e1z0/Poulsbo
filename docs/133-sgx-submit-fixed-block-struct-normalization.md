# SGX Submit Fixed-Block Struct Normalization

Date: 2026-05-12

## Problem

The blocked `plan` recovery candidate already had:

- shared control-layout hardening
- shared status-layout hardening
- a split central submit validator
- refresh preflight for all four memory-backed submit views
- structured `report`, `manifest`, `descriptor`, and `plan` backing images

But the older fixed prepare-side blocks were still inconsistent:

- queue header
- CCB record
- sync block
- owner block

Those still used ad hoc field-by-field writes in a mix of places:

- configure paths
- prepare
- issue
- transition
- validator readback

That meant the local candidate still had two submit models at once:

1. structured memory-backed submit views
2. hand-written fixed control/status blocks

## Change

Kernel-side normalization landed in:

- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`

The remaining fixed submit blocks now have structured images too:

- `struct psb_sgx_lab_queue_header_words`
- `struct psb_sgx_lab_ccb_record_words`
- `struct psb_sgx_lab_sync_words`
- `struct psb_sgx_lab_owner_words`

And shared helpers now cover:

- build
- read
- write

for each of those blocks.

The normalized helpers are now used across:

- direct configure ops:
  - `queue`
  - `ccb`
  - `sync`
  - `owner`
- `prepare`
- `issue`
- `transition`
- control-side fixed-block validation
- status-side fixed-block validation
- direct typed CCB readback in submit query

## Result

The blocked `plan` recovery candidate now has one consistent backing-image
model across the full fixed submit record:

- control-side fixed blocks:
  - queue header
  - CCB record
- status-side fixed blocks:
  - sync
  - owner
- memory-backed submit views:
  - report
  - manifest
  - descriptor
  - plan

That means the first post-recovery rerun will not mix structured readback for
the newer views with ad hoc fixed-block writes for the older queue/status path.
The validator now compares every fixed structured block through the same
`read + build + memcmp` pattern that the newer backing-image views already use.

## Local Verification

Passed locally:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

The updated module linked cleanly after the cross-path refactor.

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
