# SGX Transition Model

This note records the first explicit queue-state transition model added on top
of the already validated sync and owner blocks in the Ubuntu 5.4 Poulsbo lab
driver.

## Why This Step Exists

Before this step, the submit model already had:

- explicit queue geometry with wrapped and linear validation
- a status-backed sync block with `complete_seq` and `pending_seq`
- a status-backed owner block with `owner_state`, `issue_cookie`, and
  `complete_cookie`

But those blocks were still static after setup. They could describe an initial
queue snapshot, but not a controlled progression through queue ownership and
completion.

The goal of this step is to make queue progression explicit in memory without
claiming real SGX execution.

## Current Kernel Surface

The existing submit control remains:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

The submit path now adds:

- a new submit command:
  - `transition <submit_id> <owner_state> <completed_packets>`

The live submit view now also exposes:

- `transition_count`
- `last_transition_jiffies`

## Current Transition Semantics

The current model is still validate-only. It does not execute SGX work.

It currently supports:

- `queued -> running`
- `running -> running`
- `running -> complete`

The transition writes through the existing `control` and `status` objects and
updates:

- `ccb_offset`
- `ccb_read_offset`
- `ccb_write_offset`
- `queue_packet_count`
- `queue_tail_packet_count`
- `queue_head_packet_count`
- `sync_complete_seq`
- `sync_flags`
- `owner_state`
- `owner_complete_cookie`

The current model also permits the queue to reach:

- `queue_packet_count=0`

but only through transition, not through initial queue configuration.

When the queue becomes empty, validation now requires:

- `ccb_write_offset == ccb_read_offset`
- `queue_tail_packet_count == 0`
- `queue_head_packet_count == 0`

## Current Validation Rules

The transition command is intentionally narrow:

- `queued` is only valid as an idempotent no-op with `completed_packets=0`
- `running` is valid from `queued` or `running`
- `running` cannot retire every outstanding packet in one step
- `complete` is only valid from `running`
- `complete` must retire every remaining outstanding packet
- later transitions out of `complete` are rejected

Completion advancement is reflected in two places:

- `sync_complete_seq += completed_packets`
- `owner_complete_cookie += completed_packets`

The write-side values remain fixed in this milestone:

- `sync_pending_seq`
- `owner_issue_cookie`

That is deliberate. This step is about completion-side progression first, not
about full queue issue/retire semantics.

## Helper Changes

The original transition-only helper milestone used:

- `scripts/run-vaio-sgx-submit.sh`

with:

- `TRANSITION_ENABLE=1`

When transitions are enabled, the helper now also defaults the initial owner
state to:

- `owner_state=queued`

It then runs:

- `transition <submit_id> running <completed_packets>`
- `validate <submit_id>`
- `transition <submit_id> complete <completed_packets>`
- `validate <submit_id>`

Current defaults:

- `QUEUE_MODE=wrap`
  - `TRANSITION_RUNNING_COMPLETED_PACKETS=1`
  - `TRANSITION_COMPLETE_COMPLETED_PACKETS=1`
- `QUEUE_MODE=nowrap`
  - `TRANSITION_RUNNING_COMPLETED_PACKETS=0`
  - `TRANSITION_COMPLETE_COMPLETED_PACKETS=1`

The helper now prints:

- `SUBMITS_AFTER_TRANSITION_RUNNING`
- `SUBMITS_AFTER_TRANSITION_RUNNING_VALIDATE`
- `SUBMITS_AFTER_TRANSITION_COMPLETE`
- `SUBMITS_AFTER_TRANSITION_COMPLETE_VALIDATE`

Since the later issue-side milestone in `docs/28-sgx-issue-model.md`, the same
helper now defaults to `ISSUE_ENABLE=1` and inserts an explicit issue phase
before the transition sequence. The transition-only flow remains available with
`ISSUE_ENABLE=0`.

## i686 Build Note

The first implementation used a direct 64-bit modulus in the queue-advance
path, which broke the 32-bit Ubuntu 5.4 module build with:

- `__umoddi3` undefined

The live implementation now uses:

- `div_u64_rem()`

so the transition math remains kernel-safe on the VAIO's `i686` build target.

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026 with locally
built override module:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 0A250824D4E00B9BBB52C70`

Wrap-mode validation:

- `QUEUE_MODE=wrap`
- initial state:
  - `owner_state=queued`
  - `queue_packet_count=2`
  - `queue_wrap=1`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x42`
  - `owner_issue_cookie=0x102`
  - `owner_complete_cookie=0x100`
- after `running 1`:
  - `owner_state=running`
  - `queue_packet_count=1`
  - `queue_wrap=0`
  - `sync_complete_seq=0x41`
  - `owner_complete_cookie=0x101`
- after `complete 1`:
  - `owner_state=complete`
  - `queue_packet_count=0`
  - `sync_complete_seq=0x42`
  - `sync_pending_seq=0x42`
  - `owner_complete_cookie=0x102`
  - `transition_count=2`
  - `validate_count=4`
  - `last_validate_status=0`

Nowrap validation:

- `QUEUE_MODE=nowrap`
- initial state:
  - `owner_state=queued`
  - `queue_packet_count=1`
  - `queue_wrap=0`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x41`
  - `owner_issue_cookie=0x101`
  - `owner_complete_cookie=0x100`
- after `running 0`:
  - `owner_state=running`
  - `queue_packet_count=1`
  - `sync_complete_seq=0x40`
  - `owner_complete_cookie=0x100`
- after `complete 1`:
  - `owner_state=complete`
  - `queue_packet_count=0`
  - `sync_complete_seq=0x41`
  - `sync_pending_seq=0x41`
  - `owner_complete_cookie=0x101`
  - `transition_count=2`
  - `validate_count=4`
  - `last_validate_status=0`

Post-run target state:

- `lightdm=active`
- `fault_count=0`
- `bif_fault=0x000000`
- `capture_failed=0`

## Next Step

The next useful step is not more completion-side transitions. It is to define
explicit issue-side progression:

- how `sync_pending_seq` advances
- how `owner_issue_cookie` advances
- how queue write-side state should move independently from retirement

while keeping the current model validate-only until reset, IRQ, and MMU
evidence justify anything closer to real SGX execution.
