// SPDX-License-Identifier: GPL-2.0-only
/**************************************************************************
 * Copyright (c) 2007, Intel Corporation.
 * All Rights Reserved.
 *
 * Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 * develop this driver.
 *
 **************************************************************************/

#include <drm/drm_vblank.h>

#include <linux/string.h>

#include "mdfld_output.h"
#include "power.h"
#include "psb_drv.h"
#include "psb_intel_reg.h"
#include "psb_irq.h"
#include "psb_reg.h"

#define PSB_SGX_LAB_EVENT_MASK _PSB_CE_TWOD_COMPLETE

/*
 * Forward SGX interrupts into the coexisting pvrsrvkm (3D render) module.
 * gma500 owns the shared IRQ line and CLEARS the SGX EUR event below, running
 * first in the shared-IRQ chain -- which starves pvrsrvkm's own LISR and forces
 * its GL command-complete / DC page-flip onto a ~250ms poll timeout (~4 FPS).
 * We call this BEFORE clearing so pvrsrvkm sees the live event, runs its device
 * ISR + schedules its MISR, and the flip becomes interrupt-driven.  It is a
 * no-op (internally gated) until pvrsrvkm has finished SGX init.  Weakly
 * referenced so gma500 still links/loads if pvrsrvkm is absent.
 */
extern void gma_pvr_sgx_isr(void) __attribute__((weak));

/*
 * pvr_irq_forward - when non-zero (default), forward SGX interrupts to pvrsrvkm
 * so GL command-complete / DC page-flip is interrupt-driven (~1 ms present).
 * Set to 0 to disable the forward: pvrsrvkm falls back to its ~250 ms internal
 * poll (GL still works, ~4 FPS presents) -- a diagnostic / safety escape hatch.
 *   /sys/module/gma500_gfx/parameters/pvr_irq_forward
 */
static int pvr_irq_forward = 1;
module_param(pvr_irq_forward, int, 0644);
MODULE_PARM_DESC(pvr_irq_forward,
	"Forward SGX interrupts to pvrsrvkm for interrupt-driven GL present "
	"(1=on [default], 0=off/pvrsrvkm polls)");

struct psb_irq_bit_name {
	u32 bit;
	const char *name;
};

static const struct psb_irq_bit_name psb_sgx_stat_1_bits[] = {
	{ _PSB_CE_TA_DPM_FAULT, "TA_DPM_FAULT" },
	{ _PSB_CE_TWOD_COMPLETE, "TWOD_COMPLETE" },
	{ _PSB_CE_DPM_OUT_OF_MEMORY_ZLS, "DPM_OOM_ZLS" },
	{ _PSB_CE_DPM_TA_MEM_FREE, "DPM_TA_MEM_FREE" },
	{ _PSB_CE_PIXELBE_END_RENDER, "PIXELBE_END_RENDER" },
	{ _PSB_CE_SW_EVENT, "SW_EVENT" },
	{ _PSB_CE_TA_FINISHED, "TA_FINISHED" },
	{ _PSB_CE_TA_TERMINATE, "TA_TERMINATE" },
	{ _PSB_CE_DPM_REACHED_MEM_THRESH, "DPM_MEM_THRESH" },
	{ _PSB_CE_DPM_OUT_OF_MEMORY_GBL, "DPM_OOM_GBL" },
	{ _PSB_CE_DPM_OUT_OF_MEMORY_MT, "DPM_OOM_MT" },
	{ _PSB_CE_DPM_3D_MEM_FREE, "DPM_3D_MEM_FREE" },
};

static const struct psb_irq_bit_name psb_sgx_stat_2_bits[] = {
	{ _PSB_CE2_BIF_REQUESTER_FAULT, "BIF_REQUESTER_FAULT" },
};

static const struct psb_irq_bit_name psb_sgx_bif_requester_bits[] = {
	{ _PSB_CBI_STAT_FAULT_CACHE, "CACHE" },
	{ _PSB_CBI_STAT_FAULT_TA, "TA" },
	{ _PSB_CBI_STAT_FAULT_VDM, "VDM" },
	{ _PSB_CBI_STAT_FAULT_2D, "2D" },
	{ _PSB_CBI_STAT_FAULT_PBE, "PBE" },
	{ _PSB_CBI_STAT_FAULT_TSP, "TSP" },
	{ _PSB_CBI_STAT_FAULT_ISP, "ISP" },
	{ _PSB_CBI_STAT_FAULT_USSEPDS, "USSEPDS" },
	{ _PSB_CBI_STAT_FAULT_HOST, "HOST" },
};

/*
 * inline functions
 */

static inline u32
psb_pipestat(int pipe)
{
	if (pipe == 0)
		return PIPEASTAT;
	if (pipe == 1)
		return PIPEBSTAT;
	if (pipe == 2)
		return PIPECSTAT;
	BUG();
}

static inline u32
mid_pipe_event(int pipe)
{
	if (pipe == 0)
		return _PSB_PIPEA_EVENT_FLAG;
	if (pipe == 1)
		return _MDFLD_PIPEB_EVENT_FLAG;
	if (pipe == 2)
		return _MDFLD_PIPEC_EVENT_FLAG;
	BUG();
}

static inline u32
mid_pipe_vsync(int pipe)
{
	if (pipe == 0)
		return _PSB_VSYNC_PIPEA_FLAG;
	if (pipe == 1)
		return _PSB_VSYNC_PIPEB_FLAG;
	if (pipe == 2)
		return _MDFLD_PIPEC_VBLANK_FLAG;
	BUG();
}

static inline u32
mid_pipeconf(int pipe)
{
	if (pipe == 0)
		return PIPEACONF;
	if (pipe == 1)
		return PIPEBCONF;
	if (pipe == 2)
		return PIPECCONF;
	BUG();
}

void
psb_enable_pipestat(struct drm_psb_private *dev_priv, int pipe, u32 mask)
{
	if ((dev_priv->pipestat[pipe] & mask) != mask) {
		u32 reg = psb_pipestat(pipe);
		dev_priv->pipestat[pipe] |= mask;
		/* Enable the interrupt, clear any pending status */
		if (gma_power_begin(dev_priv->dev, false)) {
			u32 writeVal = PSB_RVDC32(reg);
			writeVal |= (mask | (mask >> 16));
			PSB_WVDC32(writeVal, reg);
			(void) PSB_RVDC32(reg);
			gma_power_end(dev_priv->dev);
		}
	}
}

void
psb_disable_pipestat(struct drm_psb_private *dev_priv, int pipe, u32 mask)
{
	if ((dev_priv->pipestat[pipe] & mask) != 0) {
		u32 reg = psb_pipestat(pipe);
		dev_priv->pipestat[pipe] &= ~mask;
		if (gma_power_begin(dev_priv->dev, false)) {
			u32 writeVal = PSB_RVDC32(reg);
			writeVal &= ~mask;
			PSB_WVDC32(writeVal, reg);
			(void) PSB_RVDC32(reg);
			gma_power_end(dev_priv->dev);
		}
	}
}

static void mid_enable_pipe_event(struct drm_psb_private *dev_priv, int pipe)
{
	if (gma_power_begin(dev_priv->dev, false)) {
		u32 pipe_event = mid_pipe_event(pipe);
		dev_priv->vdc_irq_mask |= pipe_event;
		PSB_WVDC32(~dev_priv->vdc_irq_mask, PSB_INT_MASK_R);
		PSB_WVDC32(dev_priv->vdc_irq_mask, PSB_INT_ENABLE_R);
		gma_power_end(dev_priv->dev);
	}
}

static void mid_disable_pipe_event(struct drm_psb_private *dev_priv, int pipe)
{
	if (dev_priv->pipestat[pipe] == 0) {
		if (gma_power_begin(dev_priv->dev, false)) {
			u32 pipe_event = mid_pipe_event(pipe);
			dev_priv->vdc_irq_mask &= ~pipe_event;
			PSB_WVDC32(~dev_priv->vdc_irq_mask, PSB_INT_MASK_R);
			PSB_WVDC32(dev_priv->vdc_irq_mask, PSB_INT_ENABLE_R);
			gma_power_end(dev_priv->dev);
		}
	}
}

/**
 * Display controller interrupt handler for pipe event.
 *
 */
static void mid_pipe_event_handler(struct drm_device *dev, int pipe)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;

	uint32_t pipe_stat_val = 0;
	uint32_t pipe_stat_reg = psb_pipestat(pipe);
	uint32_t pipe_enable = dev_priv->pipestat[pipe];
	uint32_t pipe_status = dev_priv->pipestat[pipe] >> 16;
	uint32_t pipe_clear;
	uint32_t i = 0;

	spin_lock(&dev_priv->irqmask_lock);

	pipe_stat_val = PSB_RVDC32(pipe_stat_reg);
	pipe_stat_val &= pipe_enable | pipe_status;
	pipe_stat_val &= pipe_stat_val >> 16;

	spin_unlock(&dev_priv->irqmask_lock);

	/* Clear the 2nd level interrupt status bits
	 * Sometimes the bits are very sticky so we repeat until they unstick */
	for (i = 0; i < 0xffff; i++) {
		PSB_WVDC32(PSB_RVDC32(pipe_stat_reg), pipe_stat_reg);
		pipe_clear = PSB_RVDC32(pipe_stat_reg) & pipe_status;

		if (pipe_clear == 0)
			break;
	}

	if (pipe_clear)
		dev_err(dev->dev,
		"%s, can't clear status bits for pipe %d, its value = 0x%x.\n",
		__func__, pipe, PSB_RVDC32(pipe_stat_reg));

	if (pipe_stat_val & PIPE_VBLANK_STATUS)
		drm_handle_vblank(dev, pipe);

	if (pipe_stat_val & PIPE_TE_STATUS)
		drm_handle_vblank(dev, pipe);
}

/*
 * Display controller interrupt handler.
 */
static void psb_vdc_interrupt(struct drm_device *dev, uint32_t vdc_stat)
{
	if (vdc_stat & _PSB_IRQ_ASLE)
		psb_intel_opregion_asle_intr(dev);

	if (vdc_stat & _PSB_VSYNC_PIPEA_FLAG)
		mid_pipe_event_handler(dev, 0);

	if (vdc_stat & _PSB_VSYNC_PIPEB_FLAG)
		mid_pipe_event_handler(dev, 1);
}

static void
psb_sgx_format_bits(const struct psb_irq_bit_name *bits, size_t bit_count,
		    u32 value, char *buf, size_t buf_size)
{
	size_t pos = 0;
	bool first = true;
	unsigned int i;

	if (!buf_size)
		return;

	buf[0] = '\0';

	for (i = 0; i < bit_count; i++) {
		if (!(value & bits[i].bit))
			continue;
		if (pos >= buf_size)
			break;
		pos += scnprintf(buf + pos, buf_size - pos, "%s%s",
				 first ? "" : ",", bits[i].name);
		first = false;
	}

	if (first)
		scnprintf(buf, buf_size, "none");
}

static void psb_sgx_store_snapshot(struct drm_psb_private *dev_priv,
				   struct psb_sgx_snapshot *snapshot)
{
	unsigned long flags;

	spin_lock_irqsave(&dev_priv->sgx_state_lock, flags);
	snapshot->sequence = ++dev_priv->sgx_state_sequence;
	dev_priv->sgx_state = *snapshot;
	spin_unlock_irqrestore(&dev_priv->sgx_state_lock, flags);
}

void psb_sgx_capture_state(struct drm_device *dev, const char *reason,
			   bool power_cycle)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_snapshot snapshot;
	struct psb_mmu_driver *mmu;
	struct psb_mmu_pd *default_pd;
	bool powered = true;

	memset(&snapshot, 0, sizeof(snapshot));
	snapshot.captured_jiffies = jiffies;
	snapshot.irq_count = dev_priv->sgx_irq.irq_count;
	snapshot.fault_count = dev_priv->sgx_irq.fault_count;
	snapshot.vdc_irq_mask = dev_priv->vdc_irq_mask;
	snapshot.gatt_start = dev_priv->gtt.gatt_start;
	snapshot.mmu_gatt_start = dev_priv->gtt.mmu_gatt_start;
	snapshot.gatt_free_offset = dev_priv->gatt_free_offset;

	if (reason && *reason)
		strscpy(snapshot.reason, reason, sizeof(snapshot.reason));
	else
		strscpy(snapshot.reason, "unspecified",
			sizeof(snapshot.reason));

	mmu = dev_priv->mmu;
	if (mmu) {
		snapshot.mmu_driver_bif_ctrl = mmu->bif_ctrl;
		snapshot.mmu_needs_tlbflush =
			atomic_read(&mmu->needs_tlbflush);
		default_pd = mmu->default_pd;
		if (default_pd)
			snapshot.mmu_default_pd_context =
				default_pd->hw_context;
		else
			snapshot.mmu_default_pd_context = U32_MAX;
	} else {
		snapshot.mmu_default_pd_context = U32_MAX;
	}

	if (power_cycle)
		powered = gma_power_begin(dev, false);

	if (!powered) {
		snapshot.capture_failed = 1;
		psb_sgx_store_snapshot(dev_priv, &snapshot);
		return;
	}

	snapshot.core_id = PSB_RSGX32(PSB_CR_CORE_ID);
	snapshot.core_revision = PSB_RSGX32(PSB_CR_CORE_REVISION);
	snapshot.clkgatectl = PSB_RSGX32(PSB_CR_CLKGATECTL);
	snapshot.sgx_soft_reset = PSB_RSGX32(PSB_CR_SOFT_RESET);
	snapshot.event_status = PSB_RSGX32(PSB_CR_EVENT_STATUS);
	snapshot.event_status2 = PSB_RSGX32(PSB_CR_EVENT_STATUS2);
	snapshot.event_host_enable = PSB_RSGX32(PSB_CR_EVENT_HOST_ENABLE);
	snapshot.event_host_enable2 = PSB_RSGX32(PSB_CR_EVENT_HOST_ENABLE2);
	snapshot.event_kicker = PSB_RSGX32(PSB_CR_EVENT_KICKER);
	snapshot.event_kick = PSB_RSGX32(PSB_CR_EVENT_KICK);
	snapshot.pds_exec_base = PSB_RSGX32(PSB_CR_PDS_EXEC_BASE);
	snapshot.use_code_base0 = PSB_RSGX32(PSB_CR_USE_CODE_BASE0);
	snapshot.use_code_base1 = PSB_RSGX32(PSB_CR_USE_CODE_BASE1);
	snapshot.bif_ctrl = PSB_RSGX32(PSB_CR_BIF_CTRL);
	snapshot.bif_int_stat = PSB_RSGX32(PSB_CR_BIF_INT_STAT);
	snapshot.bif_fault = PSB_RSGX32(PSB_CR_BIF_FAULT);
	snapshot.bif_dir_list_base0 = PSB_RSGX32(PSB_CR_BIF_DIR_LIST_BASE0);
	snapshot.bif_dir_list_base1 = PSB_RSGX32(PSB_CR_BIF_DIR_LIST_BASE1);
	snapshot.bif_bank0 = PSB_RSGX32(PSB_CR_BIF_BANK0);
	snapshot.bif_bank1 = PSB_RSGX32(PSB_CR_BIF_BANK1);
	snapshot.bif_twod_req_base = PSB_RSGX32(PSB_CR_BIF_TWOD_REQ_BASE);
	snapshot.bif_3d_req_base = PSB_RSGX32(PSB_CR_BIF_3D_REQ_BASE);
	snapshot.twod_socif = PSB_RSGX32(PSB_CR_2D_SOCIF);
	snapshot.twod_blit_status = PSB_RSGX32(PSB_CR_2D_BLIT_STATUS);

	if (power_cycle)
		gma_power_end(dev);

	psb_sgx_store_snapshot(dev_priv, &snapshot);
}

ssize_t psb_sgx_dump_state(struct drm_device *dev, char *buf, size_t buf_size)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_snapshot snapshot;
	unsigned long flags;

	spin_lock_irqsave(&dev_priv->sgx_state_lock, flags);
	snapshot = dev_priv->sgx_state;
	spin_unlock_irqrestore(&dev_priv->sgx_state_lock, flags);

	return scnprintf(buf, buf_size,
			 "reason=%s sequence=%u captured_jiffies=%lu capture_failed=%u irq_count=%u fault_count=%u vdc_irq_mask=%#x\n"
			 "gatt_start=%#08x mmu_gatt_start=%#08x gatt_free_offset=%#08x mmu_bif_ctrl=%#08x mmu_needs_tlbflush=%u mmu_default_pd_context=%u\n"
			 "core_id=%#08x core_revision=%#08x clkgate=%#08x soft_reset=%#08x\n"
			 "event_status=%#08x event_status2=%#08x event_enable=%#08x event_enable2=%#08x event_kicker=%#08x event_kick=%#08x\n"
			 "pds_exec_base=%#08x use_code_base0=%#08x use_code_base1=%#08x\n"
			 "bif_ctrl=%#08x bif_int_stat=%#08x bif_fault=%#08x dir0=%#08x dir1=%#08x bank0=%#08x bank1=%#08x 2d_base=%#08x 3d_base=%#08x\n"
			 "2d_socif=%#08x 2d_blit_status=%#08x\n",
			 snapshot.reason[0] ? snapshot.reason : "none",
			 snapshot.sequence, snapshot.captured_jiffies,
			 snapshot.capture_failed, snapshot.irq_count,
			 snapshot.fault_count, snapshot.vdc_irq_mask,
			 snapshot.gatt_start, snapshot.mmu_gatt_start,
			 snapshot.gatt_free_offset,
			 snapshot.mmu_driver_bif_ctrl,
			 snapshot.mmu_needs_tlbflush,
			 snapshot.mmu_default_pd_context,
			 snapshot.core_id, snapshot.core_revision,
			 snapshot.clkgatectl, snapshot.sgx_soft_reset,
			 snapshot.event_status, snapshot.event_status2,
			 snapshot.event_host_enable,
			 snapshot.event_host_enable2,
			 snapshot.event_kicker, snapshot.event_kick,
			 snapshot.pds_exec_base, snapshot.use_code_base0,
			 snapshot.use_code_base1, snapshot.bif_ctrl,
			 snapshot.bif_int_stat, snapshot.bif_fault,
			 snapshot.bif_dir_list_base0,
			 snapshot.bif_dir_list_base1, snapshot.bif_bank0,
			 snapshot.bif_bank1, snapshot.bif_twod_req_base,
			 snapshot.bif_3d_req_base, snapshot.twod_socif,
			 snapshot.twod_blit_status);
}

static void
psb_sgx_snapshot_irq(struct drm_psb_private *dev_priv, u32 stat_1, u32 stat_2,
		     u32 bif_int_stat, u32 bif_fault, u32 twod_blit_status)
{
	dev_priv->sgx_irq.irq_count++;
	dev_priv->sgx_irq.last_stat_1 = stat_1;
	dev_priv->sgx_irq.last_stat_2 = stat_2;
	dev_priv->sgx_irq.last_bif_int_stat = bif_int_stat;
	dev_priv->sgx_irq.last_bif_fault = bif_fault;
	dev_priv->sgx_irq.last_2d_blit_status = twod_blit_status;
	dev_priv->sgx_irq.last_irq_jiffies = jiffies;

	if (stat_2 & _PSB_CE2_BIF_REQUESTER_FAULT) {
		dev_priv->sgx_irq.fault_count++;
		dev_priv->sgx_irq.last_fault_jiffies = jiffies;
	}
}

static void
psb_sgx_log_irq_summary(struct drm_device *dev, const char *reason, u32 stat_1,
			u32 stat_2, u32 bif_int_stat, u32 bif_fault,
			u32 twod_blit_status, bool is_fault)
{
	char stat_1_desc[160];
	char stat_2_desc[64];
	char requester_desc[96];

	psb_sgx_format_bits(psb_sgx_stat_1_bits, ARRAY_SIZE(psb_sgx_stat_1_bits),
			    stat_1, stat_1_desc, sizeof(stat_1_desc));
	psb_sgx_format_bits(psb_sgx_stat_2_bits, ARRAY_SIZE(psb_sgx_stat_2_bits),
			    stat_2, stat_2_desc, sizeof(stat_2_desc));

	if (stat_2 & _PSB_CE2_BIF_REQUESTER_FAULT)
		psb_sgx_format_bits(psb_sgx_bif_requester_bits,
				    ARRAY_SIZE(psb_sgx_bif_requester_bits),
				    bif_int_stat, requester_desc,
				    sizeof(requester_desc));
	else
		scnprintf(requester_desc, sizeof(requester_desc), "none");

	if (is_fault) {
		dev_err(dev->dev,
			"SGX %s: stat1=%#08x [%s] stat2=%#08x [%s] bif_int=%#08x requesters=%s fault_addr=%#08x twod_blit=%#08x\n",
			reason, stat_1, stat_1_desc, stat_2, stat_2_desc,
			bif_int_stat, requester_desc, bif_fault,
			twod_blit_status);
		return;
	}

	dev_info_ratelimited(dev->dev,
			     "SGX %s: stat1=%#08x [%s] stat2=%#08x [%s] twod_blit=%#08x\n",
			     reason, stat_1, stat_1_desc, stat_2, stat_2_desc,
			     twod_blit_status);
}

static void psb_sgx_dump_fault_registers(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;

	dev_err(dev->dev,
		"SGX regs: core_id=%#08x core_rev=%#08x clkgate=%#08x soft_reset=%#08x event_enable=%#08x event_enable2=%#08x\n",
		PSB_RSGX32(PSB_CR_CORE_ID),
		PSB_RSGX32(PSB_CR_CORE_REVISION),
		PSB_RSGX32(PSB_CR_CLKGATECTL),
		PSB_RSGX32(PSB_CR_SOFT_RESET),
		PSB_RSGX32(PSB_CR_EVENT_HOST_ENABLE),
		PSB_RSGX32(PSB_CR_EVENT_HOST_ENABLE2));
	dev_err(dev->dev,
		"SGX BIF: ctrl=%#08x dir0=%#08x dir1=%#08x bank0=%#08x bank1=%#08x 2d_base=%#08x 3d_base=%#08x\n",
		PSB_RSGX32(PSB_CR_BIF_CTRL),
		PSB_RSGX32(PSB_CR_BIF_DIR_LIST_BASE0),
		PSB_RSGX32(PSB_CR_BIF_DIR_LIST_BASE1),
		PSB_RSGX32(PSB_CR_BIF_BANK0),
		PSB_RSGX32(PSB_CR_BIF_BANK1),
		PSB_RSGX32(PSB_CR_BIF_TWOD_REQ_BASE),
		PSB_RSGX32(PSB_CR_BIF_3D_REQ_BASE));
}

/*
 * SGX interrupt handler
 */
static void psb_sgx_interrupt(struct drm_device *dev, u32 stat_1, u32 stat_2)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	u32 bif_int_stat = 0;
	u32 bif_fault = 0;
	u32 twod_blit_status = 0;
	bool is_fault = false;

	/* Hand the live SGX event to pvrsrvkm (3D) before we clear it below. */
	if (pvr_irq_forward && gma_pvr_sgx_isr)
		gma_pvr_sgx_isr();

	if (stat_1 & _PSB_CE_TWOD_COMPLETE)
		twod_blit_status = PSB_RSGX32(PSB_CR_2D_BLIT_STATUS);

	if (stat_2 & _PSB_CE2_BIF_REQUESTER_FAULT) {
		bif_int_stat = PSB_RSGX32(PSB_CR_BIF_INT_STAT);
		bif_fault = PSB_RSGX32(PSB_CR_BIF_FAULT);
		is_fault = true;
	}

	psb_sgx_snapshot_irq(dev_priv, stat_1, stat_2, bif_int_stat, bif_fault,
			     twod_blit_status);
	psb_sgx_capture_state(dev, is_fault ? "irq-fault" : "irq", false);

	if (is_fault) {
		dev_err(dev->dev, "SGX MMU %s fault\n",
			(bif_int_stat & _PSB_CBI_STAT_PF_N_RW) ?
			"page" : "read/write protection");
		psb_sgx_log_irq_summary(dev, "fault", stat_1, stat_2,
					bif_int_stat, bif_fault,
					twod_blit_status, true);
		psb_sgx_dump_fault_registers(dev);
	} else if (stat_1 || stat_2) {
		psb_sgx_log_irq_summary(dev, "irq", stat_1, stat_2,
					bif_int_stat, bif_fault,
					twod_blit_status, false);
	}

	/* Clear bits */
	PSB_WSGX32(stat_1, PSB_CR_EVENT_HOST_CLEAR);
	PSB_WSGX32(stat_2, PSB_CR_EVENT_HOST_CLEAR2);
	PSB_RSGX32(PSB_CR_EVENT_HOST_CLEAR2);
}

irqreturn_t psb_irq_handler(int irq, void *arg)
{
	struct drm_device *dev = arg;
	struct drm_psb_private *dev_priv = dev->dev_private;
	uint32_t vdc_stat, dsp_int = 0, sgx_int = 0, hotplug_int = 0;
	u32 sgx_stat_1, sgx_stat_2;
	int handled = 0;

	spin_lock(&dev_priv->irqmask_lock);

	vdc_stat = PSB_RVDC32(PSB_INT_IDENTITY_R);

	if (vdc_stat & (_PSB_PIPE_EVENT_FLAG|_PSB_IRQ_ASLE))
		dsp_int = 1;

	/* FIXME: Handle Medfield
	if (vdc_stat & _MDFLD_DISP_ALL_IRQ_FLAG)
		dsp_int = 1;
	*/

	if (vdc_stat & _PSB_IRQ_SGX_FLAG)
		sgx_int = 1;
	if (vdc_stat & _PSB_IRQ_DISP_HOTSYNC)
		hotplug_int = 1;

	vdc_stat &= dev_priv->vdc_irq_mask;
	spin_unlock(&dev_priv->irqmask_lock);

	if (dsp_int && gma_power_is_on(dev)) {
		psb_vdc_interrupt(dev, vdc_stat);
		handled = 1;
	}

	if (sgx_int) {
		sgx_stat_1 = PSB_RSGX32(PSB_CR_EVENT_STATUS);
		sgx_stat_2 = PSB_RSGX32(PSB_CR_EVENT_STATUS2);
		psb_sgx_interrupt(dev, sgx_stat_1, sgx_stat_2);
		handled = 1;
	}

	/* Note: this bit has other meanings on some devices, so we will
	   need to address that later if it ever matters */
	if (hotplug_int && dev_priv->ops->hotplug) {
		handled = dev_priv->ops->hotplug(dev);
		REG_WRITE(PORT_HOTPLUG_STAT, REG_READ(PORT_HOTPLUG_STAT));
	}

	PSB_WVDC32(vdc_stat, PSB_INT_IDENTITY_R);
	(void) PSB_RVDC32(PSB_INT_IDENTITY_R);
	rmb();

	if (!handled)
		return IRQ_NONE;

	return IRQ_HANDLED;
}

void psb_irq_preinstall(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	unsigned long irqflags;

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	if (gma_power_is_on(dev)) {
		PSB_WVDC32(0xFFFFFFFF, PSB_HWSTAM);
		PSB_WVDC32(0x00000000, PSB_INT_MASK_R);
		PSB_WVDC32(0x00000000, PSB_INT_ENABLE_R);
		PSB_WSGX32(0x00000000, PSB_CR_EVENT_HOST_ENABLE);
		PSB_RSGX32(PSB_CR_EVENT_HOST_ENABLE);
	}
	if (dev->vblank[0].enabled)
		dev_priv->vdc_irq_mask |= _PSB_VSYNC_PIPEA_FLAG;
	if (dev->vblank[1].enabled)
		dev_priv->vdc_irq_mask |= _PSB_VSYNC_PIPEB_FLAG;

	/* FIXME: Handle Medfield irq mask
	if (dev->vblank[1].enabled)
		dev_priv->vdc_irq_mask |= _MDFLD_PIPEB_EVENT_FLAG;
	if (dev->vblank[2].enabled)
		dev_priv->vdc_irq_mask |= _MDFLD_PIPEC_EVENT_FLAG;
	*/

	/* Revisit this area - want per device masks ? */
	if (dev_priv->ops->hotplug)
		dev_priv->vdc_irq_mask |= _PSB_IRQ_DISP_HOTSYNC;
	dev_priv->vdc_irq_mask |= _PSB_IRQ_ASLE | _PSB_IRQ_SGX_FLAG;

	/* This register is safe even if display island is off */
	PSB_WVDC32(~dev_priv->vdc_irq_mask, PSB_INT_MASK_R);
	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
}

int psb_irq_postinstall(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	unsigned long irqflags;
	unsigned int i;

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	/* Enable lab-visible SGX events plus MMU requester faults. */
	PSB_WSGX32(_PSB_CE2_BIF_REQUESTER_FAULT, PSB_CR_EVENT_HOST_ENABLE2);
	PSB_WSGX32(PSB_SGX_LAB_EVENT_MASK, PSB_CR_EVENT_HOST_ENABLE);
	PSB_RSGX32(PSB_CR_EVENT_HOST_ENABLE); /* Post */

	/* This register is safe even if display island is off */
	PSB_WVDC32(dev_priv->vdc_irq_mask, PSB_INT_ENABLE_R);
	PSB_WVDC32(0xFFFFFFFF, PSB_HWSTAM);

	for (i = 0; i < dev->num_crtcs; ++i) {
		if (dev->vblank[i].enabled)
			psb_enable_pipestat(dev_priv, i, PIPE_VBLANK_INTERRUPT_ENABLE);
		else
			psb_disable_pipestat(dev_priv, i, PIPE_VBLANK_INTERRUPT_ENABLE);
	}

	if (dev_priv->ops->hotplug_enable)
		dev_priv->ops->hotplug_enable(dev, true);

	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
	return 0;
}

void psb_irq_uninstall(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	unsigned long irqflags;
	unsigned int i;

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	if (dev_priv->ops->hotplug_enable)
		dev_priv->ops->hotplug_enable(dev, false);

	PSB_WVDC32(0xFFFFFFFF, PSB_HWSTAM);

	for (i = 0; i < dev->num_crtcs; ++i) {
		if (dev->vblank[i].enabled)
			psb_disable_pipestat(dev_priv, i, PIPE_VBLANK_INTERRUPT_ENABLE);
	}

	dev_priv->vdc_irq_mask &= _PSB_IRQ_SGX_FLAG |
				  _PSB_IRQ_MSVDX_FLAG |
				  _LNC_IRQ_TOPAZ_FLAG;

	/* These two registers are safe even if display island is off */
	PSB_WVDC32(~dev_priv->vdc_irq_mask, PSB_INT_MASK_R);
	PSB_WVDC32(dev_priv->vdc_irq_mask, PSB_INT_ENABLE_R);

	wmb();

	/* This register is safe even if display island is off */
	PSB_WVDC32(PSB_RVDC32(PSB_INT_IDENTITY_R), PSB_INT_IDENTITY_R);
	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
}

void psb_irq_turn_on_dpst(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *) dev->dev_private;
	u32 hist_reg;
	u32 pwm_reg;

	if (gma_power_begin(dev, false)) {
		PSB_WVDC32(1 << 31, HISTOGRAM_LOGIC_CONTROL);
		hist_reg = PSB_RVDC32(HISTOGRAM_LOGIC_CONTROL);
		PSB_WVDC32(1 << 31, HISTOGRAM_INT_CONTROL);
		hist_reg = PSB_RVDC32(HISTOGRAM_INT_CONTROL);

		PSB_WVDC32(0x80010100, PWM_CONTROL_LOGIC);
		pwm_reg = PSB_RVDC32(PWM_CONTROL_LOGIC);
		PSB_WVDC32(pwm_reg | PWM_PHASEIN_ENABLE
						| PWM_PHASEIN_INT_ENABLE,
							   PWM_CONTROL_LOGIC);
		pwm_reg = PSB_RVDC32(PWM_CONTROL_LOGIC);

		psb_enable_pipestat(dev_priv, 0, PIPE_DPST_EVENT_ENABLE);

		hist_reg = PSB_RVDC32(HISTOGRAM_INT_CONTROL);
		PSB_WVDC32(hist_reg | HISTOGRAM_INT_CTRL_CLEAR,
							HISTOGRAM_INT_CONTROL);
		pwm_reg = PSB_RVDC32(PWM_CONTROL_LOGIC);
		PSB_WVDC32(pwm_reg | 0x80010100 | PWM_PHASEIN_ENABLE,
							PWM_CONTROL_LOGIC);

		gma_power_end(dev);
	}
}

int psb_irq_enable_dpst(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *) dev->dev_private;
	unsigned long irqflags;

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	/* enable DPST */
	mid_enable_pipe_event(dev_priv, 0);
	psb_irq_turn_on_dpst(dev);

	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
	return 0;
}

void psb_irq_turn_off_dpst(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	u32 hist_reg;
	u32 pwm_reg;

	if (gma_power_begin(dev, false)) {
		PSB_WVDC32(0x00000000, HISTOGRAM_INT_CONTROL);
		hist_reg = PSB_RVDC32(HISTOGRAM_INT_CONTROL);

		psb_disable_pipestat(dev_priv, 0, PIPE_DPST_EVENT_ENABLE);

		pwm_reg = PSB_RVDC32(PWM_CONTROL_LOGIC);
		PSB_WVDC32(pwm_reg & ~PWM_PHASEIN_INT_ENABLE,
							PWM_CONTROL_LOGIC);
		pwm_reg = PSB_RVDC32(PWM_CONTROL_LOGIC);

		gma_power_end(dev);
	}
}

int psb_irq_disable_dpst(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	unsigned long irqflags;

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	mid_disable_pipe_event(dev_priv, 0);
	psb_irq_turn_off_dpst(dev);

	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);

	return 0;
}

/*
 * It is used to enable VBLANK interrupt
 */
int psb_enable_vblank(struct drm_device *dev, unsigned int pipe)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	unsigned long irqflags;
	uint32_t reg_val = 0;
	uint32_t pipeconf_reg = mid_pipeconf(pipe);

	/* Medfield is different - we should perhaps extract out vblank
	   and blacklight etc ops */
	if (IS_MFLD(dev))
		return mdfld_enable_te(dev, pipe);

	if (gma_power_begin(dev, false)) {
		reg_val = REG_READ(pipeconf_reg);
		gma_power_end(dev);
	}

	if (!(reg_val & PIPEACONF_ENABLE))
		return -EINVAL;

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	if (pipe == 0)
		dev_priv->vdc_irq_mask |= _PSB_VSYNC_PIPEA_FLAG;
	else if (pipe == 1)
		dev_priv->vdc_irq_mask |= _PSB_VSYNC_PIPEB_FLAG;

	PSB_WVDC32(~dev_priv->vdc_irq_mask, PSB_INT_MASK_R);
	PSB_WVDC32(dev_priv->vdc_irq_mask, PSB_INT_ENABLE_R);
	psb_enable_pipestat(dev_priv, pipe, PIPE_VBLANK_INTERRUPT_ENABLE);

	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);

	return 0;
}

/*
 * It is used to disable VBLANK interrupt
 */
void psb_disable_vblank(struct drm_device *dev, unsigned int pipe)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	unsigned long irqflags;

	if (IS_MFLD(dev))
		mdfld_disable_te(dev, pipe);
	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	if (pipe == 0)
		dev_priv->vdc_irq_mask &= ~_PSB_VSYNC_PIPEA_FLAG;
	else if (pipe == 1)
		dev_priv->vdc_irq_mask &= ~_PSB_VSYNC_PIPEB_FLAG;

	PSB_WVDC32(~dev_priv->vdc_irq_mask, PSB_INT_MASK_R);
	PSB_WVDC32(dev_priv->vdc_irq_mask, PSB_INT_ENABLE_R);
	psb_disable_pipestat(dev_priv, pipe, PIPE_VBLANK_INTERRUPT_ENABLE);

	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
}

/*
 * It is used to enable TE interrupt
 */
int mdfld_enable_te(struct drm_device *dev, int pipe)
{
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *) dev->dev_private;
	unsigned long irqflags;
	uint32_t reg_val = 0;
	uint32_t pipeconf_reg = mid_pipeconf(pipe);

	if (gma_power_begin(dev, false)) {
		reg_val = REG_READ(pipeconf_reg);
		gma_power_end(dev);
	}

	if (!(reg_val & PIPEACONF_ENABLE))
		return -EINVAL;

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	mid_enable_pipe_event(dev_priv, pipe);
	psb_enable_pipestat(dev_priv, pipe, PIPE_TE_ENABLE);

	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);

	return 0;
}

/*
 * It is used to disable TE interrupt
 */
void mdfld_disable_te(struct drm_device *dev, int pipe)
{
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *) dev->dev_private;
	unsigned long irqflags;

	if (!dev_priv->dsr_enable)
		return;

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	mid_disable_pipe_event(dev_priv, pipe);
	psb_disable_pipestat(dev_priv, pipe, PIPE_TE_ENABLE);

	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
}

/* Called from drm generic code, passed a 'crtc', which
 * we use as a pipe index
 */
u32 psb_get_vblank_counter(struct drm_device *dev, unsigned int pipe)
{
	uint32_t high_frame = PIPEAFRAMEHIGH;
	uint32_t low_frame = PIPEAFRAMEPIXEL;
	uint32_t pipeconf_reg = PIPEACONF;
	uint32_t reg_val = 0;
	uint32_t high1 = 0, high2 = 0, low = 0, count = 0;

	switch (pipe) {
	case 0:
		break;
	case 1:
		high_frame = PIPEBFRAMEHIGH;
		low_frame = PIPEBFRAMEPIXEL;
		pipeconf_reg = PIPEBCONF;
		break;
	case 2:
		high_frame = PIPECFRAMEHIGH;
		low_frame = PIPECFRAMEPIXEL;
		pipeconf_reg = PIPECCONF;
		break;
	default:
		dev_err(dev->dev, "%s, invalid pipe.\n", __func__);
		return 0;
	}

	if (!gma_power_begin(dev, false))
		return 0;

	reg_val = REG_READ(pipeconf_reg);

	if (!(reg_val & PIPEACONF_ENABLE)) {
		dev_err(dev->dev, "trying to get vblank count for disabled pipe %u\n",
								pipe);
		goto psb_get_vblank_counter_exit;
	}

	/*
	 * High & low register fields aren't synchronized, so make sure
	 * we get a low value that's stable across two reads of the high
	 * register.
	 */
	do {
		high1 = ((REG_READ(high_frame) & PIPE_FRAME_HIGH_MASK) >>
			 PIPE_FRAME_HIGH_SHIFT);
		low =  ((REG_READ(low_frame) & PIPE_FRAME_LOW_MASK) >>
			PIPE_FRAME_LOW_SHIFT);
		high2 = ((REG_READ(high_frame) & PIPE_FRAME_HIGH_MASK) >>
			 PIPE_FRAME_HIGH_SHIFT);
	} while (high1 != high2);

	count = (high1 << 8) | low;

psb_get_vblank_counter_exit:

	gma_power_end(dev);

	return count;
}
