# SGX Lab GTT-State Query Ioctl Validation

Date: 2026-05-11

## Scope

Add a bounded typed GTT/MMU state query ioctl so the live global GTT state path
no longer depends entirely on `poulsbo_gtt_state` text parsing.

This milestone covers:

- [third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
- [workloads/Makefile](/root/Poulsbo/workloads/Makefile)
- [workloads/psb_sgx_lab_gtt_state_query.c](/root/Poulsbo/workloads/psb_sgx_lab_gtt_state_query.c)
- [scripts/run-vaio-sgx-ioctl-gtt-state.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-gtt-state.sh)

## What Was Added

Kernel-side:

- new bounded ioctl query:
  - `DRM_IOCTL_PSB_SGX_LAB_QUERY_GTT_STATE`
- new typed GTT/MMU snapshot structure:
  - `drm_psb_sgx_lab_gtt_state_query`

The ioctl mirrors the current kernel-owned global GTT/MMU state, including:

- global GTT layout:
  - `gtt_start`
  - `gatt_start`
  - `mmu_gatt_start`
  - `gatt_free_offset`
  - `stolen_size`
  - `vram_stolen_size`
- MMU last-op state:
  - `mmu_last_op`
  - `mmu_last_address`
  - `mmu_last_num_pages`
  - `mmu_last_type`
  - `mmu_last_hw_context`
  - `mmu_last_jiffies`
- MMU counters:
  - `mmu_flush_count`
  - `mmu_set_pd_context_count`
  - `mmu_insert_pfn_sequence_count`
  - `mmu_insert_pages_count`
  - `mmu_remove_pfn_sequence_count`
  - `mmu_remove_pages_count`
  - `mmu_default_pd_context`
  - `mmu_pf_pd_context`
- live range counters:
  - `range_total_count`
  - `range_gem_count`
  - `range_fb_count`
  - `range_cursor_count`
  - `range_other_count`
  - `range_in_gart_count`
  - `range_mmapping_count`
  - `range_sgx_object_count`
  - `range_pf_bound_count`

Userspace-side:

- new 32-bit helper:
  - [psb_sgx_lab_gtt_state_query.c](/root/Poulsbo/workloads/psb_sgx_lab_gtt_state_query.c)
- it prints the typed GTT/MMU state fields returned by the ioctl
- it uses raw `ioctl()` for the custom lab request

Runner-side:

- new direct live validator:
  - [run-vaio-sgx-ioctl-gtt-state.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-gtt-state.sh)
- it stages:
  - `drm_probe`
  - `psb_sgx_lab_gtt_state_query`
- it compares:
  - ioctl GTT-state readout
  - sysfs GTT-state readout
- it does that:
  - before any held GEM ranges
  - while three held GEM ranges are still live
  - after those ranges are released again

## Local Build And Deploy

Rebuilt locally with:

- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy result:

- `driver_srcversion=8CB35627334C3560115D1F6`
- `override_module_size=383644`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Hardware Validation

The live bundle is:

- [logs/vaio-ioctl/20260511-110215-gtt-state-query-live](/root/Poulsbo/logs/vaio-ioctl/20260511-110215-gtt-state-query-live)

The target remained clean after the run:

- `overall=remote_usable`
- `driver_srcversion=8CB35627334C3560115D1F6`
- `override_module_size=383644`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Observed Result

Before the held GEM mappings:

- `IOCTL_GTT_STATE_BEFORE` succeeded with:
  - `gtt_state_ioctl_ret=0 errno=0`
  - `gtt_state_gtt_start=0x94280000`
  - `gtt_state_gatt_start=0x80000000`
  - `gtt_state_mmu_gatt_start=0xe0000000`
  - `gtt_state_gatt_free_offset=0xe0800000`
  - `gtt_state_mmu_last_op_name=flush`
  - `gtt_state_mmu_flush_count=2`
  - `gtt_state_mmu_insert_pages_count=2`
  - `gtt_state_mmu_remove_pages_count=0`
  - `gtt_state_range_total_count=7`
  - `gtt_state_range_gem_count=4`
  - `gtt_state_range_in_gart_count=5`
  - `gtt_state_range_mmapping_count=2`
- `SYSFS_GTT_STATE_BEFORE` matched the same MMU summary:
  - `mmu_last_op=flush`
  - `flush=2`
  - `insert_pages=2`
  - `remove_pages=0`

While the three held dumb GEM ranges remained live:

- `IOCTL_GTT_STATE_DURING` showed the expected live growth:
  - `gtt_state_mmu_flush_count=5`
  - `gtt_state_mmu_insert_pages_count=5`
  - `gtt_state_mmu_remove_pages_count=0`
  - `gtt_state_range_total_count=10`
  - `gtt_state_range_gem_count=7`
  - `gtt_state_range_in_gart_count=8`
  - `gtt_state_range_mmapping_count=5`
- the stable layout fields did not move:
  - `gtt_state_gtt_start=0x94280000`
  - `gtt_state_gatt_start=0x80000000`
  - `gtt_state_mmu_gatt_start=0xe0000000`
  - `gtt_state_gatt_free_offset=0xe0800000`
- `SYSFS_GTT_STATE_DURING` matched the same MMU summary:
  - `flush=5`
  - `insert_pages=5`
  - `remove_pages=0`

After those held ranges exited:

- `IOCTL_GTT_STATE_AFTER` showed the expected cleanup:
  - `gtt_state_mmu_flush_count=8`
  - `gtt_state_mmu_insert_pages_count=5`
  - `gtt_state_mmu_remove_pages_count=3`
  - `gtt_state_range_total_count=7`
  - `gtt_state_range_gem_count=4`
  - `gtt_state_range_in_gart_count=5`
  - `gtt_state_range_mmapping_count=2`
- `SYSFS_GTT_STATE_AFTER` matched the same final MMU summary:
  - `flush=8`
  - `insert_pages=5`
  - `remove_pages=3`

Across all three phases:

- `range_fb_count` stayed `1`
- `range_cursor_count` stayed `2`
- `range_other_count` stayed `0`
- `range_sgx_object_count` stayed `0`
- `range_pf_bound_count` stayed `0`

## Conclusion

The live global GTT/MMU state path is no longer sysfs-only.

The new bounded GTT-state query ioctl is now validated on the VAIO against the
existing sysfs dump across:

- steady-state idle readout
- live held-GEM growth
- post-release cleanup

That narrows the remaining read-side sysfs dependence again:

- human-readable text dumps
- stale cleanup convenience
- any future broader state growth that still lacks a bounded typed query
