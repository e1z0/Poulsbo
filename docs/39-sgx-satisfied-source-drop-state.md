# SGX Satisfied Source-Drop State

This note records the first hardware-validated case where a dependency is
already satisfied, the bound source submit is then dropped, and the dependent
submit still runs normally.

## Why This Step Exists

[docs/38-sgx-dependency-source-drop-state.md](/root/Poulsbo/docs/38-sgx-dependency-source-drop-state.md)
proved that the driver now records an explicit missing-source state when the
bound source submit disappears before satisfying the dependency gate.

That left the next question:

- if the dependency was already satisfied first, should dropping the source
  submit still leave the dependent submit runnable?

This step answers that on real hardware.

## Current Semantics

The current Ubuntu 5.4 SGX lab path now has a hardware-validated dependency
state progression:

- `0x0b`
  ready + source-bound + gated, but not satisfied
- `0x0f`
  ready + source-bound + gated + satisfied
- `0x1f`
  ready + source-bound + gated + satisfied + source-missing

So the driver now distinguishes:

- waiting on a present source submit
- released by a present source submit
- released by a source submit that has since been dropped

## Helper Changes

The multi-submit helper:

- [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)

now supports:

- `DROP_SOURCE_AFTER_RELEASE=1`

and the dedicated wrapper:

- [scripts/run-vaio-sgx-dependency-satisfied-drop.sh](/root/Poulsbo/scripts/run-vaio-sgx-dependency-satisfied-drop.sh)

drives the satisfied-then-drop case directly.

That flow now:

1. creates source and dependent submits
2. configures one dependency block with `depstate`
3. drives the source submit far enough to satisfy the dependency
4. validates the dependent submit at the satisfied state
5. drops the source submit
6. validates the dependent submit again at the satisfied-plus-missing state
7. proves the dependent submit still enters `running` and reaches `complete`

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026.

The validated override module remained:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 662C941AA2B250ACFFEDB1F`

The validated rerun used:

- source submit `id=5`
- dependent submit `id=6`

### Dependency Release Before Source Drop

After submit A satisfied the gate, the dependent submit showed:

- `dependency_snapshot_source_submit_id=5`
- `dependency_required_ticket=1`
- `dependency_observed_complete_ticket=1`
- `dependency_flags=0x0f`
- `dependency_config_count=1`
- `validate_count=4`
- `last_validate_status=0`

So the dependency was satisfied while the source submit still existed.

### State After Satisfied Source Drop

The source submit was then dropped.

After that drop, the dependent submit showed:

- `dependency_snapshot_source_submit_id=5`
- `dependency_required_ticket=1`
- `dependency_observed_complete_ticket=1`
- `dependency_flags=0x1f`
- `dependency_config_count=1`
- `validate_count=5`
- `last_validate_status=0`

That is the key result:

- the dependency remained satisfied
- the source-missing bit was added
- the observed completion ticket remained pinned at the satisfied value

### Dependent Submit Still Runs

With that `0x1f` state, the dependent submit still ran normally:

- after `running`:
  - `owner_state=running`
  - `dependency_flags=0x1f`
  - `global_issue_ticket=2`
  - `global_running_ticket=2`
  - `validate_count=6`
- after `complete`:
  - `owner_state=complete`
  - `dependency_flags=0x1f`
  - `global_issue_ticket=2`
  - `global_running_ticket=2`
  - `global_complete_ticket=2`
  - `validate_count=7`
- after probe exit:
  - `owner_state=complete`
  - `dependency_flags=0x1f`
  - `queue_packet_count=0`
  - `validate_count=9`
  - `last_validate_status=0`

So a satisfied dependency stays runnable even after its source submit is gone.

### Coordination State

After the satisfied source drop, the coordination node showed:

- `issue_ticket_counter=1`
- `running_ticket_counter=1`
- `complete_ticket_counter=1`
- `last_issue_submit_id=5`
- `last_running_submit_id=5`
- `last_complete_submit_id=5`
- `active_running_valid=0`
- `live_submit_count=1`

After submit B completed, the coordination node showed:

- `issue_ticket_counter=2`
- `running_ticket_counter=2`
- `complete_ticket_counter=2`
- `last_issue_submit_id=6`
- `last_running_submit_id=6`
- `last_complete_submit_id=6`
- `active_running_valid=0`
- `live_submit_count=1`

Final post-cleanup target state was:

- `lightdm=active`
- `fault_count=0`
- `bif_fault=0x000000`
- `submits_shown=0`
- `objects_shown=0`
- target health: `overall=remote_usable`

## Conclusion

The current validate-only dependency model now has a clean lifetime story for
one bound source submit:

- unsatisfied and present
- satisfied and present
- satisfied and then dropped

The last state is explicitly represented as `dependency_flags=0x1f`, and it is
still runnable.

## Next Step

The next useful dependency questions are:

- whether the current single dependency lane is still enough once more than one
  source submit or more than one wait ticket is needed
- whether the observed completion ticket should remain source-specific if other
  unrelated submits complete after the source has already been dropped
