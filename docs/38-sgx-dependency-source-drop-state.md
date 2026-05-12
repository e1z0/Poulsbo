# SGX Dependency Source-Drop State

This note records the first hardware-validated source-submit lifetime handling
for the Ubuntu 5.4 Poulsbo SGX lab path.

## Why This Step Exists

[docs/37-sgx-live-dependency-block.md](/root/Poulsbo/docs/37-sgx-live-dependency-block.md)
made the dependency block live-updated on source-submit completion.

That still left one real gap:

- a dependent submit could name a concrete `dependency_submit_id`
- the dependency block could become satisfied while the source submit existed
- but if that source submit was dropped, the path still risked collapsing into
  a plain missing-submit error instead of an explicit dependency state

This step makes source-submit disappearance observable in the dependency block.

## Current Kernel Semantics

The Ubuntu 5.4 lab driver now:

- adds `PSB_SGX_LAB_DEPENDENCY_FLAG_SOURCE_MISSING`
- keeps dependency-block refreshes working even after the bound source submit
  is no longer present in the live submit list
- refreshes dependent submit blocks when `drop <submit_id>` removes a source
  submit
- allows validate-only dependency checks to use the existing dependency
  snapshot when the source is gone and a dependency block already exists
- keeps `transition ... running ...` on the dependent submit returning
  `-EAGAIN` when the source is gone and the required completion ticket was
  never reached

The practical dependency-flag combinations are now:

- `0x0b`
  ready + source-bound + gated, but not satisfied
- `0x0f`
  ready + source-bound + gated + satisfied
- `0x1b`
  ready + source-bound + gated + source-missing, but not satisfied

So the dependency block can now say “the source existed, it was required, and
it is now gone before satisfying the gate.”

## Helper Changes

The multi-submit helper:

- [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)

now supports:

- `DROP_SOURCE_BEFORE_RUN=1`

and the dedicated wrapper:

- [scripts/run-vaio-sgx-dependency-drop.sh](/root/Poulsbo/scripts/run-vaio-sgx-dependency-drop.sh)

drives the dependency-source-drop case directly.

That flow now:

1. creates source and dependent submits
2. configures one dependency block with `depstate`
3. proves the initial dependency wait
4. drops the source submit before it ever reaches `running`
5. validates that the dependent submit moves into an explicit
   `source missing` dependency state
6. proves the dependent submit still returns `Resource temporarily unavailable`
   instead of degrading into a plain missing-submit failure

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026.

The validated override module was:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 662C941AA2B250ACFFEDB1F`

The validated rerun used:

- source submit `id=3`
- dependent submit `id=4`

### Initial Dependency Block Setup

After the one-time `depstate` setup, the dependent submit showed:

- `ta_dependency=1`
- `dependency_submit_id=3`
- `dependency_offset=0x60`
- `dependency_snapshot_source_submit_id=3`
- `dependency_required_ticket=1`
- `dependency_observed_complete_ticket=0`
- `dependency_flags=0x0b`
- `dependency_config_count=1`
- `validate_count=2`
- `last_validate_status=0`

### Dependency Wait Before Source Run

Before the source submit ran, the dependent submit attempted `running` and
failed as intended:

```text
write error: Resource temporarily unavailable
```

After that, a plain `validate` still showed:

- `dependency_snapshot_source_submit_id=3`
- `dependency_required_ticket=1`
- `dependency_observed_complete_ticket=0`
- `dependency_flags=0x0b`
- `dependency_config_count=1`
- `validate_count=3`
- `last_validate_status=0`

### State After Source Drop

The source submit was then dropped before entering `running`.

After that drop, the dependent submit showed:

- `dependency_snapshot_source_submit_id=3`
- `dependency_required_ticket=1`
- `dependency_observed_complete_ticket=0`
- `dependency_flags=0x1b`
- `dependency_config_count=1`
- `validate_count=4`
- `last_validate_status=0`

That is the key new result:

- the dependency block stayed bound to the original source submit id
- the required ticket stayed unchanged
- the observed completion ticket stayed `0`
- the new `source missing` bit appeared without marking the dependency as
  satisfied

### Dependency Wait After Source Drop

After the drop, the dependent submit attempted `running` again and still
failed with:

```text
write error: Resource temporarily unavailable
```

A plain `validate` after that still showed:

- `dependency_snapshot_source_submit_id=3`
- `dependency_required_ticket=1`
- `dependency_observed_complete_ticket=0`
- `dependency_flags=0x1b`
- `dependency_config_count=1`
- `validate_count=5`
- `last_validate_status=0`

So the submit path no longer degenerates into a generic missing-source error
when a dependency block exists. It remains a dependency wait, but now with an
explicit missing-source state recorded in memory and in the status object.

### Final Submit And Target State

After probe exit, the dependent submit still validated cleanly:

- `dependency_snapshot_source_submit_id=3`
- `dependency_required_ticket=1`
- `dependency_observed_complete_ticket=0`
- `dependency_flags=0x1b`
- `dependency_config_count=1`
- `validate_count=6`
- `last_validate_status=0`

Coordination state after the source drop showed:

- `issue_ticket_counter=0`
- `running_ticket_counter=0`
- `complete_ticket_counter=0`
- `active_running_valid=0`
- `focused_submit_id=4`
- `live_submit_count=1`

Final post-cleanup target state was:

- `lightdm=active`
- `fault_count=0`
- `bif_fault=0x000000`
- `active_running_valid=0`
- `focused_submit_valid=0`
- `live_submit_count=0`
- `submits_shown=0`
- `objects_shown=0`
- target health: `overall=remote_usable`

## Conclusion

The validate-only SGX lab path now distinguishes:

- a dependency that is merely waiting on an existing source submit
- a dependency that is still waiting, but whose bound source submit has already
  been dropped

That gives the repo a real lifetime signal for source-submit dependencies, not
just a completion signal.

## Next Step

The next useful dependency questions are:

- whether a dependency that was already satisfied should stay runnable after
  the source submit is dropped, and whether that needs a separate validated
  state beyond the current `source missing` bit
- whether later fence-like semantics need more than one dependency lane or
  more than one bound source submit
