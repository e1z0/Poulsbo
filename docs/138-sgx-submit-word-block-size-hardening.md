# SGX Submit Word-Block Size Hardening

Date: 2026-05-12

## What Changed

The blocked `plan` line exposed one concrete kernel-side size bug in
`drivers/gpu/drm/gma500/psb_drv.c`:

- `PSB_SGX_LAB_PLAN_DWORDS` still said `42`
- `struct psb_sgx_lab_plan_words` actually contained `41` `u32` fields

That meant both:

- `poulsbo_sgx_write_submit_plan_locked()`
- `poulsbo_sgx_read_submit_plan_locked()`

were iterating one dword past the end of the private plan image.

This batch fixed that immediate bug and widened it into one class-level
hardening pass for all normalized word-backed submit blocks.

## Kernel Changes

In `drivers/gpu/drm/gma500/psb_drv.c`:

- fixed `PSB_SGX_LAB_PLAN_DWORDS` from `42` to `41`
- added compile-time size checks for all normalized word-backed blocks:
  - `queue_header`
  - `ccb_record`
  - `sync`
  - `owner`
  - `report`
  - `manifest`
  - `descriptor`
  - `plan`
  - `dependency`
- added shared word-copy helpers:
  - `poulsbo_sgx_object_write_words_locked()`
  - `poulsbo_sgx_object_read_words_locked()`
- rewired the loop-based block read/write paths to derive their word counts
  from `sizeof(struct) / sizeof(u32)` instead of repeating hand-kept dword
  loop bounds:
  - queue header
  - CCB record
  - sync
  - owner
  - report
  - descriptor
  - plan
  - dependency

The important consequence is that future private-struct growth now fails
build-time instead of silently overrunning a stale loop bound.

## Local Validation

Validated locally:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

Both passed cleanly.

## Hardware Status

There is still no new hardware result for this batch because the VAIO remains
down:

- `ping_status=fail`
- `tcp_22_status=closed`
- `ssh_pubkey_status=no_route`
- `overall=ssh_port_down`

So this batch is build-clean and ready for the next recovery-driven rerun, but
not yet hardware-validated.
