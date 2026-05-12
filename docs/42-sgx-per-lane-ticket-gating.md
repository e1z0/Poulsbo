# SGX Per-Lane Ticket Gating

This note records the first hardware-validated case where the second dependency
lane uses a stricter completion-ticket threshold than the first lane.

## Why This Step Exists

[docs/41-sgx-multi-source-dependency-gating.md](/root/Poulsbo/docs/41-sgx-multi-source-dependency-gating.md)
proved that one dependent submit can wait on two concrete source submits at the
same time.

That still left one important limitation:

- both lanes were using the same `ta_dependency` threshold

So the next useful question was:

- can one lane stay gated after its source submit has already completed once,
  while another lane is already released?

This step answers that on real hardware by giving lane 2 its own required
ticket.

## Current Semantics

The Ubuntu 5.4 SGX lab path now supports:

- the original primary lane threshold through `ta_dependency`
- a second lane-specific threshold through `depticket2`

The second lane still writes its state into `dependency2_*`, but it no longer
has to inherit the primary lane's required ticket.

## Kernel And Helper Changes

Kernel-side:

- added `dependency_ticket_secondary` in
  [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
- added the new sysfs command:
  - `depticket2 <submit_id> <required_ticket>`
- updated the dependency snapshot and gate logic so lane 2 can use its own
  required ticket while lane 1 still uses `ta_dependency`

Helper-side:

- extended
  [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
  with:
  - `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET`
- added the dedicated wrapper:
  - [scripts/run-vaio-sgx-multi-source-ticket-dependency.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-source-ticket-dependency.sh)

That wrapper drives:

1. submit A once
2. submit C twice
3. submit B only after both dependency lanes release

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026.

The validated override module is now:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 22E5E2738069346A4881DE7`

The validated run used:

- source submit A:
  - `id=1`
- dependent submit B:
  - `id=2`
- source submit C:
  - `id=3`

The configured thresholds were:

- primary lane:
  - `dependency_required_ticket=1`
- secondary lane:
  - `dependency2_required_ticket=3`

### Initial State

After `depticket2` plus `depstate2`, submit B showed:

- primary lane:
  - `dependency_submit_id=1`
  - `dependency_required_ticket=1`
  - `dependency_observed_complete_ticket=0`
  - `dependency_flags=0x0b`
- secondary lane:
  - `dependency2_submit_id=3`
  - `dependency2_required_ticket=3`
  - `dependency2_observed_complete_ticket=0`
  - `dependency2_flags=0x0b`
- `dependency_config_count=1`
- `dependency2_config_count=1`
- `validate_count=4`
- `last_validate_status=0`

So lane 2 was no longer inheriting the lane 1 threshold.

### State After Submit A Completes

After submit A completed, submit B showed:

- primary lane released:
  - `dependency_observed_complete_ticket=1`
  - `dependency_flags=0x0f`
- secondary lane still waiting:
  - `dependency2_observed_complete_ticket=0`
  - `dependency2_required_ticket=3`
  - `dependency2_flags=0x0b`
- `validate_count=6`
- `last_validate_status=0`

That part matched expectation, but it still did not prove the tighter lane 2
threshold by itself because submit C had not completed yet.

### State After Submit C First Completion

Submit C then completed once.

At that point, submit B showed:

- primary lane still released:
  - `dependency_observed_complete_ticket=1`
  - `dependency_flags=0x0f`
- secondary lane still gated:
  - `dependency2_observed_complete_ticket=2`
  - `dependency2_required_ticket=3`
  - `dependency2_flags=0x0b`
- `validate_count=8`
- `last_validate_status=0`

An immediate `running` attempt on submit B still failed with
`Resource temporarily unavailable`.

That is the key new result:

- submit C had already completed once
- lane 2 had an observed ticket of `2`
- lane 2 still stayed gated because it required `3`

### State After Submit C Second Completion

Submit C then ran one refill cycle and completed again.

After that second completion, submit B showed:

- primary lane still released:
  - `dependency_observed_complete_ticket=1`
  - `dependency_flags=0x0f`
- secondary lane finally released:
  - `dependency2_observed_complete_ticket=3`
  - `dependency2_required_ticket=3`
  - `dependency2_flags=0x0f`
- `validate_count=10`
- `last_validate_status=0`

So lane 2 released only after the second completion on its bound source submit.

### Dependent Submit Then Runs Normally

With lane 2 released, submit B then ran normally:

- after `running`:
  - `owner_state=running`
  - `global_issue_ticket=4`
  - `global_running_ticket=4`
  - `validate_count=11`
- after `complete`:
  - `owner_state=complete`
  - `global_complete_ticket=4`
  - `queue_packet_count=0`
  - `validate_count=12`
- after probe exit:
  - `dependency_observed_complete_ticket=1`
  - `dependency2_observed_complete_ticket=3`
  - `validate_count=14`
  - `last_validate_status=0`

### Coordination State

The coordination node progressed cleanly:

- after submit A:
  - `issue/running/complete = 1/1/1`
- after submit C:
  - `issue/running/complete = 3/3/3`
  - because submit C completed twice
- after submit B:
  - `issue/running/complete = 4/4/4`
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

The validate-only dependency model now proves per-lane ticket semantics:

- lane 1 can be released
- lane 2 can still remain gated on the same dependent submit
- lane 2 can require a later completion ticket from its own bound source submit

So the earlier “one shared wait-ticket threshold” limitation is now gone.

## Next Step

The next useful dependency question is no longer whether the lanes can use
different thresholds.

It is whether later render work needs:

- more than two dependency lanes
- a dynamic dependency array instead of the current fixed pair
