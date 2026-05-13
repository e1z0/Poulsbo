# Ubuntu 18.04 Target

This project is now targeting Ubuntu 18.04 LTS user space for the first reproducible build environment.

The current real hardware target is a Sony VAIO VGN-P11Z running Ubuntu 18.04.5 with a `5.4.0-150-generic` `i686` kernel and Intel US15W/Poulsbo graphics.

Current live note on May 13, 2026:

- the old clean top-level `plan` session on `srcversion 2BE09D0DC1CCBDB51D2328E`
  still records the historical `sync / submit_op_kick_focus` blocker only
- the old later candidate `srcversion 0EEEE302E3FDA9F59B7CFB9` is also
  historical only now:
  - it required explicit `gma500_gfx` autoload on one headless boot
  - it later narrowed to `plan_query_offset`
  - it then panicked in `poulsbo_sgx_fill_submit_query_locked`
  - it is unsafe for further recovery-side `plan_query*` reruns
- the latest successful deploy session is now:
  [logs/vaio-deploy/20260513-183718-sgx-ta-work-item-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-183718-sgx-ta-work-item-deploy)
  and it booted the VAIO onto:
  - `driver_srcversion=2077F2C05141C6DFD29769B`
  - `override_module_size=464604`
  - `driver_module_loaded=1`
  - `drm_card0_present=1`
  - `lightdm_active=inactive`
  - `xorg_process_count=0`
  - `kdump_service_active=active`
- the exact no-redeploy `plan` ladder on that module is now clean all the way
  through `complete`, with the latest canonical terminal proof at:
  - [logs/vaio-plan-session/20260513-183909-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-183909-sgx-plan-session)
  - `session_outcome=completed`
  - `final_driver_srcversion=2077F2C05141C6DFD29769B`
- the new internal `psb_sgx_lab_submit_sgx_ta_work_item` batch is also
  hardware-validated:
  - exact regression proof:
    [logs/vaio-plan-session/20260513-183909-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-183909-sgx-plan-session)
  - focused submit-op bundle:
    [logs/vaio-ioctl/20260513-184011-sgx-submit-ta-work-item-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-184011-sgx-submit-ta-work-item-live/summary.log)
  - typed multi-submit bundle:
    [logs/vaio-ioctl/20260513-184054-sgx-multi-submit-ta-work-item-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-184054-sgx-multi-submit-ta-work-item-live/summary.log)
- the current execution-shaped SGX lab memory model now includes:
  - `queue header`
  - `ccb record`
  - `sync`
  - `owner`
  - `report`
  - `live`
  - `result`
  - `manifest`
  - `descriptor`
  - `plan`
  - `issue-plan`
  - the expanded non-executing 8-dword CCB record
- the exact operator profiles now cover:
  - `plan_query`
  - `plan_query_fetch`
  - `plan_query_basic`
  - `plan_query_offset`
  - `plan_query_config`
  - `plan_query_snapshot`
  - `prepare_op`
  - `prepare_query`
  - `arm`
  - `emit`
  - `validate`
  - `launch`
  - `issue`
  - `running`
  - `complete`
  - `post_descriptor`
  - `full`
- the shared kernel execution model now has twenty-two explicit internal layers:
  - `psb_sgx_lab_submit_exec_record`
  - `psb_sgx_lab_submit_exec_packet`
  - `psb_sgx_lab_submit_cmdta_kick`
  - `psb_sgx_lab_submit_ccb_kick`
  - `psb_sgx_lab_submit_sgx_ccb_kick`
  - `psb_sgx_lab_submit_sgx_cmdta_shared`
  - `psb_sgx_lab_submit_sgx_cmdta`
  - `psb_sgx_lab_submit_sgx_ta_kick`
  - `psb_sgx_lab_submit_sgx_ta_command`
  - `psb_sgx_lab_submit_sgx_queued_ta_command`
  - `psb_sgx_lab_submit_sgx_ta_submission`
  - `psb_sgx_lab_submit_sgx_ta_execution`
  - `psb_sgx_lab_submit_sgx_ta_timeline`
  - `psb_sgx_lab_submit_sgx_ta_runtime`
  - `psb_sgx_lab_submit_sgx_ta_gate`
  - `psb_sgx_lab_submit_sgx_ta_control`
  - `psb_sgx_lab_submit_sgx_ta_schedule`
  - `psb_sgx_lab_submit_sgx_ta_dispatch`
  - `psb_sgx_lab_submit_sgx_ta_path`
  - `psb_sgx_lab_submit_sgx_ta_commit`
  - `psb_sgx_lab_submit_sgx_ta_event`
  - `psb_sgx_lab_submit_sgx_ta_work_item`
- the shared current-boundary resolver now advances from successful exact
  evidence and currently resolves to:
  - `resolved_profile=complete`
  - `resolved_phase=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-183909`
  - `source_row_module_srcversion=2077F2C05141C6DFD29769B`
- the exact regression command is now:
  - `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete ./scripts/run-vaio-sgx-plan-session.sh`
- the preferred operator entrypoint remains:
  `./scripts/run-vaio-sgx-plan-session.sh`
- the preferred offline comparison view between reruns remains:
  `MAX_SESSIONS=10 ./scripts/summarize-vaio-plan-sessions.sh`
- the preferred single-command current-state report remains:
  `./scripts/report-vaio-plan-status.sh`
- the target is currently usable:
  - `ping_status=ok`
  - `tcp_22_status=open`
  - `ssh_pubkey_status=ok`
  - `overall=remote_usable`
- if the machine is already on the current override and you want the exact
  regression proof without a redeploy:
  `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete ./scripts/run-vaio-sgx-plan-session.sh`
- the next engineering step is no longer more exact ladder narrowing; it is
  the next SGX execution-side change on top of the new shared
  `exec_record -> exec_packet -> cmdta_kick -> ccb_kick -> sgx_ccb_kick -> cmdta_shared -> sgx_cmdta -> sgx_ta_kick -> sgx_ta_command -> sgx_queued_ta_command -> sgx_ta_submission -> sgx_ta_execution -> sgx_ta_timeline -> sgx_ta_runtime -> sgx_ta_gate -> sgx_ta_control -> sgx_ta_schedule -> sgx_ta_dispatch -> sgx_ta_path -> sgx_ta_commit -> sgx_ta_event -> sgx_ta_work_item`
  kernel model, with both live mutation entrypoints now routed through the
  shared `sgx_ta_work_item` top layer that also owns the queue/CCB/sync/owner
  write plan, next-submit-state apply image, live event-side accounting plan,
  and request execution rules, on top of the validated
  `queue + ccb + sync + owner + report + live + result + manifest + descriptor + plan + issue-plan`
  model

## Why This Target

- it matches the era of Poulsbo deployments better than a 2026 user-space stack
- it gives us old enough Xorg and Mesa packaging to reduce compatibility churn
- it still has a directly usable `ubuntu:18.04` container image for build isolation

## Validated Container Baseline

Validated on May 10, 2026 using an isolated Docker daemon and the `ubuntu:18.04` image.

Image identity:

- `ubuntu:18.04`
- `/etc/os-release` reports `Ubuntu 18.04.6 LTS (Bionic Beaver)`

APT package availability confirmed for:

- `xorg-dev` candidate `1:7.7+19ubuntu7.1`
- `xserver-xorg-dev` candidate `2:1.19.6-1ubuntu4.15`
- `libegl1-mesa-dev` candidate `20.0.8-0ubuntu1~18.04.1`
- `libgles2-mesa-dev` candidate `20.0.8-0ubuntu1~18.04.1`
- `linux-headers-generic` candidate `4.15.0.213.196`
- `build-essential` candidate `12.4ubuntu1`
- `meson` candidate `0.45.1-2ubuntu0.18.04.2`
- `ninja-build` candidate `1.8.2-1`
- `pkg-config` candidate `0.29.1-0ubuntu2`

## Build Target Split

Two build tracks matter:

### Kernel module work

- default header line: Ubuntu 18.04 generic `4.15`
- default in this repo: `4.15.0-213-generic`
- built as an out-of-tree lab module first
- current real target line: Ubuntu HWE `5.4.0-150-generic` on `i686`
- built locally against mirrored target headers, then deployed to the VAIO

### X11 and GLES work

- Xorg development packages from Bionic
- Mesa EGL and GLES2 development packages from Bionic updates
- small X11/EGL/GLES2 workloads compiled in-container

## Important Constraint

The Docker build environment is for reproducible compilation, not for running the real graphics stack. Real validation still has to happen on actual Poulsbo hardware.

For the VAIO target, do not compile on the machine itself. The working rule is:

- build locally in Docker on the workstation
- deploy the finished module over SSH
- activate it on the target with a reboot

## Validated Kernel Tree Build

The repository now carries a real Ubuntu 4.15 `gma500` lab patch plus a container build helper:

- patch: `patches/ubuntu-4.15/0001-drm-gma500-add-sgx-lab-instrumentation.patch`
- build helper: `scripts/docker-build-gma500-kernel-tree.sh`

Validated on May 10, 2026 against the Launchpad `Ubuntu-4.15.0-213.224` source tree under `third_party/linux-ubuntu-4.15`.

The build helper handles one Ubuntu-specific quirk automatically: Launchpad's source tree expects `scripts/ubuntu-retpoline-extract-one` to be staged by the Debian packaging rules, so the script copies `debian/scripts/retpoline-extract-one` into place before running Kbuild.

## Validated VAIO Build And Deploy Path

Validated on May 10, 2026 for the Sony VAIO VGN-P11Z target.

Local build inputs:

- image: `poulsbo:bionic-i386`
- mirrored target headers: `third_party/devnull-vaio-headers/linux-headers-5.4.0-150-generic`
- source tree: `third_party/linux-ubuntu-5.4`
- patch: `patches/ubuntu-5.4/0001-drm-gma500-add-sgx-lab-instrumentation.patch`

Helpers:

- build image: `scripts/docker-build-bionic-i386.sh`
- build module: `scripts/docker-build-vaio-gma500.sh`
- deploy module: `scripts/deploy-vaio-gma500.sh`
- guarded deploy module: `scripts/deploy-vaio-gma500-guarded.sh`
- exercise fbdev 2D lab path: `scripts/run-vaio-lab-2d.sh`
- build 32-bit X11/EGL workloads: `scripts/docker-build-workloads-i386.sh`
- run the X11/EGL userspace probe: `scripts/run-vaio-x11-probe.sh`
- run the X11 DRI2 probe: `scripts/run-vaio-dri2-probe.sh`
- run the PF-PD bind lab probe: `scripts/run-vaio-pf-bind.sh`
- run the SGX lab object probe: `scripts/run-vaio-sgx-object.sh`
- run the SGX lab submit and kick-packet probe: `scripts/run-vaio-sgx-submit.sh`
- revert override: `scripts/revert-vaio-gma500.sh`

The deploy helper installs the module to `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko` and runs `depmod`. This target does not carry `gma500_gfx` in the initramfs, so the override is selected by normal module resolution on the next boot.

## Validated Deterministic 2D Lab Path

Validated on May 10, 2026 on the VAIO after deploying the locally built module with `srcversion 3945602F3D24D73B294F188`.

The driver now exposes a writable PCI-device sysfs control:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_lab_2d`

Current supported commands:

- `clear <color>`
- `fill <dx> <dy> <w> <h> <color> [copy|xor]`
- `copy <sx> <sy> <dx> <dy> <w> <h>`
- `mono <dx> <dy> <w> <h> <fg> <bg> [checker|vertical|horizontal|diagonal|solid]`
- `sync`

The control reports its current framebuffer state on read. On the validated target that state is:

- `fb=1600x768`
- `bpp=32`
- `pitch=8192`
- `path=accel_2d`

The default helper run:

```bash
./scripts/run-vaio-lab-2d.sh
```

was verified to produce:

- `lab_2d clear` followed by `2D fillrect submit` and `SGX irq ... [TWOD_COMPLETE]`
- `lab_2d fill` followed by `2D fillrect submit` and `SGX irq ... [TWOD_COMPLETE]`
- `lab_2d copy` followed by `2D copyarea submit` and `SGX irq ... [TWOD_COMPLETE]`
- `lab_2d mono` followed by `2D imageblit submit-chunked` and `SGX irq ... [TWOD_COMPLETE]`

An explicit coverage run also validated:

- `fill ... xor` mapping through the 2D fill path with `rop=1`
- `mono ... diagonal` mapping through the chunked imageblit path

That gives us a deterministic way to exercise Poulsbo's 2D engine remotely without depending on whatever fbcon happens to do in a given workload.

## Validated X11 And EGL Probe Path

Validated on May 10, 2026 against the current LightDM `:0` session on the VAIO using the server auth file `/var/run/lightdm/root/:0`.

The current Xorg stack on the target is:

- `modesetting` DDX
- `glamoregl` present but rejected
- `AIGLX` initialized with `swrast`
- `OpenGL renderer string: llvmpipe (LLVM 10.0.0, 128 bits)`

The Xorg log explicitly shows:

- `modeset(0): Refusing to try glamor on llvmpipe`
- `modeset(0): glamor initialization failed`
- `AIGLX: Screen 0 is not DRI2 capable`
- `GLX: Initialized DRISWRAST GL provider for screen 0`

The new 32-bit userspace probe is:

- source: `workloads/x11_egl_probe.c`
- build helper: `scripts/docker-build-workloads-i386.sh`
- run helper: `scripts/run-vaio-x11-probe.sh`

The probe was validated to:

- open `DISPLAY=:0`
- initialize `EGL 1.4`
- create an X11 window surface
- make an ES context current
- resolve GLES entry points through `eglGetProcAddress`
- report `gl_renderer=llvmpipe (LLVM 10.0.0, 128 bits)`
- clear and swap successfully

The key warning emitted by the live stack is:

- `libEGL warning: DRI2: failed to authenticate`

The probe also reported `gles_library=missing`, so the target currently lacks a
normal `libGLESv2.so` runtime library even though `libEGL` can still create and
drive an ES context through Mesa's software path.

So the current userspace state is:

- scanout and KMS are working through `gma500`
- Xorg is running through `modesetting`
- EGL/GLES contexts can be created on X11
- rendering is still software through Mesa llvmpipe/swrast
- the DRI2 auth path is failing, which blocks the normal accelerated glamor path

## Validated DRI2 Probe Path

Validated on May 10, 2026 against the same LightDM `:0` session with:

- source: `workloads/x11_dri2_probe.c`
- run helper: `scripts/run-vaio-dri2-probe.sh`

The probe asks the X server for `DRI2QueryVersion`, `DRI2Connect`, and then
would attempt `DRI2Authenticate` if the server advertised a DRM device.

Observed result on the VAIO:

- `dri2_extension_present=1`
- `dri2_version=0.0`
- `dri2_driver_name_len=0`
- `dri2_device_name_len=0`

That is the concrete failure mode. The X server is exporting the DRI2 extension
opcode, but screen 0 never becomes a DRI2-capable screen, so there is no DRM
driver name, no device path, and no authentication exchange to perform.

In other words:

- `/dev/dri/card0` permissions are not the primary blocker
- the failure is earlier than `drmAuthMagic`
- `modesetting` fell back to a non-glamor software path, and DRI2 screen setup
  never completed

That lines up with the Xorg log and Mesa warnings:

- `modeset(0): Refusing to try glamor on llvmpipe`
- `modeset(0): glamor initialization failed`
- `libGL: screen 0 does not appear to be DRI2 capable`

## Validated DRM Probe Path

Validated on May 10, 2026 on `/dev/dri/card0` with:

- source: `workloads/drm_probe.c`
- run helper: `scripts/run-vaio-drm-probe.sh`

The probe opens the DRM device directly, prints capability bits, enumerates KMS
resources, and tests dumb create/map/destroy behavior.

Observed kernel-facing result on the VAIO:

- `drm_kernel_name=gma500`
- `drm_cap_dumb_buffer=1`
- `drm_cap_prime=0`
- `drm_cap_addfb2_modifiers=0`
- `drm_cap_syncobj=0`
- `drm_set_client_cap_universal_planes=0`
- `drm_set_client_cap_atomic=-1`
- `drm_plane_count=2`
- dumb create/map/destroy succeeds
- PRIME handle export fails
- no `/dev/dri/renderD*` node is present

The current KMS side is therefore functional:

- connector enumeration works
- the LVDS panel is reported as connected at `1600x768@60`
- dumb buffers can be allocated and CPU-mapped

But the render-facing userspace surface is still extremely small:

- no PRIME buffer sharing

## Validated Launch Submit Path

Validated on May 11, 2026 after guarded deploy of the locally built module:

- `driver_srcversion=A6F49F33394FCA892D9A5B2`
- `override_module_size=404800`
- `rollback_pending_present=0`

Canonical live bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260511-190148-sgx-submit-launch-op-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-190148-sgx-submit-launch-op-live/summary.log)
- typed multi-submit:
  - [logs/vaio-ioctl/20260511-190211-sgx-multi-submit-launch-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-190211-sgx-multi-submit-launch-live/summary.log)

The typed execution-shaped progression contract is now:

- `prepare`
- `arm`
- `validate`
- `emit`
- `validate`
- `launch`
- `validate`
- `issue`

Focused live result:

- `SUBMIT_ID=5`
- `submit_launch_valid=1`
- `submit_launch_consumed=0 -> 1` across `launch -> issue`
- `submit_launch_count=1`
- `submit_launch_prepare_count=1`
- `submit_launch_arm_count=1`
- `submit_launch_emit_count=1`
- `submit_launch_owner_state_name=queued`
- `submit_launch_issued_packets=1`
- `submit_launch_queue_packet_count=2`
- `submit_launch_sync_pending_seq=0x42`
- `submit_launch_owner_issue_cookie=0x102`
- `submit_launch_global_issue_ticket=1`
- final progression still reaches `submit_owner_state_name=complete`

Typed multi-submit live result:

- submit A final:
  - `submit_launch_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
- submit B final:
  - `submit_launch_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
- dependency-gated launched state is real:
  - submit B launch logged `dependency_active_count=1`
  - that launched dependency-bearing state then completed cleanly through the
    existing `issue -> transition running -> transition complete` path

The same batch also fixed the userspace `launch` selector bug in
`psb_sgx_lab_submit_op.c`, so `launch focus ...` now resolves the focused
submit instead of submitting `submit_id=0`, and the focused validator now
prints captured typed submit-op failure payloads when `launch` or `issue`
fails.
- no atomic modeset path
- no syncobj support
- no render node

## Validated SGX State Snapshot Path

Validated on May 10, 2026 on the VAIO after deploying the locally built module
with `srcversion 1488E719E92B4497BFCF240`.

The driver now exposes a second PCI-device sysfs view:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_state`

Reading this file captures and prints a live SGX-side snapshot. The snapshot
currently includes:

- snapshot metadata and reason string
- IRQ and fault counters
- `gatt_start`, `mmu_gatt_start`, and `gatt_free_offset`
- MMU driver cached `bif_ctrl`, pending TLB flush state, and default PD context
- SGX core, reset, event, and PDS/USE base registers
- BIF control, fault, bank, and directory-list registers
- 2D FIFO and blit status registers

The updated helper:

- `scripts/run-vaio-lab-2d.sh`

now prints `SGX_STATE_BEFORE` and `SGX_STATE_AFTER` around the default 2D lab
sequence.

Observed result on the VAIO:

- `STATE_ATTR=/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_state`
- `SGX_STATE_BEFORE` captured with `sequence=642`, `irq_count=641`,
  `fault_count=0`, `event_enable=0x8000000`, `event_enable2=0x000010`,
  `dir0=0x341da000`, `2d_base=0x80000000`, `3d_base=0x30000000`,
  `2d_blit_status=0x000582`
- `SGX_STATE_AFTER` captured with `sequence=654`, `irq_count=648`,
  `fault_count=0`, and `2d_blit_status=0x000590`

That result matters for two reasons:

- the sysfs snapshot path is working on real hardware, not just in the local
  build tree
- we now have a stable way to compare SGX/MMU/BIF state before and after known
  hardware-backed 2D activity

## Validated GTT And MMU State Path

Validated on May 10, 2026 on the VAIO after deploying the locally built module
with `srcversion FF3F35B2E190C724432A766`.

The driver now exposes a third PCI-device sysfs view:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_gtt_state`

Reading this file prints:

- GTT base layout and free-offset state
- MMU operation counters and the last recorded MMU operation
- the current `default_pd` and `pf_pd` hardware contexts
- every live `gtt_range` currently visible under the driver's resource tree

The new helper:

- `scripts/run-vaio-gtt-state.sh`

was validated on the target and reported:

- `mmu_counts flush=2 set_pd_context=2 insert_pfn_sequence=1 insert_pages=2 remove_pfn_sequence=0 remove_pages=0`
- range entries for `cursor`, `fb`, and multiple `gem` objects
- a live mmap'd GEM range with `in_gart=2`, `mmapping=1`, `npage=1200`

The result after the 2D lab run was also informative:

- SGX/TWOD IRQ activity increased
- MMU counters did not change

That tells us the current deterministic 2D lab path stays on the stolen-fb
side and does not exercise normal GEM-to-MMU binding.

The result after `scripts/run-vaio-drm-probe.sh` showed the counter path does
work for dumb-buffer GEM activity:

- `flush` increased from `2` to `4`
- `insert_pages` increased from `2` to `3`
- `remove_pages` increased from `0` to `1`

That gives us a concrete distinction between:

- stolen-fb 2D engine activity
- GEM/GTT/MMU-backed dumb-buffer activity

## Validated PF-PD Bind Lab Path

Validated on May 10, 2026 on the VAIO after deploying the locally built module
with `srcversion D18FBB9CEBF42668699D620`.

The driver now exposes a fourth PCI-device sysfs control:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_bind`

Current supported commands:

- `pf_bind <range_id>`
- `pf_unbind <range_id>`

The corresponding helper is:

- `scripts/run-vaio-pf-bind.sh`

That helper deploys the 32-bit `drm_probe` binary, creates a live dumb GEM
buffer on the VAIO, holds it open for `20` seconds, identifies the new live
`range id`, binds that range into `pf_pd`, unbinds it, then waits for userspace
destroy to remove the final default-PD mapping.

Validated observed result:

- new live range during probe hold:
  - `range id=10`
  - `name=gem`
  - `gpu_va=0x81127000`
  - `mmapping=1`
  - `npage=4`
- MMU counter path:
  - before hold: `flush=2 insert_pages=2 remove_pages=0`
  - after hold only: `flush=3 insert_pages=3 remove_pages=0`
  - after `pf_bind 10`: `flush=4 insert_pages=4 remove_pages=0`
  - after `pf_unbind 10`: `flush=5 insert_pages=4 remove_pages=1`
  - after probe exit: `flush=6 insert_pages=4 remove_pages=2`
- per-range PF state:
  - after bind: `pf_bind_count=1 pf_bind_total=1`
  - after unbind: `pf_bind_count=0 pf_unbind_total=1`
- target `dmesg`:
  - `lab pf_bind id=10 name=gem gpu_va=0x81127000 npage=4 active=1 total=1`
  - `lab pf_unbind id=10 name=gem gpu_va=0x81127000 active=0 total=1`
- `poulsbo_sgx_state` after the run still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`

This is the first explicit SGX-aware bind lifetime on the target. It is still
lab infrastructure, not a render path, but it proves that a real non-stolen GEM
object can be mapped into `pf_pd` and torn down cleanly on hardware.

## Validated SGX Lab Object Path

Validated on May 10, 2026 on the VAIO after deploying the locally built module
with `srcversion 92CA91A4FED5EB2EA73F40D`.

The driver now exposes two more PCI-device sysfs nodes:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_object`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_objects`

Current supported control commands:

- `adopt <range_id>`
- `pf_bind <object_id>`
- `pf_unbind <object_id>`
- `drop <object_id>`

The corresponding helper is:

- `scripts/run-vaio-sgx-object.sh`

That helper deploys the 32-bit `drm_probe` binary, creates and mmaps a dumb GEM
buffer, adopts the resulting `range id` into a kernel-owned SGX lab object,
binds and unbinds it through `pf_pd`, lets the userspace probe exit, confirms
that the adopted object still exists, and finally drops the object.

Validated observed result:

- during the held probe:
  - new range: `range id=14`
  - `gpu_va=0x80c77000`
  - `mmapping=1`
  - `npage=4`
- after `adopt 14`:
  - `object id=1`
  - `range_id=14`
  - `poulsbo_sgx_objects` shows one active object
- after `pf_bind 1`:
  - `flush=5 -> 6`
  - `insert_pages=4 -> 5`
  - `sgx_object_id=1`
  - `pf_bind_count=1`
- after `pf_unbind 1`:
  - `flush=6 -> 7`
  - `remove_pages=1 -> 2`
  - `pf_bind_count=0`
- after userspace exit:
  - `range id=14` still exists
  - `sgx_object_id=1` still points at it
  - `poulsbo_sgx_objects` still shows `object id=1`
- after `drop 1`:
  - `flush=7 -> 8`
  - `remove_pages=2 -> 3`
  - `range id=14` disappears
  - `poulsbo_sgx_objects` returns to `objects_total=0`

The target `dmesg` logged:

- `lab object adopt object_id=1 range_id=14 name=gem gpu_va=0x80c77000 size=0x4000`
- `lab pf_bind id=14 name=gem gpu_va=0x80c77000 npage=4 active=1 total=1`
- `lab pf_unbind id=14 name=gem gpu_va=0x80c77000 active=0 total=1`
- `lab object drop object_id=1 range_id=14 name=gem gpu_va=0x80c77000`

The steady-state SGX snapshot after the run still reported:

- `fault_count=0`
- `bif_fault=0x000000`

This is the first validated kernel-owned SGX object lifetime on the target. It
is still a lab interface, not a render ABI, but it proves that the driver can
keep a real GEM-backed object alive past userspace close and tear it down only
when explicitly told to do so.

## Validated SGX Lab Submit Path

Validated on May 11, 2026 on the VAIO after deploying the locally built module
with `srcversion 9A0DCD9E393D70292202009`.

The driver now exposes two more PCI-device sysfs nodes:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

The SGX object control also grew two new commands:

- `role <object_id> <generic|pds|use|control|target>`
- `context <object_id> <auto|default|pf>`

The submit control supports:

- `create <default|pf>`
- `attach <submit_id> <control|pds|use|target> <object_id>`
- `detach <submit_id> <slot>`
- `kick <submit_id> <cmd_type> <ccb_offset> <ctrl_flags> <num_ta_status> <num_3d_status> <ta_dependency>`
- `validate <submit_id>`
- `drop <submit_id>`

The corresponding helper is:

- `scripts/run-vaio-sgx-submit.sh`

That helper deploys the 32-bit `drm_probe` binary, creates three live dumb GEM
buffers, adopts them as SGX lab objects, assigns `pds` / `use` / `target`
roles, sets `pf` as the expected context, PF-binds all three objects, creates a
PF-context submit descriptor, validates it, waits for all three userspace probes
to exit, validates the same descriptor again, then drops the submit and all
three objects.

Validated observed result:

- three new live ranges:
  - `range id=10`
  - `range id=11`
  - `range id=12`
- adopted objects:
  - `object id=1`
  - `object id=2`
  - `object id=3`
- after role/context assignment and PF bind:
  - `object 1 -> role=pds expected_context=pf`
  - `object 2 -> role=use expected_context=pf`
  - `object 3 -> role=target expected_context=pf`
  - `flush=5 -> 8`
  - `insert_pages=5 -> 8`
- submit descriptor:
  - `submit id=1`
  - `context=pf`
  - `pds_object_id=1`
  - `use_object_id=2`
  - `target_object_id=3`
- first validation:
  - `validate_count=1`
  - `last_validate_status=0`
- after all three userspace probes exited:
  - all three objects still existed
  - the submit still existed
  - second validation still returned `0`
- final teardown:
  - `poulsbo_sgx_submits` returned to zero entries
  - `poulsbo_sgx_objects` returned to zero entries
  - `remove_pages=0 -> 6`

The target `dmesg` logged:

- object adopts for `object_id=1..3`
- role and context assignment for all three objects
- PF binds for `range id=10..12`
- `lab submit create submit_id=1 context=pf`
- attach for `pds`, `use`, and `target`
- two successful `lab submit validate ... status=0`
- final submit drop plus object drops

The steady-state SGX snapshot after the run still reported:

- `fault_count=0`
- `bif_fault=0x000000`

This is the first validated SGX-oriented multi-object submit descriptor on the
target. It still does not program SGX 3D execution, but it forces the driver to
track roles, context expectations, and submission slot structure against real
hardware-backed objects instead of ad hoc range IDs.

## Validated SGX Lab Kick Packet Layer

Validated on May 11, 2026 on the VAIO after deploying the locally built module
with `srcversion DB9A06B6BF23EF03033ED3E`.

The existing submit path now carries the first validate-only SGX-style kick
packet. The new submit control command is:

- `kick <submit_id> <cmd_type> <ccb_offset> <ctrl_flags> <num_ta_status> <num_3d_status> <ta_dependency>`

The helper remains:

- `scripts/run-vaio-sgx-submit.sh`

It now configures the submit packet before validation with:

- `cmd_type=ta`
- `ccb_offset=0x20`
- `ctrl_flags=0x1`
- `num_ta_status_vals=1`
- `num_3d_status_vals=1`
- `ta_dependency=0`

Validated observed result:

- the VAIO booted the new locally built module with
  `srcversion DB9A06B6BF23EF03033ED3E`
- the override module is loaded from:
  `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `run-vaio-sgx-submit.sh` still created three live GEM ranges:
  - `range id=10`
  - `range id=11`
  - `range id=12`
- those ranges were still adopted as `object id=1`, `object id=2`, and
  `object id=3`
- after `kick`, the live submit state showed:
  - `submit id=1`
  - `context=pf`
  - `cmd_type=ta`
  - `ccb_offset=0x20`
  - `ctrl_flags=0x1`
  - `ta_status=1`
  - `3d_status=1`
  - `ta_dependency=0`
  - `packet_config_count=1`
- after slot attach and validate:
  - `pds_object_id=1`
  - `use_object_id=2`
  - `target_object_id=3`
  - `validate_count=1`
  - `last_validate_status=0`
- after all three userspace probes exited:
  - all three objects still existed
  - the submit still existed
  - second validation still returned `0`
- final teardown returned:
  - `poulsbo_sgx_submits` to zero entries
  - `poulsbo_sgx_objects` to zero entries
  - MMU `remove_pages=0 -> 6`
- the SGX state after the run still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`

The target `dmesg` logged:

- `lab submit create submit_id=1 context=pf`
- `lab submit kick submit_id=1 cmd=ta ccb_offset=0x20 ctrl_flags=0x1 ta_status=1 3d_status=1 ta_dependency=0`
- attach for `pds`, `use`, and `target`
- two successful `lab submit validate ... status=0`
- final submit drop plus object drops

This is the first validated SGX-shaped kick packet in the repo. It still does
not execute SGX 3D work, but packet state is now explicit and visible on the
live hardware path instead of being implied by generic submit bookkeeping.

## Validated SGX Object-Class And GPU-Address Semantics

Validated on May 11, 2026 on the VAIO after deploying the locally built module
with `srcversion EF61C0BA3E7AC7F9B23DD47`.

The SGX object control now supports:

- `class <object_id> <generic|packet|code|state|surface>`
- `gpu_use <object_id> <generic|control_ccb|pds_code|use_code|render_target>`

The existing submit path now validates a four-object TA topology:

- `control`
- `pds`
- `use`
- `target`

The corresponding helper remains:

- `scripts/run-vaio-sgx-submit.sh`

It now:

- creates four held dumb GEM objects
- adopts four fresh live ranges by range-ID watermark instead of relying on the
  truncated one-page `poulsbo_gtt_state` diff
- assigns:
  - `control -> class=packet gpu_use=control_ccb`
  - `pds -> class=code gpu_use=pds_code`
  - `use -> class=code gpu_use=use_code`
  - `target -> class=surface gpu_use=render_target`
- PF-binds all four objects
- attaches all four submit slots

Validated observed result:

- the VAIO booted the new locally built module with
  `srcversion EF61C0BA3E7AC7F9B23DD47`
- the helper adopted four fresh ranges:
  - `range id=14`
  - `range id=15`
  - `range id=16`
  - `range id=17`
- those ranges became:
  - `object id=1`
  - `object id=2`
  - `object id=3`
  - `object id=4`
- after role/class/gpu-use/context assignment and PF bind:
  - `object 1 -> role=control class=packet gpu_use=control_ccb expected_context=pf`
  - `object 2 -> role=pds class=code gpu_use=pds_code expected_context=pf`
  - `object 3 -> role=use class=code gpu_use=use_code expected_context=pf`
  - `object 4 -> role=target class=surface gpu_use=render_target expected_context=pf`
  - MMU `insert_pages=10 -> 14`
  - MMU `flush=14 -> 18`
- after submit attach and validate, the live submit state showed:
  - `control_object_id=1 control_gpu_va=0x81127000`
  - `pds_object_id=2 pds_gpu_va=0x8112b000`
  - `use_object_id=3 use_gpu_va=0x8112f000`
  - `target_object_id=4 target_gpu_va=0x81133000`
  - `validate_count=1`
  - `last_validate_status=0`
- after all four userspace probes exited:
  - all four objects still existed
  - the submit still existed
  - second validation still returned `0`
- final teardown returned:
  - `poulsbo_sgx_submits` to zero entries
  - `poulsbo_sgx_objects` to zero entries
  - MMU `remove_pages=4 -> 12`
- the SGX state after the run still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`

The target `dmesg` logged:

- object adopts for `object_id=1..4`
- role assignment for `control`, `pds`, `use`, and `target`
- class assignment for `packet`, `code`, `code`, and `surface`
- GPU-use assignment for `control_ccb`, `pds_code`, `use_code`, and `render_target`
- PF binds for `range id=14..17`
- `lab submit attach` for all four slots
- two successful `lab submit validate ... status=0`
- final submit drop plus four object drops

This is the first validated explicit SGX object-class and GPU-address-use model
in the repo. The kick packet now refers to a real control object with a real
resolved GPU virtual address rather than only to anonymous slot IDs.

## Validated SGX Control-Block Model

Validated on May 11, 2026 on the VAIO after deploying the locally built module
with `srcversion A9D37B592F6EA8CB2A6BBA1`.

The submit path now treats the `control` object as a real lab control block,
not only as a symbolic slot. The submit control adds:

- `ccb <submit_id> <write_offset> <read_offset> <service> <cache> <data0> <data1>`

The current helper remains:

- `scripts/run-vaio-sgx-submit.sh`

It now:

- creates four held dumb GEM objects
- adopts four fresh ranges by range-ID watermark
- assigns `control/pds/use/target`, `packet/code/code/surface`, and
  `control_ccb/pds_code/use_code/render_target`
- PF-binds all four objects
- creates one PF-context submit descriptor
- configures the TA packet
- writes a minimal in-object CCB model into the attached `control` object
- validates once before userspace exit and once after userspace exit

The current minimal control-object layout is:

- dword `0x0`: `write_offset`
- dword `0x4`: `read_offset`
- dwords at `ccb_offset`:
  - `service`
  - `cache`
  - `data0`
  - `data1`

Validated observed result:

- the VAIO booted the new locally built module with
  `srcversion A9D37B592F6EA8CB2A6BBA1`
- the helper adopted four fresh live GEM ranges:
  - `range id=10`
  - `range id=11`
  - `range id=12`
  - `range id=13`
- those ranges became:
  - `object id=1`
  - `object id=2`
  - `object id=3`
  - `object id=4`
- after role/class/gpu-use/context assignment and PF bind:
  - `object 1 -> control / packet / control_ccb / pf`
  - `object 2 -> pds / code / pds_code / pf`
  - `object 3 -> use / code / use_code / pf`
  - `object 4 -> target / surface / render_target / pf`
- after `ccb`, the live submit state showed:
  - `ccb_write_offset=0x20`
  - `ccb_read_offset=0x0`
  - `ccb_service=0x1000`
  - `ccb_cache=0x0`
  - `ccb_data0=0xc0de0001`
  - `ccb_data1=0xc0de0002`
  - `ccb_config_count=1`
- after both validates:
  - `validate_count=2`
  - `last_validate_status=0`
- the live submit state still resolved:
  - `control_gpu_va=0x81127000`
  - `pds_gpu_va=0x8112b000`
  - `use_gpu_va=0x8112f000`
  - `target_gpu_va=0x81133000`
- after final teardown:
  - `poulsbo_sgx_submits` returned to zero entries
  - `poulsbo_sgx_objects` returned to zero entries
  - MMU `remove_pages=0 -> 8`
- the SGX state after the run still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`

The target `dmesg` logged:

- `lab submit ccb submit_id=1 write_offset=0x20 read_offset=0x0 service=0x1000 cache=0x0 data0=0xc0de0001 data1=0xc0de0002`
- two successful `lab submit validate ... status=0`

This is the first validated non-symbolic control-block model in the repo. The
current submit descriptor now owns explicit control-object contents and validates
them by readback from live GEM-backed pages on real hardware.

## Validated SGX Status-Block Model

Validated on May 11, 2026 on the VAIO after deploying the locally built module
with `srcversion 3482CBD7D1F81D9D4C6F81A`.

The loaded override module is now confirmed at:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`

The submit path now adds a fifth `status` slot and a separate state-backed
status layout. The new submit control command is:

- `status <submit_id> <ta_offset> <3d_offset> <stride> <seed>`

The current helper remains:

- `scripts/run-vaio-sgx-submit.sh`

It now:

- creates five held dumb GEM objects
- adopts five fresh ranges by range-ID watermark
- assigns `control/pds/use/target/status`
- assigns `packet/code/code/surface/state`
- assigns `control_ccb/pds_code/use_code/render_target/status_block`
- PF-binds all five objects
- creates one PF-context submit descriptor
- configures the TA packet
- writes the control block into the `control` object
- writes deterministic TA and 3D status words into the `status` object
- validates once before userspace exit and once after userspace exit

The current status-object layout is:

- TA status words at `ta_offset`, stepped by `stride`
- 3D status words at `3d_offset`, stepped by `stride`
- TA value at index `i`: `seed + 0x100 + i`
- 3D value at index `i`: `seed + 0x200 + i`

Validated observed result:

- the VAIO booted the new locally built module with
  `srcversion 3482CBD7D1F81D9D4C6F81A`
- the helper adopted five fresh live GEM ranges:
  - `range id=14`
  - `range id=15`
  - `range id=16`
  - `range id=17`
  - `range id=18`
- those ranges became:
  - `object id=1`
  - `object id=2`
  - `object id=3`
  - `object id=4`
  - `object id=5`
- after role/class/gpu-use/context assignment and PF bind:
  - `object 1 -> control / packet / control_ccb / pf`
  - `object 2 -> pds / code / pds_code / pf`
  - `object 3 -> use / code / use_code / pf`
  - `object 4 -> target / surface / render_target / pf`
  - `object 5 -> status / state / status_block / pf`
- after `status`, the live submit state showed:
  - `status_ta_offset=0x80`
  - `status_3d_offset=0xc0`
  - `status_stride=0x4`
  - `status_seed=0x5a500000`
  - `status_object_id=5`
  - `status_gpu_va=0x80c87000`
  - `status_config_count=1`
- after both validates:
  - `validate_count=2`
  - `last_validate_status=0`
- after final teardown:
  - `poulsbo_sgx_submits` returned to zero entries
  - `poulsbo_sgx_objects` returned to zero entries
  - MMU `insert_pages=8 -> 13`
  - MMU `remove_pages=1 -> 11`
- the SGX state after the run still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`

The target `dmesg` logged:

- `lab submit attach submit_id=1 slot=status object_id=5`
- `lab submit status submit_id=1 ta_offset=0x80 3d_offset=0xc0 stride=0x4 seed=0x5a500000 ta_count=1 3d_count=1`
- two successful `lab submit validate ... status=0`

This is the first validated explicit SGX status backing object in the repo. The
current submit path now has separate control, code, target, and status memory
roles on real hardware, still without attempting real SGX execution.

## Validated SGX Queue Model

Validated on May 11, 2026 on the VAIO after deploying the locally built module
with `srcversion 45E15DF2729D274214EAFB6`.

The loaded override module is now confirmed at:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`

The submit path now adds explicit queue progression metadata inside the
`control` object. The new submit control command is:

- `queue <submit_id> <base_offset> <size> <packet_size> <packet_count>`

The current helper remains:

- `scripts/run-vaio-sgx-submit.sh`

It now:

- creates five held dumb GEM objects
- adopts five fresh ranges by range-ID watermark
- assigns `control/pds/use/target/status`
- assigns `packet/code/code/surface/state`
- assigns `control_ccb/pds_code/use_code/render_target/status_block`
- PF-binds all five objects
- creates one PF-context submit descriptor
- configures the TA packet
- writes the control block into the `control` object
- writes queue metadata into the same `control` object
- writes deterministic TA and 3D status words into the `status` object
- validates once before userspace exit and once after userspace exit

The current queue layout is:

- dword `0x8`: `queue_base_offset`
- dword `0xc`: `queue_size`
- dword `0x10`: `queue_packet_size`
- dword `0x14`: `queue_packet_count`

The current queue progression semantics are:

- `ccb_offset == queue_base_offset + ccb_read_offset`
- `ccb_write_offset == ccb_read_offset + queue_packet_size * queue_packet_count`
- no wrap-around is modeled yet

Validated observed result:

- the VAIO booted the new locally built module with
  `srcversion 45E15DF2729D274214EAFB6`
- the helper adopted five fresh live GEM ranges:
  - `range id=10`
  - `range id=11`
  - `range id=12`
  - `range id=13`
  - `range id=14`
- those ranges became:
  - `object id=1`
  - `object id=2`
  - `object id=3`
  - `object id=4`
  - `object id=5`
- after `queue`, the live submit state showed:
  - `queue_base_offset=0x20`
  - `queue_size=0x40`
  - `queue_packet_size=0x20`
  - `queue_packet_count=1`
  - `queue_config_count=1`
- after both validates:
  - `validate_count=2`
  - `last_validate_status=0`
- the live submit state still resolved:
  - `control_gpu_va=0x81127000`
  - `pds_gpu_va=0x8112b000`
  - `use_gpu_va=0x8112f000`
  - `target_gpu_va=0x81133000`
  - `status_gpu_va=0x81137000`
- after final teardown:
  - `poulsbo_sgx_submits` returned to zero entries
  - `poulsbo_sgx_objects` returned to zero entries
  - MMU `insert_pages=7 -> 12`
  - MMU `remove_pages=0 -> 10`
- the SGX state after the run still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`

The target `dmesg` logged:

- `lab submit queue submit_id=1 base_offset=0x20 size=0x40 packet_size=0x20 packet_count=1 read_offset=0x0 write_offset=0x20`
- two successful `lab submit validate ... status=0`

This is the first validated explicit queue-progression model in the repo. The
current submit path now has explicit queue, control, and status semantics on
real hardware, still without attempting real SGX execution.

## Validated SGX Queue Wrap Model

Validated on May 11, 2026 on the VAIO after deploying the locally built module
with `srcversion B88E97705620D63C25579DD`.

The loaded override module is confirmed at:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`

The helper remains:

- `scripts/run-vaio-sgx-submit.sh`

The helper now defaults to:

- `QUEUE_MODE=wrap`

That wrap mode configures:

- `kick ccb_offset=0x80`
- `ccb write_offset=0x20 read_offset=0x60`
- `queue base_offset=0x20 size=0x80 packet_size=0x20 packet_count=2`

The queue model is still validate-only, but it now makes the split explicit by
writing and validating two more dwords in the `control` object:

- dword `0x18`: `queue_tail_packet_count`
- dword `0x1c`: `queue_head_packet_count`

For the validated wrap case, those became:

- `queue_tail_packet_count=1`
- `queue_head_packet_count=1`
- `queue_wrap=1`

That means one packet occupies the tail of the queue region at
`read_offset=0x60`, and one packet wraps back to the head while
`write_offset=0x20`.

Validated observed result:

- the VAIO booted the new locally built module with
  `srcversion B88E97705620D63C25579DD`
- the helper adopted five fresh live GEM ranges:
  - `range id=10`
  - `range id=11`
  - `range id=12`
  - `range id=13`
  - `range id=14`
- those ranges became:
  - `object id=1`
  - `object id=2`
  - `object id=3`
  - `object id=4`
  - `object id=5`
- after `queue`, the live submit state showed:
  - `queue_base_offset=0x20`
  - `queue_size=0x80`
  - `queue_packet_size=0x20`
  - `queue_packet_count=2`
  - `queue_tail_packet_count=1`
  - `queue_head_packet_count=1`
  - `queue_wrap=1`
- after both validates:
  - `validate_count=2`
  - `last_validate_status=0`
- the live submit state resolved:
  - `control_gpu_va=0x80c77000`
  - `pds_gpu_va=0x80c7b000`
  - `use_gpu_va=0x80c7f000`
  - `target_gpu_va=0x80c83000`
  - `status_gpu_va=0x80c87000`
- after final teardown:
  - `poulsbo_sgx_submits` returned to zero entries
  - `poulsbo_sgx_objects` returned to zero entries
  - MMU `insert_pages=3 -> 13`
  - MMU `remove_pages=1 -> 11`
- a direct post-run SGX state read reported:
  - `fault_count=0`
  - `bif_fault=0x000000`

The target `dmesg` logged:

- `lab submit queue submit_id=1 base_offset=0x20 size=0x80 packet_size=0x20 packet_count=2 read_offset=0x60 write_offset=0x20 tail_packets=1 head_packets=1 wrapped=1`
- two successful `lab submit validate ... status=0`

I also re-ran the helper in:

- `QUEUE_MODE=nowrap`

with `HOLD_SECONDS=5` as a regression check. The older linear path still
validated cleanly with:

- `queue_tail_packet_count=1`
- `queue_head_packet_count=0`
- `queue_wrap=0`
- two successful validates
- post-run teardown back to zero live submit/object entries

This is the first validated explicit wrap-aware queue model in the repo. The
current submit path now distinguishes tail and head packet occupancy inside the
queue region on real hardware, still without attempting real SGX execution.

## Validated SGX Sync Block And Recovered Remote Flow

On May 11, 2026 I extended the local kernel submit path with a validate-only
sync block in the existing `status` object and built a new local module with:

- `srcversion 8B1DA24F8B1EFA8C47EFB32`

The new sync block adds:

- mirrored queue `read_offset` and `write_offset`
- `sync_complete_seq`
- `sync_pending_seq`
- `sync_flags`

The VAIO helper now programs it through:

- `sync <submit_id> <offset> <complete_seq> <pending_seq> <flags>`

and the current design note is:

- `docs/24-sgx-sync-block-model.md`

This step did hit a real target-side recovery problem first: after the first
reboot, the VAIO answered ICMP and accepted TCP/22, but SSH and SCP stalled
after key exchange. I added:

- `scripts/check-vaio-target-health.sh`
- `scripts/deploy-vaio-gma500-guarded.sh`

and corrected the health classifier so `overall=remote_usable` reflects real SSH
command success, not only ICMP. The guarded deploy flow then brought the target
back into a usable remote state with:

- `ping_status=ok`
- `tcp_22_status=open`
- `ssh_none_status=auth_rejected`
- `ssh_pubkey_status=ok`
- `scp_status=ok`
- `overall=remote_usable`

I then validated the sync block on real hardware in both queue modes.

### Wrap validation

Validated on `10.1.11.218` with:

- `QUEUE_MODE=wrap`
- `HOLD_SECONDS=5`

Observed submit state:

- `cmd_type=ta`
- `ccb_offset=0x80`
- `queue_base_offset=0x20`
- `queue_size=0x80`
- `queue_packet_size=0x20`
- `queue_packet_count=2`
- `queue_tail_packet_count=1`
- `queue_head_packet_count=1`
- `queue_wrap=1`
- `sync_offset=0x20`
- `sync_complete_seq=0x40`
- `sync_pending_seq=0x42`
- `sync_flags=0x3`
- `validate_count=2`
- `last_validate_status=0`

Target `dmesg` logged:

- `lab submit sync submit_id=2 offset=0x20 read_offset=0x60 write_offset=0x20 complete_seq=0x40 pending_seq=0x42 flags=0x3 wrapped=1`
- two successful `lab submit validate ... status=0`

### Nowrap validation

Validated on the same target with:

- `QUEUE_MODE=nowrap`
- `HOLD_SECONDS=5`

Observed submit state:

- `cmd_type=ta`
- `ccb_offset=0x20`
- `queue_base_offset=0x20`
- `queue_size=0x40`
- `queue_packet_size=0x20`
- `queue_packet_count=1`
- `queue_tail_packet_count=1`
- `queue_head_packet_count=0`
- `queue_wrap=0`
- `sync_offset=0x20`
- `sync_complete_seq=0x40`
- `sync_pending_seq=0x41`
- `sync_flags=0x1`
- `validate_count=2`
- `last_validate_status=0`

Target `dmesg` logged:

- `lab submit sync submit_id=3 offset=0x20 read_offset=0x0 write_offset=0x20 complete_seq=0x40 pending_seq=0x41 flags=0x1 wrapped=0`
- two successful `lab submit validate ... status=0`

### Helper hardening required by the reruns

The userspace helper needed three real fixes before the validation reruns were
reliable:

- unique staged remote binary paths so repeat runs do not collide
- stale lab object and submit cleanup before a new run
- a fallback range-ID scan beyond the page-sized `poulsbo_gtt_state` dump, because that sysfs view truncates when enough live GEM ranges exist

### Final post-run target state

After the final nowrap run, the VAIO reported:

- loaded module path:
  - `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- loaded module `srcversion`:
  - `8B1DA24F8B1EFA8C47EFB32`
- `lightdm` state:
  - `active`
- direct `poulsbo_sgx_state` snapshot:
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `irq_count=643`
  - `capture_failed=0`

So the sync-block milestone is now:

- implemented locally
- deployed to the real VAIO target
- validated in both wrapped and linear queue modes
- clean at post-run SGX/BIF fault state

## Validated SGX Owner Block Model

On May 11, 2026 I extended the lab submit path again with a second validate-only
status-backed block for queue ownership and completion cookies. The current
design note is:

- `docs/26-sgx-owner-block-model.md`

The new submit control is:

- `owner <submit_id> <offset> <owner_state> <issue_cookie> <complete_cookie>`

I rebuilt locally, deployed with the guarded VAIO flow, rebooted successfully,
and validated the new block on real hardware with override module:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 29FE07980FD575A0704D90D`

### Wrap validation

- `QUEUE_MODE=wrap`
- `HOLD_SECONDS=5`
- `owner_offset=0x40`
- `owner_state=running`
- `owner_issue_cookie=0x102`
- `owner_complete_cookie=0x100`
- `validate_count=2`
- `last_validate_status=0`

Target `dmesg` logged:

- `lab submit owner submit_id=1 offset=0x40 state=running issue_cookie=0x102 complete_cookie=0x100 outstanding=0x2`
- two successful `lab submit validate ... status=0`

### Nowrap validation

- `QUEUE_MODE=nowrap`
- `HOLD_SECONDS=5`
- `owner_offset=0x40`
- `owner_state=running`
- `owner_issue_cookie=0x101`
- `owner_complete_cookie=0x100`
- `validate_count=2`
- `last_validate_status=0`

Target `dmesg` logged:

- `lab submit owner submit_id=2 offset=0x40 state=running issue_cookie=0x101 complete_cookie=0x100 outstanding=0x1`
- two successful `lab submit validate ... status=0`

### Final post-run target state

- loaded module path:
  - `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- loaded module `srcversion`:
  - `29FE07980FD575A0704D90D`
- `lightdm` state:
  - `active`
- direct `poulsbo_sgx_state` snapshot:
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `irq_count=386`

## Validated SGX Transition Model

On May 11, 2026 I extended the submit path from static owner/cookie state into
an explicit validate-only queue transition model. The current design note is:

- `docs/27-sgx-transition-model.md`

The new submit control is:

- `transition <submit_id> <owner_state> <completed_packets>`

I rebuilt locally, deployed with the guarded VAIO flow, rebooted successfully,
and validated the new model on real hardware with override module:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 0A250824D4E00B9BBB52C70`

### Wrap validation

- `QUEUE_MODE=wrap`
- `HOLD_SECONDS=5`
- `TRANSITION_ENABLE=1`
- `TRANSITION_RUNNING_COMPLETED_PACKETS=1`
- `TRANSITION_COMPLETE_COMPLETED_PACKETS=1`

Observed submit progression:

- initial owner state after config:
  - `owner_state=queued`
  - `queue_packet_count=2`
  - `queue_wrap=1`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x42`
  - `owner_issue_cookie=0x102`
  - `owner_complete_cookie=0x100`
- after `transition ... running 1`:
  - `owner_state=running`
  - `ccb_offset=0x20`
  - `ccb_read_offset=0x0`
  - `queue_packet_count=1`
  - `queue_tail_packet_count=1`
  - `queue_head_packet_count=0`
  - `queue_wrap=0`
  - `sync_complete_seq=0x41`
  - `owner_complete_cookie=0x101`
- after `transition ... complete 1`:
  - `owner_state=complete`
  - `ccb_offset=0x40`
  - `ccb_read_offset=0x20`
  - `queue_packet_count=0`
  - `queue_tail_packet_count=0`
  - `queue_head_packet_count=0`
  - `queue_wrap=0`
  - `sync_complete_seq=0x42`
  - `sync_pending_seq=0x42`
  - `owner_complete_cookie=0x102`
  - `transition_count=2`
  - `validate_count=4`
  - `last_validate_status=0`

Target `dmesg` logged:

- `lab submit transition submit_id=1 state=running completed_packets=1 new_read_offset=0x0 new_write_offset=0x20 new_ccb_offset=0x20 outstanding=1 wrapped=0 complete_seq=0x41 complete_cookie=0x101`
- `lab submit transition submit_id=1 state=complete completed_packets=1 new_read_offset=0x20 new_write_offset=0x20 new_ccb_offset=0x40 outstanding=0 wrapped=0 complete_seq=0x42 complete_cookie=0x102`
- four successful `lab submit validate ... status=0`

### Nowrap validation

- `QUEUE_MODE=nowrap`
- `HOLD_SECONDS=5`
- `TRANSITION_ENABLE=1`
- `TRANSITION_RUNNING_COMPLETED_PACKETS=0`
- `TRANSITION_COMPLETE_COMPLETED_PACKETS=1`

Observed submit progression:

- initial owner state after config:
  - `owner_state=queued`
  - `queue_packet_count=1`
  - `queue_wrap=0`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x41`
  - `owner_issue_cookie=0x101`
  - `owner_complete_cookie=0x100`
- after `transition ... running 0`:
  - `owner_state=running`
  - `ccb_offset=0x20`
  - `ccb_read_offset=0x0`
  - `queue_packet_count=1`
  - `sync_complete_seq=0x40`
  - `owner_complete_cookie=0x100`
- after `transition ... complete 1`:
  - `owner_state=complete`
  - `ccb_offset=0x40`
  - `ccb_read_offset=0x20`
  - `queue_packet_count=0`
  - `sync_complete_seq=0x41`
  - `sync_pending_seq=0x41`
  - `owner_complete_cookie=0x101`
  - `transition_count=2`
  - `validate_count=4`
  - `last_validate_status=0`

Target `dmesg` logged:

- `lab submit transition submit_id=2 state=running completed_packets=0 new_read_offset=0x0 new_write_offset=0x20 new_ccb_offset=0x20 outstanding=1 wrapped=0 complete_seq=0x40 complete_cookie=0x100`
- `lab submit transition submit_id=2 state=complete completed_packets=1 new_read_offset=0x20 new_write_offset=0x20 new_ccb_offset=0x40 outstanding=0 wrapped=0 complete_seq=0x41 complete_cookie=0x101`
- four successful `lab submit validate ... status=0`

### Final post-run target state

- loaded module path:
  - `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- loaded module `srcversion`:
  - `0A250824D4E00B9BBB52C70`
- `lightdm` state:
  - `active`
- direct `poulsbo_sgx_state` snapshot:
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `irq_count=386`

## Validated SGX Issue Model

On May 11, 2026 I extended the submit path again with an explicit validate-only
issue-side queue progression model. The current design note is:

- `docs/28-sgx-issue-model.md`

The new submit control is:

- `issue <submit_id> <owner_state> <issued_packets>`

I rebuilt locally, deployed with the guarded VAIO flow, rebooted successfully,
and validated the new model on real hardware with override module:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 2A252234DCE536B6C9CF9F6`

### Wrap validation

- `QUEUE_MODE=wrap`
- `ISSUE_ENABLE=1`
- `ISSUE_STATE=queued`
- `ISSUE_ISSUED_PACKETS=1`
- initial validated queue:
  - `ccb_read_offset=0x60`
  - `ccb_write_offset=0x0`
  - `queue_packet_count=1`
  - `queue_wrap=0`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x41`
  - `owner_issue_cookie=0x101`
  - `owner_complete_cookie=0x100`
- after `issue queued 1`:
  - `ccb_write_offset=0x20`
  - `queue_packet_count=2`
  - `queue_tail_packet_count=1`
  - `queue_head_packet_count=1`
  - `queue_wrap=1`
  - `sync_pending_seq=0x42`
  - `owner_issue_cookie=0x102`
  - `issue_count=1`
  - `validate_count=2`
  - `last_validate_status=0`
- after the later completion-side transitions:
  - `transition_count=2`
  - `validate_count=5`
  - `last_validate_status=0`

Target `dmesg` logged:

- `lab submit issue submit_id=1 state=queued issued_packets=1 read_offset=0x60 new_write_offset=0x20 outstanding=2 wrapped=1 pending_seq=0x42 issue_cookie=0x102`
- two later successful `lab submit transition ...` lines
- five successful `lab submit validate ... status=0`

### Nowrap validation

- `QUEUE_MODE=nowrap`
- `ISSUE_ENABLE=1`
- `ISSUE_STATE=queued`
- `ISSUE_ISSUED_PACKETS=1`
- initial validated queue:
  - `ccb_read_offset=0x0`
  - `ccb_write_offset=0x20`
  - `queue_packet_count=1`
  - `queue_wrap=0`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x41`
  - `owner_issue_cookie=0x101`
  - `owner_complete_cookie=0x100`
- after `issue queued 1`:
  - `ccb_write_offset=0x0`
  - `queue_packet_count=2`
  - `queue_tail_packet_count=2`
  - `queue_head_packet_count=0`
  - `queue_wrap=0`
  - `sync_pending_seq=0x42`
  - `owner_issue_cookie=0x102`
  - `issue_count=1`
  - `validate_count=2`
  - `last_validate_status=0`
- after the later completion-side transitions:
  - `transition_count=2`
  - `validate_count=5`
  - `last_validate_status=0`

Target `dmesg` logged:

- `lab submit issue submit_id=2 state=queued issued_packets=1 read_offset=0x0 new_write_offset=0x0 outstanding=2 wrapped=0 pending_seq=0x42 issue_cookie=0x102`
- two later successful `lab submit transition ...` lines
- five successful `lab submit validate ... status=0`

### Final post-run target state

- loaded module path:
  - `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- loaded module `srcversion`:
  - `2A252234DCE536B6C9CF9F6`
- `lightdm` state:
  - `active`
- direct `poulsbo_sgx_state` snapshot:
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `irq_count=386`
- live cleanup state:
  - `submits_shown=0`
  - `objects_shown=0`

## Validated SGX Queue Cycle Model

On May 11, 2026 I extended the same submit path again to validate a second
issue-and-retire cycle on one persistent submit object. The current design note
is:

- `docs/29-sgx-queue-cycle-model.md`

This step did not add a new sysfs command. It extended the existing
`issue <submit_id> <owner_state> <issued_packets>` path with explicit refill
tracking and added live submit fields:

- `queue_epoch`
- `last_epoch_jiffies`

I rebuilt locally, deployed with the guarded VAIO flow, rebooted successfully,
and validated the repeated-cycle model on real hardware with override module:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 7E750058A1E31F16F36953E`

### Wrap validation

- `QUEUE_MODE=wrap`
- first cycle:
  - after issue:
    - `queue_packet_count=2`
    - `queue_wrap=1`
    - `sync_pending_seq=0x42`
    - `owner_issue_cookie=0x102`
    - `queue_epoch=0`
    - `issue_count=1`
    - `validate_count=2`
  - after complete:
    - `queue_packet_count=0`
    - `sync_complete_seq=0x42`
    - `sync_pending_seq=0x42`
    - `owner_issue_cookie=0x102`
    - `owner_complete_cookie=0x102`
    - `transition_count=2`
    - `validate_count=4`
- second cycle refill:
  - after refill issue:
    - `queue_packet_count=1`
    - `sync_complete_seq=0x42`
    - `sync_pending_seq=0x43`
    - `owner_issue_cookie=0x103`
    - `owner_complete_cookie=0x102`
    - `queue_epoch=1`
    - `issue_count=2`
    - `validate_count=5`
  - after refill complete:
    - `queue_packet_count=0`
    - `sync_complete_seq=0x43`
    - `sync_pending_seq=0x43`
    - `owner_issue_cookie=0x103`
    - `owner_complete_cookie=0x103`
    - `queue_epoch=1`
    - `issue_count=2`
    - `transition_count=4`
    - `validate_count=7`
    - `last_validate_status=0`
- final validate after probe exit:
  - `validate_count=8`
  - `last_validate_status=0`

Target `dmesg` logged both phases explicitly:

- first-cycle issue with `epoch=0 refill=0`
- refill issue with `epoch=1 refill=1`

### Nowrap validation

- `QUEUE_MODE=nowrap`
- first cycle:
  - after issue:
    - `queue_packet_count=2`
    - `queue_wrap=0`
    - `sync_pending_seq=0x42`
    - `owner_issue_cookie=0x102`
    - `queue_epoch=0`
    - `issue_count=1`
    - `validate_count=2`
  - after complete:
    - `queue_packet_count=0`
    - `transition_count=2`
    - `validate_count=4`
- second cycle refill:
  - after refill issue:
    - `queue_packet_count=1`
    - `sync_pending_seq=0x43`
    - `owner_issue_cookie=0x103`
    - `queue_epoch=1`
    - `issue_count=2`
    - `validate_count=5`
  - after refill complete:
    - `queue_packet_count=0`
    - `sync_complete_seq=0x43`
    - `sync_pending_seq=0x43`
    - `owner_issue_cookie=0x103`
    - `owner_complete_cookie=0x103`
    - `queue_epoch=1`
    - `issue_count=2`
    - `transition_count=4`
    - `validate_count=7`
    - `last_validate_status=0`
- final validate after probe exit:
  - `validate_count=8`
  - `last_validate_status=0`

Target `dmesg` again logged:

- first-cycle issue with `epoch=0 refill=0`
- refill issue with `epoch=1 refill=1`

### Final post-run target state

- loaded module path:
  - `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- loaded module `srcversion`:
  - `7E750058A1E31F16F36953E`
- `lightdm` state:
  - `active`
- direct `poulsbo_sgx_state` snapshot:
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `irq_count=0`
- live cleanup state:
  - `submits_shown=0`
  - `objects_shown=0`

## Validated SGX Multi-Epoch Cycle Model

On May 11, 2026 I extended the same submit path again to validate more than one
refill epoch on one persistent submit object and to couple the current
`queue_epoch` into the status-backed sync and owner blocks. The current design
note is:

- `docs/30-sgx-multi-epoch-cycle-model.md`

This step did not add a new sysfs command. It extended the existing status
model so that:

- the sync block now mirrors `queue_epoch` at `sync_offset + 20`
- the owner block now mirrors `queue_epoch` at `owner_offset + 12`

The helper also gained:

- `REFILL_CYCLES=<n>`

I rebuilt locally, deployed with the guarded VAIO flow, rebooted successfully,
and validated the multi-epoch model on real hardware with override module:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion C94A92B9C66705DD4B23C07`

The hardware validation used:

- `REFILL_CYCLES=3`

### Wrap validation

- `QUEUE_MODE=wrap`
- initial setup still started with:
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x41`
  - `owner_issue_cookie=0x101`
  - `owner_complete_cookie=0x100`
  - `queue_epoch=0`
- after the third refill issue:
  - `sync_complete_seq=0x44`
  - `sync_pending_seq=0x45`
  - `owner_issue_cookie=0x105`
  - `owner_complete_cookie=0x104`
  - `queue_epoch=3`
  - `issue_count=4`
  - `transition_count=6`
  - `validate_count=11`
- after the third refill complete:
  - `queue_packet_count=0`
  - `ccb_offset=0x20`
  - `ccb_read_offset=0x0`
  - `ccb_write_offset=0x0`
  - `sync_complete_seq=0x45`
  - `sync_pending_seq=0x45`
  - `owner_issue_cookie=0x105`
  - `owner_complete_cookie=0x105`
  - `queue_epoch=3`
  - `issue_count=4`
  - `transition_count=8`
  - `validate_count=13`
  - `last_validate_status=0`
- final validate after probe exit:
  - `validate_count=14`
  - `last_validate_status=0`

Target `dmesg` showed:

- initial sync and owner setup with `epoch=0`
- issue and transition progression through:
  - `epoch=0 refill=0`
  - `epoch=1 refill=1`
  - `epoch=2 refill=1`
  - `epoch=3 refill=1`

### Nowrap validation

- `QUEUE_MODE=nowrap`
- initial setup again started with:
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x41`
  - `owner_issue_cookie=0x101`
  - `owner_complete_cookie=0x100`
  - `queue_epoch=0`
- after the third refill issue:
  - `sync_complete_seq=0x44`
  - `sync_pending_seq=0x45`
  - `owner_issue_cookie=0x105`
  - `owner_complete_cookie=0x104`
  - `queue_epoch=3`
  - `issue_count=4`
  - `transition_count=6`
  - `validate_count=11`
- after the third refill complete:
  - `queue_packet_count=0`
  - `ccb_offset=0x40`
  - `ccb_read_offset=0x20`
  - `ccb_write_offset=0x20`
  - `sync_complete_seq=0x45`
  - `sync_pending_seq=0x45`
  - `owner_issue_cookie=0x105`
  - `owner_complete_cookie=0x105`
  - `queue_epoch=3`
  - `issue_count=4`
  - `transition_count=8`
  - `validate_count=13`
  - `last_validate_status=0`
- final validate after probe exit:
  - `validate_count=14`
  - `last_validate_status=0`

Target `dmesg` again showed:

- initial sync and owner setup with `epoch=0`
- issue and transition progression through:
  - `epoch=0 refill=0`
  - `epoch=1 refill=1`
  - `epoch=2 refill=1`
  - `epoch=3 refill=1`

### Final post-run target state

- loaded module path:
  - `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- loaded module `srcversion`:
  - `C94A92B9C66705DD4B23C07`
- `lightdm` state:
  - `active`
- direct `poulsbo_sgx_state` snapshot:
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `irq_count=0`
- live cleanup state:
  - `submits_shown=0`
  - `objects_shown=0`

## Validated SGX Multi-Submit Isolation

On May 11, 2026 I extended the SGX lab path again to validate two persistent
submit objects at once and to avoid `PAGE_SIZE` truncation of the global submit
dump. The current design note is:

- `docs/31-sgx-multi-submit-isolation.md`

This step added:

- `focus <submit_id>`
- `focus clear`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit_state`

so one full submit can be inspected without relying on the truncated
`poulsbo_sgx_submits` list.

I also added:

- `scripts/run-vaio-sgx-multi-submit.sh`

to keep two PF-context submit objects alive at once and inspect them
independently.

### Concurrent validation

The first full multi-submit run on real hardware used override module:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 2716472E418F5F57F7D7D04`

The helper configured:

- submit A:
  - `QUEUE_MODE=wrap`
  - `REFILL_CYCLES=2`
- submit B:
  - `QUEUE_MODE=nowrap`
  - `REFILL_CYCLES=1`

Both submits started with:

- `queue_epoch=0`
- `issue_count=0`
- `transition_count=0`
- `validate_count=1`
- `sync_complete_seq=0x40`
- `sync_pending_seq=0x41`
- `owner_issue_cookie=0x101`
- `owner_complete_cookie=0x100`

Submit A was driven first and reached:

- `queue_epoch=2`
- `issue_count=3`
- `transition_count=6`
- `validate_count=10`
- `last_validate_status=0`
- `sync_complete_seq=0x44`
- `sync_pending_seq=0x44`
- `owner_issue_cookie=0x104`
- `owner_complete_cookie=0x104`

Before submit B was driven, the focused state view showed submit B was still
idle and unchanged:

- `queue_epoch=0`
- `issue_count=0`
- `transition_count=0`
- `validate_count=1`
- `sync_complete_seq=0x40`
- `sync_pending_seq=0x41`
- `owner_issue_cookie=0x101`
- `owner_complete_cookie=0x100`

Submit B was then driven and reached:

- `queue_epoch=1`
- `issue_count=2`
- `transition_count=4`
- `validate_count=7`
- `last_validate_status=0`
- `sync_complete_seq=0x43`
- `sync_pending_seq=0x43`
- `owner_issue_cookie=0x103`
- `owner_complete_cookie=0x103`

After submit B finished, the focused state view showed submit A was still at
its earlier final state:

- `queue_epoch=2`
- `issue_count=3`
- `transition_count=6`
- `validate_count=10`
- `last_validate_status=0`
- `sync_complete_seq=0x44`
- `sync_pending_seq=0x44`
- `owner_issue_cookie=0x104`
- `owner_complete_cookie=0x104`

After all userspace holders exited:

- submit A:
  - `validate_count=11`
  - `last_validate_status=0`
- submit B:
  - `validate_count=8`
  - `last_validate_status=0`

The target remained clean after the run:

- `lightdm=active`
- `fault_count=0`
- `bif_fault=0x000000`
- `capture_failed=0`
- `submits_shown=0`
- `objects_shown=0`

### Focus-clear follow-up

The first focused-view version left `poulsbo_sgx_submit_state` pointing at a
missing submit after cleanup. I fixed that so focus now clears automatically
when the focused submit is dropped.

That follow-up was validated on real hardware with override module:

- `srcversion DDC182564CFB437A4C27B84`

Minimal lifecycle validation:

- create one submit
- `focus <submit_id>`
- confirm `state=present`
- `drop <submit_id>`
- confirm `focused_submit=none`

### Final post-run target state

- loaded module path:
  - `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- loaded module `srcversion`:
  - `CDDD68E97136F8541780DA8`
- `lightdm` state:
  - `active`
- direct `poulsbo_sgx_state` snapshot:
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `irq_count=208`
- direct `poulsbo_sgx_coord_state` snapshot:
  - `issue_ticket_counter=5`
  - `running_ticket_counter=5`
  - `complete_ticket_counter=5`
  - `last_issue_submit_id=2`
  - `last_running_submit_id=2`
  - `last_complete_submit_id=2`
  - `active_running_valid=0`
  - `focused_submit_valid=0`
  - `live_submit_count=0`
- live cleanup state:
  - `submits_shown=0`
  - `focused_submit=none`
  - `objects_shown=0`

### Global submit coordination and per-submit global tickets

I extended the submit path with:

- new sysfs view:
  - `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_coord_state`
- new per-submit fields:
  - `global_issue_ticket`
  - `global_complete_ticket`

Those fields are mirrored into the live backing blocks:

- the sync block now carries the per-submit issue ticket
- the owner block now carries the per-submit completion ticket

The new coordination view reports:

- `issue_ticket_counter`
- `complete_ticket_counter`
- `last_issue_submit_id`
- `last_complete_submit_id`
- `focused_submit_valid`
- `focused_submit_id`
- `live_submit_count`

The multi-submit helper now prints coordination snapshots:

- `COORD_STATE_AFTER_SETUP`
- `COORD_STATE_AFTER_SUBMIT_A`
- `COORD_STATE_AFTER_SUBMIT_B`
- `COORD_STATE_AFTER_DROP`

The first hardware attempt exposed one real validation bug: the initial logic
incorrectly required a completion ticket after any transition, which rejected
the intermediate `running` state. I fixed that so only real completion-state
validation requires a completion ticket.

Final validation on May 11, 2026 used override module:

- `srcversion 1170524E4DF5B69FF27D8E2`

Validated coordination progression:

- after setup:
  - `issue_ticket_counter=0`
  - `complete_ticket_counter=0`
  - `last_issue_submit_id=0`
  - `last_complete_submit_id=0`
  - `focused_submit_id=2`
  - `live_submit_count=2`
- after submit A finished its wrap-mode run with `REFILL_CYCLES=2`:
  - `issue_ticket_counter=3`
  - `complete_ticket_counter=3`
  - `last_issue_submit_id=1`
  - `last_complete_submit_id=1`
- after submit B finished its nowrap-mode run with `REFILL_CYCLES=1`:
  - `issue_ticket_counter=5`
  - `complete_ticket_counter=5`
  - `last_issue_submit_id=2`
  - `last_complete_submit_id=2`
- after drop:
  - `issue_ticket_counter=5`
  - `complete_ticket_counter=5`
  - `focused_submit_valid=0`
  - `focused_submit_id=0`
  - `live_submit_count=0`

Per-submit coordination result:

- submit B stayed idle while submit A ran:
  - `queue_epoch=0`
  - `issue_count=0`
  - `transition_count=0`
  - `global_issue_ticket=0`
  - `global_complete_ticket=0`
  - `validate_count=1`
- submit A final state after its run:
  - `queue_epoch=2`
  - `issue_count=3`
  - `transition_count=6`
  - `global_issue_ticket=3`
  - `global_complete_ticket=3`
  - `validate_count=10`
  - `last_validate_status=0`
- submit B final state after its run:
  - `queue_epoch=1`
  - `issue_count=2`
  - `transition_count=4`
  - `global_issue_ticket=5`
  - `global_complete_ticket=5`
  - `validate_count=7`
  - `last_validate_status=0`
- after all userspace holders exited:
  - submit A:
    - `validate_count=11`
    - `last_validate_status=0`
  - submit B:
    - `validate_count=8`
    - `last_validate_status=0`

### Running-owner arbitration and active-lane exclusivity

I extended the coordination model again so active `running` ownership is now
exclusive across persistent submits.

This step adds:

- new coord-state fields:
  - `running_ticket_counter`
  - `last_running_submit_id`
  - `active_running_valid`
  - `active_running_submit_id`
  - `active_running_ticket`
- new per-submit field:
  - `global_running_ticket`
- owner-block mirroring of that running ticket

The current multi-submit helper now exercises a real contention point:

1. drive submit A into `running`
2. try to drive submit B into `running` before A completes
3. require that write to fail
4. confirm submit B stayed idle
5. confirm the coord node still shows submit A as the active owner

Final validation on May 11, 2026 used override module:

- `srcversion 39837C8555DA32AD66B21C9`

Validated arbitration result:

- after setup:
  - `issue_ticket_counter=0`
  - `running_ticket_counter=0`
  - `complete_ticket_counter=0`
  - `active_running_valid=0`
  - `live_submit_count=2`
- while submit A was active in `running`:
  - `issue_ticket_counter=1`
  - `running_ticket_counter=1`
  - `complete_ticket_counter=0`
  - `last_issue_submit_id=1`
  - `last_running_submit_id=1`
  - `active_running_valid=1`
  - `active_running_submit_id=1`
  - `active_running_ticket=1`
- the attempted conflicting transition for submit B failed as expected with:
  - `Device or resource busy`
- submit B immediately after that failed attempt was still unchanged:
  - `owner_state=queued`
  - `queue_epoch=0`
  - `issue_count=0`
  - `transition_count=0`
  - `global_issue_ticket=0`
  - `global_running_ticket=0`
  - `global_complete_ticket=0`
  - `validate_count=1`
- after submit A completed:
  - `issue_ticket_counter=3`
  - `running_ticket_counter=3`
  - `complete_ticket_counter=3`
  - `active_running_valid=0`
- after submit B completed:
  - `issue_ticket_counter=5`
  - `running_ticket_counter=5`
  - `complete_ticket_counter=5`
  - `last_issue_submit_id=2`
  - `last_running_submit_id=2`
  - `last_complete_submit_id=2`
  - `active_running_valid=0`

Final per-submit running-ticket state after userspace exit:

- submit A:
  - `global_issue_ticket=3`
  - `global_running_ticket=3`
  - `global_complete_ticket=3`
  - `validate_count=11`
  - `last_validate_status=0`
- submit B:
  - `global_issue_ticket=5`
  - `global_running_ticket=5`
  - `global_complete_ticket=5`
  - `validate_count=8`
  - `last_validate_status=0`

### Completion-ticket dependency gating through `ta_dependency`

I extended the validate-only submit model again so the existing `ta_dependency`
field is now a real gate on `queued -> running`.

Current behavior:

- `ta_dependency=0` means no dependency
- if `ta_dependency > global_complete_ticket`, `transition submit_id running 0`
  now fails with `-EAGAIN`
- active-owner arbitration still applies separately and returns `-EBUSY` when
  another submit already owns the active running lane

The multi-submit helper now sets submit B dependency automatically by default:

- `SUBMIT_B_DEPENDENCY_COMPLETE_TICKET=auto`

which resolves to:

- `SUBMIT_A_REFILL_CYCLES + 1`

Final validation on May 11, 2026 used override module:

- `srcversion CDDD68E97136F8541780DA8`

Validated dependency result:

- submit B setup state:
  - `ta_dependency=3`
  - `owner_state=queued`
  - `queue_epoch=0`
  - `global_issue_ticket=0`
  - `global_running_ticket=0`
  - `global_complete_ticket=0`
  - `validate_count=1`
- before submit A ran at all, submit B attempted `running` and failed with:
  - `Resource temporarily unavailable`
- immediately after that dependency failure:
  - submit B was still unchanged and queued
  - coord state stayed:
    - `issue_ticket_counter=0`
    - `running_ticket_counter=0`
    - `complete_ticket_counter=0`
    - `active_running_valid=0`
- later, while submit A was actively running, submit B attempted `running`
  again and failed with:
  - `Device or resource busy`
- after submit A completed:
  - `issue_ticket_counter=3`
  - `running_ticket_counter=3`
  - `complete_ticket_counter=3`
  - `active_running_valid=0`
- submit B then proceeded normally and finished cleanly

Final per-submit dependency-gated state after userspace exit:

- submit A:
  - `ta_dependency=0`
  - `global_issue_ticket=3`
  - `global_running_ticket=3`
  - `global_complete_ticket=3`
  - `validate_count=11`
  - `last_validate_status=0`
- submit B:
  - `ta_dependency=3`
  - `global_issue_ticket=5`
  - `global_running_ticket=5`
  - `global_complete_ticket=5`
  - `validate_count=8`
  - `last_validate_status=0`

### Source-submit dependency binding through `dependency_submit_id`

I extended the validate-only submit model again so the existing
completion-ticket gate can now point at a concrete source submit instead of
only an anonymous global completion threshold.

Current behavior:

- `dependency_submit_id=0` keeps the earlier global completion-ticket behavior
- `depend <submit_id> <dependency_submit_id>` binds one submit to another live
  submit
- if both `ta_dependency` and `dependency_submit_id` are set, then
  `queued -> running` requires the dependency submit's
  `global_complete_ticket` to satisfy `ta_dependency`
- active-owner arbitration still applies separately and still returns `-EBUSY`

The multi-submit helper now sets submit B's dependency source explicitly:

- `depend <submit_b_id> <submit_a_id>`

Final validation on May 11, 2026 used override module:

- `srcversion 05DE875B8F21073BDD04B44`

Validated source-submit dependency result:

- submit B setup state:
  - `ta_dependency=3`
  - `dependency_submit_id=1`
  - `owner_state=queued`
  - `queue_epoch=0`
  - `global_issue_ticket=0`
  - `global_running_ticket=0`
  - `global_complete_ticket=0`
  - `validate_count=1`
- before submit A ran at all, submit B attempted `running` and failed with:
  - `Resource temporarily unavailable`
- immediately after that dependency failure:
  - submit B was still unchanged and queued
  - `dependency_submit_id` was still `1`
  - coord state stayed:
    - `issue_ticket_counter=0`
    - `running_ticket_counter=0`
    - `complete_ticket_counter=0`
    - `active_running_valid=0`
- later, while submit A was actively running, submit B attempted `running`
  again and failed with:
  - `Device or resource busy`
- while submit A was active, submit B still remained unchanged:
  - `dependency_submit_id=1`
  - `owner_state=queued`
  - `global_issue_ticket=0`
  - `global_running_ticket=0`
  - `global_complete_ticket=0`
- after submit A completed:
  - submit A:
    - `ta_dependency=0`
    - `dependency_submit_id=0`
    - `global_issue_ticket=3`
    - `global_running_ticket=3`
    - `global_complete_ticket=3`
    - `validate_count=11`
    - `last_validate_status=0`
- submit B then proceeded normally and finished cleanly:
  - `ta_dependency=3`
  - `dependency_submit_id=1`
  - `global_issue_ticket=5`
  - `global_running_ticket=5`
  - `global_complete_ticket=5`
  - `validate_count=8`
  - `last_validate_status=0`
- confirmed post-run target health:
  - loaded module `srcversion`:
    - `05DE875B8F21073BDD04B44`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `issue_ticket_counter=5`
  - `running_ticket_counter=5`
  - `complete_ticket_counter=5`
  - `active_running_valid=0`
  - `submits_shown=0`
  - `focused_submit=none`
  - `objects_shown=0`
  - target health:
    - `overall=remote_usable`

### Explicit dependency-state block through `depstate`

I extended the validate-only submit model again so dependency state is now
snapshotted into the `status` object instead of only living in submit metadata.

Current behavior:

- `depstate <submit_id> <offset>` writes a four-dword dependency snapshot
- the snapshot now carries:
  - source submit ID
  - required completion ticket
  - observed completion ticket
  - dependency flags
- for the current source-bound gate:
  - `0xb` means `READY | SOURCE_BOUND | GATED`
  - `0xf` means `READY | SOURCE_BOUND | GATED | SATISFIED`
- validation now reads back that block and requires it to match the stored
  dependency snapshot

The multi-submit helper now refreshes submit B dependency state at three points:

- after setup
- after the expected dependency wait failure
- after submit A reaches the required completion ticket

Final validation on May 11, 2026 used override module:

- `srcversion 243EBC2DB6AC1F1149CA3AB`

Validated dependency-block result:

- initial submit B dependency snapshot:
  - `ta_dependency=3`
  - `dependency_submit_id=1`
  - `dependency_offset=0x60`
  - `dependency_snapshot_source_submit_id=1`
  - `dependency_required_ticket=3`
  - `dependency_observed_complete_ticket=0`
  - `dependency_flags=0xb`
  - `dependency_config_count=1`
  - `validate_count=2`
  - `last_validate_status=0`
- before submit A ran, submit B attempted `running` and failed with:
  - `Resource temporarily unavailable`
- after that wait failure, the refreshed dependency block still showed:
  - `dependency_snapshot_source_submit_id=1`
  - `dependency_required_ticket=3`
  - `dependency_observed_complete_ticket=0`
  - `dependency_flags=0xb`
  - `dependency_config_count=2`
  - `validate_count=3`
  - `last_validate_status=0`
- after submit A completed its required path, the refreshed dependency block
  showed:
  - `dependency_snapshot_source_submit_id=1`
  - `dependency_required_ticket=3`
  - `dependency_observed_complete_ticket=3`
  - `dependency_flags=0xf`
  - `dependency_config_count=3`
  - `validate_count=4`
  - `last_validate_status=0`
- submit B still remained queued at that point, so the block explicitly
  captured “dependency satisfied” before submit B was issued
- later submit B still completed cleanly:
  - final post-exit submit B state:
    - `dependency_snapshot_source_submit_id=1`
    - `dependency_required_ticket=3`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0xf`
    - `dependency_config_count=3`
    - `validate_count=11`
    - `last_validate_status=0`
- confirmed post-run target health:
  - loaded module `srcversion`:
    - `243EBC2DB6AC1F1149CA3AB`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `issue_ticket_counter=5`
  - `running_ticket_counter=5`
  - `complete_ticket_counter=5`
  - `active_running_valid=0`
  - `submits_shown=0`
  - `focused_submit=none`
  - `objects_shown=0`
  - target health:
    - `overall=remote_usable`

### Live dependency-state updates after one `depstate` setup

I extended the validate-only submit model again so a configured dependency
block now updates itself when the source submit reaches a new completion
ticket, instead of requiring another explicit `depstate` command.

Current behavior:

- `depstate <submit_id> <offset>` still places the dependency block once
- after that, source-submit `complete` transitions now rewrite the block for
  dependent submits
- the important result is that `dependency_config_count` stays flat after setup
  while the dependency snapshot still changes

The multi-submit helper now proves that by only doing one `depstate` call for
submit B, then relying on `validate` alone afterward.

Final validation on May 11, 2026 used override module:

- `srcversion 6CA61C95B44598D425816B1`

Validated live-update result:

- initial submit B dependency block after setup:
  - `ta_dependency=3`
  - `dependency_submit_id=1`
  - `dependency_offset=0x60`
  - `dependency_snapshot_source_submit_id=1`
  - `dependency_required_ticket=3`
  - `dependency_observed_complete_ticket=0`
  - `dependency_flags=0xb`
  - `dependency_config_count=1`
  - `validate_count=2`
  - `last_validate_status=0`
- before submit A ran, submit B attempted `running` and failed with:
  - `Resource temporarily unavailable`
- after that failure, the helper only ran `validate`, not `depstate`, and
  submit B still showed:
  - `dependency_snapshot_source_submit_id=1`
  - `dependency_required_ticket=3`
  - `dependency_observed_complete_ticket=0`
  - `dependency_flags=0xb`
  - `dependency_config_count=1`
  - `validate_count=3`
  - `last_validate_status=0`
- after submit A completed its required path, the helper again only ran
  `validate`, and submit B then showed:
  - `dependency_snapshot_source_submit_id=1`
  - `dependency_required_ticket=3`
  - `dependency_observed_complete_ticket=3`
  - `dependency_flags=0xf`
  - `dependency_config_count=1`
  - `validate_count=4`
  - `last_validate_status=0`
- submit B still remained queued at that point, so the dependency block now
  reflects a live dependency release before submit B itself runs
- final post-exit submit state stayed clean:
  - submit A:
    - `global_issue_ticket=3`
    - `global_running_ticket=3`
    - `global_complete_ticket=3`
    - `validate_count=11`
    - `last_validate_status=0`
  - submit B:
    - `dependency_snapshot_source_submit_id=1`
    - `dependency_required_ticket=3`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0xf`
    - `dependency_config_count=1`
    - `validate_count=11`
    - `last_validate_status=0`
- confirmed post-run target health:
  - loaded module `srcversion`:
    - `6CA61C95B44598D425816B1`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `issue_ticket_counter=5`
  - `running_ticket_counter=5`
  - `complete_ticket_counter=5`
  - `active_running_valid=0`
  - `submits_shown=0`
  - `focused_submit=none`
  - `objects_shown=0`
  - target health:
    - `overall=remote_usable`

### Dependency source-drop state after one `depstate` setup

- Extended the Ubuntu 5.4 submit lab path so:
  - dependency snapshots now carry an explicit
    `PSB_SGX_LAB_DEPENDENCY_FLAG_SOURCE_MISSING` state
  - `drop <submit_id>` now refreshes dependent dependency blocks after the
    source submit leaves the live submit list
  - validate and dependency-gated `running` transitions now use that
    snapshot-based missing-source state when a dependency block already exists
- Extended the multi-submit helper:
  - `scripts/run-vaio-sgx-multi-submit.sh`
  - it now supports `DROP_SOURCE_BEFORE_RUN=1`
  - the new wrapper
    `scripts/run-vaio-sgx-dependency-drop.sh`
    drives the source-drop case directly
- Wrote the design note in:
  - `docs/38-sgx-dependency-source-drop-state.md`

Validation result on May 11, 2026:

- final validated override module `srcversion`:
  - `662C941AA2B250ACFFEDB1F`
- validated rerun submit IDs:
  - source submit: `3`
  - dependent submit: `4`
- verified initial dependent submit state after one `depstate` setup:
  - `ta_dependency=1`
  - `dependency_submit_id=3`
  - `dependency_offset=0x60`
  - `dependency_snapshot_source_submit_id=3`
  - `dependency_required_ticket=1`
  - `dependency_observed_complete_ticket=0`
  - `dependency_flags=0x0b`
  - `dependency_config_count=1`
  - `validate_count=2`
  - `last_validate_status=0`
- verified dependency wait before the source submit ran:
  - dependent `running` attempt failed with:
    - `Resource temporarily unavailable`
  - a plain `validate` still showed:
    - `dependency_snapshot_source_submit_id=3`
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=0`
    - `dependency_flags=0x0b`
    - `dependency_config_count=1`
    - `validate_count=3`
    - `last_validate_status=0`
- verified state after the source submit was dropped before `running`:
  - the dependent submit then showed:
    - `dependency_snapshot_source_submit_id=3`
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=0`
    - `dependency_flags=0x1b`
    - `dependency_config_count=1`
    - `validate_count=4`
    - `last_validate_status=0`
  - `0x1b` here means:
    - ready + source-bound + gated + source-missing
  - the dependency was still not satisfied
- verified dependency wait after the source drop:
  - dependent `running` attempt again failed with:
    - `Resource temporarily unavailable`
  - a plain `validate` still showed:
    - `dependency_snapshot_source_submit_id=3`
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=0`
    - `dependency_flags=0x1b`
    - `dependency_config_count=1`
    - `validate_count=5`
    - `last_validate_status=0`
- verified final post-exit state:
  - dependent submit:
    - `dependency_snapshot_source_submit_id=3`
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=0`
    - `dependency_flags=0x1b`
    - `dependency_config_count=1`
    - `validate_count=6`
    - `last_validate_status=0`
  - coordination after the source drop:
    - `issue_ticket_counter=0`
    - `running_ticket_counter=0`
    - `complete_ticket_counter=0`
    - `active_running_valid=0`
    - `live_submit_count=1`
- confirmed post-run target health:
  - loaded module `srcversion`:
    - `662C941AA2B250ACFFEDB1F`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `active_running_valid=0`
  - `live_submit_count=0`
  - `submits_shown=0`
  - `objects_shown=0`
  - target health:
    - `overall=remote_usable`

Conclusion:

- the repo now has an explicit missing-source dependency state in addition to
  the earlier waiting and satisfied dependency states
- the dependent submit now stays on the dependency-gated path after the source
  submit is dropped, rather than degrading into a generic missing-submit error
- the next useful dependency question is whether a previously satisfied source
  dependency should stay runnable after that source submit is dropped

### Satisfied source-drop state after dependency release

- Extended the multi-submit helper:
  - `scripts/run-vaio-sgx-multi-submit.sh`
  - it now supports `DROP_SOURCE_AFTER_RELEASE=1`
  - the new wrapper
    `scripts/run-vaio-sgx-dependency-satisfied-drop.sh`
    drives the satisfied-then-drop case directly
- Wrote the design note in:
  - `docs/39-sgx-satisfied-source-drop-state.md`

Validation result on May 11, 2026:

- validated override module `srcversion` remained:
  - `662C941AA2B250ACFFEDB1F`
- validated rerun submit IDs:
  - source submit: `5`
  - dependent submit: `6`
- verified dependency release before dropping the source submit:
  - the dependent submit showed:
    - `dependency_snapshot_source_submit_id=5`
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=1`
    - `dependency_flags=0x0f`
    - `dependency_config_count=1`
    - `validate_count=4`
    - `last_validate_status=0`
- verified state after dropping the already satisfied source submit:
  - the dependent submit then showed:
    - `dependency_snapshot_source_submit_id=5`
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=1`
    - `dependency_flags=0x1f`
    - `dependency_config_count=1`
    - `validate_count=5`
    - `last_validate_status=0`
  - `0x1f` here means:
    - ready + source-bound + gated + satisfied + source-missing
- verified that the dependent submit still ran normally under that state:
  - after `running`:
    - `owner_state=running`
    - `dependency_flags=0x1f`
    - `global_issue_ticket=2`
    - `global_running_ticket=2`
    - `validate_count=6`
  - after `complete`:
    - `owner_state=complete`
    - `dependency_flags=0x1f`
    - `global_issue_ticket=2`
    - `global_running_ticket=2`
    - `global_complete_ticket=2`
    - `validate_count=7`
  - after probe exit:
    - `owner_state=complete`
    - `dependency_flags=0x1f`
    - `queue_packet_count=0`
    - `validate_count=9`
    - `last_validate_status=0`
- verified coordination state:
  - after the satisfied source drop:
    - `issue_ticket_counter=1`
    - `running_ticket_counter=1`
    - `complete_ticket_counter=1`
    - `last_issue_submit_id=5`
    - `last_running_submit_id=5`
    - `last_complete_submit_id=5`
    - `active_running_valid=0`
    - `live_submit_count=1`
  - after submit B completed:
    - `issue_ticket_counter=2`
    - `running_ticket_counter=2`
    - `complete_ticket_counter=2`
    - `last_issue_submit_id=6`
    - `last_running_submit_id=6`
    - `last_complete_submit_id=6`
    - `active_running_valid=0`
    - `live_submit_count=1`
- confirmed post-run target health:
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `submits_shown=0`
  - `objects_shown=0`
  - target health:
    - `overall=remote_usable`

Conclusion:

- the repo now has a validated `satisfied + source-missing` dependency state in
  addition to the earlier waiting and satisfied states
- a dependent submit remains runnable when its source submit is dropped after
  already satisfying the gate
- the next useful dependency question is whether the observed completion ticket
  stays source-specific once unrelated submits complete after the source is
  gone

### Unrelated submit stability after satisfied source drop

- Extended the multi-submit helper:
  - `scripts/run-vaio-sgx-multi-submit.sh`
  - it now supports `RUN_UNRELATED_AFTER_SATISFIED_DROP=1`
  - the new wrapper
    `scripts/run-vaio-sgx-dependency-unrelated-submit.sh`
    drives the three-submit source/drop/unrelated case directly
- Hardened the helper path:
  - it now performs a bounded stale submit/object ID sweep before setup
  - it now derives adopted object IDs from the first adopted object plus the
    known sequential adopt order, instead of relying on a complete
    `poulsbo_sgx_objects` dump once that view truncates
- Wrote the design note in:
  - `docs/40-sgx-unrelated-submit-stability.md`

Validation result on May 11, 2026:

- validated override module `srcversion` remained:
  - `662C941AA2B250ACFFEDB1F`
- validated rerun submit IDs:
  - source submit: `7`
  - dependent submit: `8`
  - unrelated submit: `9`
- verified satisfied source-drop baseline:
  - after source completion:
    - `dependency_snapshot_source_submit_id=7`
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0x0f`
    - `validate_count=4`
  - after dropping that satisfied source:
    - `dependency_snapshot_source_submit_id=7`
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0x1f`
    - `validate_count=5`
- verified state after the unrelated submit completed:
  - coordination moved to:
    - `issue_ticket_counter=4`
    - `running_ticket_counter=4`
    - `complete_ticket_counter=4`
    - `last_issue_submit_id=9`
    - `last_running_submit_id=9`
    - `last_complete_submit_id=9`
  - the dependent submit still showed:
    - `dependency_snapshot_source_submit_id=7`
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0x1f`
    - `dependency_config_count=1`
    - `validate_count=6`
    - `last_validate_status=0`
- verified that the dependent submit still ran normally after that unrelated
  completion:
  - after `running`:
    - `owner_state=running`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0x1f`
    - `global_issue_ticket=5`
    - `global_running_ticket=5`
  - after `complete`:
    - `owner_state=complete`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0x1f`
    - `global_issue_ticket=5`
    - `global_running_ticket=5`
    - `global_complete_ticket=5`
  - after probe exit:
    - `owner_state=complete`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0x1f`
    - `queue_packet_count=0`
    - `validate_count=10`
    - `last_validate_status=0`
- confirmed post-run target health:
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `submits_shown=0`
  - `objects_shown=0`
  - target health:
    - `overall=remote_usable`

Conclusion:

- the satisfied-plus-missing dependency state is now proven to stay
  source-specific even after an unrelated submit completes
- the current dependency snapshot behaves like a pinned per-source fence state,
  not like a rolling alias of the global completion counter
- the next useful dependency question is whether one dependency lane is still
  enough once more than one source submit or more than one wait ticket is
  needed

### Multi-source dependency gating

- Extended the kernel dependency model:
  - `psb_drv.c`
    now carries a second dependency lane with the new sysfs commands:
    `depend2` and `depstate2`
  - the live submit view now exports:
    `dependency2_submit_id`, `dependency2_offset`,
    `dependency2_snapshot_source_submit_id`,
    `dependency2_required_ticket`,
    `dependency2_observed_complete_ticket`, `dependency2_flags`, and
    `dependency2_config_count`
- Extended the target helper flow:
  - `scripts/run-vaio-sgx-multi-submit.sh`
    now supports `RUN_SECONDARY_DEPENDENCY_SOURCE=1`
  - added
    `scripts/run-vaio-sgx-multi-source-dependency.sh`
    as the dedicated three-submit wrapper
- Wrote the design note in:
  - `docs/41-sgx-multi-source-dependency-gating.md`

Validation result on May 11, 2026:

- validated override module `srcversion` is now:
  - `F0B85EC0EA42E7360092BD1`
- validated rerun submit IDs:
  - source submit A:
    - `1`
  - dependent submit B:
    - `2`
  - source submit C:
    - `3`
- verified initial two-lane dependency state on submit B:
  - primary lane:
    - `dependency_submit_id=1`
    - `dependency_observed_complete_ticket=0`
    - `dependency_flags=0x0b`
  - secondary lane:
    - `dependency2_submit_id=3`
    - `dependency2_observed_complete_ticket=0`
    - `dependency2_flags=0x0b`
  - `dependency_config_count=1`
  - `dependency2_config_count=1`
- verified partial release after source submit A completed:
  - primary lane advanced to:
    - `dependency_observed_complete_ticket=1`
    - `dependency_flags=0x0f`
  - secondary lane stayed gated at:
    - `dependency2_observed_complete_ticket=0`
    - `dependency2_flags=0x0b`
  - a `running` attempt on submit B still failed with
    `Resource temporarily unavailable`
- verified full release only after source submit C completed:
  - secondary lane then advanced to:
    - `dependency2_observed_complete_ticket=2`
    - `dependency2_flags=0x0f`
  - primary lane stayed released at:
    - `dependency_observed_complete_ticket=1`
    - `dependency_flags=0x0f`
- verified submit B then ran normally only after both lanes were released:
  - after `running`:
    - `global_issue_ticket=3`
    - `global_running_ticket=3`
    - `validate_count=8`
  - after `complete`:
    - `global_complete_ticket=3`
    - `queue_packet_count=0`
    - `validate_count=9`
  - after probe exit:
    - `dependency_observed_complete_ticket=1`
    - `dependency2_observed_complete_ticket=2`
    - `validate_count=11`
    - `last_validate_status=0`
- verified coordination state progression:
  - after submit A:
    - `issue/running/complete = 1/1/1`
  - after submit C:
    - `issue/running/complete = 2/2/2`
  - after submit B:
    - `issue/running/complete = 3/3/3`
- confirmed post-run target health:
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - target health:
    - `overall=remote_usable`

Conclusion:

- the current validate-only submit model now supports two concrete
  source-submit dependency lanes on one dependent submit
- release of only one lane is not enough to let the dependent submit enter
  `running`
- the next useful dependency question is no longer “more than one source
  submit?” but “do we need more than one wait-ticket threshold per submit?”

### Per-lane ticket gating

- Extended the kernel dependency model:
  - `psb_drv.c`
    now carries a lane-2 ticket field plus the new sysfs command:
    `depticket2`
  - the second dependency lane now uses its own required completion ticket
    instead of always inheriting `ta_dependency`
- Extended the target helper flow:
  - `scripts/run-vaio-sgx-multi-submit.sh`
    now supports `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET`
  - added
    `scripts/run-vaio-sgx-multi-source-ticket-dependency.sh`
    as the dedicated wrapper for the stricter lane-2 wait case
- Wrote the design note in:
  - `docs/42-sgx-per-lane-ticket-gating.md`

Validation result on May 11, 2026:

- validated override module `srcversion` is now:
  - `22E5E2738069346A4881DE7`
- validated rerun submit IDs:
  - source submit A:
    - `1`
  - dependent submit B:
    - `2`
  - source submit C:
    - `3`
- verified explicit per-lane thresholds on submit B after `depticket2` plus
  `depstate2`:
  - primary lane:
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=0`
    - `dependency_flags=0x0b`
  - secondary lane:
    - `dependency2_required_ticket=3`
    - `dependency2_observed_complete_ticket=0`
    - `dependency2_flags=0x0b`
  - `dependency_config_count=1`
  - `dependency2_config_count=1`
  - `validate_count=4`
  - `last_validate_status=0`
- verified split release after submit A completed:
  - primary lane advanced to:
    - `dependency_observed_complete_ticket=1`
    - `dependency_flags=0x0f`
  - secondary lane stayed gated at:
    - `dependency2_observed_complete_ticket=0`
    - `dependency2_required_ticket=3`
    - `dependency2_flags=0x0b`
- verified lane-2 stayed gated after submit C completed only once:
  - submit B then showed:
    - `dependency2_observed_complete_ticket=2`
    - `dependency2_required_ticket=3`
    - `dependency2_flags=0x0b`
  - a `running` attempt on submit B still failed with:
    - `Resource temporarily unavailable`
  - post-wait validate still showed:
    - `validate_count=9`
    - `last_validate_status=0`
- verified lane-2 released only after submit C completed a second time:
  - submit B then showed:
    - `dependency2_observed_complete_ticket=3`
    - `dependency2_required_ticket=3`
    - `dependency2_flags=0x0f`
  - primary lane stayed released at:
    - `dependency_observed_complete_ticket=1`
    - `dependency_flags=0x0f`
  - submit B validate then showed:
    - `validate_count=10`
    - `last_validate_status=0`
- verified submit B then ran normally only after both lanes released:
  - after `running`:
    - `owner_state=running`
    - `global_issue_ticket=4`
    - `global_running_ticket=4`
    - `validate_count=11`
  - after `complete`:
    - `owner_state=complete`
    - `global_complete_ticket=4`
    - `queue_packet_count=0`
    - `validate_count=12`
  - after probe exit:
    - `dependency_observed_complete_ticket=1`
    - `dependency2_observed_complete_ticket=3`
    - `dependency_flags=0x0f`
    - `dependency2_flags=0x0f`
    - `validate_count=14`
    - `last_validate_status=0`
- verified coordination progression:
  - after submit A:
    - `issue/running/complete = 1/1/1`
  - after submit C:
    - `issue/running/complete = 3/3/3`
  - after submit B:
    - `issue/running/complete = 4/4/4`
  - after cleanup:
    - `live_submit_count=0`
    - `focused_submit_valid=0`
- confirmed post-run target state:
  - `overall=remote_usable`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`

Conclusion:

- the dependency model now validates per-lane wait-ticket semantics on real
  hardware
- one lane can be released while another still remains gated on the same
  dependent submit
- the next dependency question is no longer “shared or per-lane threshold?”
  but “are two fixed lanes enough, or do we need a broader dependency array?”

### Third dependency lane

- Extended the kernel dependency model:
  - `psb_drv.c`
    now carries a third dependency lane plus the new sysfs commands:
    `depend3`, `depticket3`, and `depstate3`
  - the focused submit view now exports:
    `dependency3_submit_id`, `dependency3_offset`,
    `dependency3_snapshot_source_submit_id`,
    `dependency3_required_ticket`,
    `dependency3_observed_complete_ticket`, and `dependency3_flags`
- Extended the target helper flow:
  - `scripts/run-vaio-sgx-multi-submit.sh`
    now supports `RUN_TERTIARY_DEPENDENCY_SOURCE=1`
  - added
    `scripts/run-vaio-sgx-third-dependency-lane.sh`
    as the dedicated four-submit wrapper
- Wrote the design note in:
  - `docs/43-sgx-third-dependency-lane.md`

Validation result on May 11, 2026:

- validated override module `srcversion` is now:
  - `157E84B220F2E20EEF2CF58`
- validated rerun submit IDs:
  - source submit A:
    - `1`
  - dependent submit B:
    - `2`
  - source submit C:
    - `3`
  - source submit D:
    - `4`
- verified initial three-lane state on submit B:
  - primary lane:
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=0`
    - `dependency_flags=0x0b`
  - secondary lane:
    - `dependency2_required_ticket=2`
    - `dependency2_observed_complete_ticket=0`
    - `dependency2_flags=0x0b`
  - tertiary lane:
    - `dependency3_required_ticket=3`
    - `dependency3_observed_complete_ticket=0`
    - `dependency3_flags=0x0b`
  - `dependency_config_count=1`
  - `dependency2_config_count=1`
  - `dependency3_config_count=1`
  - `validate_count=6`
  - `last_validate_status=0`
- verified partial release after submit A completed:
  - primary lane advanced to:
    - `dependency_observed_complete_ticket=1`
    - `dependency_flags=0x0f`
  - secondary and tertiary lanes remained gated at:
    - `dependency2_observed_complete_ticket=0`
    - `dependency2_flags=0x0b`
    - `dependency3_observed_complete_ticket=0`
    - `dependency3_flags=0x0b`
- verified split release after submit C completed:
  - secondary lane advanced to:
    - `dependency2_observed_complete_ticket=2`
    - `dependency2_flags=0x0f`
  - tertiary lane still stayed gated at:
    - `dependency3_observed_complete_ticket=0`
    - `dependency3_required_ticket=3`
    - `dependency3_flags=0x0b`
  - submit B still failed `running` with:
    - `Resource temporarily unavailable`
  - post-wait validate still showed:
    - `validate_count=11`
    - `last_validate_status=0`
- verified full release only after submit D completed:
  - tertiary lane advanced to:
    - `dependency3_observed_complete_ticket=3`
    - `dependency3_required_ticket=3`
    - `dependency3_flags=0x0f`
  - primary and secondary lanes stayed released at:
    - `dependency_flags=0x0f`
    - `dependency2_flags=0x0f`
  - submit B validate then showed:
    - `validate_count=12`
    - `last_validate_status=0`
- verified submit B then ran normally only after all three lanes released:
  - after `running`:
    - `owner_state=running`
    - `global_issue_ticket=4`
    - `global_running_ticket=4`
    - `validate_count=13`
  - after `complete`:
    - `owner_state=complete`
    - `global_complete_ticket=4`
    - `queue_packet_count=0`
    - `validate_count=14`
  - after probe exit:
    - `dependency_observed_complete_ticket=1`
    - `dependency2_observed_complete_ticket=2`
    - `dependency3_observed_complete_ticket=3`
    - `dependency_flags=0x0f`
    - `dependency2_flags=0x0f`
    - `dependency3_flags=0x0f`
    - `validate_count=16`
    - `last_validate_status=0`
- verified coordination progression:
  - after submit A:
    - `issue/running/complete = 1/1/1`
  - after submit C:
    - `issue/running/complete = 2/2/2`
  - after submit D:
    - `issue/running/complete = 3/3/3`
  - after submit B:
    - `issue/running/complete = 4/4/4`
  - after cleanup:
    - `live_submit_count=0`
    - `focused_submit_valid=0`
- confirmed post-run target state:
  - `overall=remote_usable`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`

Conclusion:

- the dependency model now validates three concrete source-submit lanes on one
  dependent submit
- the third lane can still keep the dependent submit gated after the first two
  lanes are already released
- the next dependency question is no longer whether the fixed-lane model can
  grow beyond two, but whether further growth should remain fixed-lane or move
  to a broader dependency array

## Current Target Summary

As of May 11, 2026, the VAIO target is in a stable and well-instrumented state
for display-side and 2D-side work:

- the locally built override module boots reliably on the target
- deterministic fbdev-side 2D operations are validated on real hardware
- the SGX/TWOD completion path is observable through current instrumentation
- explicit PF-PD bind / unbind is now validated on a live non-stolen GEM object
- a kernel-owned SGX lab object can now outlive userspace close and drop cleanly
- a PF-context SGX lab submit descriptor now carries a validate-only TA-style
  kick packet and can still validate after userspace exits
- the current submit path now validates an explicit five-object
  `control/pds/use/target/status` topology with class and GPU-use semantics
- the current control object now carries a minimal CCB/control-block layout
  with explicit write/read offsets and readback validation
- the current control object now also carries explicit queue base, size, packet
  size, packet-count, and tail/head split semantics with wrap-aware progression
  validation
- the current status object now carries explicit TA and 3D status backing with
  deterministic readback validation
- the current status object now also carries a validate-only sync block with
  wrapped and linear queue-mode validation on real hardware
- the current status object now also carries a validate-only owner/completion
  block with wrapped and linear queue-mode validation on real hardware
- the current submit path now also validates explicit
  `queued -> running -> complete` transitions with queue, sync, and owner
  state advancement in both queue modes
- the current submit path now also validates explicit issue-side queue growth
  with `write_offset`, `sync_pending_seq`, and `owner_issue_cookie`
  advancement in both queue modes
- the current submit path now also validates one repeated
  `complete -> queued -> running -> complete` refill cycle on the same submit
  object with explicit `queue_epoch` tracking
- the current sync and owner blocks now carry an explicit epoch word and the
  submit path validates three refill epochs on one persistent submit object
  with `REFILL_CYCLES=3`
- the current submit path now also validates two concurrent persistent submits
  with focused per-submit inspection and isolated epoch/sync/owner progression
- the current submit path now also validates explicit global issue and
  completion ordering across two persistent submits through shared driver
  counters and per-submit ticket mirroring
- the current submit path now also validates exclusive active `running`
  ownership across two persistent submits through a third running-owner lane
  with real `-EBUSY` contention on the target
- the current submit path now also validates explicit completion-ticket
  dependency gating through `ta_dependency`, with a real `-EAGAIN` wait before
  the prerequisite completion ticket exists
- the current submit path now also validates explicit source-submit dependency
  binding through `dependency_submit_id`, so submit B can wait on submit A
  specifically while still keeping dependency wait and active-owner contention
  as separate `-EAGAIN` and `-EBUSY` paths
- the current submit path now also validates an explicit dependency-state block
  in the status object through `depstate`, including the real transition from
  `dependency_flags=0xb` to `dependency_flags=0xf` once submit A reaches the
  required completion ticket
- the current submit path now also validates that the dependency block can make
  that `0xb -> 0xf` transition after a single setup-time `depstate` command,
  with no second explicit refresh and with `dependency_config_count` staying
  flat at `1`
- the current submit path now also validates an explicit missing-source
  dependency state after the bound source submit is dropped before satisfying
  the gate, with `dependency_flags=0x1b` and with the dependent submit still
  returning `-EAGAIN` rather than a plain missing-submit failure
- the current submit path now also validates a satisfied-plus-missing
  dependency state after the source submit is dropped only after satisfying the
  gate, with `dependency_flags=0x1f` and with the dependent submit still
  entering `running` and reaching `complete`
- the current submit path now also validates that a later unrelated submit does
  not perturb that pinned `dependency_observed_complete_ticket` or its
  `dependency_flags=0x1f` state
- the current submit path now also validates two concrete source-submit
  dependency lanes on one dependent submit, with submit B staying blocked after
  submit A until submit C releases the second lane
- the current submit path now also validates lane-specific dependency tickets,
  with submit B staying blocked after submit C's first completion because lane
  2 still requires ticket `3`, then releasing only after submit C reaches that
  later ticket
- the current submit path now also validates a third concrete dependency lane,
  with submit B staying blocked after submit C releases lane 2 because lane 3
  is still gated on submit D until ticket `3` is reached
- the same three-lane dependency model is now also validated through the
  indexed sysfs commands `depend_slot`, `depticket_slot`, and `depstate_slot`,
  so the live VAIO path no longer depends on bespoke per-lane submit verbs to
  describe slots `1`, `2`, and `3`
- the indexed dependency model is now also validated through slot `4` with a
  fifth submit on the VAIO, so the live lab path can gate submit B on four
  concrete source submits without adding a `depend4`-style compatibility alias
- the target remains usable enough for repeated remote deploy-and-test cycles

The target is not yet in a render-capable state:

- Xorg is still running through `modesetting` on software `llvmpipe`
- the DRI2 extension exists, but screen 0 never becomes DRI2-capable
- the DRM driver exposes KMS plus dumb buffers, not a render node or a render
  uAPI

That means the project has moved past “is the display path alive?” and is now
squarely at “build the missing SGX render path.”

## Current Validated State

As of the live SGX-state query session on May 11, 2026, the VAIO is remotely
usable again, the indexed dependency-slot clear path is hardware-validated, the
rollback guard is validated in both the healthy-confirmed and automatic-restore
paths, and the bounded SGX lab query, range-query, SGX-state query, object-op,
and submit-op ioctl surfaces are now hardware-validated across:

- object-side mutation:
  - `adopt`
  - `role`
  - `class`
  - `gpu_use`
  - `context`
  - `pf_bind`
  - `pf_unbind`
  - `drop`
- submit-side mutation:
  - `create`
  - `focus`
  - `attach`
  - `detach`
  - `kick`
  - `ccb`
  - `queue`
  - `status`
  - `sync`
  - `owner`
  - `depend-slot`
  - `depticket-slot`
  - `depstate-slot`
  - `depclear-slot`
  - `validate`
  - `issue`
  - `transition`
  - `drop`

The validated live bundle is:

- [logs/vaio-recovery/20260511-065408-depclear-live](/root/Poulsbo/logs/vaio-recovery/20260511-065408-depclear-live)
- [logs/vaio-rollback/20260511-070400-autorestore-live](/root/Poulsbo/logs/vaio-rollback/20260511-070400-autorestore-live)
- [logs/vaio-ioctl/20260511-072823-sgx-query-live](/root/Poulsbo/logs/vaio-ioctl/20260511-072823-sgx-query-live)
- [logs/vaio-ioctl/20260511-074144-sgx-submit-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-074144-sgx-submit-op-live)
- [logs/vaio-ioctl/20260511-074937-sgx-transition-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-074937-sgx-transition-op-live)
- [logs/vaio-ioctl/20260511-080143-sgx-lifecycle-op-live-retry](/root/Poulsbo/logs/vaio-ioctl/20260511-080143-sgx-lifecycle-op-live-retry)
- [logs/vaio-ioctl/20260511-081924-sgx-object-op-live-retry2](/root/Poulsbo/logs/vaio-ioctl/20260511-081924-sgx-object-op-live-retry2)
- [logs/vaio-ioctl/20260511-082821-sgx-object-meta-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-082821-sgx-object-meta-op-live)
- [logs/vaio-ioctl/20260511-083705-sgx-submit-attach-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-083705-sgx-submit-attach-op-live)
- [logs/vaio-ioctl/20260511-084613-sgx-submit-kick-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-084613-sgx-submit-kick-op-live)
- [logs/vaio-ioctl/20260511-085320-sgx-submit-ccb-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-085320-sgx-submit-ccb-op-live)
- [logs/vaio-ioctl/20260511-090033-sgx-submit-queue-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-090033-sgx-submit-queue-op-live)
- [logs/vaio-ioctl/20260511-091155-sgx-submit-status-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-091155-sgx-submit-status-op-live)
- [logs/vaio-ioctl/20260511-091910-sgx-submit-sync-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-091910-sgx-submit-sync-op-live)
- [logs/vaio-ioctl/20260511-092344-sgx-submit-owner-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-092344-sgx-submit-owner-op-live)
- [logs/vaio-ioctl/20260511-100431-sgx-multi-submit-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-100431-sgx-multi-submit-op-live)
- [logs/vaio-ioctl/20260511-101953-sgx-full-object-submit-ioctl-live](/root/Poulsbo/logs/vaio-ioctl/20260511-101953-sgx-full-object-submit-ioctl-live)
- [logs/vaio-ioctl/20260511-103211-sgx-range-query-live](/root/Poulsbo/logs/vaio-ioctl/20260511-103211-sgx-range-query-live)
- [logs/vaio-ioctl/20260511-110011-sgx-query-raw-ioctl-live](/root/Poulsbo/logs/vaio-ioctl/20260511-110011-sgx-query-raw-ioctl-live)
- [logs/vaio-ioctl/20260511-110639-sgx-range-query-direct-live-clean](/root/Poulsbo/logs/vaio-ioctl/20260511-110639-sgx-range-query-direct-live-clean)
- [logs/vaio-ioctl/20260511-112522-sgx-state-query-live](/root/Poulsbo/logs/vaio-ioctl/20260511-112522-sgx-state-query-live)

The target is currently running:

- kernel: `5.4.0-150-generic`
- override module `srcversion`: `85342D8F2786B99DF520799`
- override module size: `382696`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

The latest health probe returned:

- `ping_status=ok`
- `tcp_22_status=open`
- `ssh_pubkey_status=ok`
- `overall=remote_usable`

The completed `depclear_slot` validation is documented in:

- [docs/54-sgx-dependency-slot-clear-validation.md](/root/Poulsbo/docs/54-sgx-dependency-slot-clear-validation.md)

The healthy-boot validation of the guarded deploy rollback path is documented
in:

- [docs/55-vaio-rollback-guard-healthy-boot-validation.md](/root/Poulsbo/docs/55-vaio-rollback-guard-healthy-boot-validation.md)

The failure-path auto-restore validation of that same guarded deploy rollback
path is documented in:

- [docs/56-vaio-rollback-guard-autorestore-validation.md](/root/Poulsbo/docs/56-vaio-rollback-guard-autorestore-validation.md)

The first bounded SGX lab ioctl query validation is documented in:

- [docs/57-sgx-lab-ioctl-query.md](/root/Poulsbo/docs/57-sgx-lab-ioctl-query.md)

The later bounded SGX lab range-query ioctl validation is documented in:

- [docs/73-sgx-lab-range-query-ioctl.md](/root/Poulsbo/docs/73-sgx-lab-range-query-ioctl.md)

The later raw-`ioctl()` query-helper rerun and direct range-query validation
are documented in:

- [docs/74-sgx-lab-query-tools-raw-ioctl-validation.md](/root/Poulsbo/docs/74-sgx-lab-query-tools-raw-ioctl-validation.md)

The later bounded SGX-state query ioctl validation is documented in:

- [docs/75-sgx-lab-sgx-state-query-ioctl.md](/root/Poulsbo/docs/75-sgx-lab-sgx-state-query-ioctl.md)

The later bounded GTT-state query ioctl validation is documented in:

- [docs/76-sgx-lab-gtt-state-query-ioctl.md](/root/Poulsbo/docs/76-sgx-lab-gtt-state-query-ioctl.md)

The later bounded cleanup-op ioctl validation is documented in:

- [docs/77-sgx-lab-cleanup-op-ioctl.md](/root/Poulsbo/docs/77-sgx-lab-cleanup-op-ioctl.md)

The later bounded object-query ioctl validation is documented in:

- [docs/78-sgx-lab-object-query-ioctl.md](/root/Poulsbo/docs/78-sgx-lab-object-query-ioctl.md)

The first bounded SGX lab submit-op ioctl validation is documented in:

- [docs/58-sgx-lab-submit-op-ioctl.md](/root/Poulsbo/docs/58-sgx-lab-submit-op-ioctl.md)

The later `transition` validation on that same submit-op ioctl surface is
documented in:

- [docs/59-sgx-lab-transition-op-ioctl.md](/root/Poulsbo/docs/59-sgx-lab-transition-op-ioctl.md)

The later lifecycle extension on that same submit-op ioctl surface is
documented in:

- [docs/60-sgx-lab-lifecycle-op-ioctl.md](/root/Poulsbo/docs/60-sgx-lab-lifecycle-op-ioctl.md)

The first bounded SGX lab object-op ioctl validation is documented in:

- [docs/61-sgx-lab-object-op-ioctl.md](/root/Poulsbo/docs/61-sgx-lab-object-op-ioctl.md)

The later object `role` / `class` extension on that same object-op ioctl
surface is documented in:

- [docs/62-sgx-lab-object-role-class-ioctl.md](/root/Poulsbo/docs/62-sgx-lab-object-role-class-ioctl.md)

The later submit-slot `attach` / `detach` extension on the submit-op ioctl
surface is documented in:

- [docs/63-sgx-lab-submit-attach-op-ioctl.md](/root/Poulsbo/docs/63-sgx-lab-submit-attach-op-ioctl.md)

The later submit `kick` extension on that same submit-op ioctl surface is
documented in:

- [docs/64-sgx-lab-submit-kick-op-ioctl.md](/root/Poulsbo/docs/64-sgx-lab-submit-kick-op-ioctl.md)

The later submit `ccb` extension on that same submit-op ioctl surface is
documented in:

- [docs/65-sgx-lab-submit-ccb-op-ioctl.md](/root/Poulsbo/docs/65-sgx-lab-submit-ccb-op-ioctl.md)

The later submit `queue` extension on that same submit-op ioctl surface is
documented in:

- [docs/66-sgx-lab-submit-queue-op-ioctl.md](/root/Poulsbo/docs/66-sgx-lab-submit-queue-op-ioctl.md)

The later submit `status` extension on that same submit-op ioctl surface is
documented in:

- [docs/67-sgx-lab-submit-status-op-ioctl.md](/root/Poulsbo/docs/67-sgx-lab-submit-status-op-ioctl.md)

The later submit `sync` extension on that same submit-op ioctl surface is
documented in:

- [docs/68-sgx-lab-submit-sync-op-ioctl.md](/root/Poulsbo/docs/68-sgx-lab-submit-sync-op-ioctl.md)

The later submit `owner` extension on that same submit-op ioctl surface is
documented in:

- [docs/69-sgx-lab-submit-owner-op-ioctl.md](/root/Poulsbo/docs/69-sgx-lab-submit-owner-op-ioctl.md)

The later dependency-slot extension on that same submit-op ioctl surface is
documented in:

- [docs/70-sgx-lab-submit-dependency-slot-op-ioctl.md](/root/Poulsbo/docs/70-sgx-lab-submit-dependency-slot-op-ioctl.md)

That live run is saved in:

- [logs/vaio-ioctl/20260511-093732-sgx-dependency-slot-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-093732-sgx-dependency-slot-op-live)

It validated `DEPENDENCY_SETUP=ioctl` on the real VAIO, including typed
`depend-slot`, `depticket-slot`, `depstate-slot`, and `depclear-slot`.
Submit B cleared dependency slot `4` live, completed before submit E, and still
finished with `dependency4_submit_id=0` and `dependency4_config_count=0` after
submit E later completed. The current target state after that run is still
clean: `overall=remote_usable`, `driver_srcversion=D06E0CB8A26EC70E1FAF340`,
`rollback_pending_present=0`, `rollback_backup_present=1`, `fault_count=0`,
`bif_fault=0x000000`.

The later multi-submit validation with typed submit and dependency setup is
documented in:

- [docs/71-sgx-lab-multi-submit-submit-op-validation.md](/root/Poulsbo/docs/71-sgx-lab-multi-submit-submit-op-validation.md)

The later follow-up that moved the same four-lane scenario onto the object-op,
submit-op, and dependency-slot ioctl paths together is documented in:

- [docs/72-sgx-lab-full-object-submit-ioctl-validation.md](/root/Poulsbo/docs/72-sgx-lab-full-object-submit-ioctl-validation.md)

That full run is saved in:

- [logs/vaio-ioctl/20260511-101953-sgx-full-object-submit-ioctl-live](/root/Poulsbo/logs/vaio-ioctl/20260511-101953-sgx-full-object-submit-ioctl-live)

It validated the same real VAIO scenario with:

- `OBJECT_SETUP=ioctl`
- `SUBMIT_SETUP=ioctl`
- `DEPENDENCY_SETUP=ioctl`
- `DEPENDENCY_INTERFACE=indexed`

It also validated the object-side userspace fix that made the full scenario
reliable: the object-op helper now uses raw `ioctl()` instead of libdrm's
`drmIoctl()`, and the multi-submit runner now rejects `object_resolved_id=0`
during fallback range scanning instead of poisoning the live object map with
invalid IDs. The final clean run still cleared dependency slot `4` live,
completed submit B before submit E, then finished with:

- `objects_shown=0`
- `live_submit_count=0`
- `overall=remote_usable`
- `driver_srcversion=D06E0CB8A26EC70E1FAF340`
- `rollback_pending_present=0`
- `rollback_backup_present=1`
- `fault_count=0`
- `bif_fault=0x000000`

A local auto-resume helper remains available:

```bash
./scripts/wait-vaio-target-ready.sh ./scripts/run-vaio-sgx-dependency-slot-clear.sh
```

Use it if the target becomes blocked again and you want the next validation to
start as soon as the machine returns to `overall=remote_usable`.

For the next risky deploy, prefer the guarded deploy path with
the new boot rollback guard. It now arms a target-side systemd rollback helper
before reboot and only clears it after SSH health returns to
`overall=remote_usable`. It also preserves the previous override module on the
target so rollback can restore the last known-good instrumented build when one
exists.

After recovery, the fastest way to inspect the real target state is now:

```bash
./scripts/show-vaio-gma500-state.sh
```

The guarded deploy path also prints this state automatically after a healthy
post-boot return.

If you need a real target-health timeline during the next recovery or guarded
deploy, use:

```bash
STOP_ON_OVERALL=remote_usable ./scripts/watch-vaio-target-health.sh
```

If you want one coherent recovery-session log bundle instead of several
separate commands, use:

```bash
./scripts/run-vaio-recovery-session.sh
```

For the first risky deploy after recovery, use the guarded deploy session
wrapper instead of running the guarded deploy script bare:

```bash
./scripts/run-vaio-guarded-deploy-session.sh
```

If you want to deliberately exercise the automatic previous-override restore
path again, use:

```bash
./scripts/run-vaio-rollback-autorestore-session.sh
```

To inspect saved recovery and deploy attempts across both session trees, use:

```bash
MAX_SESSIONS=10 ./scripts/summarize-vaio-sessions.sh
```

To run the new typed SGX lab ioctl query validation against the live target,
use:

```bash
./scripts/run-vaio-sgx-ioctl-query.sh
```

To run the new typed SGX lab SGX-state query validation against the live
target, use:

```bash
./scripts/run-vaio-sgx-ioctl-sgx-state.sh
```

To run the new typed SGX lab object-op ioctl validation against the live
target, use:

```bash
./scripts/run-vaio-sgx-ioctl-object-op.sh
```

To run the new typed SGX lab submit-op ioctl validation against the live
target, use:

```bash
./scripts/run-vaio-sgx-ioctl-submit-op.sh
```

The multi-submit dependency-slot-clear scenario is now also validated with the
typed submit-op path active for both submit setup and indexed dependency-slot
mutation:

- [docs/71-sgx-lab-multi-submit-submit-op-validation.md](/root/Poulsbo/docs/71-sgx-lab-multi-submit-submit-op-validation.md)
- [logs/vaio-ioctl/20260511-100431-sgx-multi-submit-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-100431-sgx-multi-submit-op-live)

That run validated:

- `SUBMIT_SETUP=ioctl`
- `DEPENDENCY_SETUP=ioctl`
- `DEPENDENCY_INTERFACE=indexed`

It also validated the userspace-side fix that made this possible: the submit-op
helper now uses raw `ioctl()` instead of libdrm's `drmIoctl()`, so deliberate
dependency-gated `-EAGAIN` transitions return to the runner instead of being
retried indefinitely by libdrm. The final clean run cleared slot `4` live,
completed submit B before submit E, then finished with:

- `objects_shown=0`
- `live_submit_count=0`
- `overall=remote_usable`
- `driver_srcversion=D06E0CB8A26EC70E1FAF340`
- `rollback_pending_present=0`
- `rollback_backup_present=1`
- `fault_count=0`
- `bif_fault=0x000000`

That same scenario is now also validated one step further with the typed
object-op path active for object adoption and configuration as well:

- [docs/72-sgx-lab-full-object-submit-ioctl-validation.md](/root/Poulsbo/docs/72-sgx-lab-full-object-submit-ioctl-validation.md)
- [logs/vaio-ioctl/20260511-101953-sgx-full-object-submit-ioctl-live](/root/Poulsbo/logs/vaio-ioctl/20260511-101953-sgx-full-object-submit-ioctl-live)

That follow-up run validated:

- `OBJECT_SETUP=ioctl`
- `SUBMIT_SETUP=ioctl`
- `DEPENDENCY_SETUP=ioctl`
- `DEPENDENCY_INTERFACE=indexed`

It also validated the userspace-side fixes that made the full object path
reliable in the higher-level runner:

- the object-op helper now uses raw `ioctl()` instead of libdrm's
  `drmIoctl()`
- the runner now rejects `object_resolved_id=0` during fallback range scanning

The final clean run still cleared slot `4` live, completed submit B before
submit E, and finished with:

- `objects_shown=0`
- `live_submit_count=0`
- `overall=remote_usable`
- `driver_srcversion=D06E0CB8A26EC70E1FAF340`
- `rollback_pending_present=0`
- `rollback_backup_present=1`
- `fault_count=0`
- `bif_fault=0x000000`

That same real VAIO scenario is now also validated one step further with typed
live GEM range discovery in the hot path:

- [docs/73-sgx-lab-range-query-ioctl.md](/root/Poulsbo/docs/73-sgx-lab-range-query-ioctl.md)
- [logs/vaio-ioctl/20260511-103211-sgx-range-query-live](/root/Poulsbo/logs/vaio-ioctl/20260511-103211-sgx-range-query-live)

That follow-up run validated:

- `OBJECT_SETUP=ioctl`
- `SUBMIT_SETUP=ioctl`
- `RANGE_DISCOVERY=ioctl`
- `DEPENDENCY_SETUP=ioctl`
- `DEPENDENCY_INTERFACE=indexed`

It also validated the new typed range helper and the runner-side hot-path
change that make the scenario independent of the truncated page-sized
`poulsbo_gtt_state` text dump for fresh GEM range discovery:

- the runner now stages the 32-bit `psb_sgx_lab_range_query` helper to the VAIO
- it enumerates typed live GEM ranges through the bounded range-query ioctl
- it compares before/during live GEM range lists to discover new candidate
  range IDs
- it no longer needs to scrape the sysfs GTT text dump in the validated hot
  path

The final clean run used:

- `RANGE_IDS=10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34`

and still cleared slot `4` live, completed submit B before submit E, and
finished with:

- `objects_shown=0`
- `live_submit_count=0`
- `overall=remote_usable`
- `driver_srcversion=8D7D3CAE32683F70FE739B7`
- `rollback_pending_present=0`
- `rollback_backup_present=1`
- `fault_count=0`
- `bif_fault=0x000000`

The later query-helper follow-up kept the same module deployed and directly
validated the query-side userspace tools after moving them off libdrm retry
behavior:

- [docs/74-sgx-lab-query-tools-raw-ioctl-validation.md](/root/Poulsbo/docs/74-sgx-lab-query-tools-raw-ioctl-validation.md)
- [logs/vaio-ioctl/20260511-110011-sgx-query-raw-ioctl-live](/root/Poulsbo/logs/vaio-ioctl/20260511-110011-sgx-query-raw-ioctl-live)
- [logs/vaio-ioctl/20260511-110639-sgx-range-query-direct-live-clean](/root/Poulsbo/logs/vaio-ioctl/20260511-110639-sgx-range-query-direct-live-clean)

That follow-up run validated:

- `psb_sgx_lab_query` now uses raw `ioctl()` for:
  - `QUERY_COORD`
  - `QUERY_SUBMIT`
- `psb_sgx_lab_range_query` now uses raw `ioctl()` for:
  - `QUERY_RANGE`
- the typed submit/coord helper still matched live sysfs state before and after
  an `issue` step on submit `6`
- the direct range-query runner enumerated seven live GEM ranges by index and
  re-queried the first one by explicit `range_id`

The final clean range run reported:

- `RANGE_QUERY_COUNT=7`
- `range_resolved_id=6 7 8 9 43 44 45`
- `range_matching_count=7`
- `overall=remote_usable`
- `driver_srcversion=8D7D3CAE32683F70FE739B7`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

The next range-query follow-up extended the same bounded path from
`index`/`id` lookup to direct GPU-VA lookup:

- [docs/98-sgx-lab-range-query-gpuva-validation.md](/root/Poulsbo/docs/98-sgx-lab-range-query-gpuva-validation.md)
- [logs/vaio-ioctl/20260511-150817-sgx-range-query-gpuva-live](/root/Poulsbo/logs/vaio-ioctl/20260511-150817-sgx-range-query-gpuva-live)

That run validated:

- new live module:
  - `driver_srcversion=C69057A8C6926335A87C2A9`
  - `override_module_size=385832`
- new range-query mode:
  - `gpuva <gpu_va> [gem]`
- new range-query fields:
  - `range_query_gpu_va`
  - `range_query_gpu_va_offset`
- canonical live VA:
  - `range_query_gpu_va=0x807c0000`
  - `range_query_gpu_va_offset=0x00001000`
  - `range_resolved_id=5`
- SGX ownership transition on that same VA:
  - before adopt:
    - `range_sgx_object_id=0`
  - after typed object adopt:
    - `object_resolved_id=2`
    - `range_sgx_object_id=2`
  - after userspace holder exit:
    - `range_sgx_object_id=2`
  - after typed object drop:
    - `RANGE_QUERY_BY_GPUVA_AFTER_DROP_RC=0`
    - `range_sgx_object_id=0`

On this warm target the canonical run did not find a fresh probe-created GEM
range, so it intentionally fell back to a persistent baseline range:

- `BASELINE_RANGE_IDS=5 7 8 9 16 17 18`
- `FRESH_RANGE_ID=5`
- `FRESH_RANGE_IS_NEW=0`

That means the final post-drop GPU-VA query still resolved the baseline range
itself, but SGX ownership on that VA was correctly gone.

The next GPU-VA follow-up carried SGX object semantics through the same typed
range query result:

- [docs/99-sgx-lab-range-query-gpuva-metadata-validation.md](/root/Poulsbo/docs/99-sgx-lab-range-query-gpuva-metadata-validation.md)
- [logs/vaio-ioctl/20260511-151741-sgx-range-query-gpuva-metadata-live](/root/Poulsbo/logs/vaio-ioctl/20260511-151741-sgx-range-query-gpuva-metadata-live)

That run validated:

- new live module:
  - `driver_srcversion=1DAAA2E3085BB97D83AA3D7`
  - `override_module_size=385984`
- new typed range-query fields:
  - `range_sgx_object_role`
  - `range_sgx_object_role_name`
  - `range_sgx_object_class`
  - `range_sgx_object_class_name`
  - `range_sgx_object_gpu_use`
  - `range_sgx_object_gpu_use_name`
  - `range_sgx_object_expected_context`
  - `range_sgx_object_expected_context_name`
- canonical live VA:
  - `range_query_gpu_va=0x807c0000`
  - `range_query_gpu_va_offset=0x00001000`
  - `range_resolved_id=6`
- semantic transition on that same VA:
  - before adopt:
    - `range_sgx_object_id=0`
    - `range_sgx_object_role_name=generic`
    - `range_sgx_object_class_name=generic`
    - `range_sgx_object_gpu_use_name=generic`
    - `range_sgx_object_expected_context_name=auto`
  - after typed adopt plus metadata setup:
    - `object_resolved_id=1`
    - `range_sgx_object_id=1`
    - `range_sgx_object_role_name=control`
    - `range_sgx_object_class_name=packet`
    - `range_sgx_object_gpu_use_name=control_ccb`
    - `range_sgx_object_expected_context_name=pf`
  - after userspace holder exit:
    - `range_sgx_object_id=1`
    - `range_sgx_object_role_name=control`
    - `range_sgx_object_class_name=packet`
    - `range_sgx_object_gpu_use_name=control_ccb`
    - `range_sgx_object_expected_context_name=pf`
  - after typed drop:
    - `RANGE_QUERY_BY_GPUVA_AFTER_DROP_RC=0`
    - `range_sgx_object_id=0`
    - `range_sgx_object_role_name=generic`
    - `range_sgx_object_class_name=generic`
    - `range_sgx_object_gpu_use_name=generic`
    - `range_sgx_object_expected_context_name=auto`

The next GPU-VA follow-up carried live SGX submit-reference state through that
same typed range query result:

- [docs/100-sgx-lab-range-query-gpuva-submit-ref-validation.md](/root/Poulsbo/docs/100-sgx-lab-range-query-gpuva-submit-ref-validation.md)
- [logs/vaio-ioctl/20260511-152849-sgx-range-query-gpuva-submit-ref-live](/root/Poulsbo/logs/vaio-ioctl/20260511-152849-sgx-range-query-gpuva-submit-ref-live)

That run validated:

- new live module:
  - `driver_srcversion=AEB8DB73A3D6849EE3A59FA`
  - `override_module_size=386328`
- new typed range-query fields:
  - `range_sgx_submit_ref_count`
  - `range_sgx_first_submit_id`
  - `range_sgx_first_submit_slot`
  - `range_sgx_first_submit_slot_name`
- canonical live VA:
  - `range_query_gpu_va=0x807c0000`
  - `range_query_gpu_va_offset=0x00001000`
  - `range_resolved_id=7`
- GPU-VA submit-reference transition on that same VA:
  - before adopt:
    - `range_sgx_submit_ref_count=0`
    - `range_sgx_first_submit_id=0`
    - `range_sgx_first_submit_slot_name=none`
  - after typed adopt plus metadata setup:
    - `object_resolved_id=2`
    - `range_sgx_object_role_name=control`
    - `range_sgx_object_class_name=packet`
    - `range_sgx_object_gpu_use_name=control_ccb`
    - `range_sgx_object_expected_context_name=pf`
    - `range_sgx_submit_ref_count=0`
  - after typed submit create plus `control` attach:
    - `submit_resolved_id=2`
    - `range_sgx_submit_ref_count=1`
    - `range_sgx_first_submit_id=2`
    - `range_sgx_first_submit_slot_name=control`
  - after userspace holder exit:
    - `range_sgx_submit_ref_count=1`
    - `range_sgx_first_submit_id=2`
    - `range_sgx_first_submit_slot_name=control`
  - after typed submit drop:
    - `range_sgx_submit_ref_count=0`
    - `range_sgx_first_submit_id=0`
    - `range_sgx_first_submit_slot_name=none`
    - object semantics still:
      - `control / packet / control_ccb / pf`
  - after typed object drop:
    - `RANGE_QUERY_BY_GPUVA_AFTER_DROP_RC=0`
    - `range_sgx_object_id=0`
    - `range_sgx_object_role_name=generic`
    - `range_sgx_object_class_name=generic`
    - `range_sgx_object_gpu_use_name=generic`
    - `range_sgx_object_expected_context_name=auto`
    - `range_sgx_submit_ref_count=0`
    - `range_sgx_first_submit_slot_name=none`

Like the two earlier GPU-VA bundles, this canonical run stayed on a persistent
baseline GEM range rather than a fresh probe-created one:

- `BASELINE_RANGE_IDS=7 8 9 13 14 15`
- `FRESH_RANGE_ID=7`
- `FRESH_RANGE_IS_NEW=0`

So the final post-drop GPU-VA query still resolved the underlying baseline
range, but both the SGX object semantics and the live SGX submit-reference
state were correctly gone.

The next GPU-VA range-query follow-up extended that same bounded path so it can
select one concrete submit-reference ordinal directly from the range-query
family:

- [docs/107-sgx-lab-range-query-submit-match-validation.md](/root/Poulsbo/docs/107-sgx-lab-range-query-submit-match-validation.md)
- [logs/vaio-ioctl/20260511-165702-sgx-range-query-submit-match-live](/root/Poulsbo/logs/vaio-ioctl/20260511-165702-sgx-range-query-submit-match-live)

That run validated:

- new live module:
  - `driver_srcversion=933E555C1A18400A2CF86AE`
  - `override_module_size=387664`
- new typed range-query request/result fields:
  - `range_query_submit_match_index`
  - `range_sgx_resolved_submit_found`
  - `range_sgx_resolved_submit_id`
  - `range_sgx_resolved_submit_slot`
  - `range_sgx_resolved_submit_slot_name`
  - `range_sgx_resolved_submit_match_index`
- canonical live VA:
  - `range_query_gpu_va=0x807c0000`
  - `range_query_gpu_va_offset=0x00001000`
  - `range_resolved_id=7`
- shared-object two-submit transition on that same VA:
  - while submit `1` and submit `2` both reference the same control object:
    - plain `gpuva`:
      - `range_sgx_submit_ref_count=2`
      - `range_sgx_first_submit_id=1`
      - `range_sgx_first_submit_slot_name=control`
    - `gpuva-submit 0`:
      - `range_sgx_resolved_submit_found=1`
      - `range_sgx_resolved_submit_id=1`
      - `range_sgx_resolved_submit_slot_name=control`
      - `range_sgx_resolved_submit_match_index=0`
    - `gpuva-submit 1`:
      - `range_sgx_resolved_submit_found=1`
      - `range_sgx_resolved_submit_id=2`
      - `range_sgx_resolved_submit_slot_name=control`
      - `range_sgx_resolved_submit_match_index=1`
  - after userspace holder exit:
    - `range_sgx_submit_ref_count=2`
    - `gpuva-submit 1` still resolved submit `2`
  - after primary submit drop:
    - plain `gpuva`:
      - `range_sgx_submit_ref_count=1`
      - `range_sgx_first_submit_id=2`
      - `range_sgx_first_submit_slot_name=control`
    - `gpuva-submit 0`:
      - `range_sgx_resolved_submit_id=2`
      - `range_sgx_resolved_submit_match_index=0`
    - `gpuva-submit 1`:
      - `range_sgx_resolved_submit_found=0`
      - `range_sgx_resolved_submit_id=0`
  - after final submit drop:
    - `range_sgx_submit_ref_count=0`
    - `range_sgx_first_submit_id=0`
    - `range_sgx_first_submit_slot_name=none`
  - after final object drop:
    - `RANGE_QUERY_BY_GPUVA_AFTER_DROP_RC=0`
    - `range_sgx_object_id=0`
    - `range_sgx_submit_ref_count=0`

That gives the range-query family its own shared-object submit-selection path,
instead of forcing callers to jump to `QUERY_SUBMIT gpuva-match` whenever one
GPU VA is referenced by more than one live submit.

The next read-side GPU-VA follow-up extended the bounded submit query itself so
one typed lookup can now resolve a live submit directly from that same VA:

- [docs/101-sgx-lab-submit-query-gpuva-validation.md](/root/Poulsbo/docs/101-sgx-lab-submit-query-gpuva-validation.md)
- [logs/vaio-ioctl/20260511-155105-sgx-submit-query-gpuva-live](/root/Poulsbo/logs/vaio-ioctl/20260511-155105-sgx-submit-query-gpuva-live)

That run validated:

- new live module:
  - `driver_srcversion=CA6AF1E225E6F5B90E66C7D`
  - `override_module_size=386888`
- new typed submit-query fields:
  - `submit_query_gpu_va`
  - `submit_resolved_slot`
  - `submit_resolved_slot_name`
  - `submit_resolved_object_id`
  - `submit_query_gpu_va_offset`
- canonical live VA:
  - `CONTROL_QUERY_GPU_VA=0x807c0000`
  - `CONTROL_QUERY_OFFSET=0x1000`
- live attached-submit transition on that same VA:
  - staged ranges:
    - `RANGE_IDS=5 7 9 40 41`
  - staged adopted objects:
    - `OBJECT_IDS=31 32 33 34 35`
  - staged submit:
    - `SUBMIT_ID=7`
  - while the submit exists:
    - `submit_resolved_id=7`
    - `submit_resolved_slot_name=control`
    - `submit_resolved_object_id=31`
    - `submit_query_gpu_va_offset=0x00001000`
    - `submit_validate_count=0`
    - `submit_issue_count=0`
    - `submit_transition_count=0`
  - after userspace holder exit:
    - the same `submit / slot / object / offset` tuple still resolved cleanly
  - after typed submit drop:
    - `coord_live_submit_count=0`
    - `submit_ioctl_ret=-1 errno=2`
  - after typed object cleanup:
    - `object_result_count=5`

This canonical run intentionally used an attached-but-unvalidated submit. That
was enough to prove direct GPU-VA submit ownership without pulling in the later
kick, queue, status, sync, and owner block setup just to satisfy the validate
path.

The next GPU-VA submit-query follow-up made that same direct lookup fully
self-contained by returning the backing range ID and type too:

- [docs/102-sgx-lab-submit-query-gpuva-range-validation.md](/root/Poulsbo/docs/102-sgx-lab-submit-query-gpuva-range-validation.md)
- [logs/vaio-ioctl/20260511-160747-sgx-submit-query-gpuva-range-live](/root/Poulsbo/logs/vaio-ioctl/20260511-160747-sgx-submit-query-gpuva-range-live)

That run validated:

- new live module:
  - `driver_srcversion=9864A6D5176BBB851C0B51E`
  - `override_module_size=387080`
- new typed submit-query fields:
  - `submit_resolved_range_id`
  - `submit_resolved_range_type`
  - `submit_resolved_range_type_name`
- canonical live VA:
  - `CONTROL_QUERY_GPU_VA=0x807c0000`
  - `CONTROL_QUERY_OFFSET=0x1000`
- live attached-submit transition on that same VA:
  - staged ranges:
    - `RANGE_IDS=7 9 10 11 12`
  - staged adopted objects:
    - `OBJECT_IDS=1 2 3 4 5`
  - staged submit:
    - `SUBMIT_ID=1`
  - while the submit exists:
    - `submit_resolved_id=1`
    - `submit_resolved_slot_name=control`
    - `submit_resolved_object_id=1`
    - `submit_resolved_range_id=7`
    - `submit_resolved_range_type_name=gem`
    - `submit_query_gpu_va_offset=0x00001000`
  - after userspace holder exit:
    - the same `submit / slot / object / range / type / offset` tuple still
      resolved cleanly
  - after typed submit drop:
    - `SUBMIT_QUERY_GPUVA_AFTER_DROP_RC=3`
    - `submit_ioctl_ret=-1 errno=2`
  - after typed object cleanup:
    - `object_result_count=5`

The next GPU-VA submit-query follow-up made that same direct lookup return the
matched object’s semantic metadata too:

- [docs/103-sgx-lab-submit-query-gpuva-object-validation.md](/root/Poulsbo/docs/103-sgx-lab-submit-query-gpuva-object-validation.md)
- [logs/vaio-ioctl/20260511-161543-sgx-submit-query-gpuva-object-live](/root/Poulsbo/logs/vaio-ioctl/20260511-161543-sgx-submit-query-gpuva-object-live)

That run validated:

- new live module:
  - `driver_srcversion=19426C7ED791DCAAD913C11`
  - `override_module_size=387144`
- new typed submit-query fields:
  - `submit_resolved_object_role`
  - `submit_resolved_object_role_name`
  - `submit_resolved_object_class`
  - `submit_resolved_object_class_name`
  - `submit_resolved_object_gpu_use`
  - `submit_resolved_object_gpu_use_name`
  - `submit_resolved_object_expected_context`
  - `submit_resolved_object_expected_context_name`
- canonical live VA:
  - `CONTROL_QUERY_GPU_VA=0x807c0000`
  - `CONTROL_QUERY_OFFSET=0x1000`
- live attached-submit transition on that same VA:
  - staged ranges:
    - `RANGE_IDS=7 9 10 11 12`
  - staged adopted objects:
    - `OBJECT_IDS=1 2 3 4 5`
  - staged submit:
    - `SUBMIT_ID=1`
  - while the submit exists:
    - `submit_resolved_id=1`
    - `submit_resolved_slot_name=control`
    - `submit_resolved_object_id=1`
    - `submit_resolved_range_id=7`
    - `submit_resolved_range_type_name=gem`
    - `submit_resolved_object_role_name=control`
    - `submit_resolved_object_class_name=packet`
    - `submit_resolved_object_gpu_use_name=control_ccb`
    - `submit_resolved_object_expected_context_name=pf`
    - `submit_query_gpu_va_offset=0x00001000`
  - after userspace holder exit:
    - the same `submit / slot / object / range / role / class / gpu_use /
      context / offset` tuple still resolved cleanly
  - after typed submit drop:
    - `SUBMIT_QUERY_GPUVA_AFTER_DROP_RC=3`
    - `submit_ioctl_ret=-1 errno=2`
  - after typed object cleanup:
    - `object_result_count=5`

The next direct GPU-VA submit-query follow-up removed the last ambiguity in
that path by reporting how many live submits currently match the same VA:

- [docs/104-sgx-lab-submit-query-gpuva-multi-ref-validation.md](/root/Poulsbo/docs/104-sgx-lab-submit-query-gpuva-multi-ref-validation.md)
- [logs/vaio-ioctl/20260511-162454-sgx-submit-query-gpuva-multi-ref-live](/root/Poulsbo/logs/vaio-ioctl/20260511-162454-sgx-submit-query-gpuva-multi-ref-live)

That run validated:

- new live module:
  - `driver_srcversion=11920D5153C981A7E8A264A`
  - `override_module_size=387336`
- new typed submit-query field:
  - `submit_matching_submit_count`
- canonical live VA:
  - `CONTROL_QUERY_GPU_VA=0x807c0000`
  - `CONTROL_QUERY_OFFSET=0x1000`
- shared-object direct submit-query handoff on that same VA:
  - staged ranges:
    - `RANGE_IDS=7 9 10 11 12`
  - staged adopted objects:
    - `OBJECT_IDS=1 2 3 4 5`
  - staged submits:
    - `SUBMIT_ID=1`
    - `SECOND_SUBMIT_ID=2`
  - while both submits exist:
    - `submit_resolved_id=1`
    - `submit_resolved_slot_name=control`
    - `submit_resolved_object_id=1`
    - `submit_resolved_range_id=7`
    - `submit_resolved_range_type_name=gem`
    - `submit_resolved_object_role_name=control`
    - `submit_resolved_object_class_name=packet`
    - `submit_resolved_object_gpu_use_name=control_ccb`
    - `submit_resolved_object_expected_context_name=pf`
    - `submit_query_gpu_va_offset=0x00001000`
    - `submit_matching_submit_count=2`
  - after userspace holder exit:
    - submit A still resolved first
    - `submit_matching_submit_count=2`
  - after typed submit-A drop:
    - the same GPU VA resolved submit B instead:
      - `submit_resolved_id=2`
      - `submit_matching_submit_count=1`
  - after typed submit-B drop:
    - `SUBMIT_QUERY_GPUVA_AFTER_DROP_RC=3`
    - `submit_ioctl_ret=-1 errno=2`
  - after typed object cleanup:
    - `object_result_count=5`

This closes the remaining ambiguity in the direct GPU-VA submit-query path:
the same bounded read can now prove both shared ownership and later handoff
without first going through `QUERY_RANGE`.

The next direct GPU-VA submit-query follow-up made that shared-ownership state
explicitly selectable by match ordinal:

- [docs/105-sgx-lab-submit-query-gpuva-match-validation.md](/root/Poulsbo/docs/105-sgx-lab-submit-query-gpuva-match-validation.md)
- [logs/vaio-ioctl/20260511-163739-sgx-submit-query-gpuva-match-live](/root/Poulsbo/logs/vaio-ioctl/20260511-163739-sgx-submit-query-gpuva-match-live)

That run validated:

- new live module:
  - `driver_srcversion=0F9AE3730BF455D57D74E17`
  - `override_module_size=387528`
- new typed submit-query selector and fields:
  - `DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_GPU_VA_MATCH_INDEX`
  - `submit_query_match_index`
  - `submit_resolved_match_index`
- canonical live VA:
  - `CONTROL_QUERY_GPU_VA=0x807c0000`
  - `CONTROL_QUERY_OFFSET=0x1000`
- shared-object direct submit-query selection on that same VA:
  - staged ranges:
    - `RANGE_IDS=7 9 10 11 12`
  - staged adopted objects:
    - `OBJECT_IDS=1 2 3 4 5`
  - staged submits:
    - `SUBMIT_ID=1`
    - `SECOND_SUBMIT_ID=2`
  - while both submits exist:
    - plain `gpuva` resolved the first match:
      - `submit_resolved_id=1`
      - `submit_matching_submit_count=2`
      - `submit_resolved_match_index=0`
    - `gpuva-match 0` resolved submit A:
      - `submit_query_match_index=0`
      - `submit_resolved_id=1`
      - `submit_matching_submit_count=2`
      - `submit_resolved_match_index=0`
    - `gpuva-match 1` resolved submit B:
      - `submit_query_match_index=1`
      - `submit_resolved_id=2`
      - `submit_matching_submit_count=2`
      - `submit_resolved_match_index=1`
  - after userspace holder exit:
    - `gpuva-match 1` still resolved submit B with:
      - `submit_matching_submit_count=2`
      - `submit_resolved_match_index=1`
  - after typed submit-A drop:
    - plain `gpuva` and `gpuva-match 0` both resolved submit B with:
      - `submit_resolved_id=2`
      - `submit_matching_submit_count=1`
      - `submit_resolved_match_index=0`
    - `gpuva-match 1` failed cleanly:
      - `SUBMIT_QUERY_GPUVA_MATCH1_AFTER_PRIMARY_DROP_RC=3`
      - `submit_ioctl_ret=-1 errno=2`
  - after typed submit-B drop:
    - plain `gpuva` also failed cleanly:
      - `SUBMIT_QUERY_GPUVA_AFTER_DROP_RC=3`
      - `submit_ioctl_ret=-1 errno=2`
  - after typed object cleanup:
    - `object_result_count=5`

This extends the direct GPU-VA submit-query path from passive ambiguity
reporting into active disambiguation: the same bounded read path can now count
matches and select a specific one.

The next wrapper-level follow-up turned that same typed path into a canonical
enumeration flow for one shared GPU VA:

- [docs/106-sgx-lab-submit-query-gpuva-list-validation.md](/root/Poulsbo/docs/106-sgx-lab-submit-query-gpuva-list-validation.md)
- [logs/vaio-ioctl/20260511-164757-sgx-submit-query-gpuva-list-live](/root/Poulsbo/logs/vaio-ioctl/20260511-164757-sgx-submit-query-gpuva-list-live)

That run validated:

- no new kernel deploy was needed:
  - `driver_srcversion=0F9AE3730BF455D57D74E17`
  - `override_module_size=387528`
- new wrapper-level list mode:
  - `ENUMERATE_MATCHES=1`
  - [run-vaio-sgx-ioctl-submit-query-gpuva-list.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-query-gpuva-list.sh)
- canonical live shared-VA enumeration:
  - staged submits:
    - `SUBMIT_ID=3`
    - `SECOND_SUBMIT_ID=4`
  - while both submits exist:
    - `SUBMIT_QUERY_GPUVA_LIST_LIVE_0`
      - `submit_resolved_id=3`
      - `submit_matching_submit_count=2`
      - `submit_resolved_match_index=0`
    - `SUBMIT_QUERY_GPUVA_LIST_LIVE_1`
      - `submit_resolved_id=4`
      - `submit_matching_submit_count=2`
      - `submit_resolved_match_index=1`
  - after userspace holder exit:
    - `SUBMIT_QUERY_GPUVA_LIST_POST_PROBE_EXIT_0`
      - `submit_resolved_id=3`
    - `SUBMIT_QUERY_GPUVA_LIST_POST_PROBE_EXIT_1`
      - `submit_resolved_id=4`
  - after typed primary-submit drop:
    - `SUBMIT_QUERY_GPUVA_LIST_AFTER_PRIMARY_DROP_0`
      - `submit_resolved_id=4`
      - `submit_matching_submit_count=1`
      - `submit_resolved_match_index=0`

This gives the repo a canonical typed “list all live matches for this VA”
validation path on top of the lower-level `gpuva-match` selector.

The next read-side follow-up added a bounded typed SGX snapshot query and
validated it directly against the live `poulsbo_sgx_state` path:

- [docs/75-sgx-lab-sgx-state-query-ioctl.md](/root/Poulsbo/docs/75-sgx-lab-sgx-state-query-ioctl.md)
- [logs/vaio-ioctl/20260511-112522-sgx-state-query-live](/root/Poulsbo/logs/vaio-ioctl/20260511-112522-sgx-state-query-live)

That run validated:

- `QUERY_SGX_STATE` with direct capture mode
- cached SGX-state reads after an ioctl-triggered capture
- cached SGX-state reads after a sysfs-triggered capture

The live run showed:

- before the 2D sequence:
  - `sgx_state_reason=ioctl-sgx-state`
  - `sgx_state_sequence=1`
  - `sgx_state_irq_count=0`
  - `sgx_state_fault_count=0`
  - `sgx_state_twod_blit_status=0x0`
- after the sysfs pre-read:
  - `sgx_state_reason=poulsbo_sgx_state`
  - `sgx_state_sequence=2`
- after the 2D sequence:
  - `sgx_state_reason=ioctl-sgx-state`
  - `sgx_state_sequence=14`
  - `sgx_state_irq_count=7`
  - `sgx_state_fault_count=0`
  - `sgx_state_twod_blit_status=0xe`
- after the sysfs post-read:
  - `sgx_state_reason=poulsbo_sgx_state`
  - `sgx_state_sequence=15`
  - `sgx_state_irq_count=7`
  - `sgx_state_fault_count=0`
  - `sgx_state_twod_blit_status=0xe`

The final target state after that run is still clean:

- `overall=remote_usable`
- `driver_srcversion=85342D8F2786B99DF520799`
- `rollback_pending_present=0`
- `rollback_backup_present=1`
- `fault_count=0`
- `bif_fault=0x000000`

The next read-side follow-up added a bounded typed GTT/MMU state query and
validated it directly against the live `poulsbo_gtt_state` path:

- [docs/76-sgx-lab-gtt-state-query-ioctl.md](/root/Poulsbo/docs/76-sgx-lab-gtt-state-query-ioctl.md)
- [logs/vaio-ioctl/20260511-110215-gtt-state-query-live](/root/Poulsbo/logs/vaio-ioctl/20260511-110215-gtt-state-query-live)

That run validated:

- `QUERY_GTT_STATE` against the existing global GTT/MMU sysfs dump
- typed idle readout before any held GEM objects
- typed live growth while three dumb GEM ranges remained mapped
- typed cleanup after those ranges were released

The live run showed:

- before the held ranges:
  - `gtt_state_mmu_flush_count=2`
  - `gtt_state_mmu_insert_pages_count=2`
  - `gtt_state_mmu_remove_pages_count=0`
  - `gtt_state_range_total_count=7`
  - `gtt_state_range_gem_count=4`
  - `gtt_state_range_mmapping_count=2`
- during the held ranges:
  - `gtt_state_mmu_flush_count=5`
  - `gtt_state_mmu_insert_pages_count=5`
  - `gtt_state_mmu_remove_pages_count=0`
  - `gtt_state_range_total_count=10`
  - `gtt_state_range_gem_count=7`
  - `gtt_state_range_mmapping_count=5`
- after release:
  - `gtt_state_mmu_flush_count=8`
  - `gtt_state_mmu_insert_pages_count=5`
  - `gtt_state_mmu_remove_pages_count=3`
  - `gtt_state_range_total_count=7`
  - `gtt_state_range_gem_count=4`
  - `gtt_state_range_mmapping_count=2`

The stable layout fields stayed unchanged across all three phases:

- `gtt_state_gtt_start=0x94280000`
- `gtt_state_gatt_start=0x80000000`
- `gtt_state_mmu_gatt_start=0xe0000000`
- `gtt_state_gatt_free_offset=0xe0800000`

The final target state after that run is still clean:

- `overall=remote_usable`
- `driver_srcversion=8CB35627334C3560115D1F6`
- `override_module_size=383644`
- `rollback_pending_present=0`
- `rollback_backup_present=1`
- `fault_count=0`
- `bif_fault=0x000000`

The next control-surface follow-up added typed cleanup verbs to the existing
object-op and submit-op ioctl paths and validated them on seeded stale state:

- [docs/77-sgx-lab-cleanup-op-ioctl.md](/root/Poulsbo/docs/77-sgx-lab-cleanup-op-ioctl.md)
- [logs/vaio-ioctl/20260511-111529-sgx-cleanup-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-111529-sgx-cleanup-op-live)

That run validated:

- typed `submit cleanup`
- typed `object cleanup`
- focused-submit clearing through submit cleanup
- PF-bound object drop and unbind through object cleanup

The live run seeded:

- two held GEM ranges:
  - `RANGE_ID_A=5`
  - `RANGE_ID_B=7`
- two adopted objects:
  - object `1` on range `5`
  - object `2` on range `7`
- two PF submits with one focused submit

Before cleanup:

- `focused_submit_valid=1`
- `focused_submit_id=1`
- `live_submit_count=2`
- `objects_shown=2`
- object `1` had `pf_bind_count=1`

Then the typed cleanup ops completed with:

- `submit_result_count=2`
- `object_result_count=2`

And the post-cleanup state was clean:

- `COORD_AFTER_SUBMIT_CLEANUP`:
  - `focused_submit_valid=0`
  - `focused_submit_id=0`
  - `live_submit_count=0`
- `SUBMITS_AFTER_SUBMIT_CLEANUP`:
  - `submits_shown=0`
- `OBJECTS_AFTER_OBJECT_CLEANUP`:
  - `objects_shown=0`
- typed range-query readback for both source ranges:
  - `range_sgx_object_id=0`
- the PF-bound source range also showed:
  - `range_pf_bind_count=0`
  - `range_pf_bind_total=1`
  - `range_pf_unbind_total=1`

The updated multi-submit runner was also sanity-rerun immediately after that in:

- [logs/vaio-ioctl/20260511-111946-sgx-multi-submit-cleanup-rerun](/root/Poulsbo/logs/vaio-ioctl/20260511-111946-sgx-multi-submit-cleanup-rerun)

That rerun proved the runner itself actually took:

- `STALE_SUBMIT_CLEANUP=ioctl`
- `STALE_OBJECT_CLEANUP=ioctl`

and still finished with:

- `overall=remote_usable`

The final target state after that run is still clean:

- `overall=remote_usable`
- `driver_srcversion=C142911E7A8FF2E7821CAE0`
- `override_module_size=384424`
- `rollback_pending_present=0`
- `rollback_backup_present=1`
- `fault_count=0`
- `bif_fault=0x000000`

The next object-side readout follow-up added a bounded typed object query and
validated it directly against the live `poulsbo_sgx_objects` path:

- [docs/78-sgx-lab-object-query-ioctl.md](/root/Poulsbo/docs/78-sgx-lab-object-query-ioctl.md)
- [logs/vaio-ioctl/20260511-112759-sgx-object-query-live](/root/Poulsbo/logs/vaio-ioctl/20260511-112759-sgx-object-query-live)

That run validated:

- `QUERY_OBJECT` by explicit object ID
- `QUERY_OBJECT` by live object index
- object-state persistence after userspace close
- clean `ENOENT` after explicit cleanup

The live run seeded:

- two held GEM ranges:
  - `RANGE_ID_A=5`
  - `RANGE_ID_B=7`
- two adopted objects:
  - object `1` on range `5`
  - object `2` on range `7`

Object `1` was then moved to a non-default state:

- `role=control`
- `class=packet`
- `gpu_use=control_ccb`
- `expected_context=pf`
- `pf_bind_count=1`

The typed query matched the sysfs object dump:

- `OBJECT_QUERY_ID_A_DURING`:
  - `object_resolved_id=1`
  - `object_resolved_index=0`
  - `object_total_count=2`
  - `object_range_id=5`
  - `object_role_name=control`
  - `object_class_name=packet`
  - `object_gpu_use_name=control_ccb`
  - `object_expected_context_name=pf`
  - `object_pf_bind_count=1`
- `OBJECT_QUERY_ID_B_DURING`:
  - `object_resolved_id=2`
  - `object_resolved_index=1`
  - `object_total_count=2`
  - `object_range_id=7`
  - `object_role_name=generic`
  - `object_class_name=generic`

The index-based queries also worked:

- `OBJECT_QUERY_INDEX_0_DURING` resolved object `1`
- `OBJECT_QUERY_INDEX_1_DURING` resolved object `2`

After the userspace holders exited, both ID queries still succeeded. After
typed cleanup:

- `object_result_count=2`
- `objects_shown=0`
- `OBJECT_QUERY_ID_A_AFTER_CLEANUP` returned `errno=2`
- `OBJECT_QUERY_ID_B_AFTER_CLEANUP` returned `errno=2`

The final target state after that run is still clean:

- `overall=remote_usable`
- `driver_srcversion=176E7F4A4C9DCF459B10C8E`
- `override_module_size=385256`
- `rollback_pending_present=0`
- `rollback_backup_present=1`
- `fault_count=0`
- `bif_fault=0x000000`

The next submit-side readout follow-up extended the bounded submit query to
support live submit-index lookup and validated that directly against a two-
submit live state:

- [docs/79-sgx-lab-submit-query-index-ioctl.md](/root/Poulsbo/docs/79-sgx-lab-submit-query-index-ioctl.md)
- [logs/vaio-ioctl/20260511-113812-sgx-submit-query-live](/root/Poulsbo/logs/vaio-ioctl/20260511-113812-sgx-submit-query-live)

That run validated:

- `QUERY_SUBMIT` by explicit submit ID
- `QUERY_SUBMIT` by focused submit
- `QUERY_SUBMIT` by live submit index
- distinct per-submit state readback on two live submits
- submit-state persistence after userspace close
- clean `ENOENT` after typed submit cleanup for:
  - explicit submit ID
  - live submit index
  - focused submit

The live run seeded:

- five held GEM ranges:
  - `RANGE_IDS=7 9 15 16 17`
- five adopted objects:
  - `OBJECT_IDS=6 7 8 9 10`
- two live submits:
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`

Submit A was advanced one step while submit B was left queued:

- submit A:
  - `submit_resolved_index=0`
  - `submit_total_count=2`
  - `submit_issue_count=1`
  - `submit_queue_packet_count=2`
  - `submit_sync_pending_seq=0x42`
  - `submit_owner_issue_cookie=0x102`
  - `submit_global_issue_ticket=2`
- submit B:
  - `submit_resolved_index=1`
  - `submit_total_count=2`
  - `submit_issue_count=0`
  - `submit_queue_packet_count=1`
  - `submit_sync_pending_seq=0x41`
  - `submit_owner_issue_cookie=0x101`
  - `submit_global_issue_ticket=0`

The new index-based queries matched those same live submits exactly:

- `SUBMIT_QUERY_INDEX_A`:
  - `submit_query_index=0`
  - `submit_resolved_id=3`
  - `submit_resolved_index=0`
  - `submit_issue_count=1`
- `SUBMIT_QUERY_INDEX_B`:
  - `submit_query_index=1`
  - `submit_resolved_id=4`
  - `submit_resolved_index=1`
  - `submit_issue_count=0`

The focused query also matched submit B:

- `FOCUS_QUERY_BASE`:
  - `submit_resolved_id=4`
  - `submit_resolved_index=1`

After the userspace holders exited, both ID queries and both index queries
still succeeded. After typed cleanup:

- `submit_result_count=2`
- `object_result_count=5`
- `SUBMIT_QUERY_A_AFTER_CLEANUP_RC=3`
- `SUBMIT_QUERY_B_AFTER_CLEANUP_RC=3`
- `SUBMIT_QUERY_INDEX_0_AFTER_CLEANUP_RC=3`
- `FOCUS_QUERY_AFTER_CLEANUP_RC=3`
- all four returned `submit_ioctl_ret=-1 errno=2`

The final target state after that run is still clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

The next read-side follow-up removed the last practical sysfs inspection
dependency for live SGX lab state by validating a typed no-sysfs dump flow:

- [docs/80-sgx-lab-typed-state-dump-validation.md](/root/Poulsbo/docs/80-sgx-lab-typed-state-dump-validation.md)
- [logs/vaio-ioctl/20260511-114905-sgx-state-dump-live](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live)

That run validated a live dump path that used only:

- `psb_sgx_lab_query`
- `psb_sgx_lab_object_query`
- `psb_sgx_lab_sgx_state_query`
- `psb_sgx_lab_gtt_state_query`
- `psb_sgx_lab_object_op`
- `psb_sgx_lab_submit_op`

and did not depend on PCI-device sysfs reads during validation.

The live run seeded:

- five held GEM ranges:
  - `RANGE_IDS=7 9 20 21 22`
- five adopted objects:
  - `OBJECT_IDS=11 12 13 14 15`
- two live submits:
  - `SUBMIT_A_ID=5`
  - `SUBMIT_B_ID=6`

The typed coord dump showed the expected live shape:

- `coord_focus_valid=1`
- `coord_focused_submit_id=6`
- `coord_live_submit_count=2`
- `coord_live_object_count=5`
- `coord_issue_ticket=3`
- `coord_issue_submit_id=5`

The typed global state dumps also matched the staged state:

- `GTT_DURING`:
  - `gtt_state_range_sgx_object_count=5`
  - `gtt_state_range_pf_bound_count=5`
- `SGX_DURING`:
  - `sgx_state_ioctl_ret=0 errno=0`
  - `sgx_state_reason=ioctl-sgx-state`
  - `sgx_state_fault_count=0`
  - `sgx_state_bif_fault=0x0`

The typed per-object and per-submit index dumps also matched the staged roles
and progression:

- `OBJECT_INDEX_0_DURING`:
  - `object_resolved_id=11`
  - `object_role_name=control`
  - `object_class_name=packet`
  - `object_gpu_use_name=control_ccb`
- `OBJECT_INDEX_4_DURING`:
  - `object_resolved_id=15`
  - `object_role_name=status`
  - `object_class_name=state`
  - `object_gpu_use_name=status_block`
- `SUBMIT_INDEX_0_DURING`:
  - `submit_resolved_id=5`
  - `submit_issue_count=1`
  - `submit_queue_packet_count=2`
  - `submit_sync_pending_seq=0x42`
- `SUBMIT_INDEX_1_DURING`:
  - `submit_resolved_id=6`
  - `submit_issue_count=0`
  - `submit_queue_packet_count=1`
  - `submit_sync_pending_seq=0x41`

After all userspace holders exited, the typed dump still reported:

- `coord_live_submit_count=2`
- `coord_live_object_count=5`
- `OBJECT_INDEX_0_AFTER_PROBES` still resolved object `11`
- `SUBMIT_INDEX_0_AFTER_PROBES` still resolved submit `5`

After typed cleanup:

- `submit_result_count=2`
- `object_result_count=5`
- `COORD_AFTER_CLEANUP`:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
- `GTT_AFTER_CLEANUP`:
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

The final target state after that run is still clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

The same typed no-sysfs state path now also has a standalone userspace dump
tool validated on the VAIO:

- [docs/81-sgx-lab-dump-tool-validation.md](/root/Poulsbo/docs/81-sgx-lab-dump-tool-validation.md)
- [logs/vaio-ioctl/20260511-114905-sgx-state-dump-live](/root/Poulsbo/logs/vaio-ioctl/20260511-114905-sgx-state-dump-live)

That rerun added and validated:

- [workloads/psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)

The live dump tool itself reported, during the staged state:

- `coord_live_submit_count=2`
- `coord_live_object_count=5`
- `gtt_state_range_sgx_object_count=5`
- `sgx_state_fault_count=0`
- `object[0]_role_name=control`
- `submit[0]_issue_count=1`

and after typed cleanup:

- `coord_live_submit_count=0`
- `coord_live_object_count=0`
- `gtt_state_range_sgx_object_count=0`

The final target state after that rerun is still clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

The focused SGX lab cleanup validator has now also been moved off sysfs for
its verification path:

- [docs/83-sgx-lab-cleanup-nosysfs-validation.md](/root/Poulsbo/docs/83-sgx-lab-cleanup-nosysfs-validation.md)
- [logs/vaio-ioctl/20260511-122500-sgx-cleanup-nosysfs-live](/root/Poulsbo/logs/vaio-ioctl/20260511-122500-sgx-cleanup-nosysfs-live)

This rerun stayed on the same validated VAIO module:

- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`

The cleanup runner now verifies state through typed lab helpers only:

- typed coord query
- typed submit-index query
- typed object-ID query
- typed range-ID query
- standalone typed dump snapshots

It no longer uses:

- `poulsbo_sgx_coord_state`
- `poulsbo_sgx_submits`
- `poulsbo_sgx_objects`
- `poulsbo_gtt_state`

The typed evidence on this run was:

- before cleanup:
  - `coord_focus_valid=1`
  - `coord_focused_submit_id=13`
  - `coord_live_submit_count=2`
  - `coord_live_object_count=2`
  - submit indices `0/1` resolved submits `13/14`
  - `gtt_state_range_sgx_object_count=2`
  - `gtt_state_range_pf_bound_count=1`
- after typed submit cleanup:
  - `coord_focus_valid=0`
  - `coord_live_submit_count=0`
  - focused-submit query failed with `errno=2`
  - submit index `0` query failed with `errno=2`
- after typed object cleanup:
  - object-ID queries for objects `28/29` failed with `errno=2`
  - `coord_live_object_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

The final target state after that rerun is still clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

The focused SGX lab object-op validator has now also been moved off sysfs for
its verification path:

- [docs/84-sgx-lab-object-op-nosysfs-validation.md](/root/Poulsbo/docs/84-sgx-lab-object-op-nosysfs-validation.md)
- [logs/vaio-ioctl/20260511-123200-sgx-object-op-nosysfs-live](/root/Poulsbo/logs/vaio-ioctl/20260511-123200-sgx-object-op-nosysfs-live)

This rerun stayed on the same validated VAIO module:

- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`

The object-op runner now verifies state through typed lab helpers only:

- typed object-ID query
- typed range-ID query
- typed global GTT/MMU state query

It no longer uses:

- `poulsbo_sgx_objects`
- `poulsbo_gtt_state`

The typed evidence on this run was:

- before adopt:
  - `range_sgx_object_id=0`
  - `range_pf_bind_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`
- after adopt:
  - `object_role_name=generic`
  - `object_class_name=generic`
  - `object_gpu_use_name=generic`
  - `object_expected_context_name=auto`
  - `range_sgx_object_id=30`
  - `gtt_state_range_sgx_object_count=1`
- after metadata updates:
  - `object_role_name=control`
  - `object_class_name=packet`
  - `object_gpu_use_name=control_ccb`
  - `object_expected_context_name=pf`
- after PF bind:
  - `object_pf_bind_count=1`
  - `range_pf_bind_count=1`
  - `gtt_state_range_pf_bound_count=1`
- after PF unbind:
  - `object_pf_bind_count=0`
  - `range_pf_bind_count=0`
  - `gtt_state_range_pf_bound_count=0`
- after typed drop:
  - object-ID query failed with `errno=2`
  - `range_sgx_object_id=0`
  - `gtt_state_range_sgx_object_count=0`

The final target state after that rerun is still clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

The focused SGX lab submit-op validator has now also been moved off sysfs for
both setup and verification:

- [docs/85-sgx-lab-submit-op-nosysfs-validation.md](/root/Poulsbo/docs/85-sgx-lab-submit-op-nosysfs-validation.md)
- [logs/vaio-ioctl/20260511-123800-sgx-submit-op-nosysfs-live](/root/Poulsbo/logs/vaio-ioctl/20260511-123800-sgx-submit-op-nosysfs-live)

This rerun stayed on the same validated VAIO module:

- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`

The submit-op runner now stages and verifies through typed lab helpers only:

- typed stale cleanup through `submit cleanup` and `object cleanup`
- typed live GEM range discovery through range-query by index
- typed object-ID and range-ID readback after object setup
- typed global GTT/MMU state readback after PF binds and cleanup
- typed submit query by explicit ID and by focused submit
- typed coord readback around `issue`, `running`, and `complete`

It no longer uses:

- `poulsbo_sgx_object`
- `poulsbo_sgx_objects`
- `poulsbo_sgx_submit`
- `poulsbo_sgx_submits`
- `poulsbo_sgx_submit_state`
- `poulsbo_sgx_coord_state`
- `poulsbo_gtt_state`

The typed evidence on this run was:

- staged live state:
  - `SUBMIT_ID=16`
  - `RANGE_IDS=7 9 45 46 47`
  - `OBJECT_IDS=36 37 38 39 40`
- after typed object setup:
  - control object `36` resolved as:
    - `object_role_name=control`
    - `object_class_name=packet`
    - `object_gpu_use_name=control_ccb`
    - `object_expected_context_name=pf`
    - `object_pf_bind_count=1`
  - status object `40` resolved as:
    - `object_role_name=status`
    - `object_class_name=state`
    - `object_gpu_use_name=status_block`
    - `object_expected_context_name=pf`
    - `object_pf_bind_count=1`
  - typed GTT state showed:
    - `gtt_state_range_sgx_object_count=5`
    - `gtt_state_range_pf_bound_count=5`
- after typed attach:
  - submit query showed:
    - `submit_control_object_id=36`
    - `submit_pds_object_id=37`
    - `submit_use_object_id=38`
    - `submit_target_object_id=39`
    - `submit_status_object_id=40`
    - `submit_owner_state_name=queued`
- after typed `validate`:
  - `submit_validate_count=1`
  - `submit_issue_count=0`
  - `submit_transition_count=0`
- after typed `issue`:
  - `coord_issue_submit_id=16`
  - `submit_owner_state_name=queued`
  - `submit_issue_count=1`
  - `submit_global_issue_ticket=7`
- after typed `transition running`:
  - `coord_running_submit_id=16`
  - `submit_owner_state_name=running`
  - `submit_transition_count=1`
  - `submit_global_running_ticket=2`
- after typed `transition complete`:
  - `coord_complete_submit_id=16`
  - `submit_owner_state_name=complete`
  - `submit_transition_count=2`
  - `submit_global_complete_ticket=2`
- after typed submit drop:
  - focused-submit query failed with `errno=2`
  - explicit submit-ID query failed with `errno=2`
  - `coord_live_submit_count=0`
  - `coord_live_object_count=5`
- after typed object cleanup:
  - `object_result_count=5`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`
  - object-ID query for object `36` failed with `errno=2`

The final target state after that rerun is still clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

The higher-level SGX multi-submit runner now also has a validated no-sysfs
verification mode:

- [docs/86-sgx-lab-multi-submit-nosysfs-validation.md](/root/Poulsbo/docs/86-sgx-lab-multi-submit-nosysfs-validation.md)
- [logs/vaio-ioctl/20260511-124033-sgx-multi-submit-nosysfs-live](/root/Poulsbo/logs/vaio-ioctl/20260511-124033-sgx-multi-submit-nosysfs-live)

This rerun stayed on the same validated VAIO module:

- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`

The updated runner now supports:

- `USE_IOCTL_VERIFICATION=1`

That mode requires:

- `USE_IOCTL_OBJECT_SETUP=1`
- `USE_IOCTL_SUBMIT_SETUP=1`
- `USE_IOCTL_DEPENDENCY_SETUP=1`
- `USE_IOCTL_RANGE_DISCOVERY=1`

The dedicated wrapper:

- [scripts/run-vaio-sgx-multi-submit-nosysfs.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit-nosysfs.sh)

drives the full four-lane dependency-slot-clear scenario under that fully
typed mode.

The saved run reported:

- `SUBMIT_SETUP=ioctl`
- `OBJECT_SETUP=ioctl`
- `RANGE_DISCOVERY=ioctl`
- `DEPENDENCY_SETUP=ioctl`
- `VERIFICATION_MODE=ioctl`
- `DEPENDENCY_INTERFACE=indexed`

The typed setup and verification path showed the expected live staged state:

- `coord_live_submit_count=5`
- `coord_live_object_count=25`
- `gtt_state_range_sgx_object_count=25`
- `gtt_state_range_pf_bound_count=25`

The same typed run still proved the live slot-4 clear path:

- submit B finished with:
  - `dependency[4]_submit_id=0`
  - `dependency[4]_flags=0x0`
- submit E later completed independently after that clear

The same typed run also returned cleanly to zero after final drop:

- `coord_live_submit_count=0`
- `gtt_state_range_sgx_object_count=0`
- `gtt_state_range_pf_bound_count=0`

The final target state after that rerun is still clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

That same standalone dump tool now also has validated structured JSON output:

- [docs/82-sgx-lab-dump-json-validation.md](/root/Poulsbo/docs/82-sgx-lab-dump-json-validation.md)
- [logs/vaio-ioctl/20260511-120359-sgx-dump-json-live](/root/Poulsbo/logs/vaio-ioctl/20260511-120359-sgx-dump-json-live)

This rerun stayed on the same validated VAIO module:

- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`

The extended dump tool now supports:

- `cached`
- `capture`
- `json`
- `capture-json`

The new run validated both text and JSON dumps against the same live staged
state:

- `RANGE_IDS=7 9 30 31 32`
- `OBJECT_IDS=21 22 23 24 25`
- `SUBMIT_A_ID=9`
- `SUBMIT_B_ID=10`

The JSON dump matched the text dump during staging:

- `"live_submit_count": 2`
- `"live_object_count": 5`
- `"range_sgx_object_count": 5`
- `"fault_count": 0`
- `"role_name": "control"`
- `"issue_count": 1`

and after typed cleanup:

- `"live_submit_count": 0`
- `"live_object_count": 0`
- `"range_sgx_object_count": 0`

The final target state after that rerun is still clean:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

The dependency-specific wrapper layer now also defaults to that same typed
path:

- [docs/87-sgx-dependency-wrapper-typed-defaults.md](/root/Poulsbo/docs/87-sgx-dependency-wrapper-typed-defaults.md)
- [logs/vaio-ioctl/20260511-124744-sgx-dependency-slot-clear-typed-wrapper-live-retry](/root/Poulsbo/logs/vaio-ioctl/20260511-124744-sgx-dependency-slot-clear-typed-wrapper-live-retry)

The new generic wrapper is:

- `scripts/run-vaio-sgx-multi-submit-typed.sh`

and the dependency-oriented scenario wrappers now call it instead of calling
`run-vaio-sgx-multi-submit.sh` directly.

The validated wrapper rerun stayed on the typed path throughout:

- `SUBMIT_SETUP=ioctl`
- `OBJECT_SETUP=ioctl`
- `RANGE_DISCOVERY=ioctl`
- `DEPENDENCY_SETUP=ioctl`
- `VERIFICATION_MODE=ioctl`
- `DEPENDENCY_INTERFACE=indexed`

The same wrapper run staged the full live state:

- `coord_live_submit_count=5`
- `coord_live_object_count=25`
- `gtt_state_range_sgx_object_count=25`
- `gtt_state_range_pf_bound_count=25`

and still proved live slot-4 dependency removal on submit B:

- before clear:
  - `dependency[4]_submit_id=31`
  - `dependency[4]_required_ticket=4`
  - `dependency[4]_flags=0xb`
- after clear:
  - `dependency[4]_submit_id=0`
  - `dependency[4]_required_ticket=0`
  - `dependency[4]_flags=0x0`

The same wrapper run also returned to zero cleanly through typed cleanup:

- `coord_live_submit_count=0`
- `coord_live_object_count=0`
- `gtt_state_range_sgx_object_count=0`
- `gtt_state_range_pf_bound_count=0`

The older focused helper names now also default onto the typed ioctl path:

- [docs/88-sgx-focused-wrapper-typed-defaults.md](/root/Poulsbo/docs/88-sgx-focused-wrapper-typed-defaults.md)
- object wrapper bundle:
  - [logs/vaio-ioctl/20260511-125525-sgx-object-wrapper-typed-default-live-rerun](/root/Poulsbo/logs/vaio-ioctl/20260511-125525-sgx-object-wrapper-typed-default-live-rerun)
- submit wrapper bundle:
  - [logs/vaio-ioctl/20260511-125341-sgx-submit-wrapper-typed-default-live](/root/Poulsbo/logs/vaio-ioctl/20260511-125341-sgx-submit-wrapper-typed-default-live)

New focused typed wrappers:

- `scripts/run-vaio-sgx-object-typed.sh`
- `scripts/run-vaio-sgx-submit-typed.sh`

Default focused helper behavior:

- `scripts/run-vaio-sgx-object.sh` now defaults to the typed object-op path
- `scripts/run-vaio-sgx-submit.sh` now defaults to the typed submit-op path

Explicit legacy escape hatches:

- `SGX_OBJECT_DRIVER_MODE=legacy`
- `SGX_SUBMIT_DRIVER_MODE=legacy`

The validated object-wrapper rerun showed:

- `REMOTE_OP_BIN=/root/poulsbo-workloads/psb_sgx_lab_object_op`
- `RANGE_ID=7`
- `OBJECT_ID=123`
- `object_role_name=control`
- `object_class_name=packet`
- `object_gpu_use_name=control_ccb`
- `object_expected_context_name=pf`
- post-bind:
  - `object_pf_bind_count=1`
  - `range_pf_bind_count=1`
  - `gtt_state_range_pf_bound_count=1`
- post-drop:
  - `object_ioctl_ret=-1 errno=2`
  - `gtt_state_range_sgx_object_count=0`

The validated submit-wrapper rerun showed:

- `REMOTE_SUBMIT_OP_BIN=/root/poulsbo-workloads/psb_sgx_lab_submit_op`
- `SUBMIT_ID=32`
- `RANGE_IDS=7 9 125 126 127`
- `OBJECT_IDS=117 118 119 120 121`
- post-setup:
  - `gtt_state_range_sgx_object_count=5`
  - `gtt_state_range_pf_bound_count=5`
- post-validate:
  - `submit_validate_count=1`
- post-issue:
  - `submit_issue_count=1`
  - `coord_issue_submit_id=32`
- post-running:
  - `submit_transition_count=1`
  - `submit_owner_state_name=running`
  - `coord_running_submit_id=32`
- post-complete:
  - `submit_transition_count=2`
  - `submit_owner_state_name=complete`
  - `coord_complete_submit_id=32`
- post-drop:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

The dependency-specific multi-submit wrappers now also expose an explicit
default-vs-legacy gate:

- [docs/89-sgx-multi-submit-wrapper-mode-gate.md](/root/Poulsbo/docs/89-sgx-multi-submit-wrapper-mode-gate.md)
- wrapper bundle:
  - [logs/vaio-ioctl/20260511-130053-sgx-indexed-wrapper-default-live](/root/Poulsbo/logs/vaio-ioctl/20260511-130053-sgx-indexed-wrapper-default-live)

New dispatcher:

- `scripts/run-vaio-sgx-multi-submit-default.sh`

Default behavior:

- `SGX_MULTI_SUBMIT_DRIVER_MODE=typed`

Explicit legacy escape hatch:

- `SGX_MULTI_SUBMIT_DRIVER_MODE=legacy`

The validated indexed-wrapper rerun showed:

- `SUBMIT_SETUP=ioctl`
- `OBJECT_SETUP=ioctl`
- `RANGE_DISCOVERY=ioctl`
- `DEPENDENCY_SETUP=ioctl`
- `VERIFICATION_MODE=ioctl`
- `DEPENDENCY_INTERFACE=indexed`
- staged state:
  - `coord_live_submit_count=4`
  - `coord_live_object_count=20`
  - `gtt_state_range_sgx_object_count=20`
  - `gtt_state_range_pf_bound_count=20`
- indexed dependency bindings:
  - `dependency[1]_submit_id=33`
  - `dependency[2]_submit_id=35`
  - `dependency[3]_submit_id=36`
- final cleanup:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

The explicit legacy escape hatches are now also hardware-validated:

- [docs/90-sgx-legacy-mode-gate-validation.md](/root/Poulsbo/docs/90-sgx-legacy-mode-gate-validation.md)
- focused object legacy bundle:
  - [logs/vaio-legacy/20260511-130407-sgx-object-wrapper-legacy-live](/root/Poulsbo/logs/vaio-legacy/20260511-130407-sgx-object-wrapper-legacy-live)
- focused submit legacy bundle:
  - [logs/vaio-legacy/20260511-130601-sgx-submit-wrapper-legacy-live-rerun](/root/Poulsbo/logs/vaio-legacy/20260511-130601-sgx-submit-wrapper-legacy-live-rerun)
- indexed multi-submit legacy bundle:
  - [logs/vaio-legacy/20260511-130640-sgx-indexed-wrapper-legacy-live](/root/Poulsbo/logs/vaio-legacy/20260511-130640-sgx-indexed-wrapper-legacy-live)

The focused legacy submit validation also exposed and fixed one real issue in
the old sysfs-heavy script:

- `scripts/run-vaio-sgx-submit.sh`

New legacy scan knob:

- `RANGE_SCAN_MAX`

Current default:

- `512`

The same symmetry fix is now also applied to the direct legacy multi-submit
script:

- `scripts/run-vaio-sgx-multi-submit.sh`

The validated explicit legacy runs showed:

- focused object legacy still completed with:
  - `RANGE_ID=153`
  - `OBJECT_ID=144`
  - clean `GTT_STATE_AFTER_DROP`
  - clean `OBJECTS_AFTER_DROP`
- focused submit legacy now again completes with:
  - `RANGE_IDS=159 160 161 162 163`
  - `OBJECT_IDS=147 148 149 150 151`
  - `SUBMIT_ID=37`
  - clean `SUBMITS_AFTER_DROP`
  - clean `OBJECTS_AFTER_DROP`
- indexed multi-submit legacy still completes with:
  - `SUBMIT_A_ID=38`
  - `SUBMIT_B_ID=39`
  - `SUBMIT_C_ID=40`
  - `SUBMIT_D_ID=41`
  - clean `SUBMITS_AFTER_DROP`
  - clean `OBJECTS_AFTER_DROP`

The indexed legacy path was then rerun after the multi-submit-side
`RANGE_SCAN_MAX` symmetry fix and still completed cleanly:

- rerun bundle:
  - [logs/vaio-legacy/20260511-131035-sgx-indexed-wrapper-legacy-live-rerun](/root/Poulsbo/logs/vaio-legacy/20260511-131035-sgx-indexed-wrapper-legacy-live-rerun)
- rerun IDs:
  - `SUBMIT_A_ID=42`
  - `SUBMIT_B_ID=43`
  - `SUBMIT_C_ID=44`
  - `SUBMIT_D_ID=45`
  - `RANGE_IDS=184 185 186 187 188 189 190 191 192 193 194 195 196 197 198 199 200 201 202 203`
  - `OBJECT_IDS_A=172 173 174 175 176`
  - `OBJECT_IDS_B=177 178 179 180 181`
  - `OBJECT_IDS_C=182 183 184 185 186`
  - `OBJECT_IDS_D=187 188 189 190 191`
  - clean `SUBMITS_AFTER_DROP`
  - clean `OBJECTS_AFTER_DROP`

The wrapper layer now also has direct typed-versus-legacy compare sessions:

- [docs/91-sgx-mode-compare-session-validation.md](/root/Poulsbo/docs/91-sgx-mode-compare-session-validation.md)
- [docs/92-sgx-dependency-wrapper-mode-compare-validation.md](/root/Poulsbo/docs/92-sgx-dependency-wrapper-mode-compare-validation.md)
- [docs/93-sgx-indexed-dependency-array-mode-compare-validation.md](/root/Poulsbo/docs/93-sgx-indexed-dependency-array-mode-compare-validation.md)
- [docs/94-sgx-relative-dependency-ticket-fix.md](/root/Poulsbo/docs/94-sgx-relative-dependency-ticket-fix.md)
- focused object compare bundle:
  - [logs/vaio-compare/20260511-132342-sgx-object-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-132342-sgx-object-mode-compare)
- focused submit compare bundle:
  - [logs/vaio-compare/20260511-132458-sgx-submit-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-132458-sgx-submit-mode-compare)
- multi-submit compare bundle:
  - [logs/vaio-compare/20260511-132628-sgx-multi-submit-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-132628-sgx-multi-submit-mode-compare)
- dependency-slot-clear compare bundle:
  - [logs/vaio-compare/20260511-134016-sgx-dependency-slot-clear-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-134016-sgx-dependency-slot-clear-mode-compare)
- indexed-dependency-array compare bundle:
  - [logs/vaio-compare/20260511-134740-sgx-indexed-dependency-array-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-134740-sgx-indexed-dependency-array-mode-compare)
- multi-source-ticket compare bundle:
  - [logs/vaio-compare/20260511-140046-sgx-multi-source-ticket-dependency-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-140046-sgx-multi-source-ticket-dependency-mode-compare)

Those compare sessions now prove the dispatcher layer itself:

- focused object:
  - typed:
    - `typed_range_id=7`
    - `typed_object_id=204`
    - `typed_object_role=control`
    - `typed_object_class=packet`
    - `typed_object_gpu_use=control_ccb`
    - `typed_object_context=pf`
  - legacy:
    - `legacy_range_id=217`
    - `legacy_object_id=205`
- focused submit:
  - typed:
    - `typed_submit_id=48`
    - `typed_validate_count=1`
    - `typed_issue_count=1`
    - `typed_transition_count=2`
    - `typed_owner_state=complete`
  - legacy:
    - `legacy_submit_id=49`
    - `legacy_queue_mode=wrap`
- multi-submit:
  - typed:
    - `typed_submit_setup=ioctl`
    - `typed_object_setup=ioctl`
    - `typed_range_discovery=ioctl`
    - `typed_dependency_setup=ioctl`
    - `typed_verification_mode=ioctl`
    - `typed_submit_a_id=50`
    - `typed_submit_b_id=51`
    - `typed_submit_validate_count=11`
    - `typed_submit_issue_count=2`
    - `typed_submit_transition_count=4`
  - legacy:
    - `legacy_submit_a_id=52`
    - `legacy_submit_b_id=53`
    - `legacy_dependency_interface=named`

All three compare sessions completed with:

- `session_outcome=completed`
- `typed_health_overall=remote_usable`
- `legacy_health_overall=remote_usable`
- final target state still:
  - `overall=remote_usable`
  - `driver_srcversion=D43951B174F84BE870230AB`
  - `override_module_size=385384`
  - `rollback_pending_present=0`

The dependency-specific wrapper layer is now covered too:

- typed/default dependency-slot-clear compare:
  - `typed_submit_setup=ioctl`
  - `typed_object_setup=ioctl`
  - `typed_range_discovery=ioctl`
  - `typed_dependency_setup=ioctl`
  - `typed_verification_mode=ioctl`
  - `typed_dependency_interface=indexed`
  - `typed_submit_a_id=54`
  - `typed_submit_b_id=55`
  - `typed_submit_c_id=56`
  - `typed_submit_d_id=57`
  - `typed_submit_e_id=58`
  - `typed_dependency4_submit_id_after_clear=0`
  - `typed_dependency4_flags_after_clear=0x0`
  - final typed cleanup:
    - `typed_live_submit_count_after_drop=0`
    - `typed_live_object_count_after_drop=0`
    - `typed_range_sgx_object_count_after_drop=0`
    - `typed_range_pf_bound_count_after_drop=0`
- explicit legacy dependency-slot-clear compare:
  - `legacy_submit_setup=sysfs`
  - `legacy_object_setup=sysfs`
  - `legacy_range_discovery=sysfs`
  - `legacy_dependency_setup=sysfs`
  - `legacy_verification_mode=sysfs`
  - `legacy_dependency_interface=indexed`
  - `legacy_submit_a_id=59`
  - `legacy_submit_b_id=60`
  - `legacy_submit_c_id=61`
  - `legacy_submit_d_id=62`
  - `legacy_submit_e_id=63`
  - cleanup markers still present:
    - `legacy_has_submits_after_drop=SUBMITS_AFTER_DROP`
    - `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`
    - `legacy_has_coord_after_drop=COORD_STATE_AFTER_DROP`

The indexed dependency-array wrapper now also has a direct compare bundle:

- typed/default indexed dependency-array compare:
  - `typed_submit_setup=ioctl`
  - `typed_object_setup=ioctl`
  - `typed_range_discovery=ioctl`
  - `typed_dependency_setup=ioctl`
  - `typed_verification_mode=ioctl`
  - `typed_dependency_interface=indexed`
  - `typed_submit_a_id=64`
  - `typed_submit_b_id=65`
  - `typed_submit_c_id=66`
  - `typed_submit_d_id=67`
  - `typed_submit_validate_count=5`
  - `typed_submit_issue_count=1`
  - `typed_submit_transition_count=2`
  - `typed_submit_owner_state=complete`
  - final typed cleanup:
    - `typed_live_submit_count=0`
    - `typed_live_object_count=0`
    - `typed_range_sgx_object_count=0`
    - `typed_range_pf_bound_count=0`
- explicit legacy indexed dependency-array compare:
  - `legacy_dependency_interface=indexed`
  - `legacy_submit_a_id=68`
  - `legacy_submit_b_id=69`
  - `legacy_submit_c_id=70`
  - `legacy_submit_d_id=71`
  - cleanup markers still present:
    - `legacy_has_submits_after_drop=SUBMITS_AFTER_DROP`
    - `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`
    - `legacy_has_coord_after_drop=COORD_STATE_AFTER_DROP`

The multi-source ticket wrapper now also has a direct compare bundle, and the
runner bug behind it is fixed:

- the old absolute lane-2 default ticket was wrong once the persistent
  driver-global completion counter moved far above `3`
- the runner now supports `+<n>` dependency-ticket syntax resolved relative to
  the current coordination baseline
- `scripts/run-vaio-sgx-multi-source-ticket-dependency.sh` now defaults lane 2
  to `+3`

Typed/default multi-source ticket compare:

- `typed_submit_setup=ioctl`
- `typed_object_setup=ioctl`
- `typed_range_discovery=ioctl`
- `typed_dependency_setup=ioctl`
- `typed_verification_mode=ioctl`
- `typed_dependency_interface=named`
- `typed_submit_a_id=90`
- `typed_submit_b_id=91`
- `typed_submit_c_id=92`
- terminal typed state:
  - `typed_submit_validate_count=8`
  - `typed_submit_issue_count=2`
  - `typed_submit_transition_count=4`
  - `typed_submit_owner_state=complete`
  - final typed cleanup:
    - `typed_live_submit_count=0`
    - `typed_live_object_count=0`
    - `typed_range_sgx_object_count=0`
    - `typed_range_pf_bound_count=0`
- typed relative-ticket evidence:
  - `COORD_COMPLETE_TICKET_BASELINE=84`
  - `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET=87`
  - `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET_RELATIVE=3`

Explicit legacy multi-source ticket compare:

- `legacy_dependency_interface=named`
- `legacy_submit_a_id=93`
- `legacy_submit_b_id=94`
- `legacy_submit_c_id=95`
- legacy relative-ticket evidence:
  - `COORD_COMPLETE_TICKET_BASELINE=88`
  - `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET=91`
  - `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET_RELATIVE=3`
- cleanup markers still present:
  - `legacy_has_submits_after_drop=SUBMITS_AFTER_DROP`
  - `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`
  - `legacy_has_coord_after_drop=COORD_STATE_AFTER_DROP`

The same work also exposed and fixed one real helper bug:

- `scripts/summarize-vaio-sessions.sh`

It was sorting by path prefix instead of session timestamp, so new compare
bundles under `logs/vaio-compare/` did not rise to the top of recent-session
output. It now sorts by the session directory timestamp prefix, and

- `MAX_SESSIONS=8 ./scripts/summarize-vaio-sessions.sh`

now surfaces the newest compare bundles first.

The compare wrappers now also persist the final remote module identity in
`session-result.env`, and the session summary helper now falls back to saved
pre/post state logs for older compare bundles. That means compare sessions now
show:

- `module_srcversion=D43951B174F84BE870230AB`

instead of `unknown`, and identical typed/legacy health is collapsed to:

- `last_state=remote_usable`

## Compare Reset Plus Legacy Range-Baseline Fix

The dependency-slot-clear compare wrapper regressed on a warm target for two
separate reasons:

- it could enter the explicit legacy branch before remote holder teardown had
  fully settled, leaving stale SGX lab objects behind
- the direct legacy multi-submit path still used the truncated
  `poulsbo_gtt_state` dump to establish its "fresh GEM range" baseline

Both are now fixed and validated in:

- [docs/95-sgx-compare-reset-and-legacy-range-baseline-fix.md](/root/Poulsbo/docs/95-sgx-compare-reset-and-legacy-range-baseline-fix.md)
- [logs/vaio-compare/20260511-141658-sgx-dependency-slot-clear-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-141658-sgx-dependency-slot-clear-mode-compare)

New helper:

- [scripts/run-vaio-sgx-lab-reset.sh](/root/Poulsbo/scripts/run-vaio-sgx-lab-reset.sh)

The compare wrappers now call it before the typed branch and again between the
typed and legacy branches, recording `pre-reset.log` and `inter-reset.log`.
The direct legacy multi-submit path now also uses the typed range-query helper
for before/during live range ID baselines when that query path is available,
while keeping legacy object, submit, dependency, and verification behavior
unchanged.

Live validation result:

- `session_outcome=completed`
- `pre_reset_rc=0`
- `typed_rc=0`
- `inter_reset_rc=0`
- `legacy_rc=0`
- `legacy_health_overall=remote_usable`
- typed/default range IDs:
  - `685..709`
- explicit legacy range IDs:
  - `710..734`
- typed relative-ticket evidence:
  - `COORD_COMPLETE_TICKET_BASELINE=112`
  - `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET=114`
  - `SUBMIT_B_DEPENDENCY3_COMPLETE_TICKET=115`
  - `SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET=116`
- legacy relative-ticket evidence:
  - `COORD_COMPLETE_TICKET_BASELINE=117`
  - `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET=119`
  - `SUBMIT_B_DEPENDENCY3_COMPLETE_TICKET=120`
  - `SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET=121`
- final target state stayed clean:
  - `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Focused Compare Reset And Range-Query Follow-Up

The same reset-assisted warm-target path is now also validated for the focused
wrapper families in:

- [docs/96-sgx-focused-compare-reset-and-range-query-fix.md](/root/Poulsbo/docs/96-sgx-focused-compare-reset-and-range-query-fix.md)

Focused object compare bundle:

- [logs/vaio-compare/20260511-142409-sgx-object-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-142409-sgx-object-mode-compare)

Saved result:

- `session_outcome=completed`
- `pre_reset_rc=0`
- `typed_rc=0`
- `inter_reset_rc=0`
- `legacy_rc=0`
- `last_overall=remote_usable`

Key object-compare evidence:

- typed/default:
  - `typed_range_id=7`
  - `typed_object_id=641`
  - `typed_object_role=control`
  - `typed_object_class=packet`
  - `typed_object_gpu_use=control_ccb`
  - `typed_object_context=pf`
- explicit legacy:
  - `legacy_range_id=736`
  - `legacy_object_id=642`

Focused submit compare bundle:

- [logs/vaio-compare/20260511-144832-sgx-submit-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-144832-sgx-submit-mode-compare)

Saved result:

- `session_outcome=completed`
- `pre_reset_rc=0`
- `typed_rc=0`
- `inter_reset_rc=0`
- `legacy_rc=0`
- `legacy_health_overall=remote_usable`
- `last_overall=remote_usable`

Key submit-compare evidence:

- typed/default:
  - `typed_submit_id=131`
  - `typed_range_ids=7 9 792 793 794`
  - `typed_object_ids=672 673 674 675 676`
  - `typed_validate_count=1`
  - `typed_issue_count=1`
  - `typed_transition_count=2`
  - `typed_owner_state=complete`
- explicit legacy:
  - `legacy_queue_mode=wrap`
  - `legacy_submit_id=132`
  - `legacy_range_ids=797 798 799 800 801`
  - `legacy_object_ids=677 678 679 680 681`

This follow-up also fixed the focused legacy submit helper and the reset helper
parser paths:

- [scripts/run-vaio-sgx-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit.sh)
- [scripts/run-vaio-sgx-lab-reset.sh](/root/Poulsbo/scripts/run-vaio-sgx-lab-reset.sh)

The real fix points were:

- the focused legacy submit path now uses the typed range-query helper for
  before/during fresh GEM-range baselines whenever that query path is
  available
- both helpers now parse `range_type_name=...` instead of the nonexistent
  `range_name=...`
- both helpers now parse `range_ioctl_ret=0 errno=0` by extracting only the
  first token after the prefix, so index `0` no longer looks like a failed
  query
- the reset helper's mmapping-baseline convergence check is now validated with:
  - `range_query_live_gem_count=2`
  - `baseline_mmapping_gem_max=2`

The remaining compare-family follow-up is now also closed in:

- [docs/97-sgx-generic-multi-submit-compare-reset-follow-up.md](/root/Poulsbo/docs/97-sgx-generic-multi-submit-compare-reset-follow-up.md)
- [logs/vaio-compare/20260511-145551-sgx-multi-submit-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-145551-sgx-multi-submit-mode-compare)

Saved result:

- `session_outcome=completed`
- `pre_reset_rc=0`
- `typed_rc=0`
- `inter_reset_rc=0`
- `legacy_rc=0`
- `legacy_health_overall=remote_usable`
- `last_overall=remote_usable`

Key multi-submit evidence:

- typed/default:
  - `typed_submit_setup=ioctl`
  - `typed_object_setup=ioctl`
  - `typed_range_discovery=ioctl`
  - `typed_dependency_setup=ioctl`
  - `typed_verification_mode=ioctl`
  - `typed_dependency_interface=named`
  - `typed_submit_a_id=133`
  - `typed_submit_b_id=134`
  - `typed_range_ids=802 803 804 805 806 807 808 810 809 811`
  - `typed_submit_validate_count=11`
  - `typed_submit_issue_count=2`
  - `typed_submit_transition_count=4`
- explicit legacy:
  - `legacy_dependency_interface=named`
  - `legacy_submit_a_id=135`
  - `legacy_submit_b_id=136`
  - `legacy_range_ids=812 813 814 815 816 817 818 819 820 821`

### Submit Prepare Materialization Validation

The next larger batch moved the typed submit path from “fully described” to
“materialized into backing control/status memory” with a bounded `prepare`
submit-op ioctl.

Canonical live bundle:

- [logs/vaio-ioctl/20260511-172038-sgx-submit-prepare-live](/root/Poulsbo/logs/vaio-ioctl/20260511-172038-sgx-submit-prepare-live)

Guarded deploy result:

- `driver_srcversion=AE18527FD5F6C10BE6C85A5`
- `override_module_size=390608`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

Focused typed submit-op result:

- the focused runner now issues typed `prepare focus` after:
  - `kick`
  - `ccb`
  - `queue`
  - `status`
  - `sync`
  - `owner`
- the same live submit then proved:
  - `submit_prepare_count=1`
  - `submit_validate_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - `submit_owner_state_name=complete`

Typed multi-submit result:

- the typed multi-submit setup now also issues one typed `prepare` per submit
  before the first validate
- canonical live submit IDs in the saved rerun:
  - `submit_id=5`
  - `submit_id=6`
- kernel log evidence:
  - `lab submit prepare submit_id=5 packet_count=1 write_offset=0x0 prepare_count=1`
  - `lab submit prepare submit_id=6 packet_count=1 write_offset=0x20 prepare_count=1`
- typed live terminal state:
  - submit A:
    - `submit_prepare_count=1`
    - `submit_validate_count=11`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_owner_state_name=complete`
  - submit B:
    - `submit_prepare_count=1`
    - `submit_validate_count=11`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_owner_state_name=complete`
- final typed cleanup:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

The next execution-shaped step is now also validated: the same submit-op path
has a hardware-validated typed `arm` stage. The focused submit-op validator and
the typed multi-submit runner now both use the stricter sequence:

- `prepare`
- `arm`
- `validate`
- `issue`

Canonical live bundles:

- [logs/vaio-ioctl/20260511-175155-sgx-submit-arm-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-175155-sgx-submit-arm-op-live)
- [logs/vaio-ioctl/20260511-175233-sgx-multi-submit-arm-live](/root/Poulsbo/logs/vaio-ioctl/20260511-175233-sgx-multi-submit-arm-live)

Saved result on the VAIO:

- `driver_srcversion=270B70E360AEFF73D0C52F3`
- `override_module_size=396320`
- `overall=remote_usable`

Key focused evidence:

- after `prepare`:
  - `submit_prepare_count=1`
  - `submit_arm_valid=0`
- after `arm`:
  - `submit_arm_valid=1`
  - `submit_arm_count=1`
  - `submit_arm_prepare_count=1`
  - `submit_arm_queue_packet_count=1`
  - `submit_arm_owner_state_name=queued`
- after `issue`:
  - `submit_arm_valid=0`
- final state:
  - `submit_validate_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - `submit_owner_state_name=complete`

Key multi-submit evidence:

- all five typed live submits reached:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
- post-issue and post-transition validation returned:
  - `submit_arm_valid=0`
- dependency-gated submit B still completed cleanly with:
  - `submit_validate_count=23`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- final typed cleanup returned:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

That milestone is documented in
[docs/109-sgx-lab-submit-arm-op-ioctl.md](/root/Poulsbo/docs/109-sgx-lab-submit-arm-op-ioctl.md).

## Validated Typed SGX Submit Emit Stage

Validated on May 11, 2026 on the VAIO with the current override module
`srcversion 333CBAAFFFDE848128DC9EC`.

The bounded submit-op ioctl path now also has an explicit emitted execution
record:

- `emit` requires an already armed submit image
- `emit` snapshots the current armed queue, sync, owner, object, GPU-VA, and
  issue-side state into the submit descriptor
- `issue` now consumes that emitted record and invalidates it afterward

Canonical live bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260511-181000-sgx-submit-emit-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-181000-sgx-submit-emit-op-live)
- typed multi-submit:
  - [logs/vaio-ioctl/20260511-181100-sgx-multi-submit-emit-live](/root/Poulsbo/logs/vaio-ioctl/20260511-181100-sgx-multi-submit-emit-live)

Focused typed evidence:

- post-emit query showed:
  - `submit_emit_valid=1`
  - `submit_emit_count=1`
  - `submit_emit_cmd_type_name=ta`
  - `submit_emit_owner_state_name=queued`
  - `submit_emit_control_object_id=1`
  - `submit_emit_status_object_id=5`
- post-issue query showed:
  - `submit_emit_valid=0`
  - `submit_issue_count=1`
- final state still completed cleanly with:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_transition_count=2`

Typed multi-submit evidence:

- the typed path stayed fully enabled:
  - `SUBMIT_SETUP=ioctl`
  - `OBJECT_SETUP=ioctl`
  - `RANGE_DISCOVERY=ioctl`
  - `DEPENDENCY_SETUP=ioctl`
  - `VERIFICATION_MODE=ioctl`
- submit A finished with:
  - `submit_prepare_count=1`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
- submit B finished with:
  - `submit_prepare_count=1`
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
- while live, submit B's emitted state showed:
  - `submit_emit_dependency_active_count=1`
- final typed cleanup returned:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

Kernel log evidence from those bundles shows the new sequence directly:

- `lab submit emit submit_id=1 arm_count=1 packet_count=1 global_issue_ticket=0 dependency_active_count=0`
- `lab submit emit submit_id=2 arm_count=1 ... dependency_active_count=0`
- `lab submit emit submit_id=2 arm_count=2 ... dependency_active_count=0`
- `lab submit emit submit_id=2 arm_count=3 ... dependency_active_count=0`
- `lab submit emit submit_id=3 arm_count=1 ... dependency_active_count=1`
- `lab submit emit submit_id=3 arm_count=2 ... dependency_active_count=1`

That milestone is documented in
[docs/110-sgx-lab-submit-emit-op-ioctl.md](/root/Poulsbo/docs/110-sgx-lab-submit-emit-op-ioctl.md).

## Validated Typed SGX Submit Dispatch State

Validated on May 11, 2026 on the VAIO with the current override module
`srcversion D73A90E161CB5B4680FD013`.

The bounded typed submit path now keeps one persistent post-`issue`
execution snapshot named `dispatch`. The focused validator was also repaired
so it uses live coordination-ticket baselines instead of assuming a cold
target where global tickets always start at `0`.

Canonical live bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260511-192912-sgx-submit-dispatch-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-192912-sgx-submit-dispatch-live/summary.log)
- typed multi-submit:
  - [logs/vaio-ioctl/20260511-192947-sgx-multi-submit-dispatch-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-192947-sgx-multi-submit-dispatch-live/summary.log)

Focused typed evidence:

- coordination baseline before launch was:
  - `coord_issue_ticket=6`
  - `coord_running_ticket=6`
  - `coord_complete_ticket=6`
- the same submit finished at:
  - `coord_issue_ticket=7`
  - `coord_running_ticket=7`
  - `coord_complete_ticket=7`
- final live submit state still completed cleanly:
  - `submit_validate_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - `submit_owner_state_name=complete`
- final dispatch snapshot stayed live after `issue` and both transitions:
  - `submit_dispatch_valid=1`
  - `submit_dispatch_count=1`
  - `submit_dispatch_prepare_count=1`
  - `submit_dispatch_arm_count=1`
  - `submit_dispatch_emit_count=1`
  - `submit_dispatch_launch_count=1`
  - `submit_dispatch_owner_state_name=complete`
  - `submit_dispatch_issued_packets=1`
  - `submit_dispatch_completed_packets=2`
  - `submit_dispatch_queue_packet_count=0`
  - `submit_dispatch_sync_complete_seq=0x42`
  - `submit_dispatch_sync_pending_seq=0x42`
  - `submit_dispatch_owner_issue_cookie=0x102`
  - `submit_dispatch_owner_complete_cookie=0x102`
  - `submit_dispatch_global_issue_ticket=7`
  - `submit_dispatch_global_running_ticket=7`
  - `submit_dispatch_global_complete_ticket=7`

Typed multi-submit evidence:

- canonical live submit IDs were:
  - `SUBMIT_A_ID=7`
  - `SUBMIT_B_ID=8`
- baseline complete ticket for the scenario was:
  - `COORD_COMPLETE_TICKET_BASELINE=7`
- final dispatch-bearing state for submit A:
  - `submit_prepare_count=1`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_dispatch_owner_state_name=complete`
  - `submit_dispatch_completed_packets=1`
  - `submit_dispatch_queue_packet_count=0`
  - `submit_dispatch_dependency_active_count=0`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
- final dispatch-bearing state for submit B:
  - `submit_prepare_count=1`
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_dispatch_owner_state_name=complete`
  - `submit_dispatch_completed_packets=1`
  - `submit_dispatch_queue_packet_count=0`
  - `submit_dispatch_dependency_active_count=1`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
- final typed cleanup still returned:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

That milestone is documented in
[docs/112-sgx-lab-submit-dispatch-state.md](/root/Poulsbo/docs/112-sgx-lab-submit-dispatch-state.md).

Recovery on the VAIO console is:

```bash
rm -f /lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko
depmod -a 5.4.0-150-generic
reboot
```

## Next Work On This Target

The next useful target-side work is kernel render instrumentation, not more
Xorg debugging.

Priority order:

- use the new SGX snapshot path to map MMU/BIF/register deltas for future
  render-side experiments
- use the new typed GTT/MMU state path when future validation needs global
  live-range or MMU-counter evidence instead of page-sized text dumps
- extend the current SGX lab object and submit model with broader queue,
  synchronization, and SGX state-block semantics where hardware evidence
  justifies them
- decide whether the remaining SGX lab object-adoption convenience path and the
  remaining human-readable sysfs text dumps should stay in sysfs now that the
  bounded object-op, submit-op, cleanup-op, SGX-state query, GTT-state query,
  and multi-submit ioctl paths are all validated on hardware
- use the new typed object-query path when future validation needs exact
  object-side role/class/gpu_use/context or PF-bind evidence instead of parsing
  the page-sized object text dump
- use the new typed submit-query path when future validation needs exact
  per-submit queue, sync, owner, dependency, or ordering evidence instead of
  parsing the page-sized submit text dump
- prefer the updated submit-op validator when proving focused single-submit
  setup, attach, kick/queue/block programming, progression, or drop behavior,
  because it now uses only typed lab reads instead of the older sysfs
  verification path
- use the new typed `prepare` plus `arm` plus `emit` plus `launch` stages
  whenever future focused or multi-submit work needs one coherent
  materialized control/status image, one explicit issue-side snapshot, one
  explicit emitted execution record, and one explicit launched issue-plan
  before progression
- use the new persistent dispatch snapshot whenever future focused or
  multi-submit validation needs one post-`issue` execution image that survives
  `running` and `complete`, instead of inferring those transitions only from
  the later mutable live submit state
- prefer the default `run-vaio-sgx-submit.sh` path for focused single-submit
  work now, because it now lands on the typed submit-op validator by default;
  only force `SGX_SUBMIT_DRIVER_MODE=legacy` if you intentionally need the
  older sysfs-heavy script body
- prefer the new `run-vaio-sgx-multi-submit-nosysfs.sh` path when proving the
  full four-lane dependency-slot-clear scenario, because it now uses typed
  object, submit, dependency, range, coord, GTT, and dump helpers instead of
  the older sysfs verification path
- prefer the updated dependency-specific wrappers when proving dependency-drop,
  multi-source, indexed, third-lane, or slot-clear scenarios, because they now
  default through `run-vaio-sgx-multi-submit-default.sh`, which currently
  selects the typed path by default; only force
  `SGX_MULTI_SUBMIT_DRIVER_MODE=legacy` when you intentionally need the older
  direct `run-vaio-sgx-multi-submit.sh` behavior
- use the new mode-compare wrappers before changing wrapper dispatch or default
  mode behavior again:
  - `scripts/run-vaio-sgx-object-mode-compare.sh`
  - `scripts/run-vaio-sgx-submit-mode-compare.sh`
  - `scripts/run-vaio-sgx-multi-submit-mode-compare.sh`
  - `scripts/run-vaio-sgx-dependency-slot-clear-mode-compare.sh`
  - `scripts/run-vaio-sgx-indexed-dependency-array-mode-compare.sh`
  - `scripts/run-vaio-sgx-multi-source-ticket-dependency-mode-compare.sh`
  - that gives one saved typed-vs-legacy regression bundle per wrapper family
    instead of proving only one side in isolation
- if you intentionally use the focused legacy submit path, keep
  `RANGE_SCAN_MAX` available as the range-ID discovery escape hatch once live
  GEM IDs on the VAIO move beyond the old narrow fallback window
- apply the same rule to the direct legacy multi-submit path, which now also
  uses `RANGE_SCAN_MAX` instead of a fixed narrow fallback window
- use the new typed no-sysfs state-dump flow when future validation needs one
  human-readable end-to-end SGX lab snapshot without touching the old sysfs
  dump nodes
- use the new `json` and `capture-json` dump modes when future validation
  needs the same no-sysfs SGX lab snapshot in a machine-checked form instead
  of grepping the text dump
- prefer the updated cleanup validator when proving stale-submit or
  stale-object cleanup, because it now uses only typed lab reads instead of
  the older sysfs verification path
- prefer the updated object-op validator when proving adopt, object metadata,
  PF bind/unbind, and final drop behavior, because it now uses only typed lab
  reads instead of `poulsbo_sgx_objects` and `poulsbo_gtt_state`
- prefer the default `run-vaio-sgx-object.sh` path for focused object work
  now, because it now lands on the typed object-op validator by default; only
  force `SGX_OBJECT_DRIVER_MODE=legacy` if you intentionally need the older
  sysfs-heavy script body
- keep any still-unmoved dependency or broader state-block setup narrowly
  scoped until each piece has a similarly bounded ioctl or another clearly lab-
  only control surface
- if more state setup moves, keep it narrowly scoped to already validated lab
  object or submit operations instead of claiming a broad public render ABI
- extend the new status-block model beyond one TA region and one 3D region
  toward more realistic queue or sync ownership semantics
- decide whether the now-validated issue/running/complete plus
  source-submit completion-dependency semantics plus the explicit dependency
  state block, live-update path, and missing-source state need any broader
  fence, generation, or queue-ownership model beyond the current per-submit
  epoch plus shared counters; the next immediate dependency question is whether
  the now-validated indexed four-slot interface is enough or whether the lab
  path should move to a broader dependency array or fence block
- keep submission work validate-only until reset, IRQ, and fault evidence is
  strong enough to justify real SGX execution attempts
- the next render-side kernel question is now beyond read-side inspection and
  beyond materialized lab setup:
  - define the first execution-oriented SGX structure after `prepare` plus
    `arm` plus `emit` plus `launch` plus `dispatch`, not another small query
    or wrapper extension
- only return to DRI2/Xorg/glamor once the kernel can expose a real render path

That is consistent with the upstream `gma500` code shape: this target is
currently a display/KMS+dumb-buffer driver, not a render driver.

## 2026-05-11 Cleanup Deadlock Follow-Up

The typed submit cleanup hang was traced to a real kernel self-deadlock in
`poulsbo_sgx_submits_show()`: the sysfs dump path took
`sgx_lab_submit_mutex` twice and only unlocked it once. One stuck
`cat /sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits` reader then
held the submit mutex forever, and later typed cleanup calls blocked behind
it in `poulsbo_sgx_drop_all_submits_count`.

The local fix is now in:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

The main mutation/reset runners now also serialize on the target with
`flock(1)` through:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
- [run-vaio-sgx-ioctl-object-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-object-op.sh)
- [run-vaio-sgx-ioctl-cleanup.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-cleanup.sh)
- [run-vaio-sgx-lab-reset.sh](/root/Poulsbo/scripts/run-vaio-sgx-lab-reset.sh)

The deadlock-fix candidate override is now fully validated on the target.

Guarded redeploy returned cleanly with candidate module:

- `srcversion BB080942FE121033007E2EC`
- `override_module_size=408344`

The guarded deploy session is:

- [logs/vaio-deploy/20260511-203613-deadlock-fix-redeploy](/root/Poulsbo/logs/vaio-deploy/20260511-203613-deadlock-fix-redeploy)

The focused typed submit validator then passed on the candidate override:

- [logs/vaio-ioctl/20260511-203809-sgx-submit-op-deadlock-fix-candidate/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-203809-sgx-submit-op-deadlock-fix-candidate/summary.log)

That run proved:

- `submit_prepare_count=1`
- `submit_arm_count=1`
- `submit_emit_count=1`
- `submit_launch_count=1`
- `submit_dispatch_count=1`
- `submit_retired_count=1`
- `submit_retired_owner_state_name=complete`
- final zero cleanup

The safe typed/default multi-submit wrapper also now passes on the same
candidate override:

- [logs/vaio-ioctl/20260511-204244-sgx-multi-submit-default-deadlock-fix-candidate-rerun/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-204244-sgx-multi-submit-default-deadlock-fix-candidate-rerun/summary.log)

That rerun proved:

- submit A completed the two-refill path with:
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
- submit B completed the dependency-gated path with:
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
- final typed cleanup returned:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

The only extra fix needed in this follow-up was runner-side: the typed
refill-cycle verification had stale retired-snapshot expectations and now uses
explicit snapshot helpers plus a persisted-retired assertion path. The full
write-up is:

- [docs/115-sgx-deadlock-fix-candidate-validation.md](/root/Poulsbo/docs/115-sgx-deadlock-fix-candidate-validation.md)

The raw implementation script is still locally guarded: direct legacy/sysfs
execution requires `ALLOW_DIRECT_LEGACY_MULTI_SUBMIT=1`, while the default
dispatcher [run-vaio-sgx-multi-submit-default.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit-default.sh)
keeps the old path only behind the explicit
`SGX_MULTI_SUBMIT_DRIVER_MODE=legacy` branch.

The next execution-shaped batch is now also validated on the same target:
a memory-backed typed submit `report` block.

The guarded deploy session is:

- [logs/vaio-deploy/20260511-212444-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260511-212444-guarded-deploy)

The current live override after that deploy is:

- `srcversion E774FBBBD966270FEA6C67B`
- `override_module_size=416736`

Canonical report-validation bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260511-212632-sgx-submit-report-op-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-212632-sgx-submit-report-op-live/summary.log)
- typed multi-submit:
  - [logs/vaio-ioctl/20260511-212931-sgx-multi-submit-report-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-212931-sgx-multi-submit-report-live/summary.log)

That batch proved:

- focused report-stage progression:
  - `none -> prepared -> armed -> emitted -> launched -> dispatched -> retired`
- focused report-flag progression:
  - `0x0 -> 0x1 -> 0x3 -> 0x7 -> 0x1c -> 0x10 -> 0x30`
- typed multi-submit refill cycles now re-expose:
  - `prepared`
  - `armed`
  - `emitted`
  - `launched`
  even after an earlier cycle has already reached `retired`
- final typed cleanup still returned:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

The full write-up is:

- [docs/116-sgx-lab-submit-report-op-ioctl.md](/root/Poulsbo/docs/116-sgx-lab-submit-report-op-ioctl.md)

The next execution-shaped batch is now also validated on the same target:
the current fixed CCB payload has been expanded into an SGX-shaped
non-executing 8-dword record.

The guarded deploy session is:

- [logs/vaio-deploy/20260511-214857-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260511-214857-guarded-deploy)

The current live override after that deploy is:

- `srcversion 111E4E9C4E0999DA366432F`
- `override_module_size=418808`

Canonical CCB-record validation bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260511-215431-sgx-submit-ccb-record-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-215431-sgx-submit-ccb-record-live/summary.log)
- typed multi-submit:
  - [logs/vaio-ioctl/20260511-215509-sgx-multi-submit-ccb-record-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-215509-sgx-multi-submit-ccb-record-live/summary.log)

That batch proved:

- the focused typed path now materializes one memory-backed 8-dword CCB
  record at `prepare` time, and query readback matches the attached object
  GPU VAs:
  - `submit_ccb_pds_gpu_va32=0x81123000`
  - `submit_ccb_use_gpu_va32=0x81127000`
  - `submit_ccb_target_gpu_va32=0x8112b000`
  - `submit_ccb_status_gpu_va32=0x8112f000`
  - matching:
    - `submit_pds_gpu_va=0x81123000`
    - `submit_use_gpu_va=0x81127000`
    - `submit_target_gpu_va=0x8112b000`
    - `submit_status_gpu_va=0x8112f000`
- the same focused path still completed cleanly:
  - `submit_prepare_count=1`
  - `submit_validate_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- the typed multi-submit path now validates the same expanded record for both
  live submits:
  - submit A final CCB GPU-VA32 words:
    - `0x8112b000`
    - `0x8112f000`
    - `0x81133000`
    - `0x81137000`
  - submit B final CCB GPU-VA32 words:
    - `0x8113f000`
    - `0x81143000`
    - `0x81147000`
    - `0x8114b000`
  - final progression counts:
    - submit A:
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B:
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_emit_dependency_active_count=1`
- final typed cleanup still returned:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

The full write-up is:

- [docs/117-sgx-lab-ccb-record-expansion.md](/root/Poulsbo/docs/117-sgx-lab-ccb-record-expansion.md)

## 2026-05-11: SGX Lab Submit Manifest Block

The next execution-shaped batch is now also live and validated on the VAIO.

Guarded redeploy:

- [logs/vaio-deploy/20260511-222440-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260511-222440-guarded-deploy)
- previous override:
  - `srcversion 111E4E9C4E0999DA366432F`
  - `override_module_size=418808`
- rebuilt override:
  - `srcversion 9E29F79593770BEEC5AEFD2`
  - `override_module_size=423020`

Canonical live bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260511-222845-sgx-submit-manifest-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-222845-sgx-submit-manifest-live/summary.log)
- typed multi-submit:
  - [logs/vaio-ioctl/20260511-222923-sgx-multi-submit-manifest-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-222923-sgx-multi-submit-manifest-live/summary.log)

Validated result:

- the typed submit-op path now has a memory-backed `manifest` block at
  `submit_manifest_offset=0x180`
- focused single-submit validation now proves control-side manifest stage
  progression end to end:
  - `none`
  - `prepared`
  - `armed`
  - `emitted`
  - `launched`
  - `dispatched`
  - `retired`
- the same focused run also proved the live queue and owner transitions in the
  manifest:
  - after `issue`:
    - `submit_manifest_owner_state_name=queued`
    - `submit_manifest_queue_packet_count=2`
  - after `transition running`:
    - `submit_manifest_owner_state_name=running`
    - `submit_manifest_queue_packet_count=1`
  - after `transition complete`:
    - `submit_manifest_owner_state_name=complete`
    - `submit_manifest_queue_packet_count=0`
- the manifest GPU-VA32 words matched the expanded CCB record exactly:
  - `submit_manifest_pds_gpu_va32=0x80c6f000`
  - `submit_manifest_use_gpu_va32=0x81123000`
  - `submit_manifest_target_gpu_va32=0x81127000`
  - `submit_manifest_status_gpu_va32=0x8112b000`
- the focused path still completed cleanly:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- the typed multi-submit path validated the same block through refill and
  dependency gating:
  - submit A final:
    - `submit_manifest_stage_name=retired`
    - `submit_manifest_stage_count=3`
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_manifest_stage_name=retired`
    - `submit_manifest_stage_count=2`
    - `submit_manifest_dependency_active_count=1`
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
- final typed cleanup still returned:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

The full write-up is:

- [docs/118-sgx-lab-submit-manifest-op-ioctl.md](/root/Poulsbo/docs/118-sgx-lab-submit-manifest-op-ioctl.md)

## Validated Descriptor Submit Path

Validated on May 11, 2026 on the already-deployed descriptor-capable override,
with one follow-up userspace rebuild to fix the focused-selector path:

- `driver_srcversion=9E540439CC3313278F803B0`
- `override_module_size=427328`
- `rollback_pending_present=0`

Canonical live bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260511-230546-sgx-submit-descriptor-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-230546-sgx-submit-descriptor-live/summary.log)
- typed multi-submit:
  - [logs/vaio-ioctl/20260511-230625-sgx-multi-submit-descriptor-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-230625-sgx-multi-submit-descriptor-live/summary.log)

Validated result:

- the typed submit-op path now has a memory-backed `descriptor` block at
  `submit_descriptor_offset=0x200`
- focused single-submit validation now proves control-side descriptor stage
  progression end to end:
  - `none`
  - `prepared`
  - `armed`
  - `emitted`
  - `launched`
  - `dispatched`
  - `retired`
- the focused run also proved the descriptor GPU-VA32 words match the expanded
  CCB record and the attached objects:
  - `submit_descriptor_control_gpu_va32=0x80c6f000`
  - `submit_descriptor_pds_gpu_va32=0x81123000`
  - `submit_descriptor_use_gpu_va32=0x81127000`
  - `submit_descriptor_target_gpu_va32=0x8112b000`
  - `submit_descriptor_status_gpu_va32=0x8112f000`
  - `submit_ccb_pds_gpu_va32=0x81123000`
  - `submit_ccb_use_gpu_va32=0x81127000`
  - `submit_ccb_target_gpu_va32=0x8112b000`
  - `submit_ccb_status_gpu_va32=0x8112f000`
- the focused path still completed cleanly:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- the typed multi-submit path validated the same block through dependency
  gating and refill:
  - `SUBMIT_A_ID=6`
  - `SUBMIT_B_ID=7`
  - the dependency-gated path hit:
    - `submit_descriptor_dependency_active_count=1`
    - `submit_emit_dependency_active_count=1`
  - submit A final:
    - `submit_descriptor_stage_name=retired`
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_descriptor_stage_name=retired`
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
- final typed cleanup still returned:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

The full write-up is:

- [docs/119-sgx-lab-submit-descriptor-op-ioctl.md](/root/Poulsbo/docs/119-sgx-lab-submit-descriptor-op-ioctl.md)

## May 11, 2026: submit `plan` batch landed locally, deployed once, target blocked

The next execution-shaped control-side batch adds a memory-backed typed
`plan` block above the existing `report + manifest + descriptor + expanded
8-dword CCB` model. The code landed in the kernel tree and userspace helpers,
the local workload and module builds both passed, and the guarded deploy
completed successfully.

The VAIO came back after deploy with:

- `driver_srcversion=3D21987EBD495B4E78FBBA7`
- `override_module_size=433996`
- `rollback_pending_present=0`

The first focused validation attempt was then started with
`./scripts/run-vaio-sgx-ioctl-submit-op.sh`, but the machine dropped out of
service during that run and did not return during six bounded health polls on
May 11, 2026:

- `2026-05-11T23:37:34+03:00`: `overall=ssh_port_down`
- `2026-05-11T23:37:54+03:00`: `overall=ssh_port_down`
- `2026-05-11T23:38:13+03:00`: `overall=ssh_port_down`
- `2026-05-11T23:38:33+03:00`: `overall=ssh_port_down`
- `2026-05-11T23:38:52+03:00`: `overall=ssh_port_down`
- `2026-05-11T23:39:11+03:00`: `overall=ssh_port_down`

So the `plan` batch is currently:

- implemented
- locally verified
- deployed once
- not yet hardware-validated end to end

The full batch note is:

- [docs/120-sgx-lab-submit-plan-op-ioctl.md](/root/Poulsbo/docs/120-sgx-lab-submit-plan-op-ioctl.md)

To avoid repeating another full blind rerun after console recovery, the focused
typed submit runner now supports `STOP_AFTER_STAGE=<stage>` and there is a
dedicated phase-isolation wrapper at
`./scripts/run-vaio-sgx-submit-op-phase-matrix.sh`. That harness reruns the
same focused path one stage at a time, probes target health after each stage,
and is the first command that should be used once the machine is reachable
again.

The crash-isolation note is:

- [docs/121-sgx-submit-plan-crash-isolation-harness.md](/root/Poulsbo/docs/121-sgx-submit-plan-crash-isolation-harness.md)

There is now also a recovery-session wrapper that packages the intended first
post-recovery flow into one saved bundle:

- [docs/122-sgx-plan-recovery-session-wrapper.md](/root/Poulsbo/docs/122-sgx-plan-recovery-session-wrapper.md)

The preferred first post-recovery validator is now the lighter prefix-bisect
wrapper rather than the full phase matrix:

- [docs/123-sgx-submit-plan-bisect-harness.md](/root/Poulsbo/docs/123-sgx-submit-plan-bisect-harness.md)

There is now also a composed first-pass investigation wrapper that runs bisect
and then a narrowed confirmation step:

- [docs/124-sgx-submit-plan-investigation-harness.md](/root/Poulsbo/docs/124-sgx-submit-plan-investigation-harness.md)

For the next risky full-cycle attempt, there is now also a parent wrapper that
starts at guarded deploy and only then runs the `plan` investigation:

- [docs/125-sgx-plan-guarded-deploy-session-wrapper.md](/root/Poulsbo/docs/125-sgx-plan-guarded-deploy-session-wrapper.md)
- [docs/126-sgx-plan-investigation-parent-session-validation.md](/root/Poulsbo/docs/126-sgx-plan-investigation-parent-session-validation.md)

For operator use there is now also one top-level `plan` session entrypoint that
selects recovery versus full-cycle deploy automatically from the current target
health:

- [docs/127-sgx-plan-session-entrypoint.md](/root/Poulsbo/docs/127-sgx-plan-session-entrypoint.md)

While the target is still down, the kernel-side local follow-up has now
hardened both sides of the submit memory model around the blocked `plan`
batch:

- control object layout:
  - [docs/128-sgx-submit-control-layout-hardening.md](/root/Poulsbo/docs/128-sgx-submit-control-layout-hardening.md)
- status object layout:
  - [docs/129-sgx-submit-status-layout-hardening.md](/root/Poulsbo/docs/129-sgx-submit-status-layout-hardening.md)

Both batches are build-clean locally and intended to make the first recovery
run fail at one bounded kernel validation point instead of silently accepting
overlapping control or status blocks.

The same local candidate now also carries a validator stack split so those
backing-image checks no longer sit on one monolithic submit-validator frame:

- [docs/130-sgx-submit-validator-stack-split.md](/root/Poulsbo/docs/130-sgx-submit-validator-stack-split.md)

The same local candidate now also preflights the submit-view refresh pass so a
late `manifest`, `descriptor`, or `plan` failure does not happen after earlier
view blocks were already rewritten:

- [docs/131-sgx-submit-refresh-preflight.md](/root/Poulsbo/docs/131-sgx-submit-refresh-preflight.md)

The same local candidate now also normalizes `report` onto the same structured
backing-image path as `manifest`, `descriptor`, and `plan`, so refresh
preflight builds all four memory-backed submit views before the first write and
status-side validation compares `report` as one bounded image instead of as a
field-by-field special case:

- [docs/132-sgx-submit-report-struct-normalization.md](/root/Poulsbo/docs/132-sgx-submit-report-struct-normalization.md)

The same local candidate now also normalizes the remaining fixed submit blocks
onto structured helper paths, so queue header, CCB record, sync, and owner all
use matching build/read/write helpers across configure, prepare, issue,
transition, validation, and query readback:

- [docs/133-sgx-submit-fixed-block-struct-normalization.md](/root/Poulsbo/docs/133-sgx-submit-fixed-block-struct-normalization.md)

The same local candidate now also normalizes dependency-state slots onto the
same structured helper pattern, so dependency snapshot build, on-object
write/readback, submit-state mirror, validation, and clear all use one bounded
4-dword image model:

- [docs/134-sgx-submit-dependency-block-struct-normalization.md](/root/Poulsbo/docs/134-sgx-submit-dependency-block-struct-normalization.md)

On May 12, 2026, the VAIO came back to `overall=remote_usable` on override
`srcversion 2BE09D0DC1CCBDB51D2328E`, and the top-level `plan` operator path
was rerun in recovery mode on the already-loaded candidate.

The first live recovery-mode session
[logs/vaio-plan-session/20260512-104039-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-104039-sgx-plan-session)
already isolated the first bad cumulative phase to `sync`, but the focused
submit-op runner still leaked local heredoc expansion noise.

That runner-side bug was then fixed, along with missing parent-session
`preflight_overall`, `last_overall`, and `final_driver_srcversion` propagation,
and the clean rerun
[logs/vaio-plan-session/20260512-104449-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-104449-sgx-plan-session)
confirmed the same result without shell-noise:

- bisect bundle:
  [logs/vaio-plan-session/20260512-104449-sgx-plan-session/children/20260512-104502-recovery/children/20260512-104502-bisect](/root/Poulsbo/logs/vaio-plan-session/20260512-104449-sgx-plan-session/children/20260512-104502-recovery/children/20260512-104502-bisect)
  - `session_outcome=bisect_identified_bad_prefix`
  - `probe_order=prepare,report,sync`
  - `first_bad_phase=sync`
  - `preflight_overall=remote_usable`
  - `last_overall=remote_usable`
- confirm bundle:
  [logs/vaio-plan-session/20260512-104449-sgx-plan-session/children/20260512-104502-recovery/children/20260512-104638-confirm](/root/Poulsbo/logs/vaio-plan-session/20260512-104449-sgx-plan-session/children/20260512-104502-recovery/children/20260512-104638-confirm)
  - `session_outcome=phase_failed`
  - `last_phase=sync`
  - `last_phase_rc=1`
  - `last_phase_health=remote_usable`
- parent session:
  - `session_outcome=validation_failed`
  - `preflight_overall=remote_usable`
  - `last_overall=remote_usable`
  - `final_driver_srcversion=2BE09D0DC1CCBDB51D2328E`

One more capture batch then taught the bisect, confirm, recovery, and top-level
parent wrappers to preserve stderr and explicit failed-step fields from the
focused submit runner. The corrected clean rerun is:

- [logs/vaio-plan-session/20260512-111909-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-111909-sgx-plan-session)

That parent session now records the real blocker directly:

- `selected_failed_phase=sync`
- `selected_failed_step=submit_op_kick_focus`
- `selected_failed_rc=1`

The corrected child results also now record the same live substep:

- bisect:
  - `probe_order=prepare,report,sync`
  - `last_good_phase=none`
  - `first_bad_phase=sync`
  - `first_bad_failed_step=submit_op_kick_focus`
- confirm:
  - `last_phase=sync`
  - `last_phase_failed_step=submit_op_kick_focus`

So the current `plan` state is no longer “target outage” or even just
“generic sync-stage failure”. It is now “clean live `kick focus` `-EINVAL`
inside the `sync` prefix on a remotely usable candidate”.

The write-ups are:

- [docs/135-sgx-plan-sync-failure-clean-rerun.md](/root/Poulsbo/docs/135-sgx-plan-sync-failure-clean-rerun.md)
- [docs/136-sgx-plan-kick-failure-clean-rerun.md](/root/Poulsbo/docs/136-sgx-plan-kick-failure-clean-rerun.md)

The operator wrapper stack around those `plan` sessions is now hardened too:

- interrupted parent sessions now still write fallback `session-result.env`
- remaining timeout-sensitive local `tee` capture sites are removed
- rollback autorestore sessions now use collision-safe directory allocation
- `scripts/summarize-vaio-sessions.sh` now shows older incomplete bundles as
  `interrupted_or_incomplete`

Documented in:

- [docs/140-sgx-session-result-finalization-and-summary-hardening.md](/root/Poulsbo/docs/140-sgx-session-result-finalization-and-summary-hardening.md)

## 2026-05-12 Panic Capture And Auto-Reboot Validation

The VAIO is now configured to reboot automatically after a kernel panic and to
collect a crash dump for offline inspection.

Validated setup bundle:

- [logs/vaio-panic/20260512-221904-panic-capture-setup](/root/Poulsbo/logs/vaio-panic/20260512-221904-panic-capture-setup)

That setup session:

- installed `kdump-tools`, `kexec-tools`, and `makedumpfile`
- persisted:
  - `kernel.panic=10`
  - `kernel.panic_on_oops=1`
  - `kernel.sysrq=1`
- added boot arguments:
  - `crashkernel=128M`
  - `panic=10`
  - `panic_on_oops=1`
- enabled the target-side collector service:
  - `poulsbo-panic-collect.service`

Current validated crash-capture state on the target:

- `proc_cmdline=... crashkernel=128M panic=10 panic_on_oops=1 ...`
- `kernel_panic=10`
- `kernel_panic_on_oops=1`
- `kernel_sysrq=1`
- `kdump_tools_installed=1`
- `kdump_service_enabled=enabled`
- `kdump_service_active=active`
- `kexec_crash_loaded=1`
- `kdump-config status -> current state   : ready to kdump`

The end-to-end destructive validation bundle is:

- [logs/vaio-panic-test/20260512-222708-panic-test](/root/Poulsbo/logs/vaio-panic-test/20260512-222708-panic-test)

That session intentionally triggered `echo c > /proc/sysrq-trigger`, observed
the target drop to `overall=ssh_port_down`, then watched it return to
`overall=remote_usable` automatically.

Ubuntu kdump on this target does not save the crash as a literal `vmcore`
file. The real saved crash layout is:

- `/var/crash/202605122227/dump.202605122227` (`27153539` bytes)
- `/var/crash/202605122227/dmesg.202605122227` (`101171` bytes)

The collector/fetch/state layer was then widened to treat both classic
`vmcore` dumps and Ubuntu `dump.<timestamp>` dumps as canonical panic artifacts.
The refreshed collector validation bundle is:

- [logs/vaio-panic/20260512-223405-panic-capture-setup](/root/Poulsbo/logs/vaio-panic/20260512-223405-panic-capture-setup)

That rerun proved the collector now resolves:

- `panic_latest_dump_present=1`
- `panic_latest_dump_kind=dumpfile`
- `panic_latest_dump_path=/var/crash/202605122227/dump.202605122227`
- `panic_latest_dump_dmesg_path=/var/crash/202605122227/dmesg.202605122227`

The panic-test bundle was also backfilled with the saved crash dump under:

- [logs/vaio-panic-test/20260512-222708-panic-test/panic-artifacts/crashdump/202605122227](/root/Poulsbo/logs/vaio-panic-test/20260512-222708-panic-test/panic-artifacts/crashdump/202605122227)

This means future risky SGX validation can rely on:

- automatic reboot after panic
- preserved previous-boot kernel log
- saved crash dump plus crash dmesg under `/var/crash`
- one-command local fetch through
  [scripts/fetch-vaio-panic-artifacts.sh](/root/Poulsbo/scripts/fetch-vaio-panic-artifacts.sh)

## 2026-05-12 Fixed-Module `plan` Line Through Terminal `complete`

After the panic-safe target path was in place, the exact fixed-module `plan`
line was continued on live override `srcversion 6E95EDFFFB5384C8E58F25B`.

Validated exact bundles:

- [logs/vaio-plan-session/20260512-225457-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-225457-sgx-plan-session)
- [logs/vaio-plan-session/20260512-225600-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-225600-sgx-plan-session)
- [logs/vaio-plan-session/20260512-225704-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-225704-sgx-plan-session)
- [logs/vaio-plan-session/20260512-225809-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-225809-sgx-plan-session)
- [logs/vaio-plan-session/20260512-225915-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-225915-sgx-plan-session)
- [logs/vaio-plan-session/20260512-230021-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-230021-sgx-plan-session)

Those exact reruns now prove the current fixed module clears:

- `emit`
- `validate`
- `launch`
- `issue`
- `running`
- `complete`

Combined with the earlier fixed-module exact reruns, the whole current
single-submit `plan` ladder now validates on hardware:

- `plan_query_offset`
- `plan_query_config`
- `plan_query_snapshot`
- `prepare_op`
- `prepare_query`
- `arm`
- `emit`
- `validate`
- `launch`
- `issue`
- `running`
- `complete`

The same batch also exposed and fixed one offline resolver bug: after a clean
terminal `complete`, the saved current-boundary view used to fall back to
`post_descriptor`. It now stays terminal:

- `resolved_profile=complete`
- `resolved_reason=latest_phase_evidence_terminal`

That means the next useful target-side work is beyond exact single-submit
phase narrowing, not another no-code repeat of the same ladder.

## 2026-05-13 Shared Submit View-Record Collection

The next kernel batch on top of that fixed-module line now shares one
collection pass for the typed submit `live`, `plan`, and `result` execution
records.

Validated guarded deploy:

- [logs/vaio-deploy/20260513-012143-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-012143-guarded-deploy)

That deploy booted the VAIO back to:

- `driver_srcversion=7E90BF7EA4740AF68A6B588`
- `override_module_size=438196`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

The exact no-redeploy `plan` regression proof on that same module is:

- [logs/vaio-plan-session/20260513-012335-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-012335-sgx-plan-session)

That proof still cleared terminal `complete`:

- `session_outcome=completed`
- `selected_phase_profile=complete`
- `final_driver_srcversion=7E90BF7EA4740AF68A6B588`

Focused typed submit validation is now saved in:

- [logs/vaio-ioctl/20260513-012435-sgx-submit-view-records-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-012435-sgx-submit-view-records-live/summary.log)

Final focused state:

- `SUBMIT_ID=4`
- `submit_prepare_count=1`
- `submit_arm_count=1`
- `submit_emit_count=1`
- `submit_launch_count=1`
- `submit_dispatch_count=1`
- `submit_retired_count=1`
- `submit_issue_count=1`
- `submit_transition_count=2`
- `submit_result_stage_name=retired`
- `submit_result_source_name=retired`
- `submit_result_owner_state_name=complete`
- `coord_live_submit_count=0`
- `gtt_state_range_sgx_object_count=0`

Typed multi-submit validation is now saved in:

- [logs/vaio-ioctl/20260513-012435-sgx-multi-submit-view-records-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-012435-sgx-multi-submit-view-records-live/summary.log)

Final typed multi-submit state:

- `SUBMIT_A_ID=2`
- `SUBMIT_B_ID=3`
- submit A final:
  - `submit_prepare_count=1`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
  - `submit_result_stage_name=retired`
  - `submit_result_source_name=retired`
  - `submit_result_owner_state_name=complete`
- submit B final:
  - `submit_prepare_count=1`
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
  - `submit_result_stage_name=retired`
  - `submit_result_source_name=retired`
  - `submit_result_owner_state_name=complete`
- final cleanup:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

The saved current-boundary view therefore stays terminal on the new module:

- `resolved_profile=complete`
- `resolved_reason=latest_phase_evidence_terminal`
- `source_timestamp=20260513-012335`

## 2026-05-13 SGX Submit CMDTA Kick Object

The next kernel batch on top of the shared `exec-packet` line now adds one
internal TA-specific object:

- `psb_sgx_lab_submit_cmdta_kick`

Validated guarded deploy:

- [logs/vaio-deploy/20260513-032841-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-032841-guarded-deploy)

That deploy booted the VAIO back to:

- `driver_srcversion=C1CC8033D0035B46726BAB0`
- `override_module_size=447532`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

The exact no-redeploy `plan` regression proof on that same module is:

- [logs/vaio-plan-session/20260513-033102-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-033102-sgx-plan-session)

That proof still cleared terminal `complete`:

- `session_outcome=completed`
- `selected_phase_profile=complete`
- `final_driver_srcversion=C1CC8033D0035B46726BAB0`

Focused typed submit validation is now saved in:

- [logs/vaio-ioctl/20260513-033202-sgx-submit-cmdta-kick-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-033202-sgx-submit-cmdta-kick-live/summary.log)

Final focused state:

- `submit_prepare_count=1`
- `submit_arm_count=1`
- `submit_emit_count=1`
- `submit_launch_count=1`
- `submit_dispatch_count=1`
- `submit_retired_count=1`
- `submit_issue_count=1`
- `submit_transition_count=2`
- `submit_live_stage=6`
- `submit_result_stage=6`
- `submit_manifest_stage=6`
- `submit_descriptor_stage=6`
- `submit_plan_stage=6`
- `submit_issue_plan_stage_name=retired`
- `submit_issue_plan_stage_count=1`
- `coord_live_submit_count=0`

Typed multi-submit validation is now saved in:

- [logs/vaio-ioctl/20260513-033241-sgx-multi-submit-cmdta-kick-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-033241-sgx-multi-submit-cmdta-kick-live/summary.log)

Final typed multi-submit state:

- `SUBMIT_A_ID=3`
- `SUBMIT_B_ID=4`
- submit A final:
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
  - `submit_issue_plan_stage_name=retired`
  - `submit_issue_plan_stage_count=3`
- submit B final:
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
  - `submit_issue_plan_stage_name=retired`
  - `submit_issue_plan_stage_count=2`
- final cleanup:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

The saved current-boundary view therefore stays terminal on the new module:

- `resolved_profile=complete`
- `resolved_reason=latest_phase_evidence_terminal`
- `source_timestamp=20260513-033102`

## 2026-05-13 SGX Submit CCB Kick Object

The next kernel batch on top of the shared `cmdta-kick` line now adds one
internal kick-shaped object:

- `psb_sgx_lab_submit_ccb_kick`

Validated guarded deploy:

- [logs/vaio-deploy/20260513-034651-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-034651-guarded-deploy)

That deploy booted the VAIO back to:

- `driver_srcversion=8B053004C83E74C5824A815`
- `override_module_size=448296`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

The exact no-redeploy `plan` regression proof on that same module is:

- [logs/vaio-plan-session/20260513-034951-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-034951-sgx-plan-session)

That proof still cleared terminal `complete`:

- `session_outcome=completed`
- `selected_phase_profile=complete`
- `final_driver_srcversion=8B053004C83E74C5824A815`

Focused typed submit validation is now saved in:

- [logs/vaio-ioctl/20260513-035055-sgx-submit-ccb-kick-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-035055-sgx-submit-ccb-kick-live/summary.log)

Final focused state:

- `submit_prepare_count=1`
- `submit_arm_count=1`
- `submit_emit_count=1`
- `submit_launch_count=1`
- `submit_dispatch_count=1`
- `submit_retired_count=1`
- `submit_issue_count=1`
- `submit_transition_count=2`
- `submit_live_stage=6`
- `submit_result_stage=6`
- `submit_manifest_stage=6`
- `submit_descriptor_stage=6`
- `submit_plan_stage=6`
- `submit_issue_plan_stage_name=retired`
- `submit_issue_plan_stage_count=1`
- `coord_live_submit_count=0`
- `gtt_state_range_sgx_object_count=0`

Typed multi-submit validation is now saved in:

- [logs/vaio-ioctl/20260513-035151-sgx-multi-submit-ccb-kick-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-035151-sgx-multi-submit-ccb-kick-live/summary.log)

Final typed multi-submit state:

- submit B final retained explicit dependency state through the normalized
  views:
  - `submit_result_dependency_active_count=1`
  - `submit_manifest_dependency_active_count=1`
  - `submit_descriptor_dependency_active_count=1`
  - `submit_plan_dependency_active_count=1`
  - `submit_dependency_active_count=1`
  - `dependency[1]_submit_id=3`
  - `dependency[1]_required_ticket=3`
  - `dependency[1]_observed_complete_ticket=5`
  - `dependency[1]_flags=0xf`
- final cleanup:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

The saved current-boundary view therefore stays terminal on the new module:

- `resolved_profile=complete`
- `resolved_reason=latest_phase_evidence_terminal`
- `source_timestamp=20260513-034951`

## 2026-05-13 SGX Submit Device Sync Objects

The next kernel batch kept the same six-layer SGX-shaped execution stack, but
normalized the top shared layer around explicit device-sync objects and the
hardware destination-sync list:

- `psb_sgx_lab_submit_sgx_device_sync_object`
- `psb_sgx_lab_submit_sgx_hwdevice_sync_list`
- updated `psb_sgx_lab_submit_sgx_cmdta_shared`

That batch also removed the temporary validator stack-frame regression by
moving the `ccb_kick` scratch object in
`poulsbo_sgx_validate_submit_sgx_cmdta_shared_locked()` off stack.

Validated guarded deploy:

- [logs/vaio-deploy/20260513-052151-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-052151-guarded-deploy)

That deploy booted the VAIO back to:

- `driver_srcversion=41BF53B53E121CEEB0F3594`
- `override_module_size=449960`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

The exact no-redeploy `plan` regression proof on that same module is:

- [logs/vaio-plan-session/20260513-052334-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-052334-sgx-plan-session)

That proof still cleared terminal `complete`:

- `session_outcome=completed`
- `selected_phase_profile=complete`
- `final_driver_srcversion=41BF53B53E121CEEB0F3594`

Focused typed submit validation is now saved in:

- [logs/vaio-ioctl/20260513-052451-sgx-submit-device-sync-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-052451-sgx-submit-device-sync-live/summary.log)

Final focused state:

- `submit_prepare_count=1`
- `submit_arm_count=1`
- `submit_emit_count=1`
- `submit_launch_count=1`
- `submit_dispatch_count=1`
- `submit_retired_count=1`
- `submit_issue_count=1`
- `submit_transition_count=2`
- `submit_live_stage_name=retired`
- `submit_result_stage_name=retired`
- `submit_manifest_stage_name=retired`
- `submit_descriptor_stage_name=retired`
- `submit_plan_stage_name=retired`
- `submit_issue_plan_stage_name=retired`
- `submit_issue_plan_dependency_active_count=0`
- `coord_live_submit_count=0`
- `gtt_state_range_sgx_object_count=0`

Typed multi-submit validation is now saved in:

- [logs/vaio-ioctl/20260513-052529-sgx-multi-submit-device-sync-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-052529-sgx-multi-submit-device-sync-live/summary.log)

Final typed multi-submit state:

- `SUBMIT_A_ID=3`
- `SUBMIT_B_ID=4`
- submit A final:
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
  - `submit_issue_plan_stage_name=retired`
  - `submit_dependency_active_count=0`
- submit B final:
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
  - `submit_result_dependency_active_count=1`
  - `submit_manifest_dependency_active_count=1`
  - `submit_descriptor_dependency_active_count=1`
  - `submit_plan_dependency_active_count=1`
  - `submit_dependency_active_count=1`
  - `submit_issue_plan_stage_name=retired`
- final cleanup:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

The saved current-boundary view therefore stays terminal on the new module:

- `resolved_profile=complete`
- `resolved_reason=latest_phase_evidence_terminal`
- `source_timestamp=20260513-052334`

## 2026-05-13 SGX Submit CMDTA Object

The next kernel batch added one new internal SGX-shaped execution object above
the shared layer:

- `psb_sgx_lab_submit_sgx_cmdta`

That object embeds `psb_sgx_lab_submit_sgx_cmdta_shared` and materializes the
bounded TA-destination and 3D-source sync-object arrays the current lab model
supports today. The submit validator now consumes that full `sgx_cmdta`
object instead of validating only the lower shared layer.

Validated guarded deploy:

- [logs/vaio-deploy/20260513-053804-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-053804-guarded-deploy)

That deploy booted the VAIO back to:

- `driver_srcversion=2DA0FEA89EBE73F96BE4053`
- `override_module_size=450568`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

The exact no-redeploy `plan` regression proof on that same module is:

- [logs/vaio-plan-session/20260513-053956-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-053956-sgx-plan-session)

That proof still cleared terminal `complete`:

- `session_outcome=completed`
- `selected_phase_profile=complete`
- `final_driver_srcversion=2DA0FEA89EBE73F96BE4053`

Focused typed submit validation is now saved in:

- [logs/vaio-ioctl/20260513-054056-sgx-submit-cmdta-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-054056-sgx-submit-cmdta-live/summary.log)

Final focused state:

- `submit_prepare_count=1`
- `submit_arm_count=1`
- `submit_emit_count=1`
- `submit_launch_count=1`
- `submit_dispatch_count=1`
- `submit_retired_count=1`
- `submit_issue_count=1`
- `submit_transition_count=2`
- `submit_live_stage_name=retired`
- `submit_result_stage_name=retired`
- `submit_manifest_stage_name=retired`
- `submit_descriptor_stage_name=retired`
- `submit_plan_stage_name=retired`
- `submit_issue_plan_stage_name=retired`
- `submit_issue_plan_dependency_active_count=0`
- `coord_live_submit_count=0`
- `gtt_state_range_sgx_object_count=0`

Typed multi-submit validation is now saved in:

- [logs/vaio-ioctl/20260513-054133-sgx-multi-submit-cmdta-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-054133-sgx-multi-submit-cmdta-live/summary.log)

Final typed multi-submit state:

- `SUBMIT_A_ID=3`
- `SUBMIT_B_ID=4`
- submit A final:
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
  - `submit_issue_plan_stage_name=retired`
  - `submit_dependency_active_count=0`
- submit B final:
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
  - `submit_result_dependency_active_count=1`
  - `submit_manifest_dependency_active_count=1`
  - `submit_descriptor_dependency_active_count=1`
  - `submit_plan_dependency_active_count=1`
  - `submit_dependency_active_count=1`
  - `submit_issue_plan_stage_name=retired`
- final cleanup:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

The saved current-boundary view therefore stays terminal on the new module:

- `resolved_profile=complete`
- `resolved_reason=latest_phase_evidence_terminal`
- `source_timestamp=20260513-053956`

## SGX submit TA submission object

The next validated internal SGX-shaped execution layer is now:

- `psb_sgx_lab_submit_sgx_ta_submission`

This layer sits above `psb_sgx_lab_submit_sgx_queued_ta_command` and keeps one
full TA submission coherent as:

- validated queued TA command
- typed `live` image
- typed `manifest` image
- typed `descriptor` image
- typed `plan` image
- optional launch-time typed `issue-plan` image

This batch needed two live fixes before the exact ladder was green again:

- first guarded deploy:
  - [logs/vaio-deploy/20260513-075745-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-075745-guarded-deploy)
  - live override `srcversion 0AD6D850315762A2E4F6E45`
  - exact proof then failed at `issue` in
    [logs/vaio-plan-session/20260513-080115-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-080115-sgx-plan-session)
- second guarded deploy after relaxing the `issue-plan` cross-check:
  - [logs/vaio-deploy/20260513-080356-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-080356-guarded-deploy)
  - live override `srcversion 9172ED422A56E8DA614C79D`
  - exact proof still failed at `issue` in
    [logs/vaio-plan-session/20260513-080540-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-080540-sgx-plan-session)
- final guarded deploy after relaxing the stage-switched `plan` cross-check:
  - [logs/vaio-deploy/20260513-080856-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-080856-guarded-deploy)

Validated live module state on the final rerun:

- `driver_srcversion=329853678DD9A992924BB5C`
- `override_module_size=453508`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

The exact no-redeploy `plan` proof on the same module is:

- [logs/vaio-plan-session/20260513-081039-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-081039-sgx-plan-session)

That proof cleared terminal `complete` again:

- `session_outcome=completed`
- `selected_phase_profile=complete`
- `final_driver_srcversion=329853678DD9A992924BB5C`

Focused typed submit validation is now saved in:

- [logs/vaio-ioctl/20260513-081614-sgx-submit-ta-submission-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-081614-sgx-submit-ta-submission-live/summary.log)

Final focused state:

- `submit_prepare_count=1`
- `submit_arm_count=1`
- `submit_emit_count=1`
- `submit_launch_count=1`
- `submit_dispatch_count=1`
- `submit_retired_count=1`
- `submit_issue_count=1`
- `submit_transition_count=2`
- `submit_dependency_active_count=0`
- `coord_live_submit_count=0`
- `gtt_state_range_sgx_object_count=0`

Typed multi-submit validation is now saved in:

- [logs/vaio-ioctl/20260513-081700-sgx-multi-submit-ta-submission-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-081700-sgx-multi-submit-ta-submission-live/summary.log)

Final typed multi-submit state:

- `SUBMIT_A_ID=6`
- `SUBMIT_B_ID=7`
- submit A final:
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
  - `submit_dependency_active_count=0`
- submit B final:
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
  - `submit_dependency_active_count=1`
- final cleanup:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

The saved current-boundary view therefore stays terminal on the new module:

- `resolved_profile=complete`
- `resolved_reason=latest_phase_evidence_terminal`
- `source_timestamp=20260513-081039`

## SGX submit TA timeline object

The next validated internal SGX-shaped runtime layer is now:

- `psb_sgx_lab_submit_sgx_ta_timeline`

This layer sits above `psb_sgx_lab_submit_sgx_ta_execution` and keeps one
explicit TA runtime object coherent across:

- the live submission snapshot
- the optional future launch snapshot
- the historical launch, dispatch, and retired queued-command snapshots
- the runtime issue, transition, and global-ticket state around them

This batch needed one live fix after the first draft over-constrained the
running multi-submit path.

First draft:

- guarded deploy:
  - `logs/vaio-deploy/20260513-123521-guarded-deploy`
  - live override `srcversion 82C2FB699E0C6937AB04CC2`
- exact no-redeploy `plan` proof still passed:
  - `logs/vaio-plan-session/20260513-123726-sgx-plan-session`
- focused typed submit also still passed:
  - `logs/vaio-ioctl/20260513-123846-sgx-submit-ta-timeline-live/summary.log`
- typed multi-submit exposed the bad invariant:
  - `logs/vaio-ioctl/20260513-123933-sgx-multi-submit-ta-timeline-live/summary.log`
  - failure at `SUBMIT_A_AFTER_RUNNING_VALIDATE`
  - `submit_op_ret=-1 errno=22`

Final repaired module:

- guarded deploy:
  - `logs/vaio-deploy/20260513-124147-guarded-deploy`
  - live override:
    - `driver_srcversion=A1A66310C19EE4235C25ED2`
    - `override_module_size=455932`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`

Canonical live proof set on the repaired module:

- exact `complete` proof:
  - `logs/vaio-plan-session/20260513-124338-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=A1A66310C19EE4235C25ED2`
- focused typed submit:
  - `logs/vaio-ioctl/20260513-124444-sgx-submit-ta-timeline-live/summary.log`
  - final focused state:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- typed multi-submit:
  - `logs/vaio-ioctl/20260513-124523-sgx-multi-submit-ta-timeline-live/summary.log`
  - final typed state:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=1`
      - `submit_emit_count=1`
      - `submit_launch_count=1`
      - `submit_dispatch_count=1`
      - `submit_retired_count=1`
      - `submit_issue_count=1`
      - `submit_transition_count=2`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=1`
      - `submit_emit_count=1`
      - `submit_launch_count=1`
      - `submit_dispatch_count=1`
      - `submit_retired_count=1`
      - `submit_issue_count=1`
      - `submit_transition_count=2`
      - `submit_dependency_active_count=1`

The saved current-boundary view therefore still stays terminal on the repaired
module:

- `resolved_profile=complete`
- `resolved_reason=latest_phase_evidence_terminal`
- `source_timestamp=20260513-124338`

2026-05-13: added `psb_sgx_lab_submit_sgx_ta_runtime` above
`psb_sgx_lab_submit_sgx_ta_timeline`, then fixed the first draft after live
hardware showed that unresolved dependencies are valid during `prepare` and
pre-launch validation.

First draft:

- deploy:
  - `logs/vaio-deploy/20260513-125935-guarded-deploy`
  - live override `srcversion BC32B83F0BFBD10FB1D0645`
- exact `complete` still passed:
  - `logs/vaio-plan-session/20260513-130122-sgx-plan-session`
- focused typed submit still passed:
  - `logs/vaio-ioctl/20260513-130233-sgx-submit-ta-runtime-live/summary.log`
- typed multi-submit failed at `submit_prepare 6` with `errno=22`:
  - `logs/vaio-ioctl/20260513-130314-sgx-multi-submit-ta-runtime-live/summary.log`
- traced failure bundle:
  - `logs/vaio-ioctl/20260513-130855-sgx-multi-submit-ta-runtime-trace/summary.log`

Repaired draft:

- deploy:
  - `logs/vaio-deploy/20260513-131259-guarded-deploy`
  - final live override:
    - `driver_srcversion=440FDB3F77708F25340DA25`
    - `override_module_size=456232`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- exact proof back to green:
  - `logs/vaio-plan-session/20260513-131445-sgx-plan-session`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=440FDB3F77708F25340DA25`
- focused typed submit passed:
  - `logs/vaio-ioctl/20260513-131554-sgx-submit-ta-runtime-live/summary.log`
  - final focused state:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- typed multi-submit passed on the repaired validator:
  - `logs/vaio-ioctl/20260513-131635-sgx-multi-submit-ta-runtime-live/summary.log`
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_dependency_active_count=1`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
- documented in:
  - `docs/180-sgx-submit-ta-runtime-object.md`

2026-05-13: added `psb_sgx_lab_submit_sgx_ta_gate` above
`psb_sgx_lab_submit_sgx_ta_runtime`, and moved the live `transition -> running`
gate onto that shared typed object.

- deploy:
  - `logs/vaio-deploy/20260513-151922-guarded-deploy`
  - final live override:
    - `driver_srcversion=643122821ACD1D37A506EB0`
    - `override_module_size=458160`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- exact proof back to green:
  - `logs/vaio-plan-session/20260513-152113-sgx-plan-session`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=643122821ACD1D37A506EB0`
- focused typed submit passed:
  - `logs/vaio-ioctl/20260513-152220-sgx-submit-ta-gate-live/summary.log`
  - final focused state:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- typed multi-submit passed on the new gate object:
  - `logs/vaio-ioctl/20260513-152220-sgx-multi-submit-ta-gate-live/summary.log`
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_dependency_active_count=1`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
- documented in:
  - `docs/181-sgx-submit-ta-gate-object.md`

2026-05-13: added `psb_sgx_lab_submit_sgx_ta_control` above
`psb_sgx_lab_submit_sgx_ta_gate`, and moved both launch-time issue readiness
and `transition -> running` admission onto that shared typed control object.

- deploy:
  - `logs/vaio-deploy/20260513-153601-guarded-deploy`
  - final live override:
    - `driver_srcversion=B77A6FEA8B9671946B32272`
    - `override_module_size=459916`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- exact proof back to green:
  - `logs/vaio-plan-session/20260513-153751-sgx-plan-session`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=B77A6FEA8B9671946B32272`
- focused typed submit passed:
  - `logs/vaio-ioctl/20260513-153922-sgx-submit-ta-control-live/summary.log`
  - final focused state:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- typed multi-submit passed on the new control object:
  - `logs/vaio-ioctl/20260513-154028-sgx-multi-submit-ta-control-live/summary.log`
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_dependency_active_count=1`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
- documented in:
  - `docs/182-sgx-submit-ta-control-object.md`

2026-05-13: added `psb_sgx_lab_submit_sgx_ta_schedule` above
`psb_sgx_lab_submit_sgx_ta_control`, and moved queue/ticket/sync progression
for live owner-state transitions onto that shared typed schedule object.

- deploy:
  - `logs/vaio-deploy/20260513-160724-guarded-deploy`
  - final live override:
    - `driver_srcversion=801A7DA2CBDDDCBEBC3AFEF`
    - `override_module_size=461036`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- exact proof back to green:
  - `logs/vaio-plan-session/20260513-160909-sgx-plan-session`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=801A7DA2CBDDDCBEBC3AFEF`
- focused typed submit passed:
  - `logs/vaio-ioctl/20260513-161024-sgx-submit-ta-schedule-live/summary.log`
  - final focused state:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- typed multi-submit passed on the new schedule object:
  - `logs/vaio-ioctl/20260513-161102-sgx-multi-submit-ta-schedule-live/summary.log`
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_dependency_active_count=1`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
- documented in:
  - `docs/183-sgx-submit-ta-schedule-object.md`

2026-05-13: added `psb_sgx_lab_submit_sgx_ta_dispatch` above
`psb_sgx_lab_submit_sgx_ta_schedule`, and moved the launch-side issue
writeback image onto that shared typed dispatch object.

- deploy:
  - `logs/vaio-deploy/20260513-162106-guarded-deploy`
  - final live override:
    - `driver_srcversion=AA6BEDA312078692D47D0A8`
    - `override_module_size=462560`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- exact proof back to green:
  - `logs/vaio-plan-session/20260513-162255-sgx-plan-session`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=AA6BEDA312078692D47D0A8`
- focused typed submit passed:
  - `logs/vaio-ioctl/20260513-162355-sgx-submit-ta-dispatch-live/summary.log`
  - final focused state:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- typed multi-submit passed on the new dispatch object:
  - `logs/vaio-ioctl/20260513-162434-sgx-multi-submit-ta-dispatch-live/summary.log`
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_dependency_active_count=1`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
- documented in:
  - `docs/184-sgx-submit-ta-dispatch-object.md`

2026-05-13: added `psb_sgx_lab_submit_sgx_ta_path` above
`psb_sgx_lab_submit_sgx_ta_dispatch`, and moved the transition-side
queue/CCB/sync/owner writeback image onto that shared typed path object.

- deploy:
  - `logs/vaio-deploy/20260513-164652-guarded-deploy`
  - final live override:
    - `driver_srcversion=A84CAC0D63EBA733BE134C6`
    - `override_module_size=462848`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- exact proof back to green:
  - `logs/vaio-plan-session/20260513-164848-sgx-plan-session`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=A84CAC0D63EBA733BE134C6`
- focused typed submit passed:
  - `logs/vaio-ioctl/20260513-165147-sgx-submit-ta-path-live/summary.log`
  - final focused state:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- typed multi-submit passed on the new path object:
  - `logs/vaio-ioctl/20260513-165002-sgx-multi-submit-ta-path-live/summary.log`
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_dependency_active_count=1`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
- documented in:
  - `docs/185-sgx-submit-ta-path-object.md`

2026-05-13: added `psb_sgx_lab_submit_sgx_ta_commit` above
`psb_sgx_lab_submit_sgx_ta_path`, and moved the queue/CCB/sync/owner write
plan plus next-submit-state apply image onto that shared typed top object.

- deploy:
  - `logs/vaio-deploy/20260513-172835-sgx-ta-commit-deploy`
  - final live override:
    - `driver_srcversion=008618C5501CD0CD7A0CB2B`
    - `override_module_size=464244`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- exact proof back to green:
  - `logs/vaio-plan-session/20260513-173037-sgx-plan-session`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=008618C5501CD0CD7A0CB2B`
- focused typed submit passed:
  - `logs/vaio-ioctl/20260513-173156-sgx-submit-ta-commit-live/summary.log`
  - final focused state:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- typed multi-submit passed on the new commit object:
  - `logs/vaio-ioctl/20260513-173156-sgx-multi-submit-ta-commit-live/summary.log`
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_dependency_active_count=1`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
- documented in:
  - `docs/187-sgx-submit-ta-commit-object.md`

2026-05-13: added `psb_sgx_lab_submit_sgx_ta_event` above
`psb_sgx_lab_submit_sgx_ta_commit`, and moved the live event-side count,
timestamp, and refresh bookkeeping onto that shared typed top object.

- deploy:
  - `logs/vaio-deploy/20260513-181949-sgx-ta-event-deploy`
  - final live override:
    - `driver_srcversion=42F7BCB76CD9DEFD437A03A`
    - `override_module_size=464220`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- exact proof back to green:
  - `logs/vaio-plan-session/20260513-182148-sgx-plan-session`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=42F7BCB76CD9DEFD437A03A`
- focused typed submit passed:
  - `logs/vaio-ioctl/20260513-182251-sgx-submit-ta-event-live/summary.log`
  - final focused state:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- typed multi-submit passed on the new event object:
  - `logs/vaio-ioctl/20260513-182331-sgx-multi-submit-ta-event-live/summary.log`
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_dependency_active_count=1`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
- documented in:
  - `docs/188-sgx-submit-ta-event-object.md`
