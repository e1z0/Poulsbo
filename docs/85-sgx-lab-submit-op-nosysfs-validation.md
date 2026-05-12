# SGX Lab Submit-Op No-Sysfs Validation

Date: 2026-05-11

## Scope

Validate that the focused SGX lab submit-op runner no longer needs PCI-device
sysfs reads to prove typed object setup, submit setup, attach or detach,
kick/queue/block programming, submit progression, submit drop, and final object
cleanup.

This milestone covers:

- [scripts/run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)

## What Changed

The focused submit-op validator now stages and verifies through typed lab
helpers only:

- [workloads/drm_probe.c](/root/Poulsbo/workloads/drm_probe.c)
- [workloads/psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [workloads/psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
- [workloads/psb_sgx_lab_object_op.c](/root/Poulsbo/workloads/psb_sgx_lab_object_op.c)
- [workloads/psb_sgx_lab_object_query.c](/root/Poulsbo/workloads/psb_sgx_lab_object_query.c)
- [workloads/psb_sgx_lab_range_query.c](/root/Poulsbo/workloads/psb_sgx_lab_range_query.c)
- [workloads/psb_sgx_lab_gtt_state_query.c](/root/Poulsbo/workloads/psb_sgx_lab_gtt_state_query.c)

Instead of reading:

- `poulsbo_sgx_object`
- `poulsbo_sgx_objects`
- `poulsbo_sgx_submit`
- `poulsbo_sgx_submits`
- `poulsbo_sgx_submit_state`
- `poulsbo_sgx_coord_state`
- `poulsbo_gtt_state`

the runner now proves each stage through:

- typed stale cleanup through object-op and submit-op `cleanup`
- typed live GEM range discovery through range-query by index
- typed object-ID and range-ID readback after object setup
- typed global GTT/MMU state readback after PF binds and cleanup
- typed submit query by explicit ID and by focused submit
- typed coord readback around `issue`, `running`, and `complete`

## Hardware Validation

The live bundle is:

- [logs/vaio-ioctl/20260511-123800-sgx-submit-op-nosysfs-live](/root/Poulsbo/logs/vaio-ioctl/20260511-123800-sgx-submit-op-nosysfs-live)

The relevant live log is:

- [ioctl-submit-op.log](/root/Poulsbo/logs/vaio-ioctl/20260511-123800-sgx-submit-op-nosysfs-live/ioctl-submit-op.log)

No kernel deploy was needed for this step. The VAIO stayed on:

- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`

## Observed Result

The runner staged five held GEM ranges and adopted five SGX lab objects:

- `SUBMIT_ID=16`
- `RANGE_IDS=7 9 45 46 47`
- `OBJECT_IDS=36 37 38 39 40`

The typed baseline was already clean:

- `gtt_state_range_sgx_object_count=0`
- `gtt_state_range_pf_bound_count=0`

After typed object setup and PF bind, the runner verified:

- control object `36`:
  - `object_role_name=control`
  - `object_class_name=packet`
  - `object_gpu_use_name=control_ccb`
  - `object_expected_context_name=pf`
  - `object_pf_bind_count=1`
- status object `40`:
  - `object_role_name=status`
  - `object_class_name=state`
  - `object_gpu_use_name=status_block`
  - `object_expected_context_name=pf`
  - `object_pf_bind_count=1`
- control range `7`:
  - `range_sgx_object_id=36`
  - `range_pf_bind_count=1`
- global GTT state:
  - `gtt_state_range_sgx_object_count=5`
  - `gtt_state_range_pf_bound_count=5`

After typed attach, submit query by explicit ID and by focused submit both
resolved the same live submit and showed the expected five-object topology:

- `submit_control_object_id=36`
- `submit_pds_object_id=37`
- `submit_use_object_id=38`
- `submit_target_object_id=39`
- `submit_status_object_id=40`
- `submit_owner_state_name=queued`

The runner then programmed the same submit entirely through typed submit-op
verbs:

- `kick`
- `ccb`
- `queue`
- `status`
- `sync`
- `owner`
- `validate`
- `issue`
- `transition running`
- `transition complete`
- `drop`

The typed progression was then verified without reading the old text nodes:

- after `validate`:
  - `submit_validate_count=1`
  - `submit_issue_count=0`
  - `submit_transition_count=0`
- after `issue`:
  - `coord_issue_submit_id=16`
  - `submit_owner_state_name=queued`
  - `submit_issue_count=1`
  - `submit_global_issue_ticket=7`
- after `transition running`:
  - `coord_running_submit_id=16`
  - `submit_owner_state_name=running`
  - `submit_transition_count=1`
  - `submit_global_running_ticket=2`
- after `transition complete`:
  - `coord_complete_submit_id=16`
  - `submit_owner_state_name=complete`
  - `submit_transition_count=2`
  - `submit_global_complete_ticket=2`

The nonzero global tickets are cumulative live-driver counters, not per-run
counters. The runner validates them as advancing live state, not as fixed
constants.

After typed submit drop:

- focused-submit query failed cleanly with:
  - `submit_ioctl_ret=-1 errno=2`
- explicit-ID query for submit `16` failed cleanly with:
  - `submit_ioctl_ret=-1 errno=2`
- typed coord readback showed:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=5`

After typed object cleanup:

- `object_result_count=5`
- typed global GTT state returned:
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`
- typed object-ID query for control object `36` failed cleanly with:
  - `object_ioctl_ret=-1 errno=2`

The final target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Conclusion

The focused submit-op validator no longer depends on sysfs text reads for its
setup or verification path.

That narrows the remaining sysfs dependence again:

- legacy compatibility fallbacks in older runners
- older non-ioctl wrappers kept for bring-up history
- any later dependency or state-block growth that still lacks a bounded typed
  read path
