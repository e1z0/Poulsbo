# Overview

## Hardware Model

Poulsbo is not a normal Intel integrated GPU in the `i915` sense. The platform combines:

- an Intel display engine handled today by the upstream `gma500` DRM driver
- an Imagination PowerVR SGX535 render core that never received an upstream open 3D stack

The result is a split problem:

- display is mostly a Linux DRM/KMS problem that already has an upstream base
- acceleration is a render submission, memory-management, and Mesa compiler problem

## Project Goal

Build an open stack that can do all of the following on real Poulsbo hardware:

- boot with upstream-style DRM/KMS display support
- expose a stable SGX535 render path to Mesa
- run GLES 2.0 workloads with hardware acceleration
- accelerate Xorg through `modesetting` + `glamor`

## Realistic API Target

The first serious target is:

- OpenGL ES 2.0
- enough Gallium integration to support desktop OpenGL 2.x where Mesa lowering makes that feasible

This matches the era and capability class of SGX535 much better than chasing modern OpenGL or Vulkan.

## Success Criteria

Phase success is defined by behavior, not by code volume.

### Phase 0

- reproducible hardware capture workflow
- stable archive of target logs, traces, and artifact naming

### Phase 1

- kernel can identify and recover from basic SGX faults
- kernel can submit a minimal render job and wait on completion

### Phase 2

- Mesa can clear, draw a triangle, sample a texture, and render to an FBO

### Phase 3

- Xorg `modesetting` + `glamor` loads successfully on the hardware path
- desktop feels accelerated rather than llvmpipe-bound

## Why Xorg Is Not The First Problem

Xorg no longer wants a large chip-specific 2D driver if a GL-based acceleration path exists. The modern path is:

1. DRM/KMS in the kernel
2. Mesa driver for rendering
3. Xorg `modesetting` DDX
4. `glamor` for 2D acceleration over GL

If Mesa is missing, a custom Xorg DDX does not solve the core problem.
