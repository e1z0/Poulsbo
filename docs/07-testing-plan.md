# Testing Plan

## Hardware Targets

Minimum required:

- one real Poulsbo machine for Linux bring-up

Strongly recommended:

- a second Poulsbo system for regression confirmation
- at least one non-Poulsbo `gma500` family system for sanity checking shared code paths

## Test Layers

### Layer 0: Inventory

- PCI IDs
- connector topology
- current `gma500` behavior

### Layer 1: Kernel

- register reads
- reset sequence
- fault decode
- minimal job completion

### Layer 2: Mesa

- context create
- clear
- triangle
- texture
- FBO

### Layer 3: User Space

- EGL/GLES sample apps
- piglit subset
- dEQP GLES2 subset

### Layer 4: Desktop

- Xorg `modesetting`
- `glamor` initialization
- compositor smoke test after baseline Xorg works

## Failure Capture Rules

For every hang or fault, save:

- exact kernel revision
- exact Mesa revision
- workload name
- full kernel log
- any MMIO or command trace
- screenshot if visible corruption happened

## Exit Criteria Per Phase

Do not advance based on optimism.

### Kernel exit criteria

- can recover from a deliberate bad submit without reboot

### Mesa exit criteria

- repeated clear and triangle runs pass for hours without hangs

### Xorg exit criteria

- `glamor` loads reliably and software fallback is not the normal path
