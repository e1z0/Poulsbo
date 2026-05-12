# PF-PD Bind Lab

This note records the first explicit SGX-aware bind path added to the Ubuntu
5.4 `gma500` lab tree.

The intent is narrow: prove that a live non-stolen `gtt_range` can be mapped
into the secondary SGX page directory (`pf_pd`, hardware context `1`) with
explicit bind and unbind lifetime, while keeping the existing fbdev and dumb
buffer paths intact.

## What Was Added

Kernel-side changes:

- `gtt_range` now tracks:
  - `sgx_pf_bind_count`
  - `sgx_pf_bind_total`
  - `sgx_pf_unbind_total`
  - `last_sgx_pf_bind_jiffies`
  - `last_sgx_pf_unbind_jiffies`
- `gtt.c` now has internal locked helpers for:
  - range lookup by `debug_id`
  - shared pin / unpin logic
  - lab PF-PD bind / unbind
- `psb_drv.c` now exposes:
  - `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_bind`
  - `pf_bind <range_id>`
  - `pf_unbind <range_id>`
- `poulsbo_gtt_state` now prints PF-PD bind counters per live range.

Userspace-side changes:

- `workloads/drm_probe.c` now accepts an optional hold duration in seconds as a
  second positional argument.
- `scripts/run-vaio-pf-bind.sh` deploys that probe, creates a live dumb GEM
  object on the VAIO, identifies its `range id`, performs `pf_bind`, performs
  `pf_unbind`, waits for probe exit, and prints the relevant GTT/MMU state.

## Binding Model

The current lab model is deliberately conservative:

- the bind target is an existing `gtt_range`
- the PF-PD mapping uses the same GPU virtual address as the normal GTT range:
  - `gpu_va = dev_priv->gtt.gatt_start + gt->offset`
- the first PF bind:
  - pins the range if needed
  - inserts pages into `pf_pd`
- the last PF unbind:
  - removes pages from `pf_pd`
  - drops the extra pin held for the PF bind lifetime

This does not create a render object or a submit path. It only proves that an
explicit secondary-context bind lifetime can exist over the current GEM/GTT/MMU
path.

## Validated Result On The VAIO

Validated on `2026-05-10` on `10.1.11.218` with locally built module
`srcversion D18FBB9CEBF42668699D620`.

The helper run:

```bash
./scripts/run-vaio-pf-bind.sh
```

produced a live non-stolen range:

- `range id=10`
- `name=gem`
- `gpu_va=0x81127000`
- `npage=4`
- `mmapping=1`

Observed MMU transitions:

- before the held probe:
  - `flush=2`
  - `insert_pages=2`
  - `remove_pages=0`
- after the held probe created and mmap'd the dumb buffer:
  - `flush=3`
  - `insert_pages=3`
  - `remove_pages=0`
- after `pf_bind 10`:
  - `flush=4`
  - `insert_pages=4`
  - `remove_pages=0`
  - `pf_bind_count=1`
  - `pf_bind_total=1`
- after `pf_unbind 10`:
  - `flush=5`
  - `insert_pages=4`
  - `remove_pages=1`
  - `pf_bind_count=0`
  - `pf_unbind_total=1`
- after the probe exited and destroyed the dumb buffer:
  - `flush=6`
  - `insert_pages=4`
  - `remove_pages=2`
  - range `id=10` disappeared from the live resource tree

Observed kernel log lines:

- `lab pf_bind id=10 name=gem gpu_va=0x81127000 npage=4 active=1 total=1`
- `lab pf_unbind id=10 name=gem gpu_va=0x81127000 active=0 total=1`

Observed SGX state after the run:

- `fault_count=0`
- `bif_fault=0x000000`

## What This Establishes

- explicit context-aware mapping is now real in the lab driver
- `pf_pd` mapping lifetime can be observed separately from normal dumb-buffer
  lifetime
- the current `gtt_range` object is sufficient for first bind/unbind studies
- no SGX render submission is implied by this result

## What This Does Not Solve

- no render object metadata yet
- no SGX job submission path
- no PDS/USE/TA/3D command programming
- no fence or sync model
- no userspace render uAPI
- no Mesa acceleration

## Next Work

- decide whether the next step extends `gtt_range` again or wraps it in a new
  SGX render object
- capture which SGX/BIF registers move, if any, around future PF-PD experiments
- add a controlled lab interface for object setup beyond raw bind/unbind
- define the first submission-path experiment only after the object model is
  explicit
