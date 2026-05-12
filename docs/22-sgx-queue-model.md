# SGX Queue Model

This note records the first explicit queue-progression model in the Ubuntu 5.4
Poulsbo lab driver. It extends the earlier control-block and status milestones
by making queue base, size, packet size, and packet count explicit inside the
attached `control` object.

## Why This Step Exists

Before this step, the submit model already had:

- explicit `control/pds/use/target/status` roles
- a real in-object control block inside the `control` object
- a separate `status` object with deterministic TA and 3D status words

But the queue progression implied by `ccb_read_offset`, `ccb_write_offset`, and
`ccb_offset` was still only partially described. The driver knew those offsets,
but no queue metadata declared what queue region they belonged to or how many
packet records they represented.

The goal of this step is to make queue progression explicit before any real SGX
execution attempt.

## Current Kernel Surface

The existing submit control remains:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

The current submit path now adds:

- a new submit command:
  - `queue <submit_id> <base_offset> <size> <packet_size> <packet_count>`

It writes queue metadata into the attached `control` object and makes
validation enforce non-wrapping queue progression.

## Minimal Queue Layout

The current queue model writes four dwords into the `control` object:

- dword `0x8`: `queue_base_offset`
- dword `0xc`: `queue_size`
- dword `0x10`: `queue_packet_size`
- dword `0x14`: `queue_packet_count`

Combined with the existing control-block header:

- dword `0x0`: `ccb_write_offset`
- dword `0x4`: `ccb_read_offset`

the current queue semantics are:

- `queue_base_offset` is the start of the queue region inside the `control`
  object
- `ccb_read_offset` and `ccb_write_offset` are relative offsets inside that
  queue region
- `ccb_offset` must equal `queue_base_offset + ccb_read_offset`
- current validation only supports non-wrapping progression
- current validation requires:
  - aligned `queue_base_offset`
  - non-zero `queue_size`
  - aligned `queue_packet_size`
  - `queue_size` to be a multiple of `queue_packet_size`
  - `queue_packet_count` to fit within the queue region
  - `ccb_write_offset == ccb_read_offset + queue_packet_size * queue_packet_count`

This is still validate-only. No SGX execution is triggered yet.

## Validated Hardware Result

Validated on May 11, 2026 on the Sony VAIO VGN-P11Z target after deploying the
locally built module with `srcversion 45E15DF2729D274214EAFB6`, loaded from:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`

Helper used:

- `scripts/run-vaio-sgx-submit.sh`

Validated flow:

- created five held dumb GEM objects
- adopted five fresh GTT ranges:
  - `range id=10`
  - `range id=11`
  - `range id=12`
  - `range id=13`
  - `range id=14`
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
- configured the queue block:
  - `queue_base_offset=0x20`
  - `queue_size=0x40`
  - `queue_packet_size=0x20`
  - `queue_packet_count=1`
  - `queue_config_count=1`
- configured the status block as before
- validated once while userspace holders were alive
- waited for all userspace holders to exit
- validated the same descriptor a second time

Observed result:

- both validates returned `status=0`
- the live submit view resolved:
  - `control_gpu_va=0x81127000`
  - `pds_gpu_va=0x8112b000`
  - `use_gpu_va=0x8112f000`
  - `target_gpu_va=0x81133000`
  - `status_gpu_va=0x81137000`
- the live submit state showed:
  - `ccb_read_offset=0x0`
  - `ccb_write_offset=0x20`
  - `queue_base_offset=0x20`
  - `queue_size=0x40`
  - `queue_packet_size=0x20`
  - `queue_packet_count=1`
- MMU counts moved as expected:
  - `insert_pages=7 -> 12`
  - `remove_pages=0 -> 10`
- final teardown returned the object and submit views to zero entries
- post-run SGX state still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`

The target `dmesg` logged:

- `lab submit queue submit_id=1 base_offset=0x20 size=0x40 packet_size=0x20 packet_count=1 read_offset=0x0 write_offset=0x20`
- two successful `lab submit validate ... status=0`

## What This Means

The current submit model now has explicit structure for:

- queue header and progression inside the `control` object
- packet payload placement inside that queue
- separate status backing inside the `status` object

That is enough to move the lab model from “fixed control and status words” to
“fixed control and status words plus explicit queue semantics” without
pretending we already have real SGX execution or wrap-around behavior.

## Next Step

The next useful extension is not “make queue progression explicit.” That is
done. The next step is to decide which higher-order semantics should become
explicit next, most likely one of:

- wrap-around queue behavior
- broader queue-head or queue-tail ownership semantics
- sync or fence state tied to queue progression
- a more SGX-shaped non-executing submit record spanning control, queue, and
  status objects
