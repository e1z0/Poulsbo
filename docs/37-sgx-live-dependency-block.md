# SGX Live Dependency-State Updates

This note records the first hardware-validated live update path for the SGX lab
dependency block on the Ubuntu 5.4 Poulsbo target.

## Why This Step Exists

The previous milestone in [docs/36-sgx-dependency-state-block.md](/root/Poulsbo/docs/36-sgx-dependency-state-block.md)
added an explicit dependency block in the status object.

That still left one practical gap:

- the helper had to call `depstate` again after the dependency wait and again
  after submit A completed
- the block existed, but it was still an explicit-refresh snapshot
- a more useful fence-like model needs dependency state to move when the source
  submit actually makes progress

This step makes the dependency block live-updated when the source submit
reaches a new completion ticket.

## Current Kernel Semantics

The Ubuntu 5.4 lab driver now:

- refreshes an already configured dependency block automatically when a source
  submit reaches `owner_state=complete`
- refreshes that block in-place in the `status` object
- keeps `dependency_config_count` stable after the original `depstate`
  configuration

So the current dependency block now has two phases:

- explicit placement through `depstate <submit_id> <offset>`
- live updates when the prerequisite submit completes

The submit view still exposes:

- `dependency_snapshot_source_submit_id`
- `dependency_required_ticket`
- `dependency_observed_complete_ticket`
- `dependency_flags`
- `dependency_config_count`
- `last_dependency_block_jiffies`

but now those fields can change without another explicit `depstate` command.

## Helper Changes

The concurrent validator:

- `scripts/run-vaio-sgx-multi-submit.sh`

now only does one explicit dependency-block setup:

- `depstate <submit_b_id> <offset>`

After that it no longer refreshes the block explicitly. Instead it only:

1. validates submit B after the expected dependency-wait failure
2. validates submit B again after submit A reaches the required completion
   ticket

That makes the next state change attributable to the kernel update path, not to
another helper-issued refresh.

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026.

The validated override module was:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 6CA61C95B44598D425816B1`

### Initial Dependency Block Setup

After the one-time setup, submit B showed:

- `ta_dependency=3`
- `dependency_submit_id=1`
- `dependency_offset=0x60`
- `dependency_snapshot_source_submit_id=1`
- `dependency_required_ticket=3`
- `dependency_observed_complete_ticket=0`
- `dependency_flags=0xb`
- `dependency_config_count=1`
- `validate_count=2`
- `last_validate_status=0`

So the initial explicit setup still behaved exactly as before.

### Validate After The Dependency Wait

Before submit A ran, submit B attempted `running` and failed as intended:

```text
write error: Resource temporarily unavailable
```

After that, the helper only ran `validate`, not `depstate`, and submit B still
showed:

- `dependency_snapshot_source_submit_id=1`
- `dependency_required_ticket=3`
- `dependency_observed_complete_ticket=0`
- `dependency_flags=0xb`
- `dependency_config_count=1`
- `validate_count=3`
- `last_validate_status=0`

So the dependency block remained unsatisfied, and the config count proved there
was no second explicit refresh.

### Live Update After Submit A Completed

After submit A reached the required completion ticket, the helper again only
ran `validate` on submit B. At that point submit B showed:

- `dependency_snapshot_source_submit_id=1`
- `dependency_required_ticket=3`
- `dependency_observed_complete_ticket=3`
- `dependency_flags=0xf`
- `dependency_config_count=1`
- `validate_count=4`
- `last_validate_status=0`

That is the key result:

- the dependency block flipped from unsatisfied to satisfied
- `dependency_config_count` stayed flat at `1`
- the change came from the kernel completion path, not another helper refresh

Submit B still remained queued at that point, so the block now captures a live
dependency release before submit B itself runs.

### Final Submit State

Final state after userspace exit:

- submit A:
  - `global_issue_ticket=3`
  - `global_running_ticket=3`
  - `global_complete_ticket=3`
  - `validate_count=11`
  - `last_validate_status=0`
- submit B:
  - `dependency_snapshot_source_submit_id=1`
  - `dependency_required_ticket=3`
  - `dependency_observed_complete_ticket=3`
  - `dependency_flags=0xf`
  - `dependency_config_count=1`
  - `validate_count=11`
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
- `focused_submit=none`
- `objects_shown=0`
- target health: `overall=remote_usable`

## Conclusion

The validate-only SGX lab path now has a live-updated dependency block, not
just an explicit-refresh dependency snapshot.

That is a better base for later fence-like work because the repo now has:

- one-time dependency block placement
- source-submit completion driven block updates
- an explicit proof that the block can change without another helper-issued
  refresh command

## Next Step

The next useful questions are:

- whether one live dependency block is enough, or whether later queue/fence
  semantics need more than one wait lane or dependency source
- whether the current completion-driven live update path should grow into a
  broader fence-like model beyond the current single dependency block
