# SGX Deadlock-Fix Candidate Validation

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Candidate override: `gma500_gfx` `srcversion BB080942FE121033007E2EC`

## Scope

This batch closed the still-open validation gap from
[docs/113-sgx-submit-cleanup-deadlock-fix.md](/root/Poulsbo/docs/113-sgx-submit-cleanup-deadlock-fix.md):
the kernel deadlock fix is now validated on the actual candidate override, not
just on the rollback-restored previous module.

It also fixed one runner-side verification bug that the new candidate exposed:
the typed multi-submit refill-cycle assertions still assumed the persisted
retired snapshot would report `submit_retired_completed_packets=1`, while the
live typed snapshot correctly preserved the last retired completion count of
`2` from the previous cycle.

## What Changed

Runner-side only:

- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
  now has:
  - `require_snapshot_value`
  - `require_snapshot_int_ge`
  - `assert_retired_persisted_snapshot_file`
- the typed refill-cycle verification now treats the retired snapshot as a
  persisted last-complete image:
  - exact on count, dispatch count, owner state, queue count, and dependency
    count
  - lower-bounded on `submit_retired_completed_packets`
- assertion failures now print the offending snapshot contents instead of
  failing silently through `grep -q`

No kernel-tree files changed in this follow-up; the kernel candidate stayed the
same `BB080942FE121033007E2EC` build that already contained the submit-sysfs
deadlock fix.

## Deployment

The candidate override was redeployed with the guarded session path:

- [logs/vaio-deploy/20260511-203613-deadlock-fix-redeploy](/root/Poulsbo/logs/vaio-deploy/20260511-203613-deadlock-fix-redeploy)

Result:

- preflight target state: `overall=remote_usable`
- previous loaded override before redeploy:
  - `srcversion 801B0633670F92DA0659014`
- guarded reboot returned cleanly
- post-reboot loaded override:
  - `srcversion BB080942FE121033007E2EC`
  - `override_module_size=408344`
- rollback guard cleared successfully

## Focused Submit Validation

The focused typed submit-op path passed on the candidate override:

- [logs/vaio-ioctl/20260511-203809-sgx-submit-op-deadlock-fix-candidate/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-203809-sgx-submit-op-deadlock-fix-candidate/summary.log)

Key result:

- the run still reaches the full execution-shaped path on the candidate:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_retired_owner_state_name=complete`
- final typed cleanup still returned:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

## Multi-Submit Validation

The first typed/default multi-submit run on the candidate exposed the stale
runner assertion:

- [logs/vaio-ioctl/20260511-203848-sgx-multi-submit-default-deadlock-fix-candidate/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-203848-sgx-multi-submit-default-deadlock-fix-candidate/summary.log)

Important result:

- the failure happened after the scenario had already progressed into refill
  cycle validation
- the driver did not deadlock
- the mismatch was in the runner’s expectation for the persisted retired
  snapshot during refill

After fixing the typed refill-cycle assertions, the same safe wrapper path
completed cleanly on the same candidate override:

- [logs/vaio-ioctl/20260511-204244-sgx-multi-submit-default-deadlock-fix-candidate-rerun/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-204244-sgx-multi-submit-default-deadlock-fix-candidate-rerun/summary.log)

Key results from the clean rerun:

- submit A completed the full two-refill scenario with:
  - `submit_prepare_count=1`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
- submit B completed the dependency-gated path with:
  - `submit_prepare_count=1`
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
  - `submit_dependency_active_count=1` while the dependency lane was live
- final typed cleanup returned:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

## Final Target State

Post-validation target state is clean:

- `overall=remote_usable`
- loaded override module:
  - `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- loaded override `srcversion`:
  - `BB080942FE121033007E2EC`
- `override_module_size=408344`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Conclusion

The submit-sysfs deadlock fix is now validated on the actual candidate module.

The remaining issue from this batch was not a kernel regression; it was a
runner-side typed refill assertion that lagged behind the current retired
snapshot semantics. That is now fixed and documented.

This closes the old “candidate is built but not yet hardware-validated” state
from [docs/113-sgx-submit-cleanup-deadlock-fix.md](/root/Poulsbo/docs/113-sgx-submit-cleanup-deadlock-fix.md).

The next useful work item is back where it should be: execution-oriented SGX
kernel structure work above the current `prepare -> arm -> emit -> launch ->
dispatch -> retired` lab path.
