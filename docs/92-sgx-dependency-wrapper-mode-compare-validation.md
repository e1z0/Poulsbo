# SGX Dependency Wrapper Mode-Compare Validation

## What Changed

Added a direct typed-versus-legacy compare wrapper for the dependency-specific
multi-submit layer:

- `scripts/run-vaio-sgx-dependency-slot-clear-mode-compare.sh`

This compare wrapper is narrower than the generic multi-submit compare session.
It targets the default dependency alias wrapper itself:

- typed/default path:
  - `scripts/run-vaio-sgx-dependency-slot-clear.sh`
- explicit legacy path:
  - `SGX_MULTI_SUBMIT_DRIVER_MODE=legacy scripts/run-vaio-sgx-dependency-slot-clear.sh`

Like the earlier compare wrappers, it now:

- confirms the VAIO is `remote_usable`
- captures pre-state with `scripts/show-vaio-gma500-state.sh`
- runs the default typed wrapper path
- rechecks health
- runs the explicit legacy wrapper path
- rechecks health again
- captures post-state
- writes `mode-summary.log`
- writes `session-result.env`

## Hardware Validation

Validated on May 11, 2026 on `10.1.11.218` with:

- `logs/vaio-compare/20260511-134016-sgx-dependency-slot-clear-mode-compare`

Saved session result:

- `session_kind=sgx_dependency_slot_clear_mode_compare`
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

### Typed Wrapper Result

The default wrapper path stayed on the bounded ioctl stack end to end:

- `typed_submit_setup=ioctl`
- `typed_object_setup=ioctl`
- `typed_range_discovery=ioctl`
- `typed_dependency_setup=ioctl`
- `typed_verification_mode=ioctl`
- `typed_dependency_interface=indexed`

Saved typed state:

- `typed_submit_a_id=54`
- `typed_submit_b_id=55`
- `typed_submit_c_id=56`
- `typed_submit_d_id=57`
- `typed_submit_e_id=58`
- `typed_range_ids=250 251 252 253 254 255 256 257 258 259 260 261 262 263 264 265 266 267 268 269 270 271 272 273 274`

The slot-clear behavior still landed exactly where expected:

- `typed_clear_dependency4_after_d=1`
- `typed_run_submit_e_after_clear=1`
- `typed_dependency4_submit_id_after_clear=0`
- `typed_dependency4_flags_after_clear=0x0`
- `typed_submit_b_owner_state_after_post_clear=complete`
- `typed_submit_b_validate_count_after_post_clear=21`
- `typed_submit_b_issue_count_after_post_clear=1`
- `typed_submit_b_transition_count_after_post_clear=2`

Final typed cleanup returned to zero:

- `typed_live_submit_count_after_drop=0`
- `typed_live_object_count_after_drop=0`
- `typed_range_sgx_object_count_after_drop=0`
- `typed_range_pf_bound_count_after_drop=0`

### Explicit Legacy Wrapper Result

The explicit legacy path stayed on the direct sysfs-heavy runner:

- `legacy_submit_setup=sysfs`
- `legacy_object_setup=sysfs`
- `legacy_range_discovery=sysfs`
- `legacy_dependency_setup=sysfs`
- `legacy_verification_mode=sysfs`
- `legacy_dependency_interface=indexed`

Saved legacy state:

- `legacy_submit_a_id=59`
- `legacy_submit_b_id=60`
- `legacy_submit_c_id=61`
- `legacy_submit_d_id=62`
- `legacy_submit_e_id=63`
- `legacy_range_ids=275 276 277 278 279 280 281 282 283 284 285 286 287 288 289 290 291 292 293 294 295 296 297 298 299`

The legacy branch also preserved the expected slot-clear flow:

- `legacy_clear_dependency4_after_d=1`
- `legacy_run_submit_e_after_clear=1`

Legacy cleanup still ends in its older text markers:

- `legacy_has_submits_after_drop=SUBMITS_AFTER_DROP`
- `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`
- `legacy_has_coord_after_drop=COORD_STATE_AFTER_DROP`

## Session Summary Helper

Validated after this run with:

- `MAX_SESSIONS=6 ./scripts/summarize-vaio-sessions.sh`

The new dependency compare bundle now appears correctly among recent sessions
with:

- `session_kind=sgx_dependency_slot_clear_mode_compare`
- `last_state=remote_usable`
- `module_srcversion=D43951B174F84BE870230AB`

That matters because it confirms the newer compare session kind is handled the
same way as the earlier object, submit, and generic multi-submit compare
bundles.

## Conclusion

The repo now has a wrapper-level compare harness for the dependency-specific
alias layer, not just for the three broader wrapper families. That closes the
gap between:

- focused object wrapper dispatch
- focused submit wrapper dispatch
- generic multi-submit wrapper dispatch
- dependency-specific wrapper dispatch

So future wrapper-default changes can be regression-checked at the exact layer
where users actually enter the dependency scenarios, while still keeping the
explicit legacy path available and validated.
