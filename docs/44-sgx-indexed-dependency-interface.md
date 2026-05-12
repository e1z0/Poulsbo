# SGX Indexed Dependency Interface

This note records the first hardware-validated run of the SGX dependency model
through generic indexed slot commands instead of the older per-lane submit
verbs.

## Why This Step Exists

[docs/43-sgx-third-dependency-lane.md](/root/Poulsbo/docs/43-sgx-third-dependency-lane.md)
proved that three concrete source-submit lanes can gate one dependent submit on
real hardware.

That still left an interface problem:

- the lab path only exposed lane-specific commands like `depend2`, `depend3`,
  `depticket3`, and `depstate3`

That was enough to prove the model, but it was the wrong long-term shape.
Before considering any broader array or fence block, the next practical step
was to prove that the same hardware behavior can be driven through one indexed
surface:

- `depend_slot`
- `depticket_slot`
- `depstate_slot`

## Current Semantics

The Ubuntu 5.4 SGX lab path now accepts both:

- the older compatibility aliases:
  - `depend`
  - `depend2`
  - `depend3`
  - `depticket2`
  - `depticket3`
  - `depstate`
  - `depstate2`
  - `depstate3`
- and the new indexed commands:
  - `depend_slot <submit_id> <slot> <dependency_submit_id>`
  - `depticket_slot <submit_id> <slot> <required_ticket>`
  - `depstate_slot <submit_id> <slot> <offset>`

User slot numbering is `1..3`, mapped onto the existing three submit-side
dependency lanes.

## Kernel And Helper Changes

Kernel-side:

- exposed the indexed dependency-slot helpers already present in
  [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
  through the live sysfs parser
- added parser support for:
  - `depend_slot`
  - `depticket_slot`
  - `depstate_slot`
- updated the SGX submit help text on the PCI sysfs nodes so the indexed
  commands are discoverable on-target

Helper-side:

- extended
  [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
  with `USE_INDEXED_DEPENDENCIES=1`
- added the dedicated wrapper
  [scripts/run-vaio-sgx-indexed-dependency-array.sh](/root/Poulsbo/scripts/run-vaio-sgx-indexed-dependency-array.sh)
- fixed one runner bug from the first rerun attempt by exporting the primary
  dependency ticket into the remote shell before the indexed commands were used

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026.

The validated override module is now:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 381C2A7C9AB93F0C3220005`

The validated wrapper reported:

- `DEPENDENCY_INTERFACE=indexed`

The validated run used:

- source submit A:
  - `id=5`
- dependent submit B:
  - `id=6`
- source submit C:
  - `id=7`
- source submit D:
  - `id=8`

The configured slot thresholds were:

- slot 1:
  - required ticket `1`
  - offset `0x60`
- slot 2:
  - required ticket `2`
  - offset `0x70`
- slot 3:
  - required ticket `3`
  - offset `0xe0`

### Indexed Slot Setup

After `depticket_slot 6 1 1`, submit B showed:

- `dependency_submit_id=5`
- `ta_dependency=1`
- `dependency_offset=0x0`
- `dependency_required_ticket=0`
- `dependency_flags=0x0`
- `validate_count=2`

So slot 1 ticket programming is now explicit through the generic interface
before the state block is configured.

After `depstate_slot 6 1 0x60`, submit B showed:

- `dependency_offset=0x60`
- `dependency_snapshot_source_submit_id=5`
- `dependency_required_ticket=1`
- `dependency_observed_complete_ticket=0`
- `dependency_flags=0x0b`
- `dependency_config_count=1`
- `validate_count=3`

After `depticket_slot 6 2 2` and `depstate_slot 6 2 0x70`, submit B showed:

- `dependency2_submit_id=7`
- `dependency2_offset=0x70`
- `dependency2_required_ticket=2`
- `dependency2_observed_complete_ticket=0`
- `dependency2_flags=0x0b`
- `dependency2_config_count=1`
- `validate_count=5`

After `depticket_slot 6 3 3` and `depstate_slot 6 3 0xe0`, submit B showed:

- `dependency3_submit_id=8`
- `dependency3_offset=0xe0`
- `dependency3_required_ticket=3`
- `dependency3_observed_complete_ticket=0`
- `dependency3_flags=0x0b`
- `dependency3_config_count=1`
- `validate_count=7`

So all three lanes are now configured through one indexed slot surface, not
through three bespoke command families.

### Dependency Wait Semantics

Before any source submit completed, submit B still failed `running` with:

- `Resource temporarily unavailable`

After submit A completed, submit B showed:

- slot 1 released:
  - `dependency_observed_complete_ticket=1`
  - `dependency_flags=0x0f`
- slot 2 still gated:
  - `dependency2_observed_complete_ticket=0`
  - `dependency2_flags=0x0b`
- slot 3 still gated:
  - `dependency3_observed_complete_ticket=0`
  - `dependency3_flags=0x0b`
- `validate_count=9`

An immediate `running` attempt still failed with
`Resource temporarily unavailable`.

After submit C completed, submit B showed:

- slot 1 still released:
  - `dependency_flags=0x0f`
- slot 2 released:
  - `dependency2_observed_complete_ticket=2`
  - `dependency2_flags=0x0f`
- slot 3 still gated:
  - `dependency3_observed_complete_ticket=0`
  - `dependency3_required_ticket=3`
  - `dependency3_flags=0x0b`
- `validate_count=11`

Another `running` attempt still failed with
`Resource temporarily unavailable`.

After submit D completed, submit B showed:

- slot 1 released:
  - `dependency_flags=0x0f`
- slot 2 released:
  - `dependency2_flags=0x0f`
- slot 3 released:
  - `dependency3_observed_complete_ticket=3`
  - `dependency3_flags=0x0f`
- `validate_count=13`
- `last_validate_status=0`

So the indexed slot interface preserves exactly the staged A/C/D release
behavior that the earlier fixed-lane wrapper validated.

### Dependent Submit Then Runs Normally

With all three slots released, submit B then ran normally:

- after probe exit:
  - `owner_state=complete`
  - `dependency_observed_complete_ticket=1`
  - `dependency2_observed_complete_ticket=2`
  - `dependency3_observed_complete_ticket=3`
  - `dependency_flags=0x0f`
  - `dependency2_flags=0x0f`
  - `dependency3_flags=0x0f`
  - `global_issue_ticket=4`
  - `global_running_ticket=4`
  - `global_complete_ticket=4`
  - `validate_count=17`
  - `last_validate_status=0`

### Coordination State

The coordination node progressed cleanly:

- after submit A:
  - `issue/running/complete = 1/1/1`
- after submit C:
  - `issue/running/complete = 2/2/2`
- after submit D:
  - `issue/running/complete = 3/3/3`
- after submit B:
  - `issue/running/complete = 4/4/4`

After cleanup, the live target reported:

- `live_submit_count=0`
- `active_running_valid=0`

### Final Target State

Post-run target state stayed clean:

- `overall=remote_usable`
- `fault_count=0`
- `bif_fault=0x000000`
- `issue_ticket_counter=4`
- `running_ticket_counter=4`
- `complete_ticket_counter=4`

## Conclusion

The important result is no longer only that three lanes work.

The important result is that:

- the three-lane dependency model is now hardware-validated through one indexed
  slot interface
- the fixed-lane sysfs verbs can remain compatibility aliases instead of being
  the only control surface
- the next dependency question is no longer “can we generalize beyond
  `depend3`?” but whether three indexed slots are enough before the lab path
  needs a broader dependency array or fence block
