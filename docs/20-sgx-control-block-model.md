# SGX Control-Block Model

This note records the first non-symbolic control-block model in the Ubuntu 5.4
Poulsbo lab driver. It does not submit real SGX 3D work. It turns the existing
`control` object from a role-only attachment into a buffer with defined control
contents that the driver writes and validates on real hardware.

## Why This Step Exists

Before this step, the current SGX lab submit path already had:

- explicit `control/pds/use/target` object roles
- object classes and intended GPU-use semantics
- a validate-only TA-style packet description
- resolved GPU virtual addresses for all four slots

But the control object was still only symbolic. Validation knew there was a
`control` object and that `ccb_offset` fit inside it, but no defined command or
header words were written into that object.

The goal of this step is to make the control object carry the smallest useful
CCB-like layout we can validate without attempting real SGX execution.

## Current Kernel Surface

The existing submit control remains:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

The new submit command is:

- `ccb <submit_id> <write_offset> <read_offset> <service> <cache> <data0> <data1>`

It configures one attached submit descriptor and writes those values into the
attached `control` object.

## Minimal In-Object Layout

The current model writes two logical pieces into the `control` object:

1. CCB header at fixed offsets:
   - dword `0x0`: `write_offset`
   - dword `0x4`: `read_offset`
2. Command payload at `ccb_offset`:
   - dword `+0x0`: `service`
   - dword `+0x4`: `cache`
   - dword `+0x8`: `data0`
   - dword `+0xc`: `data1`

This is intentionally small. It is only large enough to prove:

- the control object is writable through the live GEM-backed pages
- the submit descriptor can name a concrete in-object payload location
- the driver can read the same words back during validation and reject drift

## Current Validation Rules

Current submit validation for `cmd_type=ta` now requires:

- all four submit slots attached:
  - `control`
  - `pds`
  - `use`
  - `target`
- expected role, class, GPU-use, and PF-context metadata on those objects
- a configured packet
- a configured CCB model
- `ccb_offset` aligned to `16`
- `write_offset` and `read_offset` aligned to `4`
- `write_offset >= ccb_offset`
- `read_offset <= write_offset`
- both header and payload words to fit inside the attached control object
- header and payload readback to match the submit's cached values

This is still a validate-only model. No SGX execution is triggered yet.

## Validated Hardware Result

Validated on May 11, 2026 on the Sony VAIO VGN-P11Z target after deploying the
locally built module with `srcversion A9D37B592F6EA8CB2A6BBA1`.

Helper used:

- `scripts/run-vaio-sgx-submit.sh`

Validated flow:

- created four held dumb GEM objects
- adopted four fresh GTT ranges:
  - `range id=10`
  - `range id=11`
  - `range id=12`
  - `range id=13`
- mapped them to:
  - `object id=1` as `control / packet / control_ccb / pf`
  - `object id=2` as `pds / code / pds_code / pf`
  - `object id=3` as `use / code / use_code / pf`
  - `object id=4` as `target / surface / render_target / pf`
- PF-bound all four objects
- created `submit id=1`
- configured the TA packet:
  - `cmd_type=ta`
  - `ccb_offset=0x20`
  - `ctrl_flags=0x1`
  - `ta_status=1`
  - `3d_status=1`
  - `ta_dependency=0`
- configured the control-block words:
  - `ccb_write_offset=0x20`
  - `ccb_read_offset=0x0`
  - `ccb_service=0x1000`
  - `ccb_cache=0x0`
  - `ccb_data0=0xc0de0001`
  - `ccb_data1=0xc0de0002`
  - `ccb_config_count=1`
- validated once while all userspace holders were still alive
- waited for all userspace holders to exit
- validated the same descriptor a second time

Observed result:

- both validates returned `status=0`
- the live submit view resolved:
  - `control_gpu_va=0x81127000`
  - `pds_gpu_va=0x8112b000`
  - `use_gpu_va=0x8112f000`
  - `target_gpu_va=0x81133000`
- `dmesg` logged:
  - `lab submit ccb submit_id=1 write_offset=0x20 read_offset=0x0 service=0x1000 cache=0x0 data0=0xc0de0001 data1=0xc0de0002`
  - two successful `lab submit validate ... status=0`
- final teardown returned the object and submit views to zero entries
- MMU `remove_pages` moved from `0` to `8` in this clean-boot validation run
- post-run SGX state still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`

## What This Means

The control object is no longer only a labeled attachment. It now has:

- a minimal in-object header
- a minimal in-object command payload
- defined offsets
- real driver-side readback validation against live GEM-backed pages

That is enough to move the project from anonymous submit metadata toward a real
control/data layout without pretending we already have executable SGX job
submission.

## Next Step

The next useful extension is not “make the control object exist.” That is done.
The next step is to decide what additional control-block semantics should be
explicit next, most likely one of:

- status or sync backing objects
- broader command payload ownership rules
- first ring-style progression semantics beyond one fixed payload
- first non-executing submit record that more closely mirrors real SGX queues
