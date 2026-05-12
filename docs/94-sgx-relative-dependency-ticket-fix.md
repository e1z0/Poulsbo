# SGX Relative Dependency Ticket Fix

## What Changed

Fixed a real scenario bug in the multi-submit dependency runner:

- `scripts/run-vaio-sgx-multi-submit.sh`

The bug was in dependency-ticket semantics, not in the kernel driver.

The runner had been treating secondary and later dependency tickets as absolute
global completion-ticket values. That worked only while the driver-global
completion counters were still near zero. Once the VAIO had accumulated many
completed lab submits across earlier sessions, the old default:

- `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET=3`

was no longer meaningful. Lane 2 would already be satisfied after the first
completion on its source submit, because the source submit's mirrored global
completion ticket was already far above `3`.

The runner now supports relative ticket syntax:

- `+<n>`

That value is resolved on the target against the current coordination baseline:

- `required_ticket = coord_complete_ticket_baseline + n`

This resolution now works in both output formats used by the runner:

- typed query output:
  - `coord_complete_ticket=...`
- legacy sysfs coord output:
  - `complete_ticket_counter=...`

The dedicated wrapper:

- `scripts/run-vaio-sgx-multi-source-ticket-dependency.sh`

now uses:

- `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET=+3`

instead of the stale absolute `3`.

## Hardware Validation

Validated on May 11, 2026 on `10.1.11.218`.

### Direct Scenario Revalidation

After the fix, the direct scenario runner completed cleanly again:

- `scripts/run-vaio-sgx-multi-source-ticket-dependency.sh`

The direct rerun returned:

- `RC=0`

and the live target log showed the lane-2 required ticket being resolved
relative to the current global completion baseline:

- `lab submit depticket_slot submit_id=82 slot=2 required_ticket=75`

That restored the intended behavior:

1. submit A completes once
2. submit C completes once and lane 2 stays gated
3. submit C completes a second time and lane 2 releases
4. submit B then runs normally

### Typed-Versus-Legacy Compare Validation

Also validated through the new compare wrapper:

- `scripts/run-vaio-sgx-multi-source-ticket-dependency-mode-compare.sh`

Saved bundle:

- `logs/vaio-compare/20260511-140046-sgx-multi-source-ticket-dependency-mode-compare`

Saved session result:

- `session_kind=sgx_multi_source_ticket_dependency_mode_compare`
- `session_outcome=completed`
- `preflight_overall=remote_usable`
- `typed_rc=0`
- `typed_health_overall=remote_usable`
- `legacy_rc=0`
- `legacy_health_overall=remote_usable`
- `last_overall=remote_usable`

The target stayed clean after the compare run:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

### Typed Path Result

The default typed path stayed on the bounded ioctl stack:

- `typed_submit_setup=ioctl`
- `typed_object_setup=ioctl`
- `typed_range_discovery=ioctl`
- `typed_dependency_setup=ioctl`
- `typed_verification_mode=ioctl`
- `typed_dependency_interface=named`

Saved typed state:

- `typed_submit_a_id=90`
- `typed_submit_b_id=91`
- `typed_submit_c_id=92`
- `typed_range_ids=460 461 462 463 464 465 466 467 468 469 470 471 472 473 474`
- terminal typed state:
  - `typed_submit_validate_count=8`
  - `typed_submit_issue_count=2`
  - `typed_submit_transition_count=4`
  - `typed_submit_owner_state=complete`
  - `typed_live_submit_count=0`
  - `typed_live_object_count=0`
  - `typed_range_sgx_object_count=0`
  - `typed_range_pf_bound_count=0`

The live typed log captured the relative fix explicitly:

- `COORD_COMPLETE_TICKET_BASELINE=84`
- `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET=87`
- `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET_RELATIVE=3`

and it hit the intended intermediate states:

- `SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_C_FIRST_COMPLETE`
- `SUBMIT_B_DEPENDENCY_WAIT_AFTER_C_FIRST_COMPLETE`

So lane 2 stayed gated after the first completion on submit C, then released
only after the second completion brought the observed ticket up to the resolved
required ticket.

### Explicit Legacy Path Result

The explicit legacy branch also completed cleanly:

- `legacy_dependency_interface=named`
- `legacy_submit_a_id=93`
- `legacy_submit_b_id=94`
- `legacy_submit_c_id=95`
- `legacy_range_ids=475 476 477 478 479 480 481 482 483 484 485 486 487 488 489`

The legacy log captured the same relative fix in the sysfs-oriented path:

- `COORD_COMPLETE_TICKET_BASELINE=88`
- `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET=91`
- `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET_RELATIVE=3`

and it also reached the intended intermediate labels:

- `SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_C_FIRST_COMPLETE`
- `SUBMIT_B_DEPENDENCY_WAIT_AFTER_C_FIRST_COMPLETE`

Legacy cleanup still ends in the expected markers:

- `legacy_has_submits_after_drop=SUBMITS_AFTER_DROP`
- `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`
- `legacy_has_coord_after_drop=COORD_STATE_AFTER_DROP`

## Why This Matters

This fix closes a real correctness gap in the validation harness:

- the driver-global completion counters persist across sessions
- absolute dependency tickets are therefore unstable across time
- scenario wrappers that mean “N completions after now” need relative
  semantics, not fixed small integers

Without this change, the multi-source ticket scenario looked healthy only while
the target had a low global completion count. After enough prior lab runs, the
scenario became silently wrong.

## Conclusion

The multi-submit runner now has a stable dependency-ticket model for scenarios
that need thresholds relative to the current driver-global completion counter.

That gives the repo two concrete improvements:

1. the direct multi-source ticket scenario is correct again
2. the typed-vs-legacy compare wrapper for that scenario is now hardware-
   validated and trustworthy
