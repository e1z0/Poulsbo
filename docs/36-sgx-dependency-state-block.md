# SGX Dependency-State Block

This note records the first hardware-validated dependency snapshot block in the
status object for persistent SGX lab submits on the Ubuntu 5.4 Poulsbo target.

## Why This Step Exists

The previous milestone in [docs/35-sgx-source-submit-dependency.md](/root/Poulsbo/docs/35-sgx-source-submit-dependency.md)
made the dependency source explicit through `dependency_submit_id`.

That still left one gap:

- dependency state only lived in submit metadata
- there was no explicit memory-backed block showing whether the dependency was
  still waiting or already satisfied
- future fence-like work would be easier to reason about if dependency state
  had an explicit status-object layout like sync and owner already do

This step adds that explicit block without attempting any real SGX execution.

## Current Kernel Semantics

The Ubuntu 5.4 lab driver now supports:

- `depstate <submit_id> <offset>`

on `poulsbo_sgx_submit`.

That writes a four-dword dependency snapshot into the submit's `status` object:

1. source submit ID
2. required completion ticket
3. observed completion ticket
4. dependency flags

Current flags:

- `0x1` `READY`
- `0x2` `SOURCE_BOUND`
- `0x4` `SATISFIED`
- `0x8` `GATED`

For the current submit B dependency on submit A:

- `0xb` means `READY | SOURCE_BOUND | GATED`
- `0xf` means `READY | SOURCE_BOUND | GATED | SATISFIED`

The submit view now also exposes:

- `dependency_offset`
- `dependency_snapshot_source_submit_id`
- `dependency_required_ticket`
- `dependency_observed_complete_ticket`
- `dependency_flags`
- `dependency_config_count`
- `last_dependency_block_jiffies`

Validation now readbacks that block from the status object and requires it to
match the current stored snapshot.

## Helper Changes

The concurrent validator:

- `scripts/run-vaio-sgx-multi-submit.sh`

now refreshes submit B's dependency block at three points:

1. immediately after setup and source binding
2. immediately after the expected dependency-wait failure before submit A runs
3. immediately after submit A reaches the required completion ticket

Each refresh also runs `validate` so the new block is read back and checked on
real hardware.

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026.

The validated override module was:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 243EBC2DB6AC1F1149CA3AB`

### Initial Dependency Snapshot

After setup, submit B showed:

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

So the explicit block started in the expected unsatisfied state.

### Dependency Wait Before Submit A Runs

Before submit A started, submit B attempted to enter `running` and failed as
intended:

```text
write error: Resource temporarily unavailable
```

The helper then refreshed submit B's dependency block again. It still showed:

- `dependency_snapshot_source_submit_id=1`
- `dependency_required_ticket=3`
- `dependency_observed_complete_ticket=0`
- `dependency_flags=0xb`
- `dependency_config_count=2`
- `validate_count=3`
- `last_validate_status=0`

So the block stayed unsatisfied while the dependency gate was still closed.

### Dependency Snapshot After Submit A Released The Gate

After submit A finished its required completion path, submit B's dependency
block was refreshed a third time and showed:

- `dependency_snapshot_source_submit_id=1`
- `dependency_required_ticket=3`
- `dependency_observed_complete_ticket=3`
- `dependency_flags=0xf`
- `dependency_config_count=3`
- `validate_count=4`
- `last_validate_status=0`

Submit B still remained queued at that point, so the block now explicitly shows
the difference between:

- dependency satisfied
- submit not yet issued/running

### Later Submit B Progression

After the dependency block flipped to satisfied, submit B proceeded normally:

- `queued -> running -> complete` still validated cleanly
- the refill cycle also stayed clean
- final submit B post-exit state showed:
  - `dependency_snapshot_source_submit_id=1`
  - `dependency_required_ticket=3`
  - `dependency_observed_complete_ticket=3`
  - `dependency_flags=0xf`
  - `dependency_config_count=3`
  - `validate_count=11`
  - `last_validate_status=0`

Final submit A post-exit state still showed:

- `global_issue_ticket=3`
- `global_running_ticket=3`
- `global_complete_ticket=3`
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

The validate-only SGX lab path now has an explicit dependency-state block in
memory, not just dependency metadata in the submit descriptor.

That is a better base for future fence-like work because the repo now has:

- a concrete dependency source submit
- a required completion ticket
- an observed completion ticket
- an explicit satisfied/unsatisfied flag transition in the status object

## Next Step

The next useful questions are:

- whether one explicit dependency block is enough, or whether later queue/fence
  semantics need more than one dependency source or wait lane
- whether dependency snapshots should stay explicit-refresh lab state or grow
  into a more live-updated fence/dependency model before any real SGX execution
  attempt
