# SGX Compare Reset And Legacy Range-Baseline Fix

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion D43951B174F84BE870230AB`

## Problem

The dependency-slot-clear typed-versus-legacy compare wrapper exposed two
separate warm-target failures:

1. The compare wrappers could hand control from the typed branch to the legacy
   branch before remote probe teardown had fully settled, so the second branch
   sometimes inherited stale SGX lab objects or PF-bound ranges.
2. The direct legacy multi-submit path still derived its "fresh GEM range"
   baseline from `poulsbo_gtt_state`, which is page-sized and truncates once
   live range IDs climb high enough on a warm target.

The first bad compare bundle was:

- [logs/vaio-compare/20260511-140709-sgx-dependency-slot-clear-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-140709-sgx-dependency-slot-clear-mode-compare)

It finished the typed/default branch, but the explicit legacy branch failed
before scenario setup completed.

## Changes

Added a reusable reset helper:

- [scripts/run-vaio-sgx-lab-reset.sh](/root/Poulsbo/scripts/run-vaio-sgx-lab-reset.sh)

It runs typed submit/object cleanup directly on the target, then verifies zero
state through typed coord and GTT queries until all of the following converge
to zero:

- `coord_live_submit_count`
- `coord_live_object_count`
- `coord_focus_valid`
- `gtt_state_range_sgx_object_count`
- `gtt_state_range_pf_bound_count`

The compare wrappers now call that helper before the typed branch and again
between the typed and legacy branches, recording:

- `pre-reset.log`
- `inter-reset.log`

Updated wrappers:

- [scripts/run-vaio-sgx-object-mode-compare.sh](/root/Poulsbo/scripts/run-vaio-sgx-object-mode-compare.sh)
- [scripts/run-vaio-sgx-submit-mode-compare.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-mode-compare.sh)
- [scripts/run-vaio-sgx-multi-submit-mode-compare.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit-mode-compare.sh)
- [scripts/run-vaio-sgx-dependency-slot-clear-mode-compare.sh](/root/Poulsbo/scripts/run-vaio-sgx-dependency-slot-clear-mode-compare.sh)

Also fixed the legacy fresh-range baseline in:

- [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)

The runner now always uses the typed range-query helper for before/during GEM
range ID baselines when that query path is available, even if the actual
scenario is running in explicit legacy mode for object, submit, dependency,
and verification behavior. It still falls back to the old sysfs dump only if
the typed range-query path produces no data.

That keeps the legacy branch honest on warm targets without changing the
scenario's legacy mutation path.

## Validation

Reset helper live check:

- local run:
  - `./scripts/run-vaio-sgx-lab-reset.sh`
- result:
  - `RC=0`
  - first attempt cleared:
    - `submit_result_count=0`
    - `object_result_count=2`
    - `coord_live_submit_count=0`
    - `coord_live_object_count=0`
    - `coord_focus_valid=0`
    - `gtt_state_range_sgx_object_count=0`
    - `gtt_state_range_pf_bound_count=0`

Clean compare bundle after both fixes:

- [logs/vaio-compare/20260511-141658-sgx-dependency-slot-clear-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-141658-sgx-dependency-slot-clear-mode-compare)

Saved result:

- `session_outcome=completed`
- `pre_reset_rc=0`
- `typed_rc=0`
- `typed_health_overall=remote_usable`
- `inter_reset_rc=0`
- `legacy_rc=0`
- `legacy_health_overall=remote_usable`
- `last_overall=remote_usable`

Reset evidence inside that bundle:

- `pre-reset.log`:
  - `object_result_count=2`
  - all typed coord/GTT live counts converged to `0`
- `inter-reset.log`:
  - `submit_result_count=0`
  - `object_result_count=0`
  - all typed coord/GTT live counts remained `0`

Typed/default dependency-slot-clear branch:

- `typed_submit_setup=ioctl`
- `typed_object_setup=ioctl`
- `typed_range_discovery=ioctl`
- `typed_dependency_setup=ioctl`
- `typed_verification_mode=ioctl`
- `typed_dependency_interface=indexed`
- `typed_submit_a_id=116`
- `typed_submit_b_id=117`
- `typed_submit_c_id=118`
- `typed_submit_d_id=119`
- `typed_submit_e_id=120`
- `typed_range_ids=685..709`
- `typed_dependency4_submit_id_after_clear=0`
- `typed_dependency4_flags_after_clear=0x0`
- final typed cleanup:
  - `typed_live_submit_count_after_drop=0`
  - `typed_live_object_count_after_drop=0`
  - `typed_range_sgx_object_count_after_drop=0`
  - `typed_range_pf_bound_count_after_drop=0`

Typed relative-ticket evidence:

- `COORD_COMPLETE_TICKET_BASELINE=112`
- `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET=114`
- `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET_RELATIVE=2`
- `SUBMIT_B_DEPENDENCY3_COMPLETE_TICKET=115`
- `SUBMIT_B_DEPENDENCY3_COMPLETE_TICKET_RELATIVE=3`
- `SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET=116`
- `SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET_RELATIVE=4`

Explicit legacy branch also completed cleanly on the same warm target:

- `legacy_submit_setup=sysfs`
- `legacy_object_setup=sysfs`
- `legacy_range_discovery=sysfs`
- `legacy_dependency_setup=sysfs`
- `legacy_verification_mode=sysfs`
- `legacy_dependency_interface=indexed`
- `legacy_submit_a_id=121`
- `legacy_submit_b_id=122`
- `legacy_submit_c_id=123`
- `legacy_submit_d_id=124`
- `legacy_submit_e_id=125`
- `legacy_range_ids=710..734`
- cleanup markers:
  - `legacy_has_submits_after_drop=SUBMITS_AFTER_DROP`
  - `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`
  - `legacy_has_coord_after_drop=COORD_STATE_AFTER_DROP`

Legacy relative-ticket evidence:

- `COORD_COMPLETE_TICKET_BASELINE=117`
- `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET=119`
- `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET_RELATIVE=2`
- `SUBMIT_B_DEPENDENCY3_COMPLETE_TICKET=120`
- `SUBMIT_B_DEPENDENCY3_COMPLETE_TICKET_RELATIVE=3`
- `SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET=121`
- `SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET_RELATIVE=4`

Final target state after the completed compare:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Result

The dependency-slot-clear compare path is now stable again on a warm VAIO.

The two fixes serve different purposes:

- the reset helper removes stale SGX lab state between compare branches
- the range-query baseline fix stops the direct legacy multi-submit path from
  being fooled by truncated sysfs GTT dumps once live range IDs grow large

No kernel-tree files changed in this milestone.
