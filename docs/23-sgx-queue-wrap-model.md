# SGX Queue Wrap Model

This note records the first explicit wrap-aware queue model in the Ubuntu 5.4
Poulsbo lab driver. It extends the earlier queue milestone by distinguishing
queue occupancy at the tail of the queue region from queue occupancy that wraps
back to the head.

## Why This Step Exists

Before this step, the submit model already had:

- explicit `control/pds/use/target/status` roles
- a real in-object control block inside the `control` object
- a separate `status` object with deterministic TA and 3D status words
- explicit queue base, size, packet size, and packet count metadata

But validation still assumed a linear queue progression:

- `ccb_offset == queue_base_offset + ccb_read_offset`
- `ccb_write_offset == ccb_read_offset + queue_packet_size * queue_packet_count`

That was enough for one fixed packet or a purely linear packet run, but not for
the first realistic ring-style case where the queue tail fills and the write
pointer wraps back to the queue head.

## Current Kernel Surface

The existing submit control remains:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

The submit command surface is unchanged:

- `queue <submit_id> <base_offset> <size> <packet_size> <packet_count>`

But queue configuration and validation now derive and store two more fields in
the attached `control` object:

- dword `0x18`: `queue_tail_packet_count`
- dword `0x1c`: `queue_head_packet_count`

The live submit view also exposes:

- `queue_tail_packet_count`
- `queue_head_packet_count`
- `queue_wrap`

where `queue_wrap` is derived from `queue_head_packet_count != 0`.

## Queue Progression Semantics

The queue geometry still comes from:

- `queue_base_offset`
- `queue_size`
- `queue_packet_size`
- `queue_packet_count`

and the current packet location still comes from:

- `ccb_read_offset`
- `ccb_write_offset`
- `ccb_offset`

The new validation rules are:

- `ccb_read_offset` must be packet-aligned within the queue region
- `queue_packet_count` must fit within the queue region
- the queue can advance linearly or wrap once
- `queue_tail_packet_count` records how many packet slots fit before the queue
  region ends
- `queue_head_packet_count` records how many remaining packet slots wrap back to
  offset `0`
- `ccb_write_offset` must equal the derived wrapped write offset

This is still validate-only. The driver is not yet executing SGX work. It is
only making the queue layout and pointer progression explicit enough to study
later sync and ownership semantics on hardware.

## Validated Hardware Result

Validated on May 11, 2026 on the Sony VAIO VGN-P11Z target after deploying the
locally built module with `srcversion B88E97705620D63C25579DD`, loaded from:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`

Helper used:

- `scripts/run-vaio-sgx-submit.sh`

Default helper mode:

- `QUEUE_MODE=wrap`

Validated wrap-mode flow:

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
  - `ccb_offset=0x80`
  - `ctrl_flags=0x1`
  - `ta_status=1`
  - `3d_status=1`
  - `ta_dependency=0`
- configured the control block:
  - `ccb_write_offset=0x20`
  - `ccb_read_offset=0x60`
  - `ccb_service_address=0x1000`
  - `ccb_cache_control=0x0`
  - `ccb_data0=0xc0de0001`
  - `ccb_data1=0xc0de0002`
- configured the queue block:
  - `queue_base_offset=0x20`
  - `queue_size=0x80`
  - `queue_packet_size=0x20`
  - `queue_packet_count=2`
  - `queue_tail_packet_count=1`
  - `queue_head_packet_count=1`
  - `queue_wrap=1`
- configured the status block as before
- validated once while userspace holders were alive
- waited for all userspace holders to exit
- validated the same descriptor a second time

Observed result:

- both validates returned `status=0`
- the live submit state showed:
  - `queue_base_offset=0x20`
  - `queue_size=0x80`
  - `queue_packet_size=0x20`
  - `queue_packet_count=2`
  - `queue_tail_packet_count=1`
  - `queue_head_packet_count=1`
  - `queue_wrap=1`
- the live submit state resolved:
  - `control_gpu_va=0x80c77000`
  - `pds_gpu_va=0x80c7b000`
  - `use_gpu_va=0x80c7f000`
  - `target_gpu_va=0x80c83000`
  - `status_gpu_va=0x80c87000`
- MMU counts moved as expected:
  - `insert_pages=3 -> 13`
  - `remove_pages=1 -> 11`
- final teardown returned the object and submit views to zero entries
- a direct SGX state read after the run still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`

The target `dmesg` logged:

- `lab submit queue submit_id=1 base_offset=0x20 size=0x80 packet_size=0x20 packet_count=2 read_offset=0x60 write_offset=0x20 tail_packets=1 head_packets=1 wrapped=1`
- two successful `lab submit validate ... status=0`

## Regression Check

I also re-ran the same helper in:

- `QUEUE_MODE=nowrap`

with `HOLD_SECONDS=5`.

That older linear case still validated cleanly with:

- `queue_tail_packet_count=1`
- `queue_head_packet_count=0`
- `queue_wrap=0`
- two successful validates
- zero live submit/object entries after teardown

That confirms the new queue split logic did not break the earlier non-wrapping
path.

## What This Means

The current submit model now has explicit structure for:

- queue geometry
- queue pointer placement
- queue occupancy split across tail and head regions
- separate control, code, target, and status backing

That is enough to move the lab model from “linear packet queue” to “first
ring-style queue progression” without pretending the driver can already execute
real SGX work.

## Next Step

The next useful extension is not more queue geometry. That is now explicit.
The next step is to layer synchronization and queue ownership semantics on top
of this queue model, most likely one of:

- explicit sync or fence words
- queue-owner or queue-progress cookies
- completion state tied to the existing `status` object or a later dedicated
  state object
