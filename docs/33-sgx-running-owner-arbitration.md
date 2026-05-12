# SGX Running-Owner Arbitration

This note records the first hardware-validated exclusive running-owner lane for
multiple persistent SGX lab submits on the Ubuntu 5.4 Poulsbo target.

## Why This Step Exists

The previous milestone in [docs/32-sgx-global-submit-coordination.md](/root/Poulsbo/docs/32-sgx-global-submit-coordination.md)
added global issue-side and completion-side ordering, but it still left one
practical gap:

- cross-submit ordering existed, but active running ownership was not exclusive
- the coord view could not show which submit currently owned the running lane
- the helper had not yet proven that a second submit would be rejected while
  one submit was still active

This step adds that explicit arbitration.

## Current Kernel Surface

The coord view now also reports:

- `running_ticket_counter`
- `last_running_submit_id`
- `active_running_valid`
- `active_running_submit_id`
- `active_running_ticket`

Each submit now also carries:

- `global_running_ticket`

and that value is mirrored into the owner block alongside the existing
completion ticket.

## Validation Semantics

The current running-owner lane is still validate-only, but it is now explicit:

- the first `queued -> running` transition for a submit allocates a new global
  running ticket
- that transition marks the submit as the active running owner
- a second submit trying to enter `running` while another submit is active now
  fails with `-EBUSY`
- `running -> complete` clears the active running owner
- validation now rejects inconsistent states such as:
  - more than one submit in `running`
  - a `running` submit without the active-owner slot
  - an active-owner slot that points at a non-running submit

## Helper Changes

The concurrent helper:

- `scripts/run-vaio-sgx-multi-submit.sh`

now does one extra arbitration step:

1. drive submit A into `running`
2. attempt `transition submit_b running 0`
3. require that write to fail
4. confirm submit B stayed queued and idle
5. confirm the coord node still shows submit A as the active owner
6. let submit A complete, then continue with submit B normally

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026.

The validated override module was:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 39837C8555DA32AD66B21C9`

### Arbitration Point

After setup:

- `issue_ticket_counter=0`
- `running_ticket_counter=0`
- `complete_ticket_counter=0`
- `active_running_valid=0`
- `live_submit_count=2`

While submit A was in `running`:

- `issue_ticket_counter=1`
- `running_ticket_counter=1`
- `complete_ticket_counter=0`
- `last_issue_submit_id=1`
- `last_running_submit_id=1`
- `active_running_valid=1`
- `active_running_submit_id=1`
- `active_running_ticket=1`

At that point submit B attempted to enter `running` and was rejected exactly as
intended:

```text
write error: Device or resource busy
```

Immediately after that failed transition, submit B was still unchanged:

- `owner_state=queued`
- `queue_epoch=0`
- `issue_count=0`
- `transition_count=0`
- `global_issue_ticket=0`
- `global_running_ticket=0`
- `global_complete_ticket=0`
- `validate_count=1`

### Post-Arbitration Progression

After submit A finished:

- `issue_ticket_counter=3`
- `running_ticket_counter=3`
- `complete_ticket_counter=3`
- `last_issue_submit_id=1`
- `last_running_submit_id=1`
- `last_complete_submit_id=1`
- `active_running_valid=0`

After submit B then completed its own run:

- `issue_ticket_counter=5`
- `running_ticket_counter=5`
- `complete_ticket_counter=5`
- `last_issue_submit_id=2`
- `last_running_submit_id=2`
- `last_complete_submit_id=2`
- `active_running_valid=0`

Final per-submit state after userspace exit:

- submit A:
  - `queue_epoch=2`
  - `issue_count=3`
  - `transition_count=6`
  - `global_issue_ticket=3`
  - `global_running_ticket=3`
  - `global_complete_ticket=3`
  - `validate_count=11`
  - `last_validate_status=0`
- submit B:
  - `queue_epoch=1`
  - `issue_count=2`
  - `transition_count=4`
  - `global_issue_ticket=5`
  - `global_running_ticket=5`
  - `global_complete_ticket=5`
  - `validate_count=8`
  - `last_validate_status=0`

### Final Target State

- `lightdm=active`
- `fault_count=0`
- `bif_fault=0x000000`
- `capture_failed=0`
- `issue_ticket_counter=5`
- `running_ticket_counter=5`
- `complete_ticket_counter=5`
- `active_running_valid=0`
- `submits_shown=0`
- `focused_submit=none`
- `objects_shown=0`

## Conclusion

The lab driver now has three distinct validated coordination lanes:

- global issue ordering
- global running-owner arbitration
- global completion ordering

That is a better approximation of the state a real render path will eventually
need than the earlier issue/complete-only model.

## Next Step

The next useful question is whether those three lanes are enough, or whether
future queue dependencies or fence-like semantics need:

- additional wait/dependency lanes
- per-submit or per-context ownership generations
- a less ad-hoc control surface than the current sysfs lab path
