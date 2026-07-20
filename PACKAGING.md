# Building the Poulsbo SGX packages yourself

This describes how to rebuild the three `.deb` packages from source — both the
kernel modules and the package assembly. Everything lives under `packaging/`,
with the module sources under `features/`.

This is a **development snapshot**, not a validated release. It builds a stack
that brings up a HW-accelerated **marco** desktop (HW OpenGL/GLX/DRI2 + HW 2D for
offscreen pixmaps via `PVR2DBlt`), with the known limitations documented in
README.md and ENGINEERING.md §9 (on-screen 2D is CPU; compiz does not work). Pass
whatever `VERSION=` you like when building; the examples below use `1.0-rc1` only
as a placeholder string.

```
packaging/
  build-modules.sh   # compile the 3 kernel modules from ../features
  build-debs.sh      # assemble + build the .deb packages from artifacts
  modules/           # dpkg tree for poulsbo-sgx-modules (control, scripts, configs)
  userspace/         # dpkg tree for poulsbo-sgx-userspace (ld/xorg config)
  meta/              # dpkg tree for the poulsbo-sgx metapackage
  glx-extra/         # GLX/DRI2 enablement: glx-enable.sh, xorg.conf.glx, emgd_dri.so
  desktop/           # poulsbo-desktop.service + marco session xinitrc (auto-start, no compiz)
  out/               # built .deb files land here
```

The build splits into two stages: **(1)** build the kernel modules, **(2)**
collect the userspace binaries and run `dpkg-deb`.

---

## 0. Prerequisites

- A Debian/Ubuntu build host with `dpkg-deb` and Docker.
- The i386 kbuild container. Build it once from the repo:

  ```bash
  docker build -t poulsbo-kbuild:18.04 docker/kbuild
  ```

  It provides a 32-bit GCC toolchain matching the target's `gcc`/binutils so the
  module vermagic and CRCs match `5.4.0-150-generic`.

- The **target kernel headers**. The modules are built out-of-tree against
  `linux-headers-5.4.0-150-generic` (i686). Copy that tree (and its
  `-generic` build symlink target) from the target machine, e.g.:

  ```bash
  # on the target (or from an apt cache):
  #   /usr/src/linux-headers-5.4.0-150
  #   /usr/src/linux-headers-5.4.0-150-generic
  # place them under a directory KBUILD_SRC so that
  #   $KBUILD_SRC/usr/src/linux-headers-5.4.0-150-generic  exists
  ```

To target a **different kernel**, set `KERNEL=` / `KDIR=` for both scripts and
supply that kernel's headers. The C sources are kernel-5.4-era; newer kernels
may need small API fixups in `features/pvrsrvkm-port` and `features/gma500-sgx`.

---

## 1. Build the kernel modules

```bash
cd packaging
KBUILD_SRC=/path/to/headers-root ./build-modules.sh
```

This compiles, **in dependency order**:

1. `pvrsrvkm.ko` — from `features/pvrsrvkm-port/src`. Exports the PVR-over-DRM
   bridge and the `gma_pvr_sgx_isr` interrupt forwarder.
2. `gma500_gfx.ko` — from `features/gma500-sgx`, built with
   `KBUILD_EXTRA_SYMBOLS` pointing at pvrsrvkm's `Module.symvers` so the bridge +
   forwarder symbols resolve. This is mainline `gma500` plus the bridge,
   `gma500_flip.c` (the page-flip helper), the SGX interrupt forwarder in
   `psb_irq.c`, and the `pvr_irq_forward` / `flip_vsync` module parameters.
3. `gma500dc.ko` — from `features/gma500dc`, built against the concatenated
   pvrsrvkm + gma500 symbols. The display-class device that backs the PVR2D flip
   chain; carries the `flip_enable` parameter.

`pvrsrvkm.ko` also now carries the two GLX-speed module parameters
`event_timeout_ms` (default 2) and `sgx_apm_latency_ms` (default 500) — see
ENGINEERING.md §4.3 / §5.

The `.ko` files land in `packaging/artifacts/`.

**A fourth module — `proc_dri_compat.ko`** — is also needed (it recreates the
legacy `/proc/dri/0/name` interface the closed `emgd_dri.so` requires for GLX).
It is proc-only and kernel-header-built from `features/glx-dri2` (has its own
`Kbuild`); build it in the same i386 container and drop the resulting
`proc_dri_compat.ko` into `packaging/artifacts/` alongside the other three.
`build-debs.sh` installs it into `poulsbo-sgx-modules`.

> Why Docker + out-of-tree: the target is a slow i686 netbook. Building in a
> matching i386 container on a fast host and copying the `.ko` over is far
> quicker, and the vermagic still matches.

---

## 2. Collect the userspace artifacts

`build-debs.sh` expects these files in one directory (default
`packaging/artifacts/`, override with `ARTIFACTS=`):

| File | Where it comes from |
|---|---|
| `pvrsrvkm.ko`, `gma500_gfx.ko`, `gma500dc.ko` | step 1 above |
| `proc_dri_compat.ko` | step 1 above (`features/glx-dri2`) — the `/proc/dri` compat shim for GLX |
| `srvinit` | the SGX services init helper (i386 ELF) — from the EMGD userspace |
| `gma500blt_drv.so` | build `features/xf86-video-gma500blt` against the Xorg 1.20 (HWE, ABI 24) SDK; the result installs to `/usr/lib/xorg/modules/drivers/`. This DDX now also provides DRI2 (`gma500blt_dri2.c`), which is what enables GLX. |
| `emgd-lib.tgz` | `tar czf emgd-lib.tgz lib` from a provisioned `/opt/emgd` — the EMGD 1.18 (DDK 1.5.15.3226) EGL/GLES/OpenVG runtime; must include `dri/emgd_dri.so` (the desktop-GL DRI2 driver) |

Two more GLX inputs feed `build-debs.sh`. The recovered DDK window WSEGL
`features/emgd-wsegl/orig-ddk/libemgdPVR2D_DRIWSEGL.so.1.5.15.3226` is **committed
in-repo** and used directly. The glapi-patched DRI driver
`packaging/glx-extra/emgd_dri.so` (placed on the Mesa loader path) is a closed
binary and is **gitignored — stage it before building**: copy `emgd_dri.so` from
`/opt/emgd/lib/dri/` and `patchelf --add-needed libglapi.so.0` it into
`packaging/glx-extra/emgd_dri.so`.

The EMGD runtime is the closed Intel/Imagination userspace (see
`firmware/` and `research/external/` in this repo, and ENGINEERING.md for the
provenance). For GLX, `build-debs.sh` installs the **original DDK** window WSEGL
(`features/emgd-wsegl/orig-ddk/…so.1.5.15.3226`) as
`libemgdPVR2D_DRIWSEGL.so.1.5.15.3226` and points the `libemgdPVR2D_DRIWSEGL.so`
symlink at it (dropping any `…so.real` backup). The DDK WSEGL — not the earlier
custom flip stub — is what creates the window render surface the emgd DRI2 path
needs (ENGINEERING.md §4.3).

Building the EXA X driver (`gma500blt_drv.so`) — a plain `make`, no autotools,
built against the installed Xorg SDK (do this on the target or a matching i386
environment so it links the right ABI):

```bash
cd features/xf86-video-gma500blt
# needs: xserver-xorg-dev-hwe-18.04 (ABI 24), pkg-config, libdrm-dev
make                      # -> gma500blt_drv.so
# features/xf86-video-gma500blt/build-test.sh builds + smoke-tests it under X.
```

---

## 3. Build the `.deb` packages

```bash
cd packaging
VERSION=1.0-rc1 ARCH=i386 ARTIFACTS=/path/to/artifacts ./build-debs.sh
```

Produces in `packaging/out/`:

```
poulsbo-sgx-modules_1.0-rc1_i386.deb
poulsbo-sgx-userspace_1.0-rc1_i386.deb
poulsbo-sgx_1.0-rc1_all.deb
```

`VERSION=` / `ARCH=` / `KERNEL=` override the defaults (the script's built-in
default `VERSION` is still `1.0.0`, so pass `VERSION=1.0-rc1` for the release
build). The script stamps the version into each `control` file — including the
meta package's `(= $VERSION)` dependencies — and records `Installed-Size`.

Inspect a result with `dpkg-deb --info` / `dpkg-deb --contents`.

---

## What the packages install (and why it boots accelerated)

**`poulsbo-sgx-modules`**

```
/opt/poulsbo-sgx/{pvrsrvkm,gma500_gfx,gma500dc}.ko   the driver stack
/opt/poulsbo-sgx/proc_dri_compat.ko                  /proc/dri/0/name shim (for GLX)
/opt/poulsbo-sgx/load.sh                             insmods them in order
/opt/poulsbo-sgx/srvinit, srvinit.sh                 SGX INIT_COMPLETE (for GL)
/lib/systemd/system/poulsbo-sgx.service              loads the stack at sysinit
/lib/systemd/system/poulsbo-sgx-srvinit.service      runs srvinit before login
/lib/systemd/system/poulsbo-desktop.service          starts the marco desktop (commit d2dfbe64)
/etc/modprobe.d/poulsbo-sgx.conf                     blacklists stock gma500_gfx
/etc/default/poulsbo-sgx                             module-parameter tunables
```

The `poulsbo-desktop.service` unit (with a marco session xinitrc, sources under
`packaging/desktop/`) brings up the accelerated desktop on boot using **marco**,
the non-compositing window manager — deliberately **without compiz**, which does
not work and can hard-hang the box (README.md *What does not work*; ENGINEERING.md
§9).

`postinst` runs `depmod`, `update-initramfs -u` (so the blacklist takes effect
in early boot), and `systemctl enable` on both units. `load.sh` insmods
`pvrsrvkm` → `gma500_gfx gma_no_sgx=1 …` → `gma500dc` by absolute path (a
blacklist does not block an explicit `insmod`), then insmods
`proc_dri_compat.ko` (proc-only, needed for GLX), passing the parameters from
`/etc/default/poulsbo-sgx`. `srvinit` runs later so it can never wedge early
boot; 2D X acceleration does not need it (but GLX does — it brings PVR services
up).

**`poulsbo-sgx-userspace`**

```
/opt/emgd/lib/*                                      EMGD EGL/GLES2 + dri/emgd_dri.so + DDK WSEGL
/etc/ld.so.conf.d/poulsbo-emgd.conf                  adds /opt/emgd/lib to ld path
/usr/lib/xorg/modules/drivers/gma500blt_drv.so       the EXA 2D + DRI2 X driver
/usr/share/X11/xorg.conf.d/20-poulsbo-gma500.conf    selects gma500blt for the GPU
/opt/poulsbo-sgx-glx/{glx-enable.sh,emgd_dri.so,xorg.conf.glx}   GLX payload
```

`postinst` runs `ldconfig`, forces the `libemgdPVR2D_DRIWSEGL.so` symlink to the
DDK WSEGL, then runs **`/opt/poulsbo-sgx-glx/glx-enable.sh`** to finish GLX/DRI2
enablement (the parts that live outside `/opt/emgd`, on the system Mesa paths).
`glx-enable.sh` (idempotent, re-runnable):

1. installs the glapi-patched `emgd_dri.so` to
   `/usr/lib/i386-linux-gnu/dri/emgd_dri.so` (the Mesa DRI loader path);
2. adds `MESA_LOADER_DRIVER_OVERRIDE=emgd` to `/etc/environment` (otherwise the
   PCI-id→driver map picks the kernel `gma500` name and falls to llvmpipe);
3. binary-patches `libGLX_mesa.so.0` to neutralise an unguarded
   `DRI2_Config_Query` call that NULL-derefs with the emgd driver (keeps a
   `.orig` backup; skipped if already patched);
4. installs `xorg.conf.glx` (gma500blt + AIGLX + glx/dri2) to `/etc/X11/`, making
   it the default `xorg.conf` if none exists.

On the next X start the server loads `gma500blt`, initialises EXA + DRI2,
**offscreen 2D is accelerated via `PVR2DBlt`** (on-screen/scanout 2D falls back to
CPU — ENGINEERING.md §9), and desktop **OpenGL (GLX/DRI2) runs on the SGX535** —
`glxinfo` shows direct rendering on *EMGD on PowerVR SGX535* (OpenGL 2.1). EGL/GLES2
apps get the SGX535 via the EMGD runtime as before.

### Tunable kernel parameters

Load-time module parameters are set in **`/etc/default/poulsbo-sgx`** (read by
`load.sh`) — see README.md. The two GLX-speed `pvrsrvkm` knobs already default to
their fast values and are additionally **runtime-tunable** via sysfs (`0644`), no
reload needed:

```bash
cat  /sys/module/pvrsrvkm/parameters/event_timeout_ms     # default 2  (was hard 100 ms)
cat  /sys/module/pvrsrvkm/parameters/sgx_apm_latency_ms    # default 500 (Poulsbo default was 1 ms)
echo 500 | sudo tee /sys/module/pvrsrvkm/parameters/sgx_apm_latency_ms
```

See ENGINEERING.md §4.3 for what these fix (they lift the GLX frame rate; measured
glxgears on the current Path B desktop is ~66 FPS).

---

## Notes

- Prebuilt binary artifacts (`.ko`, the EMGD `.so`s including
  `glx-extra/emgd_dri.so`) are intentionally **not** committed to git; the
  scripts stage them at build time. The C sources that produce the `.ko` are
  under `features/` (including `features/glx-dri2/proc_dri_compat.c` and the DDX
  DRI2 `gma500blt_dri2.c`). The recovered DDK WSEGL under
  `features/emgd-wsegl/orig-ddk/` is the one binary that *is* committed.
- Kernel-version coupling lives in `poulsbo-sgx-modules`
  (`TARGET_KERNEL` in its `postinst`, and the compiled vermagic) — the four
  `.ko`s, `proc_dri_compat.ko` included. The userspace package (EMGD runtime,
  DDX, GLX enablement) is kernel-independent.
- For the internals — the interrupt forwarding, the flip chain, the SGX
  microkernel bring-up — see [ENGINEERING.md](ENGINEERING.md).
