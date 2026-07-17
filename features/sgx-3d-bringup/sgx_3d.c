// SPDX-License-Identifier: GPL-2.0
/*
 * sgx_3d.c - FIRST-CUT SGX535 3D bring-up for the Poulsbo gma500 driver.
 *
 * Stands up a real SGX render MMU context, uploads the EMGD 1.18 microkernel
 * blobs, runs the reset -> USE/PDS base program -> host-ctl init -> single boot
 * EVENT_KICK -> bounded poll for INIT_COMPLETE handshake, and exposes an
 * on-demand sysfs trigger (NOT run at module load).
 *
 * SAFETY CONTRACT (see notes.md):
 *   - Whole path gated behind module param sgx3d_enable (default 0). Off == the
 *     driver boots exactly as today (2D-only).
 *   - Never run at load; only via the sysfs trigger.
 *   - Every hardware wait is BOUNDED. On any timeout: one SGXReset, bail to
 *     2D-only, return -EIO. Two consecutive failed boots latch 3D permanently
 *     off for the module lifetime.
 *   - All MMIO bracketed by gma_power_begin(force_on=true)/gma_power_end.
 *   - Validate CORE_REVISION + blob sizes before kicking a bad image.
 *
 * This is bring-up scaffold: it WILL need hardware iteration. Every value that
 * is a guess is marked "GUESSED".
 *
 * Intended to be built into the psb/gma500 module. It depends on a few driver
 * primitives that are currently 'static'; notes.md lists the exact one-line
 * exports to add (psb_gtt_sgx3d_bind/unbind). Those are declared extern below.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <asm/cacheflush.h>

#include "psb_drv.h"
#include "psb_reg.h"
#include "power.h"
#include "mmu.h"
#include "gtt.h"
#include "sgx_fw.h"
#include "sgx_pds.h"

/* ------------------------------------------------------------------ */
/* Module parameters                                                   */
/* ------------------------------------------------------------------ */

static bool sgx3d_enable;
module_param(sgx3d_enable, bool, 0644);
MODULE_PARM_DESC(sgx3d_enable,
	"Enable the experimental SGX535 3D bring-up path (default 0 = 2D-only).");

/*
 * Device VA the ukernel code is mapped at AND encoded against. MUST equal the
 * value programmed into USE_CODE_BASE(15). OPEN QUESTION: 0xF8000000 (single
 * mem-context) vs 0x0E400000 (multi). Tunable so it can be iterated on hardware
 * without a rebuild. GUESSED default.
 */
static uint sgx3d_code_base = 0xF8000000; /* SGX_KERNEL_CODE_HEAP_BASE (confirmed) */
module_param(sgx3d_code_base, uint, 0644);
MODULE_PARM_DESC(sgx3d_code_base,
	"Device VA / USE_CODE_BASE(15) for the ukernel code heap (GUESSED).");

/* HOST_CTL.ui32InitStatus struct offset (0 or +8). Tunable for HW confirmation. */
static uint sgx3d_initstatus_off = SGX_HOSTCTL_INITSTATUS_OFFSET;
module_param(sgx3d_initstatus_off, uint, 0644);
MODULE_PARM_DESC(sgx3d_initstatus_off,
	"HOST_CTL ui32InitStatus byte offset (0=breakpoints undef, 8=def).");

/* ------------------------------------------------------------------ */
/* Register offsets not present in psb_reg.h (from sgx535defs.h).       */
/* ------------------------------------------------------------------ */

/* Soft-reset block masks (sgx535defs.h:88-100) - CONFIRMED. */
#define SGX_SR_BIF	0x00000001u
#define SGX_SR_TWOD	0x00000002u
#define SGX_SR_DPM	0x00000004u
#define SGX_SR_TA	0x00000008u
#define SGX_SR_USE	0x00000010u
#define SGX_SR_ISP	0x00000020u
#define SGX_SR_TSP	0x00000040u
/* Composite "reset every block" used by SGXResetSoftReset. */
#define SGX_SR_ALL_NOBIF (SGX_SR_TWOD | SGX_SR_DPM | SGX_SR_TA | \
			  SGX_SR_USE | SGX_SR_ISP | SGX_SR_TSP)

/* EUR_CR_BIF_MEM_REQ_STAT (sgx535defs.h:577) - CONFIRMED. */
#define SGX_BIF_MEM_REQ_STAT		0x0CA8u
#define SGX_BIF_MEM_REQ_STAT_READS_MASK	0x000000FFu
/* EUR_CR_BIF_CTRL_INVALDC (sgx535defs.h:366) - CONFIRMED. */
#define SGX_BIF_CTRL_INVALDC		0x00000008u
/* EUR_CR_EVENT_STATUS DPM out-of-memory bits (sgx535defs.h:192,194). */
#define SGX_ES_DPM_OOM_GBL		0x00000004u
#define SGX_ES_DPM_OOM_MT		0x00000002u

/*
 * NOTE: EUR_CR_POWER is NOT defined for SGX535 in sgx535defs.h (the DDK write is
 * #if defined(EUR_CR_POWER)). So we deliberately DO NOT write it here - doing so
 * on 535 would poke an undefined register. (Design P4's "EUR_CR_POWER=0" is a
 * no-op on this core.)
 */

/* Bounded-wait constants mirroring PollForValueKM (sysinfo.h:26-27). */
#define SGX_POLL_INTERVAL_US	50
#define SGX_POLL_TRIES		10000	/* ~500ms cap */
#define SGX_RESET_DRAIN_TRIES	1000

/* ------------------------------------------------------------------ */
/* Driver primitives that must be exposed (see notes.md integration).   */
/* Tiny wrappers to be un-static'd / added in gtt.c.                     */
/* ------------------------------------------------------------------ */

/*
 * Pin @gt and psb_mmu_insert_pages it into dev_priv->pf_pd at an arbitrary
 * @gpu_va (unlike psb_gtt_lab_bind_pf which forces gatt_start+offset). Needed to
 * place the ukernel code at sgx3d_code_base. Implementation in notes.md.
 */
extern int psb_gtt_sgx3d_bind(struct drm_device *dev, struct gtt_range *gt,
			      unsigned long gpu_va);
extern void psb_gtt_sgx3d_unbind(struct drm_device *dev, struct gtt_range *gt,
				 unsigned long gpu_va);

/* ------------------------------------------------------------------ */
/* Per-device 3D state.                                                 */
/* Single-instance for bring-up (one Poulsbo GPU). Not in dev_priv to    */
/* avoid touching the shared struct in this first cut.                   */
/* ------------------------------------------------------------------ */

struct sgx3d_bo {
	struct gtt_range *gt;
	unsigned long gpu_va;	/* device VA where bound into pf_pd */
	u32 size;
};

struct sgx3d_state {
	struct drm_device *dev;
	bool ready;		/* M3 buffers stood up */
	bool init_complete;	/* M4 handshake done */
	bool latched_off;	/* kill switch: 3D permanently disabled */
	int  reset_fail_count;

	struct sgx3d_bo main_code;
	struct sgx3d_bo ta_code;
	struct sgx3d_bo td_code;	/* 3d */
	struct sgx3d_bo spm_code;
	struct sgx3d_bo d2_code;	/* 2d */
	struct sgx3d_bo host_ctl;
	struct sgx3d_bo kccb;		/* kernel CCB command ring (EDM-CTL+0xDC, alloc[0x18]) */
	struct sgx3d_bo ccb_ctl;	/* CCB-CTL: {WriteOffset,ReadOffset} (EDM-CTL+0x08, alloc[3]) */
	struct sgx3d_bo edm_misc6;	/* scratch (EDM-CTL+0x04, alloc[6], 0x34B, HWPerfFlags@0x2c) */
	struct sgx3d_bo kicker;		/* pui32KernelCCBEventKicker dword */
	struct sgx3d_bo edm_ctl;	/* TA3D-CTL buffer (blob[5912] points here) */
	struct sgx3d_bo edm_status[2];	/* EDM status buffers (TA3D-CTL +0xD4/+0xD8) */
	struct sgx3d_bo edm_dpm;	/* EDM/DPM private buffer (EDM-CTL +0xDC, >=0x3810) */
	struct sgx3d_bo dpm_temp;	/* scratch / GETMISCINFO output buffer */
	struct sgx3d_bo hw2dctx;	/* HW 2D context (2D cmd Data[1]) */
	struct sgx3d_bo client_ccb;	/* client CCB: 2DCMD_SHARED + 2D-core stream */
	struct sgx3d_bo dst_surf;	/* 2D fill destination (LOW VA, 28-bit 2D addr) */
	struct sgx3d_bo pds[5];		/* init-PDS programs: 0=pds1_evt 1=pds2_elb
					 * 2=primary1 3=secondary 4=primary2 */

	/* SGXReset scratch - allocated once, before the first reset. */
	struct page *reset_pd;
	struct page *reset_pt;

	u32 kick_count;			/* device-memory kicker value */
};

static struct sgx3d_state g_sgx3d;	/* GUESSED: single GPU assumption */
static DEFINE_MUTEX(g_sgx3d_lock);

/* ------------------------------------------------------------------ */
/* Small BO helpers (kmap-based, mirrors poulsbo_sgx_object_*).         */
/* ------------------------------------------------------------------ */

static u32 sgx3d_gpu_va(struct drm_psb_private *dev_priv, struct gtt_range *gt)
{
	return dev_priv->gtt.gatt_start + gt->offset;
}

/* Copy @len bytes into the BO's backing pages starting at byte 0. */
static int sgx3d_upload(struct sgx3d_bo *bo, const void *src, u32 len)
{
	struct gtt_range *gt = bo->gt;
	u32 done = 0;

	if (!gt || !gt->pages || (u32)gt->npage << PAGE_SHIFT < len)
		return -EINVAL;

	while (done < len) {
		u32 in_page = done & (PAGE_SIZE - 1);
		u32 page_idx = done >> PAGE_SHIFT;
		u32 chunk = min_t(u32, PAGE_SIZE - in_page, len - done);
		void *map = kmap(gt->pages[page_idx]);

		memcpy((u8 *)map + in_page, (const u8 *)src + done, chunk);
		clflush_cache_range((u8 *)map + in_page, chunk);
		kunmap(gt->pages[page_idx]);
		done += chunk;
	}
	return 0;
}

/* Zero the whole BO. */
static void sgx3d_zero(struct sgx3d_bo *bo)
{
	struct gtt_range *gt = bo->gt;
	int i;

	if (!gt || !gt->pages)
		return;
	for (i = 0; i < gt->npage; i++) {
		void *map = kmap(gt->pages[i]);

		memset(map, 0, PAGE_SIZE);
		clflush_cache_range(map, PAGE_SIZE);
		kunmap(gt->pages[i]);
	}
}

static int sgx3d_write_u32(struct sgx3d_bo *bo, u32 byte_off, u32 val)
{
	struct gtt_range *gt = bo->gt;
	void *map;

	if (!gt || !gt->pages)
		return -EINVAL;
	if ((u64)byte_off + 4 > (u64)gt->npage << PAGE_SHIFT)
		return -EINVAL;
	map = kmap(gt->pages[byte_off >> PAGE_SHIFT]);
	*(u32 *)((u8 *)map + (byte_off & (PAGE_SIZE - 1))) = val;
	clflush_cache_range((u8 *)map + (byte_off & (PAGE_SIZE - 1)), 4);
	kunmap(gt->pages[byte_off >> PAGE_SHIFT]);
	return 0;
}

static int sgx3d_read_u32(struct sgx3d_bo *bo, u32 byte_off, u32 *out)
{
	struct gtt_range *gt = bo->gt;
	void *map;

	if (!gt || !gt->pages)
		return -EINVAL;
	if ((u64)byte_off + 4 > (u64)gt->npage << PAGE_SHIFT)
		return -EINVAL;
	map = kmap(gt->pages[byte_off >> PAGE_SHIFT]);
	/* Poulsbo GPU is not cache-coherent: invalidate the line so we see the
	 * ukernel's write in RAM rather than a stale cached value (a repeated poll
	 * would otherwise cache the pre-write value forever). */
	clflush_cache_range((u8 *)map + (byte_off & (PAGE_SIZE - 1)), 4);
	*out = *(u32 *)((u8 *)map + (byte_off & (PAGE_SIZE - 1)));
	kunmap(gt->pages[byte_off >> PAGE_SHIFT]);
	return 0;
}

/*
 * fn@0x1530 bit-splicer: write full 32-bit @V into the 2-dword USE/PDS
 * instruction pair at @off, preserving the opcode-template bits. Verified
 * encoding from the srvinit disasm.
 */
static void sgx3d_enc1530(struct sgx3d_bo *bo, u32 off, u32 V)
{
	u32 w0 = 0, w1 = 0;

	sgx3d_read_u32(bo, off, &w0);
	sgx3d_read_u32(bo, off + 4, &w1);
	w0 = (V & 0x001FFFFF) | (w0 & 0xFFE00000);
	w1 = (w1 & 0xFFFC0E0F) | ((V >> 0x11) & 0x1F0) | (((V >> 0x1A) << 0xC));
	sgx3d_write_u32(bo, off, w0);
	sgx3d_write_u32(bo, off + 4, w1);
}

/* DOUTA descriptor for a USE entry at device VA @use_va (in KERNEL_CODE heap). */
static u32 sgx3d_douta(u32 use_va)
{
	u32 X = use_va - SGX_KERNEL_CODE_HEAP_BASE;
	u32 low19 = X & 0x7FFFF, hi = X >> 19;

	return 0x200000u | ((hi + 0xF) & 0xF) | ((low19 >> 15) << 4)
	       | ((low19 << 4) & 0x7FF00);
}

/*
 * Allocate a GPU-visible BO and bind it into pf_pd at @gpu_va. If @gpu_va == 0
 * the default gatt_start+offset placement is used (via the same helper).
 */
static int sgx3d_alloc_bo(struct drm_device *dev, struct sgx3d_bo *bo,
			  u32 size, const char *name, u32 align,
			  unsigned long gpu_va)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	int ret;

	bo->gt = psb_gtt_alloc_range(dev, size, name, 0, align);
	if (!bo->gt)
		return -ENOMEM;

	/*
	 * psb_gtt_alloc_range leaves the embedded gem object uninitialised
	 * (no shmem filp), so pinning it would NULL-deref in
	 * drm_gem_get_pages. Initialise it exactly like psb_gem_create():
	 * shmem backing + 32-bit-only mappings (SGX GPU VA is 32-bit).
	 */
	if (drm_gem_object_init(dev, &bo->gt->gem, size) != 0) {
		psb_gtt_free_range(dev, bo->gt);
		bo->gt = NULL;
		return -ENOMEM;
	}
	mapping_set_gfp_mask(bo->gt->gem.filp->f_mapping,
			     GFP_KERNEL | __GFP_DMA32);

	if (gpu_va == 0)
		gpu_va = sgx3d_gpu_va(dev_priv, bo->gt);

	ret = psb_gtt_sgx3d_bind(dev, bo->gt, gpu_va);
	if (ret) {
		drm_gem_object_release(&bo->gt->gem);
		psb_gtt_free_range(dev, bo->gt);
		bo->gt = NULL;
		return ret;
	}
	bo->gpu_va = gpu_va;
	bo->size = size;
	return 0;
}

static void sgx3d_free_bo(struct drm_device *dev, struct sgx3d_bo *bo)
{
	if (!bo->gt)
		return;
	psb_gtt_sgx3d_unbind(dev, bo->gt, bo->gpu_va);
	drm_gem_object_release(&bo->gt->gem);
	psb_gtt_free_range(dev, bo->gt);
	bo->gt = NULL;
	bo->gpu_va = 0;
}

/* ------------------------------------------------------------------ */
/* M3: allocate + upload firmware + build render context.              */
/* Caller holds a power ref. Returns 0 with g_sgx3d populated.          */
/* ------------------------------------------------------------------ */

static int sgx3d_load_blob(struct drm_device *dev, struct sgx3d_bo *bo,
			   const char *fwname, u32 expect_size,
			   unsigned long gpu_va)
{
	const struct firmware *fw;
	int ret;

	ret = request_firmware(&fw, fwname, dev->dev);
	if (ret) {
		dev_err(dev->dev, "sgx3d: request_firmware(%s) failed %d\n",
			fwname, ret);
		return ret;
	}
	if (fw->size != expect_size) {
		dev_err(dev->dev,
			"sgx3d: %s size %zu != expected %u - refusing\n",
			fwname, fw->size, expect_size);
		release_firmware(fw);
		return -EINVAL;
	}
	/* 0x8000-aligned code buffer, rounded up to whole pages by alloc. */
	ret = sgx3d_alloc_bo(dev, bo, ALIGN(expect_size, PAGE_SIZE),
			     fwname, SGX_FW_CODE_ALIGN, gpu_va);
	if (ret) {
		release_firmware(fw);
		return ret;
	}
	ret = sgx3d_upload(bo, fw->data, fw->size);
	release_firmware(fw);
	if (ret) {
		dev_err(dev->dev, "sgx3d: upload %s failed %d\n", fwname, ret);
		sgx3d_free_bo(dev, bo);
		return ret;
	}
	dev_info(dev->dev, "sgx3d: uploaded %s (%u B) at gpu_va %#lx\n",
		 fwname, expect_size, bo->gpu_va);
	return 0;
}

/*
 * Relocate the ukernel blob + PDS programs. All target VAs are our own
 * allocations, encoded via the fn@0x1530 splicer / DOUTA per the srvinit RE.
 * (Some intricate PDS handler slots are best-effort; iterate on HW.)
 */
static void sgx3d_relocate(struct drm_device *dev)
{
	struct sgx3d_state *s = &g_sgx3d;
	struct sgx3d_bo *b = &s->main_code;
	u32 uk = (u32)s->main_code.gpu_va;
	u32 ta = (u32)s->ta_code.gpu_va, td = (u32)s->td_code.gpu_va;
	u32 spm = (u32)s->spm_code.gpu_va, d2 = (u32)s->d2_code.gpu_va;
	u32 p1 = (u32)s->pds[0].gpu_va, p2 = (u32)s->pds[1].gpu_va;
	u32 pb = (u32)s->pds[3].gpu_va, pc = (u32)s->pds[4].gpu_va;
	u32 elb = p2 >> 4;
	u32 edm = (u32)s->edm_ctl.gpu_va;

	/* --- main-blob IUKP fields (Ghidra section 4, #11-17). --- */
	sgx3d_enc1530(b, 5888, 0x41800800);		/* IUKP1_PDSCONSTSIZESEC (LIMM w0 - must splice) */
	sgx3d_enc1530(b, 5904, pb >> 4);		/* IUKP1_PDSDEVADDRSEC=secondary */
	sgx3d_enc1530(b, 5912, edm);			/* IUKP1_TA3DCTL=EDM ctrl buf */
	sgx3d_enc1530(b, 5968, 0x41800100);		/* IUKP1_PDSCONSTSIZEPRIM2 (LIMM w0 - must splice) */
	sgx3d_enc1530(b, 5984, pc >> 4);		/* IUKP1_PDSDEVADDRPRIM2=primary2 */
	sgx3d_enc1530(b, 6064, p1 & ~0xFu);		/* IUKP2_PDSEXEC=pds1_evt */
	sgx3d_enc1530(b, 6080, 0x8);			/* IUKP2_PDSDATA (LIMM w0 - sibling 6084 is LIMM; must splice) */

	/* --- ELB relocations for all 5 programs (Ghidra section 4, #1-10). --- */
	sgx3d_enc1530(b, 17320, 0x42000100);		/* main ELB_PDSCONSTSIZE */
	sgx3d_enc1530(b, 17344, elb);			/* main ELB_PDSDEVADDR */
	sgx3d_enc1530(&s->ta_code, 0x4028, 0x42000100);
	sgx3d_enc1530(&s->ta_code, 0x4040, elb);
	sgx3d_enc1530(&s->td_code, 0x4E28, 0x42000100);
	sgx3d_enc1530(&s->td_code, 0x4E40, elb);
	sgx3d_enc1530(&s->spm_code, 0x3328, 0x42000100);
	sgx3d_enc1530(&s->spm_code, 0x3340, elb);
	sgx3d_enc1530(&s->d2_code, 0x2528, 0x42000100);
	sgx3d_enc1530(&s->d2_code, 0x2540, elb);

	/* --- PDS#1 (pds[0]) data seg: all 5 handler DOUTAs (Ghidra section 3). --- */
	sgx3d_write_u32(&s->pds[0], 0x00, 0xC4000000);
	sgx3d_write_u32(&s->pds[0], 0x04, 0x00000000);
	sgx3d_write_u32(&s->pds[0], 0x20, 0x00000000);
	sgx3d_write_u32(&s->pds[0], 0x24, 0x100);
	sgx3d_write_u32(&s->pds[0], 0x28, 0x200);
	sgx3d_write_u32(&s->pds[0], 0x2c, sgx3d_douta(d2));		/* 2D */
	sgx3d_write_u32(&s->pds[0], 0x30, sgx3d_douta(td));		/* 3D EVTHANDLER(.bss=0) */
	sgx3d_write_u32(&s->pds[0], 0x34, sgx3d_douta(ta));		/* TA EVTHANDLER(.bss=0) */
	sgx3d_write_u32(&s->pds[0], 0x38, sgx3d_douta(spm));		/* SPM */
	sgx3d_write_u32(&s->pds[0], 0x3c, sgx3d_douta(uk + 0x10));	/* EVENTHANDLER */

	/* --- PDS#2 (pds[1]) = pds2_elb: TA/3D/2D/SPM handler DOUTAs. --- */
	sgx3d_write_u32(&s->pds[1], 0x00, sgx3d_douta(td + 0x900));	/* 3D_THREEDHANDLER */
	sgx3d_write_u32(&s->pds[1], 0x04, 0xC4000000);
	sgx3d_write_u32(&s->pds[1], 0x08, sgx3d_douta(ta + 0x680));	/* TA_TAHANDLER */
	sgx3d_write_u32(&s->pds[1], 0x0c, 0xC4000000);
	sgx3d_write_u32(&s->pds[1], 0x10, sgx3d_douta(d2 + 0x680));	/* 2D */
	sgx3d_write_u32(&s->pds[1], 0x14, 0xC4000000);
	sgx3d_write_u32(&s->pds[1], 0x18, sgx3d_douta(spm + 0xB0));	/* SPM */
	sgx3d_write_u32(&s->pds[1], 0x1c, 0xC4000000);
	sgx3d_write_u32(&s->pds[1], 0x20, 0x00000000);
	sgx3d_write_u32(&s->pds[1], 0x24, 0x200);
	sgx3d_write_u32(&s->pds[1], 0x28, 0x00000000);
	sgx3d_write_u32(&s->pds[1], 0x2c, 0x8000F);
	sgx3d_write_u32(&s->pds[1], 0x30, 0x240100);
	sgx3d_write_u32(&s->pds[1], 0x34, 0x420000);

	/* --- PRIMARY1 (pds[2]) --- */
	sgx3d_write_u32(&s->pds[2], 0x00, sgx3d_douta(uk + 5888));
	sgx3d_write_u32(&s->pds[2], 0x04, 0xC4000000);
	/* --- SECONDARY (pds[3]) --- */
	sgx3d_write_u32(&s->pds[3], 0x00, edm);
	sgx3d_write_u32(&s->pds[3], 0x04, 0x01E0001F);
	sgx3d_write_u32(&s->pds[3], 0x08, sgx3d_douta(uk + 6016));
	sgx3d_write_u32(&s->pds[3], 0x0c, 0x04000000);
	/* --- PRIMARY2 (pds[4]) --- */
	sgx3d_write_u32(&s->pds[4], 0x00, sgx3d_douta(uk + 6032));
	sgx3d_write_u32(&s->pds[4], 0x04, 0xC4000000);

	/* --- TA3D-CTL buffer (Ghidra SGXInit field layout, section 2). --- */
	/* EDM-CTL layout EXACT per Ghidra decomp (piVar18[]): +0x00 self,
	 * +0x04 scratch(alloc6), +0x08 CCB-CTL(alloc3), +0x60 host-ctl(alloc0),
	 * +0xDC kernel-CCB ring(alloc0x18). (Prior layout was wrong but init still
	 * passed; corrected for real command submission.) */
	/* USSE-decode correction (GETMISCINFO handler RE): the EDM reads command
	 * BODIES (incl. Data[1]) from the ring at +0x60 (alloc0, 256x16=0x1000);
	 * +0x04 is host-ctl/InitStatus (alloc6, HW-proven); +0x08 is CCB-CTL
	 * {Woff,Roff}; +0xDC is a separate EDM/DPM private buffer (alloc0x18,
	 * >=0x3810), NOT the ring. Previously +0x60/+0xDC were swapped so the EDM
	 * read a zeroed command -> Data[1]=0 -> handler wrote to a dummy page. */
	sgx3d_write_u32(&s->edm_ctl, 0x00, edm);			/* self */
	sgx3d_write_u32(&s->edm_ctl, 0x04, (u32)s->host_ctl.gpu_va);	/* HOST-CTL (InitStatus) */
	sgx3d_write_u32(&s->edm_ctl, 0x08, (u32)s->ccb_ctl.gpu_va);	/* CCB-CTL {Woff,Roff} */
	sgx3d_write_u32(&s->edm_ctl, 0x60, (u32)s->kccb.gpu_va);		/* kernel CCB command ring */
	sgx3d_write_u32(&s->edm_ctl, 0xDC, (u32)s->edm_dpm.gpu_va);	/* EDM/DPM private buf */
	sgx3d_write_u32(&s->edm_ctl, 0xD0, SGX_KERNEL_DATA_HEAP_BASE);	/* heap base */
	sgx3d_write_u32(&s->edm_ctl, 0xD4,				/* EDM status 0 */
			((u32)s->edm_status[0].gpu_va - SGX_KERNEL_DATA_HEAP_BASE) | 0x80000000u);
	sgx3d_write_u32(&s->edm_ctl, 0xD8,				/* EDM status 1 */
			((u32)s->edm_status[1].gpu_va - SGX_KERNEL_DATA_HEAP_BASE) | 0x80000000u);
	/* NOTE: +0xDC is the kernel-CCB ring (set above); dpm_temp is NOT an
	 * EDM-CTL field per the decomp - kept allocated but unreferenced here. */

	dev_info(dev->dev,
		 "sgx3d: relocated (uk=%#x pds1=%#x pds2=%#x sec=%#x pri2=%#x edm=%#x)\n",
		 uk, p1, p2, pb, pc, edm);
}

static int sgx3d_setup_buffers(struct drm_device *dev)
{
	struct sgx3d_state *s = &g_sgx3d;
	unsigned long base = sgx3d_code_base;
	unsigned long ta_va = base + ALIGN(SGX_FW_MAIN_SIZE, SGX_FW_CODE_ALIGN);
	unsigned long td_va = ta_va + ALIGN(SGX_FW_TA_SIZE, SGX_FW_CODE_ALIGN);
	unsigned long spm_va = td_va + ALIGN(SGX_FW_3D_SIZE, SGX_FW_CODE_ALIGN);
	unsigned long d2_va = spm_va + ALIGN(SGX_FW_SPM_SIZE, SGX_FW_CODE_ALIGN);
	int ret;

	/* Code blobs mapped contiguously in the kernel code heap. Main at
	 * base so USE_CODE_BASE(15)=base gives delta==offset. All 5 USE programs
	 * (main/TA/3D/SPM/2D) are uploaded - the init EDM references all of them. */
	ret = sgx3d_load_blob(dev, &s->main_code, SGX_FW_MAIN_NAME,
			      SGX_FW_MAIN_SIZE, base);
	if (ret)
		return ret;
	ret = sgx3d_load_blob(dev, &s->ta_code, SGX_FW_TA_NAME,
			      SGX_FW_TA_SIZE, ta_va);
	if (ret)
		goto err_ta;
	ret = sgx3d_load_blob(dev, &s->td_code, SGX_FW_3D_NAME,
			      SGX_FW_3D_SIZE, td_va);
	if (ret)
		goto err_td;
	ret = sgx3d_load_blob(dev, &s->spm_code, SGX_FW_SPM_NAME,
			      SGX_FW_SPM_SIZE, spm_va);
	if (ret)
		goto err_spm;
	ret = sgx3d_load_blob(dev, &s->d2_code, SGX_FW_2D_NAME,
			      SGX_FW_2D_SIZE, d2_va);
	if (ret)
		goto err_d2;

	/*
	 * Mailbox / control BOs placed in the KERNEL_DATA heap (0xFA000000),
	 * where the DDK allocates them and where the ukernel's baked-in
	 * host-ctl pointer expects them. host-ctl is the first allocation.
	 */
	ret = sgx3d_alloc_bo(dev, &s->host_ctl, PAGE_SIZE, "sgx3d_hostctl",
			     PAGE_SIZE, SGX_KERNEL_DATA_HEAP_BASE);
	if (ret)
		goto err_hostctl;
	ret = sgx3d_alloc_bo(dev, &s->kccb, 0x10000, "sgx3d_kccb",
			     PAGE_SIZE, SGX_KERNEL_DATA_HEAP_BASE + 0x10000);
	if (ret)
		goto err_kccb;
	ret = sgx3d_alloc_bo(dev, &s->kicker, PAGE_SIZE, "sgx3d_kicker",
			     PAGE_SIZE, SGX_KERNEL_DATA_HEAP_BASE + 0x20000);
	if (ret)
		goto err_kicker;
	/* CCB-CTL (separate 8B buffer) + alloc[6] scratch - decomp EDM-CTL layout. */
	ret = sgx3d_alloc_bo(dev, &s->ccb_ctl, PAGE_SIZE, "sgx3d_ccbctl",
			     PAGE_SIZE, SGX_KERNEL_DATA_HEAP_BASE + 0x21000);
	if (ret)
		goto err_kicker;
	ret = sgx3d_alloc_bo(dev, &s->edm_misc6, PAGE_SIZE, "sgx3d_misc6",
			     PAGE_SIZE, SGX_KERNEL_DATA_HEAP_BASE + 0x22000);
	if (ret)
		goto err_kicker;

	sgx3d_zero(&s->host_ctl);
	sgx3d_zero(&s->kccb);
	sgx3d_zero(&s->kicker);
	sgx3d_zero(&s->ccb_ctl);
	sgx3d_zero(&s->edm_misc6);

	/* TA3D-CTL buffer (KERNEL_DATA heap) - blob[5912] points here. */
	ret = sgx3d_alloc_bo(dev, &s->edm_ctl, PAGE_SIZE, "sgx3d_ta3dctl",
			     PAGE_SIZE, SGX_KERNEL_DATA_HEAP_BASE + 0x30000);
	if (ret)
		goto err_pds;
	sgx3d_zero(&s->edm_ctl);

	/* EDM status buffers (0x40000 each) + DPM temp state (KERNEL_DATA heap). */
	ret = sgx3d_alloc_bo(dev, &s->edm_status[0], 0x40000, "sgx3d_edmst0",
			     PAGE_SIZE, SGX_KERNEL_DATA_HEAP_BASE + 0x40000);
	if (ret)
		goto err_pds;
	ret = sgx3d_alloc_bo(dev, &s->edm_status[1], 0x40000, "sgx3d_edmst1",
			     PAGE_SIZE, SGX_KERNEL_DATA_HEAP_BASE + 0x100000);
	if (ret)
		goto err_pds;
	ret = sgx3d_alloc_bo(dev, &s->dpm_temp, 0x4000, "sgx3d_dpmtemp",
			     PAGE_SIZE, SGX_KERNEL_DATA_HEAP_BASE + 0x1C0000);
	if (ret)
		goto err_pds;
	/* EDM/DPM private buffer for EDM-CTL+0xDC (must be >=0x3810, separate from
	 * the ring and from dpm_temp which is the GETMISCINFO output). */
	ret = sgx3d_alloc_bo(dev, &s->edm_dpm, 0x4000, "sgx3d_edmdpm",
			     PAGE_SIZE, SGX_KERNEL_DATA_HEAP_BASE + 0x1D0000);
	if (ret)
		goto err_pds;
	/* 2D render: HW context + client CCB (any VA - DMA'd via SGX MMU). */
	ret = sgx3d_alloc_bo(dev, &s->hw2dctx, PAGE_SIZE, "sgx3d_hw2dctx",
			     PAGE_SIZE, SGX_KERNEL_DATA_HEAP_BASE + 0x1E0000);
	if (ret)
		goto err_pds;
	ret = sgx3d_alloc_bo(dev, &s->client_ccb, PAGE_SIZE, "sgx3d_2dccb",
			     PAGE_SIZE, SGX_KERNEL_DATA_HEAP_BASE + 0x1F0000);
	if (ret)
		goto err_pds;
	/* dst surface MUST sit below 0x10000000 - the 2D core dst addr is 28-bit
	 * (PSB_2D_DST_ADDR_MASK 0x0FFFFFFC). Bind it low in pf_pd. */
	ret = sgx3d_alloc_bo(dev, &s->dst_surf, 0x10000, "sgx3d_dst2d",
			     PAGE_SIZE, 0x04000000);
	if (ret)
		goto err_pds;
	sgx3d_zero(&s->hw2dctx);
	sgx3d_zero(&s->client_ccb);
	sgx3d_zero(&s->dst_surf);
	sgx3d_zero(&s->edm_status[0]);
	sgx3d_zero(&s->edm_status[1]);
	sgx3d_zero(&s->dpm_temp);
	sgx3d_zero(&s->edm_dpm);

	/* 5 init-PDS programs uploaded to the KERNEL_CODE heap (after td). */
	{
		static const struct { const u8 *d; u32 n; } pdsrc[5] = {
			{ sgx535_pds1_evt,      sizeof(sgx535_pds1_evt) },
			{ sgx535_pds2_elb,      sizeof(sgx535_pds2_elb) },
			{ sgx535_pds_primary1,  sizeof(sgx535_pds_primary1) },
			{ sgx535_pds_secondary, sizeof(sgx535_pds_secondary) },
			{ sgx535_pds_primary2,  sizeof(sgx535_pds_primary2) },
		};
		unsigned long pv = d2_va + ALIGN(SGX_FW_2D_SIZE, SGX_FW_CODE_ALIGN);
		int k;

		for (k = 0; k < 5; k++, pv += 0x8000) {
			ret = sgx3d_alloc_bo(dev, &s->pds[k], PAGE_SIZE,
					     "sgx3d_pds", PAGE_SIZE, pv);
			if (ret)
				goto err_pds;
			sgx3d_zero(&s->pds[k]);
			sgx3d_upload(&s->pds[k], pdsrc[k].d, pdsrc[k].n);
		}
	}

	/* Relocate the blob + PDS data segments now that all VAs are known. */
	sgx3d_relocate(dev);

	/* SGXReset scratch pages - MUST exist before any reset fault-drain. */
	s->reset_pd = alloc_page(GFP_DMA32 | __GFP_ZERO);
	s->reset_pt = alloc_page(GFP_DMA32 | __GFP_ZERO);
	if (!s->reset_pd || !s->reset_pt) {
		ret = -ENOMEM;
		goto err_scratch;
	}

	s->ready = true;
	dev_info(dev->dev, "sgx3d: buffers up (code_base=%#lx ta=%#lx 3d=%#lx "
		 "hostctl=%#lx kccb=%#lx kicker=%#lx)\n",
		 base, ta_va, td_va, s->host_ctl.gpu_va, s->kccb.gpu_va,
		 s->kicker.gpu_va);
	return 0;

err_scratch:
	if (s->reset_pd)
		__free_page(s->reset_pd);
	if (s->reset_pt)
		__free_page(s->reset_pt);
	s->reset_pd = s->reset_pt = NULL;
err_pds:
	{
		int k;

		for (k = 0; k < 5; k++)
			sgx3d_free_bo(dev, &s->pds[k]);
	}
	sgx3d_free_bo(dev, &s->dpm_temp);
	sgx3d_free_bo(dev, &s->edm_dpm);
	sgx3d_free_bo(dev, &s->dst_surf);
	sgx3d_free_bo(dev, &s->client_ccb);
	sgx3d_free_bo(dev, &s->hw2dctx);
	sgx3d_free_bo(dev, &s->edm_status[0]);
	sgx3d_free_bo(dev, &s->edm_status[1]);
	sgx3d_free_bo(dev, &s->edm_ctl);
	sgx3d_free_bo(dev, &s->kicker);
err_kicker:
	sgx3d_free_bo(dev, &s->kccb);
err_kccb:
	sgx3d_free_bo(dev, &s->host_ctl);
err_hostctl:
	sgx3d_free_bo(dev, &s->d2_code);
err_d2:
	sgx3d_free_bo(dev, &s->spm_code);
err_spm:
	sgx3d_free_bo(dev, &s->td_code);
err_td:
	sgx3d_free_bo(dev, &s->ta_code);
err_ta:
	sgx3d_free_bo(dev, &s->main_code);
	return ret;
}

static void sgx3d_teardown_buffers(struct drm_device *dev)
{
	struct sgx3d_state *s = &g_sgx3d;

	if (s->reset_pd)
		__free_page(s->reset_pd);
	if (s->reset_pt)
		__free_page(s->reset_pt);
	s->reset_pd = s->reset_pt = NULL;
	{ int k; for (k = 0; k < 5; k++) sgx3d_free_bo(dev, &s->pds[k]); }
	sgx3d_free_bo(dev, &s->dpm_temp);
	sgx3d_free_bo(dev, &s->edm_dpm);
	sgx3d_free_bo(dev, &s->dst_surf);
	sgx3d_free_bo(dev, &s->client_ccb);
	sgx3d_free_bo(dev, &s->hw2dctx);
	sgx3d_free_bo(dev, &s->edm_status[0]);
	sgx3d_free_bo(dev, &s->edm_status[1]);
	sgx3d_free_bo(dev, &s->edm_ctl);
	sgx3d_free_bo(dev, &s->edm_misc6);
	sgx3d_free_bo(dev, &s->ccb_ctl);
	sgx3d_free_bo(dev, &s->kicker);
	sgx3d_free_bo(dev, &s->kccb);
	sgx3d_free_bo(dev, &s->host_ctl);
	sgx3d_free_bo(dev, &s->d2_code);
	sgx3d_free_bo(dev, &s->spm_code);
	sgx3d_free_bo(dev, &s->td_code);
	sgx3d_free_bo(dev, &s->ta_code);
	sgx3d_free_bo(dev, &s->main_code);
	s->ready = false;
	s->init_complete = false;
}

/* ------------------------------------------------------------------ */
/* SGXReset - bounded transliteration of sgxreset.c:281.               */
/* Caller holds power ref + g_sgx3d_lock. Returns 0 or -ETIMEDOUT.      */
/* ------------------------------------------------------------------ */

static int sgx3d_bounded_wait_bif_idle(struct drm_psb_private *dev_priv)
{
	int i;

	/* Posted InvalDC then wait EUR_CR_BIF_MEM_REQ_STAT reads == 0. */
	PSB_WSGX32(SGX_BIF_CTRL_INVALDC, PSB_CR_BIF_CTRL);
	PSB_RSGX32(PSB_CR_BIF_CTRL);		/* post */
	PSB_WSGX32(0, PSB_CR_BIF_CTRL);
	PSB_RSGX32(PSB_CR_BIF_CTRL);		/* post */

	for (i = 0; i < SGX_RESET_DRAIN_TRIES; i++) {
		u32 st = PSB_RSGX32(SGX_BIF_MEM_REQ_STAT);

		if ((st & SGX_BIF_MEM_REQ_STAT_READS_MASK) == 0)
			return 0;
		udelay(SGX_POLL_INTERVAL_US);
	}
	return -ETIMEDOUT;
}

static int sgx3d_reset(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct sgx3d_state *s = &g_sgx3d;
	u32 reset_pd_pa;
	int ret;

	if (!s->reset_pd || !s->reset_pt) {
		dev_err(dev->dev, "sgx3d: reset scratch missing - abort\n");
		return -EINVAL;
	}

	/* (a) Assert soft-reset on all blocks including BIF. */
	PSB_WSGX32(SGX_SR_ALL_NOBIF | SGX_SR_BIF, PSB_CR_SOFT_RESET);
	PSB_RSGX32(PSB_CR_SOFT_RESET);
	udelay(100);				/* GUESSED settle time */

	/* (d) BIF init: clear ctrl + dir-list base. */
	PSB_WSGX32(0, PSB_CR_BIF_CTRL);
	PSB_RSGX32(PSB_CR_BIF_CTRL);
	PSB_WSGX32(0, PSB_CR_BIF_DIR_LIST_BASE0);
	PSB_RSGX32(PSB_CR_BIF_DIR_LIST_BASE0);

	/* (e) Program the transient reset PD so a fault has somewhere to walk.
	 * Point its single PDE at the reset PT (both zeroed scratch pages). */
	reset_pd_pa = page_to_pfn(s->reset_pd) << PAGE_SHIFT;
	/* GUESSED: PDE[0] = reset_pt phys | VALID(1). Real PDE encoding is
	 * MMU-format specific; this only has to be a walkable non-NULL page. */
	{
		void *pd = kmap(s->reset_pd);
		*(u32 *)pd = (page_to_pfn(s->reset_pt) << PAGE_SHIFT) | 1u;
		kunmap(s->reset_pd);
	}
	PSB_WSGX32(reset_pd_pa, PSB_CR_BIF_DIR_LIST_BASE0);
	PSB_RSGX32(PSB_CR_BIF_DIR_LIST_BASE0);

	/* (f) Deassert BIF reset (keep other blocks in reset). */
	PSB_WSGX32(SGX_SR_ALL_NOBIF, PSB_CR_SOFT_RESET);
	PSB_RSGX32(PSB_CR_SOFT_RESET);

	/* (g) InvalDC + bounded wait for BIF idle. */
	ret = sgx3d_bounded_wait_bif_idle(dev_priv);
	if (ret) {
		dev_err(dev->dev, "sgx3d: BIF drain timeout in reset\n");
		/*
		 * Must not leave the SGX blocks held in soft-reset on the error
		 * path: that can wedge the shared 2D/display engine. Always
		 * release SOFT_RESET before bailing.
		 */
		PSB_WSGX32(0, PSB_CR_SOFT_RESET);
		PSB_RSGX32(PSB_CR_SOFT_RESET);
		return ret;
	}

	/* (h) Drain any pending BIF fault (read-to-clear, bounded). */
	{
		int i;

		for (i = 0; i < 8; i++) {
			u32 intst = PSB_RSGX32(PSB_CR_BIF_INT_STAT);

			if (!intst)
				break;
			PSB_RSGX32(PSB_CR_BIF_FAULT);	/* read fault addr */
			PSB_WSGX32(SGX_BIF_CTRL_INVALDC, PSB_CR_BIF_CTRL);
			PSB_RSGX32(PSB_CR_BIF_CTRL);
			PSB_WSGX32(0, PSB_CR_BIF_CTRL);
			PSB_RSGX32(PSB_CR_BIF_CTRL);
		}
	}

	/* (i) Program the REAL render PD for ctx1 (pf_pd) + ctx0 default.
	 * set_pd_context writes BIF_DIR_LIST_BASE0/1 and flushes. */
	psb_mmu_set_pd_context(psb_mmu_get_default_pd(dev_priv->mmu), 0);
	psb_mmu_set_pd_context(dev_priv->pf_pd, 1);

	/* (j) 2D request base = gatt_start (matches psb_do_init). */
	PSB_WSGX32(dev_priv->gtt.gatt_start, PSB_CR_BIF_TWOD_REQ_BASE);
	PSB_RSGX32(PSB_CR_BIF_TWOD_REQ_BASE);

	/* (k) final InvalDC. */
	PSB_WSGX32(SGX_BIF_CTRL_INVALDC, PSB_CR_BIF_CTRL);
	PSB_RSGX32(PSB_CR_BIF_CTRL);
	PSB_WSGX32(0, PSB_CR_BIF_CTRL);
	PSB_RSGX32(PSB_CR_BIF_CTRL);

	/* (l) Release all soft-resets. */
	PSB_WSGX32(0, PSB_CR_SOFT_RESET);
	PSB_RSGX32(PSB_CR_SOFT_RESET);
	udelay(100);				/* GUESSED settle time */

	dev_info(dev->dev, "sgx3d: SGXReset done\n");
	return 0;
}

/* ------------------------------------------------------------------ */
/* M4: post-reset base regs + host-ctl init + boot kick + poll.        */
/* ------------------------------------------------------------------ */

/*
 * Patch the encoded USE service addresses into the host-ctl / init template.
 * The precise template layout is only partially recovered (OPEN QUESTION b);
 * these struct offsets are GUESSED from the recon (0x2c..0x3c primary slots).
 * Wrong offsets == ukernel jumps to garbage; the bounded poll makes that
 * recoverable, and these are the first knobs to tune on hardware.
 */
static void sgx3d_patch_service_addrs(struct drm_device *dev)
{
	struct sgx3d_state *s = &g_sgx3d;
	u32 base = sgx3d_code_base;
	u32 mainva = s->main_code.gpu_va;

	/* delta==offset holds because mainva == base. */
	u32 ev   = sgx_pack_use_addr(mainva, SGX_UK_OFF_EVENTHANDLER, base);
	u32 kta  = sgx_pack_use_addr(mainva, SGX_UK_OFF_HOSTKICK_TA, base);
	u32 k2d  = sgx_pack_use_addr(mainva, SGX_UK_OFF_HOSTKICK_2D, base);
	u32 ktr  = sgx_pack_use_addr(mainva, SGX_UK_OFF_HOSTKICK_TRANSFER, base);
	u32 kcl  = sgx_pack_use_addr(mainva, SGX_UK_OFF_HOSTKICK_CLEANUP, base);

	/* GUESSED struct offsets (see comment above). */
	sgx3d_write_u32(&s->host_ctl, 0x2c, ev);
	sgx3d_write_u32(&s->host_ctl, 0x30, kta);
	sgx3d_write_u32(&s->host_ctl, 0x34, k2d);
	sgx3d_write_u32(&s->host_ctl, 0x38, ktr);
	sgx3d_write_u32(&s->host_ctl, 0x3c, kcl);

	dev_info(dev->dev, "sgx3d: service addrs ev=%#x kta=%#x (code_base=%#x)\n",
		 ev, kta, base);
}

/*
 * DDK SGX535 init register scripts, extracted verbatim from srvinit (SGXInit
 * disasm). Part1 runs BEFORE SGXReset (clock-gate enable); Part2 runs AFTER,
 * before the boot EVENT_KICK, and sets up the EVENT/USE/PDS/DPM/EDM state that
 * routes an EVENT_KICK into the ukernel instead of faulting at VA 0.
 */
static void sgx3d_run_part1(struct drm_psb_private *dev_priv)
{
	PSB_WSGX32(0x00111111, 0x0000);	/* CLKGATECTL (0x111111, CONFIRMED from Ghidra) */
	PSB_WSGX32(0x00000000, 0x0A58);
	PSB_WSGX32(0x00000000, 0x0ACC);	/* EVENT_TIMER */
	PSB_RSGX32(0x0000);
}

static void sgx3d_run_part2(struct drm_psb_private *dev_priv)
{
	struct sgx3d_state *s = &g_sgx3d;

	PSB_WSGX32(0x00004000, 0x0130);	/* EVENT_HOST_ENABLE: SW_EVENT */
	PSB_WSGX32(0x00000000, 0x0110);	/* EVENT_HOST_ENABLE2 */
	PSB_WSGX32(0x00000000, 0x013C);
	PSB_WSGX32(0x00000000, 0x0A7C);
	PSB_WSGX32(0x00000000, 0x0A80);
	PSB_WSGX32(0x003F8200, 0x0A74);
	PSB_WSGX32(0x00000000, 0x04E8);
	PSB_WSGX32(0x00000000, 0x0A78);
	PSB_WSGX32(0x00000044, 0x0AAC);
	PSB_WSGX32(0x0000004C, 0x0ABC);	/* EUR_CR_PDS */
	PSB_WSGX32(0x0000FFFF, 0x0804);
	PSB_WSGX32(0x0007C000, 0x0A00);
	/* USE_CODE_BASE(15) = (codeBase>>7)|(DM3<<25); PDS_EXEC_BASE = PDS heap. */
	PSB_WSGX32(sgx_pack_use_code_base(sgx3d_code_base), 0x0A48);
	PSB_WSGX32(SGX_PDS_EXEC_BASE_DEV_VA, 0x0AB8);
	PSB_WSGX32(0x00000100, 0x0B30);
	/* EVENT_KICKER = device VA of the CCB event-kicker page. */
	PSB_WSGX32((u32)s->kicker.gpu_va, 0x0AC4);
	/*
	 * 0x0A68 = the PRIMARY1 init-PDS device VA (CONFIRMED from Ghidra SGXInit
	 * lines 489-493). THIS is the PDS the boot EVENT_KICK runs to bootstrap the
	 * EDM; it DOUTUs INITUKERNELPRIMARY1. Was wrongly set to host-ctl -> the
	 * hardware ran a zeroed page -> the microkernel never executed.
	 */
	PSB_WSGX32((u32)s->pds[2].gpu_va & 0xFFFFFFF0, 0x0A68);
	PSB_WSGX32(0x00000003, 0x0A6C);
	PSB_WSGX32(0x00003D01, 0x0A70);
	PSB_WSGX32(0x2A04F00F, 0x0A58);
	PSB_RSGX32(0x0A58);
}

static int sgx3d_boot(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct sgx3d_state *s = &g_sgx3d;
	u32 status = 0;
	int i, ret;

	/* --- Part2 init register script (post-reset), extracted from srvinit. --- */
	sgx3d_run_part2(dev_priv);
	dev_info(dev->dev,
		 "sgx3d: Part2 script done (USE_CODE_BASE=%#x PDS_EXEC_BASE=%#x kicker_va=%#lx)\n",
		 sgx_pack_use_code_base(sgx3d_code_base), SGX_PDS_EXEC_BASE_DEV_VA,
		 s->kicker.gpu_va);

	/* --- host-ctl init + service addrs --- */
	sgx3d_zero(&s->host_ctl);
	sgx3d_patch_service_addrs(dev);

	/* Clear ui32InitStatus (the dword we poll). */
	sgx3d_write_u32(&s->host_ctl, sgx3d_initstatus_off, 0);

	/* Zero the device-memory kicker counter (distinct from the doorbell). */
	sgx3d_write_u32(&s->kicker, 0, 0);
	s->kick_count = 0;

	/* --- ordering barrier before the doorbell --- */
	wmb();
	psb_mmu_flush(dev_priv->mmu);		/* flush WC/UC render pages */

	/* Increment device-memory kicker then ring the ONE boot doorbell. */
	s->kick_count = (s->kick_count + 1) & 0xff;
	sgx3d_write_u32(&s->kicker, 0, s->kick_count);
	wmb();

	/* Pre-kick state (disambiguate: does the ukernel actually run?). Do NOT
	 * clobber EVENT_KICKER(0x0AC4) - Part2 already set it to the kicker VA. */
	dev_info(dev->dev,
		 "sgx3d: PRE-KICK EDMTask0=%#x EDMTask1=%#x EVENT_STATUS=%#x EVENT_KICKER=%#x\n",
		 PSB_RSGX32(0x0B10), PSB_RSGX32(0x0B1C), PSB_RSGX32(0x012C),
		 PSB_RSGX32(PSB_CR_EVENT_KICKER));

	PSB_WSGX32(_PSB_CE_KICK_NOW, PSB_CR_EVENT_KICK);
	PSB_RSGX32(PSB_CR_EVENT_KICK);		/* post */

	/*
	 * CALIBRATION: sample PDS_PC_BASE(0x0B2C) rapidly right after the kick to
	 * watch the PDS fetch address walk primary1 -> secondary and to confirm the
	 * VA>>2 encoding (primary1 VA>>2 = pds[2].gpu_va>>2; secondary = pds[3]>>2).
	 * Also track MEM_REQ_STAT to see exactly when the read gets stuck.
	 */
	{
		int j;
		u32 exp_p1 = (u32)s->pds[2].gpu_va >> 2;
		u32 exp_sec = (u32)s->pds[3].gpu_va >> 2;

		dev_info(dev->dev,
			 "sgx3d: CAL expect PDS_PC primary1=%#x secondary=%#x\n",
			 exp_p1, exp_sec);
		for (j = 0; j < 10; j++) {
			dev_info(dev->dev,
				 "sgx3d: CAL[%d] PDS_PC=%#x MEM_REQ=%#x EDMTask0=%#x\n",
				 j, PSB_RSGX32(0x0B2C), PSB_RSGX32(0x0CA8),
				 PSB_RSGX32(0x0B10));
			udelay(20);
		}
	}

	dev_info(dev->dev, "sgx3d: boot EVENT_KICK rung, polling InitStatus@%u\n",
		 sgx3d_initstatus_off);

	/* --- bounded poll for INIT_COMPLETE; re-kick periodically in case the
	 * EDM advances one init phase per event (probe: does it self-schedule?). */
	for (i = 0; i < SGX_POLL_TRIES; i++) {
		ret = sgx3d_read_u32(&s->host_ctl, sgx3d_initstatus_off,
				     &status);
		if (ret)
			return ret;
		if (status & SGX_USSE_EDM_INIT_COMPLETE) {
			dev_info(dev->dev,
				 "sgx3d: INIT_COMPLETE (status=%#x) after %d polls\n",
				 status, i);
			s->init_complete = true;
			return 0;
		}
		/* Re-kick every ~2ms (bump the device kicker counter first). */
		if ((i % 40) == 39) {
			s->kick_count = (s->kick_count + 1) & 0xff;
			sgx3d_write_u32(&s->kicker, 0, s->kick_count);
			wmb();
			PSB_WSGX32(_PSB_CE_KICK_NOW, PSB_CR_EVENT_KICK);
			PSB_RSGX32(PSB_CR_EVENT_KICK);
			if (i == 39)
				dev_info(dev->dev, "sgx3d: re-kick, EDMTask0=%#x\n",
					 PSB_RSGX32(0x0B10));
		}
		udelay(SGX_POLL_INTERVAL_US);
	}
	dev_err(dev->dev, "sgx3d: after re-kicks EDMTask0=%#x\n", PSB_RSGX32(0x0B10));

	/*
	 * Diagnostic: did the ukernel execute at all? Read the EDM task-status
	 * regs, event status, and scan the first dwords of every device buffer
	 * for any write the ukernel made. Tells us if it ran and where it wrote.
	 */
	{
		u32 h0 = 0, h1 = 0, h2 = 0, kk = 0, cc0 = 0;

		sgx3d_read_u32(&s->host_ctl, 0, &h0);
		sgx3d_read_u32(&s->host_ctl, 4, &h1);
		sgx3d_read_u32(&s->host_ctl, 8, &h2);
		sgx3d_read_u32(&s->kicker, 0, &kk);
		sgx3d_read_u32(&s->kccb, 0, &cc0);
		dev_err(dev->dev,
			"sgx3d: DIAG EDMTask0[0xB10]=%#x EDMTask1[0xB1C]=%#x EVENT_STATUS[0x12C]=%#x CLKGATE[0x0]=%#x\n",
			PSB_RSGX32(0x0B10), PSB_RSGX32(0x0B1C),
			PSB_RSGX32(0x012C), PSB_RSGX32(0x0000));
		dev_err(dev->dev,
			"sgx3d: DIAG hostctl[0,4,8]=%#x,%#x,%#x kicker=%#x kccb[0]=%#x kickcnt=%u\n",
			h0, h1, h2, kk, cc0, s->kick_count);

		/* EDM status buffers: the microkernel writes debug/panic here. */
		{
			u32 e00 = 0, e04 = 0, e08 = 0, e0c = 0, f00 = 0, f04 = 0;
			u32 t0, t1, t2;

			sgx3d_read_u32(&s->edm_status[0], 0, &e00);
			sgx3d_read_u32(&s->edm_status[0], 4, &e04);
			sgx3d_read_u32(&s->edm_status[0], 8, &e08);
			sgx3d_read_u32(&s->edm_status[0], 0xc, &e0c);
			sgx3d_read_u32(&s->edm_status[1], 0, &f00);
			sgx3d_read_u32(&s->edm_status[1], 4, &f04);
			dev_err(dev->dev,
				"sgx3d: DIAG edmst0[0,4,8,c]=%#x,%#x,%#x,%#x edmst1[0,4]=%#x,%#x\n",
				e00, e04, e08, e0c, f00, f04);
			/* Time-series: is EDMTask0 stuck or advancing? */
			t0 = PSB_RSGX32(0x0B10);
			udelay(200);
			t1 = PSB_RSGX32(0x0B10);
			udelay(2000);
			t2 = PSB_RSGX32(0x0B10);
			dev_err(dev->dev,
				"sgx3d: DIAG EDMTask0 t0=%#x t1=%#x t2=%#x (advancing? %d) BIF_INT=%#x BIF_FAULT=%#x\n",
				t0, t1, t2, (t2 != t0),
				PSB_RSGX32(PSB_CR_BIF_INT_STAT), PSB_RSGX32(PSB_CR_BIF_FAULT));
		}

		/*
		 * Full DDK SGXDumpDebugInfo register set (sgxinit.c:973). These
		 * distinguish the halt modes we cannot otherwise tell apart:
		 *   BIF_FAULT != 0            -> ukernel touched an unmapped VA
		 *                                (the fault addr maps back to a buffer)
		 *   BIF_MEM_REQ_STAT reads>0  -> STUCK on a memory read that never
		 *                                returns (hang, not a fault)
		 *   EVENT_STATUS DPM_OOM bits -> DPM out of parameter memory
		 *   PDS_PC_BASE               -> where PDS execution stopped
		 */
		{
			u32 evst  = PSB_RSGX32(0x012C);	/* EUR_CR_EVENT_STATUS  */
			u32 evst2 = PSB_RSGX32(0x0118);	/* EUR_CR_EVENT_STATUS2 */
			u32 bifc  = PSB_RSGX32(0x0C00);	/* EUR_CR_BIF_CTRL      */
			u32 bifm  = PSB_RSGX32(0x0CA8);	/* EUR_CR_BIF_MEM_REQ_STAT */
			u32 pdspc = PSB_RSGX32(0x0B2C);	/* EUR_CR_PDS_PC_BASE   */
			u32 clk   = PSB_RSGX32(0x0000);	/* EUR_CR_CLKGATECTL    */

			dev_err(dev->dev,
				"sgx3d: DIAG DDK EVENT_STATUS=%#x STATUS2=%#x BIF_CTRL=%#x MEM_REQ_STAT=%#x PDS_PC_BASE=%#x CLKGATE=%#x\n",
				evst, evst2, bifc, bifm, pdspc, clk);
			/* Active page-directory + BIF fault addr: confirm the EDM's memory
			 * context maps the 0xFA data heap (BIF_DIR_LIST_BASE0=0x0C84,
			 * BANK0=0x0C78, FAULT addr in bits 31:12 of 0x0C08). */
			dev_err(dev->dev,
				"sgx3d: DIAG DIR_LIST_BASE0=%#x BANK0=%#x FAULT_ADDR=%#x  (pds[2]VA>>2=%#x pds[3]VA>>2=%#x)\n",
				PSB_RSGX32(0x0C84), PSB_RSGX32(0x0C78),
				PSB_RSGX32(0x0C08) & 0xFFFFF000u,
				(u32)s->pds[2].gpu_va >> 2, (u32)s->pds[3].gpu_va >> 2);
			if (evst & (SGX_ES_DPM_OOM_GBL | SGX_ES_DPM_OOM_MT))
				dev_err(dev->dev, "sgx3d: DIAG *** DPM OUT OF MEMORY ***\n");
			if (bifm & SGX_BIF_MEM_REQ_STAT_READS_MASK)
				dev_err(dev->dev,
					"sgx3d: DIAG *** %u BIF reads outstanding (ukernel STUCK on memory) ***\n",
					bifm & SGX_BIF_MEM_REQ_STAT_READS_MASK);
		}

		/* Scan the whole host-ctl + kccb pages for ANY ukernel write. */
		{
			int off, found = 0;
			u32 v;

			for (off = 0; off < PAGE_SIZE && found < 6; off += 4) {
				if (!sgx3d_read_u32(&s->host_ctl, off, &v) && v) {
					dev_err(dev->dev,
						"sgx3d: DIAG hostctl+%#x = %#x\n",
						off, v);
					found++;
				}
			}
			for (off = 0; off < 2 * PAGE_SIZE && found < 12; off += 4) {
				if (!sgx3d_read_u32(&s->kccb, off, &v) && v) {
					dev_err(dev->dev,
						"sgx3d: DIAG kccb+%#x = %#x\n",
						off, v);
					found++;
				}
			}
			if (!found)
				dev_err(dev->dev, "sgx3d: DIAG no ukernel writes in hostctl/kccb pages\n");
		}
	}

	dev_err(dev->dev,
		"sgx3d: INIT_COMPLETE timeout (last status=%#x) - will reset+bail\n",
		status);
	return -ETIMEDOUT;
}

/* ------------------------------------------------------------------ */
/* Orchestrator - the on-demand init.                                   */
/* ------------------------------------------------------------------ */

static int sgx3d_validate_hw(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	u32 rev = PSB_RSGX32(PSB_CR_CORE_REVISION);

	if (rev != 0x00010201) {
		dev_err(dev->dev,
			"sgx3d: CORE_REVISION %#x != 0x00010201 - refusing 3D\n",
			rev);
		return -ENODEV;
	}
	return 0;
}

int sgx3d_init(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct sgx3d_state *s = &g_sgx3d;
	int ret;

	if (!sgx3d_enable) {
		dev_info(dev->dev, "sgx3d: disabled (sgx3d_enable=0), 2D-only\n");
		return -EPERM;
	}

	mutex_lock(&g_sgx3d_lock);

	if (s->latched_off) {
		dev_warn(dev->dev, "sgx3d: latched off after repeated failures\n");
		ret = -EIO;
		goto out;
	}
	if (s->init_complete) {
		dev_info(dev->dev, "sgx3d: already initialised\n");
		ret = 0;
		goto out;
	}

	s->dev = dev;

	/* All MMIO below inside a forced power window. */
	if (!gma_power_begin(dev, true)) {
		dev_err(dev->dev, "sgx3d: gma_power_begin failed - abort\n");
		ret = -EIO;
		goto out;
	}

	ret = sgx3d_validate_hw(dev);
	if (ret)
		goto out_power;

	if (!s->ready) {
		ret = sgx3d_setup_buffers(dev);	/* M3 */
		if (ret) {
			dev_err(dev->dev, "sgx3d: buffer setup failed %d\n", ret);
			goto out_power;
		}
	}

	/* Part1 (clock-gate enable) runs BEFORE the reset, per DDK sequence. */
	sgx3d_run_part1(dev_priv);

	/* SGXReset FIRST, before any kick. */
	ret = sgx3d_reset(dev);			/* part of M4 */
	if (ret)
		goto out_fail;

	ret = sgx3d_boot(dev);			/* M4 handshake */
	if (ret)
		goto out_fail;

	s->reset_fail_count = 0;
	gma_power_end(dev);
	dev_info(dev->dev, "sgx3d: 3D bring-up SUCCESS\n");
	ret = 0;
	goto out;

out_fail:
	/* Timeout/failure: drive exactly one reset, bail to 2D-only. */
	dev_err(dev->dev, "sgx3d: init failed %d - one reset + bail\n", ret);
	sgx3d_reset(dev);
	s->init_complete = false;
	if (++s->reset_fail_count >= 2) {
		s->latched_off = true;
		dev_err(dev->dev,
			"sgx3d: two consecutive failures - 3D latched OFF for module lifetime\n");
	}
	ret = -EIO;
out_power:
	gma_power_end(dev);
out:
	mutex_unlock(&g_sgx3d_lock);
	return ret;
}

void sgx3d_fini(struct drm_device *dev)
{
	mutex_lock(&g_sgx3d_lock);
	if (g_sgx3d.ready) {
		if (gma_power_begin(dev, true)) {
			sgx3d_reset(dev);
			gma_power_end(dev);
		}
		sgx3d_teardown_buffers(dev);
	}
	mutex_unlock(&g_sgx3d_lock);
}

/* ------------------------------------------------------------------ */
/* On-demand sysfs trigger (matches the tree's DEVICE_ATTR style).      */
/* echo "init" | reset | off  > /sys/.../poulsbo_sgx_3d                 */
/* ------------------------------------------------------------------ */

static ssize_t poulsbo_sgx_3d_show(struct device *kdev,
				   struct device_attribute *attr, char *buf)
{
	struct sgx3d_state *s = &g_sgx3d;

	return scnprintf(buf, PAGE_SIZE,
		"enable=%d ready=%d init_complete=%d latched_off=%d fails=%d "
		"code_base=%#x initstatus_off=%u\n",
		sgx3d_enable, s->ready, s->init_complete, s->latched_off,
		s->reset_fail_count, sgx3d_code_base, sgx3d_initstatus_off);
}

/*
 * First real CCB command submission (mirrors DDK SGXScheduleCCBCommand):
 *  - write a SGXMKIF_COMMAND {ServiceAddress, CacheControl, Data[2]} into the
 *    kernel-CCB ring at ccb_ctl.WriteOffset, ServiceAddress = HOSTKICK>>3;
 *  - advance WriteOffset (mod 256) in ccb_ctl+0; ring EVENT_KICK;
 *  - poll ccb_ctl+4 (ReadOffset) until it catches up -> ukernel consumed it.
 * PROCESS_QUEUES(6) is the most benign first command (no render state needed).
 */
/* SGX 2D-core command-stream opcodes (from the proven blt-ioctl / psb_reg). */
#define P2D_FENCE_BH	0x70000000u
#define P2D_BLIT_BH	0x80000000u
#define P2D_DST_SURF_BH	0xA0000000u
#define P2D_FLUSH_BH	0xF0000000u
#define P2D_DST_8888ARGB 0x00060000u
#define P2D_USE_FILL	0x00000000u
#define P2D_ROP3_PATCOPY 0x0000F0F0u
#define P2D_DST_ADDR_MASK 0x0FFFFFFCu

/*
 * Build a minimal no-sync 2D solid-fill for the microkernel 2D handler
 * (derived from the USSE 2D-handler decode). client_ccb = SGXMKIF_2DCMD_SHARED
 * (0x64B, all-zero = no sync) followed by the 8-dword 2D-core fill stream at
 * +0x64; hw2dctx +0x04 = client CCB base (the field the handler dereferences),
 * +0x00 = client CCB (EA base). dst is a 256x64 ARGB8888 surface at low VA.
 */
static void sgx3d_build_2d_fill(struct drm_device *dev)
{
	struct sgx3d_state *s = &g_sgx3d;
	u32 pitch = 256u * 4u;			/* bytes */
	u32 dst = (u32)s->dst_surf.gpu_va & P2D_DST_ADDR_MASK;
	u32 b = 0x64;				/* stream start (after SHARED hdr) */

	sgx3d_zero(&s->hw2dctx);
	sgx3d_zero(&s->client_ccb);		/* zeroes the 0x64 SHARED header */
	sgx3d_zero(&s->dst_surf);

	/* 2D-core fill stream (8 dwords) at client_ccb+0x64. */
	sgx3d_write_u32(&s->client_ccb, b + 0x00, P2D_FENCE_BH);
	sgx3d_write_u32(&s->client_ccb, b + 0x04, P2D_DST_SURF_BH | P2D_DST_8888ARGB | pitch);
	sgx3d_write_u32(&s->client_ccb, b + 0x08, dst);			/* dst addr slot */
	sgx3d_write_u32(&s->client_ccb, b + 0x0c, P2D_BLIT_BH | P2D_USE_FILL | P2D_ROP3_PATCOPY);
	sgx3d_write_u32(&s->client_ccb, b + 0x10, 0x00C0FFEEu);		/* fill colour */
	sgx3d_write_u32(&s->client_ccb, b + 0x14, (32u << 12) | 16u);	/* dst xy */
	sgx3d_write_u32(&s->client_ccb, b + 0x18, (64u << 12) | 32u);	/* dst wh */
	sgx3d_write_u32(&s->client_ccb, b + 0x1c, P2D_FLUSH_BH);

	/* HW 2D context: +0x04 = client CCB base (CONFIRMED deref); +0x00 = EA base. */
	sgx3d_write_u32(&s->hw2dctx, 0x00, (u32)s->client_ccb.gpu_va);
	sgx3d_write_u32(&s->hw2dctx, 0x04, (u32)s->client_ccb.gpu_va);
	wmb();

	dev_info(dev->dev,
		 "sgx3d: 2D fill built (ctx=%#x ccb=%#x dst=%#x[VA %#lx] colour=0x00C0FFEE 64x32@32,16)\n",
		 (u32)s->hw2dctx.gpu_va, (u32)s->client_ccb.gpu_va, dst,
		 s->dst_surf.gpu_va);
}

static int sgx3d_submit_ccb(struct drm_device *dev, u32 cmd_type)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct sgx3d_state *s = &g_sgx3d;
	u32 base = sgx3d_code_base, mainva = (u32)s->main_code.gpu_va;
	static const u32 hk_off[7] = {
		SGX_UK_OFF_HOSTKICK_TA,        /* 0 TA        */
		SGX_UK_OFF_HOSTKICK_TRANSFER,  /* 1 TRANSFER  */
		SGX_UK_OFF_HOSTKICK_2D,        /* 2 2D        */
		SGX_UK_OFF_HOSTKICK_POWER,     /* 3 POWER     */
		SGX_UK_OFF_HOSTKICK_CLEANUP,   /* 4 CLEANUP   */
		SGX_UK_OFF_HOSTKICK_GETMISC,   /* 5 GETMISC   */
		SGX_UK_OFF_HOSTKICK_PROCESS_Q, /* 6 PROC_QUE  */
	};
	u32 woff = 0, roff = 0, svc, i, data1 = 0;

	(void)base; (void)mainva;
	if (cmd_type >= 7)
		return -EINVAL;
	if (!s->init_complete) {
		dev_err(dev->dev, "sgx3d: CCB kick before INIT_COMPLETE\n");
		return -EAGAIN;
	}
	/* aui32HostKickAddr[type] = HOSTKICK byte-offset >> 3 (decomp lines 158-169). */
	svc = hk_off[cmd_type] >> 3;

	/* GETMISCINFO(5): Data[1] = misc-info buffer devVA; ukernel fills it with
	 * SGX features (CoreRev@+0x04 ...) and sets MISCINFO_READY(0x1) at +0x00.
	 * Reuse the (otherwise idle) dpm_temp buffer as the misc buffer. */
	if (cmd_type == 5) {
		sgx3d_zero(&s->dpm_temp);		/* clears flags incl. READY */
		sgx3d_write_u32(&s->dpm_temp, 0x00, 0x2); /* PVRSRV_USSE_MISCINFO_GET_STRUCT_SIZES */
		data1 = (u32)s->dpm_temp.gpu_va;
		wmb();
	} else if (cmd_type == 2) {
		/* 2D: Data[1] = HW 2D context devVA; Data[0] must be 0. */
		sgx3d_build_2d_fill(dev);
		data1 = (u32)s->hw2dctx.gpu_va;
	}

	sgx3d_read_u32(&s->ccb_ctl, 0, &woff);
	woff &= 255;
	sgx3d_write_u32(&s->kccb, woff * 16 + 0x0, svc);		/* ui32ServiceAddress */
	sgx3d_write_u32(&s->kccb, woff * 16 + 0x4, 0);		/* ui32CacheControl   */
	/* Data[0]: 0 for 2D (DDK sets only Data[1]); mirror for the GETMISC probe. */
	sgx3d_write_u32(&s->kccb, woff * 16 + 0x8, (cmd_type == 2) ? 0 : data1);
	sgx3d_write_u32(&s->kccb, woff * 16 + 0xc, data1);	/* ui32Data[1]        */
	wmb();
	woff = (woff + 1) & 255;
	sgx3d_write_u32(&s->ccb_ctl, 0, woff);		/* advance WriteOffset */
	wmb();

	s->kick_count = (s->kick_count + 1) & 0xff;
	sgx3d_write_u32(&s->kicker, 0, s->kick_count);
	wmb();
	dev_info(dev->dev, "sgx3d: CCB submit type=%u svc=%#x Woff->%u; ringing EVENT_KICK\n",
		 cmd_type, svc, woff);
	PSB_WSGX32(_PSB_CE_KICK_NOW, PSB_CR_EVENT_KICK);
	PSB_RSGX32(PSB_CR_EVENT_KICK);

	for (i = 0; i < 2000; i++) {
		sgx3d_read_u32(&s->ccb_ctl, 4, &roff);
		if ((roff & 255) == woff) {
			dev_info(dev->dev,
				 "sgx3d: *** CCB cmd type=%u CONSUMED *** (Woff=%u Roff=%u) after %u us\n",
				 cmd_type, woff, roff & 255, i * 10);
			/* GETMISCINFO: wait for the ukernel to fill the buffer + set
			 * MISCINFO_READY, then report what it wrote back. */
			if (cmd_type == 5) {
				u32 flags = 0, corerev = 0, coreid = 0, j;

				for (j = 0; j < 2000; j++) {
					sgx3d_read_u32(&s->dpm_temp, 0x00, &flags);
					if (flags & 0x1)	/* PVRSRV_USSE_MISCINFO_READY */
						break;
					udelay(10);
				}
				sgx3d_read_u32(&s->dpm_temp, 0x04, &corerev);
				sgx3d_read_u32(&s->dpm_temp, 0x08, &coreid);
				if (flags & 0x1)
					dev_info(dev->dev,
						 "sgx3d: *** GETMISCINFO READY: CoreRev=%#x CoreID=%#x flags=%#x *** (HW CORE_REVISION=%#x)\n",
						 corerev, coreid, flags,
						 PSB_RSGX32(PSB_CR_CORE_REVISION));
				else {
					u32 d[8], q;
					for (q = 0; q < 8; q++)
						sgx3d_read_u32(&s->dpm_temp, q * 4, &d[q]);
					dev_err(dev->dev,
						"sgx3d: GETMISCINFO READY never set. dpm_temp[0..7]=%#x %#x %#x %#x %#x %#x %#x %#x BIF_FAULT=%#x miscVA=%#x\n",
						d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
						PSB_RSGX32(0x0C08), (u32)s->dpm_temp.gpu_va);
				}
			} else if (cmd_type == 2) {
				/* 2D fill: check the 2D core ran + the dst pixels. */
				u32 p0 = 0, pfill = 0, pborder = 0, blstat, j;

				/* wait for the 2D core to finish the blit. */
				for (j = 0; j < 5000; j++) {
					blstat = PSB_RSGX32(0x0E04); /* EUR_CR_2D_BLIT_STATUS */
					if (!(blstat & 0x01000000u)) /* not BUSY */
						break;
					udelay(10);
				}
				blstat = PSB_RSGX32(0x0E04);
				/* dst pixel at fill origin (32,16): byte off = 16*pitch+32*4 */
				sgx3d_read_u32(&s->dst_surf, 0, &p0);
				sgx3d_read_u32(&s->dst_surf, 16 * 1024 + 32 * 4, &pfill);
				sgx3d_read_u32(&s->dst_surf, 0 * 1024 + 0 * 4, &pborder);
				if (pfill == 0x00C0FFEEu)
					dev_info(dev->dev,
						 "sgx3d: *** 2D FILL RENDERED *** dst[32,16]=%#x (==colour!) border=%#x BLIT_STATUS=%#x\n",
						 pfill, pborder, blstat);
				else
					dev_err(dev->dev,
						"sgx3d: 2D consumed but dst NOT filled: dst[32,16]=%#x dst[0,0]=%#x BLIT_STATUS=%#x BIF_FAULT=%#x\n",
						pfill, pborder, blstat, PSB_RSGX32(0x0C08));
			}
			return 0;
		}
		udelay(10);
	}
	dev_err(dev->dev,
		"sgx3d: CCB cmd type=%u NOT consumed (Woff=%u Roff=%#x) BIF_FAULT=%#x MEM_REQ=%#x PDS_PC=%#x EDMTask0=%#x\n",
		cmd_type, woff, roff, PSB_RSGX32(0x0C08), PSB_RSGX32(0x0CA8),
		PSB_RSGX32(0x0B2C), PSB_RSGX32(0x0B10));
	return -ETIMEDOUT;
}

static ssize_t poulsbo_sgx_3d_store(struct device *kdev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct pci_dev *pdev = to_pci_dev(kdev);
	struct drm_device *dev = pci_get_drvdata(pdev);
	int ret;

	if (!dev || !dev->dev_private)
		return -ENODEV;

	if (sysfs_streq(buf, "init")) {
		ret = sgx3d_init(dev);
	} else if (!strncmp(buf, "kick", 4)) {
		/* "kick" or "kickN" (N=0..6 command type; default 6 PROCESS_QUEUES) */
		u32 t = 6;
		if (buf[4] >= '0' && buf[4] <= '6')
			t = buf[4] - '0';
		mutex_lock(&g_sgx3d_lock);
		if (gma_power_begin(dev, true)) {
			ret = sgx3d_submit_ccb(dev, t);
			gma_power_end(dev);
		} else {
			ret = -EIO;
		}
		mutex_unlock(&g_sgx3d_lock);
	} else if (sysfs_streq(buf, "reset")) {
		mutex_lock(&g_sgx3d_lock);
		if (gma_power_begin(dev, true)) {
			ret = sgx3d_reset(dev);
			gma_power_end(dev);
		} else {
			ret = -EIO;
		}
		mutex_unlock(&g_sgx3d_lock);
	} else if (sysfs_streq(buf, "off")) {
		sgx3d_fini(dev);
		ret = 0;
	} else {
		return -EINVAL;
	}

	if (ret && ret != -EPERM)
		return ret;
	return count;
}
static DEVICE_ATTR_RW(poulsbo_sgx_3d);

/* Register/unregister from psb_driver_load / unload (see notes.md). */
int sgx3d_sysfs_register(struct drm_device *dev)
{
	return device_create_file(dev->dev, &dev_attr_poulsbo_sgx_3d);
}

void sgx3d_sysfs_unregister(struct drm_device *dev)
{
	device_remove_file(dev->dev, &dev_attr_poulsbo_sgx_3d);
}
