# SGX Dependency Wrapper Typed Defaults

## What Changed

Added a generic typed multi-submit wrapper:

- `scripts/run-vaio-sgx-multi-submit-typed.sh`

and moved the dependency-oriented scenario wrappers onto it:

- `scripts/run-vaio-sgx-dependency-slot-clear.sh`
- `scripts/run-vaio-sgx-fourth-dependency-slot.sh`
- `scripts/run-vaio-sgx-indexed-dependency-array.sh`
- `scripts/run-vaio-sgx-third-dependency-lane.sh`
- `scripts/run-vaio-sgx-multi-source-dependency.sh`
- `scripts/run-vaio-sgx-multi-source-ticket-dependency.sh`
- `scripts/run-vaio-sgx-dependency-drop.sh`
- `scripts/run-vaio-sgx-dependency-satisfied-drop.sh`
- `scripts/run-vaio-sgx-dependency-unrelated-submit.sh`

The new generic wrapper only forces the already-validated typed path:

- `USE_IOCTL_OBJECT_SETUP=1`
- `USE_IOCTL_SUBMIT_SETUP=1`
- `USE_IOCTL_DEPENDENCY_SETUP=1`
- `USE_IOCTL_RANGE_DISCOVERY=1`
- `USE_IOCTL_VERIFICATION=1`

It does not force any specific scenario shape, so the older dependency wrappers
keep owning their own source-count, ticket, and drop/clear behavior.

## Local Fix

The first live rerun failed locally with:

- `Permission denied`

because the new wrapper file had been added without the executable bit. That
was corrected with `chmod +x scripts/run-vaio-sgx-multi-submit-typed.sh`
before the validated rerun.

## Hardware Validation

Validated on May 11, 2026 on `10.1.11.218` with:

- wrapper run:
  - `logs/vaio-ioctl/20260511-124744-sgx-dependency-slot-clear-typed-wrapper-live-retry`
- target state after the run:
  - `overall=remote_usable`
  - `driver_srcversion=D43951B174F84BE870230AB`
  - `override_module_size=385384`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

The saved wrapper run reported the typed path all the way through:

- `SUBMIT_SETUP=ioctl`
- `OBJECT_SETUP=ioctl`
- `RANGE_DISCOVERY=ioctl`
- `DEPENDENCY_SETUP=ioctl`
- `VERIFICATION_MODE=ioctl`
- `DEPENDENCY_INTERFACE=indexed`

The same run staged the full live dependency-clear scenario through the wrapper:

- `coord_live_submit_count=5`
- `coord_live_object_count=25`
- `gtt_state_range_sgx_object_count=25`
- `gtt_state_range_pf_bound_count=25`

The slot-4 dependency lane on submit B was observed both before and after the
clear:

- before clear:
  - `dependency[4]_submit_id=31`
  - `dependency[4]_required_ticket=4`
  - `dependency[4]_flags=0xb`
- after clear:
  - `dependency[4]_submit_id=0`
  - `dependency[4]_required_ticket=0`
  - `dependency[4]_flags=0x0`

The final typed cleanup returned the system to zero live SGX lab state:

- `coord_live_submit_count=0`
- `coord_live_object_count=0`
- `gtt_state_range_sgx_object_count=0`
- `gtt_state_range_pf_bound_count=0`
- `fault_count=0`
- `bif_fault=0x000000`

## Conclusion

The dependency-specific wrapper layer no longer defaults callers back onto the
legacy sysfs verification path. The high-level dependency scenarios now inherit
the already-validated typed object, submit, dependency, range, coord, GTT, and
dump path by default, while keeping their scenario-specific control variables.
