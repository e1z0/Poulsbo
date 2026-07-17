# Poulsbo SGX — accelerated GMA500 driver for Ubuntu 18.04 LTS

Open + closed driver stack that gives the **Intel GMA500 / Poulsbo (PowerVR
SGX535)** real graphics acceleration on Ubuntu 18.04 LTS:

- **Accelerated 2D on X11** — the SGX 2D BLIT engine drives window fills, copies
  and scrolls through an EXA driver (`xf86-video-gma500blt`), instead of the
  unaccelerated `modesetting` fallback.
- **OpenGL ES 2.0 on the SGX535** — the Intel EMGD EGL/GLES runtime on a
  kernel `pvrsrvkm` render module, with a **zero-copy hardware page-flip
  present** (`eglSwapBuffers` flips the scanned-out buffer directly — no CPU
  copy), driven by SGX interrupts (~1 ms/present).
- **Accelerated 2D on the framebuffer console** (SGX 2D BLIT).

After you install the packages and reboot, X comes up accelerated automatically.

> This targets one specific configuration: **Ubuntu 18.04 LTS, kernel
> `5.4.0-150-generic` (i686/i386)** on Poulsbo hardware (e.g. the Sony Vaio P /
> similar US15W netbooks). The prebuilt modules load only on that kernel; for a
> different kernel, rebuild them (see [PACKAGING.md](PACKAGING.md)).
>
> The OpenGL ES side uses closed Intel EMGD binaries, redistributed here for
> personal use on Poulsbo hardware. The kernel modules and the 2D X driver are
> open. See [ENGINEERING.md](ENGINEERING.md) for how it all works.

---

## Install

Grab the three `.deb` files (from `packaging/out/`, or build them per
[PACKAGING.md](PACKAGING.md)) and install:

```bash
sudo dpkg -i poulsbo-sgx-modules_1.0.0_i386.deb \
             poulsbo-sgx-userspace_1.0.0_i386.deb \
             poulsbo-sgx_1.0.0_all.deb
sudo reboot
```

What the packages do on install:

- blacklist the stock `gma500_gfx` and load the accelerated stack
  (`pvrsrvkm` → `gma500_gfx` → `gma500dc`) at boot via a systemd unit;
- rebuild the initramfs so the stock driver never grabs the device first;
- install the EMGD GL runtime under `/opt/emgd/lib` and register it with the
  dynamic linker;
- install the `gma500blt` X driver and an `xorg.conf.d` snippet that selects it;
- bring the SGX up (`srvinit`) before the login screen so GL apps work.

The three packages:

| Package | Contents |
|---|---|
| `poulsbo-sgx-modules` | the 3 kernel modules + boot loader service (kernel-specific) |
| `poulsbo-sgx-userspace` | EMGD EGL/GLES2 runtime, flip WSEGL, `gma500blt` EXA X driver |
| `poulsbo-sgx` | metapackage that pulls in both |

For the **2D X acceleration** you also need Ubuntu 18.04's HWE X server
(video ABI 24): `sudo apt install xserver-xorg-core-hwe-18.04`. On the base X
server the `gma500blt` driver won't load and X falls back to `modesetting`
(still works, just unaccelerated).

## Verify

```bash
# stack loaded on boot?
journalctl -t poulsbo-sgx -b        # -> pvrsrvkm=0 gma500_gfx=0 gma500dc=0
lsmod | grep -E 'pvrsrvkm|gma500'

# X using the accelerated driver?
grep -E 'gma500blt|EXA' /var/log/Xorg.0.log

# OpenGL ES renderer (in an X session):
#   an EGL/GLES2 app reports  GL_RENDERER = "EMGD on PowerVR SGX 535"
```

---

## Kernel module parameters

All parameters are runtime-writable under `/sys/module/<module>/parameters/`,
and can be set persistently for boot in **`/etc/default/poulsbo-sgx`** (read by
the loader). Change that file then reboot (a reboot is the reliable way to
reload the modules — hot reload can hang the machine).

| Parameter | Module | Default | Meaning |
|---|---|---|---|
| `pvr_irq_forward` | `gma500_gfx` | `1` | Forward SGX interrupts to `pvrsrvkm` so GL present is interrupt-driven (~1 ms, ~950 FPS). Set `0` to fall back to `pvrsrvkm`'s internal ~250 ms poll (~4 FPS) — diagnostic only. |
| `flip_vsync` | `gma500_gfx` | `0` | `0` = present as soon as the buffer is armed (max rate). `1` = throttle presents to roughly the panel refresh and reduce tearing, via a deferred completion that does **not** block the SGX command pipeline. **Note:** on this hardware the closed present-completion path limits the throttled rate to ~15–20 FPS for trivial workloads, so `1` is for tear-sensitive/static content, not maximum FPS. (A phase-locked vblank-IRQ variant reached ~40 FPS tear-free but proved unstable on this box, so the stable timer-based throttle is shipped.) |
| `flip_enable` | `gma500dc` | `1` | `1` = zero-copy hardware page-flip present. `0` = complete presents without moving the scanout (diagnostic). |
| `gma_no_sgx` | `gma500_gfx` | `1` | Hand the SGX render core to `pvrsrvkm` (leave at 1). |

Examples:

```bash
# tear-free presentation for this session:
echo 1 | sudo tee /sys/module/gma500_gfx/parameters/flip_vsync

# make it persistent across reboots:
sudo sed -i 's/^FLIP_VSYNC=.*/FLIP_VSYNC=1/' /etc/default/poulsbo-sgx
```

`gma500_gfx` also exposes experimental `sgx3d_*` parameters used during 3D
bring-up; leave them at their defaults for normal use (see ENGINEERING.md).

---

## Uninstall / revert to the stock driver

```bash
sudo apt-get remove poulsbo-sgx poulsbo-sgx-modules poulsbo-sgx-userspace
# or: sudo dpkg -r poulsbo-sgx poulsbo-sgx-modules poulsbo-sgx-userspace
sudo reboot
```

Removing `poulsbo-sgx-modules` un-blacklists the stock `gma500_gfx` and rebuilds
the initramfs, so the next boot uses the in-kernel driver again.

## Troubleshooting

- **No acceleration / black screen after boot** — check
  `journalctl -t poulsbo-sgx -b`. If a load didn't complete, the loader skips on
  the next boot (leaving you unaccelerated but bootable). Remove the package to
  return to stock, or investigate `dmesg`.
- **X didn't start / fell back to modesetting** — the `gma500blt` driver needs
  the HWE X server (ABI 24). Install `xserver-xorg-core-hwe-18.04`, or disable
  the accelerated X driver by renaming
  `/usr/share/X11/xorg.conf.d/20-poulsbo-gma500.conf`.
- **GL app can't initialise EGL** — make sure the SGX came up:
  `systemctl status poulsbo-sgx-srvinit.service` and
  `journalctl -t poulsbo-sgx -b | grep srvinit`. Re-run with
  `sudo /opt/poulsbo-sgx/srvinit.sh`.
- **Wrong kernel** — these modules are built for `5.4.0-150-generic`. On another
  kernel they won't load; rebuild per [PACKAGING.md](PACKAGING.md).

## More

- **[ENGINEERING.md](ENGINEERING.md)** — how the driver works: the
  gma500/pvrsrvkm coexistence, the PVR-over-DRM bridge, the zero-copy flip chain,
  the SGX interrupt forwarding, the SGX535 bring-up, and the register-level
  details.
- **[PACKAGING.md](PACKAGING.md)** — build the kernel modules and the `.deb`
  packages yourself.
