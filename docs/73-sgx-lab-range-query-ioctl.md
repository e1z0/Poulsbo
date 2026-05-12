# SGX Lab Range Query Ioctl Validation

Date: 2026-05-11

## Scope

Add a bounded live-range query ioctl and use it to move the multi-submit
runner's hot-path range discovery off the page-sized `poulsbo_gtt_state` sysfs
dump.

Validated mode:

- `USE_IOCTL_RANGE_DISCOVERY=1`
- `USE_IOCTL_OBJECT_SETUP=1`
- `USE_IOCTL_SUBMIT_SETUP=1`
- `USE_IOCTL_DEPENDENCY_SETUP=1`

## Files Changed

- [third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
- [workloads/Makefile](/root/Poulsbo/workloads/Makefile)
- [workloads/psb_sgx_lab_range_query.c](/root/Poulsbo/workloads/psb_sgx_lab_range_query.c)
- [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)

## What Was Added

Kernel-side:

- new bounded ioctl query:
  - `DRM_IOCTL_PSB_SGX_LAB_QUERY_RANGE`
- new typed range-query structure:
  - `drm_psb_sgx_lab_range_query`
- new range-query flags:
  - `DRM_PSB_SGX_LAB_QUERY_RANGE_USE_ID`
  - `DRM_PSB_SGX_LAB_QUERY_RANGE_FILTER_GEM`
- new typed range categories:
  - `unknown`
  - `gem`
  - `fb`
  - `cursor`
  - `other`

Userspace-side:

- new 32-bit helper:
  - [psb_sgx_lab_range_query.c](/root/Poulsbo/workloads/psb_sgx_lab_range_query.c)
- it supports:
  - `index <query_index> [gem]`
  - `id <range_id> [gem]`
- it prints typed live-range fields including:
  - resolved range ID and index
  - type/name
  - `start/end/len`
  - `gpu_va`
  - `offset`
  - `stolen/in_gart/mmapping/npage/roll`
  - SGX object ownership and PF-bind counters

Runner-side:

- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
  now supports:
  - `USE_IOCTL_RANGE_DISCOVERY=1`
  - `RANGE_QUERY_SCAN_MAX=<n>`
- with that mode enabled, the runner:
  - stages the new range-query helper to the VAIO
  - enumerates live GEM ranges by typed ioctl index queries
  - computes new candidate range IDs by comparing before/during live GEM lists
  - no longer depends on scraping `poulsbo_gtt_state` to find fresh GEM range
    IDs in the hot path

## Full Hardware Validation

Local build and deploy:

- rebuilt 32-bit workloads with:
  - `./scripts/docker-build-workloads-i386.sh`
- rebuilt the locally patched Ubuntu `5.4.0-150-generic` override with:
  - `./scripts/docker-build-vaio-gma500.sh`
- guarded-deployed the new module to the VAIO

Live target after deploy:

- `driver_srcversion=8D7D3CAE32683F70FE739B7`
- `override_module_size=381292`
- `rollback_pending_present=0`
- `rollback_backup_present=1`
- target health:
  - `overall=remote_usable`

Clean live bundle:

- [logs/vaio-ioctl/20260511-103211-sgx-range-query-live](/root/Poulsbo/logs/vaio-ioctl/20260511-103211-sgx-range-query-live)

That run completed with:

- `OBJECT_SETUP=ioctl`
- `SUBMIT_SETUP=ioctl`
- `RANGE_DISCOVERY=ioctl`
- `DEPENDENCY_SETUP=ioctl`
- `DEPENDENCY_INTERFACE=indexed`

The typed range-discovery path successfully identified and used:

- `RANGE_IDS=10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34`

That is the same five-submit object set later mapped as:

- `OBJECT_IDS_A=1 2 3 4 5`
- `OBJECT_IDS_B=6 7 8 9 10`
- `OBJECT_IDS_C=11 12 13 14 15`
- `OBJECT_IDS_D=16 17 18 19 20`
- `OBJECT_IDS_E=21 22 23 24 25`

The scenario still validated the later dependency behavior:

- `SUBMIT_B_AFTER_DEPCLEAR4` showed:
  - `dependency4_submit_id=0`
  - `dependency4_offset=0x0`
  - `dependency4_required_ticket=0`
  - `dependency4_flags=0x0`
  - `dependency4_config_count=0`
- `COORD_STATE_AFTER_SUBMIT_E_POST_CLEAR` showed:
  - `issue_ticket_counter=5`
  - `running_ticket_counter=5`
  - `complete_ticket_counter=5`
  - `live_submit_count=5`
- final cleanup still completed cleanly:
  - `OBJECTS_AFTER_DROP` reported `objects_shown=0 objects_total=0`
  - `COORD_STATE_AFTER_DROP` reported `live_submit_count=0`

Saved post-run state:

- [health-after.log](/root/Poulsbo/logs/vaio-ioctl/20260511-103211-sgx-range-query-live/health-after.log)
- [state-after.log](/root/Poulsbo/logs/vaio-ioctl/20260511-103211-sgx-range-query-live/state-after.log)
- [sgx-state-after.log](/root/Poulsbo/logs/vaio-ioctl/20260511-103211-sgx-range-query-live/sgx-state-after.log)
- [gtt-state-after.log](/root/Poulsbo/logs/vaio-ioctl/20260511-103211-sgx-range-query-live/gtt-state-after.log)

Those confirm:

- `overall=remote_usable`
- `driver_srcversion=8D7D3CAE32683F70FE739B7`
- `rollback_pending_present=0`
- `rollback_backup_present=1`
- `fault_count=0`
- `bif_fault=0x000000`
- `objects_shown=0`
- no leaked SGX lab object ownership in the post-run GTT view

## Conclusion

The real multi-submit dependency-slot-clear scenario is now validated on the
VAIO with:

- range discovery on the new typed range-query ioctl path
- object setup on the object-op ioctl path
- submit setup and progression on the submit-op ioctl path
- indexed dependency-slot setup and clear on the submit-op ioctl path

That narrows the remaining sysfs dependence again. The runner no longer needs
the page-sized `poulsbo_gtt_state` text dump to discover fresh GEM ranges in
its hot path. The remaining sysfs use in this area is now mainly:

- human-readable inspection dumps
- stale cleanup convenience
- any later lab-only state growth that still lacks a bounded ioctl surface
