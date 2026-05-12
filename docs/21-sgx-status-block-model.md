# SGX Status-Block Model

This note records the first explicit SGX status backing object in the Ubuntu
5.4 Poulsbo lab driver. It extends the earlier control-block milestone by
moving TA and 3D status words into a separate attached object with explicit
offsets, stride, and deterministic readback validation.

## Why This Step Exists

Before this step, the submit model already had:

- explicit `control/pds/use/target` object roles
- object classes and intended GPU-use semantics
- a validate-only TA-style packet description
- a real in-object control block inside the `control` object

But the existing `ta_status` and `3d_status` packet counts had no separate
backing object. The driver tracked the counts, but no state object carried the
status words they implied.

The goal of this step is to make status backing explicit before any real SGX
execution attempt.

## Current Kernel Surface

The existing submit control remains:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

The current submit path now adds:

- a fifth attachable slot: `status`
- a new GPU-use semantic: `status_block`
- a new submit command:
  - `status <submit_id> <ta_offset> <3d_offset> <stride> <seed>`

The current object expectation for the new slot is:

- role: `status`
- class: `state`
- gpu_use: `status_block`

## Minimal Status Layout

The current status model writes deterministic words into the attached `status`
object using:

- `ta_offset`
- `3d_offset`
- `stride`
- `seed`

For each TA status index `i`, the driver writes:

- `seed + 0x100 + i`

For each 3D status index `i`, the driver writes:

- `seed + 0x200 + i`

Current validation requires:

- aligned `ta_offset` and `3d_offset`
- aligned non-zero `stride`
- regions large enough for the configured TA and 3D status counts
- non-overlapping TA and 3D status regions
- live readback from the attached `status` object to match the expected values

This is still validate-only. No SGX execution is triggered yet.

## Validated Hardware Result

Validated on May 11, 2026 on the Sony VAIO VGN-P11Z target after deploying the
locally built module with `srcversion 3482CBD7D1F81D9D4C6F81A`, loaded from:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`

Helper used:

- `scripts/run-vaio-sgx-submit.sh`

Validated flow:

- created five held dumb GEM objects
- adopted five fresh GTT ranges:
  - `range id=14`
  - `range id=15`
  - `range id=16`
  - `range id=17`
  - `range id=18`
- mapped them to:
  - `object id=1` as `control / packet / control_ccb / pf`
  - `object id=2` as `pds / code / pds_code / pf`
  - `object id=3` as `use / code / use_code / pf`
  - `object id=4` as `target / surface / render_target / pf`
  - `object id=5` as `status / state / status_block / pf`
- PF-bound all five objects
- created `submit id=1`
- configured the TA packet:
  - `cmd_type=ta`
  - `ccb_offset=0x20`
  - `ctrl_flags=0x1`
  - `ta_status=1`
  - `3d_status=1`
  - `ta_dependency=0`
- configured the control block as before
- configured the status block:
  - `status_ta_offset=0x80`
  - `status_3d_offset=0xc0`
  - `status_stride=0x4`
  - `status_seed=0x5a500000`
  - `status_config_count=1`
- validated once while userspace holders were alive
- waited for all userspace holders to exit
- validated the same descriptor a second time

Observed result:

- both validates returned `status=0`
- the live submit view resolved:
  - `control_gpu_va=0x80c77000`
  - `pds_gpu_va=0x80c7b000`
  - `use_gpu_va=0x80c83000`
  - `target_gpu_va=0x80c7f000`
  - `status_gpu_va=0x80c87000`
- MMU counts moved as expected:
  - `insert_pages=8 -> 13`
  - `remove_pages=1 -> 11`
- final teardown returned the object and submit views to zero entries
- post-run SGX state still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`

The target `dmesg` logged:

- `lab submit attach submit_id=1 slot=status object_id=5`
- `lab submit status submit_id=1 ta_offset=0x80 3d_offset=0xc0 stride=0x4 seed=0x5a500000 ta_count=1 3d_count=1`
- two successful `lab submit validate ... status=0`

## What This Means

The current submit model now has three explicit SGX-shaped memory roles:

- control-block backing in the `control` object
- code/data role separation across `pds`, `use`, and `target`
- separate status backing in the `status` object

That is enough to move the lab model from “one attached packet object plus
metadata” toward “multiple explicit memory roles with independent validation”
without pretending we already have real SGX execution.

## Next Step

The next useful extension is not “make status backing explicit.” That is done.
The next step is to decide which progression or queue semantics should become
explicit next, most likely one of:

- first queue-head or queue-tail semantics beyond fixed offsets
- broader status ownership or synchronization semantics
- a more SGX-shaped non-executing submit record spanning control and status
  objects
