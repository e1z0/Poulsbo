# SGX Sync Block Model

This note records the first explicit sync/progress block added on top of the
existing wrap-aware queue model in the Ubuntu 5.4 Poulsbo lab driver.

## Why This Step Exists

Before this step, the submit model already had:

- explicit `control/pds/use/target/status` roles
- a real in-object control block inside the `control` object
- explicit queue geometry and wrap-aware tail/head packet split
- deterministic TA and 3D status words in the `status` object

But queue progression still had no explicit progress-state words that could act
as the first stand-in for later ownership, completion, or fence semantics.

The goal of this step is to stop inferring everything from queue geometry alone
and instead make a small progress block explicit in memory.

## Current Kernel Surface

The existing submit control remains:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

The submit path now adds:

- a new submit command:
  - `sync <submit_id> <offset> <complete_seq> <pending_seq> <flags>`

The sync block lives in the existing `status` object. The current layout is:

- dword `+0x0`: mirrored `ccb_read_offset`
- dword `+0x4`: mirrored `ccb_write_offset`
- dword `+0x8`: `sync_complete_seq`
- dword `+0xc`: `sync_pending_seq`
- dword `+0x10`: `sync_flags`

The currently supported flags are:

- `0x1`: `READY`
- `0x2`: `WRAPPED`

## Current Validation Rules

The current sync block is still validate-only. It does not execute SGX work.

If configured, validation now requires:

- a live `status` object
- a configured queue model
- a non-overlapping sync block region inside the `status` object
- `READY` to be set
- `WRAPPED` to match the queue wrap state
- `pending_seq >= complete_seq`
- `pending_seq - complete_seq == queue_packet_count`
- live readback of the mirrored queue read/write offsets from the `status`
  object
- live readback of the configured sync sequence values and flag word

This makes queue progress, completion distance, and wrapped state explicit
without pretending the driver already has real SGX fences or execution.

## Helper Changes

The VAIO helper:

- `scripts/run-vaio-sgx-submit.sh`

now programs the sync block after `status`.

Current defaults:

- `QUEUE_MODE=wrap`
  - `sync_offset=0x20`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x42`
  - `sync_flags=0x3`
- `QUEUE_MODE=nowrap`
  - `sync_offset=0x20`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x41`
  - `sync_flags=0x1`

The live submit view now also exposes:

- `sync_offset`
- `sync_complete_seq`
- `sync_pending_seq`
- `sync_flags`
- `sync_config_count`
- `last_sync_jiffies`

## Current Status

Validated status on May 11, 2026:

- code compiled successfully for the Ubuntu `5.4.0-150-generic` VAIO target
- locally built module `srcversion`:
  - `8B1DA24F8B1EFA8C47EFB32`
- helper syntax check passed:
  - `bash -n scripts/run-vaio-sgx-submit.sh`
- kernel worktree diff check passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- the VAIO is running the override module:
  - `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `lightdm` is active again after the validation runs

## Hardware Validation Result

This step is now hardware-validated on `10.1.11.218` in both queue modes.

Wrap-mode validation:

- `QUEUE_MODE=wrap`
- `HOLD_SECONDS=5`
- observed submit state:
  - `sync_offset=0x20`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x42`
  - `sync_flags=0x3`
  - `queue_wrap=1`
  - `validate_count=2`
  - `last_validate_status=0`
- target `dmesg` logged:
  - `lab submit sync submit_id=2 offset=0x20 read_offset=0x60 write_offset=0x20 complete_seq=0x40 pending_seq=0x42 flags=0x3 wrapped=1`
  - two successful `lab submit validate ... status=0`

Nowrap validation:

- `QUEUE_MODE=nowrap`
- `HOLD_SECONDS=5`
- observed submit state:
  - `sync_offset=0x20`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x41`
  - `sync_flags=0x1`
  - `queue_wrap=0`
  - `validate_count=2`
  - `last_validate_status=0`
- target `dmesg` logged:
  - `lab submit sync submit_id=3 offset=0x20 read_offset=0x0 write_offset=0x20 complete_seq=0x40 pending_seq=0x41 flags=0x1 wrapped=0`
  - two successful `lab submit validate ... status=0`

Post-run SGX state:

- `fault_count=0`
- `bif_fault=0x000000`
- `capture_failed=0`

## Helper Hardening

The reruns needed three helper fixes before the validation became reliably
repeatable:

- unique staged remote binary paths
- stale SGX lab object and submit cleanup before each run
- a fallback range-ID scan beyond the page-sized `poulsbo_gtt_state` dump when
  fresh GEM ranges are truncated out of the visible snapshot

## Next Step

The next useful step is not more sync-block bring-up. It is to build queue
ownership or completion semantics on top of this now-validated sync state.
