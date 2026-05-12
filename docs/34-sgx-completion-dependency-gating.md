# SGX Completion-Dependency Gating

This note records the first hardware-validated use of `ta_dependency` as a real
completion-ticket gate for persistent SGX lab submits on the Ubuntu 5.4
Poulsbo target.

## Why This Step Exists

The previous milestone in [docs/33-sgx-running-owner-arbitration.md](/root/Poulsbo/docs/33-sgx-running-owner-arbitration.md)
proved that active `running` ownership is exclusive across submits.

That still left one missing piece:

- a queued submit could be blocked by an active owner, but there was still no
  explicit way to say “this submit must not run until some earlier completion
  ticket exists”
- the existing `ta_dependency` field was still effectively metadata

This step turns that field into a real validate-only gate.

## Current Kernel Semantics

The Ubuntu 5.4 lab driver now treats `ta_dependency` as:

- a minimum global completion ticket required before a submit may enter
  `running`

Current rules:

- `ta_dependency=0` means no dependency gate
- if `ta_dependency > global_complete_ticket`, a
  `queued -> running` transition fails with `-EAGAIN`
- once a submit is in `running` or `complete`, validation requires the global
  complete counter to satisfy `ta_dependency`
- active-owner arbitration still takes precedence once another submit is
  already running, so that path still returns `-EBUSY`

## Helper Changes

The concurrent validator:

- `scripts/run-vaio-sgx-multi-submit.sh`

now sets submit B's dependency automatically by default:

- `SUBMIT_B_DEPENDENCY_COMPLETE_TICKET=auto`

That resolves to:

- `SUBMIT_A_REFILL_CYCLES + 1`

for the expected final completion ticket of submit A.

The helper now exercises two distinct failure modes:

1. before submit A runs at all, submit B attempts `running` and must fail with
   `Resource temporarily unavailable`
2. while submit A is actively running, submit B attempts `running` again and
   must fail with `Device or resource busy`

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026.

The validated override module was:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion CDDD68E97136F8541780DA8`

### Dependency Gate Before Submit A Runs

Submit B setup state:

- `ta_dependency=3`
- `owner_state=queued`
- `queue_epoch=0`
- `global_issue_ticket=0`
- `global_running_ticket=0`
- `global_complete_ticket=0`
- `validate_count=1`

Before submit A started, submit B attempted to enter `running` and failed as
intended:

```text
write error: Resource temporarily unavailable
```

Immediately afterward:

- submit B was still unchanged
- the coord node still showed:
  - `issue_ticket_counter=0`
  - `running_ticket_counter=0`
  - `complete_ticket_counter=0`
  - `active_running_valid=0`

So the dependency gate blocked submit B before any shared coordination state
advanced.

### Running-Owner Gate While Submit A Was Active

After submit A entered `running`, the helper attempted to enter `running` on
submit B again. That second attempt failed with the expected owner-arbitration
error:

```text
write error: Device or resource busy
```

At that moment the coord node showed:

- `issue_ticket_counter=1`
- `running_ticket_counter=1`
- `complete_ticket_counter=0`
- `last_issue_submit_id=1`
- `last_running_submit_id=1`
- `active_running_valid=1`
- `active_running_submit_id=1`
- `active_running_ticket=1`

Submit B still remained fully idle and queued.

### Post-Dependency Progression

After submit A finished:

- `issue_ticket_counter=3`
- `running_ticket_counter=3`
- `complete_ticket_counter=3`
- `active_running_valid=0`

At that point submit B's dependency was satisfied and its normal run completed
cleanly.

Final state after userspace exit:

- submit A:
  - `ta_dependency=0`
  - `queue_epoch=2`
  - `global_issue_ticket=3`
  - `global_running_ticket=3`
  - `global_complete_ticket=3`
  - `validate_count=11`
  - `last_validate_status=0`
- submit B:
  - `ta_dependency=3`
  - `queue_epoch=1`
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

The lab path now has two distinct cross-submit gates:

- dependency gating on global completion progress
- arbitration gating on active running ownership

That is a materially better validate-only model for future render submission
than the earlier owner-only path.

## Next Step

The next useful questions are:

- whether one global completion-ticket dependency is enough, or whether future
  queue/fence semantics need multiple wait lanes
- whether dependency state should stay implicit in `ta_dependency` or become a
  more explicit state block once real render evidence exists
