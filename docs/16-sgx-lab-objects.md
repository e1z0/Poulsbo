# SGX Lab Objects

This note records the first kernel-owned SGX lab object layer added on top of
the existing `gtt_range` and PF-PD bind lab work.

The goal is to move past raw `range_id` commands and prove a stronger lifetime:

- adopt a live non-stolen GEM range
- hold a real GEM reference in-kernel
- bind and unbind it through the existing PF-PD path
- let userspace exit
- keep the object alive until an explicit drop

## Interface

New sysfs nodes on the live PCI device:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_object`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_objects`

Supported control commands:

- `adopt <range_id>`
- `pf_bind <object_id>`
- `pf_unbind <object_id>`
- `drop <object_id>`

Object state view:

- one line per active SGX lab object
- object ID
- adopted `range_id`
- size
- GPU virtual address
- live `in_gart`, `mmapping`, and `npage`
- PF bind totals
- object creation/bind/unbind timestamps

## Kernel Model

The current implementation is intentionally small:

- one SGX lab object wraps exactly one live `gtt_range`
- adopting a range takes a real GEM reference with `drm_gem_object_get()`
- dropping the object releases that GEM reference with
  `drm_gem_object_put_unlocked()`
- the wrapped range carries `sgx_lab_object_id` in `poulsbo_gtt_state`
- PF-PD bind and unbind still use the existing `psb_gtt_lab_bind_pf()` and
  `psb_gtt_lab_unbind_pf()` path underneath

This is still a lab interface, not a stable uAPI and not a render object model
in the Mesa sense. It is just the first kernel-owned object lifetime that can
survive userspace close.

## Validated Result On The VAIO

Validated on `2026-05-10` on `10.1.11.218` with locally built module
`srcversion 92CA91A4FED5EB2EA73F40D`.

Helper used:

```bash
./scripts/run-vaio-sgx-object.sh
```

Observed sequence:

1. A held `drm_probe` run created a new live GEM range:
   - `range id=14`
   - `gpu_va=0x80c77000`
   - `mmapping=1`
   - `npage=4`
2. `adopt 14` created:
   - `object id=1`
   - `range_id=14`
3. `pf_bind 1` changed:
   - `flush=5 -> 6`
   - `insert_pages=4 -> 5`
   - `pf_bind_count=1`
4. `pf_unbind 1` changed:
   - `flush=6 -> 7`
   - `remove_pages=1 -> 2`
   - `pf_bind_count=0`
5. The userspace probe exited and destroyed its dumb buffer, but:
   - `range id=14` still existed
   - `sgx_object_id=1` still pointed at it
   - `poulsbo_sgx_objects` still showed `object id=1`
6. `drop 1` changed:
   - `flush=7 -> 8`
   - `remove_pages=2 -> 3`
   - `range id=14` disappeared
   - `poulsbo_sgx_objects` returned to `objects_total=0`

Observed kernel log lines:

- `lab object adopt object_id=1 range_id=14 name=gem gpu_va=0x80c77000 size=0x4000`
- `lab pf_bind id=14 name=gem gpu_va=0x80c77000 npage=4 active=1 total=1`
- `lab pf_unbind id=14 name=gem gpu_va=0x80c77000 active=0 total=1`
- `lab object drop object_id=1 range_id=14 name=gem gpu_va=0x80c77000`

Observed steady-state SGX snapshot after the run:

- `fault_count=0`
- `bif_fault=0x000000`
- `objects_total=0`

## What This Establishes

- the repo now has a kernel-owned SGX object lifetime, not just range-ID
  commands
- a real non-stolen GEM object can outlive userspace close when held by the
  lab object layer
- PF-PD binding is now attachable to an explicit kernel object ID
- the current object layer is sufficient for the next design step

## What Is Still Missing

- no SGX object roles yet
  - no distinction between general data, PDS code, USE code, TA state, or 3D
    state
- no explicit per-context address-space model beyond PF-PD versus default PD
- no submit queue or job packet structure
- no fences or completion objects
- no real render uAPI

## Next Step

The next useful change is not another lifetime experiment. It is to turn this
lab object into the first real SGX-oriented object model:

- add explicit object role metadata
- decide which object types must live in which MMU context
- define the first lab submission-side structure on top of those objects
