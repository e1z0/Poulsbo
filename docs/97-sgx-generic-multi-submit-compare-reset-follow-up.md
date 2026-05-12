# Generic Multi-Submit Compare Reset Follow-Up

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion D43951B174F84BE870230AB`

## Context

After the warm-target reset helper and focused legacy submit range-query fixes
were validated, the remaining open compare-family follow-up was the generic
multi-submit wrapper:

- [scripts/run-vaio-sgx-multi-submit-mode-compare.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit-mode-compare.sh)

The specific goal was to prove that the same reset-assisted typed-to-legacy
handoff still holds on the generic two-submit scenario, not just on the
dependency-slot-clear and focused wrapper families.

## Validation

Completed live bundle:

- [logs/vaio-compare/20260511-145551-sgx-multi-submit-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-145551-sgx-multi-submit-mode-compare)

Saved result:

- `session_kind=sgx_multi_submit_mode_compare`
- `session_outcome=completed`
- `pre_reset_rc=0`
- `typed_rc=0`
- `typed_health_overall=remote_usable`
- `inter_reset_rc=0`
- `legacy_rc=0`
- `legacy_health_overall=remote_usable`
- `last_overall=remote_usable`
- `final_driver_srcversion=D43951B174F84BE870230AB`
- `final_override_module_size=385384`

Typed/default branch:

- `typed_submit_setup=ioctl`
- `typed_object_setup=ioctl`
- `typed_range_discovery=ioctl`
- `typed_dependency_setup=ioctl`
- `typed_verification_mode=ioctl`
- `typed_dependency_interface=named`
- `typed_submit_a_id=133`
- `typed_submit_b_id=134`
- `typed_range_ids=802 803 804 805 806 807 808 810 809 811`
- `typed_submit_validate_count=11`
- `typed_submit_issue_count=2`
- `typed_submit_transition_count=4`
- `typed_submit_owner_state=complete`
- final typed cleanup:
  - `typed_live_submit_count=0`
  - `typed_live_object_count=0`
  - `typed_range_sgx_object_count=0`
  - `typed_range_pf_bound_count=0`

Explicit legacy branch:

- `legacy_dependency_interface=named`
- `legacy_submit_a_id=135`
- `legacy_submit_b_id=136`
- `legacy_range_ids=812 813 814 815 816 817 818 819 820 821`
- cleanup markers:
  - `legacy_has_submits_after_drop=SUBMITS_AFTER_DROP`
  - `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`
  - `legacy_has_coord_after_drop=COORD_STATE_AFTER_DROP`

Final target state:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Result

The reset-assisted compare path is now revalidated across the generic
multi-submit wrapper too.

At this point the full compare family touched by the wrapper-default and
warm-target legacy-baseline work is back in a clean state on the live VAIO:

- focused object compare
- focused submit compare
- generic multi-submit compare
- dependency-slot-clear compare

No kernel-tree files changed in this milestone.
