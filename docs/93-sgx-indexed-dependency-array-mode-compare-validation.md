# SGX Indexed Dependency Array Mode-Compare Validation

## What Changed

Added a thin typed-versus-legacy compare wrapper for the indexed dependency
array scenario:

- `scripts/run-vaio-sgx-indexed-dependency-array-mode-compare.sh`

This wrapper does not duplicate compare-session logic. Instead it reuses the
generic multi-submit compare harness:

- `scripts/run-vaio-sgx-multi-submit-mode-compare.sh`

with these scenario-specific defaults:

- `SESSION_NAME=sgx-indexed-dependency-array-mode-compare`
- `SESSION_KIND=sgx_indexed_dependency_array_mode_compare`
- `MULTI_SUBMIT_SCRIPT=scripts/run-vaio-sgx-indexed-dependency-array.sh`

The generic compare harness was extended so `SESSION_KIND` can now be
overridden by thin scenario wrappers instead of forcing every scenario into the
same generic `sgx_multi_submit_mode_compare` bucket.

## Hardware Validation

Validated on May 11, 2026 on `10.1.11.218` with:

- `logs/vaio-compare/20260511-134740-sgx-indexed-dependency-array-mode-compare`

Saved session result:

- `session_kind=sgx_indexed_dependency_array_mode_compare`
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

The default wrapper path stayed on the bounded ioctl stack:

- `typed_submit_setup=ioctl`
- `typed_object_setup=ioctl`
- `typed_range_discovery=ioctl`
- `typed_dependency_setup=ioctl`
- `typed_verification_mode=ioctl`
- `typed_dependency_interface=indexed`

Saved typed state:

- `typed_submit_a_id=64`
- `typed_submit_b_id=65`
- `typed_submit_c_id=66`
- `typed_submit_d_id=67`
- `typed_range_ids=300 301 302 303 304 305 306 307 308 309 310 311 312 313 314 315 316 317 318 319`

Terminal typed state:

- `typed_submit_validate_count=5`
- `typed_submit_issue_count=1`
- `typed_submit_transition_count=2`
- `typed_submit_owner_state=complete`
- `typed_live_submit_count=0`
- `typed_live_object_count=0`
- `typed_range_sgx_object_count=0`
- `typed_range_pf_bound_count=0`

### Explicit Legacy Wrapper Result

The explicit legacy branch stayed on the older sysfs-heavy runner:

- `legacy_dependency_interface=indexed`
- `legacy_submit_a_id=68`
- `legacy_submit_b_id=69`
- `legacy_submit_c_id=70`
- `legacy_submit_d_id=71`
- `legacy_range_ids=320 321 322 323 324 325 326 327 328 329 330 331 332 333 334 335 336 337 338 339`

Legacy cleanup still ends in its expected text markers:

- `legacy_has_submits_after_drop=SUBMITS_AFTER_DROP`
- `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`
- `legacy_has_coord_after_drop=COORD_STATE_AFTER_DROP`

## Session Summary Helper

Validated after this run with:

- `MAX_SESSIONS=6 ./scripts/summarize-vaio-sessions.sh`

The new indexed-array compare bundle now appears as its own session kind:

- `session_kind=sgx_indexed_dependency_array_mode_compare`

That matters because it keeps dependency-scenario compare runs distinct from:

- the generic multi-submit compare bundle
- the dependency-slot-clear compare bundle

## Conclusion

The compare layer now scales one step better:

- one generic multi-submit compare harness
- thin scenario wrappers with explicit `SESSION_KIND`
- distinct saved bundles per dependency scenario

That keeps the session logic centralized while still giving each higher-level
dependency workflow its own typed-versus-legacy regression artifact.
