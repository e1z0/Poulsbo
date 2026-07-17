/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sgx_fw.h - SGX535 (EMGD 1.18) microkernel firmware description for the
 * Poulsbo gma500 3D bring-up.
 *
 * All numeric values are transcribed from the recon dump of
 * libemgdsrv_init.so.1.5.15.3226 (.data 0x1d538..0x1d5a8) and are byte-exact
 * against the extracted blobs in firmware/sgx535-emgd-1.18/. Each blob is a
 * single FLAT USE binary uploaded verbatim (no code-byte relocation); the only
 * relocation is computing USE service exec-addresses from the offsets below.
 *
 * FIRST-CUT / bring-up code. Constants marked GUESSED still need live-hardware
 * confirmation; everything else is confirmed by the recon.
 */
#ifndef _SGX_FW_H_
#define _SGX_FW_H_

#include <linux/types.h>

/* request_firmware() names; the .bin files are copied verbatim to /lib/firmware. */
#define SGX_FW_MAIN_NAME "sgx535/pbuKernelProgram.bin"
#define SGX_FW_TA_NAME   "sgx535/pbuKernelTAProgram.bin"
#define SGX_FW_3D_NAME   "sgx535/pbuKernel3DProgram.bin"

/* Byte-exact flat-image sizes (recon: .data scalars == .bin file sizes). */
#define SGX_FW_MAIN_SIZE 22552u  /* 0x5818 */
#define SGX_FW_TA_SIZE   21672u  /* 0x54a8 */
#define SGX_FW_3D_SIZE   25240u  /* 0x6298 */
#define SGX_FW_SPM_NAME  "sgx535/pbuKernelSPMProgram.bin"
#define SGX_FW_2D_NAME   "sgx535/pbuKernel2DProgram.bin"
#define SGX_FW_SPM_SIZE  18328u  /* 0x4798 */
#define SGX_FW_2D_SIZE   14744u  /* 0x3998 */

/* DDK allocates code buffers 0x8000 (32KB) aligned, flags 0x9. */
#define SGX_FW_CODE_ALIGN 0x8000u

/*
 * Service byte-offsets into the MAIN flat blob. All CONFIRMED + static (safe to
 * hardcode). The EVENTHANDLER (0x10) is the entry the HW runs on EVENT_KICK.
 */
#define SGX_UK_OFF_EVENTHANDLER		0x0010u
#define SGX_UK_OFF_HOSTKICK_POWER	0x1038u
#define SGX_UK_OFF_HOSTKICK_2D		0x10f8u
#define SGX_UK_OFF_HOSTKICK_TRANSFER	0x11d0u
#define SGX_UK_OFF_HOSTKICK_TA		0x12a0u
#define SGX_UK_OFF_HOSTKICK_CLEANUP	0x1388u
#define SGX_UK_OFF_HOSTKICK_PROCESS_Q	0x1470u
#define SGX_UK_OFF_HOSTKICK_GETMISC	0x1520u
#define SGX_UK_OFF_INIT_PRIMARY1	0x1700u
#define SGX_UK_OFF_INIT_SECONDARY	0x1780u
#define SGX_UK_OFF_INIT_PRIMARY2	0x1790u

/* Offsets INTERNAL to the TA / 3D blobs (relative to their own devVAddr). */
#define SGX_UK_TA_OFF_HANDLER		0x0680u
#define SGX_UK_3D_OFF_HANDLER		0x0900u
/*
 * WARNING (recon gotcha): TA_TAEVENTHANDLER and 3D_THREEDEVENTHANDLER live in
 * .bss and are runtime-computed by SGXInit. They are deliberately NOT defined
 * here - do not hardcode them. For the first TA increment the main-ukernel
 * dispatch via HOSTKICK_TA is expected to suffice (open question, needs HW).
 */

/*
 * Code-heap base used in the USE exec-address encoding. MUST equal the value
 * programmed into USE_CODE_BASE(15). Two candidates exist depending on the DDK
 * mem-context build; which one is legal for this tree's MMU layout is an OPEN
 * QUESTION that only the live box resolves.
 *   0xF8000000 - single mem-context (sgxconfig.h:115)   <- default guess
 *   0x0E400000 - multi mem-context  (sgxconfig.h:155)
 * GUESSED: default to the single-context base.
 */
#ifndef SGX_KERNEL_CODE_HEAP_BASE
#define SGX_KERNEL_CODE_HEAP_BASE 0xF8000000u
#endif

/* PVRSRV_USSE_EDM_INIT_COMPLETE (sgx_mkif_km.h:276) - HOST_CTL init-status bit0. */
#define SGX_USSE_EDM_INIT_COMPLETE (1u << 0)

/*
 * HOST_CTL.ui32InitStatus struct offset = 0 — CONFIRMED (was open 0-vs-8).
 * In SGXMKIF_HOST_CTL (sgx_mkif_km.h:105) ui32InitStatus is the first member
 * UNLESS PVRSRV_USSE_EDM_BREAKPOINTS adds two u32 before it. The EMGD 1.18
 * build flags extracted from libemgdsrv_init.so do NOT define
 * PVRSRV_USSE_EDM_BREAKPOINTS (SUPPORT_HW_RECOVERY / FIX_HW_BRN_28889 only add
 * fields AFTER ui32InitStatus). Kept overridable as a safety hatch.
 */
#ifndef SGX_HOSTCTL_INITSTATUS_OFFSET
#define SGX_HOSTCTL_INITSTATUS_OFFSET 0u
#endif

/*
 * EUR_CR_USE_CODE_BASE(15) @ 0x0A48 field encoding (sgx535defs.h:620-624).
 * NOT a raw VA: 25-bit ADDR field + 2-bit data-master select. Writing the raw
 * code_heap_base (e.g. 0xF8000000) yields ADDR=0,DM=0 by accident, which is only
 * correct if the kernel USE code sits at USE-code-space offset 0. The value is
 * really computed by srvinit's init script (SGXRunScript, asInitCommandsPart2)
 * from where the code is mapped; replicate that: ADDR = code USE-space offset,
 * DM per the kernel data master. GUESSED DM=0; confirm live.
 */
#define SGX_USE_CODE_BASE_REG(X)     (0x0A0Cu + 4u * (X))
#define SGX_KERNEL_USE_CODE_INDEX    15u
#define SGX_USE_CODE_BASE_ADDR_MASK  0x01FFFFFFu
#define SGX_USE_CODE_BASE_DM_SHIFT   25u
/*
 * CONFIRMED from SGXInit disasm (0x28a2): value = (codeBase >> 7) | (DM << 25),
 * DM=3 for the kernel/EDM data master. 128-byte granular. e.g.
 * (0xF8000000 >> 7) | (3<<25) = 0x01F00000 | 0x06000000 = 0x07F00000.
 */
#define SGX_USE_CODE_BASE_DM 3u
static inline u32 sgx_pack_use_code_base(u32 code_base_dev_va)
{
	return ((code_base_dev_va >> 7) & SGX_USE_CODE_BASE_ADDR_MASK) |
	       (SGX_USE_CODE_BASE_DM << SGX_USE_CODE_BASE_DM_SHIFT);
}

/* PDS heap base — EUR_CR_PDS_EXEC_BASE(0x0AB8) gets this verbatim (1MB granular). */
#define SGX_PDS_EXEC_BASE_DEV_VA  0xF6000000u
#define SGX_KERNEL_DATA_HEAP_BASE 0xFA000000u

/*
 * USE exec-address packing (SGXInit disasm 0x1e65-0x1eec).
 *   delta = buf_dev_vaddr + offset - code_heap_base
 * Getting this wrong makes the EDM jump to garbage -> hang. Keep the boot path
 * bounded so that is recoverable.
 */
static inline u32 sgx_pack_use_addr(u32 buf_dev_vaddr, u32 offset,
				    u32 code_heap_base)
{
	u32 delta = buf_dev_vaddr + offset - code_heap_base;
	u32 low19 = delta & 0x7ffff;
	u32 high = delta >> 19;

	return (((high + 0xf) & 0xf)
		| 0x200000u
		| ((low19 >> 15) << 4)
		| ((low19 << 4) & 0x7ff00u));
}

#endif /* _SGX_FW_H_ */
