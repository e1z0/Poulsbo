# SGX Multi-Submit Isolation

This note records the first concurrent multi-submit validation on the Ubuntu 5.4
Poulsbo lab driver.

## Why This Step Exists

The previous milestone in [docs/30-sgx-multi-epoch-cycle-model.md](/root/Poulsbo/docs/30-sgx-multi-epoch-cycle-model.md)
validated several refill epochs on one persistent submit object.

That still left a practical gap:

- the global `poulsbo_sgx_submits` view was too large to inspect more than one
  live submit cleanly
- the current epoch-coupled model had not yet been validated with two
  persistent submits alive at the same time

This step fixes both.

## Current Kernel Surface

The existing submit control remains:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

This milestone adds one focused per-submit view:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit_state`

and one new control operation on the existing submit node:

- `focus <submit_id>`
- `focus clear`

## Current Focused-View Semantics

The global `poulsbo_sgx_submits` dump is still useful for totals and cleanup,
but it is limited by sysfs `PAGE_SIZE`.

The new focused view solves that by:

- selecting one live submit with `focus <submit_id>`
- returning one full submit record from `poulsbo_sgx_submit_state`
- returning `focused_submit=none` when no submit is selected
- automatically clearing focus when the focused submit is dropped or when all
  submits are dropped during unload

That gives the lab path a stable way to inspect multiple persistent submits one
at a time without changing any submit semantics.

## Helper Changes

The repo now has a dedicated validator:

- `scripts/run-vaio-sgx-multi-submit.sh`

It creates two concurrent PF-context submits with separate object sets:

- submit A defaults to `QUEUE_MODE=wrap`
- submit B defaults to `QUEUE_MODE=nowrap`

The helper drives them in an interleaved pattern:

1. configure both submits
2. drive submit A through issue, retire, and refill cycles while submit B stays
   idle
3. inspect submit B to confirm it did not change
4. drive submit B through its own issue and refill path
5. inspect submit A again to confirm its final state held steady

Current defaults:

- `SUBMIT_A_REFILL_CYCLES=2`
- `SUBMIT_B_REFILL_CYCLES=1`

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026.

The concurrent two-submit validation ran with override module:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 2716472E418F5F57F7D7D04`

Submit A setup state:

- `QUEUE_MODE=wrap`
- `queue_epoch=0`
- `issue_count=0`
- `transition_count=0`
- `validate_count=1`
- `sync_complete_seq=0x40`
- `sync_pending_seq=0x41`
- `owner_issue_cookie=0x101`
- `owner_complete_cookie=0x100`

Submit B setup state:

- `QUEUE_MODE=nowrap`
- `queue_epoch=0`
- `issue_count=0`
- `transition_count=0`
- `validate_count=1`
- `sync_complete_seq=0x40`
- `sync_pending_seq=0x41`
- `owner_issue_cookie=0x101`
- `owner_complete_cookie=0x100`

Submit A was then driven first with:

- `SUBMIT_A_REFILL_CYCLES=2`

Its final state after submit B had not yet run was:

- `queue_epoch=2`
- `issue_count=3`
- `transition_count=6`
- `validate_count=10`
- `last_validate_status=0`
- `sync_complete_seq=0x44`
- `sync_pending_seq=0x44`
- `owner_issue_cookie=0x104`
- `owner_complete_cookie=0x104`

At that point submit B was re-read through the focused state node and was still
idle:

- `queue_epoch=0`
- `issue_count=0`
- `transition_count=0`
- `validate_count=1`
- `sync_complete_seq=0x40`
- `sync_pending_seq=0x41`
- `owner_issue_cookie=0x101`
- `owner_complete_cookie=0x100`

Submit B was then driven with:

- `SUBMIT_B_REFILL_CYCLES=1`

Its final state was:

- `queue_epoch=1`
- `issue_count=2`
- `transition_count=4`
- `validate_count=7`
- `last_validate_status=0`
- `sync_complete_seq=0x43`
- `sync_pending_seq=0x43`
- `owner_issue_cookie=0x103`
- `owner_complete_cookie=0x103`

After submit B finished, submit A was re-read again and remained unchanged:

- `queue_epoch=2`
- `issue_count=3`
- `transition_count=6`
- `validate_count=10`
- `last_validate_status=0`
- `sync_complete_seq=0x44`
- `sync_pending_seq=0x44`
- `owner_issue_cookie=0x104`
- `owner_complete_cookie=0x104`

After all userspace holders exited, both submits still validated cleanly:

- submit A:
  - `validate_count=11`
  - `last_validate_status=0`
- submit B:
  - `validate_count=8`
  - `last_validate_status=0`

Post-run target state:

- `lightdm=active`
- `fault_count=0`
- `bif_fault=0x000000`
- `capture_failed=0`
- `submits_shown=0`
- `objects_shown=0`

## Focus-Clear Follow-Up

The first focused-view implementation left `poulsbo_sgx_submit_state` pointing
at a missing submit after the helper dropped the focused ID.

That cleanup bug is now fixed in a later follow-up module:

- `srcversion DDC182564CFB437A4C27B84`

Minimal validation on the VAIO:

- create one submit
- `focus <submit_id>`
- confirm `state=present`
- `drop <submit_id>`
- confirm `focused_submit=none`

So the focused-view API is now self-consistent after drop as well as during
concurrent validation.

## Next Step

The next useful question is no longer whether two persistent submits can
coexist. That is now validated.

The next questions are:

- whether any later render-facing state needs global or per-submit generation
  semantics beyond the current per-submit `queue_epoch`
- whether later queue or fence ownership should include explicit cross-submit
  coordination before any real SGX execution attempt
