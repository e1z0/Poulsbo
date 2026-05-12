# SGX Source-Submit Dependency Binding

This note records the first hardware-validated use of an explicit source-submit
dependency for persistent SGX lab submits on the Ubuntu 5.4 Poulsbo target.

## Why This Step Exists

The previous milestone in [docs/34-sgx-completion-dependency-gating.md](/root/Poulsbo/docs/34-sgx-completion-dependency-gating.md)
proved that `ta_dependency` is a real wait gate.

That still left one missing piece:

- the dependency was only an implicit global completion-ticket threshold
- there was still no way to say “submit B depends on submit A”
- future fence-like work would be harder to reason about if dependency state
  stayed anonymous

This step makes the dependency source explicit without changing the current
validate-only execution model.

## Current Kernel Semantics

The Ubuntu 5.4 lab driver now also tracks:

- `dependency_submit_id`

on each persistent SGX lab submit.

Current rules:

- `dependency_submit_id=0` keeps the earlier global completion-ticket behavior
- `depend <submit_id> <dependency_submit_id>` binds one submit to another live
  submit
- self-dependency is rejected
- a missing dependency source is rejected at configuration and validation time
- if both `ta_dependency` and `dependency_submit_id` are set, then
  `queued -> running` requires:
  - the dependency submit to still exist
  - the dependency submit's `global_complete_ticket` to satisfy
    `ta_dependency`
- active running-owner arbitration still applies separately and still returns
  `-EBUSY`

So the current lab model can now express both:

- “wait for completion ticket N”
- “wait for completion ticket N from submit X”

## Helper Changes

The concurrent validator:

- `scripts/run-vaio-sgx-multi-submit.sh`

now configures submit B in two layers:

- `kick ... ta_dependency=<expected submit A completion ticket>`
- `depend <submit_b_id> <submit_a_id>`

That helper now verifies three states on hardware:

1. submit B waits on submit A specifically before any submit A progress exists
2. submit B still cannot enter `running` while submit A owns the active running
   lane
3. submit B proceeds normally once submit A reaches the required completion
   ticket

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026.

The validated override module was:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 05DE875B8F21073BDD04B44`

### Submit B Setup State

After setup, submit B showed:

- `ta_dependency=3`
- `dependency_submit_id=1`
- `owner_state=queued`
- `queue_epoch=0`
- `global_issue_ticket=0`
- `global_running_ticket=0`
- `global_complete_ticket=0`
- `validate_count=1`

So the dependency is no longer anonymous; it is bound to submit A directly.

### Dependency Wait Before Submit A Runs

Before submit A started, submit B attempted to enter `running` and failed as
intended:

```text
write error: Resource temporarily unavailable
```

Immediately afterward, submit B still showed:

- `dependency_submit_id=1`
- `owner_state=queued`
- `global_issue_ticket=0`
- `global_running_ticket=0`
- `global_complete_ticket=0`

The shared coordination node still showed:

- `issue_ticket_counter=0`
- `running_ticket_counter=0`
- `complete_ticket_counter=0`
- `active_running_valid=0`

So the dependency wait is now explicitly tied to submit A, and it still blocks
before any shared running or completion state advances.

### Active-Owner Gate While Submit A Was Running

After submit A entered `running`, the helper attempted to enter `running` on
submit B again. That second attempt failed with the expected owner-arbitration
error:

```text
write error: Device or resource busy
```

At that moment submit B still remained unchanged:

- `dependency_submit_id=1`
- `owner_state=queued`
- `global_issue_ticket=0`
- `global_running_ticket=0`
- `global_complete_ticket=0`

So the repo now distinguishes cleanly between:

- dependency wait on submit A completion: `-EAGAIN`
- active running-owner contention: `-EBUSY`

### Post-Dependency Progression

After submit A completed, it showed:

- `ta_dependency=0`
- `dependency_submit_id=0`
- `global_issue_ticket=3`
- `global_running_ticket=3`
- `global_complete_ticket=3`
- `validate_count=11`
- `last_validate_status=0`

At that point submit B's dependency was satisfied and its normal run completed
cleanly. Final submit B state after userspace exit:

- `ta_dependency=3`
- `dependency_submit_id=1`
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
- target health: `overall=remote_usable`

## Conclusion

The validate-only SGX lab path now has an explicit source-submit dependency
model, not just a raw global completion-ticket threshold.

That is a better base for future fence-like or queue-ownership work because the
dependency now carries:

- a required completion ticket
- a concrete prerequisite submit
- a separate active-owner arbitration result

## Next Step

The next useful questions are:

- whether one dependency source plus one completion ticket is enough for future
  SGX queue/fence semantics
- whether dependency state should stay in the current submit metadata or move
  into a more explicit state block before any real SGX execution attempt
