# Building the Poulsbo SGX packages yourself

This describes how to rebuild the three `.deb` packages from source — both the
kernel modules and the package assembly. Everything lives under `packaging/`,
with the module sources under `features/`.

```
packaging/
  build-modules.sh   # compile the 3 kernel modules from ../features
  build-debs.sh      # assemble + build the .deb packages from artifacts
  modules/           # dpkg tree for poulsbo-sgx-modules (control, scripts, configs)
  userspace/         # dpkg tree for poulsbo-sgx-userspace (ld/xorg config)
  meta/              # dpkg tree for the poulsbo-sgx metapackage
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

The `.ko` files land in `packaging/artifacts/`.

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
| `srvinit` | the SGX services init helper (i386 ELF) — from the EMGD userspace |
| `gma500blt_drv.so` | build `features/xf86-video-gma500blt` against the Xorg 1.20 (HWE, ABI 24) SDK; the result installs to `/usr/lib/xorg/modules/drivers/` |
| `emgd-lib.tgz` | `tar czf emgd-lib.tgz lib` from a provisioned `/opt/emgd` — the EMGD 1.18 (DDK 1.5.15.3226) EGL/GLES/OpenVG runtime plus the flip WSEGL |

The EMGD runtime is the closed Intel/Imagination userspace (see
`firmware/` and `research/external/` in this repo, and ENGINEERING.md for the
provenance). `build-debs.sh` repoints the `libemgdPVR2D_DRIWSEGL.so` symlink at
the **zero-copy flip** WSEGL (`…so.1.5.15.3226`), not the stock DRI2 build
(`…so.real`), so window presents page-flip.

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
ARTIFACTS=/path/to/artifacts ./build-debs.sh
```

Produces in `packaging/out/`:

```
poulsbo-sgx-modules_1.0.0_i386.deb
poulsbo-sgx-userspace_1.0.0_i386.deb
poulsbo-sgx_1.0.0_all.deb
```

Override `VERSION=`, `ARCH=`, `KERNEL=` as needed. The script stamps the version
into each `control` file and records `Installed-Size`.

Inspect a result with `dpkg-deb --info` / `dpkg-deb --contents`.

---

## What the packages install (and why it boots accelerated)

**`poulsbo-sgx-modules`**

```
/opt/poulsbo-sgx/{pvrsrvkm,gma500_gfx,gma500dc}.ko   the driver stack
/opt/poulsbo-sgx/load.sh                             insmods them in order
/opt/poulsbo-sgx/srvinit, srvinit.sh                 SGX INIT_COMPLETE (for GL)
/lib/systemd/system/poulsbo-sgx.service              loads the stack at sysinit
/lib/systemd/system/poulsbo-sgx-srvinit.service      runs srvinit before login
/etc/modprobe.d/poulsbo-sgx.conf                     blacklists stock gma500_gfx
/etc/default/poulsbo-sgx                             module-parameter tunables
```

`postinst` runs `depmod`, `update-initramfs -u` (so the blacklist takes effect
in early boot), and `systemctl enable` on both units. `load.sh` insmods
`pvrsrvkm` → `gma500_gfx gma_no_sgx=1 …` → `gma500dc` by absolute path (a
blacklist does not block an explicit `insmod`), passing the parameters from
`/etc/default/poulsbo-sgx`. `srvinit` runs later so it can never wedge early
boot; 2D X acceleration does not need it.

**`poulsbo-sgx-userspace`**

```
/opt/emgd/lib/*                                      EMGD EGL/GLES2 + flip WSEGL
/etc/ld.so.conf.d/poulsbo-emgd.conf                  adds /opt/emgd/lib to ld path
/usr/lib/xorg/modules/drivers/gma500blt_drv.so       the EXA 2D X driver
/usr/share/X11/xorg.conf.d/20-poulsbo-gma500.conf    selects gma500blt for the GPU
```

`postinst` runs `ldconfig`. On the next X start the server loads `gma500blt`,
initialises EXA, and 2D is accelerated; EGL/GLES2 apps get the SGX535 via the
EMGD runtime.

---

## Notes

- Prebuilt binary artifacts (`.ko`, the EMGD `.so`s) are intentionally **not**
  committed to git; the scripts stage them at build time. The C sources that
  produce the `.ko` are under `features/`.
- Kernel-version coupling lives only in `poulsbo-sgx-modules`
  (`TARGET_KERNEL` in its `postinst`, and the compiled vermagic). The userspace
  package is kernel-independent.
- For the internals — the interrupt forwarding, the flip chain, the SGX
  microkernel bring-up — see [ENGINEERING.md](ENGINEERING.md).
