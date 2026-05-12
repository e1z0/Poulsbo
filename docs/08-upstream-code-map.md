# Upstream Code Map

This is the current upstream `drivers/gpu/drm/gma500/` map as it matters for a Poulsbo render bring-up.

## Files That Matter First

- `psb_drv.c`
  - PCI probe, DRM driver registration, device tables, driver feature flags.
  - First place to inspect when adding a private experimental render uAPI.

- `psb_drv.h`
  - central per-device structures and shared declarations.
  - likely home for new SGX job, fence, and fault-tracking state.

- `psb_device.c`
  - Poulsbo-specific device initialization and platform wiring.
  - first likely location for SGX block initialization sequencing.

- `psb_irq.c`
  - interrupt handling for PSB-class hardware.
  - first patch point for event decode, completion tracking, and fault capture.

- `psb_irq.h`
  - IRQ-related declarations.

- `mmu.c` and `mmu.h`
  - existing MMU and page-directory support.
  - high-value files for understanding BIF directory list programming and page-fault behavior.

- `gtt.c` and `gtt.h`
  - GTT mapping and memory plumbing used by the display path.
  - likely reused for BO management contracts and scanout sharing.

- `gem.c` and `gem.h`
  - GEM object allocation and dumb-buffer support.
  - starting point for render BO flags, mapping rules, and submit object ownership.

- `psb_reg.h`
  - the most important public SGX/BIF/2D register header.
  - source of truth for the initial register taxonomy in this repo.

## Display-Side Files

These are useful, but not the first RE patch targets:

- `psb_intel_display.c`
- `psb_intel_modes.c`
- `psb_intel_lvds.c`
- `psb_intel_sdvo.c`
- `psb_intel_drv.h`
- `psb_intel_reg.h`
- `psb_intel_sdvo_regs.h`
- `framebuffer.c`
- `fbdev.c`
- `backlight.c`
- `opregion.c`
- `power.c`

These matter when validating scanout, power, and mode behavior, but they are not where the missing SGX render path will be built.

## Shared Or Family-Level Files

- `gma_device.c` and `gma_device.h`
- `gma_display.c` and `gma_display.h`
- `intel_bios.c` and `intel_bios.h`
- `intel_gmbus.c`
- `intel_i2c.c`

These are shared support layers. Touch them only if the render work proves a common abstraction is actually needed.

## Non-Poulsbo Device Families In The Same Driver

- `oaktrail_*`
- `cdv_*`
- `mid_bios.*`
- `oaktrail.h`
- `cdv_device.*`

These files support related but different `gma500` family hardware. They are useful for avoiding regressions, but they are noise during the first Poulsbo SGX bring-up.

## First Patch Sites

If we were starting kernel changes against an upstream tree tomorrow, the first instrumentation work would likely go here:

1. `psb_irq.c`
   - decode `PSB_CR_EVENT_STATUS`, `PSB_CR_EVENT_STATUS2`, and `PSB_CR_BIF_FAULT`
2. `psb_device.c`
   - add SGX reset and debug helpers
3. `gem.c`
   - prototype narrow BO usage flags for render experiments
4. `psb_drv.c`
   - hang debug IOCTL or debugfs exposure for controlled lab builds
5. `mmu.c`
   - capture and inspect directory list programming

## Working Rule

Do not start by scattering logging everywhere. Instrument only the paths that answer one of these questions:

- did the engine get kicked?
- did it fault?
- where did the MMU point?
- did the IRQ fire?
- did reset actually recover the engine?
