# SGX Multi-Submit Wrapper Mode Gate

## What Changed

Added a wrapper dispatcher:

- `scripts/run-vaio-sgx-multi-submit-default.sh`

It selects the multi-submit driver mode with:

- `SGX_MULTI_SUBMIT_DRIVER_MODE=typed`
- `SGX_MULTI_SUBMIT_DRIVER_MODE=legacy`

Current behavior:

- default: `typed`
- explicit legacy opt-in: `legacy`

Mode selection is:

- `typed`:
  - `scripts/run-vaio-sgx-multi-submit-typed.sh`
- `legacy`:
  - `scripts/run-vaio-sgx-multi-submit.sh`

The dependency-oriented scenario wrappers now call the dispatcher instead of
calling the typed wrapper directly:

- `scripts/run-vaio-sgx-dependency-slot-clear.sh`
- `scripts/run-vaio-sgx-fourth-dependency-slot.sh`
- `scripts/run-vaio-sgx-indexed-dependency-array.sh`
- `scripts/run-vaio-sgx-third-dependency-lane.sh`
- `scripts/run-vaio-sgx-multi-source-dependency.sh`
- `scripts/run-vaio-sgx-multi-source-ticket-dependency.sh`
- `scripts/run-vaio-sgx-dependency-drop.sh`
- `scripts/run-vaio-sgx-dependency-satisfied-drop.sh`
- `scripts/run-vaio-sgx-dependency-unrelated-submit.sh`

This keeps the typed path as the default while making the older direct
multi-submit script an explicit compatibility mode instead of an invisible
fallback.

## Hardware Validation

Validated on May 11, 2026 on `10.1.11.218` with:

- wrapper run:
  - `logs/vaio-ioctl/20260511-130053-sgx-indexed-wrapper-default-live`

Final target state after the run:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

The validated scenario was the indexed three-lane dependency wrapper:

- `scripts/run-vaio-sgx-indexed-dependency-array.sh`

The saved run reported the typed path at the wrapper level:

- `SUBMIT_SETUP=ioctl`
- `OBJECT_SETUP=ioctl`
- `RANGE_DISCOVERY=ioctl`
- `DEPENDENCY_SETUP=ioctl`
- `VERIFICATION_MODE=ioctl`
- `DEPENDENCY_INTERFACE=indexed`

The live staged state was correct for the indexed wrapper scenario:

- `coord_live_submit_count=4`
- `coord_live_object_count=20`
- `gtt_state_range_sgx_object_count=20`
- `gtt_state_range_pf_bound_count=20`

The dependent submit showed the expected indexed lane bindings:

- lane 1:
  - `dependency[1]_submit_id=33`
- lane 2:
  - `dependency[2]_submit_id=35`
- lane 3:
  - `dependency[3]_submit_id=36`

The same wrapper run completed and cleaned up through the typed path:

- terminal submit-side counts reached:
  - `submit_validate_count=17`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- final global state returned to zero:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

## Conclusion

The multi-submit wrapper layer now behaves like the focused helper layer:

- typed path by default
- explicit legacy escape hatch

That keeps the current validated default aligned with the bounded ioctl path
without removing the older direct multi-submit bring-up route when comparison
against the legacy behavior is still useful.
