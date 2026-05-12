# Reverse-Engineering Plan

## Working Assumption

No released public SGX535 open-source stack will save this project. The fastest credible path is clean-room reverse engineering backed by real hardware captures.

## Artifact Classes

We care about collecting these artifacts:

- PCI config snapshots
- MMIO access logs
- interrupt and fault observations
- GEM or buffer layout observations
- command stream payloads
- shader binaries and relocation patterns
- API-level traces for tiny GLES workloads
- frame outputs for behavioral comparison

## Reference Environments

Set up at least one known-good proprietary environment in an isolated lab image:

- legacy Windows install that supports Poulsbo
- or a legacy Linux distribution with the vendor stack, if available and reproducible

Keep that environment offline and snapshot-driven. Its only purpose is black-box observation.

## Capture Strategy

Start with tiny workloads. Avoid full desktop noise.

Recommended order:

1. context create / destroy
2. clear color
3. triangle without textures
4. textured quad
5. FBO render and resolve
6. multiple buffers and swaps

For each workload, archive:

- app binary or source
- exact driver version
- API trace
- MMIO trace
- screenshots
- notes on timing, hangs, and reset behavior

## Milestones

### M0: Baseline Inventory

Output:

- Linux hardware inventory
- upstream `gma500` behavior logs
- connector and mode list

### M1: Register Taxonomy

Output:

- map of SGX, BIF, 2D, event, and display-related registers actually touched by the working stack
- initial read/write sequencing notes

### M2: Buffer And MMU Model

Output:

- which allocations are required for the smallest working job
- page-table assumptions
- fault decoding notes

### M3: Submission Model

Output:

- command buffer structure
- job kick sequence
- completion interrupt behavior
- fence or polling model

### M4: Shader Model

Output:

- shader binary container structure
- relocation model
- vertex and fragment linkage notes

### M5: Mesa Bring-Up

Output:

- Gallium winsys
- working clear and draw
- first dEQP smoke results

## Hard Rules

- Only use publicly available information, self-produced traces, and behavior notes.
- Do not paste proprietary source into this repository.
- Do not let implementation code be written from vendor source exposure.

See [docs/06-clean-room-rules.md](/root/Poulsbo/docs/06-clean-room-rules.md) for operating constraints.
