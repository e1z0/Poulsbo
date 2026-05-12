# GEM And MMU Audit

This document records the current Poulsbo object-to-mapping path in the Ubuntu
5.4 `gma500` tree and identifies the concrete attachment points for future SGX
render work.

## Audited Files

- [gem.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/gem.c)
- [gtt.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/gtt.c)
- [gtt.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/gtt.h)
- [mmu.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/mmu.c)
- [mmu.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/mmu.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

## Current Object Path

Normal GEM object creation is currently:

1. `psb_gem_create()`
2. `psb_gtt_alloc_range()`
3. `drm_gem_object_init()`
4. `drm_gem_handle_create()`

The object implementation is not a separate SGX BO type. It is a
`struct gtt_range` with an embedded `struct drm_gem_object`.

That means current userspace-visible memory is fundamentally "GTT allocation
plus GEM wrapper", not "SGX-aware render object".

## Current Mapping Path

The first meaningful object binding happens later:

1. a GEM object is pinned through `psb_gtt_pin()`
2. pages are attached with `drm_gem_get_pages()`
3. pages are inserted into the CPU-visible GTT with `psb_gtt_insert()`
4. the same pages are inserted into the SGX MMU with
   `psb_mmu_insert_pages(psb_mmu_get_default_pd(...), ...)`

The key constraint is that every normal GEM pin currently targets the MMU
driver's default page directory. There is no per-object SGX PD selection, no
render-domain metadata, and no object-level distinction between:

- display-only scanout objects
- CPU-mmap'd dumb buffers
- hypothetical future SGX render buffers

## Current SGX MMU Topology

The driver initializes:

- one default PD in the MMU driver
- one extra `pf_pd` page directory

At load time:

- stolen memory is mapped into the default PD through
  `psb_mmu_insert_pfn_sequence()`
- the default PD is assigned to hardware context `0`
- `pf_pd` is assigned to hardware context `1`

Today, normal pinned GEM objects only use the default PD path.

## Findings

### Finding 1

`psb_gem_create()` accepted an `align` argument but ignored it, always
allocating with `PAGE_SIZE` alignment.

Status:

- fixed in [gem.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/gem.c)

### Finding 2

`psb_mmu_insert_pfn_sequence()` returned `0` even if allocation failed and
`ret` was left at `-ENOMEM`.

Status:

- fixed in [mmu.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/mmu.c)

### Finding 3

The current fbdev 2D lab path runs against stolen framebuffer memory and does
not exercise normal GEM-to-MMU insert/remove flows.

Evidence:

- the 2D lab produced SGX `TWOD_COMPLETE` interrupts
- the new MMU operation counters did not change during that run

Interpretation:

- the deterministic 2D lab is valid SGX/TWOD coverage
- it is not a proxy for the GEM/dumb-buffer binding path

### Finding 4

The DRM dumb-buffer path does exercise the GEM-to-MMU layer.

Evidence from the VAIO after `drm_probe`:

- `flush` changed from `2` to `4`
- `insert_pages` changed from `2` to `3`
- `remove_pages` changed from `0` to `1`

Interpretation:

- the new counters are attached to the correct layer
- dumb-buffer mmap and teardown are a useful lab path for future MMU work

## New Observability

The driver now exposes:

- [poulsbo_gtt_state](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

That view reports:

- current GTT resource ranges
- per-range pin/unpin counters
- live `in_gart`, `mmapping`, `npage`, and `roll`
- MMU operation counts
- the last MMU operation address, type, page count, and hardware context

This is the current low-risk inspection surface for GEM/GTT/MMU behavior on the
VAIO.

## Practical Attachment Points For Render Work

The next real render-side structure should attach at one of these seams:

1. Extend `gtt_range` with SGX render ownership metadata.
   This is the smallest incremental step, because `gtt_range` is already the
   real object that gets pinned and mapped.

2. Add an explicit SGX bind/unbind helper above `psb_mmu_insert_pages()` and
   `psb_mmu_remove_pages()`.
   This is the cleaner place to distinguish display-only mappings from future
   render mappings.

3. Introduce a render-side wrapper that references a `gtt_range` instead of
   exposing raw `gtt_range` objects directly.
   This is the more scalable path if a future submit API needs relocations,
   fences, domains, or per-context bindings.

## Recommended Next Step

Do not jump directly to a public render ioctl.

First:

- decide what the first lab-grade render object should be
- decide whether that object owns a `gtt_range` or is a decorated `gtt_range`
- define one explicit MMU bind/unbind path for that object type

Only then does it make sense to sketch job submission around it.
