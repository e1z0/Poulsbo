# SGX Multi-Epoch Cycle Model

This note records the first multi-epoch queue-cycle validation on one
persistent submit object in the Ubuntu 5.4 Poulsbo lab driver.

## Why This Step Exists

The previous milestone in [docs/29-sgx-queue-cycle-model.md](/root/Poulsbo/docs/29-sgx-queue-cycle-model.md)
proved one refill epoch:

- `complete -> queued -> running -> complete`
- `queue_epoch` incrementing from `0` to `1`

That still left two open questions:

- whether the same submit object could survive more than one refill epoch
- whether `queue_epoch` should stay purely driver-side or become coupled to the
  status-backed sync and owner blocks that already mirror queue progress

This step answers both by validating three refill epochs on the same submit and
making the sync and owner blocks carry the current epoch word in memory.

## Current Kernel Surface

The existing submit control remains:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

This milestone does not add a new command.

It extends the existing memory-backed state model so that:

- the sync block now writes and validates `queue_epoch` at `sync_offset + 20`
- the owner block now writes and validates `queue_epoch` at `owner_offset + 12`

The live submit view continues to expose:

- `queue_epoch`
- `last_epoch_jiffies`

## Current Epoch-Coupling Semantics

The model is still validate-only. It does not execute SGX work.

The current coupling rules are:

- `sync` configuration writes the current `queue_epoch` into the sync block
- `owner` configuration writes the current `queue_epoch` into the owner block
- `issue` rewrites both epoch words with the new epoch after refill
- `transition` rewrites both epoch words with the current epoch during
  completion-side progression
- `validate` now reads both epoch words back and requires them to match the
  live submit `queue_epoch`

That means the epoch is no longer only a driver-side counter. It is now part of
the explicit status-backed queue state that validation reads from memory.

## Helper Changes

The VAIO helper:

- `scripts/run-vaio-sgx-submit.sh`

now also supports:

- `REFILL_CYCLES=<n>`

The default remains:

- `REFILL_CYCLES=1`

so the existing single-refill path and logs stay stable.

For `REFILL_CYCLES > 1`, the helper now loops the same refill pattern on the
same submit object and prints numbered sections:

- `SUBMITS_AFTER_REFILL_CYCLE_1_ISSUE`
- `SUBMITS_AFTER_REFILL_CYCLE_1_ISSUE_VALIDATE`
- `SUBMITS_AFTER_REFILL_CYCLE_1_RUNNING`
- `SUBMITS_AFTER_REFILL_CYCLE_1_RUNNING_VALIDATE`
- `SUBMITS_AFTER_REFILL_CYCLE_1_COMPLETE`
- `SUBMITS_AFTER_REFILL_CYCLE_1_COMPLETE_VALIDATE`

and so on for later cycles.

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026 with locally
built override module:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion C94A92B9C66705DD4B23C07`

The validation used:

- `REFILL_CYCLES=3`

Wrap-mode multi-epoch validation:

- `QUEUE_MODE=wrap`
- initial configuration still started with:
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x41`
  - `owner_issue_cookie=0x101`
  - `owner_complete_cookie=0x100`
  - `queue_epoch=0`
- after the third refill issue:
  - `sync_complete_seq=0x44`
  - `sync_pending_seq=0x45`
  - `owner_issue_cookie=0x105`
  - `owner_complete_cookie=0x104`
  - `queue_epoch=3`
  - `issue_count=4`
  - `transition_count=6`
  - `validate_count=11`
- after the third refill complete:
  - `queue_packet_count=0`
  - `ccb_offset=0x20`
  - `ccb_read_offset=0x0`
  - `ccb_write_offset=0x0`
  - `sync_complete_seq=0x45`
  - `sync_pending_seq=0x45`
  - `owner_issue_cookie=0x105`
  - `owner_complete_cookie=0x105`
  - `queue_epoch=3`
  - `issue_count=4`
  - `transition_count=8`
  - `validate_count=13`
  - `last_validate_status=0`
- final validate after probe exit:
  - `validate_count=14`
  - `last_validate_status=0`

Relevant `dmesg` progression on the target:

- initial `sync` and `owner` setup logged `epoch=0`
- first-cycle issue logged `epoch=0 refill=0`
- later refill issues logged:
  - `epoch=1 refill=1`
  - `epoch=2 refill=1`
  - `epoch=3 refill=1`
- the matching transition logs carried the same epoch values through
  `running` and `complete`

Nowrap multi-epoch validation:

- `QUEUE_MODE=nowrap`
- initial configuration again started with:
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x41`
  - `owner_issue_cookie=0x101`
  - `owner_complete_cookie=0x100`
  - `queue_epoch=0`
- after the third refill issue:
  - `sync_complete_seq=0x44`
  - `sync_pending_seq=0x45`
  - `owner_issue_cookie=0x105`
  - `owner_complete_cookie=0x104`
  - `queue_epoch=3`
  - `issue_count=4`
  - `transition_count=6`
  - `validate_count=11`
- after the third refill complete:
  - `queue_packet_count=0`
  - `ccb_offset=0x40`
  - `ccb_read_offset=0x20`
  - `ccb_write_offset=0x20`
  - `sync_complete_seq=0x45`
  - `sync_pending_seq=0x45`
  - `owner_issue_cookie=0x105`
  - `owner_complete_cookie=0x105`
  - `queue_epoch=3`
  - `issue_count=4`
  - `transition_count=8`
  - `validate_count=13`
  - `last_validate_status=0`
- final validate after probe exit:
  - `validate_count=14`
  - `last_validate_status=0`

The same epoch progression also appeared in `dmesg` for submit `id=2`:

- `epoch=0` during initial sync and owner setup
- `epoch=0 refill=0` for the initial issue
- `epoch=1`, `epoch=2`, and `epoch=3` for later refill issues and transitions

Post-run target state:

- `lightdm=active`
- `fault_count=0`
- `bif_fault=0x000000`
- `capture_failed=0`
- `submits_shown=0`
- `objects_shown=0`

## Next Step

The next useful step is no longer “can one submit survive several epochs?” That
is now validated.

The next questions are:

- whether the current epoch-coupled single-submit model should grow into
  broader generation semantics across multiple persistent submits
- whether any later sync or fence state needs its own epoch word instead of
  piggybacking on the current sync/owner blocks
- how far the current validate-only queue model should be pushed before moving
  toward a more SGX-shaped render-facing interface
