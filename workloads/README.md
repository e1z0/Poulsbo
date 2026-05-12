# Workloads

These are intentionally tiny GLES2/X11 programs for reverse-engineering sessions.

## Purpose

Use small workloads first so register traces stay understandable:

- `gles2_clear_x11`
  - one window
  - one clear
  - one swap

- `gles2_triangle_x11`
  - one window
  - simple shaders
  - one triangle draw
  - one swap

- `x11_egl_probe`
  - opens the live X11 display
  - initializes EGL on that display
  - creates a tiny window surface
  - prints X/EGL/GLES renderer information
  - does a single clear/swap if GLES entry points are available

- `x11_dri2_probe`
  - connects to the live X11 display via XCB
  - queries DRI2 version/connect/auth state
  - opens the advertised DRM device
  - prints kernel DRM caps and DRI2 authentication result

- `drm_probe`
  - opens `/dev/dri/card0` directly
  - prints DRM version and capability bits
  - enumerates KMS resources/connectors
  - tries dumb create/map/destroy and PRIME export
  - if given a second positional argument, keeps the dumb buffer alive for that
    many seconds before destroy

- `psb_sgx_lab_query`
  - opens `/dev/dri/card0` directly
  - queries the SGX lab coordination ioctl
  - supports `coord` for coordination-only typed readout
  - queries one SGX lab submit by explicit ID, by focused-submit lookup, or by
    live submit index, or by `gpuva <gpu_va>` / `gpuva-match <gpu_va> <match_index>`
  - prints typed queue, status, sync, owner, dependency, object-ID, and
    GPU-VA fields including:
    - prepare count and last-prepare timestamp
    - arm-valid state, arm count, and last-arm timestamp
    - armed queue, sync, owner, and issue-side snapshot fields
    - emit-valid state, emit count, and last-emit timestamp
    - emitted queue, sync, owner, object-ID, GPU-VA, and issue-side snapshot fields
    - launch-valid state, launch count, and last-launch timestamp
    - launched queue, sync, owner, object-ID, GPU-VA, and issue-plan snapshot fields
    - dispatch-valid state, dispatch count, and last-dispatch timestamp
    - dispatched queue, sync, owner, object-ID, GPU-VA, and post-issue snapshot fields
    - manifest offset, stage, stage count, owner state, queue, dependency,
      GPU-VA32, sync, ticket, and owner-cookie fields from the memory-backed
      control-side manifest block
    - descriptor offset, stage, stage count, owner state, issued/completed
      packets, queue, dependency, GPU-VA32, queue-layout, report/manifest
      offsets, and ticket fields from the memory-backed control-side
      descriptor block
    - plan offset, stage, source, owner state, issued/completed packets,
      queue, queue epoch, dependency, sync, owner, ticket, report/manifest/
      descriptor offsets, CCB words, and GPU-VA32 fields from the
      memory-backed control-side plan block
    - resolved slot
    - resolved object ID
    - resolved range ID
    - resolved range type
    - resolved object role/class/gpu_use/context
    - matching submit count on direct GPU-VA lookup
    - requested and resolved GPU-VA match index
    - in-range GPU-VA offset
    - memory-backed CCB GPU-VA32 words for:
      - `pds`
      - `use`
      - `target`
      - `status`
  - uses raw `ioctl()` for the custom lab query requests so typed query-side
    behavior is not coupled to libdrm retry semantics
  - serves as the first non-sysfs userspace inspection path for live SGX lab
    submit state

- `psb_sgx_lab_object_op`
  - opens `/dev/dri/card0` directly
  - runs one bounded SGX lab object ioctl op
  - currently supports `adopt`, `role`, `class`, `gpu_use`, `context`,
    `pf_bind`, `pf_unbind`, `drop`, and `cleanup`
  - returns the resolved object ID for `adopt`
  - returns `object_result_count` for `cleanup`
  - uses raw `ioctl()` for the custom lab object request so the object-side
    lab path is not coupled to libdrm retry behavior
  - serves as the first non-sysfs userspace mutation path for live SGX lab
    object lifecycle and configuration state

- `psb_sgx_lab_submit_op`
  - opens `/dev/dri/card0` directly
  - runs one bounded SGX lab submit ioctl op
  - currently supports `create`, `focus`, `focus-clear`, `cleanup`, `attach`, `detach`,
    `kick`, `ccb`, `queue`, `status`, `sync`, `owner`, `report`, `manifest`,
    `descriptor`, `plan`,
    `prepare`, `arm`, `emit`, `launch`, `depend-slot`,
    `depticket-slot`, `depstate-slot`, `depclear-slot`, `validate`, `issue`,
    `transition`, and `drop`
  - accepts either an explicit submit ID or `focus` where the selected op
    needs an existing submit
  - returns `submit_result_count` for `cleanup`
  - uses raw `ioctl()` for the custom lab submit request so dependency-gated
    `-EAGAIN` transitions return to the caller instead of being retried by
    libdrm
  - serves as the paired non-sysfs userspace mutation path for live SGX lab
    submit lifecycle and progression state
  - `prepare` now materializes one SGX-shaped non-executing 8-dword CCB
    record with:
    - service address
    - cache control
    - data0
    - data1
    - `pds_gpu_va32`
    - `use_gpu_va32`
    - `target_gpu_va32`
    - `status_gpu_va32`

- `psb_sgx_lab_dump`
  - opens `/dev/dri/card0` directly
  - runs one typed end-to-end SGX lab dump
  - supports:
    - `cached`
    - `capture`
    - `json`
    - `capture-json`
  - prints typed coord, GTT, SGX, object, and submit state in one pass
  - in JSON mode, emits one structured typed snapshot with:
    - `coord`
    - `gtt_state`
    - `sgx_state`
    - `objects`
    - `submits`
  - uses only the bounded query ioctls
  - serves as the standalone non-sysfs userspace inspection path for one
    human-readable or machine-checked live SGX lab snapshot
  - includes submit-side `prepare`, `arm`, `emit`, `launch`, and `dispatch`
    state in both text and JSON dumps
  - includes the memory-backed submit `report` block in both text and JSON
    dumps
  - includes the memory-backed submit `manifest` block in both text and JSON
    dumps
  - includes the memory-backed submit `descriptor` block in both text and
    JSON dumps
  - includes the memory-backed submit `plan` block in both text and JSON
    dumps
  - includes the memory-backed submit CCB GPU-VA32 words in both text and
    JSON dumps

- `psb_sgx_lab_range_query`
  - opens `/dev/dri/card0` directly
  - runs one bounded live-range query ioctl
  - supports:
    - `index <query_index> [gem]`
    - `id <range_id> [gem]`
    - `gpuva <gpu_va> [gem]`
    - `gpuva-submit <gpu_va> <submit_match_index> [gem]`
  - prints typed live-range fields including:
    - resolved range ID and index
    - type and type name
    - `start/end/len`
    - `query_gpu_va`
    - `query_gpu_va_offset`
    - `gpu_va`
    - `offset`
    - `stolen/in_gart/mmapping/npage/roll`
    - SGX object ownership and PF-bind counters
    - SGX object role/class/gpu_use/context when the range is adopted
    - live SGX submit reference count for that adopted object
    - first matching live submit ID and slot when the object is attached
    - requested submit-match index for `gpuva-submit`
    - whether a selected submit-match resolved
    - selected submit ID, slot, and resolved match index
  - uses raw `ioctl()` for the custom lab range-query request so future
    lab-only range-query error semantics are not hidden by libdrm retries
  - serves as the first non-sysfs userspace inspection path for live
    `gtt_range` enumeration, hot-path GEM range discovery, direct GPU-VA
    ownership lookup, direct GPU-VA-to-submit correlation, and direct
    range-side submit-match selection on shared-object GPU VAs

- `psb_sgx_lab_sgx_state_query`
  - opens `/dev/dri/card0` directly
  - runs one bounded SGX snapshot query ioctl
  - supports:
    - `capture`
    - `cached`
  - prints typed snapshot fields including:
    - `reason`
    - `sequence`
    - `captured_jiffies`
    - `capture_failed`
    - `irq_count`
    - `fault_count`
    - MMU and GATT snapshot fields
    - SGX core and event registers
    - BIF fault and directory-bank state
    - 2D request base and blit status
  - uses raw `ioctl()` for the custom lab SGX-state query request
  - serves as the first non-sysfs userspace inspection path for the live SGX
    snapshot state itself, not only submit/coord/query metadata

- `psb_sgx_lab_gtt_state_query`
  - opens `/dev/dri/card0` directly
  - runs one bounded GTT/MMU state query ioctl
  - prints typed global GTT/MMU fields including:
    - `gtt_start`
    - `gatt_start`
    - `mmu_gatt_start`
    - `gatt_free_offset`
    - MMU last-op state
    - MMU counter totals
    - live range counts by type and state
  - uses raw `ioctl()` for the custom lab GTT-state query request
  - serves as the first non-sysfs userspace inspection path for the live
    global GTT/MMU state itself, not only per-range or per-submit state

- `psb_sgx_lab_object_query`
  - opens `/dev/dri/card0` directly
  - runs one bounded SGX lab object query ioctl
  - supports:
    - `index <query_index>`
    - `id <object_id>`
  - prints typed object fields including:
    - `resolved_object_id`
    - `resolved_index`
    - `total_object_count`
    - `range_id`
    - `range_type`
    - `role`
    - `object_class`
    - `gpu_use`
    - `expected_context`
    - `size_bytes`
    - `gpu_va`
    - PF-bind counters and object timestamps
  - uses raw `ioctl()` for the custom lab object-query request
  - serves as the first non-sysfs userspace inspection path for live SGX lab
    object state itself, not only object mutation or range ownership

These probes are meant to be deployed to the target machine after a local
container build. They are small enough to use as first-line diagnostics for the
live X11 stack.

## Build

Requirements:

- X11 development headers
- EGL development headers
- GLES2 development headers
- `pkg-config`
- C compiler

Build:

```bash
make -C workloads
```

## Notes

- These were not compile-tested in this container because the required development packages are not installed here.
- The sources are intended for the target lab machine or a compatible build environment.
- Keep workloads tiny. If a larger sample is needed later, add it as a separate file rather than bloating the first-stage tests.
