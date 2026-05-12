# SGX Multi-Source Dependency Gating

This note records the first hardware-validated case where one dependent submit
waits on two concrete source submits at once through two separate dependency
lanes.

## Why This Step Exists

[docs/40-sgx-unrelated-submit-stability.md](/root/Poulsbo/docs/40-sgx-unrelated-submit-stability.md)
proved that the single dependency lane stays pinned to its source submit even
after unrelated later completions.

That answered the source-specificity question, but not the next structural one:

- is one dependency lane enough once one submit needs to wait on more than one
  concrete source submit?

This step answers the first half of that on real hardware by adding a second
dependency lane with the same completion-ticket threshold.

## Current Semantics

The Ubuntu 5.4 SGX lab path now carries two independent dependency snapshots on
one submit:

- `dependency_*`
- `dependency2_*`

Both currently share the same `ta_dependency` threshold, but each lane binds to
its own source submit and maintains its own:

- source submit ID
- required ticket
- observed completion ticket
- dependency flags

The dependent submit remains blocked until both lanes reach the required
completion ticket.

## Kernel And Helper Changes

Kernel-side:

- added a second submit-side dependency lane in
  [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
- added new sysfs commands:
  - `depend2 <submit_id> <dependency_submit_id>`
  - `depstate2 <submit_id> <offset>`
- extended the live submit view with:
  - `dependency2_submit_id`
  - `dependency2_offset`
  - `dependency2_snapshot_source_submit_id`
  - `dependency2_required_ticket`
  - `dependency2_observed_complete_ticket`
  - `dependency2_flags`
  - `dependency2_config_count`

Helper-side:

- extended
  [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
  with `RUN_SECONDARY_DEPENDENCY_SOURCE=1`
- added the dedicated wrapper
  [scripts/run-vaio-sgx-multi-source-dependency.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-source-dependency.sh)

That wrapper drives:

1. source submit A
2. dependent submit B
3. source submit C

with submit B bound to both A and C.

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026.

The validated override module is now:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion F0B85EC0EA42E7360092BD1`

The validated run used:

- source submit A:
  - `id=1`
- dependent submit B:
  - `id=2`
- source submit C:
  - `id=3`

The dependent submit used:

- primary dependency block offset:
  - `0x60`
- secondary dependency block offset:
  - `0x70`
- shared completion-ticket threshold:
  - `1`

### Initial Two-Lane State

After `depstate` and `depstate2`, submit B showed:

- primary lane:
  - `dependency_submit_id=1`
  - `dependency_observed_complete_ticket=0`
  - `dependency_flags=0x0b`
- secondary lane:
  - `dependency2_submit_id=3`
  - `dependency2_observed_complete_ticket=0`
  - `dependency2_flags=0x0b`
- `dependency_config_count=1`
- `dependency2_config_count=1`
- `validate_count=3`
- `last_validate_status=0`

So both lanes started in the same waiting state, but against different source
submits.

### State After Source Submit A Completes

After submit A completed, submit B showed:

- primary lane released:
  - `dependency_observed_complete_ticket=1`
  - `dependency_flags=0x0f`
- secondary lane still waiting:
  - `dependency2_observed_complete_ticket=0`
  - `dependency2_flags=0x0b`
- `validate_count=5`
- `last_validate_status=0`

An immediate `running` attempt on submit B still failed with
`Resource temporarily unavailable`.

That is the key result in the middle of the test:

- one lane had already satisfied the gate
- the other lane had not
- submit B still did not enter `running`

### State After Source Submit C Completes

After submit C completed, submit B showed:

- primary lane still released:
  - `dependency_observed_complete_ticket=1`
  - `dependency_flags=0x0f`
- secondary lane now released too:
  - `dependency2_observed_complete_ticket=2`
  - `dependency2_flags=0x0f`
- `validate_count=7`
- `last_validate_status=0`

So the two dependency lanes remained distinct and advanced from different
source-submit completion tickets.

### Dependent Submit Then Runs Normally

With both lanes released, submit B then ran normally:

- after `running`:
  - `owner_state=running`
  - `global_issue_ticket=3`
  - `global_running_ticket=3`
  - `dependency_flags=0x0f`
  - `dependency2_flags=0x0f`
  - `validate_count=8`
- after `complete`:
  - `owner_state=complete`
  - `global_issue_ticket=3`
  - `global_running_ticket=3`
  - `global_complete_ticket=3`
  - `queue_packet_count=0`
  - `validate_count=9`
- after probe exit:
  - `owner_state=complete`
  - `global_complete_ticket=3`
  - `dependency_observed_complete_ticket=1`
  - `dependency2_observed_complete_ticket=2`
  - `validate_count=11`
  - `last_validate_status=0`

### Coordination State

The global coordination node progressed cleanly through the three-submit run:

- after setup:
  - `issue_ticket_counter=0`
  - `running_ticket_counter=0`
  - `complete_ticket_counter=0`
  - `live_submit_count=3`
- after submit A:
  - `issue_ticket_counter=1`
  - `running_ticket_counter=1`
  - `complete_ticket_counter=1`
  - `last_issue_submit_id=1`
  - `last_running_submit_id=1`
  - `last_complete_submit_id=1`
- after submit C:
  - `issue_ticket_counter=2`
  - `running_ticket_counter=2`
  - `complete_ticket_counter=2`
  - `last_issue_submit_id=3`
  - `last_running_submit_id=3`
  - `last_complete_submit_id=3`
- after submit B:
  - `issue_ticket_counter=3`
  - `running_ticket_counter=3`
  - `complete_ticket_counter=3`
  - `last_issue_submit_id=2`
  - `last_running_submit_id=2`
  - `last_complete_submit_id=2`
- after cleanup:
  - `live_submit_count=0`
  - `focused_submit_valid=0`

### Final Target State

Post-run target state stayed clean:

- `overall=remote_usable`
- `lightdm=active`
- `fault_count=0`
- `bif_fault=0x000000`

## Conclusion

The current validate-only dependency model now proves a concrete two-source
case:

- one submit can wait on two concrete source submits
- the two lanes keep separate observed completion tickets
- release of one lane is not enough to let the dependent submit run
- the dependent submit only enters `running` after both lanes satisfy the gate

That means the “one dependency lane only” limitation is now lifted for the
shared-ticket case.

## Next Step

The next useful dependency question is narrower now:

- whether later render work needs more than one wait ticket threshold per
  submit, not just more than one source submit
