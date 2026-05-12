# SGX Fourth Indexed Dependency Slot

This note records the first hardware-validated case where the indexed SGX
dependency interface gates one dependent submit on four concrete source submits
at once.

## Why This Step Exists

[docs/44-sgx-indexed-dependency-interface.md](/root/Poulsbo/docs/44-sgx-indexed-dependency-interface.md)
proved that the generic slot interface works for slots `1`, `2`, and `3`.

That answered the immediate parser/interface question, but not the structural
one that followed:

- does the indexed surface really let the model grow without inventing another
  `depend4`-style command family?

This step answers that directly on real hardware by adding a fourth submit-side
dependency slot that is reachable only through:

- `depend_slot`
- `depticket_slot`
- `depstate_slot`

There is no `depend4` compatibility alias in this milestone.

## Current Semantics

The Ubuntu 5.4 SGX lab path now carries four submit-side dependency snapshots
on one dependent submit:

- `dependency_*`
- `dependency2_*`
- `dependency3_*`
- `dependency4_*`

All four lanes are controlled through the indexed sysfs interface. Slot `4`
inherits the same model as the earlier lanes:

- concrete source submit ID
- required completion ticket
- observed completion ticket
- dependency flags
- status-object snapshot backing

The dependent submit remains blocked until all four lanes release.

## Kernel And Helper Changes

Kernel-side:

- widened `PSB_SGX_LAB_DEPENDENCY_SLOTS` from `3` to `4`
- added fourth-lane submit fields in
  [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
  for:
  - `dependency_ticket_quaternary`
  - `dependency_submit_id_quaternary`
  - `dependency_offset_quaternary`
  - `dependency_snapshot_*_quaternary`
  - `dependency_config_count_quaternary`
  - `last_dependency_*_quaternary`
- extended the generic slot accessors, validation path, and focused submit view
  so slot `4` participates in the same loop-based machinery as slots `1..3`

Helper-side:

- extended
  [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
  with:
  - `RUN_QUATERNARY_DEPENDENCY_SOURCE`
  - `SUBMIT_E_QUEUE_MODE`
  - `SUBMIT_E_REFILL_CYCLES`
  - `SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET`
  - `SUBMIT_B_DEPENDENCY4_OFFSET`
- required `USE_INDEXED_DEPENDENCIES=1` for the new fourth-slot path
- added the dedicated wrapper
  [scripts/run-vaio-sgx-fourth-dependency-slot.sh](/root/Poulsbo/scripts/run-vaio-sgx-fourth-dependency-slot.sh)

That wrapper drives:

1. source submit A
2. source submit C
3. source submit D
4. source submit E
5. dependent submit B only after all four lanes release

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026.

The validated override module is now:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 9CDF8E2749FF727D47C0BA4`

The validated wrapper reported:

- `DEPENDENCY_INTERFACE=indexed`

The validated run used:

- source submit A:
  - `id=1`
- dependent submit B:
  - `id=2`
- source submit C:
  - `id=3`
- source submit D:
  - `id=4`
- source submit E:
  - `id=5`

The configured thresholds were:

- slot 1:
  - required ticket `1`
  - offset `0x60`
- slot 2:
  - required ticket `2`
  - offset `0x70`
- slot 3:
  - required ticket `3`
  - offset `0xe0`
- slot 4:
  - required ticket `4`
  - offset `0x100`

### Fourth Slot Setup

After `depticket_slot 2 4 4`, submit B showed:

- `dependency4_submit_id=5`
- `dependency4_offset=0x0`
- `dependency4_required_ticket=0`
- `dependency4_observed_complete_ticket=0`
- `dependency4_flags=0x0`
- `dependency4_config_count=0`
- `validate_count=8`

So slot `4` ticket programming is explicit through the indexed surface before
the dependency state block is configured.

After `depstate_slot 2 4 0x100`, submit B showed:

- `dependency4_submit_id=5`
- `dependency4_offset=0x100`
- `dependency4_snapshot_source_submit_id=5`
- `dependency4_required_ticket=4`
- `dependency4_observed_complete_ticket=0`
- `dependency4_flags=0x0b`
- `dependency4_config_count=1`
- `validate_count=9`

That proves the fourth lane is not a parser alias trick. It has its own real
submit-side state and status-object backing.

### Dependency Wait Semantics

The first three slots behaved as before:

- after submit A:
  - slot 1 released
- after submit C:
  - slot 2 released
- after submit D:
  - slot 3 released

The key new result is what happened after submit D:

- slot 4 still waited:
  - `dependency4_required_ticket=4`
  - `dependency4_observed_complete_ticket=0`
  - `dependency4_flags=0x0b`

At that point, submit B still failed `running` with:

- `Resource temporarily unavailable`

So even after slots `1..3` had released, the new fourth slot still held the
dependent submit in the gated state.

### State After Submit E Completes

After submit E completed, submit B showed:

- slot 1 released:
  - `dependency_observed_complete_ticket=1`
  - `dependency_flags=0x0f`
- slot 2 released:
  - `dependency2_observed_complete_ticket=2`
  - `dependency2_flags=0x0f`
- slot 3 released:
  - `dependency3_observed_complete_ticket=3`
  - `dependency3_flags=0x0f`
- slot 4 released:
  - `dependency4_observed_complete_ticket=4`
  - `dependency4_flags=0x0f`
- `validate_count=17`
- `last_validate_status=0`

So the indexed four-slot model stayed gated until the fourth concrete source
submit released.

### Dependent Submit Then Runs Normally

With all four slots released, submit B then ran normally.

After probe exit, submit B showed:

- `owner_state=complete`
- `dependency_observed_complete_ticket=1`
- `dependency2_observed_complete_ticket=2`
- `dependency3_observed_complete_ticket=3`
- `dependency4_observed_complete_ticket=4`
- `dependency_flags=0x0f`
- `dependency2_flags=0x0f`
- `dependency3_flags=0x0f`
- `dependency4_flags=0x0f`
- `global_issue_ticket=5`
- `global_running_ticket=5`
- `global_complete_ticket=5`
- `dependency4_config_count=1`
- `validate_count=21`
- `last_validate_status=0`

### Source Submit E State

The new source submit also validated cleanly after probe exit:

- submit E:
  - `owner_state=complete`
  - `global_issue_ticket=4`
  - `global_running_ticket=4`
  - `global_complete_ticket=4`
  - `validate_count=5`
  - `last_validate_status=0`

### Coordination State

The coordination node progressed cleanly:

- after submit A:
  - `issue/running/complete = 1/1/1`
- after submit C:
  - `issue/running/complete = 2/2/2`
- after submit D:
  - `issue/running/complete = 3/3/3`
- after submit E:
  - `issue/running/complete = 4/4/4`
- after submit B:
  - `issue/running/complete = 5/5/5`

After cleanup, the live target reported:

- `active_running_valid=0`
- `live_submit_count=0`

### Final Target State

Post-run target state stayed clean:

- `overall=remote_usable`
- `fault_count=0`
- `bif_fault=0x000000`
- `issue_ticket_counter=5`
- `running_ticket_counter=5`
- `complete_ticket_counter=5`

## Conclusion

The important result is not just “a fourth lane works.”

The important result is:

- the indexed dependency interface now scales past the old fixed-lane alias set
- slot `4` is validated on hardware without adding a `depend4` compatibility
  verb
- the next dependency question is no longer whether indexed growth is possible
  at all, but whether the model should stop at four slots or move to a broader
  dependency array or fence block
