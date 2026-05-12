# SGX Issue Model

This note records the first explicit issue-side queue progression model added on
top of the already validated sync, owner, and transition blocks in the Ubuntu
5.4 Poulsbo lab driver.

## Why This Step Exists

Before this step, the submit model could:

- describe queued work with explicit queue geometry
- describe completion distance with `sync_complete_seq` and `sync_pending_seq`
- describe ownership with `owner_state`, `owner_issue_cookie`, and
  `owner_complete_cookie`
- retire queued work through `queued -> running -> complete` transitions

But write-side progression was still static. The model could retire work, but
not explicitly issue new work into the queue.

The goal of this step is to make queue growth explicit in memory without
claiming real SGX execution.

## Current Kernel Surface

The existing submit control remains:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

The submit path now adds:

- a new submit command:
  - `issue <submit_id> <owner_state> <issued_packets>`

The live submit view now also exposes:

- `issue_count`
- `last_issue_jiffies`

## Current Issue Semantics

The current model is still validate-only. It does not execute SGX work.

The issue command updates write-side queue state and leaves read-side
completion state untouched. It currently advances:

- `ccb_write_offset`
- `queue_packet_count`
- `queue_tail_packet_count`
- `queue_head_packet_count`
- `sync_pending_seq`
- `sync_flags`
- `owner_state`
- `owner_issue_cookie`

It also writes the existing dummy command payload into each newly issued queue
slot so later read-side validation still sees a fully populated queue record.

The current completion-side values remain fixed during issue:

- `ccb_read_offset`
- `sync_complete_seq`
- `owner_complete_cookie`

## Current Validation Rules

The issue command is intentionally narrow:

- `issued_packets` must be at least `1`
- the new outstanding count must fit the configured queue capacity
- `queued -> queued` is allowed
- `running -> running` is allowed
- `complete -> queued` is allowed only from an empty queue
- `issue` does not move directly into `running` or `complete`

That keeps issue-side growth distinct from completion-side retirement.

## Helper Changes

The VAIO helper:

- `scripts/run-vaio-sgx-submit.sh`

now defaults to:

- `ISSUE_ENABLE=1`
- `TRANSITION_ENABLE=1`

When issue is enabled, the helper starts from a smaller queued state, validates
that state, then issues more work before retirement.

Current defaults:

- `QUEUE_MODE=wrap`
  - initial queue:
    - `ccb_read_offset=0x60`
    - `ccb_write_offset=0x0`
    - `queue_packet_count=1`
    - `sync_pending_seq=0x41`
    - `owner_issue_cookie=0x101`
  - issue phase:
    - `ISSUE_STATE=queued`
    - `ISSUE_ISSUED_PACKETS=1`
- `QUEUE_MODE=nowrap`
  - initial queue:
    - `ccb_read_offset=0x0`
    - `ccb_write_offset=0x20`
    - `queue_packet_count=1`
    - `sync_pending_seq=0x41`
    - `owner_issue_cookie=0x101`
  - issue phase:
    - `ISSUE_STATE=queued`
    - `ISSUE_ISSUED_PACKETS=1`

The helper now prints:

- `SUBMITS_AFTER_ISSUE`
- `SUBMITS_AFTER_ISSUE_VALIDATE`

before continuing into the later transition sequence.

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026 with locally
built override module:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 2A252234DCE536B6C9CF9F6`

Wrap-mode validation:

- `QUEUE_MODE=wrap`
- initial validated queue:
  - `queue_packet_count=1`
  - `queue_wrap=0`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x41`
  - `owner_issue_cookie=0x101`
  - `owner_complete_cookie=0x100`
- after `issue queued 1`:
  - `ccb_write_offset=0x20`
  - `queue_packet_count=2`
  - `queue_tail_packet_count=1`
  - `queue_head_packet_count=1`
  - `queue_wrap=1`
  - `sync_pending_seq=0x42`
  - `owner_issue_cookie=0x102`
  - `issue_count=1`
  - `validate_count=2`
  - `last_validate_status=0`

Nowrap validation:

- `QUEUE_MODE=nowrap`
- initial validated queue:
  - `queue_packet_count=1`
  - `queue_wrap=0`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x41`
  - `owner_issue_cookie=0x101`
  - `owner_complete_cookie=0x100`
- after `issue queued 1`:
  - `ccb_write_offset=0x0`
  - `queue_packet_count=2`
  - `queue_tail_packet_count=2`
  - `queue_head_packet_count=0`
  - `queue_wrap=0`
  - `sync_pending_seq=0x42`
  - `owner_issue_cookie=0x102`
  - `issue_count=1`
  - `validate_count=2`
  - `last_validate_status=0`

In both modes, the helper then retired the issued work through the existing
transition model and finished with:

- `transition_count=2`
- `validate_count=5`
- `last_validate_status=0`

Post-run target state:

- `lightdm=active`
- `fault_count=0`
- `bif_fault=0x000000`
- `capture_failed=0`
- `submits_shown=0`
- `objects_shown=0`

## Next Step

The immediate next step from this milestone is now complete in:

- `docs/29-sgx-queue-cycle-model.md`

That later step validated:

- `complete -> queued` refill on the same submit object
- a first explicit `queue_epoch`
- one repeated issue-and-retire cycle after the first `complete`

The remaining question is whether broader generation tracking beyond the
current `queue_epoch` is needed across many refill epochs before any real SGX
execution attempt.
