# Additional Source Triage

This document records the usefulness of later source leads suggested after the
initial repo bootstrap.

The goal is not to collect every Poulsbo-adjacent link. The goal is to decide
which sources are worth spending time on for the open driver effort.

## Highest-Value Leads

### Old Intel / EMGD / OpenEmbedded SGX trees

Most useful source found so far:

- Yocto EMGD patchset carrying a large `drivers/gpu/drm/emgd/pvr/services4`
  tree:
  - https://docs.yoctoproject.org/pipermail/poky/2011-May/006149.html
- Yocto CDV patchset carrying a large `drivers/staging/cdv/pvr/services4` tree:
  - https://www.mail-archive.com/yocto%40yoctoproject.org/msg05861.html

Why this matters:

- these trees are Intel-platform oriented, not just TI-platform oriented
- they explicitly carry `sgx535defs.h`
- they also carry SGX kernel-side building blocks such as:
  - `sgxinit.c`
  - `sgxkick.c`
  - `sgxpower.c`
  - `sgxreset.c`
  - `sgxtransfer.c`
  - `mmu.c`
  - `pvr_drm.c`
  - Intel/MRST/CDV DRM glue files

Practical use:

- architectural reference for job submission and reset flow
- naming reference for bridge and MMU concepts
- historical evidence for how Intel integrated SGX into a Linux DRM stack

Constraints:

- this is legacy vendor code, not something to merge directly into the current
  repo
- use as reference input, not as a drop-in implementation
- keep clean-room discipline around any code copied or rewritten

## Useful But Platform-Skewed Leads

### TI SGX DDK kernel tree

- TI SGX kernel tree:
  - https://git.ti.com/cgit/graphics/omap5-sgx-ddk-linux/
- TI SGX build guide:
  - https://software-dl.ti.com/processor-sdk-linux/esd/AM437X/09_03_05_02/exports/docs/linux/Foundational_Components/Graphics/SGX/Build_Guide.html

Why it helps:

- TI explicitly documents the split:
  - kernel module is open source
  - GLES implementation is closed source
  - Mesa shim is open source
- the tree is a real SGX kernel stack and includes SGX535 references

Why it is limited:

- the build and platform assumptions are TI SoC specific
- the userspace/kmod/firmware matching model is extremely brittle
- it is not an Intel/Poulsbo integration

Best use:

- compare SGX-side MMU, power, reset, and userspace-server concepts
- mine `sgx535defs.h` and related hwdefs for register naming and feature
  structure

## Useful As Project Precedent

### OpenPVRSGX

- repository:
  - https://github.com/openpvrsgx-devgroup/linux_openpvrsgx
- related 2023 thread:
  - https://lists.goldelico.com/pipermail/openpvrsgx-devgroup/2023-June/000556.html

Why it helps:

- it proves there is still active community effort around SGX5 kernel work
- it gives binding, glue, and upstreaming precedent
- the discussion states an important design constraint: SGX firmware,
  userspace, and kernel pieces are tightly matched rather than universal

Why it is limited:

- the project is focused on shaping existing kernel drivers, not reverse
  engineering SGX userspace
- the thread explicitly contrasts SGX with newer Rogue work and notes the lack
  of universal SGX userspace components

Best use:

- precedent for kernel driver structure and upstreaming strategy
- not a replacement for Poulsbo-specific reverse engineering

## Useful For Reference Environments Only

### EMGD community binaries

- https://github.com/EMGD-Community/intel-binaries-linux

Why it helps:

- it preserves Intel EMGD binaries, docs, and packaging material
- it may help build a legacy reference environment for trace capture

Why it is limited:

- it is not an open SGX userspace implementation
- it is closer to a vendor stack archive than a clean design reference

Best use:

- trace capture
- behavior comparison
- installation and configuration archaeology

### Recent community forum pointer

- https://forums.imgtec.com/t/request-for-powervr-sgx-535-linux-driver-source-code/4136

Why it helps:

- it confirms there is no current Imagination plan to revive SGX in that way
- it points at the TI open-kernel/closed-userspace split

Why it is limited:

- it is not technical documentation by itself

## Low-Value Or Misleading Leads

### GMA500 Booster overclock blog

- https://gma500booster.blogspot.com/2013/06/internal-gma-500-vs-gma-600-overclock.html

Why it is low value:

- it is mainly Windows-era tweaking and clock experimentation
- it may provide a few chipset/clock clues, but not a render-driver plan

Best use:

- optional hardware trivia only

### Reddit post about Rogue ISA docs

- https://www.reddit.com/r/linux/comments/2jvwrm/powervr_sdk_v34_released_with_full_isa/

Why it is low value for Poulsbo:

- the thread itself notes the docs are for newer Series 6 "Rogue" hardware
- Poulsbo is SGX535, not Rogue

Best use:

- none for direct SGX535 bring-up beyond historical context

## Likely Useful But Not Yet Confirmed Here

### 2026 lore thread

- https://lore.kernel.org/dri-devel/20260305081140.GA171266@linux.fritz.box/

Current assessment:

- likely useful for recent `gma500` display-side maintenance context
- unlikely to change the SGX render conclusion by itself

Status:

- not yet triaged in detail from this environment

## Current Recommendation

If time is spent mining external source trees, the priority order should be:

1. old Intel/EMGD/OpenEmbedded `pvr/services4` trees
2. TI SGX kernel tree and build documentation
3. OpenPVRSGX kernel/upstreaming work
4. EMGD binaries only for reference-stack setup

The overclock blog and Rogue-era Reddit thread should not drive engineering
decisions for the open Poulsbo render path.
