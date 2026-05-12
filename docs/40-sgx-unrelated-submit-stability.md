# SGX Unrelated Submit Stability

This note records the first hardware-validated case where a satisfied-and-missing
source dependency stays pinned while an unrelated submit completes afterward.

## Why This Step Exists

[docs/39-sgx-satisfied-source-drop-state.md](/root/Poulsbo/docs/39-sgx-satisfied-source-drop-state.md)
proved that a dependent submit can still run after its bound source submit
first satisfies the dependency and is then dropped.

That left the next question:

- once the source is gone, does an unrelated later submit accidentally move the
  dependent submit's observed completion ticket, or does the dependency
  snapshot stay source-specific?

This step answers that on real hardware.

## Current Semantics

The current validate-only dependency model now has a stronger guarantee:

- after a bound source submit satisfies the dependency and is then dropped,
  the dependent submit moves to `dependency_flags=0x1f`
- if an unrelated submit later completes, that dependent submit keeps the same
  `dependency_snapshot_source_submit_id`
  and the same `dependency_observed_complete_ticket`

So the missing-source satisfied state is not merely runnable; it is also stable
against later unrelated queue activity.

## Helper Changes

The multi-submit helper:

- [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)

now supports:

- `RUN_UNRELATED_AFTER_SATISFIED_DROP=1`

and the dedicated wrapper:

- [scripts/run-vaio-sgx-dependency-unrelated-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-dependency-unrelated-submit.sh)

drives the three-submit case directly.

This step also hardened the helper itself:

- it now does a bounded stale submit/object ID sweep before setup, because the
  bulk `poulsbo_sgx_objects` dump truncates once too many live objects exist
- it now derives adopted object IDs from the first adopted object plus the
  known sequential adopt order, instead of relying on the truncated bulk object
  view for complete object-to-range mapping

That makes the three-submit target path reproducible even after aborted runs.

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026.

The validated override module remained:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 662C941AA2B250ACFFEDB1F`

The validated rerun used:

- source submit `id=7`
- dependent submit `id=8`
- unrelated submit `id=9`

### Satisfied Source-Drop Baseline

After the bound source submit released the dependency, the dependent submit
showed:

- `dependency_snapshot_source_submit_id=7`
- `dependency_required_ticket=1`
- `dependency_observed_complete_ticket=3`
- `dependency_flags=0x0f`
- `validate_count=4`

After dropping that already satisfied source submit, the dependent submit
showed:

- `dependency_snapshot_source_submit_id=7`
- `dependency_required_ticket=1`
- `dependency_observed_complete_ticket=3`
- `dependency_flags=0x1f`
- `validate_count=5`

So the source-drop baseline matched the earlier milestone, with the current
global ticket values on this target.

### State After Unrelated Submit Completion

The unrelated submit then ran to completion, advancing the global counters to:

- `issue_ticket_counter=4`
- `running_ticket_counter=4`
- `complete_ticket_counter=4`
- `last_issue_submit_id=9`
- `last_running_submit_id=9`
- `last_complete_submit_id=9`

After that unrelated completion, the dependent submit still showed:

- `dependency_snapshot_source_submit_id=7`
- `dependency_required_ticket=1`
- `dependency_observed_complete_ticket=3`
- `dependency_flags=0x1f`
- `dependency_config_count=1`
- `validate_count=6`
- `last_validate_status=0`

That is the key result:

- the unrelated submit advanced the global completion counter
- the dependent submit's observed completion ticket stayed pinned to the
  original source submit's satisfied value
- the dependency snapshot did not drift to the unrelated submit's completion

### Dependent Submit Still Runs

With that unchanged `0x1f` state after the unrelated completion, the dependent
submit still ran normally:

- after `running`:
  - `owner_state=running`
  - `dependency_observed_complete_ticket=3`
  - `dependency_flags=0x1f`
  - `global_issue_ticket=5`
  - `global_running_ticket=5`
- after `complete`:
  - `owner_state=complete`
  - `dependency_observed_complete_ticket=3`
  - `dependency_flags=0x1f`
  - `global_issue_ticket=5`
  - `global_running_ticket=5`
  - `global_complete_ticket=5`
- after probe exit:
  - `owner_state=complete`
  - `dependency_observed_complete_ticket=3`
  - `dependency_flags=0x1f`
  - `queue_packet_count=0`
  - `validate_count=10`
  - `last_validate_status=0`

So the source-specific dependency snapshot stayed pinned even while submit B's
own issue/running/complete tickets advanced beyond the old source ticket.

### Final Target State

After cleanup:

- `submits_shown=0`
- `objects_shown=0`
- `issue_ticket_counter=5`
- `running_ticket_counter=5`
- `complete_ticket_counter=5`
- `active_running_valid=0`
- `focused_submit_valid=0`
- `lightdm=active`
- `fault_count=0`
- `bif_fault=0x000000`
- target health: `overall=remote_usable`

## Conclusion

The current dependency model now has a hardware-validated source-specific
stability property:

- a satisfied source dependency can become missing
- later unrelated submit completions do not perturb the dependent submit's
  observed completion ticket

That means the current single-source dependency snapshot behaves like a pinned
per-source fence state, not like a rolling alias of the driver's global
completion counter.

## Next Step

The next useful dependency questions are:

- whether the current single dependency lane is still enough once more than one
  source submit or more than one wait ticket is needed
- whether later render work needs a broader dependency block with more than one
  bound source or more than one observed completion word
