# SGX Lab Submit Path

This note records the first lab submission descriptor layer added on top of the
kernel-owned SGX lab objects.

It still does not execute SGX 3D work. The purpose is narrower and more
important for now:

- define SGX-oriented object roles
- define expected MMU context per object
- attach multiple objects to a single submission descriptor
- validate that descriptor against live object state on real hardware

## Interface

New sysfs nodes on the live PCI device:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

Extended object commands on `poulsbo_sgx_object`:

- `role <object_id> <generic|pds|use|control|target>`
- `context <object_id> <auto|default|pf>`

Submission control commands:

- `create <default|pf>`
- `attach <submit_id> <control|pds|use|target> <object_id>`
- `detach <submit_id> <slot>`
- `validate <submit_id>`
- `drop <submit_id>`

## Current Object Metadata

Each SGX lab object now carries:

- `role`
- `expected_context`
- `last_role_jiffies`
- `last_context_jiffies`

The object-state view now prints those fields so hardware traces can be tied to
the intended SGX role of each object instead of only its raw range identity.

## Current Submission Model

Each lab submit descriptor currently stores:

- `submit_id`
- `context`
- `control_object_id`
- `pds_object_id`
- `use_object_id`
- `target_object_id`
- `validate_count`
- `last_validate_status`
- creation and validation timestamps

The validation rules are deliberately simple:

- submit context must be explicit, not `auto`
- `pds`, `use`, and `target` slots must all be present
- attached object roles must match their slots
- object `expected_context` must be `auto` or equal the submit context
- for `pf` submissions, every attached object must currently have
  `pf_bind_count > 0`
- attached objects must be distinct

This is enough to prove that a submission descriptor can be checked against real
live object state instead of free-form notes or assumptions.

## Validated Result On The VAIO

Validated on `2026-05-11` on `10.1.11.218` with locally built module
`srcversion 9A0DCD9E393D70292202009`.

Helper used:

```bash
./scripts/run-vaio-sgx-submit.sh
```

Observed sequence:

1. Three held `drm_probe` runs created three new live GEM ranges:
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
7. Slots were attached:
   - `pds_object_id=1`
   - `use_object_id=2`
   - `target_object_id=3`
8. First validation succeeded:
   - `validate_count=1`
   - `last_validate_status=0`
9. Userspace probes exited and destroyed their dumb-buffer handles, but:
   - all three adopted objects still existed
   - the submit descriptor still existed
   - a second validation still returned `0`
10. After `drop submit` and `drop object` for all three objects:
    - `poulsbo_sgx_submits` returned to `0`
    - `poulsbo_sgx_objects` returned to `0`
    - MMU `remove_pages` moved from `0` to `6`

Observed steady-state SGX snapshot after the run:

- `fault_count=0`
- `bif_fault=0x000000`

## What This Establishes

- the repo now has the first SGX-specific object metadata
- the repo now has the first multi-object submission descriptor
- validation can succeed against real PF-bound hardware-backed objects
- validation can succeed even after userspace closes the original GEM handles,
  because the kernel-owned SGX objects keep them alive

## What Is Still Missing

- no actual SGX command buffer format
- no submit-to-hardware path
- no per-object GPU address policy beyond broad context expectations
- no distinction yet between data buffers and more SGX-specific state blocks
- no fencing or completion model

## Next Step

The next useful move is to enrich the object model and submit descriptor with
the smallest SGX-specific fields that hardware work will care about next:

- explicit intended GPU-address usage
- explicit object classes for code versus state versus surfaces
- the first non-executing submission packet layout that mirrors likely SGX
  kick structure without yet poking real 3D registers
