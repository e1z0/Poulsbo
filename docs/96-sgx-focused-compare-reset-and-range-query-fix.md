# Focused Compare Reset And Range-Query Fix

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion D43951B174F84BE870230AB`

## Problem

After the warm-target reset helper landed, the focused object compare path was
healthy again, but the focused submit compare path still regressed in its
explicit legacy branch.

The failing submit-compare bundles were:

- [logs/vaio-compare/20260511-142528-sgx-submit-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-142528-sgx-submit-mode-compare)
- [logs/vaio-compare/20260511-142805-sgx-submit-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-142805-sgx-submit-mode-compare)
- [logs/vaio-compare/20260511-143134-sgx-submit-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-143134-sgx-submit-mode-compare)
- [logs/vaio-compare/20260511-143948-sgx-submit-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-143948-sgx-submit-mode-compare)
- [logs/vaio-compare/20260511-144519-sgx-submit-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-144519-sgx-submit-mode-compare)

The root causes were all in the warm-target legacy range-discovery path:

1. [scripts/run-vaio-sgx-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit.sh)
   still depended on the truncated `poulsbo_gtt_state` dump for fresh live GEM
   range discovery in its explicit legacy branch.
2. Both [scripts/run-vaio-sgx-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit.sh)
   and [scripts/run-vaio-sgx-lab-reset.sh](/root/Poulsbo/scripts/run-vaio-sgx-lab-reset.sh)
   parsed the typed range-query helper as if it emitted `range_name=...`, while
   the real helper emits `range_type_name=...`.
3. Both scripts also parsed `range_ioctl_ret=0 errno=0` with `awk -F=`, which
   produced `0 errno` instead of the numeric `0`, so index `0` looked like a
   failed range query and the typed range baseline collapsed immediately.

Those bugs made the reset helper undercount live mmapping GEM ranges and left
the focused legacy submit branch thinking only two fresh SGX-adoptable ranges
existed on a warm target.

## Changes

Extended the typed reset helper:

- [scripts/run-vaio-sgx-lab-reset.sh](/root/Poulsbo/scripts/run-vaio-sgx-lab-reset.sh)

It now uses the typed range-query helper to count live mmapping GEM ranges and
will only report convergence once that count drops back to the expected steady
baseline:

- `range_query_live_gem_count`
- `baseline_mmapping_gem_max`

The helper now parses:

- `range_ioctl_ret=... errno=...` by extracting only the first field after the
  prefix
- `range_type_name=...` instead of the old nonexistent `range_name=...`

Extended the focused legacy submit helper:

- [scripts/run-vaio-sgx-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit.sh)

The explicit legacy branch now:

- deploys the typed range-query helper alongside the legacy `drm_probe`
  workload
- collects a typed "before" GEM-range baseline
- collects a typed "during" GEM-range baseline
- derives fresh SGX-adoptable ranges from that typed diff when available
- only falls back to the old sysfs GTT dump if the typed range query produces
  no data at all

The same script also now parses:

- `range_ioctl_ret=... errno=...` correctly
- `range_type_name=...` correctly

No kernel-tree files changed in this milestone.

## Validation

Warm-target reset helper after the parser fix:

- local run:
  - `./scripts/run-vaio-sgx-lab-reset.sh`
- result:
  - `RC=0`
  - `submit_result_count=0`
  - `object_result_count=2`
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
  - `coord_focus_valid=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`
  - `range_query_live_gem_count=2`
  - `baseline_mmapping_gem_max=2`

Focused object compare revalidated through the reset-assisted warm-target path:

- [logs/vaio-compare/20260511-142409-sgx-object-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-142409-sgx-object-mode-compare)

Saved result:

- `session_outcome=completed`
- `pre_reset_rc=0`
- `typed_rc=0`
- `inter_reset_rc=0`
- `legacy_rc=0`
- `last_overall=remote_usable`

Key object-compare evidence:

- typed/default branch:
  - `typed_range_id=7`
  - `typed_object_id=641`
  - `typed_object_role=control`
  - `typed_object_class=packet`
  - `typed_object_gpu_use=control_ccb`
  - `typed_object_context=pf`
  - `typed_object_pf_bind_count=1`
  - `typed_object_after_drop_rc=-1 errno=2`
- explicit legacy branch:
  - `legacy_range_id=736`
  - `legacy_object_id=642`
  - `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`
  - `legacy_has_gtt_after_drop=GTT_STATE_AFTER_DROP`

Focused submit compare now also completes cleanly:

- [logs/vaio-compare/20260511-144832-sgx-submit-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-144832-sgx-submit-mode-compare)

Saved result:

- `session_outcome=completed`
- `pre_reset_rc=0`
- `typed_rc=0`
- `inter_reset_rc=0`
- `legacy_rc=0`
- `legacy_health_overall=remote_usable`
- `last_overall=remote_usable`

Typed/default focused submit branch:

- `typed_submit_id=131`
- `typed_range_ids=7 9 792 793 794`
- `typed_object_ids=672 673 674 675 676`
- `typed_validate_count=1`
- `typed_issue_count=1`
- `typed_transition_count=2`
- `typed_owner_state=complete`
- final typed cleanup:
  - `typed_live_submit_count=0`
  - `typed_range_sgx_object_count=0`
  - `typed_range_pf_bound_count=0`

Explicit legacy focused submit branch:

- `legacy_queue_mode=wrap`
- `legacy_submit_id=132`
- `legacy_range_ids=797 798 799 800 801`
- `legacy_object_ids=677 678 679 680 681`
- cleanup markers:
  - `legacy_has_submits_after_drop=SUBMITS_AFTER_DROP`
  - `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`

Final target state after the completed submit compare:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Result

The warm-target reset path is now proven across all focused compare families
that were touched by the wrapper-mode work:

- focused object compare
- focused submit compare
- dependency-slot-clear compare

The critical practical fix was not in SGX logic itself. It was in treating the
typed range-query helper as the authoritative live-range baseline for warm
legacy submit runs, and in parsing its real output correctly.
