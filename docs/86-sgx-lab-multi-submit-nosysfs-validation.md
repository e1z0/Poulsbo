# SGX Lab Multi-Submit No-Sysfs Validation

Date: 2026-05-11

## Scope

Validate that the higher-level SGX multi-submit runner no longer needs the old
SGX or GTT sysfs text nodes for its verification path when all typed setup
paths are enabled.

This milestone covers:

- [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
- [scripts/run-vaio-sgx-multi-submit-nosysfs.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit-nosysfs.sh)

## What Changed

The multi-submit runner now has a strict typed verification mode driven by:

- `USE_IOCTL_VERIFICATION=1`

That mode requires the already-validated typed setup paths:

- `USE_IOCTL_OBJECT_SETUP=1`
- `USE_IOCTL_SUBMIT_SETUP=1`
- `USE_IOCTL_DEPENDENCY_SETUP=1`
- `USE_IOCTL_RANGE_DISCOVERY=1`

When enabled, the runner stages and uses typed helpers for verification:

- [workloads/psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [workloads/psb_sgx_lab_gtt_state_query.c](/root/Poulsbo/workloads/psb_sgx_lab_gtt_state_query.c)
- [workloads/psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)
- [workloads/psb_sgx_lab_range_query.c](/root/Poulsbo/workloads/psb_sgx_lab_range_query.c)
- [workloads/psb_sgx_lab_object_op.c](/root/Poulsbo/workloads/psb_sgx_lab_object_op.c)
- [workloads/psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)

Instead of reading:

- `poulsbo_sgx_object`
- `poulsbo_sgx_objects`
- `poulsbo_sgx_submit`
- `poulsbo_sgx_submits`
- `poulsbo_sgx_submit_state`
- `poulsbo_sgx_coord_state`
- `poulsbo_gtt_state`

the runner now verifies through:

- typed stale cleanup through submit-op and object-op `cleanup`
- typed live GEM range discovery through range-query by index
- typed focused-submit and coord queries
- typed global GTT/MMU state queries
- typed end-to-end dump snapshots

The dedicated wrapper `run-vaio-sgx-multi-submit-nosysfs.sh` locks in the
full four-lane dependency-slot-clear scenario with all typed setup paths and
typed verification enabled.

## Hardware Validation

The live bundle is:

- [logs/vaio-ioctl/20260511-124033-sgx-multi-submit-nosysfs-live](/root/Poulsbo/logs/vaio-ioctl/20260511-124033-sgx-multi-submit-nosysfs-live)

The relevant live log is:

- [ioctl-multi-submit-nosysfs.log](/root/Poulsbo/logs/vaio-ioctl/20260511-124033-sgx-multi-submit-nosysfs-live/ioctl-multi-submit-nosysfs.log)

No kernel deploy was needed for this step. The VAIO stayed on:

- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`

## Observed Result

The saved run reported the fully typed mode directly:

- `SUBMIT_SETUP=ioctl`
- `OBJECT_SETUP=ioctl`
- `RANGE_DISCOVERY=ioctl`
- `DEPENDENCY_SETUP=ioctl`
- `VERIFICATION_MODE=ioctl`
- `DEPENDENCY_INTERFACE=indexed`

It staged the full four-lane dependency-clear scenario:

- `SUBMIT_A_ID=22`
- `SUBMIT_B_ID=23`
- `SUBMIT_C_ID=24`
- `SUBMIT_D_ID=25`
- `SUBMIT_E_ID=26`
- `RANGE_IDS=75..99`
- `OBJECT_IDS_A=66 67 68 69 70`
- `OBJECT_IDS_B=71 72 73 74 75`
- `OBJECT_IDS_C=76 77 78 79 80`
- `OBJECT_IDS_D=81 82 83 84 85`
- `OBJECT_IDS_E=86 87 88 89 90`

The typed setup and verification path showed the expected live counts during
the staged state:

- `coord_live_submit_count=5`
- `coord_live_object_count=25`
- `gtt_state_range_sgx_object_count=25`
- `gtt_state_range_pf_bound_count=25`

The typed dependency-clear branch still behaved correctly without sysfs
verification:

- submit B kept lanes `1..3` active and satisfied
- lane `4` cleared live to:
  - `dependency[4]_submit_id=0`
  - `dependency[4]_flags=0x0`
- after that clear, submit B still completed before submit E
- submit E then completed independently later in the same run

The typed post-run validation still resolved all five submits before final
drop:

- submit A:
  - `submit_validate_count=5`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- submit B:
  - `submit_validate_count=22`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- submit C:
  - `submit_validate_count=5`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- submit D:
  - `submit_validate_count=5`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- submit E:
  - `submit_validate_count=5`
  - `submit_issue_count=1`
  - `submit_transition_count=2`

After typed submit and object teardown, the same run returned cleanly to zero:

- `coord_live_submit_count=0`
- `gtt_state_range_sgx_object_count=0`
- `gtt_state_range_pf_bound_count=0`

The final target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Conclusion

The higher-level SGX multi-submit runner no longer depends on sysfs text reads
for its verification path when the typed object, submit, dependency, and range
paths are enabled together.

That narrows the remaining sysfs dependence again:

- legacy compatibility fallback paths in the same runner
- older non-ioctl wrappers kept for bring-up history
- any later dependency or state-block growth that still lacks a bounded typed
  read path
