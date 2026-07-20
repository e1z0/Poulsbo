# Poulsbo — GMA500 / PowerVR SGX535 open acceleration stack

Engineering reference for the reverse-engineered, hardware-verified graphics
acceleration driver for the Intel GMA500 (Poulsbo US15W, PowerVR SGX535,
PCI `8086:8108`) on a Sony Vaio running Ubuntu 18.04 LTS, kernel
`5.4.0-150-generic` (i686 / i386).

> **This document describes a driver that exists and runs.** Earlier revisions of
> this file framed the project as "planning a driver that does not exist yet."
> That is no longer true. On the target hardware today (the "Path B"
> configuration, commit `fc6a520f`, HW verified 2026-07-20): an open EXA X11
> driver modesets and accelerates offscreen 2D via the SGX's own `PVR2DBlt`, a
> stable **MATE (marco)** desktop displays, **hardware-accelerated desktop
> OpenGL / GLX/DRI2 works and displays** (`glxinfo`: direct rendering Yes, *EMGD
> on PowerVR SGX535*, OpenGL 2.1; `glxgears` renders and displays at ~66 FPS
> measured), the SGX535's proprietary microkernel firmware boots to
> `INIT_COMPLETE` from an open hand-written loader, and the closed
> Imagination/EMGD GLES2 userspace renders true programmable-shader 3D on top of
> an open kernel PVR-over-DRM bridge.
>
> **Read [§0](#0-the-core-finding-the-2d-blt--sgx-bif-conflict) first.** The
> direct-MMIO 2D-BLT engine and the closed SGX microkernel **cannot both drive
> the graphics core** — this is the central architectural constraint, and the
> reason earlier "composited accelerated desktop" claims never reached the panel.
> Corrected honest limitations are in [§9](#9-honest-limitations-and-future-work);
> compiz does **not** work and can hard-hang the box.

All results below were measured on the target host `root@10.1.11.218` (the
"Vaio") unless noted. Dated milestones come from the project memory log; the
key source of truth is the 3D bring-up history summarized in [§8](#8-sgx-3d-bring-up-history-the-open-microkernel-track).

---

## 0. The core finding: the 2D-BLT ↔ SGX BIF conflict

**The open, direct-MMIO 2D-BLT engine and the closed SGX microkernel (pvrsrvkm,
required for OpenGL/GLX/DRI2) cannot both be active on the shared graphics core
as currently built.** Proven on hardware 2026-07-20.

**Evidence.** The standalone BLT test (`features/blt-ioctl/user/psb_blt_test.c`)
passes **12/12 with SGX uninitialised** (srvinit service masked) and fails
**5/12 the moment SGX is initialised** (partial fill, engine hang). It is not
X-specific — `SGXInitialise`/`SGXReset`, running at boot before X, is what breaks
2D.

**Mechanism.** `SGXInitialise` reroutes the 2D requestor through the SGX MMU.
`BIF_BANK0` (`0x0C78`) packs 4-bit per-requestor dir-list indices; `SGXReset`
sets `INDEX_2D` (bits `[19:16]`) to `0xF`, i.e. route the 2D engine through the
SGX page-directory model instead of the GART. Observed register diff:

| reg | SGX off (2D works) | SGX on (2D wedges) |
|---|---|---|
| `BIF_BANK0` (`0x0C78`) | `0x000000` | `0x0f00ff` |
| `BIF_TWOD_REQ_BASE` (`0x0C88`) | `0x80000000` (=`gatt_start`) | `0x10000000` |
| `BIF_DIR_LIST_BASE0` (`0x0C84`) | `0x34448000` (gma500 PD) | `0x7f04a000` (SGX's) |

Once SGX is up, 2D requests are *translated* through an MMU page directory, and
gma500's simple-model PD does not map the GTT aperture `[gatt_start, …)` the 2D
engine addresses. Multiple coexistence attempts (re-asserting `TWOD_REQ_BASE`,
clearing only `INDEX_2D`, parking gma500's PD in a free dir-list slot with
`INDEX_2D=1`) all still failed 5/12 on hardware; one per-blit IRQs-off drain-spin
**hard-hung the box**. A real fix needs a GTT-mirroring SGX-model page directory
for 2D, or routing all open-2D buffers through the SGX MMU — neither is done.

**Consequence — what this invalidates.** Earlier notes that claimed a
"HW-accelerated composited desktop working" were **process/API-level only** (glxinfo
strings, running processes), never confirmed pixels on the panel. The first real
visual test (2026-07-19/20) showed black/blue/fragment screens, not a usable
desktop, precisely because the direct 2D engine wedged the moment SGX inited.

**Path B — the working resolution (commit `fc6a520f`).** Abandon the direct-MMIO
2D-BLT engine under X. Instead the DDX allocates offscreen EXA pixmaps with
`PVR2DMemAlloc` (SGX-mapped) and fills/copies them with **`PVR2DBlt`** — the SGX
microkernel's *own* 2D path, which coexists with GL by design. Verified on
hardware: X up, **blitter hangs 0, SGX faults 0**, GL works, marco desktop
displays. On-screen 2D is accelerated too (the scanout is `PVR2DMemWrap`ped so
`PVR2DBlt` draws to the screen — window dragging is a HW blit). The compiz
danger is in [§9](#9-honest-limitations-and-future-work).

---

## 1. The three layers and current status

The stack is organized as three independently useful layers on top of the
mainline `gma500_gfx` DRM driver:

| Layer | What | Mechanism | Status |
|---|---|---|---|
| **A** | 2D acceleration on the fbcon | `GMA_BLT_SUBMIT` DRM ioctl → dedicated SGX 2D BLIT core (slave port `SGX+0x4000`) | **12/12 HW tests — but only with SGX uninitialised.** Real GPU pixels, verified via aperture + `/dev/fb0` readback. **Wedges once SGX/GL is up (see [§0](#0-the-core-finding-the-2d-blt--sgx-bif-conflict)); not usable alongside OpenGL.** |
| **B** | Open X11 2D EXA DDX | `xf86-video-gma500blt`: KMS modeset (LVDS-1 @ 1600x768) + EXA Solid/Copy | **Working + stable X (Path B).** HW 2D via `PVR2DBlt` for **offscreen pixmaps** (`PVR2DMemAlloc`) **and the on-screen scanout** (`PVR2DMemWrap`) — coexists with GL. Xorg 1.20 (HWE ABI 24.1). |
| **C** | 3D (GLES2 / EGL) | Closed EMGD/Imagination userspace (DDK 1.5.15.3226) on ported `pvrsrvkm`, coexisting with `gma500` via an open PVR-over-DRM bridge | **Working: SGX535 renders programmable-shader GLES2,** interrupt-driven page-flip present. (The high offscreen-bench FPS figures below are register/API-verified, not on-panel measurements.) |
| **C2** | Desktop OpenGL (GLX / DRI2) on X | Closed EMGD `emgd_dri.so` DRI2 driver bridged into Mesa-20's GLX, on the open `gma500blt` DDX (which is itself a PVR2D client providing the DRI2 buffers) | **Working and displaying: `glxinfo` = direct rendering Yes / EMGD on PowerVR SGX535 / OpenGL 2.1; `glxgears` renders and displays at ~66 FPS (measured). Reboot-persistent, on a marco (non-compositing) desktop. Compiz does NOT work (§9).** See [§4.3](#43-glxdri2-desktop-opengl-the-buffer-bridge-and-performance). |

A parallel, fully-open **microkernel bring-up track** (`sgx_3d.c`, no closed
userspace) independently reached `INIT_COMPLETE` and host↔microkernel CCB
command transport on real silicon. It is a reverse-engineering achievement and
diagnostic asset, but open microkernel *rendering* is materials-gated — see
[§9 Honest limitations](#9-honest-limitations-and-future-work).

### What "works" means for Layer C, precisely

- The SGX535 microkernel is uploaded, reset, kicked and reaches `INIT_COMPLETE`;
  services connect; `GETMISCINFO` returns the true `CoreRev=0x10201`.
- An **unmodified** GLES2 application — `eglCreateWindowSurface` +
  `glDrawArrays` + `eglSwapBuffers`, no app changes — renders on the SGX535's
  USSE (vertex + fragment shaders, rasterization, varying interpolation all run
  on the GPU) and is presented into a visible X11 window.
- Present is a **hardware page-flip** into GTT-pinned scanout buffers, with **no
  CPU copy**, dispatched by the SGX render-complete **interrupt**.

---

## 2. Hardware / target facts

Captured read-only from the Vaio (BAR0 `ioremap` + `ioread32`, gma500 left
bound). These are ground truth for all register work.

| Fact | Value | Source |
|---|---|---|
| PCI device | `8086:8108` at `0000:00:02.0` | lspci |
| BAR0 physical base | **`0x94200000`**, length `0x80000` (512 KiB) | M0 probe |
| SGX CR register block | **`0x94240000`** = BAR0 + `PSB_SGX_OFFSET` (`0x40000`) | M0 probe |
| SGX 2D slave port | `SGX + 0x4000` (`PSB_SGX_SIZE` = `0x8000`) | M1/M2 |
| `CORE_ID` (`0x10`) | `0x01130000` → **SGX535 confirmed** | M0 |
| `CORE_REVISION` (`0x14`) | `0x00010201` → **rev 1.2.1** | M0 |
| `CLKGATECTL` (`0x00`) | `0x01111111` | M0 |
| DDK / firmware | Imagination DDK **1.5.15.3226**, EMGD **1.18**, `SGX_CORE_REV=121` (= rev 1.2.1, exact match) | libemgdsrv_init.so |
| Build target string | `pc_i686_poulsbo_d0_linux_release` | libemgdsrv_init.so |
| Kernel | `5.4.0-150-generic`, i686, vermagic `5.4.0-150-generic SMP mod_unload modversions 686` | target |

### Display pipe: the LVDS panel is on **pipe B / plane B**

This is a load-bearing correctness fact. On this Vaio the LVDS panel is **not**
on pipe A:

| Register | Offset | Live value | Meaning |
|---|---|---|---|
| `PIPEACONF` | `0x70008` | `0x0` | pipe A **disabled** |
| `DSPACNTR` | `0x70180` | `0x0` | plane A off |
| `DSPABASE` | `0x70184` | — | (the register a naive flip would write — invisible) |
| `PIPEBCONF` | `0x71008` | `0x80000000` | pipe B **enabled** |
| `DSPBCNTR` | `0x71180` | `0xd9000000` | plane B on |
| `DSPBBASE` | `0x71184` | `0x8000` → `0x7bf000` on flip | the live fbcon scanout base |

Any scanout page-flip **must** target the active pipe. `gma500_flip_active_pipe()`
scans `psb_regmap[0..num_pipe-1]` for `PIPExCONF & PIPEACONF_ENABLE` and flips
that pipe's `regmap[].base`. To *prove* a present is visible you must read the
active scanout base register (`DSPBBASE`) or the panel — not just the CPU-side
buffer contents.

---

## 3. Architecture

### 3.1 gma500 (display) + pvrsrvkm (render) coexistence

One PCI device can be bound by only one DRM driver, and mainline `gma500_gfx`
already binds `8086:8108` for display/KMS. Therefore:

- **`gma500_gfx`** owns the PCI device, display, KMS, GTT/MMU, and the 2D BLIT
  path (Layers A/B). It also hosts the PVR-over-DRM bridge ioctls and the SGX
  IRQ forward (below).
- **`pvrsrvkm`** (the ported 2013 EMGD/DDK SGX render module) does **not** bind
  the PCI device. It acquires the SGX via the non-binding `pci_get_device`
  (`OSPCIAcquireDev`) + `ioremap` of the known SGX register block, and runs its
  init path directly from `PVRCore_Init` (guarded on `gpsPVRLDMDev == NULL`)
  rather than via an LDM `.probe` that will never fire.
- Load order matters: **`pvrsrvkm` first, then `gma500_gfx`** (gma500 links
  pvrsrvkm's exported bridge symbols), then `gma500dc`.

Historical gotchas that were resolved during the port (documented so they are
not re-hit):

- `pvrsrvkm`'s `pci_register_driver` was **removed** — in the coexistence build
  it must never bind `8086:8108`, or gma500 loses the device and there is no
  `/dev/dri/card0`.
- The "nine freezes" chasing `SGXInit` hangs were ultimately a **NULL
  `psPCIDev` deref** (release-build `PVR_ASSERT` is a no-op), turned into a
  panic+auto-reboot by `panic_on_oops=1`. Root-caused in one observable load via
  **netconsole** `KERN_EMERG` markers after nine blind power-cycles. Lesson: set
  `panic_on_oops=0` and use netconsole for any bring-up load.

### 3.2 The PVR-over-DRM bridge (kernel)

The closed EMGD userspace connects to the kernel over **DRM**, not a char
device. It opens the DRM node **by PCI busid** (`drmOpenByBusid("PCI:00:02:00")`
= `0000:00:02.0`), so it opens whatever driver is bound there — i.e. gma500's
`card0`. No `emgd`-named node is required. `/dev/pvrsrvkm` exists but userspace
never uses it.

gma500 therefore **hosts the PVR bridge command ioctls** (offsets from
`DRM_COMMAND_BASE` = `0x40`):

| ioctl | Offset | Encoding | Forwards to |
|---|---|---|---|
| `GMA_PVR_SRVKM` | `+0x12` | `DRM_IOW(BASE+0x12, PVRSRV_BRIDGE_PACKAGE)` (24 B on 32-bit) | `gma_pvr_srvkm_dispatch` → `BridgedDispatchKM` |
| `GMA_PVR_ISMASTER` | `+0x15` | `DRM_IO(BASE+0x15)`, `DRM_MASTER` | `gma_pvr_is_master` |
| `GMA_PVR_UNPRIV` | `+0x16` | `DRM_IOWR(BASE+0x16, unsigned long)` | stub |

Plus `.open = gma_pvr_srvkm_open` (`PVRSRVProcessConnect(PID)`) and
`.postclose = gma_pvr_srvkm_release`. The bridge is a standalone export shim in
pvrsrvkm (`pvr_gma_bridge.c`) keyed by **PID**, so it never touches
`drm_file->driver_priv` (gma500's). `SUPPORT_DRI_DRM` is *not* in the
`sgx_options` ABI bitmask, so the bridge **package** format is identical to the
char-device build — only the transport differs, which keeps the ABI compatible
with the closed userspace's `SGXDevInitCompatCheck` OPTIONS check. The
`GMA_PVR_SRVKM` entry uses `DRM_RENDER_ALLOW` so an unauthenticated offscreen
client works without an X master.

**The mmap path is bridged too**, and this was the missing piece that unblocked
the whole 3D bring-up. `MHANDLE_TO_MMAP_DATA` returns a linear offset token
meaningful only to pvrsrvkm's mmap, but userspace holds gma500's `card0` fd. So
gma500's `.fops.mmap` (`psb_pvr_gem_mmap`) first calls the exported
`gma_pvr_try_mmap()` (pvrsrvkm `mmap.c`: `FindOffsetStructByOffset` →
`PVRMMap`), falling back to `drm_gem_mmap`. Only after this did `DEVINITPART2` /
`SGXInitialise` (the real SGX bring-up) run under the closed userspace's
`srvinit`.

### 3.3 gma500dc — the PVR display-class driver + flip chain

`gma500dc.ko` (~460 lines) is a minimal PVR display-class driver. It fills a
`PVRSRV_DC_SRV2DISP_KMJTABLE` (Open/Close, `EnumFormats[ARGB8888]`,
`EnumDims[1600x768]`, `GetSystemBuffer`, `GetInfo`, per-page `GetBufferAddr`)
and calls `pfnPVRSRVRegisterDCDevice`. It is what makes
`PVR2DEnumerateDevices` return non-zero, which the closed userspace's PVR2D
requires (PVR2D enumerates **display-class** devices).

The DC driver also owns a **swap chain** of GTT-pinned scanout buffers and a
`DC_FLIP_COMMAND` processor (`gma500dc_process_flip`). Those swap-chain buffers
are the flip-chain buffers the WSEGL window path renders into, and whose pages
the SGX MMU also maps into the GL context — this shared mapping is what makes
zero-copy present possible. Built with
`KBUILD_EXTRA_SYMBOLS=pvrsrvkm.symvers`; loads after pvrsrvkm with no reboot.

### 3.4 WSEGL — the window-system glue

The closed `libEMGDegl` `dlopen`s a single WSEGL entry
(`WSEGL_GetFunctionTablePointer`) that returns a v1 function table. The stock
EMGD WSEGL (`libemgdPVR2D_DRIWSEGL`) is DRI2-based and needs a live emgd X
server (Xorg ~1.9, incompatible with the Vaio's Xorg 1.20 and no source
available). Two open WSEGL replacements were written to route around that wall
(ABI reconstructed by disassembling the closed lib's function table at
`0x5828-0x5854`):

- **`wsegl_null.c`** — pbuffer-only. `InitialiseDisplay` returns caps + one
  8888 config, window/pixmap stubbed, **no DRI2**. Lets `eglInitialize` succeed
  for offscreen `glReadPixels` rendering with no DDX.
- **`wsegl_win.c`** — window drawables. `CreateWindowDrawable` allocates an SGX
  render buffer via PVR2D; `GetDrawableParameters` hands EGL the device+CPU
  address and **`ui32Stride` in PIXELS** (the critical fix — stride in *bytes*
  made the GL use its own 8× stride so only 1/8 of rows landed; pixels gave a
  correct frame). `SwapDrawable` presents. Installed on the Vaio as the active
  `/opt/emgd/lib/libemgdPVR2D_DRIWSEGL.so.1.5.15.3226`.

### 3.5 Layer B EXA DDX

`xf86-video-gma500blt` is an open Xorg driver: `gma500blt_exa.c` builds
`PSB_2D` fill/copy streams (0-filled address slots + out-of-band relocs) and
submits via `DRM_IOCTL_PSB_BLT_SUBMIT` — the same stream format proven 12/12 in
Layer A. Only `GXcopy` + full planemask + supported formats accelerate; else
EXA software fallback, no Composite. `gma500blt_driver.c` does the KMS/RandR
glue (GEM dumb pixmaps, drmmode outputs). Two on-target segfault fixes were
needed: `gma_pixmap_is_offscreen` must return FALSE for the root pixmap, and
`ScreenInit` must set `pScreen->SaveScreen = xf86SaveScreen`. Builds against
`xserver-xorg-dev-hwe-18.04` (ABI 24.1; the base `xserver-xorg-dev` is ABI 23
and is rejected: "ABI major 23 doesn't match server 24"). Verified: Xorg starts
on `gma500blt`, drmmode reads real KMS outputs (LVDS-1 @ 1600x768, plus
SVIDEO/VGA), `xdpyinfo` 1600x768 depth 24, `x11perf -rect100` runs clean and
stable. On-screen root accel was investigated and **deliberately left
software**: a synchronous pin→submit→wait_idle→unpin BLT is ~330 ms/op vs ~1230
rect/s in software, so software root is optimal until an async/fenced submit
exists.

**Path B correction (commit `fc6a520f`).** The direct `PSB_2D`/`DRM_IOCTL_PSB_BLT_SUBMIT`
stream above is the *original* DDX path — and it is **abandoned under X**, because
the direct 2D engine wedges the moment SGX inits ([§0](#0-the-core-finding-the-2d-blt--sgx-bif-conflict)).
The shipped DDX instead allocates offscreen EXA pixmaps with `PVR2DMemAlloc`
(new `pvr2d.h`; `priv->pvr2d_mem` in `gma_create_pixmap`/`gma_destroy_pixmap`) and
implements `gma_Solid`/`gma_Copy` with **`PVR2DBlt`** (fill ROP `0xF0` / copy
`0xCC`, CPU fallback), syncing via `PVR2DQueryBlitsComplete` in
`gma_prepare_access`. This is the SGX microkernel's own 2D path, which coexists
with GL. Gotcha fixed: `gma_pixmap_is_offscreen` must return **TRUE** for
`pvr2d_mem` pixmaps (handle `== 0`) or EXA NULL-derefs in `fbBlt`. On-screen
copies still fall to CPU (the KMS front buffer is not SGX-mapped — §9).

---

## 4. Interrupt-driven, zero-copy page-flip present

This is the headline optimization of Layer C, and both halves are on-hardware
verified.

### 4.1 Zero-copy flip

`wsegl_win.c` `SwapDrawable` uses a **PVR2D flip chain**
(`PVR2DCreateFlipChain` / `GetFlipChainBuffers` / `PresentFlip`) whose buffers
are gma500dc DC-swap-chain buffers = **GTT-pinned scanout buffers** whose pages
the SGX MMU also maps into the GL context. The GL renders straight into the
current back buffer (EGL is handed that buffer's device+CPU address, stride in
pixels), and `eglSwapBuffers` becomes:

```
eglSwapBuffers → PVR2DPresentFlip → DC_FLIP_COMMAND
              → gma500dc_process_flip → gma500_flip_to(gtt_offset)
              → write DSPxBASE of the ACTIVE pipe (pipe B → DSPBBASE 0x71184)
```

No CPU copy, no `XPutImage`. HW-verified: the flip buffer center pixel holds the
rendered triangle color, `PresentFlip=0`, double-buffered, and `DSPBBASE` moves
`0x8000 → 0x7bf000` (the flip buffer's GTT offset) on present — i.e. the flip
drives the live scanout. New sources: `gma500_flip.{c,h}` (exports
`gma500_flip_to` / `alloc` / `free` / `get_drm`), the gma500dc swap-chain +
`DC_FLIP` processor, and the WSEGL flip path.

### 4.2 Interrupt forwarding — the 130× fix

Before the fix, flip present was **251 ms/frame (≈4 FPS)**.

**Root cause:** gma500 owns the shared IRQ (18), and its `psb_sgx_interrupt()`
reads and **clears** the SGX EUR event (`PSB_CR_EVENT_HOST_CLEAR`). Running
first in the shared-IRQ chain, it starved pvrsrvkm's own LISR — `SGX_ISRHandler`
always found the event already cleared, never scheduled its MISR, so GL
command-complete (which advances the DC-flip sync objects) only happened on the
~250 ms PVRSRV event-object **poll** timeout.

**Fix:** pvrsrvkm exports `gma_pvr_sgx_isr()` (runs all device ISRs +
`OSScheduleMISR`, gated by `g_bGmaPvrISRReady` set at `SysFinalise` / cleared at
`SysDeinitialise`). gma500's `psb_irq.c` calls it (weak symbol) at the **top**
of `psb_sgx_interrupt`, **before** clearing the event. Now the SGX
render-complete IRQ reaches pvrsrvkm's MISR → `ProcessQueues` advances sync →
the flip dispatches immediately and signals the global event object that wakes
the `PVR2DPresentFlip` waiter.

**Measured result:** swap `251 ms → 1.0 ms`; profiler `render = 0.1 ms`,
`swap = 1.0 ms`; **≈950 FPS sustained over 1500 frames, zero MMU faults / HW
recovery / oops, display healthy.** This is a general fix — the whole GL stack
is now interrupt-driven instead of poll-bound.

### 4.3 GLX/DRI2 desktop OpenGL: the buffer bridge and performance

Layer C above is **GLES2/EGL** — an app links `libEMGDegl` and presents through
our WSEGL flip chain. Layer **C2** is **desktop OpenGL under X via GLX/DRI2**, so
that *ordinary* GL applications (glxgears, GL compositors, generic `libGL`
programs) run hardware-accelerated with no app changes. `glxinfo` reports
**direct rendering: Yes**, **OpenGL renderer: EMGD on PowerVR SGX535**, **OpenGL
version: 2.1**.

The pieces:

- The closed EMGD `emgd_dri.so` (`/opt/emgd/lib/dri/`) is a **DRI2 desktop-GL
  driver** (`libEMGDOGL`) that renders into PVR2D flip chains on our gma500dc
  display-class device via the card0 PVR-over-DRM bridge. It is bridged into
  **Mesa 20's** GLX (Xorg 1.20): Mesa still keeps the DRI2-v2 GLX fallback path,
  and `emgd_dri.so` exports `__driDriverExtensions = { DRI_Core, DRI_DRI2 }`.
- The **open `gma500blt` DDX provides DRI2** (`gma500blt_dri2.c`:
  `DRI2ScreenInit(driverName="emgd", …)`, `CreateBuffer` / `DestroyBuffer` /
  `CopyRegion`). Xorg then loads `emgd_dri.so` as `<driverName>_dri.so`, and
  AIGLX logs *"Loaded and initialized emgd" / "GLX: Initialized DRI2 GL provider
  for screen 0."*

**The buffer bridge (the crux).** gma500 and pvrsrvkm are a **split kernel**:
gma500 owns GEM/GTT memory, pvrsrvkm owns SGX/PVR memory, and neither manages the
other's. So a plain gma500 GEM-flink name handed to `emgd_dri.so` cannot be
mapped by it (`PVR2DMemMap(name)` returned `-11 MAPPING_FAILED`). The fix makes
the **DDX itself a PVR2D client**: `CreateBuffer` does `PVR2DMemAlloc +
PVR2DMemExport` and returns the **export handle** as the DRI2 buffer name.
`emgd_dri.so`'s `PVR2DMemMap(name)` then maps the *same SGX allocation* and
returns 0 — the buffer is simultaneously SGX-renderable and (via gma500) present.

**emgd's private descriptor.** Beyond mapping, `emgd_dri.so` reads a descriptor
out of each buffer's **first 20 bytes** (its geometry-query `0x1c3b0`
dereferences the mapped buffer's `pBase`). So `CreateBuffer` writes, into each
buffer, `uint32 d[] = { type=1, export-handle ×3, count=1 }` — **after**
`priv->name = hexport` is assigned (writing it before left the handles zero and
emgd's map loop `0x1c480` skipped the buffers). emgd then `PVR2DMemMap`s those
handles into the drawable's buffer-object array (`drawable+0x24[]`), which
`KEGLGetDrawableParameters` reads. Attachment **`0x81` (= `back | 0x80`)** is
returned as emgd's metadata type **`0x90`** with `name & 3 == 0`.

**Two userspace pieces the DDX can't substitute for:**

- The **original DDK** `libemgdPVR2D_DRIWSEGL.so.1.5.15.3226` (recovered from the
  in-repo RPM into `features/emgd-wsegl/orig-ddk/`, md5 `61dc7af8`). The custom
  flip stub used by the GLES2 window path (`wsegl_win.c`) does **not** create a
  window render surface for the DRI2 path; the DDK WSEGL does. This was proven by
  A/B: with the stub, `glXMakeCurrent` crashed in `KEGLGetDrawableParameters+446`
  (the context's render surface `context->0x34/0x3c` never bound); with the DDK
  WSEGL it completes.
- **`proc_dri_compat.ko`** (`features/glx-dri2/proc_dri_compat.c`) — a proc-only
  module (no device bind, so it insmods live) that recreates the legacy
  `/proc/dri/0/name` interface removed from modern kernels. `emgd_dri.so`
  enumerates/identifies its device through that path; without it PVR services
  init fails and Mesa falls back to swrast. Param `name="emgd 0000:00:02.0"`.

**Three Mesa-side tweaks** (applied by `glx-enable.sh` at package install, §
PACKAGING.md):

1. `patchelf --add-needed libglapi.so.0` on the DRI-path copy of `emgd_dri.so`
   (it references `_glapi_Dispatch` but doesn't list glapi as NEEDED).
2. `MESA_LOADER_DRIVER_OVERRIDE=emgd` — the PCI-id→driver map has no Poulsbo
   entry, so the loader otherwise picks the kernel `gma500` name and falls to
   llvmpipe.
3. A **binary patch of `libGLX_mesa.so.0`**: Mesa 20 calls an optional
   `DRI2_Config_Query` screen hook unconditionally (`emgd` doesn't provide it),
   NULL-derefing at `dri2CreateScreen`. The two unguarded `call *0x8(%eax)`
   (`ff 50 08`) inside `ff 75 30 ff 50 08 83 c4 10` are neutralised to
   `xor %eax,%eax; nop` (`31 c0 90`); a `.orig` backup is kept.

**What actually displays.** The GL that reaches the window does so because
`gma_dri2_copy_region` was rewritten to **CPU-copy the client back-buffer straight
into the window's region of the KMS scanout** (`b->drmmode.front.cpu_ptr` + pitch,
clipped). The earlier code blitted back to an off-screen PVR2D "front", so the
window stayed black — that is one reason prior "GLX working" reports never
displayed. `glxgears` now renders **and displays** at **~66 FPS** (measured on
hardware).

**Performance tuning.** The first working GLX render was correct but slow.
`strace -T` showed one `nanosleep(~160 ms)` per frame: the EMGD GL client waits
its full event-object timeout each frame because a 3D render raises no host
`SW_EVENT`, so the MISR never signals that particular event object (this is a
*different* event than the flip fix of §4.2 woke). Three stacked fixes lifted the
rate substantially (the specific per-step FPS figures — 5→10→17→105 — come from
that earlier tuning session and predate the on-panel visual verification; treat
them as relative deltas, and ~66 FPS as the measured glxgears figure on the
current Path B desktop):

1. **`pvrsrvkm.event_timeout_ms`** (new param, default **2**, was a hard 100 ms)
   — shortens that per-frame event re-check interval. 5 → 10 FPS.
2. **`pvrsrvkm.sgx_apm_latency_ms`** (new param, default **500**; the Poulsbo
   `SYS_SGX_ACTIVE_POWER_LATENCY_MS_PLB` default was **1 ms**) — the SGX powered
   down after 1 ms idle and paid a power-up penalty every frame; keeping it awake
   gave 10 → 17 FPS. `0` disables active PM. (`sysconfig.c` sets
   `bEnableActivePM = (sgx_apm_latency_ms != 0)`.)
3. **A DDX buffer cache** (`gma500blt_dri2.c`) — DRI2 recreates buffers on every
   swap; the cache reuses freed PVR2D allocations keyed by `{attachment,w,h}` and
   just rewrites the descriptor per `CreateBuffer` (emgd zeroes the buffer). The
   X-protocol / alloc churn disappears: 17 → **105 FPS**.

Both `pvrsrvkm` params are `0644` and runtime-tunable via
`/sys/module/pvrsrvkm/parameters/`. Separately, `blt_submit.c` in gma500 gained a
**spin-first fast path** in `psb_blt_wait_idle_progress()`: a small 2D fill that
drains in microseconds returns from a short spin-poll (`PSB_BLT_SPIN_TRIES`)
without sleeping, so the X server is not starved by synchronous 2D fills under a
compositor.

The desktop that ships and displays is **MATE with marco** (the *non-compositing*
window manager); GL apps render and display accelerated on it. **Compiz / GL
compositing does NOT work and must not be run** — it bypasses the DDX DRI2 hooks
(verified by instrumentation: compiz calls neither `gma_dri2_create_buffer` nor
`gma_dri2_copy_region`, so its texture-from-pixmap and present happen entirely
inside the closed emgd GLX), renders a **black screen**, and on one launch
**hard-hung the machine** (full network death, physical power cycle required). See
[§9](#9-honest-limitations-and-future-work).

**Persistence.** The whole GLX path survives a clean reboot with zero manual
steps (reboot-verified): `poulsbo-sgx.service` → `load.sh` loads the modules
including `proc_dri_compat`; `poulsbo-sgx-srvinit.service` brings PVR services up;
`/etc/environment` sets `MESA_LOADER_DRIVER_OVERRIDE=emgd`; `xorg.conf.glx`
(gma500blt + AIGLX + glx/dri2) is the default X config; and
the getty autologin on tty1 (→ root's `.bash_profile` → `startx` on vt7 → a marco
session xinitrc, `packaging/desktop/`) starts the accelerated **marco** desktop
on boot — deliberately without compiz. After reboot the marco + HW-2D-pixmap + HW-GL desktop comes up and
`glxinfo` shows direct rendering on the SGX535.

New sources: `features/xf86-video-gma500blt/src/gma500blt_dri2.c` (DDX DRI2 +
PVR2D buffer bridge + descriptor + buffer cache), `features/glx-dri2/`
(`proc_dri_compat.c`), `features/emgd-wsegl/orig-ddk/` (recovered DDK WSEGL), the
`pvrsrvkm` `event_timeout_ms` / `sgx_apm_latency_ms` params, and the gma500
`blt_submit.c` spin-first path.

---

## 5. Module parameters reference

All are `0644` and runtime-writable via `/sys/module/<mod>/parameters/<name>`.

### `gma500_gfx.pvr_irq_forward` (int, default `1`)

Toggles the SGX-IRQ → pvrsrvkm forward (§4.2). `1` = interrupt-driven present
(~950 FPS path). `0` = the forward is disabled and pvrsrvkm falls back to the
~250 ms PVRSRV event-object poll → **HW-confirmed ~4.8 FPS**. Provided mainly to
demonstrate and A/B the optimization.

### `gma500_gfx.flip_vsync` (int, default `0`)

`0` = present returns as soon as the scanout base is written (the max-rate path,
~500–950 FPS). `1` = the present is **deferred**: `gma500_flip_to()` still just
arms the scanout base (which the display latches at the next frame start, so the
flip is tear-free), and gma500dc hands the DC command's completion to
`gma500_flip_defer_complete()`, which fires the completion ~one refresh period
later from a `delayed_work` — throttling `eglSwapBuffers` to roughly the panel
refresh **without blocking the SGX MISR** and without touching the display
vblank IRQ.

> **Design history + measured behaviour.** Three implementations were built:
> 1. a **busy-wait** on the pipe frame counter inside `gma500_flip_to()` — but it
>    ran in the single MISR workqueue, serializing render+flip and collapsing to
>    **~6.6 FPS**;
> 2. a **phase-locked vblank-IRQ** deferral (arm at the vblank interrupt, complete
>    from a worker) — this reached **~40 FPS tear-free**, but the vblank
>    enable/disable churn and cross-session state destabilised this (fragile)
>    box under repeated GL start/stop (it hung and needed a watchdog reboot);
> 3. the shipped **timer-deferred** completion — stable, non-MISR-blocking, but
>    the closed PVR2D present-completion round-trip compounds with the timer, so
>    the throttled rate is **~15–20 FPS** for trivial workloads.
>
> Net: `flip_vsync=1` is a stable tear-reduction / rate-cap toggle, not a route to
> 60 FPS. Reaching phase-locked 60 FPS needs either a more robust vblank-event
> path than this hardware/DRM-state reliably provides, or completion hooks inside
> the closed PVR2D present path.

### `gma500dc.flip_enable` (int, default `1`)

`1` = the DC flip processor moves the scanout on present. `0` = present
completes (sync/plumbing runs) **without** moving the scanout — useful for
isolating render vs. scanout.

### `pvrsrvkm.event_timeout_ms` (int, default `2`) — GLX frame-wait

The re-check interval for `EVENT_OBJECT_TIMEOUT_MS` in the PVR event-object wait
(`osfunc.c`). The stock value was a hard **100 ms**, which the EMGD desktop-GL
client paid *per frame* because a 3D render raises no host `SW_EVENT` for the
MISR to signal. `2` collapses that wait; see [§4.3](#43-glxdri2-desktop-opengl-the-buffer-bridge-and-performance).
Clamped to a `1 ms` floor.

### `pvrsrvkm.sgx_apm_latency_ms` (int, default `500`) — SGX power-down latency

The SGX active-power-management idle latency (`sysconfig.c`). The Poulsbo default
(`SYS_SGX_ACTIVE_POWER_LATENCY_MS_PLB`) was **1 ms**, so the SGX powered down
after every frame and paid a power-up penalty on the next. `500` keeps it awake
under load; `0` disables active PM entirely
(`bEnableActivePM = (sgx_apm_latency_ms != 0)`).

### `gma500_gfx.gma_no_sgx` (int) — pre-existing, display-only split

Forces gma500 to relinquish the SGX so pvrsrvkm can own it: `accel_2d=0` (fbcon
software), skips SGX power-management/reset, and avoids driving the SGX 2D core.
This mirrors the historical EMGD split (emgd = display, pvrsrvkm = SGX). Layer A
(fbcon SGX 2D accel) and Layer C (pvrsrvkm SGX render) are mutually exclusive on
the SGX; fbcon falls to software when the 3D stack is active (fine — fbcon is not
visible under X). Note a leftover `modprobe.d` conf may still add `gma_no_sgx=1`.

### `gma500_gfx.sgx3d_*` (experimental) — open microkernel bring-up track

Gate and knobs for the fully-open hand-written microkernel loader (`sgx_3d.c`),
**not** the closed-userspace render path. `sgx3d_enable` (default `0`) latches
the loader on; with it off the driver boots exactly as mainline and init never
runs at load — it is triggered only via the sysfs `poulsbo_sgx_3d` node. Others
(`sgx3d_code_base`, `sgx2d_variant`, and related) were bisection knobs during
bring-up. These are for diagnostics and reproduction of the microkernel results
in §8; they are not needed for the Layer C GLES2 path.

---

## 6. Build system

The Vaio is **too slow** to build kernel modules and **cannot hot-reload** the
graphics module (`rmmod`+`insmod gma500` hangs the box). So:

- **Build** in an **i386 Ubuntu 18.04 Docker** container against the Vaio's
  `5.4.0-150-generic` headers. This produces vermagic
  `5.4.0-150-generic SMP mod_unload modversions 686`, which matches the target
  exactly. Wrapper: `scratchpad/kbuild/build.sh` / `docker/kbuild/`.
- The DC driver and gma500's bridge link pvrsrvkm's exported symbols, so pass
  **`KBUILD_EXTRA_SYMBOLS=pvrsrvkm.symvers`** when building them.
- `pvrsrvkm` was ported from the 2013 EMGD/DDK-1.5.15.3226 tree (no Makefiles
  shipped — the Kbuild was reconstructed for ~42 objects: env/linux + common +
  bridged + devices/sgx + system plb/tnc). The EMGD-1.18 `-D` flag set
  (`SGX535`, `SGX_CORE_REV=121`, `SUPPORT_SGX535`, `SUPPORT_PERCONTEXT_PB`,
  `TRANSFER_QUEUE`, `SUPPORT_HW_RECOVERY`, `SUPPORT_ACTIVE_POWER_MANAGEMENT`,
  `PVR_SECURE_HANDLES`, `SUPPORT_SGX_EVENT_OBJECT`, `LDM_PCI`,
  `SUPPORT_SGX_HWPERF`, `SUPPORT_LINUX_X86_PAT`, `SUPPORT_CPU_CACHED_BUFFERS`,
  `SUPPORT_CACHEFLUSH_ON_ALLOC`) must match the closed-userspace ABI. The
  3.19→5.4 API churn that had to be ported: `get_user_pages` (5-arg +
  `FOLL_WRITE`), `page_cache_release`→`put_page`,
  `init_timer`→`timer_setup`/`from_timer`, `p4d_offset` level, 2-arg
  `access_ok`, `cpu_has_pat`→`boot_cpu_has`. Saved:
  `features/pvrsrvkm-BUILD_CONFIG.txt`, `features/pvrsrvkm-Kbuild`.

### Deploy layout (the real load path)

gma500 is **blacklisted** (`/etc/modprobe.d/zz-blacklist-gma500.conf`) and
`insmod`'d explicitly by `gma500-blt.service` → `/opt/gma500-blt/load.sh`. That
directory holds the three modules and `load.sh` insmods them in order:

```
/opt/gma500-blt/
  pvrsrvkm.ko          # loaded FIRST (gma500 links its bridge symbols)
  gma500_gfx.ko        # then gma500 (display + bridge + IRQ forward)  gma_no_sgx=1
  gma500dc.ko          # then the display-class driver
```

Userspace lives at `/opt/emgd/lib` (the 13 EMGD 1.18 libs + DT_SONAME
symlinks); the active WSEGL is `libwsegl_win.so` installed as
`libemgdPVR2D_DRIWSEGL.so.1.5.15.3226`. Bring the SGX up + run a GLES app with
`/opt/emgd/run-gles.sh <prog>` (starts Xvfb for the EGL X connection, runs
`./srvinit` to bring the SGX up, then the offscreen/window client). Backups of
each prior `.ko` are kept for revert; `panic_on_oops` is set `0` for survivable
iteration. **Never** `rmmod`+`insmod` gma500 on a live box — deploy via
blacklist + fresh-device insmod, or install + reboot.

---

## 7. Reverse-engineering methodology, clean-room, register DB

### 7.1 Clean-room rules (retained)

If this project becomes useful, the fastest way to destroy it is to contaminate
it with proprietary source or NDA-derived expression. The operating rules:

- **Allowed inputs:** public kernel/Mesa source, public docs and manuals, public
  patents, self-produced hardware traces, self-produced behavioral notes.
- **Disallowed inputs:** proprietary/leaked driver source, private SDK
  headers/manuals not intended for public release, notes copied from
  confidential documents, decompiled code pasted into the repo.
- **Behavior is acceptable; proprietary expression is not.** Good note:
  *"writing `0x20000000` to `PSB_CR_PDS_EXEC_BASE` precedes successful clear
  jobs."* Bad note: *"vendor function `FooSubmitKick()` does X, Y, Z."*
- **Team separation:** a tracer captures behavior into neutral notes; an
  implementer writes code from neutral notes + public info. Do not mix raw
  proprietary material into implementation discussion.
- **Repository rule:** never commit proprietary binaries unless their license
  explicitly allows redistribution, nor disassembly derived from proprietary
  objects without review.

**Note on the firmware and closed userspace.** Layer C uses Intel's EMGD 1.18
binaries — the SGX microkernel firmware is embedded as data symbols in
`libemgdsrv_init.so`, and the GLES2/EGL runtime is the closed EMGD library set.
These ship in Intel's **redistributable** EMGD RPMs
(`research/external/intel-binaries-linux/binaries/*/emgd-bin-3398-1.18.i586.rpm`).
This is the openpvrsgx model — an open driver plus a non-free firmware/userspace
blob — and it is a **personal-use** configuration on this hardware. The open
kernel work (bridge, DC driver, WSEGL, 2D BLT, `sgx_3d` loader) is clean-room;
the closed blobs are used, not reproduced.

### 7.2 Register database

`data/psb_registers.csv` is the MMIO register DB (validated against real silicon
in M0). Key offsets relative to the SGX CR block (`BAR0 + 0x40000`):

| Offset | Name | Use |
|---|---|---|
| `0x0000` | `CLKGATECTL` | clock gating (init writes `0x00111111`) |
| `0x0010` / `0x0014` | `CORE_ID` / `CORE_REVISION` | identity (`0x01130000` / `0x00010201`) |
| `0x0080` | `SOFT_RESET` | soft reset |
| `0x012C` / `0x0130` / `0x0134` | `EVENT_STATUS` / `EVENT_HOST_ENABLE` / `EVENT_HOST_CLEAR` | event/IRQ (the IRQ-forward touches `HOST_CLEAR`) |
| `0x0A0C`–`0x0A48` | `USE_CODE_BASE0..15` | USSE code bases (base15 = init ukernel; encoded `(codeBase>>7)|(DM3<<25)`) |
| `0x0A68` | (PRIMARY1 init-PDS device VA `& ~0xF`) | the boot-kick dispatch PDS — the fix that made the microkernel execute |
| `0x0AB8` | `PDS_EXEC_BASE` | PDS execution base (1 MiB-granular) |
| `0x0AC4` / `0x0AC8` | `EVENT_KICKER` / `EVENT_KICK` | doorbell (kicker VA, then kick) |
| `0x0B10` / `0x0B1C` | `EDMTask0` / `EDMTask1` | EDM task-status readback (advances per processed kick) |
| `0x0C00`–`0x0C88` | `BIF_CTRL` … `BIF_DIR_LIST_BASE0/1`, `BIF_TWOD_REQ_BASE` | MMU/BIF (fault decode, PD program, 2D req base) |
| `0x0CA8` | `BIF_MEM_REQ_STAT` | outstanding memory-request status (isolates a hang vs. a fault) |
| `0x0E04` / `0x0E18` | `2D_BLIT_STATUS` / `2D_SOCIF` | dedicated 2D BLIT core status / slave FIFO |
| display | `0x70008/0x70180/0x70184` (pipe A), `0x71008/0x71180/0x71184` (pipe B) | PIPExCONF / DSPxCNTR / DSPxBASE |

Helper scripts: `scripts/annotate_mmio.py`, `scripts/update_register_db.py`,
`scripts/compare_mmio_logs.py`. Update the DB whenever a new offset is confirmed
on silicon.

### 7.3 RE source assets (ground truth)

- **EMGD closed source-available driver:**
  `research/external/intel-binaries-linux/drm/pvr/services4/srvkm/devices/sgx/`
  (`sgxkick.c`, `sgxinit.c`, `sgxutils.c`) — the actual PowerVR SGX535 kernel
  submission code (CCB write, doorbell, microkernel upload, MMU, IRQ). This is
  the reference the ported pvrsrvkm and the `sgx_3d` loader derive from, plus the
  DRM bridge reference `drm/emgd/drm/emgd_drv.c` and `drm/pvr/.../pvr_drm.c`.
- **openpvrsgx:** `research/external/linux_openpvrsgx` — modern open SGX kernel
  driver; register offsets, firmware load, clock/power, MMU.
- **patjak/drm-gma500** `blitter_hacking` branch — real working blitter/MMU/IRQ
  glue for this hardware class; the model for the 2D BLT submit path.
- **Ghidra** (10.4 + JDK17) was decisive for the microkernel loader: it
  decompiled `SGXInit` to clean C
  (`features/sgx-3d-bringup/sgxinit_ghidra_decomp.c`), exposing the exact
  buffer/relocation layout that hand/objdump extraction had gotten wrong.
- The SGX **USSE / PDS instruction set** has no public disassembler; the subset
  needed (LIMM 21/5/6 immediate split, `DOUTA`, store/flow ops, the `enc1530`
  bit-splicer) was reversed by multi-pass decode and is documented in
  `features/sgx-3d-bringup/*.txt`.

---

## 8. SGX 3D bring-up history (the open microkernel track)

This is the fully-open, no-closed-userspace track that hand-writes the DDK
microkernel loader in the kernel (`features/sgx-3d-bringup/sgx_3d.c`). It is a
distinct achievement from the Layer C GLES2 path and is the deepest an open
SGX535 bring-up is known to have reached. The abbreviated arc:

1. **Microkernel executes** on real SGX535. The long chase to first execution
   ended at register `0x0A68` = PRIMARY1 init-PDS device VA — the driver had
   written a zeroed page there, so the boot `EVENT_KICK` ran a null PDS and the
   microkernel never executed an instruction. Fixing it made `EDMTask0` advance.
2. **`INIT_COMPLETE` reached.** The final blocker was a **USSE LIMM sibling-word
   corruption** in `sgx3d_relocate()`: eight const-size fields are word-0 of
   two-word LIMM instructions whose immediate is split 21/5/6 across both words.
   Raw-writing word-0 left a stale word-1 placeholder (`0xfc237150`), so the
   hardware loaded a corrupted constant whose bits[28:24] drove the PDS fetch to
   `secondary_base+0x1E0` = `0xF80401E0` and hung on one un-returning memory read
   (both the fetch VA and `PDS_PC_BASE=0x3e010078` reproduced bit-for-bit from
   the corrupted value). Splicing all 8 sites with the recovered `enc1530`
   encoder cleared it — the microkernel boots and writes `INIT_COMPLETE`
   essentially instantly.
3. **CCB command transport works.** Host writes
   `SGXMKIF_COMMAND{ServiceAddress, CacheControl, Data[2]}` into the kernel CCB
   ring; `EVENT_KICK`; the microkernel consumes and advances `ReadOffset`.
   `POWER` and `GETMISCINFO` commands consume cleanly (`ServiceAddress` =
   `HOSTKICK_byteoffset >> 3`). Required: separate CCB-CTL `{Woff,Roff}` buffer,
   kernel CCB ring ≥ `0x8000`, and `clflush_cache_range` before every device
   read (Poulsbo GPU is non-coherent).
4. **A handler executes real logic and returns data.** `GETMISCINFO` returns
   `CoreRev=0x10201` (== HW `CORE_REVISION`) + `CoreID=0x1130000` with the READY
   handshake. The dispatch fix was the EDM-CTL layout: the EDM reads command
   bodies (incl. `Data[1]`) from the ring at `EDM-CTL+0x60`, and the InitStatus
   target is `EDM-CTL+0x04` (HW-proven, overriding the literal Ghidra offset).

**Where it stops:** microkernel-driven **render** (2D-via-USSE, TA/3D triangle)
needs a HW render-context object + per-context CCB payload + USSE/PDS **shader
microcode** that are generated by the closed EMGD userspace (`libpvr2d` /
`libGLES`); the kernel only *registers* a userspace-built context
(`SGXRegisterHW2DContextKM`). Proven on hardware: the microkernel's `CMD_2D`
handler does **not** drive the dedicated 2D BLIT core (`2D_SOCIF` stays empty for
all `VIRTUAL_FIFO` / `TWOD_REQ_BASE` configs) — it emits to the USSE pixel data
master, which needs the userspace shader pipeline. The two-emit descriptor
layout was decoded to the byte level; the emit *mechanism* is fully understood,
but the HW-2D-context *contents* (a structured DDK object with a pointer at
`+0x04`) are userspace-generated and not in the available materials. So the open
render track is materials-gated at exactly the same wall Layer C sidesteps by
using the closed render libs.

Artifacts: `features/sgx-3d-bringup/` — the driver (`sgx_3d.c`), firmware
headers (`sgx_fw.h`, `sgx_fw_blob.h`, `sgx_pds.h`), the `SGXInit` Ghidra
decompilation, the USSE handler disassemblies (`getmisc_handler_usse.txt`,
`twod_handler_usse.txt`), `M4-first-execution.txt`, `RENDER_PLAN.md`, `notes.md`.

---

## 9. Honest limitations and future work

State these plainly; do not overclaim.

- **Open microkernel *rendering* is materials-gated.** Real 3D RENDER commands
  (TA/triangle) beyond `INIT_COMPLETE` + 2D dispatch require the
  proprietary-userspace-generated HW-context byte layout (the EMGD render libs).
  The open `sgx_3d` track reaches `INIT_COMPLETE`, CCB transport, and a
  real-data handler, but not open rendering.
- **The working GLES2 render path is via closed binaries.** Layer C's actual
  frames come from the closed EMGD EGL/GLES + WSEGL + PVR2D running on the ported
  pvrsrvkm + the open bridge. The 3D render itself is fully SGX-accelerated
  (verified: `GL_RENDERER = "EMGD on PowerVR SGX 535"`, correct interpolated
  varyings, `glGetError=0`), but it is not an open userspace.
- **`flip_vsync=1` throttles to ~15–20 FPS** for trivial workloads. It is a
  stable, non-MISR-blocking timer-deferred present (§5) that reduces tearing and
  caps the rate, but the closed present-completion round-trip keeps it well below
  the 60 Hz refresh; a phase-locked vblank-IRQ variant hit ~40 FPS tear-free but
  was unstable on this box. Default `0` keeps the max-rate path.
- **The whole stack is target-pinned:** prebuilt for kernel `5.4.0-150-generic`
  i686 on this specific Vaio, and the display-pipe assumption (pipe B) is this
  panel's.
- **Desktop OpenGL under X (GLX/DRI2) works and displays** (this was previously
  listed as open). `emgd_dri.so` is bridged into Mesa-20 GLX on our open
  `gma500blt` DDX (§4.3): `glxinfo` = direct rendering Yes / EMGD on PowerVR
  SGX535 / OpenGL 2.1, `glxgears` renders and displays at **~66 FPS (measured)**,
  reboot-persistent, on a **marco** (non-compositing) desktop. Honest caveats:
  OpenGL is **2.1** (fixed-function + GLSL 1.20, what SGX535 realistically does);
  the DRI2 present is a **CPU copy** of the client back-buffer into the window's
  KMS scanout region (not zero-copy); and the whole GLX enablement leans on a
  binary `libGLX_mesa` patch + a patchelf + a `/proc/dri` compat shim to reconcile
  the 2010-era emgd DRI2 driver with the Mesa-20 loader.
- **On-screen (scanout) 2D is now hardware-accelerated** (was CPU): the KMS front
  buffer is `PVR2DMemWrap`ped (NONCONTIGUOUS — the services layer
  `get_user_pages` the GTT-scattered CPU mapping) and hung on the root pixmap's
  EXA private, so `PVR2DBlt` targets the screen and window dragging is a HW blit.
  Verified (renders correctly, dragging smoother, 0 faults). Possible edge:
  overlapping root→root drags (PVR2DBlt overlap semantics) — not seen as
  corruption; guard `src==dst` overlap → CPU if trails appear. (This superseded
  the earlier plan that on-screen accel would need kernel PRIME/dma-buf —
  `PVR2DMemWrap` of the scanout's CPU mapping was enough.)
- **Compiz / GL compositing does NOT work and is unsafe.** It bypasses the DDX
  DRI2 hooks (goes through the closed emgd GLX), renders a **black screen**, and
  has **hard-hung the box** (full network death, physical power cycle). The
  desktop uses marco on purpose; completing compiz is a deep RE of the closed
  emgd GLX present/TFP path, not a DDX tweak. Do not run it on this hardware.
- **The direct 2D-BLT engine cannot coexist with the SGX/GL stack** (§0). The
  `features/blt-ioctl` path is proven 12/12 — but only with SGX uninitialised; it
  wedges the core once `SGXInitialise` runs.
- **No framebuffer readback.** This DDX has no `GetImage` path — `xsetroot`
  read-back and `import`/screenshots return **black** (a debugging false-negative;
  ground truth is a photo of the physical panel).
- **Layer A/C are mutually exclusive on the SGX** (direct fbcon 2D-BLT vs.
  pvrsrvkm render); this is the §0 conflict, and matches the historical EMGD split.

### Near-term future work

1. A robust phase-locked vblank present (the timer-deferred `flip_vsync` ships
   stable but throttles to ~15–20 FPS; a vblank-IRQ variant hit ~40 FPS tear-free
   but destabilised this box). Needs a more reliable vblank-event path than the
   fbcon CRTC's DRM vblank state provides here, or a completion hook inside the
   closed PVR2D present path.
2. Async/fenced kernel BLT submit so Layer B can accelerate the on-screen root
   pixmap (deferred unpin, pipelined ops) instead of falling to software.
3. Zero-copy DRI2 present: the GLX `CopyRegion` path (§4.3) currently 2D-BLTs the
   client back-buffer to the window. Routing it through the gma500dc scanout flip
   (as the GLES2 path does) or a PVR2D flip chain would drop the copy. (Basic
   GL/DDX buffer sharing is already solved by the PVR2D export bridge; full
   PRIME/dma-buf on gma500 would generalise it.)
4. Continue the open render track only if the EMGD userspace render package
   (`libpvr2d` etc.) can be sourced or the USSE ISA is more fully reversed — the
   HW-2D-context byte layout is the single remaining gate.

---

## 10. Source map

Real, on-disk locations (all paths relative to the repo root).

- **Layer A — 2D BLT ioctl:** `features/blt-ioctl/` — kernel `blt_submit.c` +
  uapi `gma_blt_drm.h` + `patch.diff`; userspace test `user/psb_blt_test.c`;
  `deploy/` (service + `load.sh` + patched `.ko`);
  `HARDWARE_PROOF_2026-07-17.txt` (12/12).
- **Layer B — EXA DDX:** `features/xf86-video-gma500blt/src/`
  (`gma500blt_exa.c`, `gma500blt_driver.c`, `gma500blt.h`).
- **Layer C2 — GLX/DRI2 desktop GL:**
  `features/xf86-video-gma500blt/src/gma500blt_dri2.c` (DDX DRI2 + PVR2D
  buffer-export bridge + emgd descriptor + buffer cache);
  `features/glx-dri2/proc_dri_compat.c` (+ `Kbuild`) the `/proc/dri/0/name`
  compat module; `features/emgd-wsegl/orig-ddk/libemgdPVR2D_DRIWSEGL.so.1.5.15.3226`
  the recovered DDK window WSEGL; the `event_timeout_ms` (`osfunc.c`) /
  `sgx_apm_latency_ms` (`sysconfig.c`) pvrsrvkm params; the `blt_submit.c`
  spin-first fast path. Package-side enablement: `packaging/glx-extra/`
  (`glx-enable.sh`, `xorg.conf.glx`).
- **Layer C — ported render module:** `features/pvrsrvkm-port/src` (+ `Kbuild`);
  build config `features/pvrsrvkm-BUILD_CONFIG.txt`, `features/pvrsrvkm-Kbuild`.
  The PVR-over-DRM bridge shim (`pvr_gma_bridge.c/.h`) and gma500 bridge/IRQ/flip
  additions (`gma500_flip.{c,h}`, `psb_pvr_gem_mmap`, `psb_sgx_interrupt`
  forward) live in the gma500 build tree; `gma500dc` (display-class driver) and
  the WSEGL (`wsegl_null.c`, `wsegl_win.c`) in the DC build dir.
- **Open microkernel track:** `features/sgx-3d-bringup/` (see §8).
- **Firmware (redistributable EMGD):**
  `research/external/intel-binaries-linux/binaries/*/emgd-bin-3398-1.18.i586.rpm`;
  microkernel-carrying `firmware/sgx535-emgd-1.18/libemgdsrv_init.so.1.5.15.3226`.
- **RE reference sources:** `research/external/intel-binaries-linux/drm/pvr/…`,
  `research/external/linux_openpvrsgx`.
- **Register DB + tooling:** `data/psb_registers.csv`,
  `scripts/annotate_mmio.py`, `scripts/update_register_db.py`,
  `scripts/compare_mmio_logs.py`.
- **Deliverable summary:** `features/DRIVER.md`.
- **Design notes (retained):** `docs/01-reverse-engineering-plan.md`,
  `docs/02-kernel-driver-design.md`, `docs/05-hardware-notes.md`,
  `docs/06-clean-room-rules.md`, `docs/09-first-hardware-session.md`,
  `docs/10-ubuntu-18.04-target.md`.

---

## 11. Repo hygiene notes

- **Build off-target, deploy on-target.** Build in the i386 Docker; scp modules
  to the Vaio; never build kernel modules on the Poulsbo machine.
- **Documentation is part of the deliverable.** For every meaningful driver,
  tooling, or deploy change, update the relevant doc and the progress log.
- **The historical `sgx_lab` in-kernel "submit pipeline" is superseded.** It was
  a large simulated submit path that did almost no real MMIO; the real 2D submit
  (Layer A) and the real SGX bring-up (Layers C / §8) replace it. Only its
  read-only introspection was worth keeping.

---

*Non-goals for now: Vulkan, Wayland compositor tuning, video decode, perfect
power management, and complete desktop GL beyond what SGX535 can realistically
do. The delivered target — a stable open 2D stack, an open X11 EXA driver, and
hardware-accelerated GLES2 on the SGX535 with sane fault handling — is met and
verified on the Vaio.*
