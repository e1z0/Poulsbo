SGX535 microkernel firmware — extracted from Intel EMGD 1.18 (build 3398)
Source: research/external/intel-binaries-linux/binaries/MeeGo1.2/emgd-bin-3398-1.18.i586.rpm
        -> usr/lib/libemgdsrv_init.so.1.5.15.3226 (data symbols)
Build:  PVR_BUILD_DIR=pc_i686_poulsbo_d0_linux  -DSGX535 -DSGX_CORE_REV=121  (rev 1.2.1)
Matches Vaio CORE_REVISION 0x00010201 exactly.

Blobs (USE microkernel programs):
  pbuKernelProgram.bin    22552  main/EDM ukernel (host-kick service routines)
  pbuKernelTAProgram.bin  21672  TA (tile accelerator) pipeline
  pbuKernel3DProgram.bin  25240  3D (ISP/TSP/PBE) pipeline
  pbuKernel2DProgram.bin  14744  2D
  pbuKernelSPMProgram.bin 18328  SPM (out-of-memory/parameter mgmt)

Service-address offsets into pbuKernelProgram (for EVENT_KICK aui32HostKickAddr[]):
  HOSTKICK_TA=0x12a0  HOSTKICK_TRANSFER=0x11d0  HOSTKICK_2D=0x10f8
  HOSTKICK_POWER=0x1038  HOSTKICK_CLEANUP=0x1388  EVENTHANDLER=0x0010

Upload/relocation logic: SGXInit() in the same .so (reverse-engineer for boot).
Kernel-side loading template: research/external/linux_openpvrsgx.
