# SGX Dependency Slot Clear Validation

This note records the first full hardware validation of the indexed
`depclear_slot` path on the Sony VAIO VGN-P11Z target.

## Context

[docs/46-sgx-dependency-slot-clear.md](/root/Poulsbo/docs/46-sgx-dependency-slot-clear.md)
captured the implementation work and the later target-side outage that blocked
validation.

That blocker is now resolved. The validation completed on May 11, 2026 through
the live recovery bundle at:

- [logs/vaio-recovery/20260511-065408-depclear-live](/root/Poulsbo/logs/vaio-recovery/20260511-065408-depclear-live)

The session result is:

- [session-result.env](/root/Poulsbo/logs/vaio-recovery/20260511-065408-depclear-live/session-result.env)
  with:
  - `session_outcome=completed`
  - `last_overall=remote_usable`
  - `state_rc=0`
  - `validation_rc=0`

## Validated Module State

The target returned to a usable SSH state and ran the current local override
module:

- kernel: `5.4.0-150-generic`
- module path:
  `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- module `srcversion`:
  `2B42B80E462B9A3C0BAAC9B`

## What Was Validated

The intended validation flow was:

1. gate submit B on indexed dependency slots `1..4`
2. run source submits A, C, and D
3. verify B is still blocked only because slot `4` remains active
4. clear slot `4` with `depclear_slot`
5. verify B can run and complete before submit E
6. run submit E afterward
7. verify slot `4` stays detached and `dependency4_*` does not repopulate

## Observed Hardware Result

The saved validator log is:

- [depclear-slot-validation.log](/root/Poulsbo/logs/vaio-recovery/20260511-065408-depclear-live/depclear-slot-validation.log)

Key observed results:

- the driver executed:
  - `lab submit depclear_slot submit_id=2 slot=4`
- after the clear, submit B showed:
  - `dependency4_submit_id=0`
  - `dependency4_offset=0x0`
  - `dependency4_required_ticket=0`
  - `dependency4_observed_complete_ticket=0`
  - `dependency4_flags=0x0`
  - `dependency4_config_count=0`
- that detached state is visible in:
  - `SUBMIT_B_STILL_IDLE_AFTER_DEPCLEAR4`
  - `COORD_STATE_AFTER_DEPCLEAR4`
- submit B then ran and completed before E, ending at:
  - `global_issue_ticket=4`
  - `global_running_ticket=4`
  - `global_complete_ticket=4`
  - `owner_state=complete`
- submit E then ran afterward and completed separately, advancing the global
  counters to:
  - `issue/running/complete = 5/5/5`
- after E completed, submit B still showed:
  - `dependency4_submit_id=0`
  - `dependency4_offset=0x0`
  - `dependency4_flags=0x0`

That means the clear path is not cosmetic. It really detaches the slot and lets
the dependent submit proceed without waiting for the former source submit.

## Stability Result

The recovery-session result stayed clean:

- `overall=remote_usable`
- `validation_rc=0`

The target-side validation log also ended with normal submit/object teardown and
did not show any new MMU or SGX fault path as part of this scenario.

## Conclusion

The indexed dependency-slot clear path is now fully validated on real hardware.

The project should no longer treat `depclear_slot` as “implemented locally but
pending validation.” It is now a completed SGX lab-submit milestone.
