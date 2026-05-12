# SGX Lab Object-Op No-Sysfs Validation

Date: 2026-05-11

## Scope

Validate that the focused SGX lab object-op runner no longer needs
`poulsbo_sgx_objects` or `poulsbo_gtt_state` text reads to prove live object
adoption, metadata mutation, PF bind/unbind, persistence across userspace
close, and final drop.

This milestone covers:

- [scripts/run-vaio-sgx-ioctl-object-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-object-op.sh)

## What Changed

The object-op validator now stages and verifies through typed lab helpers only:

- [workloads/psb_sgx_lab_object_op.c](/root/Poulsbo/workloads/psb_sgx_lab_object_op.c)
- [workloads/psb_sgx_lab_object_query.c](/root/Poulsbo/workloads/psb_sgx_lab_object_query.c)
- [workloads/psb_sgx_lab_range_query.c](/root/Poulsbo/workloads/psb_sgx_lab_range_query.c)
- [workloads/psb_sgx_lab_gtt_state_query.c](/root/Poulsbo/workloads/psb_sgx_lab_gtt_state_query.c)
- [workloads/drm_probe.c](/root/Poulsbo/workloads/drm_probe.c)

Instead of scraping:

- `poulsbo_sgx_objects`
- `poulsbo_gtt_state`

the runner now proves each stage through:

- typed object-ID queries
- typed range-ID queries
- typed global GTT/MMU state queries

## Hardware Validation

The live bundle is:

- [logs/vaio-ioctl/20260511-123200-sgx-object-op-nosysfs-live](/root/Poulsbo/logs/vaio-ioctl/20260511-123200-sgx-object-op-nosysfs-live)

The relevant live log is:

- [ioctl-object-op.log](/root/Poulsbo/logs/vaio-ioctl/20260511-123200-sgx-object-op-nosysfs-live/ioctl-object-op.log)

No kernel deploy was needed for this step. The VAIO stayed on:

- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`

## Observed Result

The runner found one live mmapping GEM range by typed range query:

- `RANGE_ID=7`

Before adoption, the typed range and global GTT state already showed a clean
baseline:

- `range_sgx_object_id=0`
- `range_pf_bind_count=0`
- `gtt_state_range_sgx_object_count=0`
- `gtt_state_range_pf_bound_count=0`

After typed adopt:

- `OBJECT_ID=30`
- typed object query showed:
  - `object_role_name=generic`
  - `object_class_name=generic`
  - `object_gpu_use_name=generic`
  - `object_expected_context_name=auto`
  - `object_pf_bind_count=0`
- typed range query showed:
  - `range_sgx_object_id=30`
  - `range_pf_bind_count=0`
- typed GTT state showed:
  - `gtt_state_range_sgx_object_count=1`
  - `gtt_state_range_pf_bound_count=0`

After typed metadata updates:

- object query showed:
  - `object_role_name=control`
  - `object_class_name=packet`
  - `object_gpu_use_name=control_ccb`
  - `object_expected_context_name=pf`

After typed PF bind:

- object query showed:
  - `object_pf_bind_count=1`
- range query showed:
  - `range_sgx_object_id=30`
  - `range_pf_bind_count=1`
- global GTT state showed:
  - `gtt_state_range_sgx_object_count=1`
  - `gtt_state_range_pf_bound_count=1`

After typed PF unbind:

- object query showed:
  - `object_pf_bind_count=0`
- range query showed:
  - `range_sgx_object_id=30`
  - `range_pf_bind_count=0`
- global GTT state showed:
  - `gtt_state_range_sgx_object_count=1`
  - `gtt_state_range_pf_bound_count=0`

After the userspace holder exited:

- the typed object-ID query still resolved object `30`

After typed drop:

- object-ID query failed cleanly with:
  - `object_ioctl_ret=-1 errno=2`
- range query showed:
  - `range_sgx_object_id=0`
  - `range_pf_bind_count=0`
- global GTT state showed:
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

The nonzero range lifetime totals after drop remained expected:

- `range_pf_bind_total=8`
- `range_pf_unbind_total=8`

Those are cumulative counters, not per-run counters.

The final target state remained clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Conclusion

The focused object-op validator no longer depends on sysfs text reads for its
verification path.

That narrows the remaining sysfs dependence again:

- legacy compatibility fallbacks in older runners
- older non-ioctl wrappers kept for bring-up history
- any later state growth that still lacks a bounded typed read path
