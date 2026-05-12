# SGX Queue Cycle Model

This note records the first repeated queue-cycle validation on one persistent
submit object in the Ubuntu 5.4 Poulsbo lab driver.

## Why This Step Exists

Before this step, the submit path could already:

- issue new queue work
- retire work through `queued -> running -> complete`
- keep one submit object alive across userspace exit

But it had only been validated across one issue-and-retire cycle.

The next missing question was whether the same submit object could cleanly
refill after `complete`, carry forward the earlier sync and owner state, and
still validate another cycle without tearing the object down.

## Current Kernel Surface

The existing submit control remains:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

This milestone does not add a new command. It extends the existing `issue`
path with explicit refill tracking.

The live submit view now also exposes:

- `queue_epoch`
- `last_epoch_jiffies`

## Current Refill Semantics

The model is still validate-only. It does not execute SGX work.

Refill is currently defined as:

- `issue <submit_id> queued <issued_packets>`

from:

- `owner_state=complete`
- `queue_packet_count=0`

When that happens, the driver now:

- increments `queue_epoch`
- records `last_epoch_jiffies`
- advances `sync_pending_seq`
- advances `owner_issue_cookie`
- rewrites the queue payload into the newly re-issued slot

The completion-side state still advances only through the existing
`transition` path.

The current refill path is intentionally narrow:

- refill is only accepted from an empty completed queue
- `queue_epoch` is unchanged during the first cycle
- `queue_epoch` increments only on a real refill from `complete`
- `queue_epoch == U32_MAX` is rejected with `-EOVERFLOW`

That keeps refill distinct from normal first-cycle issue growth.

## Helper Changes

The VAIO helper:

- `scripts/run-vaio-sgx-submit.sh`

now defaults to a second cycle after the first completion path with:

- `REFILL_ENABLE=1`
- `REFILL_ISSUE_STATE=queued`
- `REFILL_ISSUED_PACKETS=1`
- `REFILL_RUNNING_STATE=running`
- `REFILL_RUNNING_COMPLETED_PACKETS=0`
- `REFILL_COMPLETE_STATE=complete`
- `REFILL_COMPLETE_COMPLETED_PACKETS=1`

It also validates that:

- `REFILL_ISSUED_PACKETS >= 1`
- `REFILL_RUNNING_COMPLETED_PACKETS + REFILL_COMPLETE_COMPLETED_PACKETS == REFILL_ISSUED_PACKETS`

The helper now prints:

- `SUBMITS_AFTER_REFILL_ISSUE`
- `SUBMITS_AFTER_REFILL_ISSUE_VALIDATE`
- `SUBMITS_AFTER_REFILL_RUNNING`
- `SUBMITS_AFTER_REFILL_RUNNING_VALIDATE`
- `SUBMITS_AFTER_REFILL_COMPLETE`
- `SUBMITS_AFTER_REFILL_COMPLETE_VALIDATE`

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026 with locally
built override module:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 7E750058A1E31F16F36953E`

Wrap-mode repeated-cycle validation:

- `QUEUE_MODE=wrap`
- first cycle:
  - after issue:
    - `queue_packet_count=2`
    - `queue_wrap=1`
    - `sync_pending_seq=0x42`
    - `owner_issue_cookie=0x102`
    - `queue_epoch=0`
    - `issue_count=1`
    - `validate_count=2`
  - after complete:
    - `queue_packet_count=0`
    - `sync_complete_seq=0x42`
    - `sync_pending_seq=0x42`
    - `owner_issue_cookie=0x102`
    - `owner_complete_cookie=0x102`
    - `transition_count=2`
    - `validate_count=4`
- second cycle refill:
  - after refill issue:
    - `queue_packet_count=1`
    - `sync_complete_seq=0x42`
    - `sync_pending_seq=0x43`
    - `owner_issue_cookie=0x103`
    - `owner_complete_cookie=0x102`
    - `queue_epoch=1`
    - `issue_count=2`
    - `validate_count=5`
  - after refill complete:
    - `queue_packet_count=0`
    - `sync_complete_seq=0x43`
    - `sync_pending_seq=0x43`
    - `owner_issue_cookie=0x103`
    - `owner_complete_cookie=0x103`
    - `queue_epoch=1`
    - `issue_count=2`
    - `transition_count=4`
    - `validate_count=7`
    - `last_validate_status=0`
- final validate after probe exit:
  - `validate_count=8`
  - `last_validate_status=0`

Target `dmesg` logged both phases explicitly:

- first-cycle issue with `epoch=0 refill=0`
- refill issue with `epoch=1 refill=1`

Nowrap repeated-cycle validation:

- `QUEUE_MODE=nowrap`
- first cycle:
  - after issue:
    - `queue_packet_count=2`
    - `queue_wrap=0`
    - `sync_pending_seq=0x42`
    - `owner_issue_cookie=0x102`
    - `queue_epoch=0`
    - `issue_count=1`
    - `validate_count=2`
  - after complete:
    - `queue_packet_count=0`
    - `transition_count=2`
    - `validate_count=4`
- second cycle refill:
  - after refill issue:
    - `queue_packet_count=1`
    - `sync_pending_seq=0x43`
    - `owner_issue_cookie=0x103`
    - `queue_epoch=1`
    - `issue_count=2`
    - `validate_count=5`
  - after refill complete:
    - `queue_packet_count=0`
    - `sync_complete_seq=0x43`
    - `sync_pending_seq=0x43`
    - `owner_issue_cookie=0x103`
    - `owner_complete_cookie=0x103`
    - `queue_epoch=1`
    - `issue_count=2`
    - `transition_count=4`
    - `validate_count=7`
    - `last_validate_status=0`
- final validate after probe exit:
  - `validate_count=8`
  - `last_validate_status=0`

Target `dmesg` again logged:

- first-cycle issue with `epoch=0 refill=0`
- refill issue with `epoch=1 refill=1`

Post-run target state:

- `lightdm=active`
- `fault_count=0`
- `bif_fault=0x000000`
- `capture_failed=0`
- `submits_shown=0`
- `objects_shown=0`

## Next Step

The immediate next step from this milestone is now complete in:

- `docs/30-sgx-multi-epoch-cycle-model.md`

That later step validated:

- three refill epochs on one persistent submit object
- explicit `queue_epoch` coupling into the sync and owner blocks
- stable `issue_count`, `transition_count`, and `validate_count` growth across
  more than one refill epoch

The remaining question is whether the current epoch-coupled single-submit model
is sufficient across multiple persistent submits or whether broader generation
semantics are needed later.
