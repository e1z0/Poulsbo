# Kernel Module

This is a lab-only out-of-tree kernel module target for Ubuntu 18.04 builds.

## Scope

Right now it is intentionally small:

- compiles against Ubuntu 18.04 generic headers
- detects Poulsbo PCI IDs without binding to the device
- provides a stable compile harness for future kernel-side instrumentation work

It does **not** replace `gma500` and it does not claim the PCI device.

## Default Target

The default header line is:

- `4.15.0-213-generic`

Override with:

```bash
make -C module KVER=<kernel-version>
```

Or point at a custom tree:

```bash
make -C module KDIR=/path/to/kernel/build
```
