# SGX Mode-Compare Session Validation

## What Changed

Added a consistent typed-versus-legacy comparison layer for the three wrapper
families that now default to the typed ioctl path:

- `scripts/run-vaio-sgx-object-mode-compare.sh`
- `scripts/run-vaio-sgx-submit-mode-compare.sh`
- `scripts/run-vaio-sgx-multi-submit-mode-compare.sh`

Each compare wrapper now:

- confirms the VAIO is `remote_usable`
- captures pre-state with `scripts/show-vaio-gma500-state.sh`
- runs the default typed wrapper path
- confirms target health again
- runs the explicit legacy wrapper path
- confirms target health again
- captures post-state
- writes a concise `mode-summary.log`
- writes a structured `session-result.env`

The same work also exposed and fixed one real repo-side bug in
`scripts/summarize-vaio-sessions.sh`: it was sorting by path prefix instead of
session timestamp, so the new `logs/vaio-compare/` bundles did not surface at
the top of recent-session output. It now sorts by the timestamp prefix from the
session directory name instead.

## Hardware Validation

Validated on May 11, 2026 on `10.1.11.218` with:

- focused object compare:
  - `logs/vaio-compare/20260511-132342-sgx-object-mode-compare`
- focused submit compare:
  - `logs/vaio-compare/20260511-132458-sgx-submit-mode-compare`
- multi-submit compare:
  - `logs/vaio-compare/20260511-132628-sgx-multi-submit-mode-compare`

All three saved sessions completed with:

- `session_outcome=completed`
- `preflight_overall=remote_usable`
- `typed_rc=0`
- `typed_health_overall=remote_usable`
- `legacy_rc=0`
- `legacy_health_overall=remote_usable`
- `last_overall=remote_usable`

The target stayed clean after the compare runs:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

### Focused Object Compare

Typed focused object path:

- `typed_range_id=7`
- `typed_object_id=204`
- `typed_object_role=control`
- `typed_object_class=packet`
- `typed_object_gpu_use=control_ccb`
- `typed_object_context=pf`
- `typed_object_pf_bind_count=1`
- final typed cleanup:
  - `typed_range_sgx_object_count=0`
  - `typed_range_pf_bound_count=0`
  - `typed_object_after_drop_rc=-1 errno=2`

Explicit legacy focused object path:

- `legacy_range_id=217`
- `legacy_object_id=205`
- `legacy_object_node=/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_object`
- `legacy_objects_node=/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_objects`
- cleanup markers still present:
  - `legacy_has_gtt_after_drop=GTT_STATE_AFTER_DROP`
  - `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`

### Focused Submit Compare

Typed focused submit path:

- `typed_submit_id=48`
- `typed_range_ids=7 9 218 219 220`
- `typed_object_ids=206 207 208 209 210`
- progression:
  - `typed_issue_submit_id=48`
  - `typed_running_submit_id=48`
  - `typed_complete_submit_id=48`
  - `typed_validate_count=1`
  - `typed_issue_count=1`
  - `typed_transition_count=2`
  - `typed_owner_state=complete`
- final typed cleanup:
  - `typed_live_submit_count=0`
  - `typed_range_sgx_object_count=0`
  - `typed_range_pf_bound_count=0`

Explicit legacy focused submit path:

- `legacy_queue_mode=wrap`
- `legacy_submit_id=49`
- `legacy_range_ids=223 224 225 226 227`
- `legacy_object_ids=211 212 213 214 215`
- sysfs cleanup markers still present:
  - `legacy_has_submits_after_drop=SUBMITS_AFTER_DROP`
  - `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`

### Multi-Submit Compare

Typed multi-submit path:

- `typed_submit_setup=ioctl`
- `typed_object_setup=ioctl`
- `typed_range_discovery=ioctl`
- `typed_dependency_setup=ioctl`
- `typed_verification_mode=ioctl`
- `typed_dependency_interface=named`
- `typed_submit_a_id=50`
- `typed_submit_b_id=51`
- terminal typed state:
  - `typed_submit_validate_count=11`
  - `typed_submit_issue_count=2`
  - `typed_submit_transition_count=4`
  - `typed_submit_owner_state=complete`
  - `typed_live_submit_count=0`
  - `typed_live_object_count=0`
  - `typed_range_sgx_object_count=0`
  - `typed_range_pf_bound_count=0`

Explicit legacy multi-submit path:

- `legacy_dependency_interface=named`
- `legacy_submit_a_id=52`
- `legacy_submit_b_id=53`
- `legacy_range_ids=238 239 240 241 242 243 244 245 246 247`
- cleanup markers still present:
  - `legacy_has_submits_after_drop=SUBMITS_AFTER_DROP`
  - `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`
  - `legacy_has_coord_after_drop=COORD_STATE_AFTER_DROP`

### Session Summary Helper

Validated after the timestamp-sort fix with:

- `MAX_SESSIONS=8 ./scripts/summarize-vaio-sessions.sh`

The newest compare bundles now surface first:

- `20260511-132628-sgx-multi-submit-mode-compare`
- `20260511-132458-sgx-submit-mode-compare`
- `20260511-132342-sgx-object-mode-compare`

The compare wrappers now also persist the final remote module identity in their
saved `session-result.env` files:

- `final_driver_srcversion`
- `final_override_module_size`

That path was revalidated in:

- `logs/vaio-compare/20260511-133519-sgx-object-mode-compare`

with:

- `final_driver_srcversion=D43951B174F84BE870230AB`
- `final_override_module_size=385384`

The session-summary helper now uses those fields first and also falls back to
`post-state.log` or `pre-state.log` when older compare bundles do not have
structured result metadata yet. It also now collapses identical
typed-versus-legacy health into one `last_state=remote_usable` instead of
showing `remote_usable/remote_usable`.

## Conclusion

The repo now has a consistent regression harness for the wrapper migration:

- focused object typed vs explicit legacy
- focused submit typed vs explicit legacy
- multi-submit typed vs explicit legacy

That matters because wrapper behavior is now validated at the mode-dispatch
layer itself, not only through isolated typed or isolated legacy runs. The
session summary helper is also now good enough to surface those fresh compare
bundles first instead of burying them behind directory-name ordering.
