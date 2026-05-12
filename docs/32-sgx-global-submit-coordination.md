# SGX Global Submit Coordination

This note records the first hardware-validated global coordination layer across
multiple persistent SGX lab submits on the Ubuntu 5.4 Poulsbo target.

## Why This Step Exists

The previous milestone in [docs/31-sgx-multi-submit-isolation.md](/root/Poulsbo/docs/31-sgx-multi-submit-isolation.md)
proved that two persistent submits could coexist without corrupting each
other's per-submit state.

That still left one missing piece:

- there was no explicit global ordering state across submits
- there was no shared coordination view to answer which submit last advanced
  issue-side or completion-side state
- the current sync and owner blocks did not yet carry any cross-submit ticket
  semantics

This step adds those explicit coordination semantics.

## Current Kernel Surface

The Ubuntu 5.4 lab driver now exposes:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_coord_state`

That node reports:

- `issue_ticket_counter`
- `complete_ticket_counter`
- `last_issue_submit_id`
- `last_complete_submit_id`
- `last_issue_jiffies`
- `last_complete_jiffies`
- `focused_submit_valid`
- `focused_submit_id`
- `live_submit_count`

Each submit now also carries:

- `global_issue_ticket`
- `global_complete_ticket`

and those values are mirrored into the live backing objects:

- the sync block now carries `global_issue_ticket`
- the owner block now carries `global_complete_ticket`

## Validation Semantics

The current coordination model is still validate-only, but it is now explicit:

- `issue` increments a global issue counter on the driver and stamps the new
  value into the submit's sync block
- `transition ... complete ...` increments a global complete counter on the
  driver and stamps the new value into the submit's owner block
- `transition ... running ...` does not allocate a completion ticket
- submit validation now requires sync and owner readback to match the live
  per-submit ticket fields
- submit validation now requires each per-submit ticket to be less than or
  equal to the driver-global counter it belongs to

The first hardware pass exposed one real validation bug: the initial check
treated any transition as if it required a completion ticket, which incorrectly
rejected the intermediate `running` state. That logic is now fixed in the final
validated module below.

## Helper Changes

The concurrent helper:

- `scripts/run-vaio-sgx-multi-submit.sh`

now captures:

- `COORD_STATE_AFTER_SETUP`
- `COORD_STATE_AFTER_SUBMIT_A`
- `COORD_STATE_AFTER_SUBMIT_B`
- `COORD_STATE_AFTER_DROP`

That makes the multi-submit log useful for both isolation and cross-submit
ordering analysis.

## Hardware Validation Result

This step is hardware-validated on `10.1.11.218` on May 11, 2026.

The final validated override module was:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 1170524E4DF5B69FF27D8E2`

The first coordination attempt exposed the over-strict transition validation
bug described above. After fixing that, the final run completed cleanly.

### Global Coordination Progression

After setup:

- `issue_ticket_counter=0`
- `complete_ticket_counter=0`
- `last_issue_submit_id=0`
- `last_complete_submit_id=0`
- `focused_submit_id=2`
- `live_submit_count=2`

After submit A completed its wrap-mode run with `SUBMIT_A_REFILL_CYCLES=2`:

- `issue_ticket_counter=3`
- `complete_ticket_counter=3`
- `last_issue_submit_id=1`
- `last_complete_submit_id=1`
- `live_submit_count=2`

After submit B completed its nowrap-mode run with `SUBMIT_B_REFILL_CYCLES=1`:

- `issue_ticket_counter=5`
- `complete_ticket_counter=5`
- `last_issue_submit_id=2`
- `last_complete_submit_id=2`
- `live_submit_count=2`

After both submits and all objects were dropped:

- `issue_ticket_counter=5`
- `complete_ticket_counter=5`
- `last_issue_submit_id=2`
- `last_complete_submit_id=2`
- `focused_submit_valid=0`
- `focused_submit_id=0`
- `live_submit_count=0`

### Per-Submit Coordination Result

Submit B stayed completely idle while submit A ran:

- `queue_epoch=0`
- `issue_count=0`
- `transition_count=0`
- `global_issue_ticket=0`
- `global_complete_ticket=0`
- `validate_count=1`

Submit A's final state after submit B had still not run was:

- `queue_epoch=2`
- `issue_count=3`
- `transition_count=6`
- `global_issue_ticket=3`
- `global_complete_ticket=3`
- `validate_count=10`
- `last_validate_status=0`

Submit B's final state after its own run was:

- `queue_epoch=1`
- `issue_count=2`
- `transition_count=4`
- `global_issue_ticket=5`
- `global_complete_ticket=5`
- `validate_count=7`
- `last_validate_status=0`

After submit B finished, submit A was re-read and remained unchanged:

- `queue_epoch=2`
- `issue_count=3`
- `transition_count=6`
- `global_issue_ticket=3`
- `global_complete_ticket=3`
- `validate_count=10`
- `last_validate_status=0`

After all userspace holders exited, both submits still validated:

- submit A:
  - `validate_count=11`
  - `last_validate_status=0`
- submit B:
  - `validate_count=8`
  - `last_validate_status=0`

### Final Target State

- `lightdm=active`
- `fault_count=0`
- `bif_fault=0x000000`
- `capture_failed=0`
- `issue_ticket_counter=5`
- `complete_ticket_counter=5`
- `last_issue_submit_id=2`
- `last_complete_submit_id=2`
- `focused_submit_valid=0`
- `submits_shown=0`
- `focused_submit=none`
- `objects_shown=0`

## Conclusion

The lab driver now has a validated distinction between:

- per-submit progression state
- driver-global issue ordering
- driver-global completion ordering

That is enough to keep advancing validate-only SGX queue semantics without
pretending the render path already exists.

## Next Step

The next useful question is not whether cross-submit ordering exists. It does.

The next useful questions are:

- whether the current global issue and completion tickets are sufficient once
  later queue ownership or fence-like state appears
- whether future SGX-shaped state needs more than one global coordination lane
- whether this validate-only coordination model should stay in sysfs or move to
  a more controlled debug interface before any real execution attempt
