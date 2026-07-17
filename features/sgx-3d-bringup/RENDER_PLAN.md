# SGX535 render path — plan & scope (post-INIT_COMPLETE + CCB transport)

Status (2026-07-17):
- Microkernel boots to INIT_COMPLETE on real HW.
- Host->ukernel CCB command transport works: POWER(3) and GETMISCINFO(5) are
  CONSUMED (ReadOffset advances, EDMTask0 steps per kick), no fault.
- BUT command HANDLERS do not complete their work with only a bare init:
  GETMISCINFO consumes but writes nothing to its output buffer (verified: correct
  ServiceAddress, both Data words set, request flag set, cache-coherent reads,
  no BIF fault - buffer stays untouched). => handlers' data-write paths need
  device-context state (PDS/data-master/render state) minimal init does not set up.
  This is the gating discovery for all real work below.

## Command -> handler mechanics (confirmed on HW)
- SGXMKIF_COMMAND = {ServiceAddress, CacheControl, Data[2]} (16 B).
- ServiceAddress = HOSTKICK_byteoffset >> 3 (TA 0x254, XFER 0x23a, 2D 0x21f,
  POWER 0x207, GETMISC 0x2a4, PROC_Q 0x28e) - all verified to point at real code.
- CCB-CTL {WriteOffset,ReadOffset} at EDM-CTL+0x08; command ring (kccb, >=0x8000)
  at EDM-CTL+0xDC; host-ctl (InitStatus) at EDM-CTL+0x04.
- Cache-coherent reads (clflush before read) required - Poulsbo GPU non-coherent.

## What a real render needs - the full pipeline
Types 0/1/2 (TA/TRANSFER/2D): sCommand.Data[1] = HW context devVA; the real
payload lives in the context's own per-context CCB at ui32CCBOffset.
1. HW render/transfer context (Data[1]): per-context object w/ own CCB, sync state,
   (render) RT dataset.
2. Per-context CCB payload:
   - TA: SGXMKIF_CMDTA_SHARED + CMDTA proper (VDM stream, PDS vtx, ISP/TSP, RT cfg).
   - TRANSFER: SGXMKIF_TRANSFERCMD_SHARED + payload (USSE copy prog VA, PDS,
     src/dst surfaces, format, dims).
3. Render target/tiling (TA): tile bufs, ISP/TSP, parameter buffer (DPM).
4. Shaders: a USSE program + the PDS that loads it (hardware microcode).
5. VDM control stream (TA): geometry/state stream the tile accelerator walks.

## Honest scope
Items 2-5 are exactly what the userspace driver (libpvr2d / closed PowerVR GLES)
generates. Hand-crafting = emitting valid USSE+PDS microcode + VDM/ISP/TSP state
per op; large multi-session effort, and (a) shows handlers do nothing observable
until this state is real.

## Tractable ordering (smallest observable first)
1. TRANSFER blit (type 1): smallest real op - no tiling/ISP/TSP, just a USSE
   pixel program reading src, writing dst. Needs xfer context + hand-built USSE
   copy program + PDS + src/dst surfaces. Observable = dst pixels change.
2. Then TA (type 0) for 3D geometry (RT dataset, PB, VDM) - much larger.

## Two realistic routes
- A. Continue open hand-build: TRANSFER context + minimal USSE copy program
  (reuse reversed enc1530/DOUTA + partial PDS opcode table). Multi-session.
- B. Drive the closed userspace: kernel side now capable (init + CCB transport);
  period-correct closed PowerVR userspace emits items 2-5 - historically the
  shipping path for Poulsbo 3D. Needs ABI matching to this kernel module.

The command transport (hard kernel-side prerequisite) is DONE. Remainder is
render-state generation.
