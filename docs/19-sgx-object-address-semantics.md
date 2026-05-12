# SGX Object Address Semantics

This note records the first explicit SGX object-class and intended GPU-address
semantics added on top of the existing kick-packet layer.

It still does not execute SGX 3D work. The purpose is to stop treating the
submit packet as a bag of anonymous object IDs and make the current lab path
carry a real four-object topology:

- packet/control memory
- PDS code memory
- USE code memory
- render target memory

## Interface

The SGX object control gained two new commands:

- `class <object_id> <generic|packet|code|state|surface>`
- `gpu_use <object_id> <generic|control_ccb|pds_code|use_code|render_target>`

The existing submit slots remain:

- `control`
- `pds`
- `use`
- `target`

The submit-state view now resolves the attached object GPU virtual addresses and
prints:

- `control_gpu_va`
- `pds_gpu_va`
- `use_gpu_va`
- `target_gpu_va`

## Current Object Semantics

Each SGX lab object now carries:

- `role`
- `object_class`
- `gpu_use`
- `expected_context`
- timestamps for the last role/class/gpu-use/context change

The current intended mapping for a TA-style packet is:

- `control` -> `class=packet`, `gpu_use=control_ccb`
- `pds` -> `class=code`, `gpu_use=pds_code`
- `use` -> `class=code`, `gpu_use=use_code`
- `target` -> `class=surface`, `gpu_use=render_target`

## Current Validation Rules

For the current `ta` kick model, validation now requires:

- explicit `control`, `pds`, `use`, and `target` objects
- matching `role` per slot
- matching `object_class` per slot
- matching `gpu_use` per slot
- explicit submit context
- PF-bound objects for PF-context submits
- distinct object IDs across all four slots
- 16-byte `ccb_offset` alignment
- `READY` set in `ctrl_flags`
- `ccb_offset + 16` must fit inside the attached control object size

This gives the current kick packet its first real address-bearing semantics:
`ccb_offset` is no longer just an aligned integer; it now refers to a concrete
packet/control object with a concrete GPU virtual address and bounded size.

## Helper Change

The VAIO helper [scripts/run-vaio-sgx-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit.sh)
had to change for a practical reason: `poulsbo_gtt_state` is a one-page sysfs
view, so once four held GEM objects existed at the same time the “diff the live
range dump” approach started truncating before the newest range.

The helper now:

- records the highest `range id` visible before starting the probes
- starts the held dumb-buffer probes
- adopts fresh candidate range IDs above that watermark until it has the needed
  object count

That keeps the validation path stable even when the generic GTT state view is
truncated.

## Validated Result On The VAIO

Validated on `2026-05-11` on `10.1.11.218` with locally built module
`srcversion EF61C0BA3E7AC7F9B23DD47`.

Helper used:

```bash
./scripts/run-vaio-sgx-submit.sh
```

Observed sequence:

1. Four held `drm_probe` runs created four fresh live GEM ranges:
   - `range id=14`
   - `range id=15`
   - `range id=16`
   - `range id=17`
2. Those ranges were adopted as:
   - `object id=1`
   - `object id=2`
   - `object id=3`
   - `object id=4`
3. Roles were assigned:
   - `object 1 -> control`
   - `object 2 -> pds`
   - `object 3 -> use`
   - `object 4 -> target`
4. Object classes were assigned:
   - `object 1 -> packet`
   - `object 2 -> code`
   - `object 3 -> code`
   - `object 4 -> surface`
5. Intended GPU-address uses were assigned:
   - `object 1 -> control_ccb`
   - `object 2 -> pds_code`
   - `object 3 -> use_code`
   - `object 4 -> render_target`
6. Expected context was set to `pf` for all four objects.
7. All four objects were PF-bound:
   - MMU `insert_pages` moved from `10` to `14`
   - MMU `flush` moved from `14` to `18`
8. A PF-context submit descriptor was created and kicked with:
   - `cmd_type=ta`
   - `ccb_offset=0x20`
   - `ctrl_flags=0x1`
   - `num_ta_status_vals=1`
   - `num_3d_status_vals=1`
   - `ta_dependency=0`
9. All four submit slots were attached:
   - `control_object_id=1`
   - `pds_object_id=2`
   - `use_object_id=3`
   - `target_object_id=4`
10. The live submit view resolved:
    - `control_gpu_va=0x81127000`
    - `pds_gpu_va=0x8112b000`
    - `use_gpu_va=0x8112f000`
    - `target_gpu_va=0x81133000`
11. First validation succeeded:
    - `validate_count=1`
    - `last_validate_status=0`
12. After userspace probes exited:
    - all four objects still existed
    - the submit still existed
    - second validation still returned `0`
13. After `drop submit` and `drop object` for all four objects:
    - `poulsbo_sgx_submits` returned to `0`
    - `poulsbo_sgx_objects` returned to `0`
    - MMU `remove_pages` moved from `4` to `12`

Observed steady-state SGX snapshot after the run:

- `fault_count=0`
- `bif_fault=0x000000`

## What This Establishes

- the repo now has its first explicit SGX object-class model
- the repo now has its first intended GPU-address-use model
- the kick packet now validates a four-object control/pds/use/target topology
- the submit view now exposes resolved GPU virtual addresses for attached slots
- the model is validated on real Poulsbo hardware, not only in synthetic state

## What Is Still Missing

- no real CCB write/read pointer control block
- no user-supplied arbitrary SGX addresses beyond the attached object GPU VAs
- no sync/status backing objects
- no SGX register programming or execution
- no fence or completion model

## Next Step

The next useful move is to make the control object less symbolic:

- define the first CCB/control-block layout that lives inside it
- expose CCB read/write offsets or packet ownership semantics
- decide whether that next step still fits sysfs or should move to a lab debug
  ioctl
