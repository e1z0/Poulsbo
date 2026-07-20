# Poulsbo SGX — accelerated GMA500 driver for Ubuntu 18.04 LTS

Open + closed driver stack that gives the **Intel GMA500 / Poulsbo (PowerVR
SGX535)** real graphics acceleration on Ubuntu 18.04 LTS. What actually runs and
displays on the panel today (the "Path B" configuration, commit `fc6a520f`, HW
verified 2026-07-20 — blitter hangs 0, SGX faults 0):

- **Hardware-accelerated desktop OpenGL (GLX / DRI2) on X11** — real
  **OpenGL 2.1** on the SGX535 for ordinary X apps. `glxinfo` reports
  *direct rendering: Yes* and *renderer: EMGD on PowerVR SGX535*; `glxgears`
  renders **and displays** in a window at **~66 FPS** (measured on hardware).
- **Hardware 2D for offscreen X pixmaps** — the EXA driver
  (`xf86-video-gma500blt`) allocates offscreen pixmaps with `PVR2DMemAlloc` and
  fills/copies them with **`PVR2DBlt`** — the SGX microkernel's *own* 2D path,
  which coexists with OpenGL by design. See the important caveat below: **on-screen
  (scanout) 2D is still CPU**, so window dragging is not smooth.
- **A stable MATE desktop** using **marco** (the non-compositing window manager).
  **Compiz / GL compositing does NOT work on this stack and must not be run** — it
  renders black and has hard-hung the machine (see *What does not work*).
- **OpenGL ES 2.0 on the SGX535** — the Intel EMGD EGL/GLES runtime on a kernel
  `pvrsrvkm` render module, interrupt-driven present.

After you install the packages and reboot, a systemd unit brings up the marco +
HW-2D-pixmap + HW-GL desktop automatically (no compiz).

> **The core finding you should know first.** The open, direct-MMIO **2D-BLT
> engine** and the closed **SGX microkernel** (required for OpenGL/GLX/DRI2)
> *conflict* on the shared graphics-core BIF: `SGXInitialise` reroutes the 2D
> requestor through the SGX MMU (`BIF_BANK0` `INDEX_2D`, `BIF_DIR_LIST_BASE0`,
> `BIF_TWOD_REQ_BASE`), so the direct 2D-BLT engine wedges the moment SGX is up.
> The standalone 2D-BLT test passes **12/12 with SGX uninitialised** and fails
> **5/12 once SGX is initialised**. This is why earlier "accelerated composited
> desktop" claims never actually reached the panel. Path B sidesteps the conflict
> by doing X 2D through the SGX's own `PVR2DBlt` instead of the direct engine.
> See [ENGINEERING.md](ENGINEERING.md) §0 and §9.

> **Note on GL performance:** `glxgears` ~66 FPS is measured on hardware. OpenGL
> is version **2.1** (fixed-function + GLSL 1.20), which is what the SGX535
> realistically does.

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

This is a **development snapshot**, not a validated release — it brings up a
usable HW-accelerated marco desktop, with the known limitations listed below.
Grab the three `.deb` files (from `packaging/out/`, or build them per
[PACKAGING.md](PACKAGING.md)) and install (the version string in the filenames is
whatever you built with):

```bash
sudo dpkg -i poulsbo-sgx-modules_*_i386.deb \
             poulsbo-sgx-userspace_*_i386.deb \
             poulsbo-sgx_*_all.deb
sudo reboot
```

What the packages do on install:

- blacklist the stock `gma500_gfx` and load the accelerated stack
  (`pvrsrvkm` → `gma500_gfx` → `gma500dc`, plus `proc_dri_compat`) at boot via a
  systemd unit;
- rebuild the initramfs so the stock driver never grabs the device first;
- install the EMGD GL runtime under `/opt/emgd/lib` and register it with the
  dynamic linker;
- install the `gma500blt` X driver and an `xorg.conf.d` snippet that selects it;
- **enable accelerated GLX/DRI2**: put the `emgd` DRI driver on the Mesa loader
  path, set `MESA_LOADER_DRIVER_OVERRIDE=emgd`, apply a small `libGLX_mesa`
  patch, and install the AIGLX/DRI2 `xorg.conf` (so X comes up with accelerated
  OpenGL);
- bring the SGX up (`srvinit`) before the login screen so GL apps work;
- set up the auto-start desktop (getty autologin on tty1 → `startx` on vt7 via
  root's `.bash_profile` + a marco session xinitrc) so the box boots straight to
  the accelerated **marco** desktop — deliberately **without** compiz.

The three packages:

| Package | Contents |
|---|---|
| `poulsbo-sgx-modules` | the kernel modules (`pvrsrvkm`, `gma500_gfx`, `gma500dc`, `proc_dri_compat`) + boot loader service (kernel-specific) |
| `poulsbo-sgx-userspace` | EMGD EGL/GLES2 runtime, DDK WSEGL, `gma500blt` EXA + DRI2 X driver, GLX/DRI2 enablement (`emgd_dri.so` on the Mesa path, Mesa override, `xorg.conf.glx`) |
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

# Desktop OpenGL (GLX) accelerated? (in an X session)
glxinfo | grep -E 'direct rendering|OpenGL renderer|OpenGL version'
#   -> direct rendering: Yes
#   -> OpenGL renderer string: EMGD on PowerVR SGX535
#   -> OpenGL version string: 2.1 ...
glxgears        # renders AND displays, ~66 FPS (measured on HW)

# OpenGL ES renderer (in an X session):
#   an EGL/GLES2 app reports  GL_RENDERER = "EMGD on PowerVR SGX 535"
```

---

## Running the accelerated desktop (MATE + marco)

After install + reboot the box boots straight to the desktop (getty autologin on
tty1 → `startx`; marco window manager, HW 2D for offscreen pixmaps via
`PVR2DBlt`, HW OpenGL). To start it by hand from a bare session, make sure the
Mesa override is in the environment (the package sets it in `/etc/environment`, so
a fresh login already has it):

```bash
export MESA_LOADER_DRIVER_OVERRIDE=emgd     # already set by the package
marco &                                     # NON-compositing WM (deliberate)
mate-panel &
```

GL apps (e.g. `glxgears`) render and display accelerated on this desktop.

**Do not run compiz** (or any GL compositor) on this stack. See below.

## What does not work / known limitations

- **On-screen (scanout) 2D is CPU, not accelerated.** The KMS front buffer is not
  SGX-mapped, so `PVR2DBlt` cannot target it; screen-to-screen copies (e.g.
  dragging a window) fall back to the CPU on uncached memory. Result: **dragging
  tears / is not smooth.** Offscreen pixmap 2D is HW-accelerated; on-screen is not
  (yet).
- **Compiz / GL compositing does NOT work and is unsafe.** It bypasses the DDX
  hooks (going through the closed emgd GLX), renders a **black screen**, and has
  **hard-hung the machine** (full network death, requiring a physical power
  cycle). The desktop uses marco (non-compositing) on purpose. Completing compiz
  would require reverse-engineering the closed emgd GLX present path.
- **The direct 2D-BLT ioctl is not usable alongside OpenGL.** The direct-MMIO 2D
  engine (`features/blt-ioctl`) is proven 12/12 on hardware — but **only with the
  SGX uninitialised**. Once the SGX/GL stack is up it wedges the core (the BIF
  conflict above). Path B's HW 2D therefore goes through `PVR2DBlt`, not the
  direct engine.
- **No framebuffer readback.** This DDX has no `GetImage` path, so
  `import`/screenshots return black — a debugging false-negative, not a rendering
  bug (ground truth is a photo of the physical panel).

---

## Kernel module parameters

All parameters are runtime-writable under `/sys/module/<module>/parameters/`,
and can be set persistently for boot in **`/etc/default/poulsbo-sgx`** (read by
the loader). Change that file then reboot (a reboot is the reliable way to
reload the modules — hot reload can hang the machine).

| Parameter | Module | Default | Meaning |
|---|---|---|---|
| `pvr_irq_forward` | `gma500_gfx` | `1` | Forward SGX interrupts to `pvrsrvkm` so GL present is interrupt-driven instead of poll-bound (the internal poll is ~250 ms). Leave at `1`; `0` is diagnostic only. (The large FPS deltas quoted for this in older notes came from an EGL/GLES2 offscreen bench, not the marco desktop.) |
| `flip_vsync` | `gma500_gfx` | `0` | `0` = present as soon as the buffer is armed (max rate). `1` = throttle presents to roughly the panel refresh and reduce tearing, via a deferred completion that does **not** block the SGX command pipeline. **Note:** on this hardware the closed present-completion path limits the throttled rate to ~15–20 FPS for trivial workloads, so `1` is for tear-sensitive/static content, not maximum FPS. (A phase-locked vblank-IRQ variant reached ~40 FPS tear-free but proved unstable on this box, so the stable timer-based throttle is shipped.) |
| `flip_enable` | `gma500dc` | `1` | `1` = zero-copy hardware page-flip present. `0` = complete presents without moving the scanout (diagnostic). |
| `gma_no_sgx` | `gma500_gfx` | `1` | Hand the SGX render core to `pvrsrvkm` (leave at 1). |
| `event_timeout_ms` | `pvrsrvkm` | `2` | How long the GL client's event-object wait re-checks for SGX render completion. The stock value was a hard 100 ms, which cost the EMGD GL client a full timeout every frame; `2` shortens that. Runtime-tunable. |
| `sgx_apm_latency_ms` | `pvrsrvkm` | `500` | SGX active-power-management idle latency before the core powers down. The Poulsbo default was 1 ms, so the SGX powered off between every frame and paid a power-up penalty; `500` keeps it awake under load. `0` disables active PM entirely. Runtime-tunable. |

Examples:

```bash
# tear-free presentation for this session:
echo 1 | sudo tee /sys/module/gma500_gfx/parameters/flip_vsync

# make it persistent across reboots:
sudo sed -i 's/^FLIP_VSYNC=.*/FLIP_VSYNC=1/' /etc/default/poulsbo-sgx

# tune the GLX-speed pvrsrvkm knobs live (0644, no reload needed):
echo 2   | sudo tee /sys/module/pvrsrvkm/parameters/event_timeout_ms
echo 500 | sudo tee /sys/module/pvrsrvkm/parameters/sgx_apm_latency_ms
```

The two `pvrsrvkm` knobs above already default to their fast values; they are
exposed so you can A/B the GLX performance fixes at runtime.

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
- **`glxinfo` says `llvmpipe`/`softpipe` (not SGX535)** — the emgd DRI driver
  didn't load. Check that `MESA_LOADER_DRIVER_OVERRIDE=emgd` is in your
  environment (it's in `/etc/environment` — re-login if you just installed), that
  `proc_dri_compat` loaded (`ls /proc/dri/0/name`), and that
  `/usr/lib/i386-linux-gnu/dri/emgd_dri.so` exists. `srvinit` must also have run
  (see below). Re-run `sudo /opt/poulsbo-sgx-glx/glx-enable.sh` to reapply the
  Mesa-side setup.
- **GL app can't initialise EGL** — make sure the SGX came up:
  `systemctl status poulsbo-sgx-srvinit.service` and
  `journalctl -t poulsbo-sgx -b | grep srvinit`. Re-run with
  `sudo /opt/poulsbo-sgx/srvinit.sh`.
- **Wrong kernel** — these modules are built for `5.4.0-150-generic`. On another
  kernel they won't load; rebuild per [PACKAGING.md](PACKAGING.md).

## More

- **[ENGINEERING.md](ENGINEERING.md)** — how the driver works: the
  gma500/pvrsrvkm coexistence, the PVR-over-DRM bridge, the zero-copy flip chain,
  the GLX/DRI2 buffer bridge, the SGX interrupt forwarding, the SGX535 bring-up,
  and the register-level details.
- **[PACKAGING.md](PACKAGING.md)** — build the kernel modules and the `.deb`
  packages yourself.
