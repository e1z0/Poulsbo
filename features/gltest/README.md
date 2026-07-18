# glburn — GLES2 capability + burn-in test suite for the SGX535

A hardcore GL ES 2.0 test harness that exercises the full open-bridge + closed
EMGD render path on the GMA500 / PowerVR SGX535 and reports pass/fail,
correctness, and performance for each test. Everything renders **offscreen**
into an EGL pbuffer (with a depth buffer) and reads the pixels back, so it runs
headless and verifies output (coverage, animation, GL errors) without needing a
visible display.

## Build & run (on the target)

```sh
# on the Vaio (i686), with the EMGD userspace installed:
cd gltest
ln -sf /path/to/emgd-headers emgd-headers      # GLES2/EGL SDK headers
./build.sh                                      # -> ./glburn
LD_LIBRARY_PATH=/opt/emgd/lib ./glburn all 5    # 5 s per test
```

Or the full self-checking runner (verifies the stack is loaded, brings the SGX
up, runs the battery under an X server, and captures any SGX/MMU faults from the
kernel log around the run):

```sh
sudo ./run_suite.sh 5
```

## Tests

| test | what it exercises |
|---|---|
| `caps`   | GL renderer/version + limits + extensions |
| `clear`  | clear throughput (animated) |
| `tri`    | per-vertex-coloured triangle (baseline) |
| `cube`   | depth-tested, animated, per-vertex-lit spinning 3D cube |
| `plasma` | heavy procedural fragment shader (ALU / fillrate) |
| `geo`    | geometry throughput — a dense rotating mesh (3200 triangles/frame) |
| `tex`    | textured quad (texture upload + sampling) |
| `blend`  | 8 alpha-blended layers (blend + overdraw) |
| `burn`   | sustained mixed scene; watches for GL errors / slowdowns / faults |
| `all`    | every test back to back |

Usage: `glburn <test> [seconds]` (default 3 s/test). Exit code `0` = all passed.

Each test **fails** if EGL/GL setup fails, a GL error is raised, the output is
blank/constant when it shouldn't be, or an animated test's frames don't change.

## Notes

- The reported FPS is **readback-bound**, not render-bound: every frame does a
  512×512 `glReadPixels` + a CPU checksum to verify correctness, which dwarfs the
  SGX render time. It is a correctness/stability harness, not a benchmark — use
  the zero-copy present path (`es2_prof` in `../emgd-wsegl`) for present-rate
  numbers.
- Verified on the Vaio: the full battery passes with **0 GL errors and 0 SGX/MMU
  faults**, including a sustained burn; the SGX535 handles 3D depth-testing,
  procedural shaders, thousands of triangles, texture sampling and alpha blending
  correctly. See `../../ENGINEERING.md`.
