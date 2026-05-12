# SGX Third Dependency Lane

This note records the first hardware-validated case where one dependent submit
waits on three concrete source submits at once.

## Why This Step Exists

[docs/42-sgx-per-lane-ticket-gating.md](/root/Poulsbo/docs/42-sgx-per-lane-ticket-gating.md)
proved that two dependency lanes can use different completion-ticket
thresholds.

That still left one structural question:

- are two fixed dependency lanes enough for the current lab model?

The next practical step was not a dynamic ABI yet. It was to add one more fixed
lane and prove it behaves correctly on real hardware.

## Current Semantics

The Ubuntu 5.4 SGX lab path now supports:

- the primary dependency lane through `depend` plus `ta_dependency`
- a second lane through `depend2`, `depticket2`, and `depstate2`
- a third lane through `depend3`, `depticket3`, and `depstate3`

Each lane writes its own state block into the `status` object and carries its
own source submit, required ticket, observed completion ticket, and flags.

## Kernel And Helper Changes

Kernel-side:

- widened `PSB_SGX_LAB_DEPENDENCY_SLOTS` from `2` to `3`
- added third-lane fields and state plumbing in
  [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
- added new sysfs commands:
  - `depend3 <submit_id> <dependency_submit_id>`
  - `depticket3 <submit_id> <required_ticket>`
  - `depstate3 <submit_id> <offset>`
- updated the dependency validation, refresh, overlap, and gate logic so all
  dependency-slot loops operate over three lanes

Helper-side:

- extended
  [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
  with:
  - `RUN_TERTIARY_DEPENDENCY_SOURCE`
  - `SUBMIT_D_QUEUE_MODE`
  - `SUBMIT_D_REFILL_CYCLES`
  - `SUBMIT_B_DEPENDENCY3_COMPLETE_TICKET`
  - `SUBMIT_B_DEPENDENCY3_OFFSET`
- added the dedicated wrapper:
  - [scripts/run-vaio-sgx-third-dependency-lane.sh](/root/Poulsbo/scripts/run-vaio-sgx-third-dependency-lane.sh)

That wrapper drives:

1. source submit A once
2. source submit C once
3. source submit D once
4. dependent submit B only after all three lanes release

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026.

The validated override module is now:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 157E84B220F2E20EEF2CF58`

The validated run used:

- source submit A:
  - `id=1`
- dependent submit B:
  - `id=2`
- source submit C:
  - `id=3`
- source submit D:
  - `id=4`

The configured thresholds were:

- primary lane:
  - `dependency_required_ticket=1`
- secondary lane:
  - `dependency2_required_ticket=2`
- tertiary lane:
  - `dependency3_required_ticket=3`

### Initial State

After `depstate`, `depstate2`, and `depstate3`, submit B showed:

- primary lane:
  - `dependency_submit_id=1`
  - `dependency_required_ticket=1`
  - `dependency_observed_complete_ticket=0`
  - `dependency_flags=0x0b`
- secondary lane:
  - `dependency2_submit_id=3`
  - `dependency2_required_ticket=2`
  - `dependency2_observed_complete_ticket=0`
  - `dependency2_flags=0x0b`
- tertiary lane:
  - `dependency3_submit_id=4`
  - `dependency3_required_ticket=3`
  - `dependency3_observed_complete_ticket=0`
  - `dependency3_flags=0x0b`
- `dependency_config_count=1`
- `dependency2_config_count=1`
- `dependency3_config_count=1`
- `validate_count=6`
- `last_validate_status=0`

### State After Submit A Completes

After submit A completed, submit B showed:

- primary lane released:
  - `dependency_observed_complete_ticket=1`
  - `dependency_flags=0x0f`
- secondary lane still gated:
  - `dependency2_observed_complete_ticket=0`
  - `dependency2_required_ticket=2`
  - `dependency2_flags=0x0b`
- tertiary lane still gated:
  - `dependency3_observed_complete_ticket=0`
  - `dependency3_required_ticket=3`
  - `dependency3_flags=0x0b`
- `validate_count=8`
- `last_validate_status=0`

An immediate `running` attempt on submit B still failed with
`Resource temporarily unavailable`.

### State After Submit C Completes

After submit C completed, submit B showed:

- primary lane still released:
  - `dependency_observed_complete_ticket=1`
  - `dependency_flags=0x0f`
- secondary lane released:
  - `dependency2_observed_complete_ticket=2`
  - `dependency2_required_ticket=2`
  - `dependency2_flags=0x0f`
- tertiary lane still gated:
  - `dependency3_observed_complete_ticket=0`
  - `dependency3_required_ticket=3`
  - `dependency3_flags=0x0b`
- `validate_count=10`
- `last_validate_status=0`

Another `running` attempt on submit B still failed with
`Resource temporarily unavailable`.

### State After Submit D Completes

After submit D completed, submit B showed:

- primary lane released:
  - `dependency_observed_complete_ticket=1`
  - `dependency_flags=0x0f`
- secondary lane released:
  - `dependency2_observed_complete_ticket=2`
  - `dependency2_flags=0x0f`
- tertiary lane released:
  - `dependency3_observed_complete_ticket=3`
  - `dependency3_required_ticket=3`
  - `dependency3_flags=0x0f`
- `validate_count=12`
- `last_validate_status=0`

So the dependent submit stayed blocked until all three concrete source lanes
released.

### Dependent Submit Then Runs Normally

With lane 3 released, submit B then ran normally:

- after `running`:
  - `owner_state=running`
  - `global_issue_ticket=4`
  - `global_running_ticket=4`
  - `validate_count=13`
- after `complete`:
  - `owner_state=complete`
  - `global_complete_ticket=4`
  - `queue_packet_count=0`
  - `validate_count=14`
- after probe exit:
  - `dependency_observed_complete_ticket=1`
  - `dependency2_observed_complete_ticket=2`
  - `dependency3_observed_complete_ticket=3`
  - `dependency_flags=0x0f`
  - `dependency2_flags=0x0f`
  - `dependency3_flags=0x0f`
  - `validate_count=16`
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

The validate-only dependency model now proves three-lane source gating on real
hardware:

- one dependent submit can wait on three concrete source submits
- the third lane can remain gated after the first two lanes are already
  released
- the same focused submit view and state-block model still validate cleanly
  across all three lanes

So the next dependency question is no longer whether the model can grow beyond
two lanes at all.

It is whether further growth should remain fixed-lane or become a more general
dependency array or fence block before any real SGX execution attempt.
