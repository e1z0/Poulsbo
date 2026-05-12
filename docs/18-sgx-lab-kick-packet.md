# SGX Lab Kick Packet

This note records the first non-executing SGX-style kick packet layer built on
top of the existing SGX lab submit descriptor.

The later object-class and GPU-address semantics built on top of this packet
layer are recorded separately in `docs/19-sgx-object-address-semantics.md`.

It still does not submit real 3D work. The purpose is narrower:

- make the submit descriptor carry SGX-shaped packet metadata
- keep that metadata tied to live hardware-backed objects
- validate the packet on the VAIO before any real SGX execution exists

## Historical Reference

The packet shape is intentionally modeled after the old `services4` SGX kernel
interfaces mirrored in this repo:

- `research/external/intel-binaries-linux/drm/pvr/services4/include/sgxinfo.h`
- `research/external/intel-binaries-linux/drm/pvr/services4/include/sgx_mkif_km.h`

The specific fields used here are derived from:

- `SGX_CCB_KICK`
- `SGXMKIF_CMDTA_SHARED`
- `SGXMKIF_CMD_TYPE`

This repo is not importing those SGX interfaces wholesale. It is extracting the
smallest validate-only packet structure that helps the next hardware step.

## Interface

The packet layer extends the existing submit control:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

New command:

- `kick <submit_id> <cmd_type> <ccb_offset> <ctrl_flags> <num_ta_status> <num_3d_status> <ta_dependency>`

Current `cmd_type` values accepted by the parser:

- `ta`
- `transfer`
- `2d`
- `power`
- `cleanup`
- `getmiscinfo`
- `process_queues`

Current validation support is narrower than parsing support:

- only `ta` validates today

## Current Packet Metadata

Each SGX lab submit descriptor now carries:

- `cmd_type`
- `ccb_offset`
- `ctrl_flags`
- `num_ta_status_vals`
- `num_3d_status_vals`
- `ta_dependency`
- `packet_config_count`
- `last_packet_jiffies`

The submit-state view now prints those fields, so hardware captures show both
the object topology and the intended SGX packet shape.

## Current Validation Rules

The packet layer deliberately enforces only a few things so the model stays
honest:

- submit context must still be explicit, not `auto`
- packet `cmd_type` must be configured
- only `ta` packets validate today
- `ccb_offset` must be 16-byte aligned
- only the `READY` control bit is currently accepted in `ctrl_flags`
- `num_ta_status_vals` and `num_3d_status_vals` must stay within the current
  lab cap of `16`
- existing object-role, context, PF-bind, and distinct-object checks still
  apply

This is enough to make packet state matter without pretending we already have a
real SGX command stream.

## Validated Result On The VAIO

Validated on `2026-05-11` on `10.1.11.218` with locally built module
`srcversion DB9A06B6BF23EF03033ED3E`.

Helper used:

```bash
./scripts/run-vaio-sgx-submit.sh
```

Observed sequence:

1. Three held `drm_probe` runs created three live GEM ranges:
   - `range id=10`
   - `range id=11`
   - `range id=12`
2. Those ranges were adopted as:
   - `object id=1`
   - `object id=2`
   - `object id=3`
3. Roles were assigned:
   - `object 1 -> pds`
   - `object 2 -> use`
   - `object 3 -> target`
4. Expected context was set to `pf` for all three objects.
5. All three objects were PF-bound:
   - MMU `insert_pages` moved from `5` to `8`
   - MMU `flush` moved from `5` to `8`
6. A PF-context submit descriptor was created:
   - `submit id=1`
7. The submit packet was configured with:
   - `cmd_type=ta`
   - `ccb_offset=0x20`
   - `ctrl_flags=0x1`
   - `num_ta_status_vals=1`
   - `num_3d_status_vals=1`
   - `ta_dependency=0`
8. Packet slots were attached:
   - `pds_object_id=1`
   - `use_object_id=2`
   - `target_object_id=3`
9. First validation succeeded:
   - `validate_count=1`
   - `last_validate_status=0`
10. Userspace probes exited and destroyed their dumb-buffer handles, but:
    - all three adopted objects still existed
    - the submit descriptor still existed
    - a second validation still returned `0`
11. After `drop submit` and `drop object` for all three objects:
    - `poulsbo_sgx_submits` returned to `0`
    - `poulsbo_sgx_objects` returned to `0`
    - MMU `remove_pages` moved from `0` to `6`

Observed steady-state SGX snapshot after the run:

- `fault_count=0`
- `bif_fault=0x000000`

## What This Establishes

- the repo now has the first validate-only SGX-style kick packet
- submit validation now depends on packet metadata, not only object slots
- the packet layer works on real PF-bound hardware-backed objects on the VAIO
- the packet survives userspace close because the kernel-owned SGX objects still
  own the underlying GEM objects

## What Is Still Missing

- no real CCB memory object or CCB write/read pointers
- no GPU-address fields for PDS, USE, control, or target content
- no sync-object arrays or status-update backing objects
- no SGX register programming or command execution
- no fence or completion model

## Next Step

The next useful move is to extend the validate-only kick model with the first
object-address and object-class semantics that matter for real SGX submission:

- distinguish packet/control objects from code/state/surface objects
- add explicit intended GPU-address roles for attached objects
- decide whether the next step stays in sysfs or moves to a lab debug ioctl
