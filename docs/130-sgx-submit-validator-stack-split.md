# SGX Submit Validator Stack Split

Date: 2026-05-12

## Problem

After the control-layout and status-layout hardening batches, the central
submit validator still compiled with one warning:

- large stack frame in `poulsbo_sgx_validate_submit_locked()`

The reason was structural, not one stray local variable. That function still
carried all of these on one frame at once:

- control-side queue and CCB readback words
- expected and observed manifest images
- expected and observed descriptor images
- expected and observed plan images
- status-side sync, owner, report, and dependency readback words
- duplicated inline layout-overlap logic that now already existed in the new
  shared control/status layout validators

That was exactly the wrong place to leave monolithic stack growth while the
`plan` target is already down.

## Change

Kernel-side cleanup landed in:

- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`

The central validator is now split into three layers:

1. central semantic and object validation in
   `poulsbo_sgx_validate_submit_locked()`
2. control-side backing-image validation in
   `poulsbo_sgx_validate_submit_control_side_images_locked()`
3. status-side backing-image validation in
   `poulsbo_sgx_validate_submit_status_side_images_locked()`

The central path now also relies on the already-shared layout validators
instead of re-implementing their overlap checks inline:

- `poulsbo_sgx_validate_submit_control_layout_locked()`
- `poulsbo_sgx_validate_submit_status_layout_locked()`

That means:

- control-side image checks still validate queue header, queue progression,
  CCB record, manifest, descriptor, and plan contents
- status-side image checks still validate sync, owner, report, dependency
  blocks, and TA/3D status values
- but the bulk backing-image snapshots no longer sit on the same stack frame
  as the full semantic validator

## Result

The local kernel candidate is now:

- control-layout hardened
- status-layout hardened
- validator stack-clean

This is a better recovery candidate for the blocked `plan` work: the next
post-recovery run should fail, if it still fails, through one bounded kernel
validation path rather than through layout drift or a ballooning monolithic
validator frame.

## Local Verification

Passed locally:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

The module now builds without the earlier large-frame warning in
`poulsbo_sgx_validate_submit_locked()`.

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
