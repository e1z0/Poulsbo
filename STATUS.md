# Project status — Poulsbo / GMA500 (PowerVR SGX535)

_Last updated: 2026-10-13 · branch `compiz` @ `c2749fd2` · release line `1.0-rc2`_

Target: Ubuntu 18.04 LTS, kernel `5.4.0-150-generic` (i386), Sony Vaio P
(Atom Z520, US15W, SGX535 core rev 1.2.1, LVDS 1600x768 on pipe B).
See [README.md](README.md) for install, [ENGINEERING.md](ENGINEERING.md) for
internals, [PACKAGING.md](PACKAGING.md) for builds.

**One-line summary:** a usable hardware-accelerated desktop (HW 2D + HW OpenGL
2.1 / GLES2) runs on real Poulsbo hardware and ships as `.deb`s. It is built
from open kernel/X pieces plus the closed Intel EMGD GL userspace and SGX
firmware. GL compositing (compiz) works, but by default on software GL.

---

## 1. Working, verified on hardware

| Area | State | Notes |
|---|---|---|
| Kernel: `pvrsrvkm` (SGX services) | ✅ | Ported DDK 1.5.15.3226 services; microkernel firmware extracted from EMGD `libemgdsrv_init.so`; `INIT_COMPLETE`, `CoreRev=0x10201`. |
| Kernel: `gma500_gfx` + PVR-over-DRM bridge | ✅ | Display (KMS) and render coexist; SGX IRQ forwarded to pvrsrvkm (`pvr_irq_forward=1`, ~130x speed-up). |
| Kernel: `gma500dc` display-class | ✅ | Zero-copy hardware page-flip present for GLES2. |
| OpenGL ES 2.0 / EGL | ✅ | Unmodified GLES2 apps render on the USSE (vertex + fragment shaders) through closed EMGD EGL/GLES + our WSEGL. |
| Desktop OpenGL (GLX / DRI2) | ✅ | `glxinfo`: direct rendering Yes, *EMGD on PowerVR SGX535*, OpenGL 2.1 / GLSL 1.20. `glxgears` ~66 FPS on the panel. Reboot-persistent. |
| X 2D accel (`xf86-video-gma500blt`, EXA) | ✅ | "Path B": Solid/Copy via the SGX's own `PVR2DBlt`, for offscreen pixmaps (`PVR2DMemAlloc`) **and** the on-screen scanout (`PVR2DMemWrap`). Coexists with GL. |
| MATE desktop on marco | ✅ | Stable daily driver; auto-starts on boot (getty autologin + `startx`). |
| Compiz composited desktop | ✅ (software GL) | Wobbly windows + Greybird titlebars (decor + explicit `gtk-window-decorator`), artefact-free. Opt-in via `packaging/compiz/compiz-desktop.sh`. Compositing uses `LIBGL_ALWAYS_SOFTWARE=1` on purpose (see §2); apps still get HW 2D. |
| Packaging | ✅ | `poulsbo-sgx-modules`, `poulsbo-sgx-userspace`, `poulsbo-sgx` (meta) built by `packaging/build-debs.sh`; install → reboot → accelerated. |
| StarCraft: Brood War (Wine 3.0) | ✅ (windowed) | DDrawCompat **v0.3.1** = the speed fix (CPU 160% → ~18%, no GL). Launcher staged in `packaging/games/starcraft/`. |
| Open 2D-BLT ioctl (`GMA_BLT_SUBMIT`) | ✅ standalone only | 12/12 HW tests with the SGX **not** initialised (`features/blt-ioctl/`). |
| Open microkernel bring-up track (`sgx_3d.c`) | ✅ partial | Reaches `INIT_COMPLETE` + host↔microkernel CCB transport without any closed userspace. No open rendering. |

### Reverse-engineering fixes applied to the closed EMGD stack
- `emgd_dri.so` format-match bypass in `PVRDRICreateContext` (stops compiz from hard-hanging the box).
- DDX `texture_from_pixmap` fix: the front buffer is the redirected pixmap's own PVR2D export (stops black windows).
- `libcomposite.so` damage-promotion patch (every damage triggers a full repaint; fixes black-at-rest).
- `libGLX_mesa` patch + patchelf + `/proc/dri` compat shim, so the 2010-era `emgd_dri.so` loads under Mesa 20.

### Performance knobs (module params)
`pvrsrvkm.event_timeout_ms=2` (~2x GL FPS), `pvrsrvkm.sgx_apm_latency_ms=500`,
DDX buffer cache (17 → 105 FPS in a GL benchmark), `gma500dc.flip_enable=1`,
`gma500_gfx.flip_vsync=0` (1 = tear-reduced but ~15–20 FPS).

---

## 2. Known limitations / missing

| Item | Severity | Detail |
|---|---|---|
| **2D-BLT ↔ SGX BIF conflict** | Architectural | `SGXInitialise` reroutes the shared BIF (`BANK0 INDEX_2D`, `DIR_LIST_BASE0`, `TWOD_REQ_BASE`), so the direct 2D engine wedges once SGX is up (passes 5/12). Path B sidesteps it. Real fix: run 2D through the SGX dir-list/MMU model. Not done. |
| Hardware-GL compositing | Open | Compiz on SGX 3D (`compiz-desktop.sh hw`) still gives black artefacts because GL compositing contends with HW on-screen 2D. Software-GL compositing is the default. Heavy or continuous full-repaint GL load can panic-reboot the box. |
| Closed userspace dependency | Structural | GLES2/GL rendering, the shader compiler, and the SGX firmware are Intel EMGD binaries. No open Mesa driver, no open USSE compiler. Open rendering is blocked on the HW render-context layout produced by the closed libs. |
| Target pinning | Portability | Prebuilt modules load only on `5.4.0-150-generic` i386. The pipe-B assumption is specific to this panel. |
| GL feature level | HW-limited | OpenGL 2.1 max. FBOs crash the SGX under compiz (`framebuffer-object=true` → reboot). |
| DRI2 present | Perf | GLX present is a blit/copy into the window, not zero-copy (the GLES2 path is zero-copy). |
| Vsync | Perf | No stable tear-free vblank present at full rate (a vblank-IRQ variant reached ~40 FPS but was unstable). |
| Framebuffer readback | Debug | DDX has no `GetImage`, so screenshots of the root window are black. The physical panel is the ground truth. |
| StarCraft fullscreen | Open | 0.3.1 has no scaling. Next: DDrawCompat v0.4.0 + `DisplayResolution=desktop`, else a 640x480 LVDS modeline, else wmode. HW GL / GDI ddraw / DDrawCompat 0.5.4 all crash. |
| Target box fragility | Ops | No driver hot-reload (`rmmod`+`insmod gma500` hangs). Occasional soft-hangs need a physical power cycle. Deploy = install + reboot. |
| Docs drift | Housekeeping | `README.md` / `ENGINEERING.md` §1 and §9 still say compiz "does NOT work". That is outdated (see §1 above). |

---

## 3. Next steps (suggested order)
1. Merge `compiz` → `master`; update README/ENGINEERING compiz sections; ship `compiz-desktop.sh` in the userspace deb as an opt-in.
2. StarCraft fullscreen (DDrawCompat v0.4.0 test, then modeline fallback).
3. Integrate 2D into the SGX dir-list/MMU model to remove the BIF conflict, which would unblock artefact-free HW compositing.
4. Zero-copy DRI2 present via the gma500dc flip chain; robust vblank present.
5. Long term: open rendering (see §4).

---

## 4. Comparison with [gabrieelgama/SGX535-reMESA](https://github.com/gabrieelgama/SGX535-reMESA)

| | **Poulsbo (this repo)** | **SGX535-reMESA** |
|---|---|---|
| Goal | Usable accelerated desktop on Ubuntu 18.04 now | Fully open Mesa driver for the SGX535 |
| Approach | Open kernel/bridge/DDX + closed EMGD userspace & firmware, plus binary patches to the closed libs | Clean RE only; deliberately no proprietary DDK code |
| Hardware | Sony Vaio P (Z520, SGX535 rev 1.2.1) | Dell Inspiron Mini 12 (Z520, SGX535 rev121) |
| Kernel driver | Working (services, IRQ, MMU, display-class flip) | None yet |
| 3D | GLES2 + desktop GL 2.1 + GLX, compiz | First triangle and an indexed 16x16 square (Oct 2026) |
| Shader compiler | EMGD's closed one | Own USSE encoder/decoder, 447 offline tests passing |
| X / desktop | Working (EXA DDX, MATE, compiz) | None |
| Maturity | Daily-usable rc, kernel-pinned | Early experimental (~83 commits) |

**Takeaway:** we are well ahead on what runs today because we reuse the closed
EMGD stack. reMESA is the cleaner long-term path: open-source and portable
across kernels and Mesa versions. If we ever drop the blobs, their USSE ISA work
is the piece to adopt. Our register DB, firmware/BIF findings, and the
`sgx_3d.c` open bring-up could help them in return.
