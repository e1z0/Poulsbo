# SGX Lab Object Query Ioctl Validation

Date: 2026-05-11

## Scope

Add a bounded typed object query ioctl so live SGX lab object inspection no
longer depends entirely on the `poulsbo_sgx_objects` text dump.

This milestone covers:

- [third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
- [workloads/Makefile](/root/Poulsbo/workloads/Makefile)
- [workloads/psb_sgx_lab_object_query.c](/root/Poulsbo/workloads/psb_sgx_lab_object_query.c)
- [scripts/run-vaio-sgx-ioctl-object-query.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-object-query.sh)

## What Was Added

Kernel-side:

- new bounded ioctl query:
  - `DRM_IOCTL_PSB_SGX_LAB_QUERY_OBJECT`
- new object-query flag:
  - `DRM_PSB_SGX_LAB_QUERY_OBJECT_USE_ID`
- new typed object-query structure:
  - `drm_psb_sgx_lab_object_query`

The ioctl covers:

- query by:
  - live object index
  - explicit object ID
- typed object metadata:
  - `resolved_object_id`
  - `resolved_index`
  - `total_object_count`
  - `range_id`
  - `range_type`
  - `role`
  - `object_class`
  - `gpu_use`
  - `expected_context`
- live range-backed state:
  - `size_bytes`
  - `gpu_va`
  - `offset`
  - `stolen`
  - `in_gart`
  - `mmapping`
  - `npage`
  - `pf_bind_count`
  - `pf_bind_total`
  - `pf_unbind_total`
- object-side timestamps:
  - `created_jiffies`
  - `last_pf_bind_jiffies`
  - `last_pf_unbind_jiffies`
  - `last_role_jiffies`
  - `last_class_jiffies`
  - `last_gpu_use_jiffies`
  - `last_context_jiffies`

Userspace-side:

- new 32-bit helper:
  - [psb_sgx_lab_object_query.c](/root/Poulsbo/workloads/psb_sgx_lab_object_query.c)
- it supports:
  - `index <query_index>`
  - `id <object_id>`
- it prints the typed object-query fields returned by the ioctl
- it uses raw `ioctl()` for the custom lab request

Runner-side:

- new direct live validator:
  - [run-vaio-sgx-ioctl-object-query.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-object-query.sh)
- it:
  - seeds two held GEM ranges
  - adopts two SGX lab objects on top of them
  - mutates object A through:
    - `role control`
    - `class packet`
    - `gpu_use control_ccb`
    - `context pf`
    - `pf_bind`
  - compares the typed query view against the live `poulsbo_sgx_objects` dump
  - re-queries by ID after the userspace holders exit
  - then cleans the objects up and confirms the query returns `ENOENT`

## Local Build And Deploy

Rebuilt locally with:

- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`

Guarded deploy result:

- `driver_srcversion=176E7F4A4C9DCF459B10C8E`
- `override_module_size=385256`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Hardware Validation

The live bundle is:

- [logs/vaio-ioctl/20260511-112759-sgx-object-query-live](/root/Poulsbo/logs/vaio-ioctl/20260511-112759-sgx-object-query-live)

The target remained clean after the run:

- `overall=remote_usable`
- `driver_srcversion=176E7F4A4C9DCF459B10C8E`
- `override_module_size=385256`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Observed Result

The validator seeded:

- two held GEM ranges:
  - `RANGE_ID_A=5`
  - `RANGE_ID_B=7`
- two objects:
  - object `1` on range `5`
  - object `2` on range `7`

Object A was then configured to a non-default state:

- `role=control`
- `class=packet`
- `gpu_use=control_ccb`
- `expected_context=pf`
- `pf_bind_count=1`

The existing sysfs object dump showed that state directly:

- `OBJECTS_SYSFS_DURING`:
  - object `1`:
    - `range_id=5`
    - `role=control`
    - `class=packet`
    - `gpu_use=control_ccb`
    - `expected_context=pf`
    - `pf_bind_count=1`
  - object `2`:
    - `range_id=7`
    - `role=generic`
    - `class=generic`
    - `gpu_use=generic`
    - `expected_context=auto`

The new typed query matched that state by explicit ID:

- `OBJECT_QUERY_ID_A_DURING`:
  - `object_resolved_id=1`
  - `object_resolved_index=0`
  - `object_total_count=2`
  - `object_range_id=5`
  - `object_range_type_name=gem`
  - `object_role_name=control`
  - `object_class_name=packet`
  - `object_gpu_use_name=control_ccb`
  - `object_expected_context_name=pf`
  - `object_pf_bind_count=1`
  - `object_pf_bind_total=1`
- `OBJECT_QUERY_ID_B_DURING`:
  - `object_resolved_id=2`
  - `object_resolved_index=1`
  - `object_total_count=2`
  - `object_range_id=7`
  - `object_role_name=generic`
  - `object_class_name=generic`
  - `object_gpu_use_name=generic`
  - `object_expected_context_name=auto`

The new typed query also worked by live object index:

- `OBJECT_QUERY_INDEX_0_DURING`:
  - resolved object `1`
- `OBJECT_QUERY_INDEX_1_DURING`:
  - resolved object `2`

After the userspace holders exited, both objects still queried cleanly by ID:

- `OBJECT_QUERY_ID_A_AFTER_PROBES`:
  - still resolved object `1`
  - still showed `role=control`, `class=packet`, `gpu_use=control_ccb`,
    `expected_context=pf`, `pf_bind_count=1`
- `OBJECT_QUERY_ID_B_AFTER_PROBES`:
  - still resolved object `2`

After typed cleanup:

- `OBJECT_CLEANUP` succeeded with:
  - `object_op_name=cleanup`
  - `object_op_ret=0 errno=0`
  - `object_result_count=2`
- `OBJECTS_SYSFS_AFTER_CLEANUP` showed:
  - `objects_shown=0`
  - `objects_total=0`
- both ID queries then failed cleanly with `ENOENT`:
  - `OBJECT_QUERY_ID_A_AFTER_CLEANUP`:
    - `object_ioctl_ret=-1 errno=2`
  - `OBJECT_QUERY_ID_B_AFTER_CLEANUP`:
    - `object_ioctl_ret=-1 errno=2`

## Conclusion

The live SGX lab object inspection path is no longer sysfs-only.

The new bounded object-query ioctl is now validated on the VAIO for:

- explicit object-ID lookup
- live object-index lookup
- outliving userspace close until explicit cleanup
- clean `ENOENT` after cleanup

That narrows the remaining sysfs dependence again:

- human-readable submit and coordination text dumps
- any later object or submit list enumeration growth that still lacks a bounded
  typed query
