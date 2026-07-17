// SPDX-License-Identifier: GPL-2.0-only
/**************************************************************************
 * Copyright (c) 2007-2011, Intel Corporation.
 * All Rights Reserved.
 * Copyright (c) 2008, Tungsten Graphics, Inc. Cedar Park, TX., USA.
 * All Rights Reserved.
 *
 **************************************************************************/

#include <linux/cpu.h>
#include <linux/device.h>
#include <linux/highmem.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/pm_runtime.h>
#include <linux/spinlock.h>

#include <asm/set_memory.h>

#include <acpi/video.h>

#include <drm/drm.h>
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_irq.h>
#include <drm/drm_pci.h>
#include <drm/drm_pciids.h>
#include <drm/drm_vblank.h>

#include "framebuffer.h"
#include "intel_bios.h"
#include "mid_bios.h"
#include "power.h"
#include "psb_drm_lab.h"
#include "gma_blt_drm.h"
#include "pvr_gma_bridge.h"
#include "psb_drv.h"
#include "psb_intel_reg.h"
#include "psb_reg.h"

static struct drm_driver driver;
static int psb_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent);

static const char *poulsbo_mmu_debug_op_name(u32 op)
{
	switch (op) {
	case PSB_MMU_DEBUG_OP_FLUSH:
		return "flush";
	case PSB_MMU_DEBUG_OP_SET_PD_CONTEXT:
		return "set_pd_context";
	case PSB_MMU_DEBUG_OP_INSERT_PFN_SEQUENCE:
		return "insert_pfn_sequence";
	case PSB_MMU_DEBUG_OP_INSERT_PAGES:
		return "insert_pages";
	case PSB_MMU_DEBUG_OP_REMOVE_PFN_SEQUENCE:
		return "remove_pfn_sequence";
	case PSB_MMU_DEBUG_OP_REMOVE_PAGES:
		return "remove_pages";
	case PSB_MMU_DEBUG_OP_NONE:
	default:
		return "none";
	}
}

struct psb_sgx_lab_object {
	struct list_head head;
	struct gtt_range *gt;
	u32 object_id;
	u32 role;
	u32 object_class;
	u32 gpu_use;
	u32 expected_context;
	unsigned long created_jiffies;
	unsigned long last_pf_bind_jiffies;
	unsigned long last_pf_unbind_jiffies;
	unsigned long last_role_jiffies;
	unsigned long last_class_jiffies;
	unsigned long last_gpu_use_jiffies;
	unsigned long last_context_jiffies;
};

enum psb_sgx_lab_role {
	PSB_SGX_LAB_ROLE_GENERIC = 0,
	PSB_SGX_LAB_ROLE_PDS,
	PSB_SGX_LAB_ROLE_USE,
	PSB_SGX_LAB_ROLE_CONTROL,
	PSB_SGX_LAB_ROLE_TARGET,
	PSB_SGX_LAB_ROLE_STATUS,
};

enum psb_sgx_lab_context {
	PSB_SGX_LAB_CONTEXT_AUTO = 0,
	PSB_SGX_LAB_CONTEXT_DEFAULT,
	PSB_SGX_LAB_CONTEXT_PF,
};

enum psb_sgx_lab_object_class {
	PSB_SGX_LAB_CLASS_GENERIC = 0,
	PSB_SGX_LAB_CLASS_PACKET,
	PSB_SGX_LAB_CLASS_CODE,
	PSB_SGX_LAB_CLASS_STATE,
	PSB_SGX_LAB_CLASS_SURFACE,
};

enum psb_sgx_lab_gpu_use {
	PSB_SGX_LAB_GPU_USE_GENERIC = 0,
	PSB_SGX_LAB_GPU_USE_CONTROL_CCB,
	PSB_SGX_LAB_GPU_USE_PDS_CODE,
	PSB_SGX_LAB_GPU_USE_USE_CODE,
	PSB_SGX_LAB_GPU_USE_RENDER_TARGET,
	PSB_SGX_LAB_GPU_USE_STATUS_BLOCK,
};

enum psb_sgx_lab_cmd_type {
	PSB_SGX_LAB_CMD_INVALID = 0,
	PSB_SGX_LAB_CMD_TA,
	PSB_SGX_LAB_CMD_TRANSFER,
	PSB_SGX_LAB_CMD_2D,
	PSB_SGX_LAB_CMD_POWER,
	PSB_SGX_LAB_CMD_CLEANUP,
	PSB_SGX_LAB_CMD_GETMISCINFO,
	PSB_SGX_LAB_CMD_PROCESS_QUEUES,
};

#define PSB_SGX_LAB_CMDTA_CTRLFLAGS_READY 0x00000001
#define PSB_SGX_LAB_MAX_STATUS_VALS 16
#define PSB_SGX_LAB_MAX_DST_SYNC_OBJECTS 1
#define PSB_SGX_LAB_MAX_TA_SRC_SYNCS 1
#define PSB_SGX_LAB_MAX_TA_DST_SYNCS PSB_SGX_LAB_MAX_DST_SYNC_OBJECTS
#define PSB_SGX_LAB_MAX_3D_SRC_SYNCS 1
#define PSB_SGX_LAB_SYNC_FLAG_READY 0x00000001
#define PSB_SGX_LAB_SYNC_FLAG_WRAPPED 0x00000002
#define PSB_SGX_LAB_QUEUE_HEADER_DWORDS 8
#define PSB_SGX_LAB_SYNC_DWORDS 7
#define PSB_SGX_LAB_OWNER_DWORDS 6
#define PSB_SGX_LAB_REPORT_DWORDS 18
#define PSB_SGX_LAB_MANIFEST_DWORDS 27
#define PSB_SGX_LAB_DESCRIPTOR_DWORDS 32
#define PSB_SGX_LAB_PLAN_DWORDS 41
#define PSB_SGX_LAB_ISSUE_PLAN_DWORDS 38
#define PSB_SGX_LAB_DEPENDENCY_DWORDS 4
#define PSB_SGX_LAB_DEPENDENCY_SLOTS 4
#define PSB_SGX_LAB_DEPENDENCY_FLAG_READY 0x00000001
#define PSB_SGX_LAB_DEPENDENCY_FLAG_SOURCE_BOUND 0x00000002
#define PSB_SGX_LAB_DEPENDENCY_FLAG_SATISFIED 0x00000004
#define PSB_SGX_LAB_DEPENDENCY_FLAG_GATED 0x00000008
#define PSB_SGX_LAB_DEPENDENCY_FLAG_SOURCE_MISSING 0x00000010
#define PSB_SGX_LAB_SYNC_EPOCH_OFFSET 20
#define PSB_SGX_LAB_OWNER_EPOCH_OFFSET 12
#define PSB_SGX_LAB_SYNC_GLOBAL_ISSUE_TICKET_OFFSET 24
#define PSB_SGX_LAB_OWNER_GLOBAL_COMPLETE_TICKET_OFFSET 16
#define PSB_SGX_LAB_OWNER_GLOBAL_RUNNING_TICKET_OFFSET 20
#define PSB_SGX_LAB_REPORT_STAGE_OFFSET 0
#define PSB_SGX_LAB_REPORT_STAGE_COUNT_OFFSET 4
#define PSB_SGX_LAB_REPORT_PREPARE_COUNT_OFFSET 8
#define PSB_SGX_LAB_REPORT_ARM_COUNT_OFFSET 12
#define PSB_SGX_LAB_REPORT_EMIT_COUNT_OFFSET 16
#define PSB_SGX_LAB_REPORT_LAUNCH_COUNT_OFFSET 20
#define PSB_SGX_LAB_REPORT_DISPATCH_COUNT_OFFSET 24
#define PSB_SGX_LAB_REPORT_RETIRED_COUNT_OFFSET 28
#define PSB_SGX_LAB_REPORT_OWNER_STATE_OFFSET 32
#define PSB_SGX_LAB_REPORT_QUEUE_PACKET_COUNT_OFFSET 36
#define PSB_SGX_LAB_REPORT_COMPLETED_PACKETS_OFFSET 40
#define PSB_SGX_LAB_REPORT_DEPENDENCY_ACTIVE_COUNT_OFFSET 44
#define PSB_SGX_LAB_REPORT_QUEUE_EPOCH_OFFSET 48
#define PSB_SGX_LAB_REPORT_GLOBAL_ISSUE_TICKET_OFFSET 52
#define PSB_SGX_LAB_REPORT_GLOBAL_RUNNING_TICKET_OFFSET 56
#define PSB_SGX_LAB_REPORT_GLOBAL_COMPLETE_TICKET_OFFSET 60
#define PSB_SGX_LAB_REPORT_FLAGS_OFFSET 64
#define PSB_SGX_LAB_REPORT_CONTEXT_OFFSET 68
#define PSB_SGX_LAB_MANIFEST_STAGE_OFFSET 0
#define PSB_SGX_LAB_MANIFEST_STAGE_COUNT_OFFSET 4
#define PSB_SGX_LAB_MANIFEST_CONTEXT_OFFSET 8
#define PSB_SGX_LAB_MANIFEST_CMD_TYPE_OFFSET 12
#define PSB_SGX_LAB_MANIFEST_OWNER_STATE_OFFSET 16
#define PSB_SGX_LAB_MANIFEST_CTRL_FLAGS_OFFSET 20
#define PSB_SGX_LAB_MANIFEST_QUEUE_BASE_OFFSET 24
#define PSB_SGX_LAB_MANIFEST_QUEUE_SIZE_OFFSET 28
#define PSB_SGX_LAB_MANIFEST_QUEUE_PACKET_SIZE_OFFSET 32
#define PSB_SGX_LAB_MANIFEST_QUEUE_PACKET_COUNT_OFFSET 36
#define PSB_SGX_LAB_MANIFEST_QUEUE_EPOCH_OFFSET 40
#define PSB_SGX_LAB_MANIFEST_CCB_OFFSET 44
#define PSB_SGX_LAB_MANIFEST_CCB_WRITE_OFFSET 48
#define PSB_SGX_LAB_MANIFEST_CCB_READ_OFFSET 52
#define PSB_SGX_LAB_MANIFEST_DEPENDENCY_ACTIVE_COUNT_OFFSET 56
#define PSB_SGX_LAB_MANIFEST_GLOBAL_ISSUE_TICKET_OFFSET 60
#define PSB_SGX_LAB_MANIFEST_GLOBAL_RUNNING_TICKET_OFFSET 64
#define PSB_SGX_LAB_MANIFEST_GLOBAL_COMPLETE_TICKET_OFFSET 68
#define PSB_SGX_LAB_MANIFEST_PDS_GPU_VA32_OFFSET 72
#define PSB_SGX_LAB_MANIFEST_USE_GPU_VA32_OFFSET 76
#define PSB_SGX_LAB_MANIFEST_TARGET_GPU_VA32_OFFSET 80
#define PSB_SGX_LAB_MANIFEST_STATUS_GPU_VA32_OFFSET 84
#define PSB_SGX_LAB_MANIFEST_SYNC_COMPLETE_SEQ_OFFSET 88
#define PSB_SGX_LAB_MANIFEST_SYNC_PENDING_SEQ_OFFSET 92
#define PSB_SGX_LAB_MANIFEST_SYNC_FLAGS_OFFSET 96
#define PSB_SGX_LAB_MANIFEST_OWNER_ISSUE_COOKIE_OFFSET 100
#define PSB_SGX_LAB_MANIFEST_OWNER_COMPLETE_COOKIE_OFFSET 104
#define PSB_SGX_LAB_DESCRIPTOR_STAGE_OFFSET 0
#define PSB_SGX_LAB_DESCRIPTOR_STAGE_COUNT_OFFSET 4
#define PSB_SGX_LAB_DESCRIPTOR_CONTEXT_OFFSET 8
#define PSB_SGX_LAB_DESCRIPTOR_CMD_TYPE_OFFSET 12
#define PSB_SGX_LAB_DESCRIPTOR_OWNER_STATE_OFFSET 16
#define PSB_SGX_LAB_DESCRIPTOR_ISSUED_PACKETS_OFFSET 20
#define PSB_SGX_LAB_DESCRIPTOR_COMPLETED_PACKETS_OFFSET 24
#define PSB_SGX_LAB_DESCRIPTOR_DEPENDENCY_ACTIVE_COUNT_OFFSET 28
#define PSB_SGX_LAB_DESCRIPTOR_QUEUE_PACKET_COUNT_OFFSET 32
#define PSB_SGX_LAB_DESCRIPTOR_QUEUE_EPOCH_OFFSET 36
#define PSB_SGX_LAB_DESCRIPTOR_CTRL_FLAGS_OFFSET 40
#define PSB_SGX_LAB_DESCRIPTOR_QUEUE_BASE_OFFSET 44
#define PSB_SGX_LAB_DESCRIPTOR_QUEUE_SIZE_OFFSET 48
#define PSB_SGX_LAB_DESCRIPTOR_QUEUE_PACKET_SIZE_OFFSET 52
#define PSB_SGX_LAB_DESCRIPTOR_CCB_OFFSET 56
#define PSB_SGX_LAB_DESCRIPTOR_SYNC_OFFSET 60
#define PSB_SGX_LAB_DESCRIPTOR_OWNER_OFFSET 64
#define PSB_SGX_LAB_DESCRIPTOR_REPORT_OFFSET 68
#define PSB_SGX_LAB_DESCRIPTOR_MANIFEST_OFFSET 72
#define PSB_SGX_LAB_DESCRIPTOR_GLOBAL_ISSUE_TICKET_OFFSET 76
#define PSB_SGX_LAB_DESCRIPTOR_GLOBAL_RUNNING_TICKET_OFFSET 80
#define PSB_SGX_LAB_DESCRIPTOR_GLOBAL_COMPLETE_TICKET_OFFSET 84
#define PSB_SGX_LAB_DESCRIPTOR_REPORT_FLAGS_OFFSET 88
#define PSB_SGX_LAB_DESCRIPTOR_STATUS_TA_OFFSET 92
#define PSB_SGX_LAB_DESCRIPTOR_STATUS_3D_OFFSET 96
#define PSB_SGX_LAB_DESCRIPTOR_STATUS_STRIDE_OFFSET 100
#define PSB_SGX_LAB_DESCRIPTOR_STATUS_VALUE_SEED_OFFSET 104
#define PSB_SGX_LAB_DESCRIPTOR_CONTROL_GPU_VA32_OFFSET 108
#define PSB_SGX_LAB_DESCRIPTOR_PDS_GPU_VA32_OFFSET 112
#define PSB_SGX_LAB_DESCRIPTOR_USE_GPU_VA32_OFFSET 116
#define PSB_SGX_LAB_DESCRIPTOR_TARGET_GPU_VA32_OFFSET 120
#define PSB_SGX_LAB_DESCRIPTOR_STATUS_GPU_VA32_OFFSET 124
#define PSB_SGX_LAB_PLAN_STAGE_OFFSET 0
#define PSB_SGX_LAB_PLAN_STAGE_COUNT_OFFSET 4
#define PSB_SGX_LAB_PLAN_SOURCE_OFFSET 8
#define PSB_SGX_LAB_PLAN_CONTEXT_OFFSET 12
#define PSB_SGX_LAB_PLAN_CMD_TYPE_OFFSET 16
#define PSB_SGX_LAB_PLAN_OWNER_STATE_OFFSET 20
#define PSB_SGX_LAB_PLAN_ISSUED_PACKETS_OFFSET 24
#define PSB_SGX_LAB_PLAN_COMPLETED_PACKETS_OFFSET 28
#define PSB_SGX_LAB_PLAN_DEPENDENCY_ACTIVE_COUNT_OFFSET 32
#define PSB_SGX_LAB_PLAN_REFILLED_FROM_COMPLETE_OFFSET 36
#define PSB_SGX_LAB_PLAN_QUEUE_PACKET_COUNT_OFFSET 40
#define PSB_SGX_LAB_PLAN_QUEUE_EPOCH_OFFSET 44
#define PSB_SGX_LAB_PLAN_QUEUE_BASE_OFFSET 48
#define PSB_SGX_LAB_PLAN_QUEUE_SIZE_OFFSET 52
#define PSB_SGX_LAB_PLAN_QUEUE_PACKET_SIZE_OFFSET 56
#define PSB_SGX_LAB_PLAN_CCB_OFFSET 60
#define PSB_SGX_LAB_PLAN_CCB_WRITE_OFFSET 64
#define PSB_SGX_LAB_PLAN_CCB_READ_OFFSET 68
#define PSB_SGX_LAB_PLAN_SYNC_OFFSET 72
#define PSB_SGX_LAB_PLAN_SYNC_COMPLETE_SEQ_OFFSET 76
#define PSB_SGX_LAB_PLAN_SYNC_PENDING_SEQ_OFFSET 80
#define PSB_SGX_LAB_PLAN_SYNC_FLAGS_OFFSET 84
#define PSB_SGX_LAB_PLAN_OWNER_OFFSET 88
#define PSB_SGX_LAB_PLAN_OWNER_ISSUE_COOKIE_OFFSET 92
#define PSB_SGX_LAB_PLAN_OWNER_COMPLETE_COOKIE_OFFSET 96
#define PSB_SGX_LAB_PLAN_GLOBAL_ISSUE_TICKET_OFFSET 100
#define PSB_SGX_LAB_PLAN_GLOBAL_RUNNING_TICKET_OFFSET 104
#define PSB_SGX_LAB_PLAN_GLOBAL_COMPLETE_TICKET_OFFSET 108
#define PSB_SGX_LAB_PLAN_REPORT_OFFSET 112
#define PSB_SGX_LAB_PLAN_MANIFEST_OFFSET 116
#define PSB_SGX_LAB_PLAN_DESCRIPTOR_OFFSET 120
#define PSB_SGX_LAB_PLAN_CTRL_FLAGS_OFFSET 124
#define PSB_SGX_LAB_PLAN_CCB_SERVICE_ADDRESS_OFFSET 128
#define PSB_SGX_LAB_PLAN_CCB_CACHE_CONTROL_OFFSET 132
#define PSB_SGX_LAB_PLAN_CCB_DATA0_OFFSET 136
#define PSB_SGX_LAB_PLAN_CCB_DATA1_OFFSET 140
#define PSB_SGX_LAB_PLAN_CONTROL_GPU_VA32_OFFSET 144
#define PSB_SGX_LAB_PLAN_PDS_GPU_VA32_OFFSET 148
#define PSB_SGX_LAB_PLAN_USE_GPU_VA32_OFFSET 152
#define PSB_SGX_LAB_PLAN_TARGET_GPU_VA32_OFFSET 156
#define PSB_SGX_LAB_PLAN_STATUS_GPU_VA32_OFFSET 160
#define PSB_SGX_LAB_LIVE_DWORDS 54
#define PSB_SGX_LAB_RESULT_DWORDS 35
#define PSB_SGX_LAB_RESULT_STAGE_OFFSET 0
#define PSB_SGX_LAB_RESULT_STAGE_COUNT_OFFSET 4
#define PSB_SGX_LAB_RESULT_SOURCE_OFFSET 8
#define PSB_SGX_LAB_RESULT_CONTEXT_OFFSET 12
#define PSB_SGX_LAB_RESULT_CMD_TYPE_OFFSET 16
#define PSB_SGX_LAB_RESULT_OWNER_STATE_OFFSET 20
#define PSB_SGX_LAB_RESULT_ISSUED_PACKETS_OFFSET 24
#define PSB_SGX_LAB_RESULT_COMPLETED_PACKETS_OFFSET 28
#define PSB_SGX_LAB_RESULT_DEPENDENCY_ACTIVE_COUNT_OFFSET 32
#define PSB_SGX_LAB_RESULT_QUEUE_PACKET_COUNT_OFFSET 36
#define PSB_SGX_LAB_RESULT_QUEUE_EPOCH_OFFSET 40
#define PSB_SGX_LAB_RESULT_SYNC_COMPLETE_SEQ_OFFSET 44
#define PSB_SGX_LAB_RESULT_SYNC_PENDING_SEQ_OFFSET 48
#define PSB_SGX_LAB_RESULT_SYNC_FLAGS_OFFSET 52
#define PSB_SGX_LAB_RESULT_OWNER_ISSUE_COOKIE_OFFSET 56
#define PSB_SGX_LAB_RESULT_OWNER_COMPLETE_COOKIE_OFFSET 60
#define PSB_SGX_LAB_RESULT_GLOBAL_ISSUE_TICKET_OFFSET 64
#define PSB_SGX_LAB_RESULT_GLOBAL_RUNNING_TICKET_OFFSET 68
#define PSB_SGX_LAB_RESULT_GLOBAL_COMPLETE_TICKET_OFFSET 72
#define PSB_SGX_LAB_RESULT_CTRL_FLAGS_OFFSET 76
#define PSB_SGX_LAB_RESULT_NUM_TA_STATUS_VALS_OFFSET 80
#define PSB_SGX_LAB_RESULT_NUM_3D_STATUS_VALS_OFFSET 84
#define PSB_SGX_LAB_RESULT_TA_DEPENDENCY_OFFSET 88
#define PSB_SGX_LAB_RESULT_STATUS_TA_OFFSET_OFFSET 92
#define PSB_SGX_LAB_RESULT_STATUS_3D_OFFSET_OFFSET 96
#define PSB_SGX_LAB_RESULT_STATUS_STRIDE_OFFSET 100
#define PSB_SGX_LAB_RESULT_STATUS_VALUE_SEED_OFFSET 104
#define PSB_SGX_LAB_RESULT_CONTROL_OBJECT_ID_OFFSET 108
#define PSB_SGX_LAB_RESULT_STATUS_OBJECT_ID_OFFSET 112
#define PSB_SGX_LAB_RESULT_CONTROL_GPU_VA32_OFFSET 116
#define PSB_SGX_LAB_RESULT_STATUS_GPU_VA32_OFFSET 120
#define PSB_SGX_LAB_RESULT_REPORT_OFFSET 124
#define PSB_SGX_LAB_RESULT_MANIFEST_OFFSET 128
#define PSB_SGX_LAB_RESULT_DESCRIPTOR_OFFSET 132
#define PSB_SGX_LAB_RESULT_PLAN_OFFSET 136
#define PSB_SGX_LAB_CONTROL_QUEUE_HEADER_BYTES 32
#define PSB_SGX_LAB_CCB_RECORD_DWORDS 8
#define PSB_SGX_LAB_CCB_RECORD_BYTES \
	(PSB_SGX_LAB_CCB_RECORD_DWORDS * sizeof(u32))
#define PSB_SGX_LAB_CCB_PDS_GPU_VA32_OFFSET 16
#define PSB_SGX_LAB_CCB_USE_GPU_VA32_OFFSET 20
#define PSB_SGX_LAB_CCB_TARGET_GPU_VA32_OFFSET 24
#define PSB_SGX_LAB_CCB_STATUS_GPU_VA32_OFFSET 28

enum psb_sgx_lab_owner_state {
	PSB_SGX_LAB_OWNER_NONE = 0,
	PSB_SGX_LAB_OWNER_QUEUED,
	PSB_SGX_LAB_OWNER_RUNNING,
	PSB_SGX_LAB_OWNER_COMPLETE,
};

enum psb_sgx_lab_report_stage {
	PSB_SGX_LAB_REPORT_STAGE_NONE = 0,
	PSB_SGX_LAB_REPORT_STAGE_PREPARED,
	PSB_SGX_LAB_REPORT_STAGE_ARMED,
	PSB_SGX_LAB_REPORT_STAGE_EMITTED,
	PSB_SGX_LAB_REPORT_STAGE_LAUNCHED,
	PSB_SGX_LAB_REPORT_STAGE_DISPATCHED,
	PSB_SGX_LAB_REPORT_STAGE_RETIRED,
};

enum psb_sgx_lab_exec_source {
	PSB_SGX_LAB_EXEC_SOURCE_LIVE = 0,
	PSB_SGX_LAB_EXEC_SOURCE_LAUNCH,
	PSB_SGX_LAB_EXEC_SOURCE_DISPATCH,
	PSB_SGX_LAB_EXEC_SOURCE_RETIRED,
};

struct psb_sgx_lab_report_words {
	u32 stage;
	u32 stage_count;
	u32 prepare_count;
	u32 arm_count;
	u32 emit_count;
	u32 launch_count;
	u32 dispatch_count;
	u32 retired_count;
	u32 owner_state;
	u32 queue_packet_count;
	u32 completed_packets;
	u32 dependency_active_count;
	u32 queue_epoch;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 flags;
	u32 context;
};

struct psb_sgx_lab_live_words {
	u32 stage;
	u32 stage_count;
	u32 prepare_count;
	u32 arm_count;
	u32 emit_count;
	u32 launch_count;
	u32 dispatch_count;
	u32 retired_count;
	u32 context;
	u32 cmd_type;
	u32 owner_state;
	u32 issued_packets;
	u32 completed_packets;
	u32 dependency_active_count;
	u32 refilled_from_complete;
	u32 queue_packet_count;
	u32 queue_epoch;
	u32 queue_base_offset;
	u32 queue_size;
	u32 queue_packet_size;
	u32 ccb_offset;
	u32 ccb_write_offset;
	u32 ccb_read_offset;
	u32 sync_offset;
	u32 sync_complete_seq;
	u32 sync_pending_seq;
	u32 sync_flags;
	u32 owner_offset;
	u32 owner_issue_cookie;
	u32 owner_complete_cookie;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 ctrl_flags;
	u32 report_flags;
	u32 num_ta_status_vals;
	u32 num_3d_status_vals;
	u32 ta_dependency;
	u32 status_ta_offset;
	u32 status_3d_offset;
	u32 status_stride;
	u32 status_value_seed;
	u32 control_object_id;
	u32 status_object_id;
	u32 ccb_service_address;
	u32 ccb_cache_control;
	u32 ccb_data0;
	u32 ccb_data1;
	u32 control_gpu_va32;
	u32 status_gpu_va32;
	u32 pds_gpu_va32;
	u32 use_gpu_va32;
	u32 target_gpu_va32;
	u32 ccb_status_gpu_va32;
};

struct psb_sgx_lab_queue_header_words {
	u32 write_offset;
	u32 read_offset;
	u32 queue_base_offset;
	u32 queue_size;
	u32 queue_packet_size;
	u32 queue_packet_count;
	u32 queue_tail_packet_count;
	u32 queue_head_packet_count;
};

struct psb_sgx_lab_ccb_record_words {
	u32 service_address;
	u32 cache_control;
	u32 data0;
	u32 data1;
	u32 pds_gpu_va32;
	u32 use_gpu_va32;
	u32 target_gpu_va32;
	u32 status_gpu_va32;
};

struct psb_sgx_lab_sync_words {
	u32 read_offset;
	u32 write_offset;
	u32 complete_seq;
	u32 pending_seq;
	u32 flags;
	u32 epoch;
	u32 global_issue_ticket;
};

struct psb_sgx_lab_owner_words {
	u32 owner_state;
	u32 owner_issue_cookie;
	u32 owner_complete_cookie;
	u32 epoch;
	u32 global_complete_ticket;
	u32 global_running_ticket;
};

struct psb_sgx_lab_dependency_words {
	u32 source_submit_id;
	u32 required_ticket;
	u32 observed_complete_ticket;
	u32 flags;
};

struct psb_sgx_lab_manifest_words {
	u32 stage;
	u32 stage_count;
	u32 context;
	u32 cmd_type;
	u32 owner_state;
	u32 ctrl_flags;
	u32 queue_base_offset;
	u32 queue_size;
	u32 queue_packet_size;
	u32 queue_packet_count;
	u32 queue_epoch;
	u32 ccb_offset;
	u32 ccb_write_offset;
	u32 ccb_read_offset;
	u32 dependency_active_count;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 pds_gpu_va32;
	u32 use_gpu_va32;
	u32 target_gpu_va32;
	u32 status_gpu_va32;
	u32 sync_complete_seq;
	u32 sync_pending_seq;
	u32 sync_flags;
	u32 owner_issue_cookie;
	u32 owner_complete_cookie;
};

struct psb_sgx_lab_descriptor_words {
	u32 stage;
	u32 stage_count;
	u32 context;
	u32 cmd_type;
	u32 owner_state;
	u32 issued_packets;
	u32 completed_packets;
	u32 dependency_active_count;
	u32 queue_packet_count;
	u32 queue_epoch;
	u32 ctrl_flags;
	u32 queue_base_offset;
	u32 queue_size;
	u32 queue_packet_size;
	u32 ccb_offset;
	u32 sync_offset;
	u32 owner_offset;
	u32 report_offset;
	u32 manifest_offset;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 report_flags;
	u32 status_ta_offset;
	u32 status_3d_offset;
	u32 status_stride;
	u32 status_value_seed;
	u32 control_gpu_va32;
	u32 pds_gpu_va32;
	u32 use_gpu_va32;
	u32 target_gpu_va32;
	u32 status_gpu_va32;
};

struct psb_sgx_lab_plan_words {
	u32 stage;
	u32 stage_count;
	u32 source;
	u32 context;
	u32 cmd_type;
	u32 owner_state;
	u32 issued_packets;
	u32 completed_packets;
	u32 dependency_active_count;
	u32 refilled_from_complete;
	u32 queue_packet_count;
	u32 queue_epoch;
	u32 queue_base_offset;
	u32 queue_size;
	u32 queue_packet_size;
	u32 ccb_offset;
	u32 ccb_write_offset;
	u32 ccb_read_offset;
	u32 sync_offset;
	u32 sync_complete_seq;
	u32 sync_pending_seq;
	u32 sync_flags;
	u32 owner_offset;
	u32 owner_issue_cookie;
	u32 owner_complete_cookie;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 report_offset;
	u32 manifest_offset;
	u32 descriptor_offset;
	u32 ctrl_flags;
	u32 ccb_service_address;
	u32 ccb_cache_control;
	u32 ccb_data0;
	u32 ccb_data1;
	u32 control_gpu_va32;
	u32 pds_gpu_va32;
	u32 use_gpu_va32;
	u32 target_gpu_va32;
	u32 status_gpu_va32;
};

struct psb_sgx_lab_issue_plan_words {
	u32 stage;
	u32 stage_count;
	u32 valid;
	u32 owner_state;
	u32 issued_packets;
	u32 refilled_from_complete;
	u32 queue_epoch;
	u32 queue_base_offset;
	u32 queue_size;
	u32 queue_packet_size;
	u32 queue_packet_count;
	u32 queue_tail_packet_count;
	u32 queue_head_packet_count;
	u32 ccb_offset;
	u32 ccb_write_offset;
	u32 ccb_read_offset;
	u32 sync_offset;
	u32 sync_complete_seq;
	u32 sync_pending_seq;
	u32 sync_flags;
	u32 owner_offset;
	u32 owner_issue_cookie;
	u32 owner_complete_cookie;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 dependency_active_count;
	u32 ccb_service_address;
	u32 ccb_cache_control;
	u32 ccb_data0;
	u32 ccb_data1;
	u32 control_object_id;
	u32 status_object_id;
	u32 control_gpu_va32;
	u32 pds_gpu_va32;
	u32 use_gpu_va32;
	u32 target_gpu_va32;
	u32 status_gpu_va32;
};

struct psb_sgx_lab_result_words {
	u32 stage;
	u32 stage_count;
	u32 source;
	u32 context;
	u32 cmd_type;
	u32 owner_state;
	u32 issued_packets;
	u32 completed_packets;
	u32 dependency_active_count;
	u32 queue_packet_count;
	u32 queue_epoch;
	u32 sync_complete_seq;
	u32 sync_pending_seq;
	u32 sync_flags;
	u32 owner_issue_cookie;
	u32 owner_complete_cookie;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 ctrl_flags;
	u32 num_ta_status_vals;
	u32 num_3d_status_vals;
	u32 ta_dependency;
	u32 status_ta_offset;
	u32 status_3d_offset;
	u32 status_stride;
	u32 status_value_seed;
	u32 control_object_id;
	u32 status_object_id;
	u32 control_gpu_va32;
	u32 status_gpu_va32;
	u32 report_offset;
	u32 manifest_offset;
	u32 descriptor_offset;
	u32 plan_offset;
};

struct psb_sgx_lab_submit_exec_record {
	u32 stage;
	u32 stage_count;
	u32 source;
	u32 prepare_count;
	u32 arm_count;
	u32 emit_count;
	u32 launch_count;
	u32 dispatch_count;
	u32 retired_count;
	u32 context;
	u32 cmd_type;
	u32 owner_state;
	u32 issued_packets;
	u32 completed_packets;
	u32 dependency_active_count;
	u32 refilled_from_complete;
	u32 queue_packet_count;
	u32 queue_epoch;
	u32 queue_base_offset;
	u32 queue_size;
	u32 queue_packet_size;
	u32 ccb_offset;
	u32 ccb_write_offset;
	u32 ccb_read_offset;
	u32 sync_offset;
	u32 sync_complete_seq;
	u32 sync_pending_seq;
	u32 sync_flags;
	u32 owner_offset;
	u32 owner_issue_cookie;
	u32 owner_complete_cookie;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 ctrl_flags;
	u32 report_flags;
	u32 num_ta_status_vals;
	u32 num_3d_status_vals;
	u32 ta_dependency;
	u32 status_ta_offset;
	u32 status_3d_offset;
	u32 status_stride;
	u32 status_value_seed;
	u32 control_object_id;
	u32 status_object_id;
	u32 ccb_service_address;
	u32 ccb_cache_control;
	u32 ccb_data0;
	u32 ccb_data1;
	u32 control_gpu_va32;
	u32 status_gpu_va32;
	u32 pds_gpu_va32;
	u32 use_gpu_va32;
	u32 target_gpu_va32;
	u32 ccb_status_gpu_va32;
};

struct psb_sgx_lab_submit_ccb_command {
	u32 service_address;
	u32 cache_control;
	u32 data0;
	u32 data1;
};

struct psb_sgx_lab_submit_ta_shared {
	u32 ctrl_flags;
	u32 report_flags;
	u32 num_ta_status_vals;
	u32 num_3d_status_vals;
	u32 ta_dependency;
	u32 status_ta_offset;
	u32 status_3d_offset;
	u32 status_stride;
	u32 status_value_seed;
	u32 sync_offset;
	u32 sync_complete_seq;
	u32 sync_pending_seq;
	u32 sync_flags;
	u32 owner_offset;
	u32 owner_state;
	u32 owner_issue_cookie;
	u32 owner_complete_cookie;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 dependency_active_count;
};

struct psb_sgx_lab_submit_exec_packet {
	u32 stage;
	u32 stage_count;
	u32 source;
	u32 context;
	u32 cmd_type;
	u32 issued_packets;
	u32 completed_packets;
	u32 refilled_from_complete;
	u32 queue_packet_count;
	u32 queue_epoch;
	u32 queue_base_offset;
	u32 queue_size;
	u32 queue_packet_size;
	u32 queue_tail_packet_count;
	u32 queue_head_packet_count;
	u32 ccb_offset;
	u32 ccb_write_offset;
	u32 ccb_read_offset;
	u32 control_object_id;
	u32 status_object_id;
	u32 control_gpu_va32;
	u32 status_gpu_va32;
	u32 pds_gpu_va32;
	u32 use_gpu_va32;
	u32 target_gpu_va32;
	u32 ccb_status_gpu_va32;
	struct psb_sgx_lab_submit_ccb_command command;
	struct psb_sgx_lab_submit_ta_shared ta_shared;
};

struct psb_sgx_lab_submit_view_records {
	bool have_live;
	bool have_plan;
	bool have_result;
	enum psb_sgx_lab_exec_source plan_source;
	enum psb_sgx_lab_exec_source result_source;
	struct psb_sgx_lab_submit_exec_record live;
	struct psb_sgx_lab_submit_exec_record plan;
	struct psb_sgx_lab_submit_exec_record result;
};

struct psb_sgx_lab_refresh_views {
	struct psb_sgx_lab_object *control_obj;
	struct psb_sgx_lab_object *status_obj;
	bool have_live;
	bool have_report;
	bool have_manifest;
	bool have_descriptor;
	bool have_plan;
	bool have_issue_plan;
	bool have_result;
	struct psb_sgx_lab_live_words live;
	struct psb_sgx_lab_report_words report;
	struct psb_sgx_lab_manifest_words manifest;
	struct psb_sgx_lab_descriptor_words descriptor;
	struct psb_sgx_lab_plan_words plan;
	struct psb_sgx_lab_issue_plan_words issue_plan;
	struct psb_sgx_lab_result_words result;
};

static_assert(sizeof(struct psb_sgx_lab_report_words) ==
	      PSB_SGX_LAB_REPORT_DWORDS * sizeof(u32));
static_assert(sizeof(struct psb_sgx_lab_live_words) ==
	      PSB_SGX_LAB_LIVE_DWORDS * sizeof(u32));
static_assert(sizeof(struct psb_sgx_lab_queue_header_words) ==
	      PSB_SGX_LAB_QUEUE_HEADER_DWORDS * sizeof(u32));
static_assert(sizeof(struct psb_sgx_lab_ccb_record_words) ==
	      PSB_SGX_LAB_CCB_RECORD_DWORDS * sizeof(u32));
static_assert(sizeof(struct psb_sgx_lab_sync_words) ==
	      PSB_SGX_LAB_SYNC_DWORDS * sizeof(u32));
static_assert(sizeof(struct psb_sgx_lab_owner_words) ==
	      PSB_SGX_LAB_OWNER_DWORDS * sizeof(u32));
static_assert(sizeof(struct psb_sgx_lab_manifest_words) ==
	      PSB_SGX_LAB_MANIFEST_DWORDS * sizeof(u32));
static_assert(sizeof(struct psb_sgx_lab_descriptor_words) ==
	      PSB_SGX_LAB_DESCRIPTOR_DWORDS * sizeof(u32));
static_assert(sizeof(struct psb_sgx_lab_plan_words) ==
	      PSB_SGX_LAB_PLAN_DWORDS * sizeof(u32));
static_assert(sizeof(struct psb_sgx_lab_issue_plan_words) ==
	      PSB_SGX_LAB_ISSUE_PLAN_DWORDS * sizeof(u32));
static_assert(sizeof(struct psb_sgx_lab_result_words) ==
	      PSB_SGX_LAB_RESULT_DWORDS * sizeof(u32));
static_assert(sizeof(struct psb_sgx_lab_dependency_words) ==
	      PSB_SGX_LAB_DEPENDENCY_DWORDS * sizeof(u32));

struct psb_sgx_lab_submit {
	struct list_head head;
	u32 submit_id;
	u32 context;
	u32 control_object_id;
	u32 pds_object_id;
	u32 use_object_id;
	u32 target_object_id;
	u32 status_object_id;
	u32 cmd_type;
	u32 ccb_offset;
	u32 ctrl_flags;
	u32 num_ta_status_vals;
	u32 num_3d_status_vals;
	u32 ta_dependency;
	u32 dependency_ticket_secondary;
	u32 dependency_ticket_tertiary;
	u32 dependency_ticket_quaternary;
	u32 ccb_write_offset;
	u32 ccb_read_offset;
	u32 ccb_service_address;
	u32 ccb_cache_control;
	u32 ccb_data0;
	u32 ccb_data1;
	u32 queue_base_offset;
	u32 queue_size;
	u32 queue_packet_size;
	u32 queue_packet_count;
	u32 queue_tail_packet_count;
	u32 queue_head_packet_count;
	u32 status_ta_offset;
	u32 status_3d_offset;
	u32 status_stride;
	u32 status_value_seed;
	u32 sync_offset;
	u32 sync_complete_seq;
	u32 sync_pending_seq;
	u32 sync_flags;
	u32 dependency_submit_id;
	u32 dependency_submit_id_secondary;
	u32 dependency_submit_id_tertiary;
	u32 dependency_submit_id_quaternary;
	u32 dependency_offset;
	u32 dependency_offset_secondary;
	u32 dependency_offset_tertiary;
	u32 dependency_offset_quaternary;
	u32 dependency_snapshot_source_submit_id;
	u32 dependency_snapshot_source_submit_id_secondary;
	u32 dependency_snapshot_source_submit_id_tertiary;
	u32 dependency_snapshot_source_submit_id_quaternary;
	u32 dependency_snapshot_required_ticket;
	u32 dependency_snapshot_required_ticket_secondary;
	u32 dependency_snapshot_required_ticket_tertiary;
	u32 dependency_snapshot_required_ticket_quaternary;
	u32 dependency_snapshot_observed_complete_ticket;
	u32 dependency_snapshot_observed_complete_ticket_secondary;
	u32 dependency_snapshot_observed_complete_ticket_tertiary;
	u32 dependency_snapshot_observed_complete_ticket_quaternary;
	u32 dependency_snapshot_flags;
	u32 dependency_snapshot_flags_secondary;
	u32 dependency_snapshot_flags_tertiary;
	u32 dependency_snapshot_flags_quaternary;
	u32 owner_offset;
	u32 owner_state;
	u32 owner_issue_cookie;
	u32 owner_complete_cookie;
	u32 report_offset;
	u32 live_offset;
	u32 manifest_offset;
	u32 descriptor_offset;
	u32 plan_offset;
	u32 issue_plan_offset;
	u32 result_offset;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	int last_validate_status;
	u32 validate_count;
	u32 packet_config_count;
	u32 ccb_config_count;
	u32 queue_config_count;
	u32 status_config_count;
	u32 sync_config_count;
	u32 dependency_config_count;
	u32 dependency_config_count_secondary;
	u32 dependency_config_count_tertiary;
	u32 dependency_config_count_quaternary;
	u32 owner_config_count;
	u32 report_config_count;
	u32 live_config_count;
	u32 manifest_config_count;
	u32 descriptor_config_count;
	u32 plan_config_count;
	u32 issue_plan_config_count;
	u32 result_config_count;
	u32 issue_plan_owner_state;
	u32 issue_plan_packet_count;
	u32 queue_epoch;
	u32 prepare_count;
	u32 report_stage;
	u32 report_stage_count;
	u32 report_owner_state;
	u32 report_queue_packet_count;
	u32 report_completed_packets;
	u32 report_dependency_active_count;
	u32 report_queue_epoch;
	u32 report_global_issue_ticket;
	u32 report_global_running_ticket;
	u32 report_global_complete_ticket;
	u32 report_flags;
	u32 arm_prepare_count;
	u32 arm_queue_packet_count;
	u32 arm_queue_tail_packet_count;
	u32 arm_queue_head_packet_count;
	u32 arm_ccb_read_offset;
	u32 arm_ccb_write_offset;
	u32 arm_sync_complete_seq;
	u32 arm_sync_pending_seq;
	u32 arm_owner_state;
	u32 arm_owner_issue_cookie;
	u32 arm_owner_complete_cookie;
	u32 arm_queue_epoch;
	u32 arm_global_issue_ticket;
	u32 emit_prepare_count;
	u32 emit_arm_count;
	u32 emit_context;
	u32 emit_cmd_type;
	u32 emit_owner_state;
	u32 emit_ctrl_flags;
	u32 emit_num_ta_status_vals;
	u32 emit_num_3d_status_vals;
	u32 emit_ta_dependency;
	u32 emit_control_object_id;
	u32 emit_pds_object_id;
	u32 emit_use_object_id;
	u32 emit_target_object_id;
	u32 emit_status_object_id;
	u32 emit_ccb_offset;
	u32 emit_ccb_write_offset;
	u32 emit_ccb_read_offset;
	u32 emit_ccb_service_address;
	u32 emit_ccb_cache_control;
	u32 emit_ccb_data0;
	u32 emit_ccb_data1;
	u32 emit_queue_base_offset;
	u32 emit_queue_size;
	u32 emit_queue_packet_size;
	u32 emit_queue_packet_count;
	u32 emit_queue_tail_packet_count;
	u32 emit_queue_head_packet_count;
	u32 emit_sync_complete_seq;
	u32 emit_sync_pending_seq;
	u32 emit_sync_flags;
	u32 emit_owner_issue_cookie;
	u32 emit_owner_complete_cookie;
	u32 emit_queue_epoch;
	u32 emit_global_issue_ticket;
	u32 emit_dependency_active_count;
	u64 emit_control_gpu_va;
	u64 emit_pds_gpu_va;
	u64 emit_use_gpu_va;
	u64 emit_target_gpu_va;
	u64 emit_status_gpu_va;
	u32 launch_prepare_count;
	u32 launch_arm_count;
	u32 launch_emit_count;
	u32 launch_context;
	u32 launch_cmd_type;
	u32 launch_owner_state;
	u32 launch_issued_packets;
	u32 launch_refilled_from_complete;
	u32 launch_ctrl_flags;
	u32 launch_num_ta_status_vals;
	u32 launch_num_3d_status_vals;
	u32 launch_ta_dependency;
	u32 launch_control_object_id;
	u32 launch_pds_object_id;
	u32 launch_use_object_id;
	u32 launch_target_object_id;
	u32 launch_status_object_id;
	u32 launch_ccb_offset;
	u32 launch_ccb_write_offset;
	u32 launch_ccb_read_offset;
	u32 launch_new_write_offset;
	u32 launch_ccb_service_address;
	u32 launch_ccb_cache_control;
	u32 launch_ccb_data0;
	u32 launch_ccb_data1;
	u32 launch_queue_base_offset;
	u32 launch_queue_size;
	u32 launch_queue_packet_size;
	u32 launch_queue_packet_count;
	u32 launch_queue_tail_packet_count;
	u32 launch_queue_head_packet_count;
	u32 launch_sync_offset;
	u32 launch_sync_complete_seq;
	u32 launch_sync_pending_seq;
	u32 launch_sync_flags;
	u32 launch_owner_offset;
	u32 launch_owner_issue_cookie;
	u32 launch_owner_complete_cookie;
	u32 launch_queue_epoch;
	u32 launch_global_issue_ticket;
	u32 launch_global_running_ticket;
	u32 launch_global_complete_ticket;
	u32 launch_dependency_active_count;
	u64 launch_control_gpu_va;
	u64 launch_pds_gpu_va;
	u64 launch_use_gpu_va;
	u64 launch_target_gpu_va;
	u64 launch_status_gpu_va;
	u32 dispatch_prepare_count;
	u32 dispatch_arm_count;
	u32 dispatch_emit_count;
	u32 dispatch_launch_count;
	u32 dispatch_context;
	u32 dispatch_cmd_type;
	u32 dispatch_owner_state;
	u32 dispatch_issued_packets;
	u32 dispatch_completed_packets;
	u32 dispatch_refilled_from_complete;
	u32 dispatch_ctrl_flags;
	u32 dispatch_num_ta_status_vals;
	u32 dispatch_num_3d_status_vals;
	u32 dispatch_ta_dependency;
	u32 dispatch_control_object_id;
	u32 dispatch_pds_object_id;
	u32 dispatch_use_object_id;
	u32 dispatch_target_object_id;
	u32 dispatch_status_object_id;
	u32 dispatch_ccb_offset;
	u32 dispatch_ccb_write_offset;
	u32 dispatch_ccb_read_offset;
	u32 dispatch_ccb_service_address;
	u32 dispatch_ccb_cache_control;
	u32 dispatch_ccb_data0;
	u32 dispatch_ccb_data1;
	u32 dispatch_queue_base_offset;
	u32 dispatch_queue_size;
	u32 dispatch_queue_packet_size;
	u32 dispatch_queue_packet_count;
	u32 dispatch_queue_tail_packet_count;
	u32 dispatch_queue_head_packet_count;
	u32 dispatch_sync_offset;
	u32 dispatch_sync_complete_seq;
	u32 dispatch_sync_pending_seq;
	u32 dispatch_sync_flags;
	u32 dispatch_owner_offset;
	u32 dispatch_owner_issue_cookie;
	u32 dispatch_owner_complete_cookie;
	u32 dispatch_queue_epoch;
	u32 dispatch_global_issue_ticket;
	u32 dispatch_global_running_ticket;
	u32 dispatch_global_complete_ticket;
	u32 dispatch_dependency_active_count;
	u64 dispatch_control_gpu_va;
	u64 dispatch_pds_gpu_va;
	u64 dispatch_use_gpu_va;
	u64 dispatch_target_gpu_va;
	u64 dispatch_status_gpu_va;
	u32 retired_prepare_count;
	u32 retired_arm_count;
	u32 retired_emit_count;
	u32 retired_launch_count;
	u32 retired_dispatch_count;
	u32 retired_context;
	u32 retired_cmd_type;
	u32 retired_owner_state;
	u32 retired_issued_packets;
	u32 retired_completed_packets;
	u32 retired_refilled_from_complete;
	u32 retired_queue_packet_count;
	u32 retired_sync_complete_seq;
	u32 retired_sync_pending_seq;
	u32 retired_owner_issue_cookie;
	u32 retired_owner_complete_cookie;
	u32 retired_queue_epoch;
	u32 retired_global_issue_ticket;
	u32 retired_global_running_ticket;
	u32 retired_global_complete_ticket;
	u32 retired_dependency_active_count;
	u32 retired_control_object_id;
	u32 retired_status_object_id;
	u64 retired_control_gpu_va;
	u64 retired_status_gpu_va;
	bool arm_valid;
	u32 arm_count;
	bool emit_valid;
	bool launch_valid;
	bool launch_consumed;
	bool dispatch_valid;
	bool retired_valid;
	u32 emit_count;
	u32 launch_count;
	u32 dispatch_count;
	u32 retired_count;
	u32 issue_count;
	u32 transition_count;
	unsigned long created_jiffies;
	unsigned long last_validate_jiffies;
	unsigned long last_packet_jiffies;
	unsigned long last_ccb_jiffies;
	unsigned long last_queue_jiffies;
	unsigned long last_status_jiffies;
	unsigned long last_sync_jiffies;
	unsigned long last_dependency_jiffies;
	unsigned long last_dependency_secondary_jiffies;
	unsigned long last_dependency_tertiary_jiffies;
	unsigned long last_dependency_quaternary_jiffies;
	unsigned long last_dependency_block_jiffies;
	unsigned long last_dependency_block_secondary_jiffies;
	unsigned long last_dependency_block_tertiary_jiffies;
	unsigned long last_dependency_block_quaternary_jiffies;
	unsigned long last_owner_jiffies;
	unsigned long last_epoch_jiffies;
	unsigned long last_report_jiffies;
	unsigned long last_live_jiffies;
	unsigned long last_manifest_jiffies;
	unsigned long last_descriptor_jiffies;
	unsigned long last_plan_jiffies;
	unsigned long last_issue_plan_jiffies;
	unsigned long last_result_jiffies;
	unsigned long last_prepare_jiffies;
	unsigned long last_arm_jiffies;
	unsigned long last_emit_jiffies;
	unsigned long last_launch_jiffies;
	unsigned long last_dispatch_jiffies;
	unsigned long last_retired_jiffies;
	unsigned long last_issue_jiffies;
	unsigned long last_transition_jiffies;
};

struct psb_sgx_lab_issue_plan {
	u32 owner_state;
	u32 issued_packets;
	bool refilled_from_complete;
	u32 new_queue_epoch;
	u32 new_packet_count;
	u32 new_tail_packets;
	u32 new_head_packets;
	u32 new_write_offset;
	u32 new_sync_pending_seq;
	u32 new_owner_issue_cookie;
	u32 new_global_issue_ticket;
	u32 new_sync_flags;
};

struct psb_sgx_lab_submit_cmdta_kick {
	struct psb_sgx_lab_submit_exec_packet packet;
	bool have_issue_plan;
	struct psb_sgx_lab_issue_plan issue_plan;
	u32 queue_gpu_va32;
	u32 ccb_gpu_va32;
	u32 sync_gpu_va32;
	u32 owner_gpu_va32;
	u32 ta_status_gpu_va32;
	u32 status_3d_gpu_va32;
};

struct psb_sgx_lab_submit_status_update {
	u32 value_offset;
	u32 value_gpu_va32;
	u32 value;
};

struct psb_sgx_lab_submit_ccb_kick {
	struct psb_sgx_lab_submit_cmdta_kick cmdta;
	u32 ccb_offset;
	bool ta_dependency;
	u32 num_ta_status_updates;
	u32 num_3d_status_updates;
	struct psb_sgx_lab_submit_status_update
		ta_status_updates[PSB_SGX_LAB_MAX_STATUS_VALS];
	struct psb_sgx_lab_submit_status_update
		threed_status_updates[PSB_SGX_LAB_MAX_STATUS_VALS];
};

struct psb_sgx_lab_submit_sgx_ccb_kick {
	const struct psb_sgx_lab_submit_ccb_kick *kick;
	u32 num_dst_sync_objects;
	u32 num_ta_status_vals;
	u32 num_3d_status_vals;
	bool first_kick_or_resume;
	u32 ccb_offset;
	u32 num_ta_src_syncs;
	u32 num_ta_dst_syncs;
	u32 num_3d_src_syncs;
	bool ta_dependency;
	u32 ta3d_dependency_submit_id;
	u32 ta3d_dependency_required_complete_ticket;
	u32 ta3d_dependency_observed_complete_ticket;
	u32 ta3d_dependency_flags;
	u32 ta_sync_gpu_va32;
	u32 threed_sync_gpu_va32;
};

struct psb_sgx_lab_submit_sgx_device_sync_object {
	u32 read_pending_val;
	u32 read_complete_gpu_va32;
	u32 write_pending_val;
	u32 write_complete_gpu_va32;
};

struct psb_sgx_lab_submit_sgx_hwdevice_sync_list {
	u32 access_gpu_va32;
	u32 num_sync_objects;
	struct psb_sgx_lab_submit_sgx_device_sync_object
		sync_data[PSB_SGX_LAB_MAX_DST_SYNC_OBJECTS];
};

struct psb_sgx_lab_submit_sgx_ta3d_dependency {
	u32 submit_id;
	u32 required_complete_ticket;
	u32 observed_complete_ticket;
	u32 flags;
	u32 gpu_va32;
};

struct psb_sgx_lab_submit_sgx_cmdta_shared {
	const struct psb_sgx_lab_submit_ccb_kick *kick;
	u32 ctrl_flags;
	u32 num_dst_sync_objects;
	u32 num_ta_status_vals;
	u32 num_3d_status_vals;
	bool first_kick_or_resume;
	u32 ccb_offset;
	bool ta_dependency;
	struct psb_sgx_lab_submit_sgx_device_sync_object ta_tq_sync;
	struct psb_sgx_lab_submit_sgx_device_sync_object threed_tq_sync;
	struct psb_sgx_lab_submit_sgx_device_sync_object ta3d_dependency_sync;
	struct psb_sgx_lab_submit_sgx_hwdevice_sync_list hwdevice_sync_list;
	u32 num_ta_src_syncs;
	u32 num_ta_dst_syncs;
	u32 num_3d_src_syncs;
	struct psb_sgx_lab_submit_sgx_ta3d_dependency ta3d_dependency;
	const struct psb_sgx_lab_submit_status_update *ta_status_info;
	const struct psb_sgx_lab_submit_status_update *threed_status_info;
};

struct psb_sgx_lab_submit_sgx_cmdta {
	const struct psb_sgx_lab_submit_ccb_kick *kick;
	struct psb_sgx_lab_submit_sgx_cmdta_shared shared;
	struct psb_sgx_lab_submit_sgx_device_sync_object
		ta_src_syncs[PSB_SGX_LAB_MAX_TA_SRC_SYNCS];
	struct psb_sgx_lab_submit_sgx_device_sync_object
		ta_dst_syncs[PSB_SGX_LAB_MAX_TA_DST_SYNCS];
	struct psb_sgx_lab_submit_sgx_device_sync_object
		threed_src_syncs[PSB_SGX_LAB_MAX_3D_SRC_SYNCS];
};

struct psb_sgx_lab_submit_sgx_command {
	u32 service_address;
	u32 cache_control;
	u32 data0;
	u32 data1;
};

struct psb_sgx_lab_submit_sgx_ta_kick {
	const struct psb_sgx_lab_submit_ccb_kick *kick;
	struct psb_sgx_lab_submit_sgx_command command;
	u32 control_gpu_va32;
	u32 ccb_gpu_va32;
	u32 cmdta_gpu_va32;
	u32 queue_gpu_va32;
	u32 sync_gpu_va32;
	u32 owner_gpu_va32;
	u32 ta_status_gpu_va32;
	u32 status_3d_gpu_va32;
	struct psb_sgx_lab_submit_sgx_cmdta cmdta;
};

struct psb_sgx_lab_submit_sgx_ta_command {
	const struct psb_sgx_lab_submit_ccb_kick *kick;
	struct psb_sgx_lab_submit_sgx_ta_kick ta_kick;
	struct psb_sgx_lab_ccb_record_words ccb_record;
};

struct psb_sgx_lab_submit_sgx_queued_ta_command {
	const struct psb_sgx_lab_submit_ccb_kick *kick;
	struct psb_sgx_lab_submit_sgx_ta_command ta_command;
	struct psb_sgx_lab_queue_header_words queue_header;
	bool have_issue_plan;
	struct psb_sgx_lab_issue_plan issue_plan;
};

struct psb_sgx_lab_submit_sgx_ta_submission {
	const struct psb_sgx_lab_submit_ccb_kick *kick;
	struct psb_sgx_lab_submit_sgx_queued_ta_command queued_ta_command;
	bool have_live;
	bool have_manifest;
	bool have_descriptor;
	bool have_plan;
	bool have_issue_plan_view;
	struct psb_sgx_lab_live_words live;
	struct psb_sgx_lab_manifest_words manifest;
	struct psb_sgx_lab_descriptor_words descriptor;
	struct psb_sgx_lab_plan_words plan;
	struct psb_sgx_lab_issue_plan_words issue_plan;
};

struct psb_sgx_lab_submit_sgx_ta_execution {
	struct psb_sgx_lab_submit_ccb_kick live_kick;
	struct psb_sgx_lab_submit_sgx_ta_submission live_submission;
	bool have_launch_submission;
	struct psb_sgx_lab_submit_ccb_kick launch_kick;
	struct psb_sgx_lab_submit_sgx_ta_submission launch_submission;
};

struct psb_sgx_lab_submit_sgx_ta_timeline {
	struct psb_sgx_lab_submit_sgx_ta_execution execution;
	u32 report_stage;
	u32 report_stage_count;
	u32 issue_count;
	u32 transition_count;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	bool arm_valid;
	bool emit_valid;
	bool launch_valid;
	bool launch_consumed;
	bool dispatch_valid;
	bool retired_valid;
	bool have_launch_command;
	struct psb_sgx_lab_submit_ccb_kick launch_kick;
	struct psb_sgx_lab_submit_sgx_queued_ta_command launch_command;
	bool have_dispatch_command;
	struct psb_sgx_lab_submit_ccb_kick dispatch_kick;
	struct psb_sgx_lab_submit_sgx_queued_ta_command dispatch_command;
	bool have_retired_command;
	struct psb_sgx_lab_submit_ccb_kick retired_kick;
	struct psb_sgx_lab_submit_sgx_queued_ta_command retired_command;
};

struct psb_sgx_lab_submit_sgx_dependency_state {
	bool active;
	bool source_missing;
	u32 submit_id;
	u32 required_ticket;
	u32 observed_complete_ticket;
	u32 flags;
};

struct psb_sgx_lab_submit_sgx_coordination {
	u32 issue_ticket;
	u32 running_ticket;
	u32 complete_ticket;
	u32 issue_submit_id;
	u32 running_submit_id;
	u32 complete_submit_id;
	bool active_running_valid;
	u32 active_running_submit_id;
	u32 running_owner_submit_count;
	u32 running_owner_submit_id;
};

struct psb_sgx_lab_submit_sgx_ta_runtime {
	struct psb_sgx_lab_submit_sgx_ta_timeline timeline;
	struct psb_sgx_lab_submit_sgx_coordination coordination;
	u32 sync_outstanding_count;
	u32 owner_outstanding_count;
	struct psb_sgx_lab_submit_sgx_dependency_state
		dependencies[PSB_SGX_LAB_DEPENDENCY_SLOTS];
};

struct psb_sgx_lab_submit_sgx_gate_slot {
	bool active;
	bool source_bound;
	bool source_missing;
	bool gated;
	bool satisfied;
	bool blocked;
	bool ready;
	u32 submit_id;
	u32 required_ticket;
	u32 observed_complete_ticket;
	u32 flags;
};

struct psb_sgx_lab_submit_sgx_ta_gate {
	struct psb_sgx_lab_submit_sgx_gate_slot
		dependencies[PSB_SGX_LAB_DEPENDENCY_SLOTS];
	u32 active_dependency_count;
	u32 source_bound_dependency_count;
	u32 gated_dependency_count;
	u32 satisfied_dependency_count;
	u32 blocked_dependency_count;
	u32 missing_dependency_count;
	bool running_slot_busy;
	bool can_enter_running;
};

struct psb_sgx_lab_submit_sgx_issue_request {
	bool active;
	bool issue_to_running;
	bool plan_buildable;
	bool plan_matches_launch;
	bool gate_ready;
	bool can_issue;
	u32 owner_state;
	u32 issued_packets;
	struct psb_sgx_lab_issue_plan plan;
};

struct psb_sgx_lab_submit_sgx_ta_control {
	struct psb_sgx_lab_submit_sgx_ta_runtime runtime;
	struct psb_sgx_lab_submit_sgx_ta_gate gate;
	bool emit_matches_live;
	bool have_launch_snapshot;
	bool launch_consumed;
	bool launch_ready;
	bool running_owned_by_self;
	struct psb_sgx_lab_submit_sgx_issue_request issue_request;
};

struct psb_sgx_lab_submit_sgx_transition_request {
	bool active;
	bool entering_running;
	bool entering_complete;
	bool can_transition;
	u32 owner_state;
	u32 completed_packets;
	u32 new_packet_count;
	u32 new_read_offset;
	u32 new_write_offset;
	u32 new_tail_packets;
	u32 new_head_packets;
	u32 new_ccb_offset;
	u32 new_sync_complete_seq;
	u32 new_sync_flags;
	u32 new_owner_complete_cookie;
	u32 new_global_running_ticket;
	u32 new_global_complete_ticket;
};

struct psb_sgx_lab_submit_sgx_ta_schedule {
	struct psb_sgx_lab_submit_sgx_ta_control control;
	bool can_issue_queued;
	bool can_issue_running;
	bool can_enter_running;
	bool can_stay_running;
	bool can_complete;
	struct psb_sgx_lab_submit_sgx_transition_request transition_request;
};

struct psb_sgx_lab_submit_sgx_issue_image {
	bool active;
	u32 issue_write_offset;
	u32 final_write_offset;
	u32 queue_region_end;
	u32 sync_region_end;
	u32 owner_region_end;
	struct psb_sgx_lab_queue_header_words queue_header;
	struct psb_sgx_lab_ccb_record_words ccb_record;
	struct psb_sgx_lab_sync_words sync;
	struct psb_sgx_lab_owner_words owner;
};

struct psb_sgx_lab_submit_sgx_transition_image {
	bool active;
	u32 queue_region_end;
	u32 sync_region_end;
	u32 owner_region_end;
	struct psb_sgx_lab_queue_header_words queue_header;
	struct psb_sgx_lab_ccb_record_words ccb_record;
	struct psb_sgx_lab_sync_words sync;
	struct psb_sgx_lab_owner_words owner;
};

struct psb_sgx_lab_submit_sgx_ta_dispatch {
	struct psb_sgx_lab_submit_sgx_ta_schedule schedule;
	struct psb_sgx_lab_submit_sgx_issue_image issue_image;
};

struct psb_sgx_lab_submit_sgx_ta_path {
	struct psb_sgx_lab_submit_sgx_ta_dispatch dispatch;
	struct psb_sgx_lab_submit_sgx_transition_image transition_image;
};

struct psb_sgx_lab_submit_sgx_ta_commit_state {
	u32 ccb_offset;
	u32 ccb_read_offset;
	u32 ccb_write_offset;
	u32 queue_packet_count;
	u32 queue_tail_packet_count;
	u32 queue_head_packet_count;
	u32 sync_complete_seq;
	u32 sync_pending_seq;
	u32 sync_flags;
	u32 owner_state;
	u32 owner_issue_cookie;
	u32 owner_complete_cookie;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 queue_epoch;
	bool launch_consumed;
};

struct psb_sgx_lab_submit_sgx_ta_commit {
	struct psb_sgx_lab_submit_sgx_ta_path path;
	bool issue_active;
	bool transition_active;
	u32 control_object_id;
	u32 status_object_id;
	u32 pds_object_id;
	u32 use_object_id;
	u32 target_object_id;
	u32 issue_queue_base_offset;
	u32 issue_queue_size;
	u32 issue_write_offset;
	u32 issue_packet_size;
	u32 issue_packet_count;
	u32 transition_ccb_offset;
	u32 sync_offset;
	u32 owner_offset;
	u32 completed_packets;
	bool entering_running;
	bool entering_complete;
	struct psb_sgx_lab_submit_sgx_ta_commit_state next_state;
};

struct psb_sgx_lab_submit_sgx_ta_event {
	struct psb_sgx_lab_submit_sgx_ta_commit commit;
	bool refresh_views;
	bool increment_issue_count;
	bool increment_transition_count;
	bool update_last_epoch_jiffies;
	bool update_last_issue_jiffies;
	bool update_last_transition_jiffies;
};

struct psb_sgx_lab_submit_sgx_ta_work_item {
	struct psb_sgx_lab_submit_sgx_ta_event event;
	bool validate_before_apply;
	bool disarm_on_build_failure;
	u32 submit_id;
	u32 owner_state;
	u32 issued_packets;
	u32 completed_packets;
};

enum psb_sgx_lab_submit_sgx_ta_op_kind {
	PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE = 0,
	PSB_SGX_LAB_SUBMIT_SGX_TA_OP_ISSUE = 1,
	PSB_SGX_LAB_SUBMIT_SGX_TA_OP_TRANSITION = 2,
};

struct psb_sgx_lab_submit_sgx_ta_op {
	struct psb_sgx_lab_submit_sgx_ta_work_item work_item;
	u32 submit_id;
	u32 op_kind;
	u32 owner_state;
	u32 packet_count;
};

struct psb_sgx_lab_submit_sgx_scheduler_state {
	u32 issue_ticket;
	u32 running_ticket;
	u32 complete_ticket;
	u32 issue_submit_id;
	u32 running_submit_id;
	u32 complete_submit_id;
	bool active_running_valid;
	u32 active_running_submit_id;
};

struct psb_sgx_lab_submit_sgx_ta_scheduler {
	struct psb_sgx_lab_submit_sgx_ta_op ta_op;
	bool active;
	bool issue_advanced;
	bool running_advanced;
	bool complete_advanced;
	bool active_running_changed;
	struct psb_sgx_lab_submit_sgx_scheduler_state next_state;
};

struct psb_sgx_lab_submit_sgx_ta_sequence {
	struct psb_sgx_lab_submit_sgx_ta_scheduler scheduler;
	bool active;
	bool refresh_views;
	bool increment_issue_count;
	bool increment_transition_count;
	bool update_last_epoch_jiffies;
	bool update_last_issue_jiffies;
	bool update_last_transition_jiffies;
	u32 next_issue_count;
	u32 next_transition_count;
};

struct psb_sgx_lab_submit_sgx_ta_work_queue {
	struct psb_sgx_lab_submit_sgx_ta_sequence sequence;
	bool active;
	u32 submit_id;
	u32 op_kind;
	u32 owner_state;
	u32 packet_count;
	struct psb_sgx_lab_submit_sgx_scheduler_state base_state;
	u32 base_issue_count;
	u32 base_transition_count;
};

struct psb_sgx_lab_submit_sgx_ta_job {
	struct psb_sgx_lab_submit_sgx_ta_work_queue work_queue;
	bool active;
	u32 submit_id;
	u32 context;
	u32 cmd_type;
	u32 ctrl_flags;
	u32 control_object_id;
	u32 pds_object_id;
	u32 use_object_id;
	u32 target_object_id;
	u32 status_object_id;
	u32 num_ta_status_vals;
	u32 num_3d_status_vals;
	u32 ta_dependency;
	u32 active_dependency_count;
	bool running_slot_busy;
	bool can_enter_running;
	struct psb_sgx_lab_submit_sgx_dependency_state
		dependencies[PSB_SGX_LAB_DEPENDENCY_SLOTS];
};

struct psb_sgx_lab_submit_sgx_ta_request {
	struct psb_sgx_lab_submit_sgx_ta_job job;
	bool active;
	u32 submit_id;
	u32 op_kind;
	u32 owner_state;
	u32 packet_count;
	bool issue_active;
	bool transition_active;
	bool refresh_views;
	bool increment_issue_count;
	bool increment_transition_count;
	bool issue_advanced;
	bool running_advanced;
	bool complete_advanced;
	bool active_running_changed;
	struct psb_sgx_lab_submit_sgx_scheduler_state next_state;
};

struct psb_sgx_lab_submit_sgx_ta_ioctl_request {
	struct psb_sgx_lab_submit_sgx_ta_request request;
	bool active;
	bool dependency_fields_present;
	bool focus_resolved;
	u32 requested_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 op_kind;
	u32 owner_state;
	u32 packet_count;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request {
	struct psb_sgx_lab_submit_sgx_ta_ioctl_request ioctl_request;
	struct drm_psb_sgx_lab_submit_op op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	u32 resolved_submit_id;
	u32 op_kind;
};

struct psb_sgx_lab_submit_sgx_ta_queue_item {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request submit_request;
	bool active;
	bool issue_request;
	bool transition_request;
	bool refilled_from_complete;
	u32 submit_id;
	u32 owner_state;
	u32 packet_count;
	u32 new_read_offset;
	u32 new_write_offset;
	u32 new_ccb_offset;
	u32 outstanding_packets;
	bool wrapped;
	u32 pending_seq;
	u32 complete_seq;
	u32 issue_cookie;
	u32 complete_cookie;
	u32 epoch;
	u32 issue_ticket;
	u32 running_ticket;
	u32 complete_ticket;
};

struct psb_sgx_lab_submit_sgx_ta_queue_command {
	struct psb_sgx_lab_submit_sgx_ta_queue_item queue_item;
	bool active;
	bool issue_request;
	bool transition_request;
	bool refilled_from_complete;
	bool read_advanced;
	bool write_advanced;
	bool ccb_advanced;
	bool outstanding_changed;
	bool pending_advanced;
	bool complete_advanced;
	bool issue_cookie_advanced;
	bool complete_cookie_advanced;
	bool epoch_advanced;
	bool issue_ticket_advanced;
	bool running_ticket_advanced;
	bool complete_ticket_advanced;
	u32 submit_id;
	u32 owner_state;
	u32 packet_count;
	u32 prev_read_offset;
	u32 prev_write_offset;
	u32 prev_ccb_offset;
	u32 prev_outstanding_packets;
	bool prev_wrapped;
	u32 prev_pending_seq;
	u32 prev_complete_seq;
	u32 prev_issue_cookie;
	u32 prev_complete_cookie;
	u32 prev_epoch;
	u32 prev_issue_ticket;
	u32 prev_running_ticket;
	u32 prev_complete_ticket;
	u32 next_read_offset;
	u32 next_write_offset;
	u32 next_ccb_offset;
	u32 next_outstanding_packets;
	bool next_wrapped;
	u32 next_pending_seq;
	u32 next_complete_seq;
	u32 next_issue_cookie;
	u32 next_complete_cookie;
	u32 next_epoch;
	u32 next_issue_ticket;
	u32 next_running_ticket;
	u32 next_complete_ticket;
};

struct psb_sgx_lab_submit_sgx_ta_scheduler_entry {
	struct psb_sgx_lab_submit_sgx_ta_queue_command queue_command;
	bool active;
	bool issue_request;
	bool transition_request;
	bool issue_advanced;
	bool running_advanced;
	bool complete_advanced;
	bool active_running_changed;
	bool issue_submit_changed;
	bool running_submit_changed;
	bool complete_submit_changed;
	u32 submit_id;
	u32 owner_state;
	u32 packet_count;
	struct psb_sgx_lab_submit_sgx_scheduler_state prev_state;
	struct psb_sgx_lab_submit_sgx_scheduler_state next_state;
};

struct psb_sgx_lab_submit_sgx_ta_queue_stream {
	struct psb_sgx_lab_submit_sgx_ta_scheduler_entry scheduler_entry;
	bool active;
	bool issue_request;
	bool transition_request;
	bool stream_active;
	bool use_issue_stream;
	bool use_transition_stream;
	u32 submit_id;
	u32 owner_state;
	u32 packet_count;
	u32 stream_ccb_offset;
	u32 queue_region_end;
	u32 sync_region_end;
	u32 owner_region_end;
	struct psb_sgx_lab_queue_header_words queue_header;
	struct psb_sgx_lab_ccb_record_words ccb_record;
	struct psb_sgx_lab_sync_words sync;
	struct psb_sgx_lab_owner_words owner;
};

struct psb_sgx_lab_submit_sgx_ta_queue_packet {
	struct psb_sgx_lab_submit_sgx_ta_queue_stream queue_stream;
	bool active;
	bool issue_request;
	bool transition_request;
	bool packet_active;
	bool refilled_from_complete;
	bool read_advanced;
	bool write_advanced;
	bool ccb_advanced;
	bool outstanding_changed;
	bool pending_advanced;
	bool complete_advanced;
	bool issue_cookie_advanced;
	bool complete_cookie_advanced;
	bool epoch_advanced;
	bool issue_ticket_advanced;
	bool running_ticket_advanced;
	bool complete_ticket_advanced;
	bool scheduler_issue_advanced;
	bool scheduler_running_advanced;
	bool scheduler_complete_advanced;
	u32 submit_id;
	u32 owner_state;
	u32 packet_count;
	u32 queue_read_offset;
	u32 queue_write_offset;
	u32 sync_read_offset;
	u32 sync_write_offset;
	u32 ccb_offset;
	u32 queue_base_offset;
	u32 queue_size;
	u32 queue_packet_size;
	u32 queue_packet_count;
	u32 queue_tail_packet_count;
	u32 queue_head_packet_count;
	u32 sync_complete_seq;
	u32 sync_pending_seq;
	u32 sync_flags;
	u32 owner_issue_cookie;
	u32 owner_complete_cookie;
	u32 epoch;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
};

struct psb_sgx_lab_submit_sgx_ta_request_packet {
	struct psb_sgx_lab_submit_sgx_ta_queue_packet queue_packet;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	bool packet_active;
	bool refilled_from_complete;
	u32 requested_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 op_kind;
	u32 owner_state;
	u32 packet_count;
	u32 selected_read_offset;
	u32 selected_write_offset;
	u32 selected_ccb_offset;
	u32 selected_outstanding_packets;
	bool selected_wrapped;
	u32 selected_seq;
	u32 selected_cookie;
	u32 epoch;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
};

struct psb_sgx_lab_submit_sgx_ta_request_sequence {
	struct psb_sgx_lab_submit_sgx_ta_request_packet request_packet;
	bool active;
	bool issue_active;
	bool transition_active;
	bool refresh_views;
	bool increment_issue_count;
	bool increment_transition_count;
	bool update_last_epoch_jiffies;
	bool update_last_issue_jiffies;
	bool update_last_transition_jiffies;
	bool issue_advanced;
	bool running_advanced;
	bool complete_advanced;
	bool active_running_changed;
	u32 next_issue_count;
	u32 next_transition_count;
	struct psb_sgx_lab_submit_sgx_scheduler_state next_state;
};

struct psb_sgx_lab_submit_sgx_ta_request_work_packet {
	struct psb_sgx_lab_submit_sgx_ta_request_sequence request_sequence;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	bool packet_active;
	bool refilled_from_complete;
	bool refresh_views;
	bool increment_issue_count;
	bool increment_transition_count;
	bool update_last_epoch_jiffies;
	bool update_last_issue_jiffies;
	bool update_last_transition_jiffies;
	bool issue_advanced;
	bool running_advanced;
	bool complete_advanced;
	bool active_running_changed;
	bool selected_wrapped;
	u32 requested_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 op_kind;
	u32 owner_state;
	u32 packet_count;
	u32 selected_read_offset;
	u32 selected_write_offset;
	u32 selected_ccb_offset;
	u32 selected_outstanding_packets;
	u32 selected_seq;
	u32 selected_cookie;
	u32 epoch;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 next_issue_count;
	u32 next_transition_count;
	struct psb_sgx_lab_submit_sgx_scheduler_state next_state;
};

struct psb_sgx_lab_submit_sgx_ta_request_execution {
	struct psb_sgx_lab_submit_sgx_ta_request_work_packet work_packet;
	struct drm_psb_sgx_lab_submit_op op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	u32 submit_id;
	u32 requested_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 op_kind;
	u32 owner_state;
	u32 packet_count;
};

struct psb_sgx_lab_submit_sgx_ta_request_work_entry {
	struct psb_sgx_lab_submit_sgx_ta_request_execution execution;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	bool packet_active;
	bool refilled_from_complete;
	bool refresh_views;
	bool increment_issue_count;
	bool increment_transition_count;
	bool update_last_epoch_jiffies;
	bool update_last_issue_jiffies;
	bool update_last_transition_jiffies;
	bool issue_advanced;
	bool running_advanced;
	bool complete_advanced;
	bool active_running_changed;
	bool selected_wrapped;
	u32 submit_id;
	u32 requested_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 op_kind;
	u32 owner_state;
	u32 packet_count;
	u32 selected_read_offset;
	u32 selected_write_offset;
	u32 selected_ccb_offset;
	u32 selected_outstanding_packets;
	u32 selected_seq;
	u32 selected_cookie;
	u32 epoch;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 next_issue_count;
	u32 next_transition_count;
	struct psb_sgx_lab_submit_sgx_scheduler_state next_state;
};

struct psb_sgx_lab_submit_sgx_ta_request_work_result {
	struct psb_sgx_lab_submit_sgx_ta_request_work_entry work_entry;
	bool active;
	bool issue_request;
	bool transition_request;
	u32 submit_id;
	u32 resolved_submit_id;
	u32 owner_state;
	u32 packet_count;
};

struct psb_sgx_lab_submit_sgx_ta_request_work_record {
	struct psb_sgx_lab_submit_sgx_ta_request_work_result work_result;
	bool active;
	bool issue_request;
	bool transition_request;
	bool refresh_views;
	bool refilled_from_complete;
	bool issue_advanced;
	bool running_advanced;
	bool complete_advanced;
	bool active_running_changed;
	bool selected_wrapped;
	u32 submit_id;
	u32 resolved_submit_id;
	u32 owner_state;
	u32 packet_count;
	u32 selected_read_offset;
	u32 selected_write_offset;
	u32 selected_ccb_offset;
	u32 selected_outstanding_packets;
	u32 selected_seq;
	u32 selected_cookie;
	u32 epoch;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 next_issue_count;
	u32 next_transition_count;
	struct psb_sgx_lab_submit_sgx_scheduler_state next_state;
};

struct psb_sgx_lab_submit_sgx_ta_request_work_request {
	struct psb_sgx_lab_submit_sgx_ta_request_work_record work_record;
	struct drm_psb_sgx_lab_submit_op op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	u32 submit_id;
	u32 requested_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 op_kind;
	u32 owner_state;
	u32 packet_count;
};

struct psb_sgx_lab_submit_sgx_ta_request_work_operation {
	struct psb_sgx_lab_submit_sgx_ta_request_work_request work_request;
	struct drm_psb_sgx_lab_submit_op op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	bool refresh_views;
	bool refilled_from_complete;
	bool issue_advanced;
	bool running_advanced;
	bool complete_advanced;
	bool active_running_changed;
	bool selected_wrapped;
	u32 submit_id;
	u32 requested_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 op_kind;
	u32 owner_state;
	u32 packet_count;
	u32 selected_read_offset;
	u32 selected_write_offset;
	u32 selected_ccb_offset;
	u32 selected_outstanding_packets;
	u32 selected_seq;
	u32 selected_cookie;
	u32 epoch;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 next_issue_count;
	u32 next_transition_count;
	struct psb_sgx_lab_submit_sgx_scheduler_state next_state;
};

struct psb_sgx_lab_submit_sgx_ta_request_work_event {
	struct psb_sgx_lab_submit_sgx_ta_request_work_operation work_operation;
	bool active;
	bool issue_request;
	bool transition_request;
	bool refresh_views;
	bool refilled_from_complete;
	bool issue_advanced;
	bool running_advanced;
	bool complete_advanced;
	bool active_running_changed;
	bool selected_wrapped;
	u32 submit_id;
	u32 resolved_submit_id;
	u32 owner_state;
	u32 packet_count;
	u32 selected_read_offset;
	u32 selected_write_offset;
	u32 selected_ccb_offset;
	u32 selected_outstanding_packets;
	u32 selected_seq;
	u32 selected_cookie;
	u32 epoch;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 next_issue_count;
	u32 next_transition_count;
	struct psb_sgx_lab_submit_sgx_scheduler_state next_state;
};

struct psb_sgx_lab_submit_sgx_ta_request_work_state {
	struct psb_sgx_lab_submit_sgx_ta_request_work_event work_event;
	bool active;
	bool issue_request;
	bool transition_request;
	bool refresh_views;
	bool refilled_from_complete;
	bool issue_advanced;
	bool running_advanced;
	bool complete_advanced;
	bool active_running_changed;
	bool selected_wrapped;
	u32 submit_id;
	u32 resolved_submit_id;
	u32 owner_state;
	u32 packet_count;
	u32 selected_read_offset;
	u32 selected_write_offset;
	u32 selected_ccb_offset;
	u32 selected_outstanding_packets;
	u32 selected_seq;
	u32 selected_cookie;
	u32 epoch;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 next_issue_count;
	u32 next_transition_count;
	struct psb_sgx_lab_submit_sgx_scheduler_state next_state;
};

struct psb_sgx_lab_submit_sgx_ta_request_work_action {
	struct psb_sgx_lab_submit_sgx_ta_request_work_state work_state;
	struct drm_psb_sgx_lab_submit_op op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	u32 submit_id;
	u32 requested_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 op_kind;
};

struct psb_sgx_lab_submit_sgx_ta_request_work_task {
	struct psb_sgx_lab_submit_sgx_ta_request_work_action work_action;
	bool active;
	bool issue_request;
	bool transition_request;
	bool refresh_views;
	bool refilled_from_complete;
	bool issue_advanced;
	bool running_advanced;
	bool complete_advanced;
	bool active_running_changed;
	bool selected_wrapped;
	u32 submit_id;
	u32 resolved_submit_id;
	u32 owner_state;
	u32 packet_count;
	u32 selected_read_offset;
	u32 selected_write_offset;
	u32 selected_ccb_offset;
	u32 selected_outstanding_packets;
	u32 selected_seq;
	u32 selected_cookie;
	u32 epoch;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 next_issue_count;
	u32 next_transition_count;
	struct psb_sgx_lab_submit_sgx_scheduler_state next_state;
};

struct psb_sgx_lab_submit_sgx_ta_request_work_run {
	struct psb_sgx_lab_submit_sgx_ta_request_work_task work_task;
	struct drm_psb_sgx_lab_submit_op op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	bool refresh_views;
	bool refilled_from_complete;
	bool issue_advanced;
	bool running_advanced;
	bool complete_advanced;
	bool active_running_changed;
	bool selected_wrapped;
	u32 submit_id;
	u32 requested_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 op_kind;
	u32 owner_state;
	u32 packet_count;
	u32 selected_read_offset;
	u32 selected_write_offset;
	u32 selected_ccb_offset;
	u32 selected_outstanding_packets;
	u32 selected_seq;
	u32 selected_cookie;
	u32 epoch;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 next_issue_count;
	u32 next_transition_count;
	struct psb_sgx_lab_submit_sgx_scheduler_state next_state;
};

struct psb_sgx_lab_submit_sgx_ta_request_work_invocation {
	struct psb_sgx_lab_submit_sgx_ta_request_work_run work_run;
	struct drm_psb_sgx_lab_submit_op op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	u32 submit_id;
	u32 requested_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 op_kind;
};

struct psb_sgx_lab_submit_sgx_ta_request_work_submit_op {
	struct psb_sgx_lab_submit_sgx_ta_request_work_invocation work_invocation;
	struct drm_psb_sgx_lab_submit_op op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_valid;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	u32 submit_id;
	u32 requested_submit_id;
	u32 selected_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 op_kind;
};

struct psb_sgx_lab_submit_sgx_ta_request_work_outcome {
	struct psb_sgx_lab_submit_sgx_ta_request_work_submit_op submit_op;
	struct drm_psb_sgx_lab_submit_op op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_valid;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	bool refresh_views;
	bool refilled_from_complete;
	bool issue_advanced;
	bool running_advanced;
	bool complete_advanced;
	bool active_running_changed;
	bool selected_wrapped;
	u32 submit_id;
	u32 requested_submit_id;
	u32 selected_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 op_kind;
	u32 owner_state;
	u32 packet_count;
	u32 selected_read_offset;
	u32 selected_write_offset;
	u32 selected_ccb_offset;
	u32 selected_outstanding_packets;
	u32 selected_seq;
	u32 selected_cookie;
	u32 epoch;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 next_issue_count;
	u32 next_transition_count;
	struct psb_sgx_lab_submit_sgx_scheduler_state next_state;
};

struct psb_sgx_lab_submit_sgx_ta_request_work_response {
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	struct drm_psb_sgx_lab_submit_op op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_valid;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	u32 submit_id;
	u32 requested_submit_id;
	u32 selected_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 op_kind;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_reply {
	struct psb_sgx_lab_submit_sgx_ta_request_work_response work_response;
	struct drm_psb_sgx_lab_submit_op response_op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_valid;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	u32 submit_id;
	u32 requested_submit_id;
	u32 selected_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 op_kind;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_control {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_reply submit_op_reply;
	struct drm_psb_sgx_lab_submit_op ioctl_op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_valid;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	u32 submit_id;
	u32 requested_submit_id;
	u32 selected_submit_id;
	u32 resolved_submit_id;
	u32 op_kind;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_ioctl {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_control submit_op_control;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_valid;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	u32 submit_id;
	u32 requested_submit_id;
	u32 selected_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 op_kind;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_entry {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_ioctl submit_op_ioctl;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_valid;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	bool issue_entry;
	bool transition_entry;
	u32 submit_id;
	u32 requested_submit_id;
	u32 selected_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 entry_op;
	u32 op_kind;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_call {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_entry submit_op_entry;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_valid;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	bool issue_entry;
	bool transition_entry;
	bool issue_call;
	bool transition_call;
	u32 submit_id;
	u32 requested_submit_id;
	u32 selected_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 entry_op;
	u32 call_op;
	u32 op_kind;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_entrypoint {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_call submit_op_call;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_valid;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	bool issue_entry;
	bool transition_entry;
	bool issue_call;
	bool transition_call;
	bool issue_entrypoint;
	bool transition_entrypoint;
	u32 submit_id;
	u32 requested_submit_id;
	u32 selected_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 entry_op;
	u32 call_op;
	u32 entrypoint_op;
	u32 op_kind;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_route {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_entrypoint submit_op_entrypoint;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_valid;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	bool issue_entry;
	bool transition_entry;
	bool issue_call;
	bool transition_call;
	bool issue_entrypoint;
	bool transition_entrypoint;
	bool issue_route;
	bool transition_route;
	u32 submit_id;
	u32 requested_submit_id;
	u32 selected_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 entry_op;
	u32 call_op;
	u32 entrypoint_op;
	u32 route_op;
	u32 op_kind;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_result {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_route submit_op_route;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool use_focus_request;
	bool focus_valid;
	bool focus_resolved;
	bool issue_request;
	bool transition_request;
	bool issue_entry;
	bool transition_entry;
	bool issue_call;
	bool transition_call;
	bool issue_entrypoint;
	bool transition_entrypoint;
	bool issue_route;
	bool transition_route;
	bool issue_result;
	bool transition_result;
	bool refresh_views;
	bool refilled_from_complete;
	bool issue_advanced;
	bool running_advanced;
	bool complete_advanced;
	bool active_running_changed;
	bool selected_wrapped;
	u32 submit_id;
	u32 requested_submit_id;
	u32 selected_submit_id;
	u32 resolved_submit_id;
	u32 ioctl_op;
	u32 entry_op;
	u32 call_op;
	u32 entrypoint_op;
	u32 route_op;
	u32 result_op;
	u32 op_kind;
	u32 owner_state;
	u32 packet_count;
	u32 selected_read_offset;
	u32 selected_write_offset;
	u32 selected_ccb_offset;
	u32 selected_outstanding_packets;
	u32 selected_seq;
	u32 selected_cookie;
	u32 epoch;
	u32 global_issue_ticket;
	u32 global_running_ticket;
	u32 global_complete_ticket;
	u32 next_issue_count;
	u32 next_transition_count;
	struct psb_sgx_lab_submit_sgx_scheduler_state next_state;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_dispatch {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_result submit_op_result;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_dispatch;
	bool transition_dispatch;
	u32 resolved_submit_id;
	u32 dispatch_op;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_handler {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_dispatch submit_op_dispatch;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_handler;
	bool transition_handler;
	u32 resolved_submit_id;
	u32 handler_op;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request_handler {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_handler submit_op_handler;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_request;
	bool transition_request;
	u32 resolved_submit_id;
	u32 request_op_kind;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request_wrapper {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_handler
		submit_op_request_handler;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_wrapper;
	bool transition_wrapper;
	u32 resolved_submit_id;
	u32 wrapper_op;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request_entry {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_wrapper
		submit_op_request_wrapper;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_entry;
	bool transition_entry;
	u32 resolved_submit_id;
	u32 entry_op;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request_call {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_entry
		submit_op_request_entry;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_call;
	bool transition_call;
	u32 resolved_submit_id;
	u32 call_op;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request_dispatch {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_call
		submit_op_request_call;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_request_dispatch;
	bool transition_request_dispatch;
	u32 resolved_submit_id;
	u32 request_dispatch_op;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request_result {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_dispatch
		submit_op_request_dispatch;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_request_result;
	bool transition_request_result;
	u32 resolved_submit_id;
	u32 request_result_op;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request_response {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_result
		submit_op_request_result;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_request_response;
	bool transition_request_response;
	u32 resolved_submit_id;
	u32 request_response_op;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request_return {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_response
		submit_op_request_response;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_request_return;
	bool transition_request_return;
	u32 resolved_submit_id;
	u32 request_return_op;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request_resolution {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_return
		submit_op_request_return;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_request_resolution;
	bool transition_request_resolution;
	u32 resolved_submit_id;
	u32 request_resolution_op;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request_operation {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_resolution
		submit_op_request_resolution;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_request_operation;
	bool transition_request_operation;
	u32 resolved_submit_id;
	u32 request_operation_op;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request_control {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_operation
		submit_op_request_operation;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_request_control;
	bool transition_request_control;
	u32 resolved_submit_id;
	u32 request_control_op;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request_path {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_control
		submit_op_request_control;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_request_path;
	bool transition_request_path;
	u32 resolved_submit_id;
	u32 request_path_op;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request_report {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_path
		submit_op_request_path;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_request_report;
	bool transition_request_report;
	u32 resolved_submit_id;
	u32 request_report_op;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request_trace {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_report
		submit_op_request_report;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_request_trace;
	bool transition_request_trace;
	u32 resolved_submit_id;
	u32 request_trace_op;
};

struct psb_sgx_lab_submit_sgx_ta_submit_op_request_frontend {
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_trace
		submit_op_request_trace;
	struct drm_psb_sgx_lab_submit_op request_op;
	struct drm_psb_sgx_lab_submit_op response_op;
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome outcome;
	bool active;
	bool dependency_fields_present;
	bool issue_request_frontend;
	bool transition_request_frontend;
	u32 resolved_submit_id;
	u32 request_frontend_op;
};

static u32 poulsbo_sgx_lab_status_value(u32 seed, bool is_3d, u32 index);
static int poulsbo_sgx_queue_region_end(u32 base_offset, u32 queue_size,
					u32 *end_out);
static int poulsbo_sgx_sync_region_end(u32 base_offset, u32 *end_out);
static int poulsbo_sgx_owner_region_end(u32 base_offset, u32 *end_out);
static int poulsbo_sgx_fill_submit_ccb_record_words_from_packet(
	const struct psb_sgx_lab_submit_exec_packet *packet,
	struct psb_sgx_lab_ccb_record_words *ccb_record);
static int poulsbo_sgx_write_queue_header_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 header_offset,
	const struct psb_sgx_lab_queue_header_words *queue_header);
static int poulsbo_sgx_write_submit_ccb_record_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 ccb_offset,
	const struct psb_sgx_lab_ccb_record_words *ccb_record);
static int poulsbo_sgx_write_submit_sync_locked(
	struct psb_sgx_lab_object *status_obj,
	u32 sync_offset,
	const struct psb_sgx_lab_sync_words *sync);
static int poulsbo_sgx_write_submit_owner_locked(
	struct psb_sgx_lab_object *status_obj,
	u32 owner_offset,
	const struct psb_sgx_lab_owner_words *owner);
static int poulsbo_sgx_build_submit_live_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_live_words *live);
static int poulsbo_sgx_build_submit_manifest_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_manifest_words *manifest);
static int poulsbo_sgx_build_submit_descriptor_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_descriptor_words *descriptor);
static int poulsbo_sgx_build_submit_plan_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_plan_words *plan);
static int poulsbo_sgx_build_submit_issue_plan_view_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_issue_plan_words *issue_plan);
static u32 *poulsbo_sgx_dependency_submit_id_ptr(
	struct psb_sgx_lab_submit *submit, unsigned int slot);
static u32 *poulsbo_sgx_dependency_snapshot_flags_ptr(
	struct psb_sgx_lab_submit *submit, unsigned int slot);
static u32 poulsbo_sgx_dependency_required_ticket(
	struct psb_sgx_lab_submit *submit, unsigned int slot);
static int poulsbo_sgx_dependency_observed_ticket_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	unsigned int slot,
	u32 *observed_complete_ticket_out,
	bool *source_missing_out);
static bool poulsbo_sgx_submit_emit_matches_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit);
static bool poulsbo_sgx_submit_launch_base_matches_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit);
static bool poulsbo_sgx_submit_issue_plan_matches_launch_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_issue_plan *plan);
static void
poulsbo_sgx_disarm_submit_locked(struct psb_sgx_lab_submit *submit);
static void
poulsbo_sgx_record_dispatch_submit_locked(struct psb_sgx_lab_submit *submit);
static void
poulsbo_sgx_retire_submit_locked(struct psb_sgx_lab_submit *submit);
static void
poulsbo_sgx_update_dispatch_submit_locked(struct psb_sgx_lab_submit *submit,
					  u32 owner_state,
					  u32 completed_packets);
static int poulsbo_sgx_refresh_submit_views_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit);
static int poulsbo_sgx_validate_submit_sgx_ta_ioctl_op_request(
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	u32 op_kind);
static int poulsbo_sgx_validate_built_submit_sgx_ta_request_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request *request);
static int poulsbo_sgx_validate_submit_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit);
static int poulsbo_sgx_refresh_dependency_dependents_locked(
	struct drm_psb_private *dev_priv,
	u32 completed_submit_id);

static const char *poulsbo_sgx_lab_role_name(u32 role)
{
	switch (role) {
	case PSB_SGX_LAB_ROLE_PDS:
		return "pds";
	case PSB_SGX_LAB_ROLE_USE:
		return "use";
	case PSB_SGX_LAB_ROLE_CONTROL:
		return "control";
	case PSB_SGX_LAB_ROLE_TARGET:
		return "target";
	case PSB_SGX_LAB_ROLE_STATUS:
		return "status";
	case PSB_SGX_LAB_ROLE_GENERIC:
	default:
		return "generic";
	}
}

static const char *poulsbo_sgx_lab_context_name(u32 context)
{
	switch (context) {
	case PSB_SGX_LAB_CONTEXT_DEFAULT:
		return "default";
	case PSB_SGX_LAB_CONTEXT_PF:
		return "pf";
	case PSB_SGX_LAB_CONTEXT_AUTO:
	default:
		return "auto";
	}
}

static const char *poulsbo_sgx_lab_class_name(u32 object_class)
{
	switch (object_class) {
	case PSB_SGX_LAB_CLASS_PACKET:
		return "packet";
	case PSB_SGX_LAB_CLASS_CODE:
		return "code";
	case PSB_SGX_LAB_CLASS_STATE:
		return "state";
	case PSB_SGX_LAB_CLASS_SURFACE:
		return "surface";
	case PSB_SGX_LAB_CLASS_GENERIC:
	default:
		return "generic";
	}
}

static const char *poulsbo_sgx_lab_gpu_use_name(u32 gpu_use)
{
	switch (gpu_use) {
	case PSB_SGX_LAB_GPU_USE_CONTROL_CCB:
		return "control_ccb";
	case PSB_SGX_LAB_GPU_USE_PDS_CODE:
		return "pds_code";
	case PSB_SGX_LAB_GPU_USE_USE_CODE:
		return "use_code";
	case PSB_SGX_LAB_GPU_USE_RENDER_TARGET:
		return "render_target";
	case PSB_SGX_LAB_GPU_USE_STATUS_BLOCK:
		return "status_block";
	case PSB_SGX_LAB_GPU_USE_GENERIC:
	default:
		return "generic";
	}
}

static u32 poulsbo_sgx_lab_range_type(const struct gtt_range *gt)
{
	const char *name = gt->resource.name;

	if (!name)
		return DRM_PSB_SGX_LAB_RANGE_UNKNOWN;
	if (!strcmp(name, "gem"))
		return DRM_PSB_SGX_LAB_RANGE_GEM;
	if (!strcmp(name, "fb"))
		return DRM_PSB_SGX_LAB_RANGE_FB;
	if (!strcmp(name, "cursor"))
		return DRM_PSB_SGX_LAB_RANGE_CURSOR;
	return DRM_PSB_SGX_LAB_RANGE_OTHER;
}

static bool poulsbo_sgx_range_matches_query_flags(const struct gtt_range *gt,
						  u32 flags)
{
	if ((flags & DRM_PSB_SGX_LAB_QUERY_RANGE_FILTER_GEM) &&
	    poulsbo_sgx_lab_range_type(gt) != DRM_PSB_SGX_LAB_RANGE_GEM)
		return false;

	return true;
}

static bool poulsbo_sgx_range_contains_gpu_va(struct drm_psb_private *dev_priv,
					      const struct gtt_range *gt,
					      u64 query_gpu_va,
					      u64 *offset_out)
{
	u64 range_gpu_va;
	u64 range_len;

	range_gpu_va = (u64)dev_priv->gtt.gatt_start + gt->offset;
	range_len = resource_size(&gt->resource);
	if (query_gpu_va < range_gpu_va)
		return false;
	if (query_gpu_va - range_gpu_va >= range_len)
		return false;
	if (offset_out)
		*offset_out = query_gpu_va - range_gpu_va;
	return true;
}

static const char *poulsbo_sgx_lab_owner_state_name(u32 owner_state)
{
	switch (owner_state) {
	case PSB_SGX_LAB_OWNER_QUEUED:
		return "queued";
	case PSB_SGX_LAB_OWNER_RUNNING:
		return "running";
	case PSB_SGX_LAB_OWNER_COMPLETE:
		return "complete";
	case PSB_SGX_LAB_OWNER_NONE:
	default:
		return "none";
	}
}

static const char *poulsbo_sgx_lab_report_stage_name(u32 report_stage)
{
	switch (report_stage) {
	case PSB_SGX_LAB_REPORT_STAGE_PREPARED:
		return "prepared";
	case PSB_SGX_LAB_REPORT_STAGE_ARMED:
		return "armed";
	case PSB_SGX_LAB_REPORT_STAGE_EMITTED:
		return "emitted";
	case PSB_SGX_LAB_REPORT_STAGE_LAUNCHED:
		return "launched";
	case PSB_SGX_LAB_REPORT_STAGE_DISPATCHED:
		return "dispatched";
	case PSB_SGX_LAB_REPORT_STAGE_RETIRED:
		return "retired";
	case PSB_SGX_LAB_REPORT_STAGE_NONE:
	default:
		return "none";
	}
}

static const char *poulsbo_sgx_lab_plan_source_name(u32 source)
{
	switch (source) {
	case DRM_PSB_SGX_LAB_PLAN_SOURCE_LIVE:
		return "live";
	case DRM_PSB_SGX_LAB_PLAN_SOURCE_LAUNCH:
		return "launch";
	case DRM_PSB_SGX_LAB_PLAN_SOURCE_DISPATCH:
		return "dispatch";
	case DRM_PSB_SGX_LAB_PLAN_SOURCE_RETIRED:
		return "retired";
	case DRM_PSB_SGX_LAB_PLAN_SOURCE_NONE:
	default:
		return "none";
	}
}

static const char *poulsbo_sgx_lab_result_source_name(u32 source)
{
	switch (source) {
	case DRM_PSB_SGX_LAB_RESULT_SOURCE_LIVE:
		return "live";
	case DRM_PSB_SGX_LAB_RESULT_SOURCE_DISPATCH:
		return "dispatch";
	case DRM_PSB_SGX_LAB_RESULT_SOURCE_RETIRED:
		return "retired";
	case DRM_PSB_SGX_LAB_RESULT_SOURCE_NONE:
	default:
		return "none";
	}
}

static const char *poulsbo_sgx_lab_cmd_type_name(u32 cmd_type)
{
	switch (cmd_type) {
	case PSB_SGX_LAB_CMD_TA:
		return "ta";
	case PSB_SGX_LAB_CMD_TRANSFER:
		return "transfer";
	case PSB_SGX_LAB_CMD_2D:
		return "2d";
	case PSB_SGX_LAB_CMD_POWER:
		return "power";
	case PSB_SGX_LAB_CMD_CLEANUP:
		return "cleanup";
	case PSB_SGX_LAB_CMD_GETMISCINFO:
		return "getmiscinfo";
	case PSB_SGX_LAB_CMD_PROCESS_QUEUES:
		return "process_queues";
	case PSB_SGX_LAB_CMD_INVALID:
	default:
		return "invalid";
	}
}

static int poulsbo_sgx_lab_parse_role(const char *name, u32 *role_out)
{
	if (sysfs_streq(name, "generic"))
		*role_out = PSB_SGX_LAB_ROLE_GENERIC;
	else if (sysfs_streq(name, "pds"))
		*role_out = PSB_SGX_LAB_ROLE_PDS;
	else if (sysfs_streq(name, "use"))
		*role_out = PSB_SGX_LAB_ROLE_USE;
	else if (sysfs_streq(name, "control"))
		*role_out = PSB_SGX_LAB_ROLE_CONTROL;
	else if (sysfs_streq(name, "target"))
		*role_out = PSB_SGX_LAB_ROLE_TARGET;
	else if (sysfs_streq(name, "status"))
		*role_out = PSB_SGX_LAB_ROLE_STATUS;
	else
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_lab_parse_class(const char *name, u32 *class_out)
{
	if (sysfs_streq(name, "generic"))
		*class_out = PSB_SGX_LAB_CLASS_GENERIC;
	else if (sysfs_streq(name, "packet"))
		*class_out = PSB_SGX_LAB_CLASS_PACKET;
	else if (sysfs_streq(name, "code"))
		*class_out = PSB_SGX_LAB_CLASS_CODE;
	else if (sysfs_streq(name, "state"))
		*class_out = PSB_SGX_LAB_CLASS_STATE;
	else if (sysfs_streq(name, "surface"))
		*class_out = PSB_SGX_LAB_CLASS_SURFACE;
	else
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_lab_parse_gpu_use(const char *name, u32 *gpu_use_out)
{
	if (sysfs_streq(name, "generic"))
		*gpu_use_out = PSB_SGX_LAB_GPU_USE_GENERIC;
	else if (sysfs_streq(name, "control_ccb"))
		*gpu_use_out = PSB_SGX_LAB_GPU_USE_CONTROL_CCB;
	else if (sysfs_streq(name, "pds_code"))
		*gpu_use_out = PSB_SGX_LAB_GPU_USE_PDS_CODE;
	else if (sysfs_streq(name, "use_code"))
		*gpu_use_out = PSB_SGX_LAB_GPU_USE_USE_CODE;
	else if (sysfs_streq(name, "render_target"))
		*gpu_use_out = PSB_SGX_LAB_GPU_USE_RENDER_TARGET;
	else if (sysfs_streq(name, "status_block"))
		*gpu_use_out = PSB_SGX_LAB_GPU_USE_STATUS_BLOCK;
	else
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_lab_parse_owner_state(const char *name,
					     u32 *owner_state_out)
{
	if (!name || !owner_state_out)
		return -EINVAL;

	if (sysfs_streq(name, "queued"))
		*owner_state_out = PSB_SGX_LAB_OWNER_QUEUED;
	else if (sysfs_streq(name, "running"))
		*owner_state_out = PSB_SGX_LAB_OWNER_RUNNING;
	else if (sysfs_streq(name, "complete"))
		*owner_state_out = PSB_SGX_LAB_OWNER_COMPLETE;
	else
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_lab_parse_cmd_type(const char *name, u32 *cmd_type_out)
{
	if (sysfs_streq(name, "ta"))
		*cmd_type_out = PSB_SGX_LAB_CMD_TA;
	else if (sysfs_streq(name, "transfer"))
		*cmd_type_out = PSB_SGX_LAB_CMD_TRANSFER;
	else if (sysfs_streq(name, "2d"))
		*cmd_type_out = PSB_SGX_LAB_CMD_2D;
	else if (sysfs_streq(name, "power"))
		*cmd_type_out = PSB_SGX_LAB_CMD_POWER;
	else if (sysfs_streq(name, "cleanup"))
		*cmd_type_out = PSB_SGX_LAB_CMD_CLEANUP;
	else if (sysfs_streq(name, "getmiscinfo"))
		*cmd_type_out = PSB_SGX_LAB_CMD_GETMISCINFO;
	else if (sysfs_streq(name, "process_queues"))
		*cmd_type_out = PSB_SGX_LAB_CMD_PROCESS_QUEUES;
	else
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_lab_parse_context(const char *name, u32 *context_out)
{
	if (sysfs_streq(name, "auto"))
		*context_out = PSB_SGX_LAB_CONTEXT_AUTO;
	else if (sysfs_streq(name, "default"))
		*context_out = PSB_SGX_LAB_CONTEXT_DEFAULT;
	else if (sysfs_streq(name, "pf"))
		*context_out = PSB_SGX_LAB_CONTEXT_PF;
	else
		return -EINVAL;

	return 0;
}

static struct psb_sgx_lab_object *
poulsbo_sgx_find_object_locked(struct drm_psb_private *dev_priv, u32 object_id)
{
	struct psb_sgx_lab_object *obj;

	list_for_each_entry(obj, &dev_priv->sgx_lab_object_list, head) {
		if (obj->object_id == object_id)
			return obj;
	}

	return NULL;
}

static struct psb_sgx_lab_submit *
poulsbo_sgx_find_submit_locked(struct drm_psb_private *dev_priv, u32 submit_id)
{
	struct psb_sgx_lab_submit *submit;

	list_for_each_entry(submit, &dev_priv->sgx_lab_submit_list, head) {
		if (submit->submit_id == submit_id)
			return submit;
	}

	return NULL;
}

static bool poulsbo_sgx_submit_references_object(struct psb_sgx_lab_submit *submit,
						 u32 object_id)
{
	return submit->control_object_id == object_id ||
	       submit->pds_object_id == object_id ||
	       submit->use_object_id == object_id ||
	       submit->target_object_id == object_id ||
	       submit->status_object_id == object_id;
}

static u32 poulsbo_sgx_submit_reference_slot(struct psb_sgx_lab_submit *submit,
					     u32 object_id)
{
	if (submit->control_object_id == object_id)
		return DRM_PSB_SGX_LAB_SUBMIT_SLOT_CONTROL;
	if (submit->pds_object_id == object_id)
		return DRM_PSB_SGX_LAB_SUBMIT_SLOT_PDS;
	if (submit->use_object_id == object_id)
		return DRM_PSB_SGX_LAB_SUBMIT_SLOT_USE;
	if (submit->target_object_id == object_id)
		return DRM_PSB_SGX_LAB_SUBMIT_SLOT_TARGET;
	if (submit->status_object_id == object_id)
		return DRM_PSB_SGX_LAB_SUBMIT_SLOT_STATUS;
	return 0;
}

static bool poulsbo_sgx_submit_resolve_gpu_va_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u64 query_gpu_va,
	u32 *slot_out,
	u32 *object_id_out,
	u32 *range_id_out,
	u32 *range_type_out,
	u64 *offset_out)
{
	static const u32 slot_ids[] = {
		DRM_PSB_SGX_LAB_SUBMIT_SLOT_CONTROL,
		DRM_PSB_SGX_LAB_SUBMIT_SLOT_PDS,
		DRM_PSB_SGX_LAB_SUBMIT_SLOT_USE,
		DRM_PSB_SGX_LAB_SUBMIT_SLOT_TARGET,
		DRM_PSB_SGX_LAB_SUBMIT_SLOT_STATUS,
	};
	u32 object_ids[ARRAY_SIZE(slot_ids)];
	size_t i;

	object_ids[0] = submit->control_object_id;
	object_ids[1] = submit->pds_object_id;
	object_ids[2] = submit->use_object_id;
	object_ids[3] = submit->target_object_id;
	object_ids[4] = submit->status_object_id;

	for (i = 0; i < ARRAY_SIZE(slot_ids); i++) {
		struct psb_sgx_lab_object *obj;
		u64 offset = 0;

		if (!object_ids[i])
			continue;

		obj = poulsbo_sgx_find_object_locked(dev_priv, object_ids[i]);
		if (!obj || !obj->gt)
			continue;

		if (!poulsbo_sgx_range_contains_gpu_va(dev_priv, obj->gt,
						      query_gpu_va, &offset))
			continue;

		if (slot_out)
			*slot_out = slot_ids[i];
		if (object_id_out)
			*object_id_out = object_ids[i];
		if (range_id_out)
			*range_id_out = obj->gt->debug_id;
		if (range_type_out)
			*range_type_out = poulsbo_sgx_lab_range_type(obj->gt);
		if (offset_out)
			*offset_out = offset;
		return true;
	}

	return false;
}

static void poulsbo_sgx_range_submit_refs_locked(
	struct drm_psb_private *dev_priv,
	struct gtt_range *gt,
	u32 *ref_count_out,
	u32 *first_submit_id_out,
	u32 *first_submit_slot_out,
	bool use_match_index,
	u32 query_match_index,
	u32 *resolved_submit_found_out,
	u32 *resolved_submit_id_out,
	u32 *resolved_submit_slot_out,
	u32 *resolved_submit_match_index_out)
{
	struct psb_sgx_lab_submit *submit;
	u32 object_id = gt->sgx_lab_object_id;
	u32 ref_count = 0;
	u32 first_submit_id = 0;
	u32 first_submit_slot = 0;
	u32 resolved_submit_found = 0;
	u32 resolved_submit_id = 0;
	u32 resolved_submit_slot = 0;
	u32 resolved_submit_match_index = 0;

	if (!object_id)
		goto out;

	list_for_each_entry(submit, &dev_priv->sgx_lab_submit_list, head) {
		u32 slot;

		if (!poulsbo_sgx_submit_references_object(submit, object_id))
			continue;

		slot = poulsbo_sgx_submit_reference_slot(submit, object_id);
		if (!first_submit_id) {
			first_submit_id = submit->submit_id;
			first_submit_slot = slot;
		}
		if (use_match_index && ref_count == query_match_index) {
			resolved_submit_found = 1;
			resolved_submit_id = submit->submit_id;
			resolved_submit_slot = slot;
			resolved_submit_match_index = ref_count;
		}
		ref_count++;
	}

out:
	if (ref_count_out)
		*ref_count_out = ref_count;
	if (first_submit_id_out)
		*first_submit_id_out = first_submit_id;
	if (first_submit_slot_out)
		*first_submit_slot_out = first_submit_slot;
	if (resolved_submit_found_out)
		*resolved_submit_found_out = resolved_submit_found;
	if (resolved_submit_id_out)
		*resolved_submit_id_out = resolved_submit_id;
	if (resolved_submit_slot_out)
		*resolved_submit_slot_out = resolved_submit_slot;
	if (resolved_submit_match_index_out)
		*resolved_submit_match_index_out = resolved_submit_match_index;
}

static int poulsbo_sgx_set_object_role(struct drm_device *dev, u32 object_id,
				       u32 role)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_object *obj;

	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	obj = poulsbo_sgx_find_object_locked(dev_priv, object_id);
	if (!obj) {
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		return -ENOENT;
	}

	obj->role = role;
	obj->last_role_jiffies = jiffies;
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);

	dev_info(dev->dev, "lab object role object_id=%u role=%s\n",
		 object_id, poulsbo_sgx_lab_role_name(role));
	return 0;
}

static int poulsbo_sgx_set_object_class(struct drm_device *dev, u32 object_id,
					u32 object_class)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_object *obj;

	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	obj = poulsbo_sgx_find_object_locked(dev_priv, object_id);
	if (!obj) {
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		return -ENOENT;
	}

	obj->object_class = object_class;
	obj->last_class_jiffies = jiffies;
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);

	dev_info(dev->dev, "lab object class object_id=%u class=%s\n",
		 object_id, poulsbo_sgx_lab_class_name(object_class));
	return 0;
}

static int poulsbo_sgx_set_object_gpu_use(struct drm_device *dev, u32 object_id,
					  u32 gpu_use)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_object *obj;

	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	obj = poulsbo_sgx_find_object_locked(dev_priv, object_id);
	if (!obj) {
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		return -ENOENT;
	}

	obj->gpu_use = gpu_use;
	obj->last_gpu_use_jiffies = jiffies;
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);

	dev_info(dev->dev, "lab object gpu_use object_id=%u gpu_use=%s\n",
		 object_id, poulsbo_sgx_lab_gpu_use_name(gpu_use));
	return 0;
}

static int poulsbo_sgx_set_object_context(struct drm_device *dev, u32 object_id,
					  u32 context)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_object *obj;

	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	obj = poulsbo_sgx_find_object_locked(dev_priv, object_id);
	if (!obj) {
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		return -ENOENT;
	}

	obj->expected_context = context;
	obj->last_context_jiffies = jiffies;
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);

	dev_info(dev->dev, "lab object context object_id=%u context=%s\n",
		 object_id, poulsbo_sgx_lab_context_name(context));
	return 0;
}

static unsigned long poulsbo_sgx_object_gpu_va(struct drm_psb_private *dev_priv,
					       struct psb_sgx_lab_object *obj)
{
	return dev_priv->gtt.gatt_start + obj->gt->offset;
}

static int poulsbo_sgx_object_access_ok(struct psb_sgx_lab_object *obj,
					u32 byte_offset, u32 byte_count)
{
	u64 end = (u64)byte_offset + (u64)byte_count;

	if (!obj->gt->pages || obj->gt->npage <= 0)
		return -EINVAL;
	if (end > obj->gt->gem.size)
		return -EINVAL;
	return 0;
}

static int poulsbo_sgx_object_write_u32_locked(struct psb_sgx_lab_object *obj,
					       u32 byte_offset, u32 value)
{
	struct page *page;
	void *map;
	u32 page_index;
	u32 in_page;
	int ret;

	ret = poulsbo_sgx_object_access_ok(obj, byte_offset, sizeof(value));
	if (ret)
		return ret;

	page_index = byte_offset >> PAGE_SHIFT;
	in_page = byte_offset & (PAGE_SIZE - 1);
	page = obj->gt->pages[page_index];
	map = kmap(page);
	*(u32 *)((u8 *)map + in_page) = value;
	kunmap(page);
	return 0;
}

static int poulsbo_sgx_object_read_u32_locked(struct psb_sgx_lab_object *obj,
					      u32 byte_offset, u32 *value_out)
{
	struct page *page;
	void *map;
	u32 page_index;
	u32 in_page;
	int ret;

	ret = poulsbo_sgx_object_access_ok(obj, byte_offset, sizeof(*value_out));
	if (ret)
		return ret;

	page_index = byte_offset >> PAGE_SHIFT;
	in_page = byte_offset & (PAGE_SIZE - 1);
	page = obj->gt->pages[page_index];
	map = kmap(page);
	*value_out = *(u32 *)((u8 *)map + in_page);
	kunmap(page);
	return 0;
}

static u32 *poulsbo_sgx_submit_slot_ptr(struct psb_sgx_lab_submit *submit,
					const char *slot_name, u32 *role_out)
{
	if (sysfs_streq(slot_name, "control")) {
		if (role_out)
			*role_out = PSB_SGX_LAB_ROLE_CONTROL;
		return &submit->control_object_id;
	}
	if (sysfs_streq(slot_name, "pds")) {
		if (role_out)
			*role_out = PSB_SGX_LAB_ROLE_PDS;
		return &submit->pds_object_id;
	}
	if (sysfs_streq(slot_name, "use")) {
		if (role_out)
			*role_out = PSB_SGX_LAB_ROLE_USE;
		return &submit->use_object_id;
	}
	if (sysfs_streq(slot_name, "target")) {
		if (role_out)
			*role_out = PSB_SGX_LAB_ROLE_TARGET;
		return &submit->target_object_id;
	}
	if (sysfs_streq(slot_name, "status")) {
		if (role_out)
			*role_out = PSB_SGX_LAB_ROLE_STATUS;
		return &submit->status_object_id;
	}
	return NULL;
}

static void
poulsbo_sgx_unlaunch_submit_locked(struct psb_sgx_lab_submit *submit)
{
	submit->launch_valid = false;
	submit->launch_consumed = false;
	submit->launch_prepare_count = 0;
	submit->launch_arm_count = 0;
	submit->launch_emit_count = 0;
	submit->launch_context = PSB_SGX_LAB_CONTEXT_AUTO;
	submit->launch_cmd_type = PSB_SGX_LAB_CMD_INVALID;
	submit->launch_owner_state = PSB_SGX_LAB_OWNER_NONE;
	submit->launch_issued_packets = 0;
	submit->launch_refilled_from_complete = 0;
	submit->launch_ctrl_flags = 0;
	submit->launch_num_ta_status_vals = 0;
	submit->launch_num_3d_status_vals = 0;
	submit->launch_ta_dependency = 0;
	submit->launch_control_object_id = 0;
	submit->launch_pds_object_id = 0;
	submit->launch_use_object_id = 0;
	submit->launch_target_object_id = 0;
	submit->launch_status_object_id = 0;
	submit->launch_ccb_offset = 0;
	submit->launch_ccb_write_offset = 0;
	submit->launch_ccb_read_offset = 0;
	submit->launch_new_write_offset = 0;
	submit->launch_ccb_service_address = 0;
	submit->launch_ccb_cache_control = 0;
	submit->launch_ccb_data0 = 0;
	submit->launch_ccb_data1 = 0;
	submit->launch_queue_base_offset = 0;
	submit->launch_queue_size = 0;
	submit->launch_queue_packet_size = 0;
	submit->launch_queue_packet_count = 0;
	submit->launch_queue_tail_packet_count = 0;
	submit->launch_queue_head_packet_count = 0;
	submit->launch_sync_offset = 0;
	submit->launch_sync_complete_seq = 0;
	submit->launch_sync_pending_seq = 0;
	submit->launch_sync_flags = 0;
	submit->launch_owner_offset = 0;
	submit->launch_owner_issue_cookie = 0;
	submit->launch_owner_complete_cookie = 0;
	submit->launch_queue_epoch = 0;
	submit->launch_global_issue_ticket = 0;
	submit->launch_global_running_ticket = 0;
	submit->launch_global_complete_ticket = 0;
	submit->launch_dependency_active_count = 0;
	submit->launch_control_gpu_va = 0;
	submit->launch_pds_gpu_va = 0;
	submit->launch_use_gpu_va = 0;
	submit->launch_target_gpu_va = 0;
	submit->launch_status_gpu_va = 0;
}

static void
poulsbo_sgx_unemit_submit_locked(struct psb_sgx_lab_submit *submit)
{
	submit->emit_valid = false;
	submit->emit_prepare_count = 0;
	submit->emit_arm_count = 0;
	submit->emit_context = PSB_SGX_LAB_CONTEXT_AUTO;
	submit->emit_cmd_type = PSB_SGX_LAB_CMD_INVALID;
	submit->emit_owner_state = PSB_SGX_LAB_OWNER_NONE;
	submit->emit_ctrl_flags = 0;
	submit->emit_num_ta_status_vals = 0;
	submit->emit_num_3d_status_vals = 0;
	submit->emit_ta_dependency = 0;
	submit->emit_control_object_id = 0;
	submit->emit_pds_object_id = 0;
	submit->emit_use_object_id = 0;
	submit->emit_target_object_id = 0;
	submit->emit_status_object_id = 0;
	submit->emit_ccb_offset = 0;
	submit->emit_ccb_write_offset = 0;
	submit->emit_ccb_read_offset = 0;
	submit->emit_ccb_service_address = 0;
	submit->emit_ccb_cache_control = 0;
	submit->emit_ccb_data0 = 0;
	submit->emit_ccb_data1 = 0;
	submit->emit_queue_base_offset = 0;
	submit->emit_queue_size = 0;
	submit->emit_queue_packet_size = 0;
	submit->emit_queue_packet_count = 0;
	submit->emit_queue_tail_packet_count = 0;
	submit->emit_queue_head_packet_count = 0;
	submit->emit_sync_complete_seq = 0;
	submit->emit_sync_pending_seq = 0;
	submit->emit_sync_flags = 0;
	submit->emit_owner_issue_cookie = 0;
	submit->emit_owner_complete_cookie = 0;
	submit->emit_queue_epoch = 0;
	submit->emit_global_issue_ticket = 0;
	submit->emit_dependency_active_count = 0;
	submit->emit_control_gpu_va = 0;
	submit->emit_pds_gpu_va = 0;
	submit->emit_use_gpu_va = 0;
	submit->emit_target_gpu_va = 0;
	submit->emit_status_gpu_va = 0;
}

static bool poulsbo_sgx_dependency_slot_active(struct psb_sgx_lab_submit *submit,
					       unsigned int slot);
static int poulsbo_sgx_queue_progression(u32 read_offset, u32 queue_size,
					 u32 queue_packet_size,
					 u32 queue_packet_count,
					 u32 *tail_packets_out,
					 u32 *head_packets_out,
					 u32 *write_offset_out);

static void
poulsbo_sgx_clear_preissue_submit_locked(struct psb_sgx_lab_submit *submit)
{
	poulsbo_sgx_unemit_submit_locked(submit);
	submit->arm_valid = false;
	submit->arm_prepare_count = 0;
	submit->arm_queue_packet_count = 0;
	submit->arm_queue_tail_packet_count = 0;
	submit->arm_queue_head_packet_count = 0;
	submit->arm_ccb_read_offset = 0;
	submit->arm_ccb_write_offset = 0;
	submit->arm_sync_complete_seq = 0;
	submit->arm_sync_pending_seq = 0;
	submit->arm_owner_state = PSB_SGX_LAB_OWNER_NONE;
	submit->arm_owner_issue_cookie = 0;
	submit->arm_owner_complete_cookie = 0;
	submit->arm_queue_epoch = 0;
	submit->arm_global_issue_ticket = 0;
}

static bool
poulsbo_sgx_build_issue_plan_locked(struct drm_psb_private *dev_priv,
				    struct psb_sgx_lab_submit *submit,
				    u32 owner_state, u32 issued_packets,
				    struct psb_sgx_lab_issue_plan *plan)
{
	u32 queue_capacity;
	u32 new_sync_flags;
	u32 new_queue_epoch;
	u32 new_packet_count;
	u32 new_tail_packets;
	u32 new_head_packets;
	u32 new_write_offset;
	u32 new_sync_pending_seq;
	u32 new_owner_issue_cookie;
	u32 new_global_issue_ticket;
	bool refilled_from_complete;
	int ret;

	if (!submit->control_object_id || !submit->status_object_id ||
	    !submit->queue_config_count || !submit->ccb_config_count ||
	    !submit->sync_config_count || !submit->owner_config_count)
		return false;
	if (owner_state != PSB_SGX_LAB_OWNER_QUEUED &&
	    owner_state != PSB_SGX_LAB_OWNER_RUNNING)
		return false;
	if (!issued_packets)
		return false;

	queue_capacity = submit->queue_size / submit->queue_packet_size;
	if (!queue_capacity ||
	    issued_packets > queue_capacity - submit->queue_packet_count)
		return false;

	if (submit->owner_state == PSB_SGX_LAB_OWNER_COMPLETE) {
		if (submit->queue_packet_count || owner_state !=
		    PSB_SGX_LAB_OWNER_QUEUED)
			return false;
	} else if (submit->owner_state == PSB_SGX_LAB_OWNER_QUEUED) {
		if (owner_state != PSB_SGX_LAB_OWNER_QUEUED)
			return false;
	} else if (submit->owner_state == PSB_SGX_LAB_OWNER_RUNNING) {
		if (owner_state != PSB_SGX_LAB_OWNER_RUNNING)
			return false;
		if (!dev_priv->sgx_lab_running_active ||
		    dev_priv->sgx_lab_active_running_submit_id !=
			    submit->submit_id)
			return false;
	} else {
		return false;
	}

	if (U32_MAX - submit->sync_pending_seq < issued_packets ||
	    U32_MAX - submit->owner_issue_cookie < issued_packets)
		return false;
	if (dev_priv->sgx_lab_global_issue_ticket == U32_MAX)
		return false;

	refilled_from_complete =
		submit->owner_state == PSB_SGX_LAB_OWNER_COMPLETE &&
		!submit->queue_packet_count;
	new_queue_epoch = submit->queue_epoch;
	if (refilled_from_complete) {
		if (new_queue_epoch == U32_MAX)
			return false;
		new_queue_epoch++;
	}

	new_packet_count = submit->queue_packet_count + issued_packets;
	ret = poulsbo_sgx_queue_progression(submit->ccb_read_offset,
					    submit->queue_size,
					    submit->queue_packet_size,
					    new_packet_count,
					    &new_tail_packets,
					    &new_head_packets,
					    &new_write_offset);
	if (ret)
		return false;

	new_sync_pending_seq = submit->sync_pending_seq + issued_packets;
	new_owner_issue_cookie = submit->owner_issue_cookie + issued_packets;
	new_global_issue_ticket = dev_priv->sgx_lab_global_issue_ticket + 1;
	new_sync_flags = PSB_SGX_LAB_SYNC_FLAG_READY;
	if (new_head_packets)
		new_sync_flags |= PSB_SGX_LAB_SYNC_FLAG_WRAPPED;

	plan->owner_state = owner_state;
	plan->issued_packets = issued_packets;
	plan->refilled_from_complete = refilled_from_complete;
	plan->new_queue_epoch = new_queue_epoch;
	plan->new_packet_count = new_packet_count;
	plan->new_tail_packets = new_tail_packets;
	plan->new_head_packets = new_head_packets;
	plan->new_write_offset = new_write_offset;
	plan->new_sync_pending_seq = new_sync_pending_seq;
	plan->new_owner_issue_cookie = new_owner_issue_cookie;
	plan->new_global_issue_ticket = new_global_issue_ticket;
	plan->new_sync_flags = new_sync_flags;
	return true;
}

static int poulsbo_sgx_resolve_submit_ccb_gpu_va32_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 *pds_gpu_va32,
	u32 *use_gpu_va32,
	u32 *target_gpu_va32,
	u32 *status_gpu_va32)
{
	struct psb_sgx_lab_object *obj;

	if (pds_gpu_va32)
		*pds_gpu_va32 = 0;
	if (use_gpu_va32)
		*use_gpu_va32 = 0;
	if (target_gpu_va32)
		*target_gpu_va32 = 0;
	if (status_gpu_va32)
		*status_gpu_va32 = 0;

	if (submit->pds_object_id) {
		obj = poulsbo_sgx_find_object_locked(dev_priv,
						     submit->pds_object_id);
		if (!obj)
			return -ENOENT;
		if (pds_gpu_va32)
			*pds_gpu_va32 =
				(u32)poulsbo_sgx_object_gpu_va(dev_priv, obj);
	}
	if (submit->use_object_id) {
		obj = poulsbo_sgx_find_object_locked(dev_priv,
						     submit->use_object_id);
		if (!obj)
			return -ENOENT;
		if (use_gpu_va32)
			*use_gpu_va32 =
				(u32)poulsbo_sgx_object_gpu_va(dev_priv, obj);
	}
	if (submit->target_object_id) {
		obj = poulsbo_sgx_find_object_locked(dev_priv,
						     submit->target_object_id);
		if (!obj)
			return -ENOENT;
		if (target_gpu_va32)
			*target_gpu_va32 =
				(u32)poulsbo_sgx_object_gpu_va(dev_priv, obj);
	}
	if (submit->status_object_id) {
		obj = poulsbo_sgx_find_object_locked(dev_priv,
						     submit->status_object_id);
		if (!obj)
			return -ENOENT;
		if (status_gpu_va32)
			*status_gpu_va32 =
				(u32)poulsbo_sgx_object_gpu_va(dev_priv, obj);
	}

	return 0;
}

static int poulsbo_sgx_resolve_object_gpu_va32_locked(
	struct drm_psb_private *dev_priv,
	u32 object_id,
	u32 *gpu_va32)
{
	struct psb_sgx_lab_object *obj;

	if (gpu_va32)
		*gpu_va32 = 0;
	if (!object_id)
		return 0;

	obj = poulsbo_sgx_find_object_locked(dev_priv, object_id);
	if (!obj)
		return -ENOENT;
	if (gpu_va32)
		*gpu_va32 = (u32)poulsbo_sgx_object_gpu_va(dev_priv, obj);
	return 0;
}

static enum psb_sgx_lab_exec_source
poulsbo_sgx_submit_plan_exec_source_locked(u32 stage)
{
	switch (stage) {
	case PSB_SGX_LAB_REPORT_STAGE_RETIRED:
		return PSB_SGX_LAB_EXEC_SOURCE_RETIRED;
	case PSB_SGX_LAB_REPORT_STAGE_DISPATCHED:
		return PSB_SGX_LAB_EXEC_SOURCE_DISPATCH;
	case PSB_SGX_LAB_REPORT_STAGE_LAUNCHED:
		return PSB_SGX_LAB_EXEC_SOURCE_LAUNCH;
	case PSB_SGX_LAB_REPORT_STAGE_EMITTED:
	case PSB_SGX_LAB_REPORT_STAGE_ARMED:
	case PSB_SGX_LAB_REPORT_STAGE_PREPARED:
	case PSB_SGX_LAB_REPORT_STAGE_NONE:
	default:
		return PSB_SGX_LAB_EXEC_SOURCE_LIVE;
	}
}

static enum psb_sgx_lab_exec_source
poulsbo_sgx_submit_result_exec_source_locked(u32 stage)
{
	switch (stage) {
	case PSB_SGX_LAB_REPORT_STAGE_RETIRED:
		return PSB_SGX_LAB_EXEC_SOURCE_RETIRED;
	case PSB_SGX_LAB_REPORT_STAGE_DISPATCHED:
		return PSB_SGX_LAB_EXEC_SOURCE_DISPATCH;
	case PSB_SGX_LAB_REPORT_STAGE_LAUNCHED:
	case PSB_SGX_LAB_REPORT_STAGE_EMITTED:
	case PSB_SGX_LAB_REPORT_STAGE_ARMED:
	case PSB_SGX_LAB_REPORT_STAGE_PREPARED:
	case PSB_SGX_LAB_REPORT_STAGE_NONE:
	default:
		return PSB_SGX_LAB_EXEC_SOURCE_LIVE;
	}
}

static u32 poulsbo_sgx_submit_plan_source_locked(
	enum psb_sgx_lab_exec_source source)
{
	switch (source) {
	case PSB_SGX_LAB_EXEC_SOURCE_RETIRED:
		return DRM_PSB_SGX_LAB_PLAN_SOURCE_RETIRED;
	case PSB_SGX_LAB_EXEC_SOURCE_DISPATCH:
		return DRM_PSB_SGX_LAB_PLAN_SOURCE_DISPATCH;
	case PSB_SGX_LAB_EXEC_SOURCE_LAUNCH:
		return DRM_PSB_SGX_LAB_PLAN_SOURCE_LAUNCH;
	case PSB_SGX_LAB_EXEC_SOURCE_LIVE:
	default:
		return DRM_PSB_SGX_LAB_PLAN_SOURCE_LIVE;
	}
}

static u32 poulsbo_sgx_submit_result_source_locked(
	enum psb_sgx_lab_exec_source source)
{
	switch (source) {
	case PSB_SGX_LAB_EXEC_SOURCE_RETIRED:
		return DRM_PSB_SGX_LAB_RESULT_SOURCE_RETIRED;
	case PSB_SGX_LAB_EXEC_SOURCE_DISPATCH:
		return DRM_PSB_SGX_LAB_RESULT_SOURCE_DISPATCH;
	case PSB_SGX_LAB_EXEC_SOURCE_LAUNCH:
	case PSB_SGX_LAB_EXEC_SOURCE_LIVE:
	default:
		return DRM_PSB_SGX_LAB_RESULT_SOURCE_LIVE;
	}
}

static u32 poulsbo_sgx_submit_report_stage_locked(
	struct psb_sgx_lab_submit *submit);
static u32 poulsbo_sgx_submit_report_stage_count_locked(
	struct psb_sgx_lab_submit *submit, u32 report_stage);
static u32 poulsbo_sgx_submit_report_completed_packets_locked(
	struct psb_sgx_lab_submit *submit, u32 report_stage);
static u32 poulsbo_sgx_submit_report_issued_packets_locked(
	struct psb_sgx_lab_submit *submit, u32 report_stage);
static u32 poulsbo_sgx_submit_report_dependency_count_locked(
	struct psb_sgx_lab_submit *submit, u32 report_stage);
static u32 poulsbo_sgx_submit_report_flags_locked(
	struct psb_sgx_lab_submit *submit);

static int poulsbo_sgx_build_submit_exec_record_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	enum psb_sgx_lab_exec_source source,
	struct psb_sgx_lab_submit_exec_record *record)
{
	u32 stage;
	int ret;

	if (!record)
		return -EINVAL;

	memset(record, 0, sizeof(*record));
	stage = poulsbo_sgx_submit_report_stage_locked(submit);
	record->stage = stage;
	record->stage_count =
		poulsbo_sgx_submit_report_stage_count_locked(submit, stage);
	record->source = source;
	record->report_flags = poulsbo_sgx_submit_report_flags_locked(submit);
	record->num_ta_status_vals = submit->num_ta_status_vals;
	record->num_3d_status_vals = submit->num_3d_status_vals;
	record->ta_dependency = submit->ta_dependency;
	record->status_ta_offset = submit->status_ta_offset;
	record->status_3d_offset = submit->status_3d_offset;
	record->status_stride = submit->status_stride;
	record->status_value_seed = submit->status_value_seed;

	switch (source) {
	case PSB_SGX_LAB_EXEC_SOURCE_LAUNCH:
		record->prepare_count = submit->launch_prepare_count;
		record->arm_count = submit->launch_arm_count;
		record->emit_count = submit->launch_emit_count;
		record->launch_count = submit->launch_count;
		record->dispatch_count = submit->dispatch_count;
		record->retired_count = submit->retired_count;
		record->context = submit->launch_context;
		record->cmd_type = submit->launch_cmd_type;
		record->owner_state = submit->launch_owner_state;
		record->issued_packets = submit->launch_issued_packets;
		record->completed_packets = 0;
		record->dependency_active_count =
			submit->launch_dependency_active_count;
		record->refilled_from_complete =
			submit->launch_refilled_from_complete;
		record->queue_packet_count = submit->launch_queue_packet_count;
		record->queue_epoch = submit->launch_queue_epoch;
		record->queue_base_offset = submit->launch_queue_base_offset;
		record->queue_size = submit->launch_queue_size;
		record->queue_packet_size = submit->launch_queue_packet_size;
		record->ccb_offset = submit->launch_ccb_offset;
		record->ccb_write_offset = submit->launch_new_write_offset;
		record->ccb_read_offset = submit->launch_ccb_read_offset;
		record->sync_offset = submit->launch_sync_offset;
		record->sync_complete_seq = submit->launch_sync_complete_seq;
		record->sync_pending_seq = submit->launch_sync_pending_seq;
		record->sync_flags = submit->launch_sync_flags;
		record->owner_offset = submit->launch_owner_offset;
		record->owner_issue_cookie =
			submit->launch_owner_issue_cookie;
		record->owner_complete_cookie =
			submit->launch_owner_complete_cookie;
		record->global_issue_ticket =
			submit->launch_global_issue_ticket;
		record->global_running_ticket =
			submit->launch_global_running_ticket;
		record->global_complete_ticket =
			submit->launch_global_complete_ticket;
		record->ctrl_flags = submit->launch_ctrl_flags;
		record->control_object_id = submit->launch_control_object_id;
		record->status_object_id = submit->launch_status_object_id;
		record->ccb_service_address =
			submit->launch_ccb_service_address;
		record->ccb_cache_control =
			submit->launch_ccb_cache_control;
		record->ccb_data0 = submit->launch_ccb_data0;
		record->ccb_data1 = submit->launch_ccb_data1;
		record->control_gpu_va32 = (u32)submit->launch_control_gpu_va;
		record->status_gpu_va32 = (u32)submit->launch_status_gpu_va;
		record->pds_gpu_va32 = (u32)submit->launch_pds_gpu_va;
		record->use_gpu_va32 = (u32)submit->launch_use_gpu_va;
		record->target_gpu_va32 = (u32)submit->launch_target_gpu_va;
		record->ccb_status_gpu_va32 =
			(u32)submit->launch_status_gpu_va;
		return 0;
	case PSB_SGX_LAB_EXEC_SOURCE_DISPATCH:
		record->prepare_count = submit->dispatch_prepare_count;
		record->arm_count = submit->dispatch_arm_count;
		record->emit_count = submit->dispatch_emit_count;
		record->launch_count = submit->dispatch_launch_count;
		record->dispatch_count = submit->dispatch_count;
		record->retired_count = submit->retired_count;
		record->context = submit->dispatch_context;
		record->cmd_type = submit->dispatch_cmd_type;
		record->owner_state = submit->dispatch_owner_state;
		record->issued_packets = submit->dispatch_issued_packets;
		record->completed_packets = submit->dispatch_completed_packets;
		record->dependency_active_count =
			submit->dispatch_dependency_active_count;
		record->refilled_from_complete =
			submit->dispatch_refilled_from_complete;
		record->queue_packet_count = submit->dispatch_queue_packet_count;
		record->queue_epoch = submit->dispatch_queue_epoch;
		record->queue_base_offset = submit->dispatch_queue_base_offset;
		record->queue_size = submit->dispatch_queue_size;
		record->queue_packet_size = submit->dispatch_queue_packet_size;
		record->ccb_offset = submit->dispatch_ccb_offset;
		record->ccb_write_offset = submit->dispatch_ccb_write_offset;
		record->ccb_read_offset = submit->dispatch_ccb_read_offset;
		record->sync_offset = submit->dispatch_sync_offset;
		record->sync_complete_seq = submit->dispatch_sync_complete_seq;
		record->sync_pending_seq = submit->dispatch_sync_pending_seq;
		record->sync_flags = submit->dispatch_sync_flags;
		record->owner_offset = submit->dispatch_owner_offset;
		record->owner_issue_cookie =
			submit->dispatch_owner_issue_cookie;
		record->owner_complete_cookie =
			submit->dispatch_owner_complete_cookie;
		record->global_issue_ticket =
			submit->dispatch_global_issue_ticket;
		record->global_running_ticket =
			submit->dispatch_global_running_ticket;
		record->global_complete_ticket =
			submit->dispatch_global_complete_ticket;
		record->ctrl_flags = submit->dispatch_ctrl_flags;
		record->control_object_id = submit->dispatch_control_object_id;
		record->status_object_id = submit->dispatch_status_object_id;
		record->ccb_service_address =
			submit->dispatch_ccb_service_address;
		record->ccb_cache_control =
			submit->dispatch_ccb_cache_control;
		record->ccb_data0 = submit->dispatch_ccb_data0;
		record->ccb_data1 = submit->dispatch_ccb_data1;
		record->control_gpu_va32 =
			(u32)submit->dispatch_control_gpu_va;
		record->status_gpu_va32 = (u32)submit->dispatch_status_gpu_va;
		record->pds_gpu_va32 = (u32)submit->dispatch_pds_gpu_va;
		record->use_gpu_va32 = (u32)submit->dispatch_use_gpu_va;
		record->target_gpu_va32 = (u32)submit->dispatch_target_gpu_va;
		record->ccb_status_gpu_va32 =
			(u32)submit->dispatch_status_gpu_va;
		return 0;
	case PSB_SGX_LAB_EXEC_SOURCE_RETIRED:
		record->prepare_count = submit->retired_prepare_count;
		record->arm_count = submit->retired_arm_count;
		record->emit_count = submit->retired_emit_count;
		record->launch_count = submit->retired_launch_count;
		record->dispatch_count = submit->retired_dispatch_count;
		record->retired_count = submit->retired_count;
		record->context = submit->retired_context;
		record->cmd_type = submit->retired_cmd_type;
		record->owner_state = submit->retired_owner_state;
		record->issued_packets = submit->retired_issued_packets;
		record->completed_packets = submit->retired_completed_packets;
		record->dependency_active_count =
			submit->retired_dependency_active_count;
		record->refilled_from_complete =
			submit->retired_refilled_from_complete;
		record->queue_packet_count = submit->retired_queue_packet_count;
		record->queue_epoch = submit->retired_queue_epoch;
		record->queue_base_offset = submit->queue_base_offset;
		record->queue_size = submit->queue_size;
		record->queue_packet_size = submit->queue_packet_size;
		record->ccb_offset = submit->ccb_offset;
		record->ccb_write_offset = submit->ccb_write_offset;
		record->ccb_read_offset = submit->ccb_read_offset;
		record->sync_offset = submit->sync_offset;
		record->sync_complete_seq = submit->retired_sync_complete_seq;
		record->sync_pending_seq = submit->retired_sync_pending_seq;
		record->sync_flags = submit->sync_flags;
		record->owner_offset = submit->owner_offset;
		record->owner_issue_cookie =
			submit->retired_owner_issue_cookie;
		record->owner_complete_cookie =
			submit->retired_owner_complete_cookie;
		record->global_issue_ticket =
			submit->retired_global_issue_ticket;
		record->global_running_ticket =
			submit->retired_global_running_ticket;
		record->global_complete_ticket =
			submit->retired_global_complete_ticket;
		record->ctrl_flags = submit->ctrl_flags;
		record->control_object_id = submit->retired_control_object_id ?
			submit->retired_control_object_id :
			submit->control_object_id;
		record->status_object_id = submit->retired_status_object_id ?
			submit->retired_status_object_id :
			submit->status_object_id;
		record->ccb_service_address = submit->ccb_service_address;
		record->ccb_cache_control = submit->ccb_cache_control;
		record->ccb_data0 = submit->ccb_data0;
		record->ccb_data1 = submit->ccb_data1;
		record->control_gpu_va32 = submit->retired_control_gpu_va ?
			(u32)submit->retired_control_gpu_va : 0;
		record->status_gpu_va32 = submit->retired_status_gpu_va ?
			(u32)submit->retired_status_gpu_va : 0;
		if (!record->control_gpu_va32 && dev_priv) {
			ret = poulsbo_sgx_resolve_object_gpu_va32_locked(
				dev_priv, submit->control_object_id,
				&record->control_gpu_va32);
			if (ret)
				return ret;
		}
		if (!record->status_gpu_va32 && dev_priv) {
			ret = poulsbo_sgx_resolve_object_gpu_va32_locked(
				dev_priv, submit->status_object_id,
				&record->status_gpu_va32);
			if (ret)
				return ret;
		}
		if (dev_priv) {
			ret = poulsbo_sgx_resolve_submit_ccb_gpu_va32_locked(
				dev_priv, submit, &record->pds_gpu_va32,
				&record->use_gpu_va32, &record->target_gpu_va32,
				&record->ccb_status_gpu_va32);
			if (ret)
				return ret;
		}
		return 0;
	case PSB_SGX_LAB_EXEC_SOURCE_LIVE:
	default:
		record->prepare_count = submit->prepare_count;
		record->arm_count = submit->arm_count;
		record->emit_count = submit->emit_count;
		record->launch_count = submit->launch_count;
		record->dispatch_count = submit->dispatch_count;
		record->retired_count = submit->retired_count;
		record->context = submit->context;
		record->cmd_type = submit->cmd_type;
		record->owner_state = submit->owner_state;
		record->issued_packets =
			poulsbo_sgx_submit_report_issued_packets_locked(
				submit, stage);
		record->completed_packets =
			poulsbo_sgx_submit_report_completed_packets_locked(
				submit, stage);
		record->dependency_active_count =
			poulsbo_sgx_submit_report_dependency_count_locked(
				submit, stage);
		record->queue_packet_count = submit->queue_packet_count;
		record->queue_epoch = submit->queue_epoch;
		record->queue_base_offset = submit->queue_base_offset;
		record->queue_size = submit->queue_size;
		record->queue_packet_size = submit->queue_packet_size;
		record->ccb_offset = submit->ccb_offset;
		record->ccb_write_offset = submit->ccb_write_offset;
		record->ccb_read_offset = submit->ccb_read_offset;
		record->sync_offset = submit->sync_offset;
		record->sync_complete_seq = submit->sync_complete_seq;
		record->sync_pending_seq = submit->sync_pending_seq;
		record->sync_flags = submit->sync_flags;
		record->owner_offset = submit->owner_offset;
		record->owner_issue_cookie = submit->owner_issue_cookie;
		record->owner_complete_cookie = submit->owner_complete_cookie;
		record->global_issue_ticket = submit->global_issue_ticket;
		record->global_running_ticket = submit->global_running_ticket;
		record->global_complete_ticket =
			submit->global_complete_ticket;
		record->ctrl_flags = submit->ctrl_flags;
		record->control_object_id = submit->control_object_id;
		record->status_object_id = submit->status_object_id;
		record->ccb_service_address = submit->ccb_service_address;
		record->ccb_cache_control = submit->ccb_cache_control;
		record->ccb_data0 = submit->ccb_data0;
		record->ccb_data1 = submit->ccb_data1;
		if (dev_priv) {
			ret = poulsbo_sgx_resolve_object_gpu_va32_locked(
				dev_priv, submit->control_object_id,
				&record->control_gpu_va32);
			if (ret)
				return ret;
			ret = poulsbo_sgx_resolve_submit_ccb_gpu_va32_locked(
				dev_priv, submit, &record->pds_gpu_va32,
				&record->use_gpu_va32, &record->target_gpu_va32,
				&record->ccb_status_gpu_va32);
			if (ret)
				return ret;
			record->status_gpu_va32 = record->ccb_status_gpu_va32;
		}
		return 0;
	}
}

static int poulsbo_sgx_fill_submit_exec_packet_from_record_locked(
	const struct psb_sgx_lab_submit_exec_record *record,
	struct psb_sgx_lab_submit_exec_packet *packet)
{
	u32 expected_tail_packets = 0;
	u32 expected_head_packets = 0;
	u32 expected_write_offset = 0;
	int ret;

	if (!record || !packet)
		return -EINVAL;

	expected_write_offset = record->ccb_write_offset;
	if (record->queue_size && record->queue_packet_size &&
	    record->queue_packet_count) {
		ret = poulsbo_sgx_queue_progression(
			record->ccb_read_offset, record->queue_size,
			record->queue_packet_size, record->queue_packet_count,
			&expected_tail_packets, &expected_head_packets,
			&expected_write_offset);
		if (ret)
			return ret;
		if (expected_write_offset != record->ccb_write_offset)
			return -EINVAL;
	}

	memset(packet, 0, sizeof(*packet));
	packet->stage = record->stage;
	packet->stage_count = record->stage_count;
	packet->source = record->source;
	packet->context = record->context;
	packet->cmd_type = record->cmd_type;
	packet->issued_packets = record->issued_packets;
	packet->completed_packets = record->completed_packets;
	packet->refilled_from_complete = record->refilled_from_complete;
	packet->queue_packet_count = record->queue_packet_count;
	packet->queue_epoch = record->queue_epoch;
	packet->queue_base_offset = record->queue_base_offset;
	packet->queue_size = record->queue_size;
	packet->queue_packet_size = record->queue_packet_size;
	packet->queue_tail_packet_count = expected_tail_packets;
	packet->queue_head_packet_count = expected_head_packets;
	packet->ccb_offset = record->ccb_offset;
	packet->ccb_write_offset = record->ccb_write_offset;
	packet->ccb_read_offset = record->ccb_read_offset;
	packet->control_object_id = record->control_object_id;
	packet->status_object_id = record->status_object_id;
	packet->control_gpu_va32 = record->control_gpu_va32;
	packet->status_gpu_va32 = record->status_gpu_va32;
	packet->pds_gpu_va32 = record->pds_gpu_va32;
	packet->use_gpu_va32 = record->use_gpu_va32;
	packet->target_gpu_va32 = record->target_gpu_va32;
	packet->ccb_status_gpu_va32 = record->ccb_status_gpu_va32;
	packet->command.service_address = record->ccb_service_address;
	packet->command.cache_control = record->ccb_cache_control;
	packet->command.data0 = record->ccb_data0;
	packet->command.data1 = record->ccb_data1;
	packet->ta_shared.ctrl_flags = record->ctrl_flags;
	packet->ta_shared.report_flags = record->report_flags;
	packet->ta_shared.num_ta_status_vals = record->num_ta_status_vals;
	packet->ta_shared.num_3d_status_vals = record->num_3d_status_vals;
	packet->ta_shared.ta_dependency = record->ta_dependency;
	packet->ta_shared.status_ta_offset = record->status_ta_offset;
	packet->ta_shared.status_3d_offset = record->status_3d_offset;
	packet->ta_shared.status_stride = record->status_stride;
	packet->ta_shared.status_value_seed = record->status_value_seed;
	packet->ta_shared.sync_offset = record->sync_offset;
	packet->ta_shared.sync_complete_seq = record->sync_complete_seq;
	packet->ta_shared.sync_pending_seq = record->sync_pending_seq;
	packet->ta_shared.sync_flags = record->sync_flags;
	packet->ta_shared.owner_offset = record->owner_offset;
	packet->ta_shared.owner_state = record->owner_state;
	packet->ta_shared.owner_issue_cookie = record->owner_issue_cookie;
	packet->ta_shared.owner_complete_cookie =
		record->owner_complete_cookie;
	packet->ta_shared.global_issue_ticket = record->global_issue_ticket;
	packet->ta_shared.global_running_ticket =
		record->global_running_ticket;
	packet->ta_shared.global_complete_ticket =
		record->global_complete_ticket;
	packet->ta_shared.dependency_active_count =
		record->dependency_active_count;
	return 0;
}

static int poulsbo_sgx_build_submit_exec_packet_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	enum psb_sgx_lab_exec_source source,
	struct psb_sgx_lab_submit_exec_packet *packet)
{
	struct psb_sgx_lab_submit_exec_record record;
	int ret;

	if (!packet)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_exec_record_locked(dev_priv, submit,
							      source, &record);
	if (ret)
		return ret;

	return poulsbo_sgx_fill_submit_exec_packet_from_record_locked(
		&record, packet);
}

static int poulsbo_sgx_gpu_va32_add_offset(u32 base, u32 offset, u32 *out)
{
	if (!out)
		return -EINVAL;

	if (!base) {
		*out = 0;
		return 0;
	}
	if (U32_MAX - base < offset)
		return -EINVAL;

	*out = base + offset;
	return 0;
}

static void poulsbo_sgx_init_submit_sgx_device_sync_object(
	struct psb_sgx_lab_submit_sgx_device_sync_object *sync, u32 read_pending,
	u32 read_complete_gpu_va32, u32 write_pending,
	u32 write_complete_gpu_va32)
{
	if (!sync)
		return;

	memset(sync, 0, sizeof(*sync));
	sync->read_pending_val = read_pending;
	sync->read_complete_gpu_va32 = read_complete_gpu_va32;
	sync->write_pending_val = write_pending;
	sync->write_complete_gpu_va32 = write_complete_gpu_va32;
}

static bool poulsbo_sgx_submit_sgx_device_sync_object_is_zero(
	const struct psb_sgx_lab_submit_sgx_device_sync_object *sync)
{
	if (!sync)
		return true;

	return !sync->read_pending_val && !sync->read_complete_gpu_va32 &&
	       !sync->write_pending_val && !sync->write_complete_gpu_va32;
}

static bool poulsbo_sgx_submit_sgx_device_sync_object_equal(
	const struct psb_sgx_lab_submit_sgx_device_sync_object *left,
	const struct psb_sgx_lab_submit_sgx_device_sync_object *right)
{
	if (!left || !right)
		return false;

	return left->read_pending_val == right->read_pending_val &&
	       left->read_complete_gpu_va32 == right->read_complete_gpu_va32 &&
	       left->write_pending_val == right->write_pending_val &&
	       left->write_complete_gpu_va32 ==
		       right->write_complete_gpu_va32;
}

static int poulsbo_sgx_build_submit_cmdta_kick_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	enum psb_sgx_lab_exec_source source,
	bool with_issue_plan,
	struct psb_sgx_lab_submit_cmdta_kick *kick)
{
	int ret;

	if (!kick)
		return -EINVAL;

	memset(kick, 0, sizeof(*kick));

	ret = poulsbo_sgx_build_submit_exec_packet_locked(dev_priv, submit, source,
							      &kick->packet);
	if (ret)
		return ret;
	if (kick->packet.cmd_type != PSB_SGX_LAB_CMD_TA)
		return -EOPNOTSUPP;

	ret = poulsbo_sgx_gpu_va32_add_offset(kick->packet.control_gpu_va32,
					      kick->packet.queue_base_offset,
					      &kick->queue_gpu_va32);
	if (ret)
		return ret;
	ret = poulsbo_sgx_gpu_va32_add_offset(kick->packet.control_gpu_va32,
					      kick->packet.ccb_offset,
					      &kick->ccb_gpu_va32);
	if (ret)
		return ret;
	ret = poulsbo_sgx_gpu_va32_add_offset(kick->packet.status_gpu_va32,
					      kick->packet.ta_shared.sync_offset,
					      &kick->sync_gpu_va32);
	if (ret)
		return ret;
	ret = poulsbo_sgx_gpu_va32_add_offset(kick->packet.status_gpu_va32,
					      kick->packet.ta_shared.owner_offset,
					      &kick->owner_gpu_va32);
	if (ret)
		return ret;
	ret = poulsbo_sgx_gpu_va32_add_offset(
		kick->packet.status_gpu_va32,
		kick->packet.ta_shared.status_ta_offset,
		&kick->ta_status_gpu_va32);
	if (ret)
		return ret;
	ret = poulsbo_sgx_gpu_va32_add_offset(
		kick->packet.status_gpu_va32,
		kick->packet.ta_shared.status_3d_offset,
		&kick->status_3d_gpu_va32);
	if (ret)
		return ret;

	if (!with_issue_plan)
		return 0;
	if (!poulsbo_sgx_build_issue_plan_locked(
		    dev_priv, submit, submit->issue_plan_owner_state,
		    submit->issue_plan_packet_count, &kick->issue_plan))
		return -EINVAL;

	kick->have_issue_plan = true;
	return 0;
}

static int poulsbo_sgx_build_submit_status_updates_locked(
	u32 status_gpu_va32,
	u32 base_offset,
	u32 count,
	u32 stride,
	u32 value_seed,
	bool is_3d,
	struct psb_sgx_lab_submit_status_update *updates)
{
	u32 index;

	if (!count)
		return 0;
	if (!updates)
		return -EINVAL;

	for (index = 0; index < count; index++) {
		u64 value_offset = (u64)base_offset + (u64)index * stride;
		int ret;

		if (value_offset > U32_MAX)
			return -EINVAL;
		updates[index].value_offset = (u32)value_offset;
		ret = poulsbo_sgx_gpu_va32_add_offset(
			status_gpu_va32, updates[index].value_offset,
			&updates[index].value_gpu_va32);
		if (ret)
			return ret;
		updates[index].value = poulsbo_sgx_lab_status_value(
			value_seed, is_3d, index);
	}

	return 0;
}

static int poulsbo_sgx_build_submit_ccb_kick_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	enum psb_sgx_lab_exec_source source,
	bool with_issue_plan,
	struct psb_sgx_lab_submit_ccb_kick *kick)
{
	int ret;

	if (!kick)
		return -EINVAL;

	memset(kick, 0, sizeof(*kick));

	ret = poulsbo_sgx_build_submit_cmdta_kick_locked(
		dev_priv, submit, source, with_issue_plan, &kick->cmdta);
	if (ret)
		return ret;

	kick->ccb_offset = kick->cmdta.packet.ccb_offset;
	kick->ta_dependency = !!kick->cmdta.packet.ta_shared.ta_dependency;
	kick->num_ta_status_updates =
		kick->cmdta.packet.ta_shared.num_ta_status_vals;
	kick->num_3d_status_updates =
		kick->cmdta.packet.ta_shared.num_3d_status_vals;

	ret = poulsbo_sgx_build_submit_status_updates_locked(
		kick->cmdta.packet.status_gpu_va32,
		kick->cmdta.packet.ta_shared.status_ta_offset,
		kick->num_ta_status_updates,
		kick->cmdta.packet.ta_shared.status_stride,
		kick->cmdta.packet.ta_shared.status_value_seed,
		false, kick->ta_status_updates);
	if (ret)
		return ret;

	return poulsbo_sgx_build_submit_status_updates_locked(
		kick->cmdta.packet.status_gpu_va32,
		kick->cmdta.packet.ta_shared.status_3d_offset,
		kick->num_3d_status_updates,
		kick->cmdta.packet.ta_shared.status_stride,
		kick->cmdta.packet.ta_shared.status_value_seed,
		true, kick->threed_status_updates);
}

static int poulsbo_sgx_build_submit_sgx_ccb_kick_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_ccb_kick *ccb_kick,
	struct psb_sgx_lab_submit_sgx_ccb_kick *sgx_kick)
{
	bool dependency_active;

	if (!submit || !ccb_kick || !sgx_kick)
		return -EINVAL;

	memset(sgx_kick, 0, sizeof(*sgx_kick));

	sgx_kick->kick = ccb_kick;
	sgx_kick->num_dst_sync_objects =
		ccb_kick->cmdta.sync_gpu_va32 ? 1 : 0;
	sgx_kick->num_ta_status_vals = ccb_kick->num_ta_status_updates;
	sgx_kick->num_3d_status_vals = ccb_kick->num_3d_status_updates;
	sgx_kick->first_kick_or_resume =
		!ccb_kick->cmdta.packet.refilled_from_complete &&
		!ccb_kick->cmdta.packet.queue_epoch;
	sgx_kick->ccb_offset = ccb_kick->ccb_offset;
	sgx_kick->num_ta_src_syncs = 0;
	sgx_kick->num_ta_dst_syncs = sgx_kick->num_dst_sync_objects;
	dependency_active = ccb_kick->ta_dependency &&
		ccb_kick->cmdta.packet.ta_shared.dependency_active_count &&
		submit->dependency_snapshot_source_submit_id &&
		submit->dependency_snapshot_required_ticket;
	sgx_kick->num_3d_src_syncs = dependency_active ? 1 : 0;
	sgx_kick->ta_dependency = ccb_kick->ta_dependency;
	sgx_kick->ta_sync_gpu_va32 = ccb_kick->cmdta.sync_gpu_va32;
	sgx_kick->threed_sync_gpu_va32 = ccb_kick->cmdta.sync_gpu_va32;

	if (!dependency_active)
		return 0;

	sgx_kick->ta3d_dependency_submit_id =
		submit->dependency_snapshot_source_submit_id;
	sgx_kick->ta3d_dependency_required_complete_ticket =
		submit->dependency_snapshot_required_ticket;
	sgx_kick->ta3d_dependency_observed_complete_ticket =
		submit->dependency_snapshot_observed_complete_ticket;
	sgx_kick->ta3d_dependency_flags = submit->dependency_snapshot_flags;
	return 0;
}

static int poulsbo_sgx_build_submit_sgx_cmdta_shared_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_ccb_kick *kick,
	struct psb_sgx_lab_submit_sgx_cmdta_shared *shared)
{
	struct psb_sgx_lab_submit_sgx_ccb_kick sgx_kick;
	int ret;

	if (!submit || !kick || !shared)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_sgx_ccb_kick_locked(
		submit, kick, &sgx_kick);
	if (ret)
		return ret;

	memset(shared, 0, sizeof(*shared));

	shared->kick = kick;
	shared->ctrl_flags = kick->cmdta.packet.ta_shared.ctrl_flags;
	shared->num_dst_sync_objects = sgx_kick.num_dst_sync_objects;
	shared->num_ta_status_vals = sgx_kick.num_ta_status_vals;
	shared->num_3d_status_vals = sgx_kick.num_3d_status_vals;
	shared->first_kick_or_resume = sgx_kick.first_kick_or_resume;
	shared->ccb_offset = sgx_kick.ccb_offset;
	shared->ta_dependency = sgx_kick.ta_dependency;
	poulsbo_sgx_init_submit_sgx_device_sync_object(
		&shared->ta_tq_sync,
		kick->cmdta.packet.ta_shared.sync_complete_seq,
		sgx_kick.ta_sync_gpu_va32,
		kick->cmdta.packet.ta_shared.sync_pending_seq,
		sgx_kick.ta_sync_gpu_va32);
	poulsbo_sgx_init_submit_sgx_device_sync_object(
		&shared->threed_tq_sync,
		kick->cmdta.packet.ta_shared.sync_complete_seq,
		sgx_kick.threed_sync_gpu_va32,
		kick->cmdta.packet.ta_shared.sync_pending_seq,
		sgx_kick.threed_sync_gpu_va32);
	shared->hwdevice_sync_list.access_gpu_va32 = kick->cmdta.sync_gpu_va32;
	shared->hwdevice_sync_list.num_sync_objects =
		sgx_kick.num_dst_sync_objects;
	if (shared->hwdevice_sync_list.num_sync_objects >
	    ARRAY_SIZE(shared->hwdevice_sync_list.sync_data))
		return -EINVAL;
	if (shared->hwdevice_sync_list.num_sync_objects) {
		poulsbo_sgx_init_submit_sgx_device_sync_object(
			&shared->hwdevice_sync_list.sync_data[0],
			kick->cmdta.packet.ta_shared.sync_complete_seq,
			sgx_kick.ta_sync_gpu_va32,
			kick->cmdta.packet.ta_shared.sync_pending_seq,
			sgx_kick.ta_sync_gpu_va32);
	}
	shared->num_ta_src_syncs = sgx_kick.num_ta_src_syncs;
	shared->num_ta_dst_syncs = sgx_kick.num_ta_dst_syncs;
	shared->num_3d_src_syncs = sgx_kick.num_3d_src_syncs;
	if (shared->num_3d_src_syncs) {
		poulsbo_sgx_init_submit_sgx_device_sync_object(
			&shared->ta3d_dependency_sync,
			0, 0,
			sgx_kick.ta3d_dependency_observed_complete_ticket,
			sgx_kick.threed_sync_gpu_va32);
	}
	shared->ta3d_dependency.submit_id =
		sgx_kick.ta3d_dependency_submit_id;
	shared->ta3d_dependency.required_complete_ticket =
		sgx_kick.ta3d_dependency_required_complete_ticket;
	shared->ta3d_dependency.observed_complete_ticket =
		sgx_kick.ta3d_dependency_observed_complete_ticket;
	shared->ta3d_dependency.flags = sgx_kick.ta3d_dependency_flags;
	shared->ta3d_dependency.gpu_va32 = sgx_kick.threed_sync_gpu_va32;
	shared->ta_status_info = kick->ta_status_updates;
	shared->threed_status_info = kick->threed_status_updates;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_cmdta_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_ccb_kick *kick,
	struct psb_sgx_lab_submit_sgx_cmdta *cmdta)
{
	int ret;

	if (!submit || !kick || !cmdta)
		return -EINVAL;

	memset(cmdta, 0, sizeof(*cmdta));

	ret = poulsbo_sgx_build_submit_sgx_cmdta_shared_locked(
		submit, kick, &cmdta->shared);
	if (ret)
		return ret;

	cmdta->kick = kick;

	if (cmdta->shared.num_ta_src_syncs)
		return -EINVAL;
	if (cmdta->shared.num_ta_dst_syncs > ARRAY_SIZE(cmdta->ta_dst_syncs))
		return -EINVAL;
	if (cmdta->shared.num_3d_src_syncs > ARRAY_SIZE(cmdta->threed_src_syncs))
		return -EINVAL;

	if (cmdta->shared.num_ta_dst_syncs)
		memcpy(cmdta->ta_dst_syncs,
		       cmdta->shared.hwdevice_sync_list.sync_data,
		       cmdta->shared.num_ta_dst_syncs *
			       sizeof(cmdta->ta_dst_syncs[0]));
	if (cmdta->shared.num_3d_src_syncs)
		cmdta->threed_src_syncs[0] =
			cmdta->shared.ta3d_dependency_sync;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_kick_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_ccb_kick *kick,
	struct psb_sgx_lab_submit_sgx_ta_kick *ta_kick)
{
	int ret;

	if (!submit || !kick || !ta_kick)
		return -EINVAL;

	memset(ta_kick, 0, sizeof(*ta_kick));

	ret = poulsbo_sgx_build_submit_sgx_cmdta_locked(
		submit, kick, &ta_kick->cmdta);
	if (ret)
		return ret;

	ta_kick->kick = kick;
	ta_kick->command.service_address =
		kick->cmdta.packet.command.service_address;
	ta_kick->command.cache_control =
		kick->cmdta.packet.command.cache_control;
	ta_kick->command.data0 = kick->cmdta.packet.command.data0;
	ta_kick->command.data1 = kick->cmdta.packet.command.data1;
	ta_kick->control_gpu_va32 = kick->cmdta.packet.control_gpu_va32;
	ta_kick->ccb_gpu_va32 = kick->cmdta.ccb_gpu_va32;
	ta_kick->cmdta_gpu_va32 = kick->cmdta.ccb_gpu_va32;
	ta_kick->queue_gpu_va32 = kick->cmdta.queue_gpu_va32;
	ta_kick->sync_gpu_va32 = kick->cmdta.sync_gpu_va32;
	ta_kick->owner_gpu_va32 = kick->cmdta.owner_gpu_va32;
	ta_kick->ta_status_gpu_va32 = kick->cmdta.ta_status_gpu_va32;
	ta_kick->status_3d_gpu_va32 = kick->cmdta.status_3d_gpu_va32;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_command_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_ccb_kick *kick,
	struct psb_sgx_lab_submit_sgx_ta_command *ta_command)
{
	int ret;

	if (!submit || !kick || !ta_command)
		return -EINVAL;

	memset(ta_command, 0, sizeof(*ta_command));

	ret = poulsbo_sgx_build_submit_sgx_ta_kick_locked(
		submit, kick, &ta_command->ta_kick);
	if (ret)
		return ret;

	ta_command->kick = kick;
	return poulsbo_sgx_fill_submit_ccb_record_words_from_packet(
		&kick->cmdta.packet, &ta_command->ccb_record);
}

static int poulsbo_sgx_build_submit_sgx_queued_ta_command_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_ccb_kick *kick,
	struct psb_sgx_lab_submit_sgx_queued_ta_command *queued_ta_command)
{
	int ret;

	if (!submit || !kick || !queued_ta_command)
		return -EINVAL;

	memset(queued_ta_command, 0, sizeof(*queued_ta_command));

	ret = poulsbo_sgx_build_submit_sgx_ta_command_locked(
		submit, kick, &queued_ta_command->ta_command);
	if (ret)
		return ret;

	queued_ta_command->kick = kick;
	queued_ta_command->queue_header.write_offset =
		kick->cmdta.packet.ccb_write_offset;
	queued_ta_command->queue_header.read_offset =
		kick->cmdta.packet.ccb_read_offset;
	queued_ta_command->queue_header.queue_base_offset =
		kick->cmdta.packet.queue_base_offset;
	queued_ta_command->queue_header.queue_size =
		kick->cmdta.packet.queue_size;
	queued_ta_command->queue_header.queue_packet_size =
		kick->cmdta.packet.queue_packet_size;
	queued_ta_command->queue_header.queue_packet_count =
		kick->cmdta.packet.queue_packet_count;
	queued_ta_command->queue_header.queue_tail_packet_count =
		kick->cmdta.packet.queue_tail_packet_count;
	queued_ta_command->queue_header.queue_head_packet_count =
		kick->cmdta.packet.queue_head_packet_count;
	if (!kick->cmdta.have_issue_plan)
		return 0;

	queued_ta_command->have_issue_plan = true;
	queued_ta_command->issue_plan = kick->cmdta.issue_plan;
	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submission_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_ccb_kick *kick,
	struct psb_sgx_lab_submit_sgx_ta_submission *ta_submission)
{
	int ret;

	if (!dev_priv || !submit || !kick || !ta_submission)
		return -EINVAL;

	memset(ta_submission, 0, sizeof(*ta_submission));

	ret = poulsbo_sgx_build_submit_sgx_queued_ta_command_locked(
		submit, kick, &ta_submission->queued_ta_command);
	if (ret)
		return ret;

	ta_submission->kick = kick;

	if (submit->live_config_count) {
		ret = poulsbo_sgx_build_submit_live_locked(
			dev_priv, submit, &ta_submission->live);
		if (ret)
			return ret;
		ta_submission->have_live = true;
	}

	if (submit->manifest_config_count) {
		ret = poulsbo_sgx_build_submit_manifest_locked(
			dev_priv, submit, &ta_submission->manifest);
		if (ret)
			return ret;
		ta_submission->have_manifest = true;
	}

	if (submit->descriptor_config_count) {
		ret = poulsbo_sgx_build_submit_descriptor_locked(
			dev_priv, submit, &ta_submission->descriptor);
		if (ret)
			return ret;
		ta_submission->have_descriptor = true;
	}

	if (submit->plan_config_count) {
		ret = poulsbo_sgx_build_submit_plan_locked(
			dev_priv, submit, &ta_submission->plan);
		if (ret)
			return ret;
		ta_submission->have_plan = true;
	}

	if (submit->issue_plan_config_count) {
		ret = poulsbo_sgx_build_submit_issue_plan_view_locked(
			dev_priv, submit, &ta_submission->issue_plan);
		if (ret)
			return ret;
		ta_submission->have_issue_plan_view = true;
	}

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_execution_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_submit_sgx_ta_execution *execution)
{
	int ret;

	if (!dev_priv || !submit || !execution)
		return -EINVAL;

	memset(execution, 0, sizeof(*execution));

	ret = poulsbo_sgx_build_submit_ccb_kick_locked(
		dev_priv, submit, PSB_SGX_LAB_EXEC_SOURCE_LIVE, false,
		&execution->live_kick);
	if (ret)
		return ret;
	ret = poulsbo_sgx_build_submit_sgx_ta_submission_locked(
		dev_priv, submit, &execution->live_kick,
		&execution->live_submission);
	if (ret)
		return ret;

	if (!submit->launch_valid || submit->launch_consumed ||
	    !submit->issue_plan_packet_count)
		return 0;

	ret = poulsbo_sgx_build_submit_ccb_kick_locked(
		dev_priv, submit, PSB_SGX_LAB_EXEC_SOURCE_LAUNCH, true,
		&execution->launch_kick);
	if (ret)
		return ret;
	ret = poulsbo_sgx_build_submit_sgx_ta_submission_locked(
		dev_priv, submit, &execution->launch_kick,
		&execution->launch_submission);
	if (ret)
		return ret;

	execution->have_launch_submission = true;
	return 0;
}

static int poulsbo_sgx_build_submit_sgx_queued_ta_command_from_source_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	enum psb_sgx_lab_exec_source source,
	struct psb_sgx_lab_submit_ccb_kick *kick,
	struct psb_sgx_lab_submit_sgx_queued_ta_command *queued_ta_command)
{
	int ret;

	if (!dev_priv || !submit || !kick || !queued_ta_command)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_ccb_kick_locked(
		dev_priv, submit, source, false, kick);
	if (ret)
		return ret;

	return poulsbo_sgx_build_submit_sgx_queued_ta_command_locked(
		submit, kick, queued_ta_command);
}

static int poulsbo_sgx_build_submit_sgx_ta_timeline_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_submit_sgx_ta_timeline *timeline)
{
	int ret;

	if (!dev_priv || !submit || !timeline)
		return -EINVAL;

	memset(timeline, 0, sizeof(*timeline));

	ret = poulsbo_sgx_build_submit_sgx_ta_execution_locked(
		dev_priv, submit, &timeline->execution);
	if (ret)
		return ret;

	timeline->report_stage = poulsbo_sgx_submit_report_stage_locked(submit);
	timeline->report_stage_count =
		poulsbo_sgx_submit_report_stage_count_locked(
			submit, timeline->report_stage);
	timeline->issue_count = submit->issue_count;
	timeline->transition_count = submit->transition_count;
	timeline->global_issue_ticket = submit->global_issue_ticket;
	timeline->global_running_ticket = submit->global_running_ticket;
	timeline->global_complete_ticket = submit->global_complete_ticket;
	timeline->arm_valid = submit->arm_valid;
	timeline->emit_valid = submit->emit_valid;
	timeline->launch_valid = submit->launch_valid;
	timeline->launch_consumed = submit->launch_consumed;
	timeline->dispatch_valid = submit->dispatch_valid;
	timeline->retired_valid = submit->retired_valid;

	if (submit->launch_valid) {
		ret = poulsbo_sgx_build_submit_sgx_queued_ta_command_from_source_locked(
			dev_priv, submit, PSB_SGX_LAB_EXEC_SOURCE_LAUNCH,
			&timeline->launch_kick, &timeline->launch_command);
		if (ret)
			return ret;
		timeline->have_launch_command = true;
	}

	if (submit->dispatch_valid) {
		ret = poulsbo_sgx_build_submit_sgx_queued_ta_command_from_source_locked(
			dev_priv, submit, PSB_SGX_LAB_EXEC_SOURCE_DISPATCH,
			&timeline->dispatch_kick, &timeline->dispatch_command);
		if (ret)
			return ret;
		timeline->have_dispatch_command = true;
	}

	if (submit->retired_valid) {
		ret = poulsbo_sgx_build_submit_sgx_queued_ta_command_from_source_locked(
			dev_priv, submit, PSB_SGX_LAB_EXEC_SOURCE_RETIRED,
			&timeline->retired_kick, &timeline->retired_command);
		if (ret)
			return ret;
		timeline->have_retired_command = true;
	}

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_runtime_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_submit_sgx_ta_runtime *runtime)
{
	struct psb_sgx_lab_submit *peer_submit;
	unsigned int slot;
	int ret;

	if (!dev_priv || !submit || !runtime)
		return -EINVAL;

	memset(runtime, 0, sizeof(*runtime));

	ret = poulsbo_sgx_build_submit_sgx_ta_timeline_locked(
		dev_priv, submit, &runtime->timeline);
	if (ret)
		return ret;

	runtime->coordination.issue_ticket = dev_priv->sgx_lab_global_issue_ticket;
	runtime->coordination.running_ticket =
		dev_priv->sgx_lab_global_running_ticket;
	runtime->coordination.complete_ticket =
		dev_priv->sgx_lab_global_complete_ticket;
	runtime->coordination.issue_submit_id =
		dev_priv->sgx_lab_global_issue_submit_id;
	runtime->coordination.running_submit_id =
		dev_priv->sgx_lab_global_running_submit_id;
	runtime->coordination.complete_submit_id =
		dev_priv->sgx_lab_global_complete_submit_id;
	runtime->coordination.active_running_valid =
		dev_priv->sgx_lab_running_active;
	runtime->coordination.active_running_submit_id =
		dev_priv->sgx_lab_active_running_submit_id;

	if (submit->sync_config_count)
		runtime->sync_outstanding_count =
			submit->sync_pending_seq - submit->sync_complete_seq;
	if (submit->owner_config_count)
		runtime->owner_outstanding_count =
			submit->owner_issue_cookie - submit->owner_complete_cookie;

	for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
		struct psb_sgx_lab_submit_sgx_dependency_state *dependency;
		u32 *dependency_submit_id;
		u32 *dependency_snapshot_flags;

		dependency = &runtime->dependencies[slot];
		dependency_submit_id =
			poulsbo_sgx_dependency_submit_id_ptr(submit, slot);
		dependency_snapshot_flags =
			poulsbo_sgx_dependency_snapshot_flags_ptr(submit, slot);
		if (!dependency_submit_id || !dependency_snapshot_flags)
			return -EINVAL;

		dependency->active =
			poulsbo_sgx_dependency_slot_active(submit, slot);
		dependency->submit_id = *dependency_submit_id;
		dependency->required_ticket =
			poulsbo_sgx_dependency_required_ticket(submit, slot);
		dependency->flags = *dependency_snapshot_flags;

		if (!dependency->active)
			continue;

		ret = poulsbo_sgx_dependency_observed_ticket_locked(
			dev_priv, submit, slot,
			&dependency->observed_complete_ticket,
			&dependency->source_missing);
		if (ret)
			return ret;
	}

	list_for_each_entry(peer_submit, &dev_priv->sgx_lab_submit_list, head) {
		if (!peer_submit->owner_config_count)
			continue;
		if (peer_submit->owner_state != PSB_SGX_LAB_OWNER_RUNNING)
			continue;
		runtime->coordination.running_owner_submit_count++;
		runtime->coordination.running_owner_submit_id =
			peer_submit->submit_id;
	}

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_gate_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_runtime *runtime,
	struct psb_sgx_lab_submit_sgx_ta_gate *gate)
{
	unsigned int slot;

	if (!submit || !runtime || !gate)
		return -EINVAL;

	memset(gate, 0, sizeof(*gate));

	for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
		const struct psb_sgx_lab_submit_sgx_dependency_state *dependency;
		struct psb_sgx_lab_submit_sgx_gate_slot *gate_dependency;

		dependency = &runtime->dependencies[slot];
		gate_dependency = &gate->dependencies[slot];

		gate_dependency->active = dependency->active;
		gate_dependency->source_bound = !!dependency->submit_id;
		gate_dependency->source_missing = dependency->source_missing;
		gate_dependency->gated = !!dependency->required_ticket;
		gate_dependency->submit_id = dependency->submit_id;
		gate_dependency->required_ticket = dependency->required_ticket;
		gate_dependency->observed_complete_ticket =
			dependency->observed_complete_ticket;
		gate_dependency->flags = dependency->flags;
		gate_dependency->satisfied =
			dependency->active &&
			(!gate_dependency->gated ||
			 dependency->observed_complete_ticket >=
				 dependency->required_ticket);
		gate_dependency->blocked =
			dependency->active && gate_dependency->gated &&
			!gate_dependency->satisfied;
		gate_dependency->ready =
			dependency->active &&
			!gate_dependency->source_missing &&
			gate_dependency->satisfied;

		if (!gate_dependency->active)
			continue;

		gate->active_dependency_count++;
		if (gate_dependency->source_bound)
			gate->source_bound_dependency_count++;
		if (gate_dependency->gated)
			gate->gated_dependency_count++;
		if (gate_dependency->satisfied)
			gate->satisfied_dependency_count++;
		if (gate_dependency->blocked)
			gate->blocked_dependency_count++;
		if (gate_dependency->source_missing)
			gate->missing_dependency_count++;
	}

	gate->running_slot_busy =
		runtime->coordination.active_running_valid &&
		runtime->coordination.active_running_submit_id !=
			submit->submit_id;
	gate->can_enter_running =
		!gate->running_slot_busy &&
		!gate->blocked_dependency_count &&
		!gate->missing_dependency_count;

	return 0;
}

static int poulsbo_sgx_submit_sgx_ta_gate_status_locked(
	const struct psb_sgx_lab_submit_sgx_ta_gate *gate)
{
	unsigned int slot;

	if (!gate)
		return -EINVAL;
	if (gate->running_slot_busy)
		return -EBUSY;

	for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
		const struct psb_sgx_lab_submit_sgx_gate_slot *dependency;

		dependency = &gate->dependencies[slot];
		if (!dependency->active)
			continue;
		if (dependency->source_missing)
			return -ENOENT;
		if (dependency->blocked)
			return -EAGAIN;
	}

	return gate->can_enter_running ? 0 : -EINVAL;
}

static int poulsbo_sgx_build_submit_sgx_ta_control_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	bool have_issue_request,
	u32 issue_owner_state,
	u32 issued_packets,
	struct psb_sgx_lab_submit_sgx_ta_control *control)
{
	struct psb_sgx_lab_submit_sgx_issue_request *issue_request;
	int ret;

	if (!dev_priv || !submit || !control)
		return -EINVAL;

	memset(control, 0, sizeof(*control));

	ret = poulsbo_sgx_build_submit_sgx_ta_runtime_locked(
		dev_priv, submit, &control->runtime);
	if (ret)
		return ret;
	ret = poulsbo_sgx_build_submit_sgx_ta_gate_locked(
		submit, &control->runtime, &control->gate);
	if (ret)
		return ret;

	control->emit_matches_live =
		submit->emit_valid &&
		poulsbo_sgx_submit_emit_matches_locked(dev_priv, submit);
	control->have_launch_snapshot = submit->launch_valid;
	control->launch_consumed = submit->launch_consumed;
	control->launch_ready =
		poulsbo_sgx_submit_launch_base_matches_locked(dev_priv, submit);
	control->running_owned_by_self =
		control->runtime.coordination.active_running_valid &&
		control->runtime.coordination.active_running_submit_id ==
			submit->submit_id;

	if (!have_issue_request)
		return 0;

	issue_request = &control->issue_request;
	issue_request->active = true;
	issue_request->owner_state = issue_owner_state;
	issue_request->issued_packets = issued_packets;
	issue_request->issue_to_running =
		issue_owner_state == PSB_SGX_LAB_OWNER_RUNNING;
	issue_request->gate_ready =
		!issue_request->issue_to_running ||
		control->gate.can_enter_running;
	if (control->launch_ready) {
		issue_request->plan_buildable =
			poulsbo_sgx_build_issue_plan_locked(
				dev_priv, submit, issue_owner_state,
				issued_packets, &issue_request->plan);
	}
	if (issue_request->plan_buildable) {
		issue_request->plan_matches_launch =
			poulsbo_sgx_submit_issue_plan_matches_launch_locked(
				submit, &issue_request->plan);
	}
	issue_request->can_issue =
		issue_request->plan_matches_launch &&
		issue_request->gate_ready;

	return 0;
}

static int poulsbo_sgx_submit_sgx_ta_control_running_status_locked(
	const struct psb_sgx_lab_submit_sgx_ta_control *control)
{
	if (!control)
		return -EINVAL;

	return poulsbo_sgx_submit_sgx_ta_gate_status_locked(&control->gate);
}

static int poulsbo_sgx_submit_sgx_ta_control_issue_status_locked(
	const struct psb_sgx_lab_submit_sgx_ta_control *control)
{
	const struct psb_sgx_lab_submit_sgx_issue_request *issue_request;
	int ret;

	if (!control)
		return -EINVAL;

	issue_request = &control->issue_request;
	if (!issue_request->active)
		return -EINVAL;
	if (!control->have_launch_snapshot || control->launch_consumed)
		return -EINVAL;
	if (!control->launch_ready)
		return -ESTALE;
	if (issue_request->issue_to_running) {
		ret = poulsbo_sgx_submit_sgx_ta_control_running_status_locked(
			control);
		if (ret)
			return ret;
	}
	if (!issue_request->plan_buildable)
		return -EINVAL;
	if (!issue_request->plan_matches_launch)
		return -ESTALE;

	return issue_request->can_issue ? 0 : -EINVAL;
}

static void poulsbo_sgx_fill_submit_sgx_ta_schedule_from_control_locked(
	const struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_control *control,
	struct psb_sgx_lab_submit_sgx_ta_schedule *schedule)
{
	if (!submit || !control || !schedule)
		return;

	schedule->can_issue_queued =
		control->launch_ready &&
		!control->launch_consumed &&
		submit->owner_state == PSB_SGX_LAB_OWNER_QUEUED;
	schedule->can_issue_running =
		control->launch_ready &&
		!control->launch_consumed &&
		control->running_owned_by_self &&
		submit->owner_state == PSB_SGX_LAB_OWNER_RUNNING;
	schedule->can_enter_running =
		submit->owner_state == PSB_SGX_LAB_OWNER_QUEUED &&
		submit->queue_packet_count &&
		control->gate.can_enter_running;
	schedule->can_stay_running =
		submit->owner_state == PSB_SGX_LAB_OWNER_RUNNING &&
		submit->global_running_ticket &&
		submit->queue_packet_count;
	schedule->can_complete =
		submit->owner_state == PSB_SGX_LAB_OWNER_RUNNING &&
		submit->queue_packet_count;
}

static int poulsbo_sgx_build_submit_sgx_ta_schedule_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	bool have_transition_request,
	u32 transition_owner_state,
	u32 completed_packets,
	struct psb_sgx_lab_submit_sgx_ta_schedule *schedule)
{
	struct psb_sgx_lab_submit_sgx_transition_request *transition_request;
	u64 read_advance;
	u32 read_advance_mod;
	int ret;

	if (!dev_priv || !submit || !schedule)
		return -EINVAL;

	memset(schedule, 0, sizeof(*schedule));

	ret = poulsbo_sgx_build_submit_sgx_ta_control_locked(
		dev_priv, submit, false, 0, 0, &schedule->control);
	if (ret)
		return ret;

	poulsbo_sgx_fill_submit_sgx_ta_schedule_from_control_locked(
		submit, &schedule->control, schedule);

	if (!have_transition_request)
		return 0;

	transition_request = &schedule->transition_request;
	transition_request->active = true;
	transition_request->owner_state = transition_owner_state;
	transition_request->completed_packets = completed_packets;

	if (transition_owner_state != PSB_SGX_LAB_OWNER_QUEUED &&
	    transition_owner_state != PSB_SGX_LAB_OWNER_RUNNING &&
	    transition_owner_state != PSB_SGX_LAB_OWNER_COMPLETE)
		return -EINVAL;
	if (completed_packets > submit->queue_packet_count)
		return -EINVAL;
	if (submit->owner_state == PSB_SGX_LAB_OWNER_COMPLETE)
		return -EINVAL;

	if (transition_owner_state == PSB_SGX_LAB_OWNER_QUEUED) {
		if (submit->owner_state != PSB_SGX_LAB_OWNER_QUEUED ||
		    completed_packets)
			return -EINVAL;
	} else if (transition_owner_state == PSB_SGX_LAB_OWNER_RUNNING) {
		if (submit->owner_state != PSB_SGX_LAB_OWNER_QUEUED &&
		    submit->owner_state != PSB_SGX_LAB_OWNER_RUNNING)
			return -EINVAL;
		if (submit->queue_packet_count == completed_packets)
			return -EINVAL;
	} else if (transition_owner_state == PSB_SGX_LAB_OWNER_COMPLETE) {
		if (submit->owner_state != PSB_SGX_LAB_OWNER_RUNNING ||
		    completed_packets != submit->queue_packet_count)
			return -EINVAL;
	}

	if (submit->owner_state == PSB_SGX_LAB_OWNER_RUNNING &&
	    (!dev_priv->sgx_lab_running_active ||
	     dev_priv->sgx_lab_active_running_submit_id != submit->submit_id))
		return -EINVAL;

	transition_request->entering_running =
		transition_owner_state == PSB_SGX_LAB_OWNER_RUNNING &&
		submit->owner_state != PSB_SGX_LAB_OWNER_RUNNING;
	transition_request->entering_complete =
		transition_owner_state == PSB_SGX_LAB_OWNER_COMPLETE;

	if (transition_request->entering_running) {
		ret = poulsbo_sgx_submit_sgx_ta_control_running_status_locked(
			&schedule->control);
		if (ret)
			return ret;
		if (dev_priv->sgx_lab_global_running_ticket == U32_MAX)
			return -EOVERFLOW;
		transition_request->new_global_running_ticket =
			dev_priv->sgx_lab_global_running_ticket + 1;
		transition_request->can_transition = true;
	} else if (transition_owner_state == PSB_SGX_LAB_OWNER_RUNNING) {
		if (!submit->global_running_ticket)
			return -EINVAL;
		transition_request->new_global_running_ticket =
			submit->global_running_ticket;
		transition_request->can_transition = true;
	} else {
		transition_request->new_global_running_ticket =
			submit->global_running_ticket;
		transition_request->can_transition =
			transition_owner_state == PSB_SGX_LAB_OWNER_COMPLETE ||
			transition_owner_state == PSB_SGX_LAB_OWNER_QUEUED;
	}

	transition_request->new_packet_count =
		submit->queue_packet_count - completed_packets;
	transition_request->new_read_offset = submit->ccb_read_offset;
	if (completed_packets) {
		read_advance = (u64)completed_packets * submit->queue_packet_size;
		div_u64_rem(read_advance, submit->queue_size,
			    &read_advance_mod);
		transition_request->new_read_offset =
			(submit->ccb_read_offset + read_advance_mod) %
			submit->queue_size;
	}

	ret = poulsbo_sgx_queue_progression(
		transition_request->new_read_offset, submit->queue_size,
		submit->queue_packet_size, transition_request->new_packet_count,
		&transition_request->new_tail_packets,
		&transition_request->new_head_packets,
		&transition_request->new_write_offset);
	if (ret)
		return ret;

	transition_request->new_ccb_offset =
		submit->queue_base_offset +
		transition_request->new_read_offset;
	transition_request->new_sync_complete_seq =
		submit->sync_complete_seq + completed_packets;
	transition_request->new_owner_complete_cookie =
		submit->owner_complete_cookie + completed_packets;
	transition_request->new_global_complete_ticket =
		submit->global_complete_ticket;
	if (transition_request->entering_complete) {
		if (dev_priv->sgx_lab_global_complete_ticket == U32_MAX)
			return -EOVERFLOW;
		transition_request->new_global_complete_ticket =
			dev_priv->sgx_lab_global_complete_ticket + 1;
	}

	transition_request->new_sync_flags = PSB_SGX_LAB_SYNC_FLAG_READY;
	if (transition_request->new_head_packets) {
		transition_request->new_sync_flags |=
			PSB_SGX_LAB_SYNC_FLAG_WRAPPED;
	}

	return 0;
}

static int poulsbo_sgx_submit_sgx_ta_schedule_transition_status_locked(
	const struct psb_sgx_lab_submit_sgx_ta_schedule *schedule)
{
	const struct psb_sgx_lab_submit_sgx_transition_request *transition_request;

	if (!schedule)
		return -EINVAL;

	transition_request = &schedule->transition_request;
	if (!transition_request->active)
		return -EINVAL;

	return transition_request->can_transition ? 0 : -EINVAL;
}

static int poulsbo_sgx_build_submit_sgx_ta_dispatch_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	bool have_issue_request,
	u32 issue_owner_state,
	u32 issued_packets,
	struct psb_sgx_lab_submit_sgx_ta_dispatch *dispatch)
{
	struct psb_sgx_lab_submit_sgx_issue_image *issue_image;
	int ret;

	if (!dev_priv || !submit || !dispatch)
		return -EINVAL;

	memset(dispatch, 0, sizeof(*dispatch));

	ret = poulsbo_sgx_build_submit_sgx_ta_control_locked(
		dev_priv, submit, have_issue_request, issue_owner_state,
		issued_packets, &dispatch->schedule.control);
	if (ret)
		return ret;

	poulsbo_sgx_fill_submit_sgx_ta_schedule_from_control_locked(
		submit, &dispatch->schedule.control, &dispatch->schedule);

	issue_image = &dispatch->issue_image;
	issue_image->active = submit->launch_valid;
	if (!issue_image->active)
		return 0;

	ret = poulsbo_sgx_queue_region_end(submit->launch_queue_base_offset,
					   submit->launch_queue_size,
					   &issue_image->queue_region_end);
	if (ret)
		return ret;
	ret = poulsbo_sgx_sync_region_end(submit->launch_sync_offset,
					  &issue_image->sync_region_end);
	if (ret)
		return ret;
	ret = poulsbo_sgx_owner_region_end(submit->launch_owner_offset,
					   &issue_image->owner_region_end);
	if (ret)
		return ret;

	issue_image->issue_write_offset = submit->launch_ccb_write_offset;
	issue_image->final_write_offset = submit->launch_new_write_offset;

	memset(&issue_image->queue_header, 0, sizeof(issue_image->queue_header));
	issue_image->queue_header.write_offset = submit->launch_new_write_offset;
	issue_image->queue_header.read_offset = submit->launch_ccb_read_offset;
	issue_image->queue_header.queue_base_offset =
		submit->launch_queue_base_offset;
	issue_image->queue_header.queue_size = submit->launch_queue_size;
	issue_image->queue_header.queue_packet_size =
		submit->launch_queue_packet_size;
	issue_image->queue_header.queue_packet_count =
		submit->launch_queue_packet_count;
	issue_image->queue_header.queue_tail_packet_count =
		submit->launch_queue_tail_packet_count;
	issue_image->queue_header.queue_head_packet_count =
		submit->launch_queue_head_packet_count;

	memset(&issue_image->ccb_record, 0, sizeof(issue_image->ccb_record));
	issue_image->ccb_record.service_address =
		submit->launch_ccb_service_address;
	issue_image->ccb_record.cache_control =
		submit->launch_ccb_cache_control;
	issue_image->ccb_record.data0 = submit->launch_ccb_data0;
	issue_image->ccb_record.data1 = submit->launch_ccb_data1;
	issue_image->ccb_record.pds_gpu_va32 = (u32)submit->launch_pds_gpu_va;
	issue_image->ccb_record.use_gpu_va32 = (u32)submit->launch_use_gpu_va;
	issue_image->ccb_record.target_gpu_va32 =
		(u32)submit->launch_target_gpu_va;
	issue_image->ccb_record.status_gpu_va32 =
		(u32)submit->launch_status_gpu_va;

	memset(&issue_image->sync, 0, sizeof(issue_image->sync));
	issue_image->sync.read_offset = submit->launch_ccb_read_offset;
	issue_image->sync.write_offset = submit->launch_new_write_offset;
	issue_image->sync.complete_seq = submit->launch_sync_complete_seq;
	issue_image->sync.pending_seq = submit->launch_sync_pending_seq;
	issue_image->sync.flags = submit->launch_sync_flags;
	issue_image->sync.epoch = submit->launch_queue_epoch;
	issue_image->sync.global_issue_ticket =
		submit->launch_global_issue_ticket;

	memset(&issue_image->owner, 0, sizeof(issue_image->owner));
	issue_image->owner.owner_state = submit->launch_owner_state;
	issue_image->owner.owner_issue_cookie =
		submit->launch_owner_issue_cookie;
	issue_image->owner.owner_complete_cookie =
		submit->launch_owner_complete_cookie;
	issue_image->owner.epoch = submit->launch_queue_epoch;
	issue_image->owner.global_complete_ticket =
		submit->launch_global_complete_ticket;
	issue_image->owner.global_running_ticket =
		submit->launch_global_running_ticket;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_path_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	bool have_issue_request,
	u32 issue_owner_state,
	u32 issued_packets,
	bool have_transition_request,
	u32 transition_owner_state,
	u32 completed_packets,
	struct psb_sgx_lab_submit_sgx_ta_path *path)
{
	struct psb_sgx_lab_submit_sgx_transition_image *transition_image;
	int ret;

	if (!dev_priv || !submit || !path)
		return -EINVAL;
	if (have_issue_request && have_transition_request)
		return -EINVAL;

	memset(path, 0, sizeof(*path));

	ret = poulsbo_sgx_build_submit_sgx_ta_dispatch_locked(
		dev_priv, submit, have_issue_request, issue_owner_state,
		issued_packets, &path->dispatch);
	if (ret)
		return ret;

	if (!have_transition_request)
		return 0;

	ret = poulsbo_sgx_build_submit_sgx_ta_schedule_locked(
		dev_priv, submit, true, transition_owner_state,
		completed_packets, &path->dispatch.schedule);
	if (ret)
		return ret;

	transition_image = &path->transition_image;
	transition_image->active = true;

	ret = poulsbo_sgx_queue_region_end(submit->queue_base_offset,
					   submit->queue_size,
					   &transition_image->queue_region_end);
	if (ret)
		return ret;
	ret = poulsbo_sgx_sync_region_end(submit->sync_offset,
					  &transition_image->sync_region_end);
	if (ret)
		return ret;
	ret = poulsbo_sgx_owner_region_end(submit->owner_offset,
					   &transition_image->owner_region_end);
	if (ret)
		return ret;

	memset(&transition_image->queue_header, 0,
	       sizeof(transition_image->queue_header));
	transition_image->queue_header.write_offset =
		path->dispatch.schedule.transition_request.new_write_offset;
	transition_image->queue_header.read_offset =
		path->dispatch.schedule.transition_request.new_read_offset;
	transition_image->queue_header.queue_base_offset =
		submit->queue_base_offset;
	transition_image->queue_header.queue_size = submit->queue_size;
	transition_image->queue_header.queue_packet_size =
		submit->queue_packet_size;
	transition_image->queue_header.queue_packet_count =
		path->dispatch.schedule.transition_request.new_packet_count;
	transition_image->queue_header.queue_tail_packet_count =
		path->dispatch.schedule.transition_request.new_tail_packets;
	transition_image->queue_header.queue_head_packet_count =
		path->dispatch.schedule.transition_request.new_head_packets;

	memset(&transition_image->ccb_record, 0,
	       sizeof(transition_image->ccb_record));
	transition_image->ccb_record.service_address =
		submit->ccb_service_address;
	transition_image->ccb_record.cache_control =
		submit->ccb_cache_control;
	transition_image->ccb_record.data0 = submit->ccb_data0;
	transition_image->ccb_record.data1 = submit->ccb_data1;
	ret = poulsbo_sgx_resolve_submit_ccb_gpu_va32_locked(
		dev_priv, submit, &transition_image->ccb_record.pds_gpu_va32,
		&transition_image->ccb_record.use_gpu_va32,
		&transition_image->ccb_record.target_gpu_va32,
		&transition_image->ccb_record.status_gpu_va32);
	if (ret)
		return ret;

	memset(&transition_image->sync, 0, sizeof(transition_image->sync));
	transition_image->sync.read_offset =
		path->dispatch.schedule.transition_request.new_read_offset;
	transition_image->sync.write_offset =
		path->dispatch.schedule.transition_request.new_write_offset;
	transition_image->sync.complete_seq =
		path->dispatch.schedule.transition_request.new_sync_complete_seq;
	transition_image->sync.pending_seq = submit->sync_pending_seq;
	transition_image->sync.flags =
		path->dispatch.schedule.transition_request.new_sync_flags;
	transition_image->sync.epoch = submit->queue_epoch;
	transition_image->sync.global_issue_ticket =
		submit->global_issue_ticket;

	memset(&transition_image->owner, 0, sizeof(transition_image->owner));
	transition_image->owner.owner_state =
		path->dispatch.schedule.transition_request.owner_state;
	transition_image->owner.owner_issue_cookie =
		submit->owner_issue_cookie;
	transition_image->owner.owner_complete_cookie =
		path->dispatch.schedule.transition_request
			.new_owner_complete_cookie;
	transition_image->owner.epoch = submit->queue_epoch;
	transition_image->owner.global_complete_ticket =
		path->dispatch.schedule.transition_request
			.new_global_complete_ticket;
	transition_image->owner.global_running_ticket =
		path->dispatch.schedule.transition_request
			.new_global_running_ticket;

	return 0;
}

static int poulsbo_sgx_submit_sgx_ta_dispatch_issue_status_locked(
	const struct psb_sgx_lab_submit_sgx_ta_dispatch *dispatch)
{
	if (!dispatch)
		return -EINVAL;
	if (!dispatch->issue_image.active)
		return -EINVAL;

	return poulsbo_sgx_submit_sgx_ta_control_issue_status_locked(
		&dispatch->schedule.control);
}

static int poulsbo_sgx_submit_sgx_ta_path_transition_status_locked(
	const struct psb_sgx_lab_submit_sgx_ta_path *path)
{
	if (!path)
		return -EINVAL;
	if (!path->transition_image.active)
		return -EINVAL;

	return poulsbo_sgx_submit_sgx_ta_schedule_transition_status_locked(
		&path->dispatch.schedule);
}

static int poulsbo_sgx_submit_sgx_ta_path_issue_status_locked(
	const struct psb_sgx_lab_submit_sgx_ta_path *path)
{
	if (!path)
		return -EINVAL;

	return poulsbo_sgx_submit_sgx_ta_dispatch_issue_status_locked(
		&path->dispatch);
}

static void poulsbo_sgx_fill_submit_sgx_ta_commit_state_from_issue_locked(
	const struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_path *path,
	struct psb_sgx_lab_submit_sgx_ta_commit_state *next_state)
{
	const struct psb_sgx_lab_submit_sgx_issue_image *issue_image;

	if (!submit || !path || !next_state)
		return;

	issue_image = &path->dispatch.issue_image;
	memset(next_state, 0, sizeof(*next_state));
	next_state->ccb_offset = submit->ccb_offset;
	next_state->ccb_read_offset = issue_image->sync.read_offset;
	next_state->ccb_write_offset = issue_image->queue_header.write_offset;
	next_state->queue_packet_count =
		issue_image->queue_header.queue_packet_count;
	next_state->queue_tail_packet_count =
		issue_image->queue_header.queue_tail_packet_count;
	next_state->queue_head_packet_count =
		issue_image->queue_header.queue_head_packet_count;
	next_state->sync_complete_seq = issue_image->sync.complete_seq;
	next_state->sync_pending_seq = issue_image->sync.pending_seq;
	next_state->sync_flags = issue_image->sync.flags;
	next_state->owner_state = issue_image->owner.owner_state;
	next_state->owner_issue_cookie =
		issue_image->owner.owner_issue_cookie;
	next_state->owner_complete_cookie =
		issue_image->owner.owner_complete_cookie;
	next_state->global_issue_ticket =
		issue_image->sync.global_issue_ticket;
	next_state->global_running_ticket =
		issue_image->owner.global_running_ticket;
	next_state->global_complete_ticket =
		issue_image->owner.global_complete_ticket;
	next_state->queue_epoch = issue_image->sync.epoch;
	next_state->launch_consumed = true;
}

static void
poulsbo_sgx_fill_submit_sgx_ta_commit_state_from_transition_locked(
	const struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_path *path,
	struct psb_sgx_lab_submit_sgx_ta_commit_state *next_state)
{
	const struct psb_sgx_lab_submit_sgx_transition_request *transition_request;

	if (!submit || !path || !next_state)
		return;

	transition_request = &path->dispatch.schedule.transition_request;
	memset(next_state, 0, sizeof(*next_state));
	next_state->ccb_offset = transition_request->new_ccb_offset;
	next_state->ccb_read_offset = transition_request->new_read_offset;
	next_state->ccb_write_offset = transition_request->new_write_offset;
	next_state->queue_packet_count = transition_request->new_packet_count;
	next_state->queue_tail_packet_count =
		transition_request->new_tail_packets;
	next_state->queue_head_packet_count =
		transition_request->new_head_packets;
	next_state->sync_complete_seq =
		transition_request->new_sync_complete_seq;
	next_state->sync_pending_seq = submit->sync_pending_seq;
	next_state->sync_flags = transition_request->new_sync_flags;
	next_state->owner_state = transition_request->owner_state;
	next_state->owner_issue_cookie = submit->owner_issue_cookie;
	next_state->owner_complete_cookie =
		transition_request->new_owner_complete_cookie;
	next_state->global_issue_ticket = submit->global_issue_ticket;
	next_state->global_running_ticket =
		transition_request->new_global_running_ticket;
	next_state->global_complete_ticket =
		transition_request->new_global_complete_ticket;
	next_state->queue_epoch = submit->queue_epoch;
	next_state->launch_consumed = submit->launch_consumed;
}

static int poulsbo_sgx_build_submit_sgx_ta_commit_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	bool have_issue_request,
	u32 issue_owner_state,
	u32 issued_packets,
	bool have_transition_request,
	u32 transition_owner_state,
	u32 completed_packets,
	struct psb_sgx_lab_submit_sgx_ta_commit *commit)
{
	int ret;

	if (!dev_priv || !submit || !commit)
		return -EINVAL;
	if (have_issue_request && have_transition_request)
		return -EINVAL;

	memset(commit, 0, sizeof(*commit));

	ret = poulsbo_sgx_build_submit_sgx_ta_path_locked(
		dev_priv, submit, have_issue_request, issue_owner_state,
		issued_packets, have_transition_request, transition_owner_state,
		completed_packets, &commit->path);
	if (ret)
		return ret;

	if (have_issue_request) {
		commit->issue_active = true;
		commit->control_object_id = submit->launch_control_object_id;
		commit->status_object_id = submit->launch_status_object_id;
		commit->pds_object_id = submit->launch_pds_object_id;
		commit->use_object_id = submit->launch_use_object_id;
		commit->target_object_id = submit->launch_target_object_id;
		commit->issue_queue_base_offset = submit->launch_queue_base_offset;
		commit->issue_queue_size = submit->launch_queue_size;
		commit->issue_write_offset =
			commit->path.dispatch.issue_image.issue_write_offset;
		commit->issue_packet_size = submit->launch_queue_packet_size;
		commit->issue_packet_count = issued_packets;
		commit->sync_offset = submit->launch_sync_offset;
		commit->owner_offset = submit->launch_owner_offset;
		poulsbo_sgx_fill_submit_sgx_ta_commit_state_from_issue_locked(
			submit, &commit->path, &commit->next_state);
		return 0;
	}

	if (!have_transition_request)
		return 0;

	commit->transition_active = true;
	commit->control_object_id = submit->control_object_id;
	commit->status_object_id = submit->status_object_id;
	commit->transition_ccb_offset =
		commit->path.dispatch.schedule.transition_request
			.new_ccb_offset;
	commit->sync_offset = submit->sync_offset;
	commit->owner_offset = submit->owner_offset;
	commit->completed_packets = completed_packets;
	commit->entering_running =
		commit->path.dispatch.schedule.transition_request
			.entering_running;
	commit->entering_complete =
		commit->path.dispatch.schedule.transition_request
			.entering_complete;
	poulsbo_sgx_fill_submit_sgx_ta_commit_state_from_transition_locked(
		submit, &commit->path, &commit->next_state);
	return 0;
}

static int poulsbo_sgx_submit_sgx_ta_commit_issue_status_locked(
	const struct psb_sgx_lab_submit_sgx_ta_commit *commit)
{
	if (!commit)
		return -EINVAL;
	if (!commit->issue_active || commit->transition_active)
		return -EINVAL;

	return poulsbo_sgx_submit_sgx_ta_path_issue_status_locked(
		&commit->path);
}

static int poulsbo_sgx_submit_sgx_ta_commit_transition_status_locked(
	const struct psb_sgx_lab_submit_sgx_ta_commit *commit)
{
	if (!commit)
		return -EINVAL;
	if (!commit->transition_active || commit->issue_active)
		return -EINVAL;

	return poulsbo_sgx_submit_sgx_ta_path_transition_status_locked(
		&commit->path);
}

static void poulsbo_sgx_apply_submit_sgx_ta_commit_state_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_commit_state *next_state)
{
	if (!submit || !next_state)
		return;

	submit->ccb_offset = next_state->ccb_offset;
	submit->ccb_read_offset = next_state->ccb_read_offset;
	submit->ccb_write_offset = next_state->ccb_write_offset;
	submit->queue_packet_count = next_state->queue_packet_count;
	submit->queue_tail_packet_count = next_state->queue_tail_packet_count;
	submit->queue_head_packet_count = next_state->queue_head_packet_count;
	submit->sync_complete_seq = next_state->sync_complete_seq;
	submit->sync_pending_seq = next_state->sync_pending_seq;
	submit->sync_flags = next_state->sync_flags;
	submit->owner_state = next_state->owner_state;
	submit->owner_issue_cookie = next_state->owner_issue_cookie;
	submit->owner_complete_cookie = next_state->owner_complete_cookie;
	submit->global_issue_ticket = next_state->global_issue_ticket;
	submit->global_running_ticket = next_state->global_running_ticket;
	submit->global_complete_ticket = next_state->global_complete_ticket;
	submit->queue_epoch = next_state->queue_epoch;
	submit->launch_consumed = next_state->launch_consumed;
}

static int poulsbo_sgx_apply_submit_sgx_ta_commit_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_commit *commit)
{
	const struct psb_sgx_lab_submit_sgx_issue_image *issue_image;
	const struct psb_sgx_lab_submit_sgx_transition_image *transition_image;
	struct psb_sgx_lab_object *control_obj;
	struct psb_sgx_lab_object *status_obj;
	struct psb_sgx_lab_object *pds_obj;
	struct psb_sgx_lab_object *use_obj;
	struct psb_sgx_lab_object *target_obj;
	u32 issue_write_offset;
	u32 slot_offset;
	u32 index;
	int ret;

	if (!dev_priv || !submit || !commit)
		return -EINVAL;
	if (!!commit->issue_active == !!commit->transition_active)
		return -EINVAL;

	control_obj = poulsbo_sgx_find_object_locked(dev_priv,
						     commit->control_object_id);
	status_obj = poulsbo_sgx_find_object_locked(dev_priv,
						    commit->status_object_id);
	if (!control_obj || !status_obj)
		return -ENOENT;

	if (commit->issue_active) {
		pds_obj = poulsbo_sgx_find_object_locked(dev_priv,
							 commit->pds_object_id);
		use_obj = poulsbo_sgx_find_object_locked(dev_priv,
							 commit->use_object_id);
		target_obj = poulsbo_sgx_find_object_locked(dev_priv,
							    commit->target_object_id);
		if (!pds_obj || !use_obj || !target_obj)
			return -ENOENT;

		issue_image = &commit->path.dispatch.issue_image;
		if (issue_image->queue_region_end > control_obj->gt->gem.size)
			return -EINVAL;
		ret = poulsbo_sgx_object_access_ok(control_obj, 0, 32);
		if (ret)
			return ret;
		ret = poulsbo_sgx_object_access_ok(
			status_obj, commit->sync_offset,
			issue_image->sync_region_end - commit->sync_offset);
		if (ret)
			return ret;
		ret = poulsbo_sgx_object_access_ok(
			status_obj, commit->owner_offset,
			issue_image->owner_region_end - commit->owner_offset);
		if (ret)
			return ret;

		issue_write_offset = commit->issue_write_offset;
		for (index = 0; index < commit->issue_packet_count; index++) {
			slot_offset = commit->issue_queue_base_offset +
				issue_write_offset;
			ret = poulsbo_sgx_object_access_ok(
				control_obj, slot_offset,
				PSB_SGX_LAB_CCB_RECORD_BYTES);
			if (ret)
				return ret;
			ret = poulsbo_sgx_write_submit_ccb_record_locked(
				control_obj, slot_offset,
				&issue_image->ccb_record);
			if (ret)
				return ret;
			issue_write_offset += commit->issue_packet_size;
			if (issue_write_offset >= commit->issue_queue_size)
				issue_write_offset -= commit->issue_queue_size;
		}
		if (issue_write_offset != issue_image->final_write_offset)
			return -EINVAL;

		ret = poulsbo_sgx_write_queue_header_locked(
			control_obj, 0, &issue_image->queue_header);
		if (ret)
			return ret;
		ret = poulsbo_sgx_write_submit_sync_locked(
			status_obj, commit->sync_offset, &issue_image->sync);
		if (ret)
			return ret;
		ret = poulsbo_sgx_write_submit_owner_locked(
			status_obj, commit->owner_offset, &issue_image->owner);
		if (ret)
			return ret;

		poulsbo_sgx_apply_submit_sgx_ta_commit_state_locked(
			submit, &commit->next_state);
		poulsbo_sgx_record_dispatch_submit_locked(submit);
		poulsbo_sgx_clear_preissue_submit_locked(submit);
		return 0;
	}

	transition_image = &commit->path.transition_image;
	if (transition_image->queue_region_end > control_obj->gt->gem.size ||
	    commit->transition_ccb_offset + PSB_SGX_LAB_CCB_RECORD_BYTES >
		    transition_image->queue_region_end)
		return -EINVAL;
	ret = poulsbo_sgx_object_access_ok(control_obj, 0, 32);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_access_ok(control_obj,
					   commit->transition_ccb_offset,
					   PSB_SGX_LAB_CCB_RECORD_BYTES);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_access_ok(
		status_obj, commit->sync_offset,
		transition_image->sync_region_end - commit->sync_offset);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_access_ok(
		status_obj, commit->owner_offset,
		transition_image->owner_region_end - commit->owner_offset);
	if (ret)
		return ret;

	ret = poulsbo_sgx_write_queue_header_locked(control_obj, 0,
						    &transition_image->queue_header);
	if (ret)
		return ret;
	ret = poulsbo_sgx_write_submit_ccb_record_locked(
		control_obj, commit->transition_ccb_offset,
		&transition_image->ccb_record);
	if (ret)
		return ret;
	ret = poulsbo_sgx_write_submit_sync_locked(
		status_obj, commit->sync_offset, &transition_image->sync);
	if (ret)
		return ret;
	ret = poulsbo_sgx_write_submit_owner_locked(
		status_obj, commit->owner_offset, &transition_image->owner);
	if (ret)
		return ret;

	poulsbo_sgx_apply_submit_sgx_ta_commit_state_locked(
		submit, &commit->next_state);
	poulsbo_sgx_update_dispatch_submit_locked(
		submit, commit->next_state.owner_state,
		commit->completed_packets);
	if (commit->entering_complete) {
		poulsbo_sgx_retire_submit_locked(submit);
		ret = poulsbo_sgx_refresh_dependency_dependents_locked(
			dev_priv, submit->submit_id);
		if (ret)
			return ret;
	}
	poulsbo_sgx_disarm_submit_locked(submit);
	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_event_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	bool have_issue_request,
	u32 issue_owner_state,
	u32 issued_packets,
	bool have_transition_request,
	u32 transition_owner_state,
	u32 completed_packets,
	struct psb_sgx_lab_submit_sgx_ta_event *event)
{
	int ret;

	if (!dev_priv || !submit || !event)
		return -EINVAL;
	if (have_issue_request && have_transition_request)
		return -EINVAL;

	memset(event, 0, sizeof(*event));

	ret = poulsbo_sgx_build_submit_sgx_ta_commit_locked(
		dev_priv, submit, have_issue_request, issue_owner_state,
		issued_packets, have_transition_request, transition_owner_state,
		completed_packets, &event->commit);
	if (ret)
		return ret;

	if (have_issue_request) {
		ret = poulsbo_sgx_submit_sgx_ta_commit_issue_status_locked(
			&event->commit);
		if (ret)
			return ret;
		event->refresh_views = true;
		event->increment_issue_count = true;
		event->update_last_epoch_jiffies =
			submit->launch_refilled_from_complete;
		event->update_last_issue_jiffies = true;
		return 0;
	}

	if (!have_transition_request)
		return 0;

	ret = poulsbo_sgx_submit_sgx_ta_commit_transition_status_locked(
		&event->commit);
	if (ret)
		return ret;
	event->refresh_views = true;
	event->increment_transition_count = true;
	event->update_last_transition_jiffies = true;
	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_event_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_event *event)
{
	if (!dev_priv || !submit || !event)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_commit_locked(
		dev_priv, submit, &event->commit);
}

static int poulsbo_sgx_build_submit_sgx_ta_work_item_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	bool have_issue_request,
	u32 issue_owner_state,
	u32 issued_packets,
	bool have_transition_request,
	u32 transition_owner_state,
	u32 completed_packets,
	struct psb_sgx_lab_submit_sgx_ta_work_item *work_item)
{
	int ret;

	if (!dev_priv || !submit || !work_item)
		return -EINVAL;
	if (have_issue_request && have_transition_request)
		return -EINVAL;

	memset(work_item, 0, sizeof(*work_item));
	work_item->submit_id = submit->submit_id;

	if (!submit->control_object_id || !submit->status_object_id ||
	    !submit->queue_config_count || !submit->ccb_config_count ||
	    !submit->sync_config_count || !submit->owner_config_count)
		return -EINVAL;

	if (have_issue_request) {
		if (issue_owner_state != PSB_SGX_LAB_OWNER_QUEUED &&
		    issue_owner_state != PSB_SGX_LAB_OWNER_RUNNING)
			return -EINVAL;
		if (!issued_packets)
			return -EINVAL;
		work_item->validate_before_apply = true;
		work_item->disarm_on_build_failure = true;
		work_item->owner_state = issue_owner_state;
		work_item->issued_packets = issued_packets;
	} else if (have_transition_request) {
		if (transition_owner_state != PSB_SGX_LAB_OWNER_QUEUED &&
		    transition_owner_state != PSB_SGX_LAB_OWNER_RUNNING &&
		    transition_owner_state != PSB_SGX_LAB_OWNER_COMPLETE)
			return -EINVAL;
		if (completed_packets > submit->queue_packet_count)
			return -EINVAL;
		work_item->owner_state = transition_owner_state;
		work_item->completed_packets = completed_packets;
	}

	ret = poulsbo_sgx_build_submit_sgx_ta_event_locked(
		dev_priv, submit, have_issue_request, issue_owner_state,
		issued_packets, have_transition_request, transition_owner_state,
		completed_packets, &work_item->event);
	if (ret) {
		if (work_item->disarm_on_build_failure && ret == -ESTALE)
			poulsbo_sgx_disarm_submit_locked(submit);
		return ret;
	}

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_work_item_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_work_item *work_item)
{
	int ret;

	if (!dev_priv || !submit || !work_item)
		return -EINVAL;

	if (work_item->validate_before_apply) {
		ret = poulsbo_sgx_validate_submit_locked(dev_priv, submit);
		if (ret) {
			poulsbo_sgx_disarm_submit_locked(submit);
			return ret;
		}
	}

	return poulsbo_sgx_apply_submit_sgx_ta_event_locked(
		dev_priv, submit, &work_item->event);
}

static int poulsbo_sgx_build_submit_sgx_ta_op_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 op_kind,
	u32 owner_state,
	u32 packet_count,
	struct psb_sgx_lab_submit_sgx_ta_op *ta_op)
{
	bool have_issue_request = false;
	bool have_transition_request = false;
	u32 issue_owner_state = 0;
	u32 transition_owner_state = 0;
	u32 issued_packets = 0;
	u32 completed_packets = 0;
	int ret;

	if (!dev_priv || !submit || !ta_op)
		return -EINVAL;

	memset(ta_op, 0, sizeof(*ta_op));
	ta_op->submit_id = submit->submit_id;

	switch (op_kind) {
	case PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE:
		if (owner_state || packet_count)
			return -EINVAL;
		break;
	case PSB_SGX_LAB_SUBMIT_SGX_TA_OP_ISSUE:
		if (owner_state != PSB_SGX_LAB_OWNER_QUEUED &&
		    owner_state != PSB_SGX_LAB_OWNER_RUNNING)
			return -EINVAL;
		if (!packet_count)
			return -EINVAL;
		have_issue_request = true;
		issue_owner_state = owner_state;
		issued_packets = packet_count;
		break;
	case PSB_SGX_LAB_SUBMIT_SGX_TA_OP_TRANSITION:
		if (owner_state != PSB_SGX_LAB_OWNER_QUEUED &&
		    owner_state != PSB_SGX_LAB_OWNER_RUNNING &&
		    owner_state != PSB_SGX_LAB_OWNER_COMPLETE)
			return -EINVAL;
		have_transition_request = true;
		transition_owner_state = owner_state;
		completed_packets = packet_count;
		break;
	default:
		return -EINVAL;
	}

	ta_op->op_kind = op_kind;
	ta_op->owner_state = owner_state;
	ta_op->packet_count = packet_count;

	ret = poulsbo_sgx_build_submit_sgx_ta_work_item_locked(
		dev_priv, submit, have_issue_request, issue_owner_state,
		issued_packets, have_transition_request,
		transition_owner_state, completed_packets, &ta_op->work_item);
	if (ret)
		return ret;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_op_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_op *ta_op)
{
	if (!dev_priv || !submit || !ta_op)
		return -EINVAL;
	if (ta_op->submit_id != submit->submit_id)
		return -EINVAL;
	if (ta_op->op_kind != PSB_SGX_LAB_SUBMIT_SGX_TA_OP_ISSUE &&
	    ta_op->op_kind != PSB_SGX_LAB_SUBMIT_SGX_TA_OP_TRANSITION)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_work_item_locked(
		dev_priv, submit, &ta_op->work_item);
}

static void poulsbo_sgx_fill_submit_sgx_scheduler_state_from_coordination(
	const struct psb_sgx_lab_submit_sgx_coordination *coordination,
	struct psb_sgx_lab_submit_sgx_scheduler_state *state)
{
	if (!state)
		return;

	memset(state, 0, sizeof(*state));
	if (!coordination)
		return;

	state->issue_ticket = coordination->issue_ticket;
	state->running_ticket = coordination->running_ticket;
	state->complete_ticket = coordination->complete_ticket;
	state->issue_submit_id = coordination->issue_submit_id;
	state->running_submit_id = coordination->running_submit_id;
	state->complete_submit_id = coordination->complete_submit_id;
	state->active_running_valid = coordination->active_running_valid;
	state->active_running_submit_id =
		coordination->active_running_submit_id;
}

static int poulsbo_sgx_build_submit_sgx_ta_scheduler_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 op_kind,
	u32 owner_state,
	u32 packet_count,
	struct psb_sgx_lab_submit_sgx_ta_scheduler *scheduler)
{
	const struct psb_sgx_lab_submit_sgx_ta_commit *commit;
	const struct psb_sgx_lab_submit_sgx_coordination *coordination;
	int ret;

	if (!dev_priv || !submit || !scheduler)
		return -EINVAL;

	memset(scheduler, 0, sizeof(*scheduler));

	ret = poulsbo_sgx_build_submit_sgx_ta_op_locked(
		dev_priv, submit, op_kind, owner_state, packet_count,
		&scheduler->ta_op);
	if (ret)
		return ret;
	if (op_kind == PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE)
		return 0;

	commit = &scheduler->ta_op.work_item.event.commit;
	coordination =
		&commit->path.dispatch.schedule.control.runtime.coordination;
	poulsbo_sgx_fill_submit_sgx_scheduler_state_from_coordination(
		coordination, &scheduler->next_state);
	scheduler->active = true;

	switch (op_kind) {
	case PSB_SGX_LAB_SUBMIT_SGX_TA_OP_ISSUE:
		if (!commit->issue_active || commit->transition_active)
			return -EINVAL;
		scheduler->issue_advanced = true;
		scheduler->next_state.issue_ticket =
			commit->next_state.global_issue_ticket;
		scheduler->next_state.issue_submit_id = submit->submit_id;
		return 0;
	case PSB_SGX_LAB_SUBMIT_SGX_TA_OP_TRANSITION:
		if (!commit->transition_active || commit->issue_active)
			return -EINVAL;
		if (commit->next_state.owner_state == PSB_SGX_LAB_OWNER_RUNNING &&
		    commit->entering_running) {
			scheduler->running_advanced = true;
			scheduler->active_running_changed = true;
			scheduler->next_state.running_ticket =
				commit->next_state.global_running_ticket;
			scheduler->next_state.running_submit_id =
				submit->submit_id;
			scheduler->next_state.active_running_valid = true;
			scheduler->next_state.active_running_submit_id =
				submit->submit_id;
		} else if (commit->next_state.owner_state !=
				   PSB_SGX_LAB_OWNER_RUNNING &&
			   scheduler->next_state.active_running_valid &&
			   scheduler->next_state.active_running_submit_id ==
				   submit->submit_id) {
			scheduler->active_running_changed = true;
			scheduler->next_state.active_running_valid = false;
			scheduler->next_state.active_running_submit_id = 0;
		}
		if (commit->entering_complete) {
			scheduler->complete_advanced = true;
			scheduler->next_state.complete_ticket =
				commit->next_state.global_complete_ticket;
			scheduler->next_state.complete_submit_id =
				submit->submit_id;
		}
		return 0;
	default:
		return -EINVAL;
	}
}

static int poulsbo_sgx_apply_submit_sgx_ta_scheduler_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_scheduler *scheduler)
{
	int ret;

	if (!dev_priv || !submit || !scheduler)
		return -EINVAL;

	ret = poulsbo_sgx_apply_submit_sgx_ta_op_locked(
		dev_priv, submit, &scheduler->ta_op);
	if (ret)
		return ret;
	if (!scheduler->active)
		return 0;

	dev_priv->sgx_lab_global_issue_ticket =
		scheduler->next_state.issue_ticket;
	dev_priv->sgx_lab_global_running_ticket =
		scheduler->next_state.running_ticket;
	dev_priv->sgx_lab_global_complete_ticket =
		scheduler->next_state.complete_ticket;
	dev_priv->sgx_lab_global_issue_submit_id =
		scheduler->next_state.issue_submit_id;
	dev_priv->sgx_lab_global_running_submit_id =
		scheduler->next_state.running_submit_id;
	dev_priv->sgx_lab_global_complete_submit_id =
		scheduler->next_state.complete_submit_id;
	dev_priv->sgx_lab_running_active =
		scheduler->next_state.active_running_valid;
	dev_priv->sgx_lab_active_running_submit_id =
		scheduler->next_state.active_running_submit_id;

	if (scheduler->issue_advanced)
		dev_priv->sgx_lab_global_issue_jiffies = jiffies;
	if (scheduler->running_advanced)
		dev_priv->sgx_lab_global_running_jiffies = jiffies;
	if (scheduler->complete_advanced)
		dev_priv->sgx_lab_global_complete_jiffies = jiffies;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_sequence_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 op_kind,
	u32 owner_state,
	u32 packet_count,
	struct psb_sgx_lab_submit_sgx_ta_sequence *sequence)
{
	const struct psb_sgx_lab_submit_sgx_ta_event *event;
	int ret;

	if (!dev_priv || !submit || !sequence)
		return -EINVAL;

	memset(sequence, 0, sizeof(*sequence));

	ret = poulsbo_sgx_build_submit_sgx_ta_scheduler_locked(
		dev_priv, submit, op_kind, owner_state, packet_count,
		&sequence->scheduler);
	if (ret)
		return ret;
	if (op_kind == PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE)
		return 0;

	event = &sequence->scheduler.ta_op.work_item.event;
	sequence->active = true;
	sequence->refresh_views = event->refresh_views;
	sequence->increment_issue_count = event->increment_issue_count;
	sequence->increment_transition_count =
		event->increment_transition_count;
	sequence->update_last_epoch_jiffies =
		event->update_last_epoch_jiffies;
	sequence->update_last_issue_jiffies =
		event->update_last_issue_jiffies;
	sequence->update_last_transition_jiffies =
		event->update_last_transition_jiffies;
	sequence->next_issue_count =
		submit->issue_count + (event->increment_issue_count ? 1 : 0);
	sequence->next_transition_count =
		submit->transition_count +
		(event->increment_transition_count ? 1 : 0);

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_sequence_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_sequence *sequence)
{
	int ret;

	if (!dev_priv || !submit || !sequence)
		return -EINVAL;

	ret = poulsbo_sgx_apply_submit_sgx_ta_scheduler_locked(
		dev_priv, submit, &sequence->scheduler);
	if (ret)
		return ret;
	if (!sequence->active)
		return 0;

	if (sequence->increment_issue_count)
		submit->issue_count = sequence->next_issue_count;
	if (sequence->increment_transition_count)
		submit->transition_count = sequence->next_transition_count;
	if (sequence->update_last_epoch_jiffies)
		submit->last_epoch_jiffies = jiffies;
	if (sequence->update_last_issue_jiffies)
		submit->last_issue_jiffies = jiffies;
	if (sequence->update_last_transition_jiffies)
		submit->last_transition_jiffies = jiffies;
	if (!sequence->refresh_views)
		return 0;

	return poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);
}

static int poulsbo_sgx_build_submit_sgx_ta_work_queue_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 op_kind,
	u32 owner_state,
	u32 packet_count,
	struct psb_sgx_lab_submit_sgx_ta_work_queue *work_queue)
{
	const struct psb_sgx_lab_submit_sgx_ta_commit *commit;
	const struct psb_sgx_lab_submit_sgx_coordination *coordination;
	int ret;

	if (!dev_priv || !submit || !work_queue)
		return -EINVAL;

	memset(work_queue, 0, sizeof(*work_queue));

	ret = poulsbo_sgx_build_submit_sgx_ta_sequence_locked(
		dev_priv, submit, op_kind, owner_state, packet_count,
		&work_queue->sequence);
	if (ret)
		return ret;
	if (op_kind == PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE)
		return 0;

	commit = &work_queue->sequence.scheduler.ta_op.work_item.event.commit;
	coordination =
		&commit->path.dispatch.schedule.control.runtime.coordination;
	work_queue->active = true;
	work_queue->submit_id = submit->submit_id;
	work_queue->op_kind = work_queue->sequence.scheduler.ta_op.op_kind;
	work_queue->owner_state = work_queue->sequence.scheduler.ta_op.owner_state;
	work_queue->packet_count = work_queue->sequence.scheduler.ta_op.packet_count;
	poulsbo_sgx_fill_submit_sgx_scheduler_state_from_coordination(
		coordination, &work_queue->base_state);
	work_queue->base_issue_count = submit->issue_count;
	work_queue->base_transition_count = submit->transition_count;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_work_queue_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_work_queue *work_queue)
{
	if (!dev_priv || !submit || !work_queue)
		return -EINVAL;
	if (work_queue->active && work_queue->submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_sequence_locked(
		dev_priv, submit, &work_queue->sequence);
}

static int poulsbo_sgx_build_submit_sgx_ta_job_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 op_kind,
	u32 owner_state,
	u32 packet_count,
	struct psb_sgx_lab_submit_sgx_ta_job *job)
{
	const struct psb_sgx_lab_submit_sgx_ta_control *control;
	const struct psb_sgx_lab_submit_sgx_ta_runtime *runtime;
	const struct psb_sgx_lab_submit_sgx_ta_gate *gate;
	unsigned int slot;
	int ret;

	if (!dev_priv || !submit || !job)
		return -EINVAL;

	memset(job, 0, sizeof(*job));

	ret = poulsbo_sgx_build_submit_sgx_ta_work_queue_locked(
		dev_priv, submit, op_kind, owner_state, packet_count,
		&job->work_queue);
	if (ret)
		return ret;
	if (op_kind == PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE)
		return 0;

	control =
		&job->work_queue.sequence.scheduler.ta_op.work_item.event.commit.path.dispatch.schedule.control;
	runtime = &control->runtime;
	gate = &control->gate;

	job->active = true;
	job->submit_id = submit->submit_id;
	job->context = submit->context;
	job->cmd_type = submit->cmd_type;
	job->ctrl_flags = submit->ctrl_flags;
	job->control_object_id = submit->control_object_id;
	job->pds_object_id = submit->pds_object_id;
	job->use_object_id = submit->use_object_id;
	job->target_object_id = submit->target_object_id;
	job->status_object_id = submit->status_object_id;
	job->num_ta_status_vals = submit->num_ta_status_vals;
	job->num_3d_status_vals = submit->num_3d_status_vals;
	job->ta_dependency = submit->ta_dependency;
	job->active_dependency_count = gate->active_dependency_count;
	job->running_slot_busy = gate->running_slot_busy;
	job->can_enter_running = gate->can_enter_running;
	for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++)
		job->dependencies[slot] = runtime->dependencies[slot];

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_job_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_job *job)
{
	if (!dev_priv || !submit || !job)
		return -EINVAL;
	if (job->active && job->submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_work_queue_locked(
		dev_priv, submit, &job->work_queue);
}

static int poulsbo_sgx_build_submit_sgx_ta_request_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 op_kind,
	u32 owner_state,
	u32 packet_count,
	struct psb_sgx_lab_submit_sgx_ta_request *request)
{
	const struct psb_sgx_lab_submit_sgx_ta_sequence *sequence;
	const struct psb_sgx_lab_submit_sgx_ta_scheduler *scheduler;
	const struct psb_sgx_lab_submit_sgx_ta_commit *commit;
	int ret;

	if (!dev_priv || !submit || !request)
		return -EINVAL;

	memset(request, 0, sizeof(*request));

	ret = poulsbo_sgx_build_submit_sgx_ta_job_locked(
		dev_priv, submit, op_kind, owner_state, packet_count,
		&request->job);
	if (ret)
		return ret;
	if (op_kind == PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE)
		return 0;

	sequence = &request->job.work_queue.sequence;
	scheduler = &sequence->scheduler;
	commit = &scheduler->ta_op.work_item.event.commit;

	request->active = true;
	request->submit_id = submit->submit_id;
	request->op_kind = request->job.work_queue.op_kind;
	request->owner_state = request->job.work_queue.owner_state;
	request->packet_count = request->job.work_queue.packet_count;
	request->issue_active = commit->issue_active;
	request->transition_active = commit->transition_active;
	request->refresh_views = sequence->refresh_views;
	request->increment_issue_count = sequence->increment_issue_count;
	request->increment_transition_count =
		sequence->increment_transition_count;
	request->issue_advanced = scheduler->issue_advanced;
	request->running_advanced = scheduler->running_advanced;
	request->complete_advanced = scheduler->complete_advanced;
	request->active_running_changed =
		scheduler->active_running_changed;
	request->next_state = scheduler->next_state;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request *request)
{
	if (!dev_priv || !submit || !request)
		return -EINVAL;
	if (request->active && request->submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_job_locked(dev_priv, submit,
							  &request->job);
}

static int poulsbo_sgx_map_submit_sgx_ta_ioctl_op_kind(u32 ioctl_op,
							u32 *op_kind_out)
{
	if (!op_kind_out)
		return -EINVAL;

	switch (ioctl_op) {
	case DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE:
		*op_kind_out = PSB_SGX_LAB_SUBMIT_SGX_TA_OP_ISSUE;
		return 0;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION:
		*op_kind_out = PSB_SGX_LAB_SUBMIT_SGX_TA_OP_TRANSITION;
		return 0;
	default:
		return -EINVAL;
	}
}

static int poulsbo_sgx_build_submit_sgx_ta_ioctl_request_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_ioctl_request *ioctl_request)
{
	u32 op_kind;
	int ret;

	if (!dev_priv || !submit || !op || !ioctl_request)
		return -EINVAL;

	memset(ioctl_request, 0, sizeof(*ioctl_request));

	ret = poulsbo_sgx_map_submit_sgx_ta_ioctl_op_kind(op->op, &op_kind);
	if (ret)
		return ret;

	if (resolved_submit_id != submit->submit_id)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_sgx_ta_request_locked(
		dev_priv, submit, op_kind, op->owner_state, op->packet_count,
		&ioctl_request->request);
	if (ret)
		return ret;

	ioctl_request->active = ioctl_request->request.active;
	ioctl_request->dependency_fields_present = dependency_fields_present;
	ioctl_request->focus_resolved = op->submit_id != resolved_submit_id;
	ioctl_request->requested_submit_id = op->submit_id;
	ioctl_request->resolved_submit_id = resolved_submit_id;
	ioctl_request->ioctl_op = op->op;
	ioctl_request->op_kind = op_kind;
	ioctl_request->owner_state = op->owner_state;
	ioctl_request->packet_count = op->packet_count;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_request_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request *request)
{
	u32 op_kind;
	int ret;

	if (!dev_priv || !submit || !op || !request)
		return -EINVAL;

	memset(request, 0, sizeof(*request));

	ret = poulsbo_sgx_map_submit_sgx_ta_ioctl_op_kind(op->op, &op_kind);
	if (ret)
		return ret;

	ret = poulsbo_sgx_validate_submit_sgx_ta_ioctl_op_request(
		op, dependency_fields_present, op_kind);
	if (ret)
		return ret;

	ret = poulsbo_sgx_build_submit_sgx_ta_ioctl_request_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &request->ioctl_request);
	if (ret)
		return ret;

	request->op = *op;
	request->active = request->ioctl_request.active;
	request->dependency_fields_present = dependency_fields_present;
	request->use_focus_request =
		!!(op->flags & DRM_PSB_SGX_LAB_SUBMIT_OP_USE_FOCUS);
	request->focus_resolved = request->ioctl_request.focus_resolved;
	request->issue_request =
		op_kind == PSB_SGX_LAB_SUBMIT_SGX_TA_OP_ISSUE;
	request->transition_request =
		op_kind == PSB_SGX_LAB_SUBMIT_SGX_TA_OP_TRANSITION;
	request->resolved_submit_id = resolved_submit_id;
	request->op_kind = op_kind;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_ioctl_request_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_ioctl_request *ioctl_request)
{
	if (!dev_priv || !submit || !ioctl_request)
		return -EINVAL;
	if (ioctl_request->active &&
	    ioctl_request->resolved_submit_id != submit->submit_id) {
		return -EINVAL;
	}

	return poulsbo_sgx_apply_submit_sgx_ta_request_locked(
		dev_priv, submit, &ioctl_request->request);
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request *request)
{
	if (!dev_priv || !submit || !request)
		return -EINVAL;
	if (request->active && request->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_ioctl_request_locked(
		dev_priv, submit, &request->ioctl_request);
}

static int poulsbo_sgx_build_submit_sgx_ta_queue_item_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_queue_item *queue_item)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request *request;
	const struct psb_sgx_lab_submit_sgx_ta_request *ta_request;
	const struct psb_sgx_lab_submit_sgx_issue_image *issue_image;
	const struct psb_sgx_lab_submit_sgx_ta_commit_state *next_state;
	int ret;

	if (!dev_priv || !submit || !op || !queue_item)
		return -EINVAL;

	memset(queue_item, 0, sizeof(*queue_item));

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_request_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &queue_item->submit_request);
	if (ret)
		return ret;

	request = &queue_item->submit_request;
	ta_request = &request->ioctl_request.request;
	issue_image =
		&ta_request->job.work_queue.sequence.scheduler.ta_op.work_item.event.commit.path.dispatch.issue_image;
	next_state =
		&ta_request->job.work_queue.sequence.scheduler.ta_op.work_item.event.commit.next_state;

	queue_item->active = request->active;
	queue_item->issue_request = request->issue_request;
	queue_item->transition_request = request->transition_request;
	queue_item->submit_id = request->resolved_submit_id;
	queue_item->owner_state = request->op.owner_state;
	queue_item->packet_count = request->op.packet_count;
	queue_item->issue_ticket = ta_request->next_state.issue_ticket;
	queue_item->running_ticket = ta_request->next_state.running_ticket;
	queue_item->complete_ticket = ta_request->next_state.complete_ticket;

	if (request->issue_request) {
		queue_item->refilled_from_complete =
			ta_request->job.work_queue.sequence.scheduler.ta_op.work_item.event.commit.path.dispatch.schedule.control.issue_request.plan.refilled_from_complete;
		queue_item->new_read_offset = issue_image->sync.read_offset;
		queue_item->new_write_offset = issue_image->queue_header.write_offset;
		queue_item->new_ccb_offset = issue_image->issue_write_offset;
		queue_item->outstanding_packets =
			issue_image->queue_header.queue_packet_count;
		queue_item->wrapped =
			!!issue_image->queue_header.queue_head_packet_count;
		queue_item->pending_seq = issue_image->sync.pending_seq;
		queue_item->issue_cookie = issue_image->owner.owner_issue_cookie;
		queue_item->epoch = issue_image->sync.epoch;
		return 0;
	}

	if (request->transition_request) {
		queue_item->new_read_offset = next_state->ccb_read_offset;
		queue_item->new_write_offset = next_state->ccb_write_offset;
		queue_item->new_ccb_offset = next_state->ccb_offset;
		queue_item->outstanding_packets = next_state->queue_packet_count;
		queue_item->wrapped = !!next_state->queue_head_packet_count;
		queue_item->pending_seq = next_state->sync_pending_seq;
		queue_item->complete_seq = next_state->sync_complete_seq;
		queue_item->issue_cookie = next_state->owner_issue_cookie;
		queue_item->complete_cookie = next_state->owner_complete_cookie;
		queue_item->epoch = next_state->queue_epoch;
		return 0;
	}

	return -EINVAL;
}

static int poulsbo_sgx_apply_submit_sgx_ta_queue_item_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_queue_item *queue_item)
{
	if (!dev_priv || !submit || !queue_item)
		return -EINVAL;
	if (queue_item->active && queue_item->submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_locked(
		dev_priv, submit, &queue_item->submit_request);
}

static int poulsbo_sgx_build_submit_sgx_ta_queue_command_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_queue_command *queue_command)
{
	const struct psb_sgx_lab_submit_sgx_ta_queue_item *queue_item;

	if (!dev_priv || !submit || !op || !queue_command)
		return -EINVAL;

	memset(queue_command, 0, sizeof(*queue_command));

	{
		int ret;

		ret = poulsbo_sgx_build_submit_sgx_ta_queue_item_locked(
			dev_priv, submit, resolved_submit_id, op,
			dependency_fields_present, &queue_command->queue_item);
		if (ret)
			return ret;
	}

	queue_item = &queue_command->queue_item;
	queue_command->active = queue_item->active;
	queue_command->issue_request = queue_item->issue_request;
	queue_command->transition_request = queue_item->transition_request;
	queue_command->refilled_from_complete =
		queue_item->refilled_from_complete;
	queue_command->submit_id = queue_item->submit_id;
	queue_command->owner_state = queue_item->owner_state;
	queue_command->packet_count = queue_item->packet_count;

	queue_command->prev_read_offset = submit->ccb_read_offset;
	queue_command->prev_write_offset = submit->ccb_write_offset;
	queue_command->prev_ccb_offset = submit->ccb_offset;
	queue_command->prev_outstanding_packets = submit->queue_packet_count;
	queue_command->prev_wrapped = !!submit->queue_head_packet_count;
	queue_command->prev_pending_seq = submit->sync_pending_seq;
	queue_command->prev_complete_seq = submit->sync_complete_seq;
	queue_command->prev_issue_cookie = submit->owner_issue_cookie;
	queue_command->prev_complete_cookie = submit->owner_complete_cookie;
	queue_command->prev_epoch = submit->queue_epoch;
	queue_command->prev_issue_ticket =
		dev_priv->sgx_lab_global_issue_ticket;
	queue_command->prev_running_ticket =
		dev_priv->sgx_lab_global_running_ticket;
	queue_command->prev_complete_ticket =
		dev_priv->sgx_lab_global_complete_ticket;

	queue_command->next_read_offset = queue_item->new_read_offset;
	queue_command->next_write_offset = queue_item->new_write_offset;
	queue_command->next_ccb_offset = queue_item->new_ccb_offset;
	queue_command->next_outstanding_packets =
		queue_item->outstanding_packets;
	queue_command->next_wrapped = queue_item->wrapped;
	queue_command->next_pending_seq = queue_item->pending_seq;
	queue_command->next_complete_seq = queue_item->complete_seq;
	queue_command->next_issue_cookie = queue_item->issue_cookie;
	queue_command->next_complete_cookie = queue_item->complete_cookie;
	queue_command->next_epoch = queue_item->epoch;
	queue_command->next_issue_ticket = queue_item->issue_ticket;
	queue_command->next_running_ticket = queue_item->running_ticket;
	queue_command->next_complete_ticket = queue_item->complete_ticket;

	if (queue_command->issue_request) {
		queue_command->next_complete_seq =
			queue_command->prev_complete_seq;
		queue_command->next_complete_cookie =
			queue_command->prev_complete_cookie;
	}

	queue_command->read_advanced =
		queue_command->next_read_offset !=
		queue_command->prev_read_offset;
	queue_command->write_advanced =
		queue_command->next_write_offset !=
		queue_command->prev_write_offset;
	queue_command->ccb_advanced =
		queue_command->next_ccb_offset != queue_command->prev_ccb_offset;
	queue_command->outstanding_changed =
		queue_command->next_outstanding_packets !=
		queue_command->prev_outstanding_packets ||
		queue_command->next_wrapped != queue_command->prev_wrapped;
	queue_command->pending_advanced =
		queue_command->next_pending_seq != queue_command->prev_pending_seq;
	queue_command->complete_advanced =
		queue_command->next_complete_seq !=
		queue_command->prev_complete_seq;
	queue_command->issue_cookie_advanced =
		queue_command->next_issue_cookie !=
		queue_command->prev_issue_cookie;
	queue_command->complete_cookie_advanced =
		queue_command->next_complete_cookie !=
		queue_command->prev_complete_cookie;
	queue_command->epoch_advanced =
		queue_command->next_epoch != queue_command->prev_epoch;
	queue_command->issue_ticket_advanced =
		queue_command->next_issue_ticket !=
		queue_command->prev_issue_ticket;
	queue_command->running_ticket_advanced =
		queue_command->next_running_ticket !=
		queue_command->prev_running_ticket;
	queue_command->complete_ticket_advanced =
		queue_command->next_complete_ticket !=
		queue_command->prev_complete_ticket;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_queue_command_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_queue_command *queue_command)
{
	if (!dev_priv || !submit || !queue_command)
		return -EINVAL;
	if (queue_command->active &&
	    queue_command->submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_queue_item_locked(
		dev_priv, submit, &queue_command->queue_item);
}

static int poulsbo_sgx_build_submit_sgx_ta_scheduler_entry_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_scheduler_entry *scheduler_entry)
{
	const struct psb_sgx_lab_submit_sgx_ta_queue_command *queue_command;
	const struct psb_sgx_lab_submit_sgx_ta_request *ta_request;
	int ret;

	if (!dev_priv || !submit || !op || !scheduler_entry)
		return -EINVAL;

	memset(scheduler_entry, 0, sizeof(*scheduler_entry));

	ret = poulsbo_sgx_build_submit_sgx_ta_queue_command_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &scheduler_entry->queue_command);
	if (ret)
		return ret;

	queue_command = &scheduler_entry->queue_command;
	ta_request =
		&queue_command->queue_item.submit_request.ioctl_request.request;

	scheduler_entry->active = queue_command->active;
	scheduler_entry->issue_request = queue_command->issue_request;
	scheduler_entry->transition_request =
		queue_command->transition_request;
	scheduler_entry->submit_id = queue_command->submit_id;
	scheduler_entry->owner_state = queue_command->owner_state;
	scheduler_entry->packet_count = queue_command->packet_count;

	scheduler_entry->prev_state.issue_ticket =
		dev_priv->sgx_lab_global_issue_ticket;
	scheduler_entry->prev_state.running_ticket =
		dev_priv->sgx_lab_global_running_ticket;
	scheduler_entry->prev_state.complete_ticket =
		dev_priv->sgx_lab_global_complete_ticket;
	scheduler_entry->prev_state.issue_submit_id =
		dev_priv->sgx_lab_global_issue_submit_id;
	scheduler_entry->prev_state.running_submit_id =
		dev_priv->sgx_lab_global_running_submit_id;
	scheduler_entry->prev_state.complete_submit_id =
		dev_priv->sgx_lab_global_complete_submit_id;
	scheduler_entry->prev_state.active_running_valid =
		dev_priv->sgx_lab_running_active;
	scheduler_entry->prev_state.active_running_submit_id =
		dev_priv->sgx_lab_active_running_submit_id;

	scheduler_entry->next_state = ta_request->next_state;
	scheduler_entry->issue_advanced =
		scheduler_entry->next_state.issue_ticket !=
		scheduler_entry->prev_state.issue_ticket;
	scheduler_entry->running_advanced =
		scheduler_entry->next_state.running_ticket !=
		scheduler_entry->prev_state.running_ticket;
	scheduler_entry->complete_advanced =
		scheduler_entry->next_state.complete_ticket !=
		scheduler_entry->prev_state.complete_ticket;
	scheduler_entry->active_running_changed =
		scheduler_entry->next_state.active_running_valid !=
			scheduler_entry->prev_state.active_running_valid ||
		scheduler_entry->next_state.active_running_submit_id !=
			scheduler_entry->prev_state.active_running_submit_id;
	scheduler_entry->issue_submit_changed =
		scheduler_entry->next_state.issue_submit_id !=
		scheduler_entry->prev_state.issue_submit_id;
	scheduler_entry->running_submit_changed =
		scheduler_entry->next_state.running_submit_id !=
		scheduler_entry->prev_state.running_submit_id;
	scheduler_entry->complete_submit_changed =
		scheduler_entry->next_state.complete_submit_id !=
		scheduler_entry->prev_state.complete_submit_id;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_scheduler_entry_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_scheduler_entry *scheduler_entry)
{
	if (!dev_priv || !submit || !scheduler_entry)
		return -EINVAL;
	if (scheduler_entry->active &&
	    scheduler_entry->submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_queue_command_locked(
		dev_priv, submit, &scheduler_entry->queue_command);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request *request)
{
	const struct psb_sgx_lab_submit_sgx_ta_ioctl_request *ioctl_request;
	u32 op_kind;

	if (!dev_priv || !submit || !request)
		return -EINVAL;

	if (poulsbo_sgx_map_submit_sgx_ta_ioctl_op_kind(
		    request->op.op, &op_kind))
		return -EINVAL;

	if (poulsbo_sgx_validate_submit_sgx_ta_ioctl_op_request(
		    &request->op, request->dependency_fields_present, op_kind))
		return -EINVAL;

	ioctl_request = &request->ioctl_request;
	if (request->active != ioctl_request->active ||
	    request->dependency_fields_present !=
		    ioctl_request->dependency_fields_present ||
	    request->use_focus_request !=
		    !!(request->op.flags &
		       DRM_PSB_SGX_LAB_SUBMIT_OP_USE_FOCUS) ||
	    request->focus_resolved != ioctl_request->focus_resolved ||
	    request->resolved_submit_id != submit->submit_id ||
	    request->resolved_submit_id != ioctl_request->resolved_submit_id ||
	    request->op_kind != op_kind ||
	    request->op_kind != ioctl_request->op_kind ||
	    request->op.submit_id != ioctl_request->requested_submit_id ||
	    request->op.owner_state != ioctl_request->owner_state ||
	    request->op.packet_count != ioctl_request->packet_count ||
	    !request->active) {
		return -EINVAL;
	}

	switch (request->op_kind) {
	case PSB_SGX_LAB_SUBMIT_SGX_TA_OP_ISSUE:
		if (!request->issue_request || request->transition_request)
			return -EINVAL;
		break;
	case PSB_SGX_LAB_SUBMIT_SGX_TA_OP_TRANSITION:
		if (!request->transition_request || request->issue_request)
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_queue_item_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_queue_item *queue_item)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request *request;
	const struct psb_sgx_lab_submit_sgx_ta_request *ta_request;
	const struct psb_sgx_lab_submit_sgx_issue_image *issue_image;
	const struct psb_sgx_lab_submit_sgx_ta_commit_state *next_state;
	bool issue_request;
	bool transition_request;
	int ret;

	if (!dev_priv || !submit || !queue_item)
		return -EINVAL;

	request = &queue_item->submit_request;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_locked(
		dev_priv, submit, request);
	if (ret)
		return ret;

	ta_request = &request->ioctl_request.request;
	issue_image =
		&ta_request->job.work_queue.sequence.scheduler.ta_op.work_item.event.commit.path.dispatch.issue_image;
	next_state =
		&ta_request->job.work_queue.sequence.scheduler.ta_op.work_item.event.commit.next_state;
	issue_request = request->issue_request;
	transition_request = request->transition_request;

	if (!queue_item->active || queue_item->active != request->active ||
	    queue_item->issue_request != issue_request ||
	    queue_item->transition_request != transition_request ||
	    issue_request == transition_request ||
	    queue_item->submit_id != request->resolved_submit_id ||
	    queue_item->owner_state != request->op.owner_state ||
	    queue_item->packet_count != request->op.packet_count ||
	    queue_item->issue_ticket != ta_request->next_state.issue_ticket ||
	    queue_item->running_ticket != ta_request->next_state.running_ticket ||
	    queue_item->complete_ticket != ta_request->next_state.complete_ticket) {
		return -EINVAL;
	}

	if (issue_request) {
		if (!issue_image->active ||
		    queue_item->refilled_from_complete !=
			    ta_request->job.work_queue.sequence.scheduler.ta_op.work_item.event.commit.path.dispatch.schedule.control.issue_request.plan.refilled_from_complete ||
		    queue_item->new_read_offset != issue_image->sync.read_offset ||
		    queue_item->new_write_offset !=
			    issue_image->queue_header.write_offset ||
		    queue_item->new_ccb_offset != issue_image->issue_write_offset ||
		    queue_item->outstanding_packets !=
			    issue_image->queue_header.queue_packet_count ||
		    queue_item->wrapped !=
			    !!issue_image->queue_header.queue_head_packet_count ||
		    queue_item->pending_seq != issue_image->sync.pending_seq ||
		    queue_item->complete_seq ||
		    queue_item->issue_cookie !=
			    issue_image->owner.owner_issue_cookie ||
		    queue_item->complete_cookie ||
		    queue_item->epoch != issue_image->sync.epoch) {
			return -EINVAL;
		}
		return 0;
	}

	if (!transition_request)
		return -EINVAL;
	if (queue_item->refilled_from_complete ||
	    queue_item->new_read_offset != next_state->ccb_read_offset ||
	    queue_item->new_write_offset != next_state->ccb_write_offset ||
	    queue_item->new_ccb_offset != next_state->ccb_offset ||
	    queue_item->outstanding_packets != next_state->queue_packet_count ||
	    queue_item->wrapped != !!next_state->queue_head_packet_count ||
	    queue_item->pending_seq != next_state->sync_pending_seq ||
	    queue_item->complete_seq != next_state->sync_complete_seq ||
	    queue_item->issue_cookie != next_state->owner_issue_cookie ||
	    queue_item->complete_cookie != next_state->owner_complete_cookie ||
	    queue_item->epoch != next_state->queue_epoch) {
		return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_queue_command_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_queue_command *queue_command)
{
	const struct psb_sgx_lab_submit_sgx_ta_queue_item *queue_item;
	u32 next_complete_seq;
	u32 next_complete_cookie;
	bool read_advanced;
	bool write_advanced;
	bool ccb_advanced;
	bool outstanding_changed;
	bool pending_advanced;
	bool complete_advanced;
	bool issue_cookie_advanced;
	bool complete_cookie_advanced;
	bool epoch_advanced;
	bool issue_ticket_advanced;
	bool running_ticket_advanced;
	bool complete_ticket_advanced;
	int ret;

	if (!dev_priv || !submit || !queue_command)
		return -EINVAL;

	queue_item = &queue_command->queue_item;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_queue_item_locked(
		dev_priv, submit, queue_item);
	if (ret)
		return ret;

	if (!queue_command->active ||
	    queue_command->active != queue_item->active ||
	    queue_command->issue_request != queue_item->issue_request ||
	    queue_command->transition_request !=
		    queue_item->transition_request ||
	    queue_command->issue_request ==
		    queue_command->transition_request ||
	    queue_command->refilled_from_complete !=
		    queue_item->refilled_from_complete ||
	    queue_command->submit_id != queue_item->submit_id ||
	    queue_command->owner_state != queue_item->owner_state ||
	    queue_command->packet_count != queue_item->packet_count ||
	    queue_command->prev_read_offset != submit->ccb_read_offset ||
	    queue_command->prev_write_offset != submit->ccb_write_offset ||
	    queue_command->prev_ccb_offset != submit->ccb_offset ||
	    queue_command->prev_outstanding_packets !=
		    submit->queue_packet_count ||
	    queue_command->prev_wrapped !=
		    !!submit->queue_head_packet_count ||
	    queue_command->prev_pending_seq != submit->sync_pending_seq ||
	    queue_command->prev_complete_seq != submit->sync_complete_seq ||
	    queue_command->prev_issue_cookie != submit->owner_issue_cookie ||
	    queue_command->prev_complete_cookie !=
		    submit->owner_complete_cookie ||
	    queue_command->prev_epoch != submit->queue_epoch ||
	    queue_command->prev_issue_ticket !=
		    dev_priv->sgx_lab_global_issue_ticket ||
	    queue_command->prev_running_ticket !=
		    dev_priv->sgx_lab_global_running_ticket ||
	    queue_command->prev_complete_ticket !=
		    dev_priv->sgx_lab_global_complete_ticket ||
	    queue_command->next_read_offset != queue_item->new_read_offset ||
	    queue_command->next_write_offset != queue_item->new_write_offset ||
	    queue_command->next_ccb_offset != queue_item->new_ccb_offset ||
	    queue_command->next_outstanding_packets !=
		    queue_item->outstanding_packets ||
	    queue_command->next_wrapped != queue_item->wrapped ||
	    queue_command->next_pending_seq != queue_item->pending_seq ||
	    queue_command->next_issue_cookie != queue_item->issue_cookie ||
	    queue_command->next_epoch != queue_item->epoch ||
	    queue_command->next_issue_ticket != queue_item->issue_ticket ||
	    queue_command->next_running_ticket !=
		    queue_item->running_ticket ||
	    queue_command->next_complete_ticket !=
		    queue_item->complete_ticket) {
		return -EINVAL;
	}

	if (queue_command->issue_request) {
		next_complete_seq = submit->sync_complete_seq;
		next_complete_cookie = submit->owner_complete_cookie;
	} else {
		next_complete_seq = queue_item->complete_seq;
		next_complete_cookie = queue_item->complete_cookie;
	}

	if (queue_command->next_complete_seq != next_complete_seq ||
	    queue_command->next_complete_cookie != next_complete_cookie)
		return -EINVAL;

	read_advanced = queue_command->next_read_offset !=
			queue_command->prev_read_offset;
	write_advanced = queue_command->next_write_offset !=
			 queue_command->prev_write_offset;
	ccb_advanced = queue_command->next_ccb_offset !=
		       queue_command->prev_ccb_offset;
	outstanding_changed =
		queue_command->next_outstanding_packets !=
			queue_command->prev_outstanding_packets ||
		queue_command->next_wrapped != queue_command->prev_wrapped;
	pending_advanced = queue_command->next_pending_seq !=
			   queue_command->prev_pending_seq;
	complete_advanced = queue_command->next_complete_seq !=
			    queue_command->prev_complete_seq;
	issue_cookie_advanced = queue_command->next_issue_cookie !=
				queue_command->prev_issue_cookie;
	complete_cookie_advanced = queue_command->next_complete_cookie !=
				   queue_command->prev_complete_cookie;
	epoch_advanced = queue_command->next_epoch != queue_command->prev_epoch;
	issue_ticket_advanced = queue_command->next_issue_ticket !=
				queue_command->prev_issue_ticket;
	running_ticket_advanced = queue_command->next_running_ticket !=
				  queue_command->prev_running_ticket;
	complete_ticket_advanced = queue_command->next_complete_ticket !=
				   queue_command->prev_complete_ticket;

	if (queue_command->read_advanced != read_advanced ||
	    queue_command->write_advanced != write_advanced ||
	    queue_command->ccb_advanced != ccb_advanced ||
	    queue_command->outstanding_changed != outstanding_changed ||
	    queue_command->pending_advanced != pending_advanced ||
	    queue_command->complete_advanced != complete_advanced ||
	    queue_command->issue_cookie_advanced != issue_cookie_advanced ||
	    queue_command->complete_cookie_advanced !=
		    complete_cookie_advanced ||
	    queue_command->epoch_advanced != epoch_advanced ||
	    queue_command->issue_ticket_advanced != issue_ticket_advanced ||
	    queue_command->running_ticket_advanced !=
		    running_ticket_advanced ||
	    queue_command->complete_ticket_advanced !=
		    complete_ticket_advanced) {
		return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_scheduler_entry_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_scheduler_entry *scheduler_entry)
{
	const struct psb_sgx_lab_submit_sgx_ta_queue_command *queue_command;
	const struct psb_sgx_lab_submit_sgx_ta_request *ta_request;
	bool issue_advanced;
	bool running_advanced;
	bool complete_advanced;
	bool active_running_changed;
	bool issue_submit_changed;
	bool running_submit_changed;
	bool complete_submit_changed;
	int ret;

	if (!dev_priv || !submit || !scheduler_entry)
		return -EINVAL;

	queue_command = &scheduler_entry->queue_command;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_queue_command_locked(
		dev_priv, submit, queue_command);
	if (ret)
		return ret;

	ta_request =
		&queue_command->queue_item.submit_request.ioctl_request.request;

	if (!scheduler_entry->active ||
	    scheduler_entry->active != queue_command->active ||
	    scheduler_entry->issue_request != queue_command->issue_request ||
	    scheduler_entry->transition_request !=
		    queue_command->transition_request ||
	    scheduler_entry->issue_request ==
		    scheduler_entry->transition_request ||
	    scheduler_entry->submit_id != queue_command->submit_id ||
	    scheduler_entry->owner_state != queue_command->owner_state ||
	    scheduler_entry->packet_count != queue_command->packet_count ||
	    scheduler_entry->prev_state.issue_ticket !=
		    dev_priv->sgx_lab_global_issue_ticket ||
	    scheduler_entry->prev_state.running_ticket !=
		    dev_priv->sgx_lab_global_running_ticket ||
	    scheduler_entry->prev_state.complete_ticket !=
		    dev_priv->sgx_lab_global_complete_ticket ||
	    scheduler_entry->prev_state.issue_submit_id !=
		    dev_priv->sgx_lab_global_issue_submit_id ||
	    scheduler_entry->prev_state.running_submit_id !=
		    dev_priv->sgx_lab_global_running_submit_id ||
	    scheduler_entry->prev_state.complete_submit_id !=
		    dev_priv->sgx_lab_global_complete_submit_id ||
	    scheduler_entry->prev_state.active_running_valid !=
		    dev_priv->sgx_lab_running_active ||
	    scheduler_entry->prev_state.active_running_submit_id !=
		    dev_priv->sgx_lab_active_running_submit_id ||
	    scheduler_entry->next_state.issue_ticket !=
		    ta_request->next_state.issue_ticket ||
	    scheduler_entry->next_state.running_ticket !=
		    ta_request->next_state.running_ticket ||
	    scheduler_entry->next_state.complete_ticket !=
		    ta_request->next_state.complete_ticket ||
	    scheduler_entry->next_state.issue_submit_id !=
		    ta_request->next_state.issue_submit_id ||
	    scheduler_entry->next_state.running_submit_id !=
		    ta_request->next_state.running_submit_id ||
	    scheduler_entry->next_state.complete_submit_id !=
		    ta_request->next_state.complete_submit_id ||
	    scheduler_entry->next_state.active_running_valid !=
		    ta_request->next_state.active_running_valid ||
	    scheduler_entry->next_state.active_running_submit_id !=
		    ta_request->next_state.active_running_submit_id ||
	    scheduler_entry->prev_state.issue_ticket !=
		    queue_command->prev_issue_ticket ||
	    scheduler_entry->prev_state.running_ticket !=
		    queue_command->prev_running_ticket ||
	    scheduler_entry->prev_state.complete_ticket !=
		    queue_command->prev_complete_ticket ||
	    scheduler_entry->next_state.issue_ticket !=
		    queue_command->next_issue_ticket ||
	    scheduler_entry->next_state.running_ticket !=
		    queue_command->next_running_ticket ||
	    scheduler_entry->next_state.complete_ticket !=
		    queue_command->next_complete_ticket) {
		return -EINVAL;
	}

	issue_advanced = scheduler_entry->next_state.issue_ticket !=
			 scheduler_entry->prev_state.issue_ticket;
	running_advanced = scheduler_entry->next_state.running_ticket !=
			   scheduler_entry->prev_state.running_ticket;
	complete_advanced = scheduler_entry->next_state.complete_ticket !=
			    scheduler_entry->prev_state.complete_ticket;
	active_running_changed =
		scheduler_entry->next_state.active_running_valid !=
			scheduler_entry->prev_state.active_running_valid ||
		scheduler_entry->next_state.active_running_submit_id !=
			scheduler_entry->prev_state.active_running_submit_id;
	issue_submit_changed =
		scheduler_entry->next_state.issue_submit_id !=
		scheduler_entry->prev_state.issue_submit_id;
	running_submit_changed =
		scheduler_entry->next_state.running_submit_id !=
		scheduler_entry->prev_state.running_submit_id;
	complete_submit_changed =
		scheduler_entry->next_state.complete_submit_id !=
		scheduler_entry->prev_state.complete_submit_id;

	if (scheduler_entry->issue_advanced != issue_advanced ||
	    scheduler_entry->running_advanced != running_advanced ||
	    scheduler_entry->complete_advanced != complete_advanced ||
	    scheduler_entry->active_running_changed !=
		    active_running_changed ||
	    scheduler_entry->issue_submit_changed != issue_submit_changed ||
	    scheduler_entry->running_submit_changed !=
		    running_submit_changed ||
	    scheduler_entry->complete_submit_changed !=
		    complete_submit_changed) {
		return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_queue_stream_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_queue_stream *queue_stream)
{
	const struct psb_sgx_lab_submit_sgx_ta_scheduler_entry *scheduler_entry;
	const struct psb_sgx_lab_submit_sgx_ta_request *ta_request;
	const struct psb_sgx_lab_submit_sgx_issue_image *issue_image;
	const struct psb_sgx_lab_submit_sgx_transition_image *transition_image;
	int ret;

	if (!dev_priv || !submit || !op || !queue_stream)
		return -EINVAL;

	memset(queue_stream, 0, sizeof(*queue_stream));

	ret = poulsbo_sgx_build_submit_sgx_ta_scheduler_entry_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &queue_stream->scheduler_entry);
	if (ret)
		return ret;

	scheduler_entry = &queue_stream->scheduler_entry;
	ta_request =
		&scheduler_entry->queue_command.queue_item.submit_request
			 .ioctl_request.request;
	issue_image =
		&ta_request->job.work_queue.sequence.scheduler.ta_op.work_item
			 .event.commit.path.dispatch.issue_image;
	transition_image =
		&ta_request->job.work_queue.sequence.scheduler.ta_op.work_item
			 .event.commit.path.transition_image;

	queue_stream->active = scheduler_entry->active;
	queue_stream->issue_request = scheduler_entry->issue_request;
	queue_stream->transition_request =
		scheduler_entry->transition_request;
	queue_stream->submit_id = scheduler_entry->submit_id;
	queue_stream->owner_state = scheduler_entry->owner_state;
	queue_stream->packet_count = scheduler_entry->packet_count;

	if (queue_stream->issue_request) {
		if (!issue_image->active)
			return -EINVAL;

		queue_stream->stream_active = true;
		queue_stream->use_issue_stream = true;
		queue_stream->stream_ccb_offset = issue_image->issue_write_offset;
		queue_stream->queue_region_end = issue_image->queue_region_end;
		queue_stream->sync_region_end = issue_image->sync_region_end;
		queue_stream->owner_region_end = issue_image->owner_region_end;
		queue_stream->queue_header = issue_image->queue_header;
		queue_stream->ccb_record = issue_image->ccb_record;
		queue_stream->sync = issue_image->sync;
		queue_stream->owner = issue_image->owner;
		return 0;
	}

	if (!queue_stream->transition_request || !transition_image->active)
		return -EINVAL;

	queue_stream->stream_active = true;
	queue_stream->use_transition_stream = true;
	queue_stream->stream_ccb_offset =
		scheduler_entry->queue_command.next_ccb_offset;
	queue_stream->queue_region_end = transition_image->queue_region_end;
	queue_stream->sync_region_end = transition_image->sync_region_end;
	queue_stream->owner_region_end = transition_image->owner_region_end;
	queue_stream->queue_header = transition_image->queue_header;
	queue_stream->ccb_record = transition_image->ccb_record;
	queue_stream->sync = transition_image->sync;
	queue_stream->owner = transition_image->owner;
	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_queue_stream_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_queue_stream *queue_stream)
{
	if (!dev_priv || !submit || !queue_stream)
		return -EINVAL;
	if (queue_stream->active && queue_stream->submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_scheduler_entry_locked(
		dev_priv, submit, &queue_stream->scheduler_entry);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_queue_stream_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_queue_stream *queue_stream)
{
	const struct psb_sgx_lab_submit_sgx_ta_scheduler_entry *scheduler_entry;
	const struct psb_sgx_lab_submit_sgx_ta_queue_command *queue_command;
	const struct psb_sgx_lab_submit_sgx_ta_request *ta_request;
	const struct psb_sgx_lab_submit_sgx_issue_image *issue_image;
	const struct psb_sgx_lab_submit_sgx_transition_image *transition_image;
	int ret;

	if (!dev_priv || !submit || !queue_stream)
		return -EINVAL;

	scheduler_entry = &queue_stream->scheduler_entry;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_scheduler_entry_locked(
		dev_priv, submit, scheduler_entry);
	if (ret)
		return ret;

	queue_command = &scheduler_entry->queue_command;
	ta_request =
		&queue_command->queue_item.submit_request.ioctl_request.request;
	issue_image =
		&ta_request->job.work_queue.sequence.scheduler.ta_op.work_item
			 .event.commit.path.dispatch.issue_image;
	transition_image =
		&ta_request->job.work_queue.sequence.scheduler.ta_op.work_item
			 .event.commit.path.transition_image;

	if (!queue_stream->active ||
	    queue_stream->active != scheduler_entry->active ||
	    queue_stream->issue_request != scheduler_entry->issue_request ||
	    queue_stream->transition_request !=
		    scheduler_entry->transition_request ||
	    queue_stream->issue_request ==
		    queue_stream->transition_request ||
	    queue_stream->submit_id != scheduler_entry->submit_id ||
	    queue_stream->owner_state != scheduler_entry->owner_state ||
	    queue_stream->packet_count != scheduler_entry->packet_count ||
	    !queue_stream->stream_active ||
	    queue_stream->use_issue_stream ==
		    queue_stream->use_transition_stream) {
		return -EINVAL;
	}

	if (queue_stream->issue_request) {
		if (!queue_stream->use_issue_stream ||
		    queue_stream->use_transition_stream ||
		    !issue_image->active ||
		    queue_stream->stream_ccb_offset !=
			    issue_image->issue_write_offset ||
		    queue_stream->queue_region_end !=
			    issue_image->queue_region_end ||
		    queue_stream->sync_region_end !=
			    issue_image->sync_region_end ||
		    queue_stream->owner_region_end !=
			    issue_image->owner_region_end ||
		    memcmp(&queue_stream->queue_header, &issue_image->queue_header,
			   sizeof(queue_stream->queue_header)) ||
		    memcmp(&queue_stream->ccb_record, &issue_image->ccb_record,
			   sizeof(queue_stream->ccb_record)) ||
		    memcmp(&queue_stream->sync, &issue_image->sync,
			   sizeof(queue_stream->sync)) ||
		    memcmp(&queue_stream->owner, &issue_image->owner,
			   sizeof(queue_stream->owner)) ||
		    queue_stream->queue_header.write_offset !=
			    queue_command->next_write_offset ||
		    queue_stream->queue_header.queue_packet_count !=
			    queue_command->next_outstanding_packets ||
		    !!queue_stream->queue_header.queue_head_packet_count !=
			    queue_command->next_wrapped ||
		    queue_stream->sync.pending_seq !=
			    queue_command->next_pending_seq ||
		    queue_stream->sync.epoch != queue_command->next_epoch ||
		    queue_stream->owner.owner_issue_cookie !=
			    queue_command->next_issue_cookie) {
			return -EINVAL;
		}
		return 0;
	}

	if (!queue_stream->transition_request ||
	    queue_stream->use_issue_stream ||
	    !queue_stream->use_transition_stream ||
	    !transition_image->active ||
	    queue_stream->stream_ccb_offset !=
		    queue_command->next_ccb_offset ||
	    queue_stream->queue_region_end !=
		    transition_image->queue_region_end ||
	    queue_stream->sync_region_end !=
		    transition_image->sync_region_end ||
	    queue_stream->owner_region_end !=
		    transition_image->owner_region_end ||
	    memcmp(&queue_stream->queue_header, &transition_image->queue_header,
		   sizeof(queue_stream->queue_header)) ||
	    memcmp(&queue_stream->ccb_record, &transition_image->ccb_record,
		   sizeof(queue_stream->ccb_record)) ||
	    memcmp(&queue_stream->sync, &transition_image->sync,
		   sizeof(queue_stream->sync)) ||
	    memcmp(&queue_stream->owner, &transition_image->owner,
		   sizeof(queue_stream->owner)) ||
	    queue_stream->queue_header.read_offset !=
		    queue_command->next_read_offset ||
	    queue_stream->queue_header.write_offset !=
		    queue_command->next_write_offset ||
	    queue_stream->queue_header.queue_packet_count !=
		    queue_command->next_outstanding_packets ||
	    !!queue_stream->queue_header.queue_head_packet_count !=
		    queue_command->next_wrapped ||
	    queue_stream->sync.read_offset != queue_command->next_read_offset ||
	    queue_stream->sync.write_offset !=
		    queue_command->next_write_offset ||
	    queue_stream->sync.complete_seq !=
		    queue_command->next_complete_seq ||
	    queue_stream->sync.pending_seq !=
		    queue_command->next_pending_seq ||
	    queue_stream->sync.epoch != queue_command->next_epoch ||
	    queue_stream->owner.owner_issue_cookie !=
		    queue_command->next_issue_cookie ||
	    queue_stream->owner.owner_complete_cookie !=
		    queue_command->next_complete_cookie) {
		return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_queue_packet_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_queue_packet *queue_packet)
{
	const struct psb_sgx_lab_submit_sgx_ta_queue_stream *queue_stream;
	const struct psb_sgx_lab_submit_sgx_ta_scheduler_entry *scheduler_entry;
	const struct psb_sgx_lab_submit_sgx_ta_queue_command *queue_command;
	int ret;

	if (!dev_priv || !submit || !op || !queue_packet)
		return -EINVAL;

	memset(queue_packet, 0, sizeof(*queue_packet));

	ret = poulsbo_sgx_build_submit_sgx_ta_queue_stream_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &queue_packet->queue_stream);
	if (ret)
		return ret;

	queue_stream = &queue_packet->queue_stream;
	scheduler_entry = &queue_stream->scheduler_entry;
	queue_command = &scheduler_entry->queue_command;

	queue_packet->active = queue_stream->active;
	queue_packet->issue_request = queue_stream->issue_request;
	queue_packet->transition_request = queue_stream->transition_request;
	queue_packet->packet_active = queue_stream->stream_active;
	queue_packet->refilled_from_complete =
		queue_command->refilled_from_complete;
	queue_packet->read_advanced = queue_command->read_advanced;
	queue_packet->write_advanced = queue_command->write_advanced;
	queue_packet->ccb_advanced = queue_command->ccb_advanced;
	queue_packet->outstanding_changed = queue_command->outstanding_changed;
	queue_packet->pending_advanced = queue_command->pending_advanced;
	queue_packet->complete_advanced = queue_command->complete_advanced;
	queue_packet->issue_cookie_advanced =
		queue_command->issue_cookie_advanced;
	queue_packet->complete_cookie_advanced =
		queue_command->complete_cookie_advanced;
	queue_packet->epoch_advanced = queue_command->epoch_advanced;
	queue_packet->issue_ticket_advanced =
		queue_command->issue_ticket_advanced;
	queue_packet->running_ticket_advanced =
		queue_command->running_ticket_advanced;
	queue_packet->complete_ticket_advanced =
		queue_command->complete_ticket_advanced;
	queue_packet->scheduler_issue_advanced =
		scheduler_entry->issue_advanced;
	queue_packet->scheduler_running_advanced =
		scheduler_entry->running_advanced;
	queue_packet->scheduler_complete_advanced =
		scheduler_entry->complete_advanced;
	queue_packet->submit_id = queue_stream->submit_id;
	queue_packet->owner_state = queue_stream->owner_state;
	queue_packet->packet_count = queue_stream->packet_count;
	queue_packet->queue_read_offset = queue_stream->queue_header.read_offset;
	queue_packet->queue_write_offset = queue_stream->queue_header.write_offset;
	queue_packet->sync_read_offset = queue_stream->sync.read_offset;
	queue_packet->sync_write_offset = queue_stream->sync.write_offset;
	queue_packet->ccb_offset = queue_stream->stream_ccb_offset;
	queue_packet->queue_base_offset =
		queue_stream->queue_header.queue_base_offset;
	queue_packet->queue_size = queue_stream->queue_header.queue_size;
	queue_packet->queue_packet_size =
		queue_stream->queue_header.queue_packet_size;
	queue_packet->queue_packet_count =
		queue_stream->queue_header.queue_packet_count;
	queue_packet->queue_tail_packet_count =
		queue_stream->queue_header.queue_tail_packet_count;
	queue_packet->queue_head_packet_count =
		queue_stream->queue_header.queue_head_packet_count;
	queue_packet->sync_complete_seq = queue_stream->sync.complete_seq;
	queue_packet->sync_pending_seq = queue_stream->sync.pending_seq;
	queue_packet->sync_flags = queue_stream->sync.flags;
	queue_packet->owner_issue_cookie =
		queue_stream->owner.owner_issue_cookie;
	queue_packet->owner_complete_cookie =
		queue_stream->owner.owner_complete_cookie;
	queue_packet->epoch = queue_stream->sync.epoch;
	queue_packet->global_issue_ticket =
		scheduler_entry->next_state.issue_ticket;
	queue_packet->global_running_ticket =
		scheduler_entry->next_state.running_ticket;
	queue_packet->global_complete_ticket =
		scheduler_entry->next_state.complete_ticket;
	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_queue_packet_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_queue_packet *queue_packet)
{
	if (!dev_priv || !submit || !queue_packet)
		return -EINVAL;
	if (queue_packet->active && queue_packet->submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_queue_stream_locked(
		dev_priv, submit, &queue_packet->queue_stream);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_queue_packet_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_queue_packet *queue_packet)
{
	const struct psb_sgx_lab_submit_sgx_ta_queue_stream *queue_stream;
	const struct psb_sgx_lab_submit_sgx_ta_scheduler_entry *scheduler_entry;
	const struct psb_sgx_lab_submit_sgx_ta_queue_command *queue_command;
	int ret;

	if (!dev_priv || !submit || !queue_packet)
		return -EINVAL;

	queue_stream = &queue_packet->queue_stream;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_queue_stream_locked(
		dev_priv, submit, queue_stream);
	if (ret)
		return ret;

	scheduler_entry = &queue_stream->scheduler_entry;
	queue_command = &scheduler_entry->queue_command;

	if (!queue_packet->active ||
	    queue_packet->active != queue_stream->active ||
	    queue_packet->issue_request != queue_stream->issue_request ||
	    queue_packet->transition_request !=
		    queue_stream->transition_request ||
	    queue_packet->issue_request ==
		    queue_packet->transition_request ||
	    !queue_packet->packet_active ||
	    queue_packet->packet_active != queue_stream->stream_active ||
	    queue_packet->refilled_from_complete !=
		    queue_command->refilled_from_complete ||
	    queue_packet->read_advanced != queue_command->read_advanced ||
	    queue_packet->write_advanced != queue_command->write_advanced ||
	    queue_packet->ccb_advanced != queue_command->ccb_advanced ||
	    queue_packet->outstanding_changed !=
		    queue_command->outstanding_changed ||
	    queue_packet->pending_advanced !=
		    queue_command->pending_advanced ||
	    queue_packet->complete_advanced !=
		    queue_command->complete_advanced ||
	    queue_packet->issue_cookie_advanced !=
		    queue_command->issue_cookie_advanced ||
	    queue_packet->complete_cookie_advanced !=
		    queue_command->complete_cookie_advanced ||
	    queue_packet->epoch_advanced != queue_command->epoch_advanced ||
	    queue_packet->issue_ticket_advanced !=
		    queue_command->issue_ticket_advanced ||
	    queue_packet->running_ticket_advanced !=
		    queue_command->running_ticket_advanced ||
	    queue_packet->complete_ticket_advanced !=
		    queue_command->complete_ticket_advanced ||
	    queue_packet->scheduler_issue_advanced !=
		    scheduler_entry->issue_advanced ||
	    queue_packet->scheduler_running_advanced !=
		    scheduler_entry->running_advanced ||
	    queue_packet->scheduler_complete_advanced !=
		    scheduler_entry->complete_advanced ||
	    queue_packet->submit_id != queue_stream->submit_id ||
	    queue_packet->owner_state != queue_stream->owner_state ||
	    queue_packet->packet_count != queue_stream->packet_count ||
	    queue_packet->queue_read_offset !=
		    queue_stream->queue_header.read_offset ||
	    queue_packet->queue_write_offset !=
		    queue_stream->queue_header.write_offset ||
	    queue_packet->sync_read_offset != queue_stream->sync.read_offset ||
	    queue_packet->sync_write_offset != queue_stream->sync.write_offset ||
	    queue_packet->ccb_offset != queue_stream->stream_ccb_offset ||
	    queue_packet->queue_base_offset !=
		    queue_stream->queue_header.queue_base_offset ||
	    queue_packet->queue_size != queue_stream->queue_header.queue_size ||
	    queue_packet->queue_packet_size !=
		    queue_stream->queue_header.queue_packet_size ||
	    queue_packet->queue_packet_count !=
		    queue_stream->queue_header.queue_packet_count ||
	    queue_packet->queue_tail_packet_count !=
		    queue_stream->queue_header.queue_tail_packet_count ||
	    queue_packet->queue_head_packet_count !=
		    queue_stream->queue_header.queue_head_packet_count ||
	    queue_packet->sync_complete_seq !=
		    queue_stream->sync.complete_seq ||
	    queue_packet->sync_pending_seq !=
		    queue_stream->sync.pending_seq ||
	    queue_packet->sync_flags != queue_stream->sync.flags ||
	    queue_packet->owner_issue_cookie !=
		    queue_stream->owner.owner_issue_cookie ||
	    queue_packet->owner_complete_cookie !=
		    queue_stream->owner.owner_complete_cookie ||
	    queue_packet->epoch != queue_stream->sync.epoch ||
	    queue_packet->global_issue_ticket !=
		    scheduler_entry->next_state.issue_ticket ||
	    queue_packet->global_running_ticket !=
		    scheduler_entry->next_state.running_ticket ||
	    queue_packet->global_complete_ticket !=
		    scheduler_entry->next_state.complete_ticket) {
		return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_packet_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_packet *request_packet)
{
	const struct psb_sgx_lab_submit_sgx_ta_queue_packet *queue_packet;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request *submit_request;
	int ret;

	if (!dev_priv || !submit || !op || !request_packet)
		return -EINVAL;

	memset(request_packet, 0, sizeof(*request_packet));

	ret = poulsbo_sgx_build_submit_sgx_ta_queue_packet_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &request_packet->queue_packet);
	if (ret)
		return ret;

	queue_packet = &request_packet->queue_packet;
	submit_request =
		&queue_packet->queue_stream.scheduler_entry.queue_command
			 .queue_item.submit_request;

	request_packet->active = queue_packet->active;
	request_packet->dependency_fields_present =
		submit_request->dependency_fields_present;
	request_packet->use_focus_request =
		submit_request->use_focus_request;
	request_packet->focus_resolved =
		submit_request->focus_resolved;
	request_packet->issue_request = queue_packet->issue_request;
	request_packet->transition_request =
		queue_packet->transition_request;
	request_packet->packet_active = queue_packet->packet_active;
	request_packet->refilled_from_complete =
		queue_packet->refilled_from_complete;
	request_packet->requested_submit_id = submit_request->op.submit_id;
	request_packet->resolved_submit_id = submit_request->resolved_submit_id;
	request_packet->ioctl_op = submit_request->op.op;
	request_packet->op_kind = submit_request->op_kind;
	request_packet->owner_state = submit_request->op.owner_state;
	request_packet->packet_count = submit_request->op.packet_count;
	request_packet->selected_write_offset =
		queue_packet->queue_write_offset;
	request_packet->selected_ccb_offset = queue_packet->ccb_offset;
	request_packet->selected_outstanding_packets =
		queue_packet->queue_packet_count;
	request_packet->selected_wrapped =
		!!queue_packet->queue_head_packet_count;
	request_packet->epoch = queue_packet->epoch;
	request_packet->global_issue_ticket =
		queue_packet->global_issue_ticket;
	request_packet->global_running_ticket =
		queue_packet->global_running_ticket;
	request_packet->global_complete_ticket =
		queue_packet->global_complete_ticket;

	if (request_packet->issue_request) {
		request_packet->selected_read_offset =
			queue_packet->sync_read_offset;
		request_packet->selected_seq =
			queue_packet->sync_pending_seq;
		request_packet->selected_cookie =
			queue_packet->owner_issue_cookie;
		return 0;
	}

	if (!request_packet->transition_request)
		return -EINVAL;

	request_packet->selected_read_offset =
		queue_packet->queue_read_offset;
	request_packet->selected_seq = queue_packet->sync_complete_seq;
	request_packet->selected_cookie =
		queue_packet->owner_complete_cookie;
	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_packet_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_packet *request_packet)
{
	if (!dev_priv || !submit || !request_packet)
		return -EINVAL;
	if (request_packet->active &&
	    request_packet->resolved_submit_id != submit->submit_id) {
		return -EINVAL;
	}

	return poulsbo_sgx_apply_submit_sgx_ta_queue_packet_locked(
		dev_priv, submit, &request_packet->queue_packet);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_packet_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_packet *request_packet)
{
	const struct psb_sgx_lab_submit_sgx_ta_queue_packet *queue_packet;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request *submit_request;
	int ret;

	if (!dev_priv || !submit || !request_packet)
		return -EINVAL;

	queue_packet = &request_packet->queue_packet;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_queue_packet_locked(
		dev_priv, submit, queue_packet);
	if (ret)
		return ret;

	submit_request =
		&queue_packet->queue_stream.scheduler_entry.queue_command
			 .queue_item.submit_request;

	if (!request_packet->active ||
	    request_packet->active != queue_packet->active ||
	    request_packet->dependency_fields_present !=
		    submit_request->dependency_fields_present ||
	    request_packet->use_focus_request !=
		    submit_request->use_focus_request ||
	    request_packet->focus_resolved !=
		    submit_request->focus_resolved ||
	    request_packet->issue_request != queue_packet->issue_request ||
	    request_packet->transition_request !=
		    queue_packet->transition_request ||
	    request_packet->issue_request ==
		    request_packet->transition_request ||
	    !request_packet->packet_active ||
	    request_packet->packet_active != queue_packet->packet_active ||
	    request_packet->refilled_from_complete !=
		    queue_packet->refilled_from_complete ||
	    request_packet->requested_submit_id !=
		    submit_request->op.submit_id ||
	    request_packet->resolved_submit_id != submit->submit_id ||
	    request_packet->resolved_submit_id !=
		    submit_request->resolved_submit_id ||
	    request_packet->ioctl_op != submit_request->op.op ||
	    request_packet->ioctl_op !=
		    submit_request->ioctl_request.ioctl_op ||
	    request_packet->op_kind != submit_request->op_kind ||
	    request_packet->op_kind !=
		    submit_request->ioctl_request.op_kind ||
	    request_packet->owner_state != submit_request->op.owner_state ||
	    request_packet->owner_state != queue_packet->owner_state ||
	    request_packet->packet_count != submit_request->op.packet_count ||
	    request_packet->packet_count != queue_packet->packet_count ||
	    request_packet->selected_write_offset !=
		    queue_packet->queue_write_offset ||
	    request_packet->selected_ccb_offset !=
		    queue_packet->ccb_offset ||
	    request_packet->selected_outstanding_packets !=
		    queue_packet->queue_packet_count ||
	    request_packet->selected_wrapped !=
		    !!queue_packet->queue_head_packet_count ||
	    request_packet->epoch != queue_packet->epoch ||
	    request_packet->global_issue_ticket !=
		    queue_packet->global_issue_ticket ||
	    request_packet->global_running_ticket !=
		    queue_packet->global_running_ticket ||
	    request_packet->global_complete_ticket !=
		    queue_packet->global_complete_ticket) {
		return -EINVAL;
	}

	if (request_packet->issue_request) {
		if (request_packet->selected_read_offset !=
			    queue_packet->sync_read_offset ||
		    request_packet->selected_seq !=
			    queue_packet->sync_pending_seq ||
		    request_packet->selected_cookie !=
			    queue_packet->owner_issue_cookie) {
			return -EINVAL;
		}
		return 0;
	}

	if (!request_packet->transition_request ||
	    request_packet->selected_read_offset !=
		    queue_packet->queue_read_offset ||
	    request_packet->selected_seq !=
		    queue_packet->sync_complete_seq ||
	    request_packet->selected_cookie !=
		    queue_packet->owner_complete_cookie) {
		return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_sequence_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_sequence *request_sequence)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_packet *request_packet;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request *submit_request;
	const struct psb_sgx_lab_submit_sgx_ta_request *request;
	const struct psb_sgx_lab_submit_sgx_ta_sequence *sequence;
	const struct psb_sgx_lab_submit_sgx_ta_scheduler *scheduler;
	int ret;

	if (!dev_priv || !submit || !op || !request_sequence)
		return -EINVAL;

	memset(request_sequence, 0, sizeof(*request_sequence));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_packet_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &request_sequence->request_packet);
	if (ret)
		return ret;

	request_packet = &request_sequence->request_packet;
	submit_request =
		&request_packet->queue_packet.queue_stream.scheduler_entry
			 .queue_command.queue_item.submit_request;
	request = &submit_request->ioctl_request.request;
	sequence = &request->job.work_queue.sequence;
	scheduler = &sequence->scheduler;

	request_sequence->active = request_packet->active;
	request_sequence->issue_active = request->issue_active;
	request_sequence->transition_active = request->transition_active;
	request_sequence->refresh_views = sequence->refresh_views;
	request_sequence->increment_issue_count =
		sequence->increment_issue_count;
	request_sequence->increment_transition_count =
		sequence->increment_transition_count;
	request_sequence->update_last_epoch_jiffies =
		sequence->update_last_epoch_jiffies;
	request_sequence->update_last_issue_jiffies =
		sequence->update_last_issue_jiffies;
	request_sequence->update_last_transition_jiffies =
		sequence->update_last_transition_jiffies;
	request_sequence->issue_advanced = scheduler->issue_advanced;
	request_sequence->running_advanced = scheduler->running_advanced;
	request_sequence->complete_advanced = scheduler->complete_advanced;
	request_sequence->active_running_changed =
		scheduler->active_running_changed;
	request_sequence->next_issue_count = sequence->next_issue_count;
	request_sequence->next_transition_count = sequence->next_transition_count;
	request_sequence->next_state = scheduler->next_state;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_sequence_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_sequence *request_sequence)
{
	if (!dev_priv || !submit || !request_sequence)
		return -EINVAL;
	if (request_sequence->active &&
	    request_sequence->request_packet.resolved_submit_id !=
		    submit->submit_id) {
		return -EINVAL;
	}

	return poulsbo_sgx_apply_submit_sgx_ta_request_packet_locked(
		dev_priv, submit, &request_sequence->request_packet);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_sequence_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_sequence *request_sequence)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_packet *request_packet;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request *submit_request;
	const struct psb_sgx_lab_submit_sgx_ta_request *request;
	const struct psb_sgx_lab_submit_sgx_ta_sequence *sequence;
	const struct psb_sgx_lab_submit_sgx_ta_scheduler *scheduler;
	const struct psb_sgx_lab_submit_sgx_scheduler_state zero_state = {};
	int ret;

	if (!dev_priv || !submit || !request_sequence)
		return -EINVAL;

	request_packet = &request_sequence->request_packet;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_packet_locked(
		dev_priv, submit, request_packet);
	if (ret)
		return ret;

	submit_request =
		&request_packet->queue_packet.queue_stream.scheduler_entry
			 .queue_command.queue_item.submit_request;
	request = &submit_request->ioctl_request.request;
	sequence = &request->job.work_queue.sequence;
	scheduler = &sequence->scheduler;

	if (request_sequence->active != request_packet->active ||
	    request_sequence->active != request->active)
		return -EINVAL;

	if (!request_sequence->active) {
		if (request_sequence->issue_active ||
		    request_sequence->transition_active ||
		    request_sequence->refresh_views ||
		    request_sequence->increment_issue_count ||
		    request_sequence->increment_transition_count ||
		    request_sequence->update_last_epoch_jiffies ||
		    request_sequence->update_last_issue_jiffies ||
		    request_sequence->update_last_transition_jiffies ||
		    request_sequence->issue_advanced ||
		    request_sequence->running_advanced ||
		    request_sequence->complete_advanced ||
		    request_sequence->active_running_changed ||
		    request_sequence->next_issue_count ||
		    request_sequence->next_transition_count) {
			return -EINVAL;
		}
		return memcmp(&request_sequence->next_state, &zero_state,
			      sizeof(zero_state)) ? -EINVAL : 0;
	}

	if (request_sequence->issue_active != request->issue_active ||
	    request_sequence->issue_active != request_packet->issue_request ||
	    request_sequence->transition_active !=
		    request->transition_active ||
	    request_sequence->transition_active !=
		    request_packet->transition_request ||
	    request_sequence->refresh_views != sequence->refresh_views ||
	    request_sequence->increment_issue_count !=
		    sequence->increment_issue_count ||
	    request_sequence->increment_transition_count !=
		    sequence->increment_transition_count ||
	    request_sequence->update_last_epoch_jiffies !=
		    sequence->update_last_epoch_jiffies ||
	    request_sequence->update_last_issue_jiffies !=
		    sequence->update_last_issue_jiffies ||
	    request_sequence->update_last_transition_jiffies !=
		    sequence->update_last_transition_jiffies ||
	    request_sequence->issue_advanced != scheduler->issue_advanced ||
	    request_sequence->running_advanced !=
		    scheduler->running_advanced ||
	    request_sequence->complete_advanced !=
		    scheduler->complete_advanced ||
	    request_sequence->active_running_changed !=
		    scheduler->active_running_changed ||
	    request_sequence->next_issue_count != sequence->next_issue_count ||
	    request_sequence->next_transition_count !=
		    sequence->next_transition_count) {
		return -EINVAL;
	}

	if (request_sequence->next_state.issue_ticket !=
		    scheduler->next_state.issue_ticket ||
	    request_sequence->next_state.running_ticket !=
		    scheduler->next_state.running_ticket ||
	    request_sequence->next_state.complete_ticket !=
		    scheduler->next_state.complete_ticket ||
	    request_sequence->next_state.issue_submit_id !=
		    scheduler->next_state.issue_submit_id ||
	    request_sequence->next_state.running_submit_id !=
		    scheduler->next_state.running_submit_id ||
	    request_sequence->next_state.complete_submit_id !=
		    scheduler->next_state.complete_submit_id ||
	    request_sequence->next_state.active_running_valid !=
		    scheduler->next_state.active_running_valid ||
	    request_sequence->next_state.active_running_submit_id !=
		    scheduler->next_state.active_running_submit_id) {
		return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_work_packet_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_work_packet *work_packet)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_sequence *request_sequence;
	const struct psb_sgx_lab_submit_sgx_ta_request_packet *request_packet;
	int ret;

	if (!dev_priv || !submit || !op || !work_packet)
		return -EINVAL;

	memset(work_packet, 0, sizeof(*work_packet));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_sequence_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &work_packet->request_sequence);
	if (ret)
		return ret;

	request_sequence = &work_packet->request_sequence;
	request_packet = &request_sequence->request_packet;

	work_packet->active = request_sequence->active;
	work_packet->dependency_fields_present =
		request_packet->dependency_fields_present;
	work_packet->use_focus_request = request_packet->use_focus_request;
	work_packet->focus_resolved = request_packet->focus_resolved;
	work_packet->issue_request = request_packet->issue_request;
	work_packet->transition_request = request_packet->transition_request;
	work_packet->packet_active = request_packet->packet_active;
	work_packet->refilled_from_complete =
		request_packet->refilled_from_complete;
	work_packet->refresh_views = request_sequence->refresh_views;
	work_packet->increment_issue_count =
		request_sequence->increment_issue_count;
	work_packet->increment_transition_count =
		request_sequence->increment_transition_count;
	work_packet->update_last_epoch_jiffies =
		request_sequence->update_last_epoch_jiffies;
	work_packet->update_last_issue_jiffies =
		request_sequence->update_last_issue_jiffies;
	work_packet->update_last_transition_jiffies =
		request_sequence->update_last_transition_jiffies;
	work_packet->issue_advanced = request_sequence->issue_advanced;
	work_packet->running_advanced = request_sequence->running_advanced;
	work_packet->complete_advanced = request_sequence->complete_advanced;
	work_packet->active_running_changed =
		request_sequence->active_running_changed;
	work_packet->selected_wrapped = request_packet->selected_wrapped;
	work_packet->requested_submit_id = request_packet->requested_submit_id;
	work_packet->resolved_submit_id = request_packet->resolved_submit_id;
	work_packet->ioctl_op = request_packet->ioctl_op;
	work_packet->op_kind = request_packet->op_kind;
	work_packet->owner_state = request_packet->owner_state;
	work_packet->packet_count = request_packet->packet_count;
	work_packet->selected_read_offset =
		request_packet->selected_read_offset;
	work_packet->selected_write_offset =
		request_packet->selected_write_offset;
	work_packet->selected_ccb_offset = request_packet->selected_ccb_offset;
	work_packet->selected_outstanding_packets =
		request_packet->selected_outstanding_packets;
	work_packet->selected_seq = request_packet->selected_seq;
	work_packet->selected_cookie = request_packet->selected_cookie;
	work_packet->epoch = request_packet->epoch;
	work_packet->global_issue_ticket =
		request_packet->global_issue_ticket;
	work_packet->global_running_ticket =
		request_packet->global_running_ticket;
	work_packet->global_complete_ticket =
		request_packet->global_complete_ticket;
	work_packet->next_issue_count = request_sequence->next_issue_count;
	work_packet->next_transition_count =
		request_sequence->next_transition_count;
	work_packet->next_state = request_sequence->next_state;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_work_packet_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_packet *work_packet)
{
	if (!dev_priv || !submit || !work_packet)
		return -EINVAL;
	if (work_packet->active &&
	    work_packet->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_sequence_locked(
		dev_priv, submit, &work_packet->request_sequence);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_work_packet_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_packet *work_packet)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_sequence *request_sequence;
	const struct psb_sgx_lab_submit_sgx_ta_request_packet *request_packet;
	int ret;

	if (!dev_priv || !submit || !work_packet)
		return -EINVAL;

	request_sequence = &work_packet->request_sequence;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_sequence_locked(
		dev_priv, submit, request_sequence);
	if (ret)
		return ret;

	request_packet = &request_sequence->request_packet;
	if (work_packet->active != request_sequence->active ||
	    work_packet->active != request_packet->active ||
	    work_packet->dependency_fields_present !=
		    request_packet->dependency_fields_present ||
	    work_packet->use_focus_request !=
		    request_packet->use_focus_request ||
	    work_packet->focus_resolved != request_packet->focus_resolved ||
	    work_packet->issue_request != request_packet->issue_request ||
	    work_packet->transition_request !=
		    request_packet->transition_request ||
	    work_packet->packet_active != request_packet->packet_active ||
	    work_packet->refilled_from_complete !=
		    request_packet->refilled_from_complete ||
	    work_packet->refresh_views != request_sequence->refresh_views ||
	    work_packet->increment_issue_count !=
		    request_sequence->increment_issue_count ||
	    work_packet->increment_transition_count !=
		    request_sequence->increment_transition_count ||
	    work_packet->update_last_epoch_jiffies !=
		    request_sequence->update_last_epoch_jiffies ||
	    work_packet->update_last_issue_jiffies !=
		    request_sequence->update_last_issue_jiffies ||
	    work_packet->update_last_transition_jiffies !=
		    request_sequence->update_last_transition_jiffies ||
	    work_packet->issue_advanced !=
		    request_sequence->issue_advanced ||
	    work_packet->running_advanced !=
		    request_sequence->running_advanced ||
	    work_packet->complete_advanced !=
		    request_sequence->complete_advanced ||
	    work_packet->active_running_changed !=
		    request_sequence->active_running_changed ||
	    work_packet->selected_wrapped !=
		    request_packet->selected_wrapped ||
	    work_packet->requested_submit_id !=
		    request_packet->requested_submit_id ||
	    work_packet->resolved_submit_id !=
		    request_packet->resolved_submit_id ||
	    work_packet->ioctl_op != request_packet->ioctl_op ||
	    work_packet->op_kind != request_packet->op_kind ||
	    work_packet->owner_state != request_packet->owner_state ||
	    work_packet->packet_count != request_packet->packet_count ||
	    work_packet->selected_read_offset !=
		    request_packet->selected_read_offset ||
	    work_packet->selected_write_offset !=
		    request_packet->selected_write_offset ||
	    work_packet->selected_ccb_offset !=
		    request_packet->selected_ccb_offset ||
	    work_packet->selected_outstanding_packets !=
		    request_packet->selected_outstanding_packets ||
	    work_packet->selected_seq != request_packet->selected_seq ||
	    work_packet->selected_cookie != request_packet->selected_cookie ||
	    work_packet->epoch != request_packet->epoch ||
	    work_packet->global_issue_ticket !=
		    request_packet->global_issue_ticket ||
	    work_packet->global_running_ticket !=
		    request_packet->global_running_ticket ||
	    work_packet->global_complete_ticket !=
		    request_packet->global_complete_ticket ||
	    work_packet->next_issue_count !=
		    request_sequence->next_issue_count ||
	    work_packet->next_transition_count !=
		    request_sequence->next_transition_count)
		return -EINVAL;

	if (memcmp(&work_packet->next_state, &request_sequence->next_state,
		   sizeof(work_packet->next_state)))
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_execution_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_execution *execution)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_packet *work_packet;
	int ret;

	if (!dev_priv || !submit || !op || !execution)
		return -EINVAL;

	memset(execution, 0, sizeof(*execution));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_work_packet_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &execution->work_packet);
	if (ret)
		return ret;

	work_packet = &execution->work_packet;
	execution->op = *op;
	execution->active = work_packet->active;
	execution->dependency_fields_present =
		work_packet->dependency_fields_present;
	execution->use_focus_request = work_packet->use_focus_request;
	execution->focus_resolved = work_packet->focus_resolved;
	execution->issue_request = work_packet->issue_request;
	execution->transition_request = work_packet->transition_request;
	execution->submit_id = op->submit_id;
	execution->requested_submit_id = work_packet->requested_submit_id;
	execution->resolved_submit_id = work_packet->resolved_submit_id;
	execution->ioctl_op = work_packet->ioctl_op;
	execution->op_kind = work_packet->op_kind;
	execution->owner_state = work_packet->owner_state;
	execution->packet_count = work_packet->packet_count;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_execution_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_execution *execution)
{
	if (!dev_priv || !submit || !execution)
		return -EINVAL;
	if (execution->active &&
	    execution->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_work_packet_locked(
		dev_priv, submit, &execution->work_packet);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_execution_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_execution *execution)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_packet *work_packet;
	int ret;

	if (!dev_priv || !submit || !execution)
		return -EINVAL;

	work_packet = &execution->work_packet;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_work_packet_locked(
		dev_priv, submit, work_packet);
	if (ret)
		return ret;

	if (execution->active != work_packet->active ||
	    execution->dependency_fields_present !=
		    work_packet->dependency_fields_present ||
	    execution->use_focus_request != work_packet->use_focus_request ||
	    execution->focus_resolved != work_packet->focus_resolved ||
	    execution->issue_request != work_packet->issue_request ||
	    execution->transition_request !=
		    work_packet->transition_request ||
	    execution->submit_id != execution->op.submit_id ||
	    execution->requested_submit_id != work_packet->requested_submit_id ||
	    execution->resolved_submit_id != work_packet->resolved_submit_id ||
	    execution->ioctl_op != work_packet->ioctl_op ||
	    execution->ioctl_op != execution->op.op ||
	    execution->op_kind != work_packet->op_kind ||
	    execution->owner_state != work_packet->owner_state ||
	    execution->owner_state != execution->op.owner_state ||
	    execution->packet_count != work_packet->packet_count ||
	    execution->packet_count != execution->op.packet_count)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_work_entry_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_work_entry *work_entry)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_execution *execution;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_packet *work_packet;
	int ret;

	if (!dev_priv || !submit || !op || !work_entry)
		return -EINVAL;

	memset(work_entry, 0, sizeof(*work_entry));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_execution_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &work_entry->execution);
	if (ret)
		return ret;

	execution = &work_entry->execution;
	work_packet = &execution->work_packet;
	work_entry->active = execution->active;
	work_entry->dependency_fields_present =
		execution->dependency_fields_present;
	work_entry->use_focus_request = execution->use_focus_request;
	work_entry->focus_resolved = execution->focus_resolved;
	work_entry->issue_request = execution->issue_request;
	work_entry->transition_request = execution->transition_request;
	work_entry->packet_active = work_packet->packet_active;
	work_entry->refilled_from_complete = work_packet->refilled_from_complete;
	work_entry->refresh_views = work_packet->refresh_views;
	work_entry->increment_issue_count =
		work_packet->increment_issue_count;
	work_entry->increment_transition_count =
		work_packet->increment_transition_count;
	work_entry->update_last_epoch_jiffies =
		work_packet->update_last_epoch_jiffies;
	work_entry->update_last_issue_jiffies =
		work_packet->update_last_issue_jiffies;
	work_entry->update_last_transition_jiffies =
		work_packet->update_last_transition_jiffies;
	work_entry->issue_advanced = work_packet->issue_advanced;
	work_entry->running_advanced = work_packet->running_advanced;
	work_entry->complete_advanced = work_packet->complete_advanced;
	work_entry->active_running_changed =
		work_packet->active_running_changed;
	work_entry->selected_wrapped = work_packet->selected_wrapped;
	work_entry->submit_id = execution->submit_id;
	work_entry->requested_submit_id = execution->requested_submit_id;
	work_entry->resolved_submit_id = execution->resolved_submit_id;
	work_entry->ioctl_op = execution->ioctl_op;
	work_entry->op_kind = execution->op_kind;
	work_entry->owner_state = execution->owner_state;
	work_entry->packet_count = execution->packet_count;
	work_entry->selected_read_offset = work_packet->selected_read_offset;
	work_entry->selected_write_offset = work_packet->selected_write_offset;
	work_entry->selected_ccb_offset = work_packet->selected_ccb_offset;
	work_entry->selected_outstanding_packets =
		work_packet->selected_outstanding_packets;
	work_entry->selected_seq = work_packet->selected_seq;
	work_entry->selected_cookie = work_packet->selected_cookie;
	work_entry->epoch = work_packet->epoch;
	work_entry->global_issue_ticket = work_packet->global_issue_ticket;
	work_entry->global_running_ticket = work_packet->global_running_ticket;
	work_entry->global_complete_ticket =
		work_packet->global_complete_ticket;
	work_entry->next_issue_count = work_packet->next_issue_count;
	work_entry->next_transition_count = work_packet->next_transition_count;
	work_entry->next_state = work_packet->next_state;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_work_entry_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_entry *work_entry)
{
	if (!dev_priv || !submit || !work_entry)
		return -EINVAL;
	if (work_entry->active &&
	    work_entry->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_execution_locked(
		dev_priv, submit, &work_entry->execution);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_work_entry_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_entry *work_entry)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_execution *execution;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_packet *work_packet;
	int ret;

	if (!dev_priv || !submit || !work_entry)
		return -EINVAL;

	execution = &work_entry->execution;
	work_packet = &execution->work_packet;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_execution_locked(
		dev_priv, submit, execution);
	if (ret)
		return ret;

	if (work_entry->active != execution->active ||
	    work_entry->dependency_fields_present !=
		    execution->dependency_fields_present ||
	    work_entry->use_focus_request != execution->use_focus_request ||
	    work_entry->focus_resolved != execution->focus_resolved ||
	    work_entry->issue_request != execution->issue_request ||
	    work_entry->transition_request != execution->transition_request ||
	    work_entry->submit_id != execution->submit_id ||
	    work_entry->requested_submit_id != execution->requested_submit_id ||
	    work_entry->resolved_submit_id != execution->resolved_submit_id ||
	    work_entry->ioctl_op != execution->ioctl_op ||
	    work_entry->op_kind != execution->op_kind ||
	    work_entry->owner_state != execution->owner_state ||
	    work_entry->packet_count != execution->packet_count ||
	    work_entry->packet_active != work_packet->packet_active ||
	    work_entry->refilled_from_complete !=
		    work_packet->refilled_from_complete ||
	    work_entry->refresh_views != work_packet->refresh_views ||
	    work_entry->increment_issue_count !=
		    work_packet->increment_issue_count ||
	    work_entry->increment_transition_count !=
		    work_packet->increment_transition_count ||
	    work_entry->update_last_epoch_jiffies !=
		    work_packet->update_last_epoch_jiffies ||
	    work_entry->update_last_issue_jiffies !=
		    work_packet->update_last_issue_jiffies ||
	    work_entry->update_last_transition_jiffies !=
		    work_packet->update_last_transition_jiffies ||
	    work_entry->issue_advanced != work_packet->issue_advanced ||
	    work_entry->running_advanced != work_packet->running_advanced ||
	    work_entry->complete_advanced != work_packet->complete_advanced ||
	    work_entry->active_running_changed !=
		    work_packet->active_running_changed ||
	    work_entry->selected_wrapped != work_packet->selected_wrapped ||
	    work_entry->selected_read_offset !=
		    work_packet->selected_read_offset ||
	    work_entry->selected_write_offset !=
		    work_packet->selected_write_offset ||
	    work_entry->selected_ccb_offset !=
		    work_packet->selected_ccb_offset ||
	    work_entry->selected_outstanding_packets !=
		    work_packet->selected_outstanding_packets ||
	    work_entry->selected_seq != work_packet->selected_seq ||
	    work_entry->selected_cookie != work_packet->selected_cookie ||
	    work_entry->epoch != work_packet->epoch ||
	    work_entry->global_issue_ticket !=
		    work_packet->global_issue_ticket ||
	    work_entry->global_running_ticket !=
		    work_packet->global_running_ticket ||
	    work_entry->global_complete_ticket !=
		    work_packet->global_complete_ticket ||
	    work_entry->next_issue_count != work_packet->next_issue_count ||
	    work_entry->next_transition_count !=
		    work_packet->next_transition_count)
		return -EINVAL;

	if (memcmp(&work_entry->next_state, &work_packet->next_state,
		   sizeof(work_entry->next_state)))
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_work_result_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_work_result *work_result)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_entry *work_entry;
	int ret;

	if (!dev_priv || !submit || !op || !work_result)
		return -EINVAL;

	memset(work_result, 0, sizeof(*work_result));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_work_entry_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &work_result->work_entry);
	if (ret)
		return ret;

	work_entry = &work_result->work_entry;
	work_result->active = work_entry->active;
	work_result->issue_request = work_entry->issue_request;
	work_result->transition_request = work_entry->transition_request;
	work_result->submit_id = work_entry->submit_id;
	work_result->resolved_submit_id = work_entry->resolved_submit_id;
	work_result->owner_state = work_entry->owner_state;
	work_result->packet_count = work_entry->packet_count;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_work_result_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_result *work_result)
{
	if (!dev_priv || !submit || !work_result)
		return -EINVAL;
	if (work_result->active &&
	    work_result->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_work_entry_locked(
		dev_priv, submit, &work_result->work_entry);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_work_result_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_result *work_result)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_entry *work_entry;
	int ret;

	if (!dev_priv || !submit || !work_result)
		return -EINVAL;

	work_entry = &work_result->work_entry;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_work_entry_locked(
		dev_priv, submit, work_entry);
	if (ret)
		return ret;

	if (work_result->active != work_entry->active ||
	    work_result->issue_request != work_entry->issue_request ||
	    work_result->transition_request !=
		    work_entry->transition_request ||
	    work_result->submit_id != work_entry->submit_id ||
	    work_result->resolved_submit_id != work_entry->resolved_submit_id ||
	    work_result->owner_state != work_entry->owner_state ||
	    work_result->packet_count != work_entry->packet_count)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_work_record_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_work_record *work_record)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_result *work_result;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_entry *work_entry;
	int ret;

	if (!dev_priv || !submit || !op || !work_record)
		return -EINVAL;

	memset(work_record, 0, sizeof(*work_record));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_work_result_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &work_record->work_result);
	if (ret)
		return ret;

	work_result = &work_record->work_result;
	work_entry = &work_result->work_entry;
	work_record->active = work_result->active;
	work_record->issue_request = work_result->issue_request;
	work_record->transition_request = work_result->transition_request;
	work_record->submit_id = work_result->submit_id;
	work_record->resolved_submit_id = work_result->resolved_submit_id;
	work_record->owner_state = work_result->owner_state;
	work_record->packet_count = work_result->packet_count;
	work_record->refresh_views = work_entry->refresh_views;
	work_record->refilled_from_complete = work_entry->refilled_from_complete;
	work_record->issue_advanced = work_entry->issue_advanced;
	work_record->running_advanced = work_entry->running_advanced;
	work_record->complete_advanced = work_entry->complete_advanced;
	work_record->active_running_changed = work_entry->active_running_changed;
	work_record->selected_wrapped = work_entry->selected_wrapped;
	work_record->selected_read_offset = work_entry->selected_read_offset;
	work_record->selected_write_offset = work_entry->selected_write_offset;
	work_record->selected_ccb_offset = work_entry->selected_ccb_offset;
	work_record->selected_outstanding_packets =
		work_entry->selected_outstanding_packets;
	work_record->selected_seq = work_entry->selected_seq;
	work_record->selected_cookie = work_entry->selected_cookie;
	work_record->epoch = work_entry->epoch;
	work_record->global_issue_ticket = work_entry->global_issue_ticket;
	work_record->global_running_ticket =
		work_entry->global_running_ticket;
	work_record->global_complete_ticket =
		work_entry->global_complete_ticket;
	work_record->next_issue_count = work_entry->next_issue_count;
	work_record->next_transition_count =
		work_entry->next_transition_count;
	work_record->next_state = work_entry->next_state;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_work_record_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_record *work_record)
{
	if (!dev_priv || !submit || !work_record)
		return -EINVAL;
	if (work_record->active &&
	    work_record->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_work_result_locked(
		dev_priv, submit, &work_record->work_result);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_work_record_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_record *work_record)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_result *work_result;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_entry *work_entry;
	int ret;

	if (!dev_priv || !submit || !work_record)
		return -EINVAL;

	work_result = &work_record->work_result;
	work_entry = &work_result->work_entry;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_work_result_locked(
		dev_priv, submit, work_result);
	if (ret)
		return ret;

	if (work_record->active != work_result->active ||
	    work_record->issue_request != work_result->issue_request ||
	    work_record->transition_request !=
		    work_result->transition_request ||
	    work_record->submit_id != work_result->submit_id ||
	    work_record->resolved_submit_id !=
		    work_result->resolved_submit_id ||
	    work_record->owner_state != work_result->owner_state ||
	    work_record->packet_count != work_result->packet_count ||
	    work_record->refresh_views != work_entry->refresh_views ||
	    work_record->refilled_from_complete !=
		    work_entry->refilled_from_complete ||
	    work_record->issue_advanced != work_entry->issue_advanced ||
	    work_record->running_advanced != work_entry->running_advanced ||
	    work_record->complete_advanced != work_entry->complete_advanced ||
	    work_record->active_running_changed !=
		    work_entry->active_running_changed ||
	    work_record->selected_wrapped != work_entry->selected_wrapped ||
	    work_record->selected_read_offset !=
		    work_entry->selected_read_offset ||
	    work_record->selected_write_offset !=
		    work_entry->selected_write_offset ||
	    work_record->selected_ccb_offset !=
		    work_entry->selected_ccb_offset ||
	    work_record->selected_outstanding_packets !=
		    work_entry->selected_outstanding_packets ||
	    work_record->selected_seq != work_entry->selected_seq ||
	    work_record->selected_cookie != work_entry->selected_cookie ||
	    work_record->epoch != work_entry->epoch ||
	    work_record->global_issue_ticket !=
		    work_entry->global_issue_ticket ||
	    work_record->global_running_ticket !=
		    work_entry->global_running_ticket ||
	    work_record->global_complete_ticket !=
		    work_entry->global_complete_ticket ||
	    work_record->next_issue_count != work_entry->next_issue_count ||
	    work_record->next_transition_count !=
		    work_entry->next_transition_count)
		return -EINVAL;

	if (memcmp(&work_record->next_state, &work_entry->next_state,
		   sizeof(work_record->next_state)))
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_work_request_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_work_request *work_request)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_record *work_record;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_entry *work_entry;
	const struct psb_sgx_lab_submit_sgx_ta_request_execution *execution;
	int ret;

	if (!dev_priv || !submit || !op || !work_request)
		return -EINVAL;

	memset(work_request, 0, sizeof(*work_request));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_work_record_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &work_request->work_record);
	if (ret)
		return ret;

	work_record = &work_request->work_record;
	work_entry = &work_record->work_result.work_entry;
	execution = &work_entry->execution;
	work_request->op = execution->op;
	work_request->active = execution->active;
	work_request->dependency_fields_present =
		execution->dependency_fields_present;
	work_request->use_focus_request = execution->use_focus_request;
	work_request->focus_resolved = execution->focus_resolved;
	work_request->issue_request = work_record->issue_request;
	work_request->transition_request = work_record->transition_request;
	work_request->submit_id = execution->submit_id;
	work_request->requested_submit_id = execution->requested_submit_id;
	work_request->resolved_submit_id = execution->resolved_submit_id;
	work_request->ioctl_op = execution->ioctl_op;
	work_request->op_kind = execution->op_kind;
	work_request->owner_state = execution->owner_state;
	work_request->packet_count = execution->packet_count;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_work_request_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_request *work_request)
{
	if (!dev_priv || !submit || !work_request)
		return -EINVAL;
	if (work_request->active &&
	    work_request->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_work_record_locked(
		dev_priv, submit, &work_request->work_record);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_work_request_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_request *work_request)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_record *work_record;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_entry *work_entry;
	const struct psb_sgx_lab_submit_sgx_ta_request_execution *execution;
	int ret;

	if (!dev_priv || !submit || !work_request)
		return -EINVAL;

	work_record = &work_request->work_record;
	work_entry = &work_record->work_result.work_entry;
	execution = &work_entry->execution;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_work_record_locked(
		dev_priv, submit, work_record);
	if (ret)
		return ret;

	if (memcmp(&work_request->op, &execution->op,
		   sizeof(work_request->op)) ||
	    work_request->active != execution->active ||
	    work_request->dependency_fields_present !=
		    execution->dependency_fields_present ||
	    work_request->use_focus_request != execution->use_focus_request ||
	    work_request->focus_resolved != execution->focus_resolved ||
	    work_request->issue_request != work_record->issue_request ||
	    work_request->transition_request !=
		    work_record->transition_request ||
	    work_request->submit_id != execution->submit_id ||
	    work_request->requested_submit_id !=
		    execution->requested_submit_id ||
	    work_request->resolved_submit_id !=
		    execution->resolved_submit_id ||
	    work_request->ioctl_op != execution->ioctl_op ||
	    work_request->op_kind != execution->op_kind ||
	    work_request->owner_state != execution->owner_state ||
	    work_request->packet_count != execution->packet_count)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_work_operation_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_work_operation *work_operation)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_request *work_request;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_record *work_record;
	int ret;

	if (!dev_priv || !submit || !op || !work_operation)
		return -EINVAL;

	memset(work_operation, 0, sizeof(*work_operation));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_work_request_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &work_operation->work_request);
	if (ret)
		return ret;

	work_request = &work_operation->work_request;
	work_record = &work_request->work_record;
	work_operation->op = work_request->op;
	work_operation->active = work_request->active;
	work_operation->dependency_fields_present =
		work_request->dependency_fields_present;
	work_operation->use_focus_request = work_request->use_focus_request;
	work_operation->focus_resolved = work_request->focus_resolved;
	work_operation->issue_request = work_request->issue_request;
	work_operation->transition_request = work_request->transition_request;
	work_operation->refresh_views = work_record->refresh_views;
	work_operation->refilled_from_complete =
		work_record->refilled_from_complete;
	work_operation->issue_advanced = work_record->issue_advanced;
	work_operation->running_advanced = work_record->running_advanced;
	work_operation->complete_advanced = work_record->complete_advanced;
	work_operation->active_running_changed =
		work_record->active_running_changed;
	work_operation->selected_wrapped = work_record->selected_wrapped;
	work_operation->submit_id = work_request->submit_id;
	work_operation->requested_submit_id = work_request->requested_submit_id;
	work_operation->resolved_submit_id = work_request->resolved_submit_id;
	work_operation->ioctl_op = work_request->ioctl_op;
	work_operation->op_kind = work_request->op_kind;
	work_operation->owner_state = work_request->owner_state;
	work_operation->packet_count = work_request->packet_count;
	work_operation->selected_read_offset = work_record->selected_read_offset;
	work_operation->selected_write_offset = work_record->selected_write_offset;
	work_operation->selected_ccb_offset = work_record->selected_ccb_offset;
	work_operation->selected_outstanding_packets =
		work_record->selected_outstanding_packets;
	work_operation->selected_seq = work_record->selected_seq;
	work_operation->selected_cookie = work_record->selected_cookie;
	work_operation->epoch = work_record->epoch;
	work_operation->global_issue_ticket = work_record->global_issue_ticket;
	work_operation->global_running_ticket =
		work_record->global_running_ticket;
	work_operation->global_complete_ticket =
		work_record->global_complete_ticket;
	work_operation->next_issue_count = work_record->next_issue_count;
	work_operation->next_transition_count =
		work_record->next_transition_count;
	work_operation->next_state = work_record->next_state;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_work_operation_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_operation *work_operation)
{
	if (!dev_priv || !submit || !work_operation)
		return -EINVAL;
	if (work_operation->active &&
	    work_operation->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_work_request_locked(
		dev_priv, submit, &work_operation->work_request);
}

static int
poulsbo_sgx_validate_built_submit_sgx_ta_request_work_operation_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_operation *work_operation)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_request *work_request;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_record *work_record;
	int ret;

	if (!dev_priv || !submit || !work_operation)
		return -EINVAL;

	work_request = &work_operation->work_request;
	work_record = &work_request->work_record;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_work_request_locked(
		dev_priv, submit, work_request);
	if (ret)
		return ret;

	if (memcmp(&work_operation->op, &work_request->op,
		   sizeof(work_operation->op)) ||
	    work_operation->active != work_request->active ||
	    work_operation->dependency_fields_present !=
		    work_request->dependency_fields_present ||
	    work_operation->use_focus_request !=
		    work_request->use_focus_request ||
	    work_operation->focus_resolved != work_request->focus_resolved ||
	    work_operation->issue_request != work_request->issue_request ||
	    work_operation->transition_request !=
		    work_request->transition_request ||
	    work_operation->refresh_views != work_record->refresh_views ||
	    work_operation->refilled_from_complete !=
		    work_record->refilled_from_complete ||
	    work_operation->issue_advanced != work_record->issue_advanced ||
	    work_operation->running_advanced != work_record->running_advanced ||
	    work_operation->complete_advanced !=
		    work_record->complete_advanced ||
	    work_operation->active_running_changed !=
		    work_record->active_running_changed ||
	    work_operation->selected_wrapped != work_record->selected_wrapped ||
	    work_operation->submit_id != work_request->submit_id ||
	    work_operation->requested_submit_id !=
		    work_request->requested_submit_id ||
	    work_operation->resolved_submit_id !=
		    work_request->resolved_submit_id ||
	    work_operation->ioctl_op != work_request->ioctl_op ||
	    work_operation->op_kind != work_request->op_kind ||
	    work_operation->owner_state != work_request->owner_state ||
	    work_operation->packet_count != work_request->packet_count ||
	    work_operation->selected_read_offset !=
		    work_record->selected_read_offset ||
	    work_operation->selected_write_offset !=
		    work_record->selected_write_offset ||
	    work_operation->selected_ccb_offset !=
		    work_record->selected_ccb_offset ||
	    work_operation->selected_outstanding_packets !=
		    work_record->selected_outstanding_packets ||
	    work_operation->selected_seq != work_record->selected_seq ||
	    work_operation->selected_cookie != work_record->selected_cookie ||
	    work_operation->epoch != work_record->epoch ||
	    work_operation->global_issue_ticket !=
		    work_record->global_issue_ticket ||
	    work_operation->global_running_ticket !=
		    work_record->global_running_ticket ||
	    work_operation->global_complete_ticket !=
		    work_record->global_complete_ticket ||
	    work_operation->next_issue_count !=
		    work_record->next_issue_count ||
	    work_operation->next_transition_count !=
		    work_record->next_transition_count)
		return -EINVAL;

	if (memcmp(&work_operation->next_state, &work_record->next_state,
		   sizeof(work_operation->next_state)))
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_work_event_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_work_event *work_event)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_operation *work_operation;

	int ret;

	if (!dev_priv || !submit || !op || !work_event)
		return -EINVAL;

	memset(work_event, 0, sizeof(*work_event));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_work_operation_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &work_event->work_operation);
	if (ret)
		return ret;

	work_operation = &work_event->work_operation;
	work_event->active = work_operation->active;
	work_event->issue_request = work_operation->issue_request;
	work_event->transition_request = work_operation->transition_request;
	work_event->refresh_views = work_operation->refresh_views;
	work_event->refilled_from_complete =
		work_operation->refilled_from_complete;
	work_event->issue_advanced = work_operation->issue_advanced;
	work_event->running_advanced = work_operation->running_advanced;
	work_event->complete_advanced = work_operation->complete_advanced;
	work_event->active_running_changed =
		work_operation->active_running_changed;
	work_event->selected_wrapped = work_operation->selected_wrapped;
	work_event->submit_id = work_operation->submit_id;
	work_event->resolved_submit_id = work_operation->resolved_submit_id;
	work_event->owner_state = work_operation->owner_state;
	work_event->packet_count = work_operation->packet_count;
	work_event->selected_read_offset = work_operation->selected_read_offset;
	work_event->selected_write_offset = work_operation->selected_write_offset;
	work_event->selected_ccb_offset = work_operation->selected_ccb_offset;
	work_event->selected_outstanding_packets =
		work_operation->selected_outstanding_packets;
	work_event->selected_seq = work_operation->selected_seq;
	work_event->selected_cookie = work_operation->selected_cookie;
	work_event->epoch = work_operation->epoch;
	work_event->global_issue_ticket = work_operation->global_issue_ticket;
	work_event->global_running_ticket =
		work_operation->global_running_ticket;
	work_event->global_complete_ticket =
		work_operation->global_complete_ticket;
	work_event->next_issue_count = work_operation->next_issue_count;
	work_event->next_transition_count =
		work_operation->next_transition_count;
	work_event->next_state = work_operation->next_state;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_work_event_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_event *work_event)
{
	if (!dev_priv || !submit || !work_event)
		return -EINVAL;
	if (work_event->active &&
	    work_event->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_work_operation_locked(
		dev_priv, submit, &work_event->work_operation);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_work_event_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_event *work_event)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_operation *work_operation;
	int ret;

	if (!dev_priv || !submit || !work_event)
		return -EINVAL;

	work_operation = &work_event->work_operation;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_work_operation_locked(
		dev_priv, submit, work_operation);
	if (ret)
		return ret;

	if (work_event->active != work_operation->active ||
	    work_event->issue_request != work_operation->issue_request ||
	    work_event->transition_request !=
		    work_operation->transition_request ||
	    work_event->refresh_views != work_operation->refresh_views ||
	    work_event->refilled_from_complete !=
		    work_operation->refilled_from_complete ||
	    work_event->issue_advanced != work_operation->issue_advanced ||
	    work_event->running_advanced != work_operation->running_advanced ||
	    work_event->complete_advanced !=
		    work_operation->complete_advanced ||
	    work_event->active_running_changed !=
		    work_operation->active_running_changed ||
	    work_event->selected_wrapped != work_operation->selected_wrapped ||
	    work_event->submit_id != work_operation->submit_id ||
	    work_event->resolved_submit_id !=
		    work_operation->resolved_submit_id ||
	    work_event->owner_state != work_operation->owner_state ||
	    work_event->packet_count != work_operation->packet_count ||
	    work_event->selected_read_offset !=
		    work_operation->selected_read_offset ||
	    work_event->selected_write_offset !=
		    work_operation->selected_write_offset ||
	    work_event->selected_ccb_offset !=
		    work_operation->selected_ccb_offset ||
	    work_event->selected_outstanding_packets !=
		    work_operation->selected_outstanding_packets ||
	    work_event->selected_seq != work_operation->selected_seq ||
	    work_event->selected_cookie != work_operation->selected_cookie ||
	    work_event->epoch != work_operation->epoch ||
	    work_event->global_issue_ticket !=
		    work_operation->global_issue_ticket ||
	    work_event->global_running_ticket !=
		    work_operation->global_running_ticket ||
	    work_event->global_complete_ticket !=
		    work_operation->global_complete_ticket ||
	    work_event->next_issue_count !=
		    work_operation->next_issue_count ||
	    work_event->next_transition_count !=
		    work_operation->next_transition_count)
		return -EINVAL;

	if (memcmp(&work_event->next_state, &work_operation->next_state,
		   sizeof(work_event->next_state)))
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_work_state_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_work_state *work_state)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_event *work_event;
	int ret;

	if (!dev_priv || !submit || !op || !work_state)
		return -EINVAL;

	memset(work_state, 0, sizeof(*work_state));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_work_event_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &work_state->work_event);
	if (ret)
		return ret;

	work_event = &work_state->work_event;
	work_state->active = work_event->active;
	work_state->issue_request = work_event->issue_request;
	work_state->transition_request = work_event->transition_request;
	work_state->refresh_views = work_event->refresh_views;
	work_state->refilled_from_complete = work_event->refilled_from_complete;
	work_state->issue_advanced = work_event->issue_advanced;
	work_state->running_advanced = work_event->running_advanced;
	work_state->complete_advanced = work_event->complete_advanced;
	work_state->active_running_changed =
		work_event->active_running_changed;
	work_state->selected_wrapped = work_event->selected_wrapped;
	work_state->submit_id = work_event->submit_id;
	work_state->resolved_submit_id = work_event->resolved_submit_id;
	work_state->owner_state = work_event->owner_state;
	work_state->packet_count = work_event->packet_count;
	work_state->selected_read_offset = work_event->selected_read_offset;
	work_state->selected_write_offset = work_event->selected_write_offset;
	work_state->selected_ccb_offset = work_event->selected_ccb_offset;
	work_state->selected_outstanding_packets =
		work_event->selected_outstanding_packets;
	work_state->selected_seq = work_event->selected_seq;
	work_state->selected_cookie = work_event->selected_cookie;
	work_state->epoch = work_event->epoch;
	work_state->global_issue_ticket = work_event->global_issue_ticket;
	work_state->global_running_ticket = work_event->global_running_ticket;
	work_state->global_complete_ticket =
		work_event->global_complete_ticket;
	work_state->next_issue_count = work_event->next_issue_count;
	work_state->next_transition_count = work_event->next_transition_count;
	work_state->next_state = work_event->next_state;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_work_state_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_state *work_state)
{
	if (!dev_priv || !submit || !work_state)
		return -EINVAL;
	if (work_state->active &&
	    work_state->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_work_event_locked(
		dev_priv, submit, &work_state->work_event);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_work_state_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_state *work_state)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_event *work_event;
	int ret;

	if (!dev_priv || !submit || !work_state)
		return -EINVAL;

	work_event = &work_state->work_event;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_work_event_locked(
		dev_priv, submit, work_event);
	if (ret)
		return ret;

	if (work_state->active != work_event->active ||
	    work_state->issue_request != work_event->issue_request ||
	    work_state->transition_request != work_event->transition_request ||
	    work_state->refresh_views != work_event->refresh_views ||
	    work_state->refilled_from_complete !=
		    work_event->refilled_from_complete ||
	    work_state->issue_advanced != work_event->issue_advanced ||
	    work_state->running_advanced != work_event->running_advanced ||
	    work_state->complete_advanced != work_event->complete_advanced ||
	    work_state->active_running_changed !=
		    work_event->active_running_changed ||
	    work_state->selected_wrapped != work_event->selected_wrapped ||
	    work_state->submit_id != work_event->submit_id ||
	    work_state->resolved_submit_id != work_event->resolved_submit_id ||
	    work_state->owner_state != work_event->owner_state ||
	    work_state->packet_count != work_event->packet_count ||
	    work_state->selected_read_offset !=
		    work_event->selected_read_offset ||
	    work_state->selected_write_offset !=
		    work_event->selected_write_offset ||
	    work_state->selected_ccb_offset !=
		    work_event->selected_ccb_offset ||
	    work_state->selected_outstanding_packets !=
		    work_event->selected_outstanding_packets ||
	    work_state->selected_seq != work_event->selected_seq ||
	    work_state->selected_cookie != work_event->selected_cookie ||
	    work_state->epoch != work_event->epoch ||
	    work_state->global_issue_ticket !=
		    work_event->global_issue_ticket ||
	    work_state->global_running_ticket !=
		    work_event->global_running_ticket ||
	    work_state->global_complete_ticket !=
		    work_event->global_complete_ticket ||
	    work_state->next_issue_count != work_event->next_issue_count ||
	    work_state->next_transition_count !=
		    work_event->next_transition_count)
		return -EINVAL;

	if (memcmp(&work_state->next_state, &work_event->next_state,
		   sizeof(work_state->next_state)))
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_work_action_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_work_action *work_action)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_state *work_state;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_operation *work_operation;
	int ret;

	if (!dev_priv || !submit || !op || !work_action)
		return -EINVAL;

	memset(work_action, 0, sizeof(*work_action));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_work_state_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &work_action->work_state);
	if (ret)
		return ret;

	work_state = &work_action->work_state;
	work_operation = &work_state->work_event.work_operation;
	work_action->op = work_operation->op;
	work_action->active = work_operation->active;
	work_action->dependency_fields_present =
		work_operation->dependency_fields_present;
	work_action->use_focus_request = work_operation->use_focus_request;
	work_action->focus_resolved = work_operation->focus_resolved;
	work_action->issue_request = work_state->issue_request;
	work_action->transition_request = work_state->transition_request;
	work_action->submit_id = work_state->submit_id;
	work_action->requested_submit_id = work_operation->requested_submit_id;
	work_action->resolved_submit_id = work_state->resolved_submit_id;
	work_action->ioctl_op = work_operation->ioctl_op;
	work_action->op_kind = work_operation->op_kind;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_work_action_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_action *work_action)
{
	if (!dev_priv || !submit || !work_action)
		return -EINVAL;
	if (work_action->active &&
	    work_action->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_work_state_locked(
		dev_priv, submit, &work_action->work_state);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_work_action_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_action *work_action)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_state *work_state;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_operation *work_operation;
	int ret;

	if (!dev_priv || !submit || !work_action)
		return -EINVAL;

	work_state = &work_action->work_state;
	work_operation = &work_state->work_event.work_operation;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_work_state_locked(
		dev_priv, submit, work_state);
	if (ret)
		return ret;

	if (memcmp(&work_action->op, &work_operation->op,
		   sizeof(work_action->op)) ||
	    work_action->active != work_operation->active ||
	    work_action->dependency_fields_present !=
		    work_operation->dependency_fields_present ||
	    work_action->use_focus_request !=
		    work_operation->use_focus_request ||
	    work_action->focus_resolved !=
		    work_operation->focus_resolved ||
	    work_action->issue_request != work_state->issue_request ||
	    work_action->issue_request != work_operation->issue_request ||
	    work_action->transition_request != work_state->transition_request ||
	    work_action->transition_request !=
		    work_operation->transition_request ||
	    work_action->submit_id != work_state->submit_id ||
	    work_action->submit_id != work_operation->submit_id ||
	    work_action->requested_submit_id !=
		    work_operation->requested_submit_id ||
	    work_action->resolved_submit_id != work_state->resolved_submit_id ||
	    work_action->resolved_submit_id !=
		    work_operation->resolved_submit_id ||
	    work_action->ioctl_op != work_operation->ioctl_op ||
	    work_action->op_kind != work_operation->op_kind)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_work_task_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_work_task *work_task)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_action *work_action;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_state *work_state;
	int ret;

	if (!dev_priv || !submit || !op || !work_task)
		return -EINVAL;

	memset(work_task, 0, sizeof(*work_task));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_work_action_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &work_task->work_action);
	if (ret)
		return ret;

	work_action = &work_task->work_action;
	work_state = &work_action->work_state;
	work_task->active = work_state->active;
	work_task->issue_request = work_state->issue_request;
	work_task->transition_request = work_state->transition_request;
	work_task->refresh_views = work_state->refresh_views;
	work_task->refilled_from_complete = work_state->refilled_from_complete;
	work_task->issue_advanced = work_state->issue_advanced;
	work_task->running_advanced = work_state->running_advanced;
	work_task->complete_advanced = work_state->complete_advanced;
	work_task->active_running_changed = work_state->active_running_changed;
	work_task->selected_wrapped = work_state->selected_wrapped;
	work_task->submit_id = work_state->submit_id;
	work_task->resolved_submit_id = work_state->resolved_submit_id;
	work_task->owner_state = work_state->owner_state;
	work_task->packet_count = work_state->packet_count;
	work_task->selected_read_offset = work_state->selected_read_offset;
	work_task->selected_write_offset = work_state->selected_write_offset;
	work_task->selected_ccb_offset = work_state->selected_ccb_offset;
	work_task->selected_outstanding_packets =
		work_state->selected_outstanding_packets;
	work_task->selected_seq = work_state->selected_seq;
	work_task->selected_cookie = work_state->selected_cookie;
	work_task->epoch = work_state->epoch;
	work_task->global_issue_ticket = work_state->global_issue_ticket;
	work_task->global_running_ticket = work_state->global_running_ticket;
	work_task->global_complete_ticket = work_state->global_complete_ticket;
	work_task->next_issue_count = work_state->next_issue_count;
	work_task->next_transition_count = work_state->next_transition_count;
	work_task->next_state = work_state->next_state;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_work_task_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_task *work_task)
{
	if (!dev_priv || !submit || !work_task)
		return -EINVAL;
	if (work_task->active &&
	    work_task->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_work_action_locked(
		dev_priv, submit, &work_task->work_action);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_work_task_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_task *work_task)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_action *work_action;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_state *work_state;
	int ret;

	if (!dev_priv || !submit || !work_task)
		return -EINVAL;

	work_action = &work_task->work_action;
	work_state = &work_action->work_state;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_work_action_locked(
		dev_priv, submit, work_action);
	if (ret)
		return ret;

	if (work_task->active != work_state->active ||
	    work_task->issue_request != work_state->issue_request ||
	    work_task->transition_request != work_state->transition_request ||
	    work_task->refresh_views != work_state->refresh_views ||
	    work_task->refilled_from_complete !=
		    work_state->refilled_from_complete ||
	    work_task->issue_advanced != work_state->issue_advanced ||
	    work_task->running_advanced != work_state->running_advanced ||
	    work_task->complete_advanced != work_state->complete_advanced ||
	    work_task->active_running_changed !=
		    work_state->active_running_changed ||
	    work_task->selected_wrapped != work_state->selected_wrapped ||
	    work_task->submit_id != work_state->submit_id ||
	    work_task->resolved_submit_id != work_state->resolved_submit_id ||
	    work_task->owner_state != work_state->owner_state ||
	    work_task->packet_count != work_state->packet_count ||
	    work_task->selected_read_offset !=
		    work_state->selected_read_offset ||
	    work_task->selected_write_offset !=
		    work_state->selected_write_offset ||
	    work_task->selected_ccb_offset !=
		    work_state->selected_ccb_offset ||
	    work_task->selected_outstanding_packets !=
		    work_state->selected_outstanding_packets ||
	    work_task->selected_seq != work_state->selected_seq ||
	    work_task->selected_cookie != work_state->selected_cookie ||
	    work_task->epoch != work_state->epoch ||
	    work_task->global_issue_ticket !=
		    work_state->global_issue_ticket ||
	    work_task->global_running_ticket !=
		    work_state->global_running_ticket ||
	    work_task->global_complete_ticket !=
		    work_state->global_complete_ticket ||
	    work_task->next_issue_count != work_state->next_issue_count ||
	    work_task->next_transition_count !=
		    work_state->next_transition_count)
		return -EINVAL;

	if (memcmp(&work_task->next_state, &work_state->next_state,
		   sizeof(work_task->next_state)))
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_work_run_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_work_run *work_run)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_task *work_task;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_action *work_action;
	int ret;

	if (!dev_priv || !submit || !op || !work_run)
		return -EINVAL;

	memset(work_run, 0, sizeof(*work_run));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_work_task_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &work_run->work_task);
	if (ret)
		return ret;

	work_task = &work_run->work_task;
	work_action = &work_task->work_action;
	work_run->op = work_action->op;
	work_run->active = work_task->active;
	work_run->dependency_fields_present =
		work_action->dependency_fields_present;
	work_run->use_focus_request = work_action->use_focus_request;
	work_run->focus_resolved = work_action->focus_resolved;
	work_run->issue_request = work_task->issue_request;
	work_run->transition_request = work_task->transition_request;
	work_run->refresh_views = work_task->refresh_views;
	work_run->refilled_from_complete = work_task->refilled_from_complete;
	work_run->issue_advanced = work_task->issue_advanced;
	work_run->running_advanced = work_task->running_advanced;
	work_run->complete_advanced = work_task->complete_advanced;
	work_run->active_running_changed = work_task->active_running_changed;
	work_run->selected_wrapped = work_task->selected_wrapped;
	work_run->submit_id = work_task->submit_id;
	work_run->requested_submit_id = work_action->requested_submit_id;
	work_run->resolved_submit_id = work_task->resolved_submit_id;
	work_run->ioctl_op = work_action->ioctl_op;
	work_run->op_kind = work_action->op_kind;
	work_run->owner_state = work_task->owner_state;
	work_run->packet_count = work_task->packet_count;
	work_run->selected_read_offset = work_task->selected_read_offset;
	work_run->selected_write_offset = work_task->selected_write_offset;
	work_run->selected_ccb_offset = work_task->selected_ccb_offset;
	work_run->selected_outstanding_packets =
		work_task->selected_outstanding_packets;
	work_run->selected_seq = work_task->selected_seq;
	work_run->selected_cookie = work_task->selected_cookie;
	work_run->epoch = work_task->epoch;
	work_run->global_issue_ticket = work_task->global_issue_ticket;
	work_run->global_running_ticket = work_task->global_running_ticket;
	work_run->global_complete_ticket = work_task->global_complete_ticket;
	work_run->next_issue_count = work_task->next_issue_count;
	work_run->next_transition_count = work_task->next_transition_count;
	work_run->next_state = work_task->next_state;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_work_run_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_run *work_run)
{
	if (!dev_priv || !submit || !work_run)
		return -EINVAL;
	if (work_run->active &&
	    work_run->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_work_task_locked(
		dev_priv, submit, &work_run->work_task);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_work_run_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_run *work_run)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_task *work_task;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_action *work_action;
	int ret;

	if (!dev_priv || !submit || !work_run)
		return -EINVAL;

	work_task = &work_run->work_task;
	work_action = &work_task->work_action;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_work_task_locked(
		dev_priv, submit, work_task);
	if (ret)
		return ret;

	if (memcmp(&work_run->op, &work_action->op, sizeof(work_run->op)) ||
	    work_run->active != work_task->active ||
	    work_run->dependency_fields_present !=
		    work_action->dependency_fields_present ||
	    work_run->use_focus_request != work_action->use_focus_request ||
	    work_run->focus_resolved != work_action->focus_resolved ||
	    work_run->issue_request != work_task->issue_request ||
	    work_run->transition_request != work_task->transition_request ||
	    work_run->refresh_views != work_task->refresh_views ||
	    work_run->refilled_from_complete !=
		    work_task->refilled_from_complete ||
	    work_run->issue_advanced != work_task->issue_advanced ||
	    work_run->running_advanced != work_task->running_advanced ||
	    work_run->complete_advanced != work_task->complete_advanced ||
	    work_run->active_running_changed !=
		    work_task->active_running_changed ||
	    work_run->selected_wrapped != work_task->selected_wrapped ||
	    work_run->submit_id != work_task->submit_id ||
	    work_run->requested_submit_id != work_action->requested_submit_id ||
	    work_run->resolved_submit_id != work_task->resolved_submit_id ||
	    work_run->ioctl_op != work_action->ioctl_op ||
	    work_run->op_kind != work_action->op_kind ||
	    work_run->owner_state != work_task->owner_state ||
	    work_run->packet_count != work_task->packet_count ||
	    work_run->selected_read_offset != work_task->selected_read_offset ||
	    work_run->selected_write_offset != work_task->selected_write_offset ||
	    work_run->selected_ccb_offset != work_task->selected_ccb_offset ||
	    work_run->selected_outstanding_packets !=
		    work_task->selected_outstanding_packets ||
	    work_run->selected_seq != work_task->selected_seq ||
	    work_run->selected_cookie != work_task->selected_cookie ||
	    work_run->epoch != work_task->epoch ||
	    work_run->global_issue_ticket != work_task->global_issue_ticket ||
	    work_run->global_running_ticket !=
		    work_task->global_running_ticket ||
	    work_run->global_complete_ticket !=
		    work_task->global_complete_ticket ||
	    work_run->next_issue_count != work_task->next_issue_count ||
	    work_run->next_transition_count !=
		    work_task->next_transition_count)
		return -EINVAL;

	if (memcmp(&work_run->next_state, &work_task->next_state,
		   sizeof(work_run->next_state)))
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_work_invocation_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_submit_id,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_work_invocation *invocation)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_run *work_run;
	int ret;

	if (!dev_priv || !submit || !op || !invocation)
		return -EINVAL;

	memset(invocation, 0, sizeof(*invocation));

	if (resolved_submit_id != submit->submit_id)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_sgx_ta_request_work_run_locked(
		dev_priv, submit, resolved_submit_id, op,
		dependency_fields_present, &invocation->work_run);
	if (ret)
		return ret;

	work_run = &invocation->work_run;
	invocation->op = *op;
	invocation->active = work_run->active;
	invocation->dependency_fields_present =
		work_run->dependency_fields_present;
	invocation->use_focus_request = work_run->use_focus_request;
	invocation->focus_resolved = work_run->focus_resolved;
	invocation->issue_request = work_run->issue_request;
	invocation->transition_request = work_run->transition_request;
	invocation->submit_id = work_run->submit_id;
	invocation->requested_submit_id = work_run->requested_submit_id;
	invocation->resolved_submit_id = work_run->resolved_submit_id;
	invocation->ioctl_op = work_run->ioctl_op;
	invocation->op_kind = work_run->op_kind;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_work_invocation_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_invocation *invocation)
{
	if (!dev_priv || !submit || !invocation)
		return -EINVAL;
	if (invocation->active &&
	    invocation->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_work_run_locked(
		dev_priv, submit, &invocation->work_run);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_work_invocation_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_invocation *invocation)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_run *work_run;
	int ret;

	if (!dev_priv || !submit || !invocation)
		return -EINVAL;

	work_run = &invocation->work_run;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_work_run_locked(
		dev_priv, submit, work_run);
	if (ret)
		return ret;

	if (memcmp(&invocation->op, &work_run->op, sizeof(invocation->op)) ||
	    invocation->active != work_run->active ||
	    invocation->dependency_fields_present !=
		    work_run->dependency_fields_present ||
	    invocation->use_focus_request != work_run->use_focus_request ||
	    invocation->focus_resolved != work_run->focus_resolved ||
	    invocation->issue_request != work_run->issue_request ||
	    invocation->transition_request != work_run->transition_request ||
	    invocation->submit_id != work_run->submit_id ||
	    invocation->requested_submit_id != work_run->requested_submit_id ||
	    invocation->resolved_submit_id != work_run->resolved_submit_id ||
	    invocation->ioctl_op != work_run->ioctl_op ||
	    invocation->op_kind != work_run->op_kind)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_resolve_submit_op_submit_id_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	u32 *selected_submit_id,
	bool *focus_valid)
{
	if (!dev_priv || !op || !selected_submit_id || !focus_valid)
		return -EINVAL;

	*selected_submit_id = op->submit_id;
	*focus_valid = false;

	if (!(op->flags & DRM_PSB_SGX_LAB_SUBMIT_OP_USE_FOCUS))
		return 0;

	if (!dev_priv->sgx_lab_submit_focus_valid)
		return -ENOENT;

	*selected_submit_id = dev_priv->sgx_lab_submit_focus_id;
	*focus_valid = true;
	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_work_submit_op_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_work_submit_op *submit_op)
{
	struct psb_sgx_lab_submit *submit;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_invocation *invocation;
	u32 op_kind;
	u32 selected_submit_id;
	bool focus_valid;
	int ret;

	if (!dev_priv || !op || !submit_op)
		return -EINVAL;

	memset(submit_op, 0, sizeof(*submit_op));

	ret = poulsbo_sgx_map_submit_sgx_ta_ioctl_op_kind(op->op, &op_kind);
	if (ret)
		return ret;

	ret = poulsbo_sgx_validate_submit_sgx_ta_ioctl_op_request(
		op, dependency_fields_present, op_kind);
	if (ret)
		return ret;

	ret = poulsbo_sgx_resolve_submit_op_submit_id_locked(
		dev_priv, op, &selected_submit_id, &focus_valid);
	if (ret)
		return ret;

	submit = poulsbo_sgx_find_submit_locked(dev_priv, selected_submit_id);
	if (!submit)
		return -ENOENT;

	ret = poulsbo_sgx_build_submit_sgx_ta_request_work_invocation_locked(
		dev_priv, submit, selected_submit_id, op,
		dependency_fields_present, &submit_op->work_invocation);
	if (ret)
		return ret;

	invocation = &submit_op->work_invocation;
	submit_op->op = *op;
	submit_op->active = invocation->active;
	submit_op->dependency_fields_present =
		invocation->dependency_fields_present;
	submit_op->use_focus_request = invocation->use_focus_request;
	submit_op->focus_valid = focus_valid;
	submit_op->focus_resolved = invocation->focus_resolved;
	submit_op->issue_request = invocation->issue_request;
	submit_op->transition_request = invocation->transition_request;
	submit_op->submit_id = invocation->submit_id;
	submit_op->requested_submit_id = invocation->requested_submit_id;
	submit_op->selected_submit_id = selected_submit_id;
	submit_op->resolved_submit_id = invocation->resolved_submit_id;
	submit_op->ioctl_op = invocation->ioctl_op;
	submit_op->op_kind = invocation->op_kind;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_work_submit_op_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_submit_op *submit_op)
{
	if (!dev_priv || !submit || !submit_op)
		return -EINVAL;
	if (submit_op->active &&
	    submit_op->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_work_invocation_locked(
		dev_priv, submit, &submit_op->work_invocation);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_work_submit_op_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_submit_op *submit_op)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_invocation *invocation;
	u32 op_kind;
	int ret;

	if (!dev_priv || !submit || !submit_op)
		return -EINVAL;

	ret = poulsbo_sgx_map_submit_sgx_ta_ioctl_op_kind(
		submit_op->op.op, &op_kind);
	if (ret)
		return ret;

	ret = poulsbo_sgx_validate_submit_sgx_ta_ioctl_op_request(
		&submit_op->op, submit_op->dependency_fields_present, op_kind);
	if (ret)
		return ret;

	invocation = &submit_op->work_invocation;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_work_invocation_locked(
		dev_priv, submit, invocation);
	if (ret)
		return ret;

	if (memcmp(&submit_op->op, &invocation->op, sizeof(submit_op->op)) ||
	    submit_op->active != invocation->active ||
	    submit_op->dependency_fields_present !=
		    invocation->dependency_fields_present ||
	    submit_op->use_focus_request !=
		    !!(submit_op->op.flags &
		       DRM_PSB_SGX_LAB_SUBMIT_OP_USE_FOCUS) ||
	    submit_op->focus_valid != submit_op->use_focus_request ||
	    submit_op->focus_resolved != invocation->focus_resolved ||
	    submit_op->issue_request != invocation->issue_request ||
	    submit_op->transition_request != invocation->transition_request ||
	    submit_op->submit_id != invocation->submit_id ||
	    submit_op->requested_submit_id != invocation->requested_submit_id ||
	    submit_op->selected_submit_id != invocation->resolved_submit_id ||
	    submit_op->resolved_submit_id != submit->submit_id ||
	    submit_op->resolved_submit_id != invocation->resolved_submit_id ||
	    submit_op->ioctl_op != invocation->ioctl_op ||
	    submit_op->op_kind != op_kind ||
	    submit_op->op_kind != invocation->op_kind ||
	    submit_op->requested_submit_id != submit_op->op.submit_id ||
	    !submit_op->active)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_work_outcome_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_work_outcome *outcome)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_submit_op *submit_op;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_run *work_run;
	int ret;

	if (!dev_priv || !op || !outcome)
		return -EINVAL;

	memset(outcome, 0, sizeof(*outcome));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_work_submit_op_locked(
		dev_priv, op, dependency_fields_present, &outcome->submit_op);
	if (ret)
		return ret;

	submit_op = &outcome->submit_op;
	work_run = &submit_op->work_invocation.work_run;
	outcome->op = submit_op->op;
	outcome->active = submit_op->active;
	outcome->dependency_fields_present =
		submit_op->dependency_fields_present;
	outcome->use_focus_request = submit_op->use_focus_request;
	outcome->focus_valid = submit_op->focus_valid;
	outcome->focus_resolved = submit_op->focus_resolved;
	outcome->issue_request = submit_op->issue_request;
	outcome->transition_request = submit_op->transition_request;
	outcome->refresh_views = work_run->refresh_views;
	outcome->refilled_from_complete = work_run->refilled_from_complete;
	outcome->issue_advanced = work_run->issue_advanced;
	outcome->running_advanced = work_run->running_advanced;
	outcome->complete_advanced = work_run->complete_advanced;
	outcome->active_running_changed = work_run->active_running_changed;
	outcome->selected_wrapped = work_run->selected_wrapped;
	outcome->submit_id = submit_op->submit_id;
	outcome->requested_submit_id = submit_op->requested_submit_id;
	outcome->selected_submit_id = submit_op->selected_submit_id;
	outcome->resolved_submit_id = submit_op->resolved_submit_id;
	outcome->ioctl_op = submit_op->ioctl_op;
	outcome->op_kind = submit_op->op_kind;
	outcome->owner_state = work_run->owner_state;
	outcome->packet_count = work_run->packet_count;
	outcome->selected_read_offset = work_run->selected_read_offset;
	outcome->selected_write_offset = work_run->selected_write_offset;
	outcome->selected_ccb_offset = work_run->selected_ccb_offset;
	outcome->selected_outstanding_packets =
		work_run->selected_outstanding_packets;
	outcome->selected_seq = work_run->selected_seq;
	outcome->selected_cookie = work_run->selected_cookie;
	outcome->epoch = work_run->epoch;
	outcome->global_issue_ticket = work_run->global_issue_ticket;
	outcome->global_running_ticket = work_run->global_running_ticket;
	outcome->global_complete_ticket = work_run->global_complete_ticket;
	outcome->next_issue_count = work_run->next_issue_count;
	outcome->next_transition_count = work_run->next_transition_count;
	outcome->next_state = work_run->next_state;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_work_outcome_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_outcome *outcome)
{
	if (!dev_priv || !submit || !outcome)
		return -EINVAL;
	if (outcome->active &&
	    outcome->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_work_submit_op_locked(
		dev_priv, submit, &outcome->submit_op);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_work_outcome_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_outcome *outcome)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_submit_op *submit_op;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_run *work_run;
	int ret;

	if (!dev_priv || !submit || !outcome)
		return -EINVAL;

	submit_op = &outcome->submit_op;
	work_run = &submit_op->work_invocation.work_run;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_work_submit_op_locked(
		dev_priv, submit, submit_op);
	if (ret)
		return ret;

	if (memcmp(&outcome->op, &submit_op->op, sizeof(outcome->op)) ||
	    outcome->active != submit_op->active ||
	    outcome->dependency_fields_present !=
		    submit_op->dependency_fields_present ||
	    outcome->use_focus_request != submit_op->use_focus_request ||
	    outcome->focus_valid != submit_op->focus_valid ||
	    outcome->focus_resolved != submit_op->focus_resolved ||
	    outcome->issue_request != submit_op->issue_request ||
	    outcome->transition_request != submit_op->transition_request ||
	    outcome->refresh_views != work_run->refresh_views ||
	    outcome->refilled_from_complete !=
		    work_run->refilled_from_complete ||
	    outcome->issue_advanced != work_run->issue_advanced ||
	    outcome->running_advanced != work_run->running_advanced ||
	    outcome->complete_advanced != work_run->complete_advanced ||
	    outcome->active_running_changed !=
		    work_run->active_running_changed ||
	    outcome->selected_wrapped != work_run->selected_wrapped ||
	    outcome->submit_id != submit_op->submit_id ||
	    outcome->requested_submit_id != submit_op->requested_submit_id ||
	    outcome->selected_submit_id != submit_op->selected_submit_id ||
	    outcome->resolved_submit_id != submit_op->resolved_submit_id ||
	    outcome->ioctl_op != submit_op->ioctl_op ||
	    outcome->op_kind != submit_op->op_kind ||
	    outcome->owner_state != work_run->owner_state ||
	    outcome->packet_count != work_run->packet_count ||
	    outcome->selected_read_offset != work_run->selected_read_offset ||
	    outcome->selected_write_offset != work_run->selected_write_offset ||
	    outcome->selected_ccb_offset != work_run->selected_ccb_offset ||
	    outcome->selected_outstanding_packets !=
		    work_run->selected_outstanding_packets ||
	    outcome->selected_seq != work_run->selected_seq ||
	    outcome->selected_cookie != work_run->selected_cookie ||
	    outcome->epoch != work_run->epoch ||
	    outcome->global_issue_ticket != work_run->global_issue_ticket ||
	    outcome->global_running_ticket !=
		    work_run->global_running_ticket ||
	    outcome->global_complete_ticket !=
		    work_run->global_complete_ticket ||
	    outcome->next_issue_count != work_run->next_issue_count ||
	    outcome->next_transition_count !=
		    work_run->next_transition_count)
		return -EINVAL;

	if (memcmp(&outcome->next_state, &work_run->next_state,
		   sizeof(outcome->next_state)))
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_request_work_response_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_request_work_response *response)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_outcome *outcome;
	int ret;

	if (!dev_priv || !op || !response)
		return -EINVAL;

	memset(response, 0, sizeof(*response));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_work_outcome_locked(
		dev_priv, op, dependency_fields_present, &response->outcome);
	if (ret)
		return ret;

	outcome = &response->outcome;
	response->op = outcome->op;
	response->active = outcome->active;
	response->dependency_fields_present =
		outcome->dependency_fields_present;
	response->use_focus_request = outcome->use_focus_request;
	response->focus_valid = outcome->focus_valid;
	response->focus_resolved = outcome->focus_resolved;
	response->issue_request = outcome->issue_request;
	response->transition_request = outcome->transition_request;
	response->submit_id = outcome->submit_id;
	response->requested_submit_id = outcome->requested_submit_id;
	response->selected_submit_id = outcome->selected_submit_id;
	response->resolved_submit_id = outcome->resolved_submit_id;
	response->ioctl_op = outcome->ioctl_op;
	response->op_kind = outcome->op_kind;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_request_work_response_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_response *response)
{
	if (!dev_priv || !submit || !response)
		return -EINVAL;
	if (response->active &&
	    response->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_work_outcome_locked(
		dev_priv, submit, &response->outcome);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_work_response_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request_work_response *response)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_outcome *outcome;
	int ret;

	if (!dev_priv || !submit || !response)
		return -EINVAL;

	outcome = &response->outcome;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_work_outcome_locked(
		dev_priv, submit, outcome);
	if (ret)
		return ret;

	if (memcmp(&response->op, &outcome->op, sizeof(response->op)) ||
	    response->active != outcome->active ||
	    response->dependency_fields_present !=
		    outcome->dependency_fields_present ||
	    response->use_focus_request != outcome->use_focus_request ||
	    response->focus_valid != outcome->focus_valid ||
	    response->focus_resolved != outcome->focus_resolved ||
	    response->issue_request != outcome->issue_request ||
	    response->transition_request != outcome->transition_request ||
	    response->submit_id != outcome->submit_id ||
	    response->requested_submit_id != outcome->requested_submit_id ||
	    response->selected_submit_id != outcome->selected_submit_id ||
	    response->resolved_submit_id != outcome->resolved_submit_id ||
	    response->ioctl_op != outcome->ioctl_op ||
	    response->op_kind != outcome->op_kind)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_reply_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_reply *reply)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_response *response;
	int ret;

	if (!dev_priv || !op || !reply)
		return -EINVAL;

	memset(reply, 0, sizeof(*reply));

	ret = poulsbo_sgx_build_submit_sgx_ta_request_work_response_locked(
		dev_priv, op, dependency_fields_present, &reply->work_response);
	if (ret)
		return ret;

	response = &reply->work_response;
	reply->response_op = response->op;
	reply->response_op.resolved_submit_id = response->resolved_submit_id;
	reply->active = response->active;
	reply->dependency_fields_present = response->dependency_fields_present;
	reply->use_focus_request = response->use_focus_request;
	reply->focus_valid = response->focus_valid;
	reply->focus_resolved = response->focus_resolved;
	reply->issue_request = response->issue_request;
	reply->transition_request = response->transition_request;
	reply->submit_id = response->submit_id;
	reply->requested_submit_id = response->requested_submit_id;
	reply->selected_submit_id = response->selected_submit_id;
	reply->resolved_submit_id = response->resolved_submit_id;
	reply->ioctl_op = response->ioctl_op;
	reply->op_kind = response->op_kind;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_reply_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_reply *reply)
{
	if (!dev_priv || !submit || !reply)
		return -EINVAL;
	if (reply->active && reply->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_request_work_response_locked(
		dev_priv, submit, &reply->work_response);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_reply_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_reply *reply)
{
	const struct psb_sgx_lab_submit_sgx_ta_request_work_response *response;
	struct drm_psb_sgx_lab_submit_op expected_response_op;
	int ret;

	if (!dev_priv || !submit || !reply)
		return -EINVAL;

	response = &reply->work_response;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_work_response_locked(
		dev_priv, submit, response);
	if (ret)
		return ret;

	expected_response_op = response->op;
	expected_response_op.resolved_submit_id = response->resolved_submit_id;

	if (memcmp(&reply->response_op, &expected_response_op,
		   sizeof(reply->response_op)) ||
	    reply->active != response->active ||
	    reply->dependency_fields_present !=
		    response->dependency_fields_present ||
	    reply->use_focus_request != response->use_focus_request ||
	    reply->focus_valid != response->focus_valid ||
	    reply->focus_resolved != response->focus_resolved ||
	    reply->issue_request != response->issue_request ||
	    reply->transition_request != response->transition_request ||
	    reply->submit_id != response->submit_id ||
	    reply->requested_submit_id != response->requested_submit_id ||
	    reply->selected_submit_id != response->selected_submit_id ||
	    reply->resolved_submit_id != response->resolved_submit_id ||
	    reply->ioctl_op != response->ioctl_op ||
	    reply->op_kind != response->op_kind)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_control_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_control *control)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_reply *reply;
	int ret;

	if (!dev_priv || !op || !control)
		return -EINVAL;

	memset(control, 0, sizeof(*control));

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_reply_locked(
		dev_priv, op, dependency_fields_present,
		&control->submit_op_reply);
	if (ret)
		return ret;

	reply = &control->submit_op_reply;
	control->ioctl_op = reply->response_op;
	control->active = reply->active;
	control->dependency_fields_present = reply->dependency_fields_present;
	control->use_focus_request = reply->use_focus_request;
	control->focus_valid = reply->focus_valid;
	control->focus_resolved = reply->focus_resolved;
	control->issue_request = reply->issue_request;
	control->transition_request = reply->transition_request;
	control->submit_id = reply->submit_id;
	control->requested_submit_id = reply->requested_submit_id;
	control->selected_submit_id = reply->selected_submit_id;
	control->resolved_submit_id = reply->resolved_submit_id;
	control->op_kind = reply->op_kind;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_control_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_control *control)
{
	if (!dev_priv || !submit || !control)
		return -EINVAL;
	if (control->active && control->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_reply_locked(
		dev_priv, submit, &control->submit_op_reply);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_control_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_control *control)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_reply *reply;
	int ret;

	if (!dev_priv || !submit || !control)
		return -EINVAL;

	reply = &control->submit_op_reply;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_reply_locked(
		dev_priv, submit, reply);
	if (ret)
		return ret;

	if (memcmp(&control->ioctl_op, &reply->response_op,
		   sizeof(control->ioctl_op)) ||
	    control->active != reply->active ||
	    control->dependency_fields_present !=
		    reply->dependency_fields_present ||
	    control->use_focus_request != reply->use_focus_request ||
	    control->focus_valid != reply->focus_valid ||
	    control->focus_resolved != reply->focus_resolved ||
	    control->issue_request != reply->issue_request ||
	    control->transition_request != reply->transition_request ||
	    control->submit_id != reply->submit_id ||
	    control->requested_submit_id != reply->requested_submit_id ||
	    control->selected_submit_id != reply->selected_submit_id ||
	    control->resolved_submit_id != reply->resolved_submit_id ||
	    control->op_kind != reply->op_kind)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_ioctl_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_ioctl *ioctl)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_control *control;
	int ret;

	if (!dev_priv || !op || !ioctl)
		return -EINVAL;

	memset(ioctl, 0, sizeof(*ioctl));

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_control_locked(
		dev_priv, op, dependency_fields_present,
		&ioctl->submit_op_control);
	if (ret)
		return ret;

	control = &ioctl->submit_op_control;
	ioctl->request_op = *op;
	ioctl->response_op = control->ioctl_op;
	ioctl->active = control->active;
	ioctl->dependency_fields_present = control->dependency_fields_present;
	ioctl->use_focus_request = control->use_focus_request;
	ioctl->focus_valid = control->focus_valid;
	ioctl->focus_resolved = control->focus_resolved;
	ioctl->issue_request = control->issue_request;
	ioctl->transition_request = control->transition_request;
	ioctl->submit_id = control->submit_id;
	ioctl->requested_submit_id = control->requested_submit_id;
	ioctl->selected_submit_id = control->selected_submit_id;
	ioctl->resolved_submit_id = control->resolved_submit_id;
	ioctl->ioctl_op = control->ioctl_op.op;
	ioctl->op_kind = control->op_kind;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_ioctl_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_ioctl *ioctl)
{
	if (!dev_priv || !submit || !ioctl)
		return -EINVAL;
	if (ioctl->active && ioctl->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_control_locked(
		dev_priv, submit, &ioctl->submit_op_control);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_ioctl_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_ioctl *ioctl)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_control *control;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_reply *reply;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_response *response;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_outcome *outcome;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_submit_op *submit_op;
	int ret;

	if (!dev_priv || !submit || !ioctl)
		return -EINVAL;

	control = &ioctl->submit_op_control;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_control_locked(
		dev_priv, submit, control);
	if (ret)
		return ret;

	reply = &control->submit_op_reply;
	response = &reply->work_response;
	outcome = &response->outcome;
	submit_op = &outcome->submit_op;

	if (memcmp(&ioctl->request_op, &submit_op->op,
		   sizeof(ioctl->request_op)) ||
	    memcmp(&ioctl->response_op, &control->ioctl_op,
		   sizeof(ioctl->response_op)) ||
	    ioctl->active != control->active ||
	    ioctl->dependency_fields_present !=
		    control->dependency_fields_present ||
	    ioctl->use_focus_request != control->use_focus_request ||
	    ioctl->focus_valid != control->focus_valid ||
	    ioctl->focus_resolved != control->focus_resolved ||
	    ioctl->issue_request != control->issue_request ||
	    ioctl->transition_request != control->transition_request ||
	    ioctl->submit_id != control->submit_id ||
	    ioctl->requested_submit_id != control->requested_submit_id ||
	    ioctl->selected_submit_id != control->selected_submit_id ||
	    ioctl->resolved_submit_id != control->resolved_submit_id ||
	    ioctl->ioctl_op != control->ioctl_op.op ||
	    ioctl->ioctl_op != submit_op->ioctl_op ||
	    ioctl->op_kind != control->op_kind ||
	    ioctl->request_op.submit_id != control->requested_submit_id ||
	    ioctl->response_op.resolved_submit_id !=
		    control->resolved_submit_id ||
	    ioctl->request_op.op != ioctl->response_op.op ||
	    ioctl->request_op.op != ioctl->ioctl_op ||
	    !ioctl->active)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_entry_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_entry *entry)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_ioctl *ioctl;
	int ret;

	if (!dev_priv || !op || !entry)
		return -EINVAL;
	if (op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE &&
	    op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION)
		return -EINVAL;

	memset(entry, 0, sizeof(*entry));

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_ioctl_locked(
		dev_priv, op, dependency_fields_present, &entry->submit_op_ioctl);
	if (ret)
		return ret;

	ioctl = &entry->submit_op_ioctl;
	entry->request_op = ioctl->request_op;
	entry->response_op = ioctl->response_op;
	entry->active = ioctl->active;
	entry->dependency_fields_present = ioctl->dependency_fields_present;
	entry->use_focus_request = ioctl->use_focus_request;
	entry->focus_valid = ioctl->focus_valid;
	entry->focus_resolved = ioctl->focus_resolved;
	entry->issue_request = ioctl->issue_request;
	entry->transition_request = ioctl->transition_request;
	entry->issue_entry = op->op == DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE;
	entry->transition_entry =
		op->op == DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION;
	entry->submit_id = ioctl->submit_id;
	entry->requested_submit_id = ioctl->requested_submit_id;
	entry->selected_submit_id = ioctl->selected_submit_id;
	entry->resolved_submit_id = ioctl->resolved_submit_id;
	entry->ioctl_op = ioctl->ioctl_op;
	entry->entry_op = op->op;
	entry->op_kind = ioctl->op_kind;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_entry_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_entry *entry)
{
	if (!dev_priv || !submit || !entry)
		return -EINVAL;
	if (entry->active && entry->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_ioctl_locked(
		dev_priv, submit, &entry->submit_op_ioctl);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_entry_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_entry *entry)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_ioctl *ioctl;
	int ret;

	if (!dev_priv || !submit || !entry)
		return -EINVAL;

	ioctl = &entry->submit_op_ioctl;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_ioctl_locked(
		dev_priv, submit, ioctl);
	if (ret)
		return ret;

	if (memcmp(&entry->request_op, &ioctl->request_op,
		   sizeof(entry->request_op)) ||
	    memcmp(&entry->response_op, &ioctl->response_op,
		   sizeof(entry->response_op)) ||
	    entry->active != ioctl->active ||
	    entry->dependency_fields_present !=
		    ioctl->dependency_fields_present ||
	    entry->use_focus_request != ioctl->use_focus_request ||
	    entry->focus_valid != ioctl->focus_valid ||
	    entry->focus_resolved != ioctl->focus_resolved ||
	    entry->issue_request != ioctl->issue_request ||
	    entry->transition_request != ioctl->transition_request ||
	    entry->submit_id != ioctl->submit_id ||
	    entry->requested_submit_id != ioctl->requested_submit_id ||
	    entry->selected_submit_id != ioctl->selected_submit_id ||
	    entry->resolved_submit_id != ioctl->resolved_submit_id ||
	    entry->ioctl_op != ioctl->ioctl_op ||
	    entry->entry_op != ioctl->request_op.op ||
	    entry->entry_op != ioctl->response_op.op ||
	    entry->entry_op != ioctl->ioctl_op ||
	    entry->op_kind != ioctl->op_kind ||
	    entry->issue_entry !=
		    (entry->entry_op == DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE) ||
	    entry->transition_entry !=
		    (entry->entry_op ==
		     DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION) ||
	    entry->issue_entry == entry->transition_entry ||
	    entry->issue_entry != entry->issue_request ||
	    entry->transition_entry != entry->transition_request ||
	    !entry->active)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_call_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_call *call)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_entry *entry;
	int ret;

	if (!dev_priv || !op || !call)
		return -EINVAL;
	if (op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE &&
	    op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION)
		return -EINVAL;

	memset(call, 0, sizeof(*call));

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_entry_locked(
		dev_priv, op, dependency_fields_present, &call->submit_op_entry);
	if (ret)
		return ret;

	entry = &call->submit_op_entry;
	call->request_op = entry->request_op;
	call->response_op = entry->response_op;
	call->active = entry->active;
	call->dependency_fields_present = entry->dependency_fields_present;
	call->use_focus_request = entry->use_focus_request;
	call->focus_valid = entry->focus_valid;
	call->focus_resolved = entry->focus_resolved;
	call->issue_request = entry->issue_request;
	call->transition_request = entry->transition_request;
	call->issue_entry = entry->issue_entry;
	call->transition_entry = entry->transition_entry;
	call->issue_call = op->op == DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE;
	call->transition_call = op->op == DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION;
	call->submit_id = entry->submit_id;
	call->requested_submit_id = entry->requested_submit_id;
	call->selected_submit_id = entry->selected_submit_id;
	call->resolved_submit_id = entry->resolved_submit_id;
	call->ioctl_op = entry->ioctl_op;
	call->entry_op = entry->entry_op;
	call->call_op = op->op;
	call->op_kind = entry->op_kind;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_call_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_call *call)
{
	if (!dev_priv || !submit || !call)
		return -EINVAL;
	if (call->active && call->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_entry_locked(
		dev_priv, submit, &call->submit_op_entry);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_call_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_call *call)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_entry *entry;
	int ret;

	if (!dev_priv || !submit || !call)
		return -EINVAL;

	entry = &call->submit_op_entry;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_entry_locked(
		dev_priv, submit, entry);
	if (ret)
		return ret;

	if (memcmp(&call->request_op, &entry->request_op,
		   sizeof(call->request_op)) ||
	    memcmp(&call->response_op, &entry->response_op,
		   sizeof(call->response_op)) ||
	    call->active != entry->active ||
	    call->dependency_fields_present !=
		    entry->dependency_fields_present ||
	    call->use_focus_request != entry->use_focus_request ||
	    call->focus_valid != entry->focus_valid ||
	    call->focus_resolved != entry->focus_resolved ||
	    call->issue_request != entry->issue_request ||
	    call->transition_request != entry->transition_request ||
	    call->issue_entry != entry->issue_entry ||
	    call->transition_entry != entry->transition_entry ||
	    call->submit_id != entry->submit_id ||
	    call->requested_submit_id != entry->requested_submit_id ||
	    call->selected_submit_id != entry->selected_submit_id ||
	    call->resolved_submit_id != entry->resolved_submit_id ||
	    call->ioctl_op != entry->ioctl_op ||
	    call->entry_op != entry->entry_op ||
	    call->call_op != entry->request_op.op ||
	    call->call_op != entry->response_op.op ||
	    call->call_op != entry->entry_op ||
	    call->op_kind != entry->op_kind ||
	    call->issue_call !=
		    (call->call_op == DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE) ||
	    call->transition_call !=
		    (call->call_op == DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION) ||
	    call->issue_call == call->transition_call ||
	    call->issue_call != call->issue_request ||
	    call->transition_call != call->transition_request ||
	    !call->active)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_entrypoint_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_entrypoint *entrypoint)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_call *call;
	int ret;

	if (!dev_priv || !op || !entrypoint)
		return -EINVAL;
	if (op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE &&
	    op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION)
		return -EINVAL;

	memset(entrypoint, 0, sizeof(*entrypoint));

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_call_locked(
		dev_priv, op, dependency_fields_present,
		&entrypoint->submit_op_call);
	if (ret)
		return ret;

	call = &entrypoint->submit_op_call;
	entrypoint->request_op = call->request_op;
	entrypoint->response_op = call->response_op;
	entrypoint->active = call->active;
	entrypoint->dependency_fields_present =
		call->dependency_fields_present;
	entrypoint->use_focus_request = call->use_focus_request;
	entrypoint->focus_valid = call->focus_valid;
	entrypoint->focus_resolved = call->focus_resolved;
	entrypoint->issue_request = call->issue_request;
	entrypoint->transition_request = call->transition_request;
	entrypoint->issue_entry = call->issue_entry;
	entrypoint->transition_entry = call->transition_entry;
	entrypoint->issue_call = call->issue_call;
	entrypoint->transition_call = call->transition_call;
	entrypoint->issue_entrypoint =
		op->op == DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE;
	entrypoint->transition_entrypoint =
		op->op == DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION;
	entrypoint->submit_id = call->submit_id;
	entrypoint->requested_submit_id = call->requested_submit_id;
	entrypoint->selected_submit_id = call->selected_submit_id;
	entrypoint->resolved_submit_id = call->resolved_submit_id;
	entrypoint->ioctl_op = call->ioctl_op;
	entrypoint->entry_op = call->entry_op;
	entrypoint->call_op = call->call_op;
	entrypoint->entrypoint_op = op->op;
	entrypoint->op_kind = call->op_kind;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_entrypoint_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_entrypoint *entrypoint)
{
	if (!dev_priv || !submit || !entrypoint)
		return -EINVAL;
	if (entrypoint->active &&
	    entrypoint->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_call_locked(
		dev_priv, submit, &entrypoint->submit_op_call);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_entrypoint_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_entrypoint *entrypoint)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_call *call;
	int ret;

	if (!dev_priv || !submit || !entrypoint)
		return -EINVAL;

	call = &entrypoint->submit_op_call;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_call_locked(
		dev_priv, submit, call);
	if (ret)
		return ret;

	if (memcmp(&entrypoint->request_op, &call->request_op,
		   sizeof(entrypoint->request_op)) ||
	    memcmp(&entrypoint->response_op, &call->response_op,
		   sizeof(entrypoint->response_op)) ||
	    entrypoint->active != call->active ||
	    entrypoint->dependency_fields_present !=
		    call->dependency_fields_present ||
	    entrypoint->use_focus_request != call->use_focus_request ||
	    entrypoint->focus_valid != call->focus_valid ||
	    entrypoint->focus_resolved != call->focus_resolved ||
	    entrypoint->issue_request != call->issue_request ||
	    entrypoint->transition_request != call->transition_request ||
	    entrypoint->issue_entry != call->issue_entry ||
	    entrypoint->transition_entry != call->transition_entry ||
	    entrypoint->issue_call != call->issue_call ||
	    entrypoint->transition_call != call->transition_call ||
	    entrypoint->submit_id != call->submit_id ||
	    entrypoint->requested_submit_id != call->requested_submit_id ||
	    entrypoint->selected_submit_id != call->selected_submit_id ||
	    entrypoint->resolved_submit_id != call->resolved_submit_id ||
	    entrypoint->ioctl_op != call->ioctl_op ||
	    entrypoint->entry_op != call->entry_op ||
	    entrypoint->call_op != call->call_op ||
	    entrypoint->entrypoint_op != call->request_op.op ||
	    entrypoint->entrypoint_op != call->response_op.op ||
	    entrypoint->entrypoint_op != call->call_op ||
	    entrypoint->op_kind != call->op_kind ||
	    entrypoint->issue_entrypoint !=
		    (entrypoint->entrypoint_op ==
		     DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE) ||
	    entrypoint->transition_entrypoint !=
		    (entrypoint->entrypoint_op ==
		     DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION) ||
	    entrypoint->issue_entrypoint ==
		    entrypoint->transition_entrypoint ||
	    entrypoint->issue_entrypoint != entrypoint->issue_request ||
	    entrypoint->transition_entrypoint !=
		    entrypoint->transition_request ||
	    !entrypoint->active)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_route_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_route *route)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_entrypoint *entrypoint;
	int ret;

	if (!dev_priv || !op || !route)
		return -EINVAL;
	if (op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE &&
	    op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION)
		return -EINVAL;

	memset(route, 0, sizeof(*route));

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_entrypoint_locked(
		dev_priv, op, dependency_fields_present,
		&route->submit_op_entrypoint);
	if (ret)
		return ret;

	entrypoint = &route->submit_op_entrypoint;
	route->request_op = entrypoint->request_op;
	route->response_op = entrypoint->response_op;
	route->active = entrypoint->active;
	route->dependency_fields_present =
		entrypoint->dependency_fields_present;
	route->use_focus_request = entrypoint->use_focus_request;
	route->focus_valid = entrypoint->focus_valid;
	route->focus_resolved = entrypoint->focus_resolved;
	route->issue_request = entrypoint->issue_request;
	route->transition_request = entrypoint->transition_request;
	route->issue_entry = entrypoint->issue_entry;
	route->transition_entry = entrypoint->transition_entry;
	route->issue_call = entrypoint->issue_call;
	route->transition_call = entrypoint->transition_call;
	route->issue_entrypoint = entrypoint->issue_entrypoint;
	route->transition_entrypoint = entrypoint->transition_entrypoint;
	route->issue_route = op->op == DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE;
	route->transition_route = op->op == DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION;
	route->submit_id = entrypoint->submit_id;
	route->requested_submit_id = entrypoint->requested_submit_id;
	route->selected_submit_id = entrypoint->selected_submit_id;
	route->resolved_submit_id = entrypoint->resolved_submit_id;
	route->ioctl_op = entrypoint->ioctl_op;
	route->entry_op = entrypoint->entry_op;
	route->call_op = entrypoint->call_op;
	route->entrypoint_op = entrypoint->entrypoint_op;
	route->route_op = op->op;
	route->op_kind = entrypoint->op_kind;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_route_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_route *route)
{
	if (!dev_priv || !submit || !route)
		return -EINVAL;
	if (route->active && route->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_entrypoint_locked(
		dev_priv, submit, &route->submit_op_entrypoint);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_route_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_route *route)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_entrypoint *entrypoint;
	int ret;

	if (!dev_priv || !submit || !route)
		return -EINVAL;

	entrypoint = &route->submit_op_entrypoint;
	ret =
		poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_entrypoint_locked(
			dev_priv, submit, entrypoint);
	if (ret)
		return ret;

	if (memcmp(&route->request_op, &entrypoint->request_op,
		   sizeof(route->request_op)) ||
	    memcmp(&route->response_op, &entrypoint->response_op,
		   sizeof(route->response_op)) ||
	    route->active != entrypoint->active ||
	    route->dependency_fields_present !=
		    entrypoint->dependency_fields_present ||
	    route->use_focus_request != entrypoint->use_focus_request ||
	    route->focus_valid != entrypoint->focus_valid ||
	    route->focus_resolved != entrypoint->focus_resolved ||
	    route->issue_request != entrypoint->issue_request ||
	    route->transition_request != entrypoint->transition_request ||
	    route->issue_entry != entrypoint->issue_entry ||
	    route->transition_entry != entrypoint->transition_entry ||
	    route->issue_call != entrypoint->issue_call ||
	    route->transition_call != entrypoint->transition_call ||
	    route->issue_entrypoint != entrypoint->issue_entrypoint ||
	    route->transition_entrypoint !=
		    entrypoint->transition_entrypoint ||
	    route->submit_id != entrypoint->submit_id ||
	    route->requested_submit_id != entrypoint->requested_submit_id ||
	    route->selected_submit_id != entrypoint->selected_submit_id ||
	    route->resolved_submit_id != entrypoint->resolved_submit_id ||
	    route->ioctl_op != entrypoint->ioctl_op ||
	    route->entry_op != entrypoint->entry_op ||
	    route->call_op != entrypoint->call_op ||
	    route->entrypoint_op != entrypoint->entrypoint_op ||
	    route->route_op != entrypoint->request_op.op ||
	    route->route_op != entrypoint->response_op.op ||
	    route->route_op != entrypoint->entrypoint_op ||
	    route->op_kind != entrypoint->op_kind ||
	    route->issue_route !=
		    (route->route_op == DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE) ||
	    route->transition_route !=
		    (route->route_op == DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION) ||
	    route->issue_route == route->transition_route ||
	    route->issue_route != route->issue_request ||
	    route->transition_route != route->transition_request ||
	    !route->active)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_result_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_result *result)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_route *route;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_entrypoint *entrypoint;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_call *call;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_entry *entry;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_ioctl *ioctl;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_control *control;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_reply *reply;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_response *response;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_outcome *outcome;
	int ret;

	if (!dev_priv || !op || !result)
		return -EINVAL;
	if (op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE &&
	    op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION)
		return -EINVAL;

	memset(result, 0, sizeof(*result));

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_route_locked(
		dev_priv, op, dependency_fields_present, &result->submit_op_route);
	if (ret)
		return ret;

	route = &result->submit_op_route;
	entrypoint = &route->submit_op_entrypoint;
	call = &entrypoint->submit_op_call;
	entry = &call->submit_op_entry;
	ioctl = &entry->submit_op_ioctl;
	control = &ioctl->submit_op_control;
	reply = &control->submit_op_reply;
	response = &reply->work_response;
	outcome = &response->outcome;

	result->request_op = route->request_op;
	result->response_op = route->response_op;
	result->outcome = *outcome;
	result->active = route->active;
	result->dependency_fields_present = route->dependency_fields_present;
	result->use_focus_request = route->use_focus_request;
	result->focus_valid = route->focus_valid;
	result->focus_resolved = route->focus_resolved;
	result->issue_request = route->issue_request;
	result->transition_request = route->transition_request;
	result->issue_entry = route->issue_entry;
	result->transition_entry = route->transition_entry;
	result->issue_call = route->issue_call;
	result->transition_call = route->transition_call;
	result->issue_entrypoint = route->issue_entrypoint;
	result->transition_entrypoint = route->transition_entrypoint;
	result->issue_route = route->issue_route;
	result->transition_route = route->transition_route;
	result->issue_result = op->op == DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE;
	result->transition_result =
		op->op == DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION;
	result->refresh_views = outcome->refresh_views;
	result->refilled_from_complete = outcome->refilled_from_complete;
	result->issue_advanced = outcome->issue_advanced;
	result->running_advanced = outcome->running_advanced;
	result->complete_advanced = outcome->complete_advanced;
	result->active_running_changed = outcome->active_running_changed;
	result->selected_wrapped = outcome->selected_wrapped;
	result->submit_id = route->submit_id;
	result->requested_submit_id = route->requested_submit_id;
	result->selected_submit_id = route->selected_submit_id;
	result->resolved_submit_id = route->resolved_submit_id;
	result->ioctl_op = route->ioctl_op;
	result->entry_op = route->entry_op;
	result->call_op = route->call_op;
	result->entrypoint_op = route->entrypoint_op;
	result->route_op = route->route_op;
	result->result_op = op->op;
	result->op_kind = route->op_kind;
	result->owner_state = outcome->owner_state;
	result->packet_count = outcome->packet_count;
	result->selected_read_offset = outcome->selected_read_offset;
	result->selected_write_offset = outcome->selected_write_offset;
	result->selected_ccb_offset = outcome->selected_ccb_offset;
	result->selected_outstanding_packets =
		outcome->selected_outstanding_packets;
	result->selected_seq = outcome->selected_seq;
	result->selected_cookie = outcome->selected_cookie;
	result->epoch = outcome->epoch;
	result->global_issue_ticket = outcome->global_issue_ticket;
	result->global_running_ticket = outcome->global_running_ticket;
	result->global_complete_ticket = outcome->global_complete_ticket;
	result->next_issue_count = outcome->next_issue_count;
	result->next_transition_count = outcome->next_transition_count;
	result->next_state = outcome->next_state;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_result_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_result *result)
{
	if (!dev_priv || !submit || !result)
		return -EINVAL;
	if (result->active && result->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_route_locked(
		dev_priv, submit, &result->submit_op_route);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_result_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_result *result)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_route *route;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_entrypoint *entrypoint;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_call *call;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_entry *entry;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_ioctl *ioctl;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_control *control;
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_reply *reply;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_response *response;
	const struct psb_sgx_lab_submit_sgx_ta_request_work_outcome *outcome;
	int ret;

	if (!dev_priv || !submit || !result)
		return -EINVAL;

	route = &result->submit_op_route;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_route_locked(
		dev_priv, submit, route);
	if (ret)
		return ret;

	entrypoint = &route->submit_op_entrypoint;
	call = &entrypoint->submit_op_call;
	entry = &call->submit_op_entry;
	ioctl = &entry->submit_op_ioctl;
	control = &ioctl->submit_op_control;
	reply = &control->submit_op_reply;
	response = &reply->work_response;
	outcome = &response->outcome;

	if (memcmp(&result->request_op, &route->request_op,
		   sizeof(result->request_op)) ||
	    memcmp(&result->response_op, &route->response_op,
		   sizeof(result->response_op)) ||
	    memcmp(&result->outcome, outcome, sizeof(result->outcome)) ||
	    result->active != route->active ||
	    result->dependency_fields_present !=
		    route->dependency_fields_present ||
	    result->use_focus_request != route->use_focus_request ||
	    result->focus_valid != route->focus_valid ||
	    result->focus_resolved != route->focus_resolved ||
	    result->issue_request != route->issue_request ||
	    result->transition_request != route->transition_request ||
	    result->issue_entry != route->issue_entry ||
	    result->transition_entry != route->transition_entry ||
	    result->issue_call != route->issue_call ||
	    result->transition_call != route->transition_call ||
	    result->issue_entrypoint != route->issue_entrypoint ||
	    result->transition_entrypoint != route->transition_entrypoint ||
	    result->issue_route != route->issue_route ||
	    result->transition_route != route->transition_route ||
	    result->submit_id != route->submit_id ||
	    result->requested_submit_id != route->requested_submit_id ||
	    result->selected_submit_id != route->selected_submit_id ||
	    result->resolved_submit_id != route->resolved_submit_id ||
	    result->ioctl_op != route->ioctl_op ||
	    result->entry_op != route->entry_op ||
	    result->call_op != route->call_op ||
	    result->entrypoint_op != route->entrypoint_op ||
	    result->route_op != route->route_op ||
	    result->op_kind != route->op_kind ||
	    result->issue_result ==
		    result->transition_result ||
	    result->issue_result !=
		    (result->result_op == DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE) ||
	    result->transition_result !=
		    (result->result_op ==
		     DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION) ||
	    result->result_op != route->route_op ||
	    result->refresh_views != outcome->refresh_views ||
	    result->refilled_from_complete !=
		    outcome->refilled_from_complete ||
	    result->issue_advanced != outcome->issue_advanced ||
	    result->running_advanced != outcome->running_advanced ||
	    result->complete_advanced != outcome->complete_advanced ||
	    result->active_running_changed !=
		    outcome->active_running_changed ||
	    result->selected_wrapped != outcome->selected_wrapped ||
	    result->owner_state != outcome->owner_state ||
	    result->packet_count != outcome->packet_count ||
	    result->selected_read_offset != outcome->selected_read_offset ||
	    result->selected_write_offset != outcome->selected_write_offset ||
	    result->selected_ccb_offset != outcome->selected_ccb_offset ||
	    result->selected_outstanding_packets !=
		    outcome->selected_outstanding_packets ||
	    result->selected_seq != outcome->selected_seq ||
	    result->selected_cookie != outcome->selected_cookie ||
	    result->epoch != outcome->epoch ||
	    result->global_issue_ticket != outcome->global_issue_ticket ||
	    result->global_running_ticket != outcome->global_running_ticket ||
	    result->global_complete_ticket !=
		    outcome->global_complete_ticket ||
	    result->next_issue_count != outcome->next_issue_count ||
	    result->next_transition_count != outcome->next_transition_count ||
	    memcmp(&result->next_state, &outcome->next_state,
		   sizeof(result->next_state)))
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_dispatch_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_dispatch *dispatch)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_result *result;
	int ret;

	if (!dev_priv || !op || !dispatch)
		return -EINVAL;
	if (op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE &&
	    op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION)
		return -EINVAL;

	memset(dispatch, 0, sizeof(*dispatch));

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_result_locked(
		dev_priv, op, dependency_fields_present,
		&dispatch->submit_op_result);
	if (ret)
		return ret;

	result = &dispatch->submit_op_result;
	dispatch->request_op = result->request_op;
	dispatch->response_op = result->response_op;
	dispatch->outcome = result->outcome;
	dispatch->active = result->active;
	dispatch->dependency_fields_present =
		result->dependency_fields_present;
	dispatch->issue_dispatch =
		op->op == DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE;
	dispatch->transition_dispatch =
		op->op == DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION;
	dispatch->resolved_submit_id = result->response_op.resolved_submit_id;
	dispatch->dispatch_op = op->op;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_dispatch_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_dispatch *dispatch)
{
	if (!dev_priv || !submit || !dispatch)
		return -EINVAL;
	if (dispatch->active && dispatch->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_result_locked(
		dev_priv, submit, &dispatch->submit_op_result);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_dispatch_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_dispatch *dispatch)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_result *result;
	int ret;

	if (!dev_priv || !submit || !dispatch)
		return -EINVAL;

	result = &dispatch->submit_op_result;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_result_locked(
		dev_priv, submit, result);
	if (ret)
		return ret;

	if (memcmp(&dispatch->request_op, &result->request_op,
		   sizeof(dispatch->request_op)) ||
	    memcmp(&dispatch->response_op, &result->response_op,
		   sizeof(dispatch->response_op)) ||
	    memcmp(&dispatch->outcome, &result->outcome,
		   sizeof(dispatch->outcome)) ||
	    dispatch->active != result->active ||
	    dispatch->dependency_fields_present !=
		    result->dependency_fields_present ||
	    dispatch->resolved_submit_id != result->resolved_submit_id ||
	    dispatch->resolved_submit_id !=
		    result->response_op.resolved_submit_id ||
	    dispatch->dispatch_op != result->result_op ||
	    dispatch->issue_dispatch !=
		    (dispatch->dispatch_op ==
		     DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE) ||
	    dispatch->transition_dispatch !=
		    (dispatch->dispatch_op ==
		     DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION) ||
	    dispatch->issue_dispatch == dispatch->transition_dispatch ||
	    dispatch->issue_dispatch != result->issue_result ||
	    dispatch->transition_dispatch != result->transition_result ||
	    !dispatch->active)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_handler_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_handler *handler)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_dispatch *dispatch;
	int ret;

	if (!dev_priv || !op || !handler)
		return -EINVAL;
	if (op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE &&
	    op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION)
		return -EINVAL;

	memset(handler, 0, sizeof(*handler));

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_dispatch_locked(
		dev_priv, op, dependency_fields_present,
		&handler->submit_op_dispatch);
	if (ret)
		return ret;

	dispatch = &handler->submit_op_dispatch;
	handler->request_op = dispatch->request_op;
	handler->response_op = dispatch->response_op;
	handler->outcome = dispatch->outcome;
	handler->active = dispatch->active;
	handler->dependency_fields_present =
		dispatch->dependency_fields_present;
	handler->issue_handler =
		op->op == DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE;
	handler->transition_handler =
		op->op == DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION;
	handler->resolved_submit_id = dispatch->resolved_submit_id;
	handler->handler_op = op->op;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_handler_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_handler *handler)
{
	if (!dev_priv || !submit || !handler)
		return -EINVAL;
	if (handler->active && handler->resolved_submit_id != submit->submit_id)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_dispatch_locked(
		dev_priv, submit, &handler->submit_op_dispatch);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_handler_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_handler *handler)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_dispatch *dispatch;
	int ret;

	if (!dev_priv || !submit || !handler)
		return -EINVAL;

	dispatch = &handler->submit_op_dispatch;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_dispatch_locked(
		dev_priv, submit, dispatch);
	if (ret)
		return ret;

	if (memcmp(&handler->request_op, &dispatch->request_op,
		   sizeof(handler->request_op)) ||
	    memcmp(&handler->response_op, &dispatch->response_op,
		   sizeof(handler->response_op)) ||
	    memcmp(&handler->outcome, &dispatch->outcome,
		   sizeof(handler->outcome)) ||
	    handler->active != dispatch->active ||
	    handler->dependency_fields_present !=
		    dispatch->dependency_fields_present ||
	    handler->resolved_submit_id != dispatch->resolved_submit_id ||
	    handler->resolved_submit_id !=
		    dispatch->response_op.resolved_submit_id ||
	    handler->handler_op != dispatch->dispatch_op ||
	    handler->issue_handler !=
		    (handler->handler_op ==
		     DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE) ||
	    handler->transition_handler !=
		    (handler->handler_op ==
		     DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION) ||
	    handler->issue_handler == handler->transition_handler ||
	    handler->issue_handler != dispatch->issue_dispatch ||
	    handler->transition_handler != dispatch->transition_dispatch ||
	    !handler->active)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_request_handler_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_handler *request)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_handler *handler;
	int ret;

	if (!dev_priv || !op || !request)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_handler_locked(
		dev_priv, op, dependency_fields_present,
		&request->submit_op_handler);
	if (ret)
		return ret;

	handler = &request->submit_op_handler;
	request->request_op = handler->request_op;
	request->response_op = handler->response_op;
	request->outcome = handler->outcome;
	request->active = handler->active;
	request->dependency_fields_present =
		handler->dependency_fields_present;
	request->issue_request = handler->issue_handler;
	request->transition_request = handler->transition_handler;
	request->resolved_submit_id = handler->resolved_submit_id;
	request->request_op_kind = op->op;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_handler_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_handler *request)
{
	if (!dev_priv || !submit || !request)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_handler_locked(
		dev_priv, submit, &request->submit_op_handler);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_handler_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_handler *request)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_handler *handler;
	int ret;

	if (!dev_priv || !submit || !request)
		return -EINVAL;

	handler = &request->submit_op_handler;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_handler_locked(
		dev_priv, submit, handler);
	if (ret)
		return ret;

	if (!request->active ||
	    request->dependency_fields_present !=
		    handler->dependency_fields_present ||
	    request->issue_request != handler->issue_handler ||
	    request->transition_request != handler->transition_handler ||
	    request->resolved_submit_id != handler->resolved_submit_id ||
	    request->resolved_submit_id !=
		    request->response_op.resolved_submit_id ||
	    request->request_op_kind != handler->handler_op)
		return -EINVAL;

	if (memcmp(&request->request_op, &handler->request_op,
		   sizeof(request->request_op)) != 0 ||
	    memcmp(&request->response_op, &handler->response_op,
		   sizeof(request->response_op)) != 0 ||
	    memcmp(&request->outcome, &handler->outcome,
		   sizeof(request->outcome)) != 0)
		return -EINVAL;

	if (request->issue_request == request->transition_request)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_request_wrapper_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_wrapper *wrapper)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_handler *request;
	int ret;

	if (!dev_priv || !op || !wrapper)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_request_handler_locked(
		dev_priv, op, dependency_fields_present,
		&wrapper->submit_op_request_handler);
	if (ret)
		return ret;

	request = &wrapper->submit_op_request_handler;
	wrapper->request_op = request->request_op;
	wrapper->response_op = request->response_op;
	wrapper->outcome = request->outcome;
	wrapper->active = request->active;
	wrapper->dependency_fields_present =
		request->dependency_fields_present;
	wrapper->issue_wrapper = request->issue_request;
	wrapper->transition_wrapper = request->transition_request;
	wrapper->resolved_submit_id = request->resolved_submit_id;
	wrapper->wrapper_op = op->op;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_wrapper_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_wrapper *wrapper)
{
	if (!dev_priv || !submit || !wrapper)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_handler_locked(
		dev_priv, submit, &wrapper->submit_op_request_handler);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_wrapper_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_wrapper *wrapper)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_handler *request;
	int ret;

	if (!dev_priv || !submit || !wrapper)
		return -EINVAL;

	request = &wrapper->submit_op_request_handler;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_handler_locked(
		dev_priv, submit, request);
	if (ret)
		return ret;

	if (!wrapper->active ||
	    wrapper->dependency_fields_present !=
		    request->dependency_fields_present ||
	    wrapper->issue_wrapper != request->issue_request ||
	    wrapper->transition_wrapper != request->transition_request ||
	    wrapper->resolved_submit_id != request->resolved_submit_id ||
	    wrapper->resolved_submit_id !=
		    wrapper->response_op.resolved_submit_id ||
	    wrapper->wrapper_op != request->request_op_kind)
		return -EINVAL;

	if (memcmp(&wrapper->request_op, &request->request_op,
		   sizeof(wrapper->request_op)) != 0 ||
	    memcmp(&wrapper->response_op, &request->response_op,
		   sizeof(wrapper->response_op)) != 0 ||
	    memcmp(&wrapper->outcome, &request->outcome,
		   sizeof(wrapper->outcome)) != 0)
		return -EINVAL;

	if (wrapper->issue_wrapper == wrapper->transition_wrapper)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_request_entry_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_entry *entry)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_wrapper *wrapper;
	int ret;

	if (!dev_priv || !op || !entry)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_request_wrapper_locked(
		dev_priv, op, dependency_fields_present,
		&entry->submit_op_request_wrapper);
	if (ret)
		return ret;

	wrapper = &entry->submit_op_request_wrapper;
	entry->request_op = wrapper->request_op;
	entry->response_op = wrapper->response_op;
	entry->outcome = wrapper->outcome;
	entry->active = wrapper->active;
	entry->dependency_fields_present = wrapper->dependency_fields_present;
	entry->issue_entry = wrapper->issue_wrapper;
	entry->transition_entry = wrapper->transition_wrapper;
	entry->resolved_submit_id = wrapper->resolved_submit_id;
	entry->entry_op = op->op;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_entry_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_entry *entry)
{
	if (!dev_priv || !submit || !entry)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_wrapper_locked(
		dev_priv, submit, &entry->submit_op_request_wrapper);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_entry_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_entry *entry)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_wrapper *wrapper;
	int ret;

	if (!dev_priv || !submit || !entry)
		return -EINVAL;

	wrapper = &entry->submit_op_request_wrapper;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_wrapper_locked(
		dev_priv, submit, wrapper);
	if (ret)
		return ret;

	if (!entry->active ||
	    entry->dependency_fields_present !=
		    wrapper->dependency_fields_present ||
	    entry->issue_entry != wrapper->issue_wrapper ||
	    entry->transition_entry != wrapper->transition_wrapper ||
	    entry->resolved_submit_id != wrapper->resolved_submit_id ||
	    entry->resolved_submit_id != entry->response_op.resolved_submit_id ||
	    entry->entry_op != wrapper->wrapper_op)
		return -EINVAL;

	if (memcmp(&entry->request_op, &wrapper->request_op,
		   sizeof(entry->request_op)) != 0 ||
	    memcmp(&entry->response_op, &wrapper->response_op,
		   sizeof(entry->response_op)) != 0 ||
	    memcmp(&entry->outcome, &wrapper->outcome,
		   sizeof(entry->outcome)) != 0)
		return -EINVAL;

	if (entry->issue_entry == entry->transition_entry)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_request_call_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_call *call)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_entry *entry;
	int ret;

	if (!dev_priv || !op || !call)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_request_entry_locked(
		dev_priv, op, dependency_fields_present,
		&call->submit_op_request_entry);
	if (ret)
		return ret;

	entry = &call->submit_op_request_entry;
	call->request_op = entry->request_op;
	call->response_op = entry->response_op;
	call->outcome = entry->outcome;
	call->active = entry->active;
	call->dependency_fields_present = entry->dependency_fields_present;
	call->issue_call = entry->issue_entry;
	call->transition_call = entry->transition_entry;
	call->resolved_submit_id = entry->resolved_submit_id;
	call->call_op = op->op;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_call_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_call *call)
{
	if (!dev_priv || !submit || !call)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_entry_locked(
		dev_priv, submit, &call->submit_op_request_entry);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_call_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_call *call)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_entry *entry;
	int ret;

	if (!dev_priv || !submit || !call)
		return -EINVAL;

	entry = &call->submit_op_request_entry;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_entry_locked(
		dev_priv, submit, entry);
	if (ret)
		return ret;

	if (!call->active ||
	    call->dependency_fields_present !=
		    entry->dependency_fields_present ||
	    call->issue_call != entry->issue_entry ||
	    call->transition_call != entry->transition_entry ||
	    call->resolved_submit_id != entry->resolved_submit_id ||
	    call->resolved_submit_id != call->response_op.resolved_submit_id ||
	    call->call_op != entry->entry_op)
		return -EINVAL;

	if (memcmp(&call->request_op, &entry->request_op,
		   sizeof(call->request_op)) != 0 ||
	    memcmp(&call->response_op, &entry->response_op,
		   sizeof(call->response_op)) != 0 ||
	    memcmp(&call->outcome, &entry->outcome,
		   sizeof(call->outcome)) != 0)
		return -EINVAL;

	if (call->issue_call == call->transition_call)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_request_dispatch_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_dispatch *dispatch)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_call *call;
	int ret;

	if (!dev_priv || !op || !dispatch)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_request_call_locked(
		dev_priv, op, dependency_fields_present,
		&dispatch->submit_op_request_call);
	if (ret)
		return ret;

	call = &dispatch->submit_op_request_call;
	dispatch->request_op = call->request_op;
	dispatch->response_op = call->response_op;
	dispatch->outcome = call->outcome;
	dispatch->active = call->active;
	dispatch->dependency_fields_present =
		call->dependency_fields_present;
	dispatch->issue_request_dispatch = call->issue_call;
	dispatch->transition_request_dispatch = call->transition_call;
	dispatch->resolved_submit_id = call->resolved_submit_id;
	dispatch->request_dispatch_op = op->op;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_dispatch_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_dispatch *dispatch)
{
	if (!dev_priv || !submit || !dispatch)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_call_locked(
		dev_priv, submit, &dispatch->submit_op_request_call);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_dispatch_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_dispatch *dispatch)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_call *call;
	int ret;

	if (!dev_priv || !submit || !dispatch)
		return -EINVAL;

	call = &dispatch->submit_op_request_call;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_call_locked(
		dev_priv, submit, call);
	if (ret)
		return ret;

	if (!dispatch->active ||
	    dispatch->dependency_fields_present !=
		    call->dependency_fields_present ||
	    dispatch->issue_request_dispatch != call->issue_call ||
	    dispatch->transition_request_dispatch != call->transition_call ||
	    dispatch->resolved_submit_id != call->resolved_submit_id ||
	    dispatch->resolved_submit_id !=
		    dispatch->response_op.resolved_submit_id ||
	    dispatch->request_dispatch_op != call->call_op)
		return -EINVAL;

	if (memcmp(&dispatch->request_op, &call->request_op,
		   sizeof(dispatch->request_op)) != 0 ||
	    memcmp(&dispatch->response_op, &call->response_op,
		   sizeof(dispatch->response_op)) != 0 ||
	    memcmp(&dispatch->outcome, &call->outcome,
		   sizeof(dispatch->outcome)) != 0)
		return -EINVAL;

	if (dispatch->issue_request_dispatch ==
	    dispatch->transition_request_dispatch)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_request_result_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_result *result)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_dispatch *dispatch;
	int ret;

	if (!dev_priv || !op || !result)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_request_dispatch_locked(
		dev_priv, op, dependency_fields_present,
		&result->submit_op_request_dispatch);
	if (ret)
		return ret;

	dispatch = &result->submit_op_request_dispatch;
	result->request_op = dispatch->request_op;
	result->response_op = dispatch->response_op;
	result->outcome = dispatch->outcome;
	result->active = dispatch->active;
	result->dependency_fields_present =
		dispatch->dependency_fields_present;
	result->issue_request_result = dispatch->issue_request_dispatch;
	result->transition_request_result =
		dispatch->transition_request_dispatch;
	result->resolved_submit_id = dispatch->resolved_submit_id;
	result->request_result_op = op->op;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_result_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_result *result)
{
	if (!dev_priv || !submit || !result)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_dispatch_locked(
		dev_priv, submit, &result->submit_op_request_dispatch);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_result_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_result *result)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_dispatch *dispatch;
	int ret;

	if (!dev_priv || !submit || !result)
		return -EINVAL;

	dispatch = &result->submit_op_request_dispatch;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_dispatch_locked(
		dev_priv, submit, dispatch);
	if (ret)
		return ret;

	if (!result->active ||
	    result->dependency_fields_present !=
		    dispatch->dependency_fields_present ||
	    result->issue_request_result !=
		    dispatch->issue_request_dispatch ||
	    result->transition_request_result !=
		    dispatch->transition_request_dispatch ||
	    result->resolved_submit_id != dispatch->resolved_submit_id ||
	    result->resolved_submit_id !=
		    result->response_op.resolved_submit_id ||
	    result->request_result_op != dispatch->request_dispatch_op)
		return -EINVAL;

	if (memcmp(&result->request_op, &dispatch->request_op,
		   sizeof(result->request_op)) != 0 ||
	    memcmp(&result->response_op, &dispatch->response_op,
		   sizeof(result->response_op)) != 0 ||
	    memcmp(&result->outcome, &dispatch->outcome,
		   sizeof(result->outcome)) != 0)
		return -EINVAL;

	if (result->issue_request_result ==
	    result->transition_request_result)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_request_response_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_response *response)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_result *result;
	int ret;

	if (!dev_priv || !op || !response)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_request_result_locked(
		dev_priv, op, dependency_fields_present,
		&response->submit_op_request_result);
	if (ret)
		return ret;

	result = &response->submit_op_request_result;
	response->request_op = result->request_op;
	response->response_op = result->response_op;
	response->outcome = result->outcome;
	response->active = result->active;
	response->dependency_fields_present =
		result->dependency_fields_present;
	response->issue_request_response = result->issue_request_result;
	response->transition_request_response =
		result->transition_request_result;
	response->resolved_submit_id = result->resolved_submit_id;
	response->request_response_op = op->op;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_response_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_response *response)
{
	if (!dev_priv || !submit || !response)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_result_locked(
		dev_priv, submit, &response->submit_op_request_result);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_response_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_response *response)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_result *result;
	int ret;

	if (!dev_priv || !submit || !response)
		return -EINVAL;

	result = &response->submit_op_request_result;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_result_locked(
		dev_priv, submit, result);
	if (ret)
		return ret;

	if (!response->active ||
	    response->dependency_fields_present !=
		    result->dependency_fields_present ||
	    response->issue_request_response !=
		    result->issue_request_result ||
	    response->transition_request_response !=
		    result->transition_request_result ||
	    response->resolved_submit_id != result->resolved_submit_id ||
	    response->resolved_submit_id !=
		    response->response_op.resolved_submit_id ||
	    response->request_response_op != result->request_result_op)
		return -EINVAL;

	if (memcmp(&response->request_op, &result->request_op,
		   sizeof(response->request_op)) != 0 ||
	    memcmp(&response->response_op, &result->response_op,
		   sizeof(response->response_op)) != 0 ||
	    memcmp(&response->outcome, &result->outcome,
		   sizeof(response->outcome)) != 0)
		return -EINVAL;

	if (response->issue_request_response ==
	    response->transition_request_response)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_request_return_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_return *retobj)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_response *response;
	int ret;

	if (!dev_priv || !op || !retobj)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_request_response_locked(
		dev_priv, op, dependency_fields_present,
		&retobj->submit_op_request_response);
	if (ret)
		return ret;

	response = &retobj->submit_op_request_response;
	retobj->request_op = response->request_op;
	retobj->response_op = response->response_op;
	retobj->outcome = response->outcome;
	retobj->active = response->active;
	retobj->dependency_fields_present =
		response->dependency_fields_present;
	retobj->issue_request_return = response->issue_request_response;
	retobj->transition_request_return =
		response->transition_request_response;
	retobj->resolved_submit_id = response->resolved_submit_id;
	retobj->request_return_op = op->op;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_return_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_return *retobj)
{
	if (!dev_priv || !submit || !retobj)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_response_locked(
		dev_priv, submit, &retobj->submit_op_request_response);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_return_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_return *retobj)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_response *response;
	int ret;

	if (!dev_priv || !submit || !retobj)
		return -EINVAL;

	response = &retobj->submit_op_request_response;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_response_locked(
		dev_priv, submit, response);
	if (ret)
		return ret;

	if (!retobj->active ||
	    retobj->dependency_fields_present !=
		    response->dependency_fields_present ||
	    retobj->issue_request_return !=
		    response->issue_request_response ||
	    retobj->transition_request_return !=
		    response->transition_request_response ||
	    retobj->resolved_submit_id != response->resolved_submit_id ||
	    retobj->resolved_submit_id !=
		    retobj->response_op.resolved_submit_id ||
	    retobj->request_return_op != response->request_response_op)
		return -EINVAL;

	if (memcmp(&retobj->request_op, &response->request_op,
		   sizeof(retobj->request_op)) != 0 ||
	    memcmp(&retobj->response_op, &response->response_op,
		   sizeof(retobj->response_op)) != 0 ||
	    memcmp(&retobj->outcome, &response->outcome,
		   sizeof(retobj->outcome)) != 0)
		return -EINVAL;

	if (retobj->issue_request_return ==
	    retobj->transition_request_return)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_request_resolution_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_resolution
		*resolution)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_return *retobj;
	int ret;

	if (!dev_priv || !op || !resolution)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_request_return_locked(
		dev_priv, op, dependency_fields_present,
		&resolution->submit_op_request_return);
	if (ret)
		return ret;

	retobj = &resolution->submit_op_request_return;
	resolution->request_op = retobj->request_op;
	resolution->response_op = retobj->response_op;
	resolution->outcome = retobj->outcome;
	resolution->active = retobj->active;
	resolution->dependency_fields_present =
		retobj->dependency_fields_present;
	resolution->issue_request_resolution =
		retobj->issue_request_return;
	resolution->transition_request_resolution =
		retobj->transition_request_return;
	resolution->resolved_submit_id = retobj->resolved_submit_id;
	resolution->request_resolution_op = op->op;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_resolution_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_resolution
		*resolution)
{
	if (!dev_priv || !submit || !resolution)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_return_locked(
		dev_priv, submit, &resolution->submit_op_request_return);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_resolution_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_resolution
		*resolution)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_return *retobj;
	int ret;

	if (!dev_priv || !submit || !resolution)
		return -EINVAL;

	retobj = &resolution->submit_op_request_return;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_return_locked(
		dev_priv, submit, retobj);
	if (ret)
		return ret;

	if (!resolution->active ||
	    resolution->dependency_fields_present !=
		    retobj->dependency_fields_present ||
	    resolution->issue_request_resolution !=
		    retobj->issue_request_return ||
	    resolution->transition_request_resolution !=
		    retobj->transition_request_return ||
	    resolution->resolved_submit_id != retobj->resolved_submit_id ||
	    resolution->resolved_submit_id !=
		    resolution->response_op.resolved_submit_id ||
	    resolution->request_resolution_op != retobj->request_return_op)
		return -EINVAL;

	if (memcmp(&resolution->request_op, &retobj->request_op,
		   sizeof(resolution->request_op)) != 0 ||
	    memcmp(&resolution->response_op, &retobj->response_op,
		   sizeof(resolution->response_op)) != 0 ||
	    memcmp(&resolution->outcome, &retobj->outcome,
		   sizeof(resolution->outcome)) != 0)
		return -EINVAL;

	if (resolution->issue_request_resolution ==
	    resolution->transition_request_resolution)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_build_submit_sgx_ta_submit_op_request_operation_locked(
	struct drm_psb_private *dev_priv,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_operation
		*operation)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_resolution
		*resolution;
	int ret;

	if (!dev_priv || !op || !operation)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_request_resolution_locked(
		dev_priv, op, dependency_fields_present,
		&operation->submit_op_request_resolution);
	if (ret)
		return ret;

	resolution = &operation->submit_op_request_resolution;
	operation->request_op = resolution->request_op;
	operation->response_op = resolution->response_op;
	operation->outcome = resolution->outcome;
	operation->active = resolution->active;
	operation->dependency_fields_present =
		resolution->dependency_fields_present;
	operation->issue_request_operation =
		resolution->issue_request_resolution;
	operation->transition_request_operation =
		resolution->transition_request_resolution;
	operation->resolved_submit_id = resolution->resolved_submit_id;
	operation->request_operation_op = op->op;

	return 0;
}

static int poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_operation_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_operation
		*operation)
{
	if (!dev_priv || !submit || !operation)
		return -EINVAL;

	return poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_resolution_locked(
		dev_priv, submit, &operation->submit_op_request_resolution);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_operation_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_operation
		*operation)
{
	const struct psb_sgx_lab_submit_sgx_ta_submit_op_request_resolution
		*resolution;
	int ret;

	if (!dev_priv || !submit || !operation)
		return -EINVAL;

	resolution = &operation->submit_op_request_resolution;
	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_resolution_locked(
		dev_priv, submit, resolution);
	if (ret)
		return ret;

	if (!operation->active ||
	    operation->dependency_fields_present !=
		    resolution->dependency_fields_present ||
	    operation->issue_request_operation !=
		    resolution->issue_request_resolution ||
	    operation->transition_request_operation !=
		    resolution->transition_request_resolution ||
	    operation->resolved_submit_id != resolution->resolved_submit_id ||
	    operation->resolved_submit_id !=
		    operation->response_op.resolved_submit_id ||
	    operation->request_operation_op !=
		    resolution->request_resolution_op)
		return -EINVAL;

	if (memcmp(&operation->request_op, &resolution->request_op,
		   sizeof(operation->request_op)) != 0 ||
	    memcmp(&operation->response_op, &resolution->response_op,
		   sizeof(operation->response_op)) != 0 ||
	    memcmp(&operation->outcome, &resolution->outcome,
		   sizeof(operation->outcome)) != 0)
		return -EINVAL;

	if (operation->issue_request_operation ==
	    operation->transition_request_operation)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_collect_submit_view_records_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_submit_view_records *records)
{
	u32 stage;
	int ret;

	if (!records)
		return -EINVAL;

	memset(records, 0, sizeof(*records));
	ret = poulsbo_sgx_build_submit_exec_record_locked(
		dev_priv, submit, PSB_SGX_LAB_EXEC_SOURCE_LIVE,
		&records->live);
	if (ret)
		return ret;
	records->have_live = true;

	stage = records->live.stage;
	records->plan_source =
		poulsbo_sgx_submit_plan_exec_source_locked(stage);
	if (records->plan_source == PSB_SGX_LAB_EXEC_SOURCE_LIVE) {
		memcpy(&records->plan, &records->live, sizeof(records->plan));
	} else {
		ret = poulsbo_sgx_build_submit_exec_record_locked(
			dev_priv, submit, records->plan_source, &records->plan);
		if (ret)
			return ret;
	}
	records->have_plan = true;

	records->result_source =
		poulsbo_sgx_submit_result_exec_source_locked(stage);
	if (records->result_source == records->plan_source) {
		memcpy(&records->result, &records->plan,
		       sizeof(records->result));
	} else if (records->result_source == PSB_SGX_LAB_EXEC_SOURCE_LIVE) {
		memcpy(&records->result, &records->live,
		       sizeof(records->result));
	} else {
		ret = poulsbo_sgx_build_submit_exec_record_locked(
			dev_priv, submit, records->result_source,
			&records->result);
		if (ret)
			return ret;
	}
	records->have_result = true;
	return 0;
}

static int poulsbo_sgx_object_write_words_locked(
	struct psb_sgx_lab_object *obj, u32 base_offset, const u32 *words,
	unsigned int word_count)
{
	unsigned int i;
	int ret;

	if (!obj || !words)
		return -EINVAL;

	for (i = 0; i < word_count; i++) {
		ret = poulsbo_sgx_object_write_u32_locked(
			obj, base_offset + i * sizeof(u32), words[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int poulsbo_sgx_object_read_words_locked(
	struct psb_sgx_lab_object *obj, u32 base_offset, u32 *words,
	unsigned int word_count)
{
	unsigned int i;
	int ret;

	if (!obj || !words)
		return -EINVAL;

	for (i = 0; i < word_count; i++) {
		ret = poulsbo_sgx_object_read_u32_locked(
			obj, base_offset + i * sizeof(u32), &words[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int poulsbo_sgx_fill_submit_queue_header_words_from_packet(
	const struct psb_sgx_lab_submit_exec_packet *packet,
	struct psb_sgx_lab_queue_header_words *queue_header)
{
	if (!packet || !queue_header)
		return -EINVAL;

	memset(queue_header, 0, sizeof(*queue_header));
	queue_header->write_offset = packet->ccb_write_offset;
	queue_header->read_offset = packet->ccb_read_offset;
	queue_header->queue_base_offset = packet->queue_base_offset;
	queue_header->queue_size = packet->queue_size;
	queue_header->queue_packet_size = packet->queue_packet_size;
	queue_header->queue_packet_count = packet->queue_packet_count;
	queue_header->queue_tail_packet_count = packet->queue_tail_packet_count;
	queue_header->queue_head_packet_count = packet->queue_head_packet_count;
	return 0;
}

static int poulsbo_sgx_build_submit_queue_header_locked(
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_queue_header_words *queue_header)
{
	struct psb_sgx_lab_submit_exec_packet packet;
	int ret;

	ret = poulsbo_sgx_build_submit_exec_packet_locked(
		NULL, submit, PSB_SGX_LAB_EXEC_SOURCE_LIVE, &packet);
	if (ret)
		return ret;

	return poulsbo_sgx_fill_submit_queue_header_words_from_packet(
		&packet, queue_header);
}

static int poulsbo_sgx_write_queue_header_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 header_offset,
	const struct psb_sgx_lab_queue_header_words *queue_header)
{
	return poulsbo_sgx_object_write_words_locked(
		control_obj, header_offset, (const u32 *)queue_header,
		sizeof(*queue_header) / sizeof(u32));
}

static int poulsbo_sgx_read_queue_header_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 header_offset,
	struct psb_sgx_lab_queue_header_words *queue_header)
{
	if (!queue_header)
		return -EINVAL;

	memset(queue_header, 0, sizeof(*queue_header));
	return poulsbo_sgx_object_read_words_locked(
		control_obj, header_offset, (u32 *)queue_header,
		sizeof(*queue_header) / sizeof(u32));
}

static int poulsbo_sgx_fill_submit_ccb_record_words_from_packet(
	const struct psb_sgx_lab_submit_exec_packet *packet,
	struct psb_sgx_lab_ccb_record_words *ccb_record)
{
	if (!packet || !ccb_record)
		return -EINVAL;

	memset(ccb_record, 0, sizeof(*ccb_record));
	ccb_record->service_address = packet->command.service_address;
	ccb_record->cache_control = packet->command.cache_control;
	ccb_record->data0 = packet->command.data0;
	ccb_record->data1 = packet->command.data1;
	ccb_record->pds_gpu_va32 = packet->pds_gpu_va32;
	ccb_record->use_gpu_va32 = packet->use_gpu_va32;
	ccb_record->target_gpu_va32 = packet->target_gpu_va32;
	ccb_record->status_gpu_va32 = packet->ccb_status_gpu_va32;
	return 0;
}

static int poulsbo_sgx_build_submit_ccb_record_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_ccb_record_words *ccb_record)
{
	struct psb_sgx_lab_submit_exec_packet packet;
	int ret;

	ret = poulsbo_sgx_build_submit_exec_packet_locked(
		dev_priv, submit, PSB_SGX_LAB_EXEC_SOURCE_LIVE, &packet);
	if (ret)
		return ret;

	return poulsbo_sgx_fill_submit_ccb_record_words_from_packet(
		&packet, ccb_record);
}

static int poulsbo_sgx_write_submit_ccb_record_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 ccb_offset,
	const struct psb_sgx_lab_ccb_record_words *ccb_record)
{
	return poulsbo_sgx_object_write_words_locked(
		control_obj, ccb_offset, (const u32 *)ccb_record,
		sizeof(*ccb_record) / sizeof(u32));
}

static int poulsbo_sgx_read_submit_ccb_record_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 ccb_offset,
	struct psb_sgx_lab_ccb_record_words *ccb_record)
{
	if (!ccb_record)
		return -EINVAL;

	memset(ccb_record, 0, sizeof(*ccb_record));
	return poulsbo_sgx_object_read_words_locked(
		control_obj, ccb_offset, (u32 *)ccb_record,
		sizeof(*ccb_record) / sizeof(u32));
}

static int poulsbo_sgx_fill_submit_sync_words_from_packet(
	const struct psb_sgx_lab_submit_exec_packet *packet,
	struct psb_sgx_lab_sync_words *sync)
{
	u32 flags;

	if (!packet || !sync)
		return -EINVAL;

	memset(sync, 0, sizeof(*sync));
	flags = packet->ta_shared.sync_flags | PSB_SGX_LAB_SYNC_FLAG_READY;
	if (packet->queue_head_packet_count)
		flags |= PSB_SGX_LAB_SYNC_FLAG_WRAPPED;
	else
		flags &= ~PSB_SGX_LAB_SYNC_FLAG_WRAPPED;

	sync->read_offset = packet->ccb_read_offset;
	sync->write_offset = packet->ccb_write_offset;
	sync->complete_seq = packet->ta_shared.sync_complete_seq;
	sync->pending_seq = packet->ta_shared.sync_pending_seq;
	sync->flags = flags;
	sync->epoch = packet->queue_epoch;
	sync->global_issue_ticket = packet->ta_shared.global_issue_ticket;
	return 0;
}

static int poulsbo_sgx_build_submit_sync_locked(
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_sync_words *sync)
{
	struct psb_sgx_lab_submit_exec_packet packet;
	int ret;

	ret = poulsbo_sgx_build_submit_exec_packet_locked(
		NULL, submit, PSB_SGX_LAB_EXEC_SOURCE_LIVE, &packet);
	if (ret)
		return ret;

	return poulsbo_sgx_fill_submit_sync_words_from_packet(
		&packet, sync);
}

static int poulsbo_sgx_write_submit_sync_locked(
	struct psb_sgx_lab_object *status_obj,
	u32 sync_offset,
	const struct psb_sgx_lab_sync_words *sync)
{
	return poulsbo_sgx_object_write_words_locked(
		status_obj, sync_offset, (const u32 *)sync,
		sizeof(*sync) / sizeof(u32));
}

static int poulsbo_sgx_read_submit_sync_locked(
	struct psb_sgx_lab_object *status_obj,
	u32 sync_offset,
	struct psb_sgx_lab_sync_words *sync)
{
	if (!sync)
		return -EINVAL;

	memset(sync, 0, sizeof(*sync));
	return poulsbo_sgx_object_read_words_locked(
		status_obj, sync_offset, (u32 *)sync,
		sizeof(*sync) / sizeof(u32));
}

static int poulsbo_sgx_fill_submit_owner_words_from_packet(
	const struct psb_sgx_lab_submit_exec_packet *packet,
	struct psb_sgx_lab_owner_words *owner)
{
	if (!packet || !owner)
		return -EINVAL;

	memset(owner, 0, sizeof(*owner));
	owner->owner_state = packet->ta_shared.owner_state;
	owner->owner_issue_cookie = packet->ta_shared.owner_issue_cookie;
	owner->owner_complete_cookie =
		packet->ta_shared.owner_complete_cookie;
	owner->epoch = packet->queue_epoch;
	owner->global_complete_ticket =
		packet->ta_shared.global_complete_ticket;
	owner->global_running_ticket =
		packet->ta_shared.global_running_ticket;
	return 0;
}

static int poulsbo_sgx_build_submit_owner_locked(
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_owner_words *owner)
{
	struct psb_sgx_lab_submit_exec_packet packet;
	int ret;

	ret = poulsbo_sgx_build_submit_exec_packet_locked(
		NULL, submit, PSB_SGX_LAB_EXEC_SOURCE_LIVE, &packet);
	if (ret)
		return ret;

	return poulsbo_sgx_fill_submit_owner_words_from_packet(
		&packet, owner);
}

static int poulsbo_sgx_write_submit_owner_locked(
	struct psb_sgx_lab_object *status_obj,
	u32 owner_offset,
	const struct psb_sgx_lab_owner_words *owner)
{
	return poulsbo_sgx_object_write_words_locked(
		status_obj, owner_offset, (const u32 *)owner,
		sizeof(*owner) / sizeof(u32));
}

static int poulsbo_sgx_read_submit_owner_locked(
	struct psb_sgx_lab_object *status_obj,
	u32 owner_offset,
	struct psb_sgx_lab_owner_words *owner)
{
	if (!owner)
		return -EINVAL;

	memset(owner, 0, sizeof(*owner));
	return poulsbo_sgx_object_read_words_locked(
		status_obj, owner_offset, (u32 *)owner,
		sizeof(*owner) / sizeof(u32));
}

static void
poulsbo_sgx_disarm_submit_locked(struct psb_sgx_lab_submit *submit)
{
	poulsbo_sgx_unlaunch_submit_locked(submit);
	poulsbo_sgx_clear_preissue_submit_locked(submit);
}

static void
poulsbo_sgx_record_dispatch_submit_locked(struct psb_sgx_lab_submit *submit)
{
	submit->dispatch_prepare_count = submit->launch_prepare_count;
	submit->dispatch_arm_count = submit->launch_arm_count;
	submit->dispatch_emit_count = submit->launch_emit_count;
	submit->dispatch_launch_count = submit->launch_count;
	submit->dispatch_context = submit->launch_context;
	submit->dispatch_cmd_type = submit->launch_cmd_type;
	submit->dispatch_owner_state = submit->launch_owner_state;
	submit->dispatch_issued_packets = submit->launch_issued_packets;
	submit->dispatch_completed_packets = 0;
	submit->dispatch_refilled_from_complete =
		submit->launch_refilled_from_complete;
	submit->dispatch_ctrl_flags = submit->launch_ctrl_flags;
	submit->dispatch_num_ta_status_vals =
		submit->launch_num_ta_status_vals;
	submit->dispatch_num_3d_status_vals =
		submit->launch_num_3d_status_vals;
	submit->dispatch_ta_dependency = submit->launch_ta_dependency;
	submit->dispatch_control_object_id = submit->launch_control_object_id;
	submit->dispatch_pds_object_id = submit->launch_pds_object_id;
	submit->dispatch_use_object_id = submit->launch_use_object_id;
	submit->dispatch_target_object_id = submit->launch_target_object_id;
	submit->dispatch_status_object_id = submit->launch_status_object_id;
	submit->dispatch_ccb_offset = submit->launch_ccb_offset;
	submit->dispatch_ccb_write_offset = submit->ccb_write_offset;
	submit->dispatch_ccb_read_offset = submit->launch_ccb_read_offset;
	submit->dispatch_ccb_service_address =
		submit->launch_ccb_service_address;
	submit->dispatch_ccb_cache_control = submit->launch_ccb_cache_control;
	submit->dispatch_ccb_data0 = submit->launch_ccb_data0;
	submit->dispatch_ccb_data1 = submit->launch_ccb_data1;
	submit->dispatch_queue_base_offset = submit->launch_queue_base_offset;
	submit->dispatch_queue_size = submit->launch_queue_size;
	submit->dispatch_queue_packet_size = submit->launch_queue_packet_size;
	submit->dispatch_queue_packet_count = submit->queue_packet_count;
	submit->dispatch_queue_tail_packet_count =
		submit->queue_tail_packet_count;
	submit->dispatch_queue_head_packet_count =
		submit->queue_head_packet_count;
	submit->dispatch_sync_offset = submit->launch_sync_offset;
	submit->dispatch_sync_complete_seq = submit->launch_sync_complete_seq;
	submit->dispatch_sync_pending_seq = submit->sync_pending_seq;
	submit->dispatch_sync_flags = submit->sync_flags;
	submit->dispatch_owner_offset = submit->launch_owner_offset;
	submit->dispatch_owner_issue_cookie = submit->owner_issue_cookie;
	submit->dispatch_owner_complete_cookie =
		submit->launch_owner_complete_cookie;
	submit->dispatch_queue_epoch = submit->queue_epoch;
	submit->dispatch_global_issue_ticket = submit->global_issue_ticket;
	submit->dispatch_global_running_ticket =
		submit->launch_global_running_ticket;
	submit->dispatch_global_complete_ticket =
		submit->launch_global_complete_ticket;
	submit->dispatch_dependency_active_count =
		submit->launch_dependency_active_count;
	submit->dispatch_control_gpu_va = submit->launch_control_gpu_va;
	submit->dispatch_pds_gpu_va = submit->launch_pds_gpu_va;
	submit->dispatch_use_gpu_va = submit->launch_use_gpu_va;
	submit->dispatch_target_gpu_va = submit->launch_target_gpu_va;
	submit->dispatch_status_gpu_va = submit->launch_status_gpu_va;
	submit->dispatch_valid = true;
	submit->dispatch_count++;
	submit->last_dispatch_jiffies = jiffies;
}

static void
poulsbo_sgx_retire_submit_locked(struct psb_sgx_lab_submit *submit)
{
	if (!submit->dispatch_valid ||
	    submit->dispatch_owner_state != PSB_SGX_LAB_OWNER_COMPLETE)
		return;

	submit->retired_prepare_count = submit->dispatch_prepare_count;
	submit->retired_arm_count = submit->dispatch_arm_count;
	submit->retired_emit_count = submit->dispatch_emit_count;
	submit->retired_launch_count = submit->dispatch_launch_count;
	submit->retired_dispatch_count = submit->dispatch_count;
	submit->retired_context = submit->dispatch_context;
	submit->retired_cmd_type = submit->dispatch_cmd_type;
	submit->retired_owner_state = submit->dispatch_owner_state;
	submit->retired_issued_packets = submit->dispatch_issued_packets;
	submit->retired_completed_packets = submit->dispatch_completed_packets;
	submit->retired_refilled_from_complete =
		submit->dispatch_refilled_from_complete;
	submit->retired_queue_packet_count =
		submit->dispatch_queue_packet_count;
	submit->retired_sync_complete_seq =
		submit->dispatch_sync_complete_seq;
	submit->retired_sync_pending_seq =
		submit->dispatch_sync_pending_seq;
	submit->retired_owner_issue_cookie =
		submit->dispatch_owner_issue_cookie;
	submit->retired_owner_complete_cookie =
		submit->dispatch_owner_complete_cookie;
	submit->retired_queue_epoch = submit->dispatch_queue_epoch;
	submit->retired_global_issue_ticket =
		submit->dispatch_global_issue_ticket;
	submit->retired_global_running_ticket =
		submit->dispatch_global_running_ticket;
	submit->retired_global_complete_ticket =
		submit->dispatch_global_complete_ticket;
	submit->retired_dependency_active_count =
		submit->dispatch_dependency_active_count;
	submit->retired_control_object_id =
		submit->dispatch_control_object_id;
	submit->retired_status_object_id =
		submit->dispatch_status_object_id;
	submit->retired_control_gpu_va = submit->dispatch_control_gpu_va;
	submit->retired_status_gpu_va = submit->dispatch_status_gpu_va;
	submit->retired_valid = true;
	submit->retired_count++;
	submit->last_retired_jiffies = jiffies;
}

static void
poulsbo_sgx_update_dispatch_submit_locked(struct psb_sgx_lab_submit *submit,
					  u32 owner_state,
					  u32 completed_packets)
{
	if (!submit->dispatch_valid)
		return;

	submit->dispatch_owner_state = owner_state;
	submit->dispatch_completed_packets += completed_packets;
	submit->dispatch_ccb_offset = submit->ccb_offset;
	submit->dispatch_ccb_write_offset = submit->ccb_write_offset;
	submit->dispatch_ccb_read_offset = submit->ccb_read_offset;
	submit->dispatch_queue_packet_count = submit->queue_packet_count;
	submit->dispatch_queue_tail_packet_count =
		submit->queue_tail_packet_count;
	submit->dispatch_queue_head_packet_count =
		submit->queue_head_packet_count;
	submit->dispatch_sync_complete_seq = submit->sync_complete_seq;
	submit->dispatch_sync_pending_seq = submit->sync_pending_seq;
	submit->dispatch_sync_flags = submit->sync_flags;
	submit->dispatch_owner_issue_cookie = submit->owner_issue_cookie;
	submit->dispatch_owner_complete_cookie =
		submit->owner_complete_cookie;
	submit->dispatch_queue_epoch = submit->queue_epoch;
	submit->dispatch_global_issue_ticket = submit->global_issue_ticket;
	submit->dispatch_global_running_ticket =
		submit->global_running_ticket;
	submit->dispatch_global_complete_ticket =
		submit->global_complete_ticket;
	submit->last_dispatch_jiffies = jiffies;
}

static bool
poulsbo_sgx_submit_arm_matches_locked(struct psb_sgx_lab_submit *submit)
{
	return submit->arm_valid &&
	       submit->arm_prepare_count == submit->prepare_count &&
	       submit->arm_queue_packet_count == submit->queue_packet_count &&
	       submit->arm_queue_tail_packet_count ==
		       submit->queue_tail_packet_count &&
	       submit->arm_queue_head_packet_count ==
		       submit->queue_head_packet_count &&
	       submit->arm_ccb_read_offset == submit->ccb_read_offset &&
	       submit->arm_ccb_write_offset == submit->ccb_write_offset &&
	       submit->arm_sync_complete_seq == submit->sync_complete_seq &&
	       submit->arm_sync_pending_seq == submit->sync_pending_seq &&
	       submit->arm_owner_state == submit->owner_state &&
	       submit->arm_owner_issue_cookie == submit->owner_issue_cookie &&
	       submit->arm_owner_complete_cookie ==
		       submit->owner_complete_cookie &&
	       submit->arm_queue_epoch == submit->queue_epoch &&
	       submit->arm_global_issue_ticket == submit->global_issue_ticket;
}

static u32
poulsbo_sgx_submit_dependency_active_count_locked(
	struct psb_sgx_lab_submit *submit)
{
	u32 active_count = 0;
	unsigned int slot;

	for (slot = 0; slot < DRM_PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
		if (poulsbo_sgx_dependency_slot_active(submit, slot))
			active_count++;
	}

	return active_count;
}

static u32 poulsbo_sgx_submit_report_stage_locked(
	struct psb_sgx_lab_submit *submit)
{
	u32 report_stage = PSB_SGX_LAB_REPORT_STAGE_NONE;
	u32 report_stage_count = 0;

	if (submit->prepare_count) {
		report_stage = PSB_SGX_LAB_REPORT_STAGE_PREPARED;
		report_stage_count = submit->prepare_count;
	}
	if (submit->arm_valid && submit->arm_count >= report_stage_count) {
		report_stage = PSB_SGX_LAB_REPORT_STAGE_ARMED;
		report_stage_count = submit->arm_count;
	}
	if (submit->emit_valid && submit->emit_count >= report_stage_count) {
		report_stage = PSB_SGX_LAB_REPORT_STAGE_EMITTED;
		report_stage_count = submit->emit_count;
	}
	if (submit->launch_valid &&
	    submit->launch_count >= report_stage_count) {
		report_stage = PSB_SGX_LAB_REPORT_STAGE_LAUNCHED;
		report_stage_count = submit->launch_count;
	}
	if (submit->dispatch_valid &&
	    submit->dispatch_count >= report_stage_count) {
		report_stage = PSB_SGX_LAB_REPORT_STAGE_DISPATCHED;
		report_stage_count = submit->dispatch_count;
	}
	if (submit->retired_valid &&
	    submit->retired_count >= report_stage_count)
		report_stage = PSB_SGX_LAB_REPORT_STAGE_RETIRED;

	return report_stage;
}

static u32 poulsbo_sgx_submit_report_stage_count_locked(
	struct psb_sgx_lab_submit *submit, u32 report_stage)
{
	switch (report_stage) {
	case PSB_SGX_LAB_REPORT_STAGE_PREPARED:
		return submit->prepare_count;
	case PSB_SGX_LAB_REPORT_STAGE_ARMED:
		return submit->arm_count;
	case PSB_SGX_LAB_REPORT_STAGE_EMITTED:
		return submit->emit_count;
	case PSB_SGX_LAB_REPORT_STAGE_LAUNCHED:
		return submit->launch_count;
	case PSB_SGX_LAB_REPORT_STAGE_DISPATCHED:
		return submit->dispatch_count;
	case PSB_SGX_LAB_REPORT_STAGE_RETIRED:
		return submit->retired_count;
	default:
		return 0;
	}
}

static u32 poulsbo_sgx_submit_report_completed_packets_locked(
	struct psb_sgx_lab_submit *submit, u32 report_stage)
{
	switch (report_stage) {
	case PSB_SGX_LAB_REPORT_STAGE_RETIRED:
		return submit->retired_completed_packets;
	case PSB_SGX_LAB_REPORT_STAGE_DISPATCHED:
		return submit->dispatch_completed_packets;
	default:
		return 0;
	}
}

static u32 poulsbo_sgx_submit_report_issued_packets_locked(
	struct psb_sgx_lab_submit *submit, u32 report_stage)
{
	switch (report_stage) {
	case PSB_SGX_LAB_REPORT_STAGE_RETIRED:
		return submit->retired_issued_packets;
	case PSB_SGX_LAB_REPORT_STAGE_DISPATCHED:
		return submit->dispatch_issued_packets;
	case PSB_SGX_LAB_REPORT_STAGE_LAUNCHED:
		return submit->launch_issued_packets;
	default:
		return 0;
	}
}

static u32 poulsbo_sgx_submit_report_dependency_count_locked(
	struct psb_sgx_lab_submit *submit, u32 report_stage)
{
	switch (report_stage) {
	case PSB_SGX_LAB_REPORT_STAGE_RETIRED:
		return submit->retired_dependency_active_count;
	case PSB_SGX_LAB_REPORT_STAGE_DISPATCHED:
		return submit->dispatch_dependency_active_count;
	case PSB_SGX_LAB_REPORT_STAGE_LAUNCHED:
		return submit->launch_dependency_active_count;
	case PSB_SGX_LAB_REPORT_STAGE_EMITTED:
		return submit->emit_dependency_active_count;
	default:
		return poulsbo_sgx_submit_dependency_active_count_locked(submit);
	}
}

static u32 poulsbo_sgx_submit_report_flags_locked(
	struct psb_sgx_lab_submit *submit)
{
	u32 flags = 0;

	if (submit->arm_valid)
		flags |= DRM_PSB_SGX_LAB_REPORT_FLAG_ARM_VALID;
	if (submit->emit_valid)
		flags |= DRM_PSB_SGX_LAB_REPORT_FLAG_EMIT_VALID;
	if (submit->launch_valid)
		flags |= DRM_PSB_SGX_LAB_REPORT_FLAG_LAUNCH_VALID;
	if (submit->launch_consumed)
		flags |= DRM_PSB_SGX_LAB_REPORT_FLAG_LAUNCH_CONSUMED;
	if (submit->dispatch_valid)
		flags |= DRM_PSB_SGX_LAB_REPORT_FLAG_DISPATCH_VALID;
	if (submit->retired_valid)
		flags |= DRM_PSB_SGX_LAB_REPORT_FLAG_RETIRED_VALID;
	return flags;
}

static int poulsbo_sgx_build_submit_report_locked(
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_report_words *report)
{
	int ret;
	struct psb_sgx_lab_submit_exec_packet packet;

	if (!report)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_exec_packet_locked(
		NULL, submit, PSB_SGX_LAB_EXEC_SOURCE_LIVE, &packet);
	if (ret)
		return ret;

	memset(report, 0, sizeof(*report));
	report->stage = packet.stage;
	report->stage_count = packet.stage_count;
	report->prepare_count = submit->prepare_count;
	report->arm_count = submit->arm_count;
	report->emit_count = submit->emit_count;
	report->launch_count = submit->launch_count;
	report->dispatch_count = submit->dispatch_count;
	report->retired_count = submit->retired_count;
	report->owner_state = packet.ta_shared.owner_state;
	report->queue_packet_count = packet.queue_packet_count;
	report->completed_packets = packet.completed_packets;
	report->dependency_active_count =
		packet.ta_shared.dependency_active_count;
	report->queue_epoch = packet.queue_epoch;
	report->global_issue_ticket = packet.ta_shared.global_issue_ticket;
	report->global_running_ticket =
		packet.ta_shared.global_running_ticket;
	report->global_complete_ticket =
		packet.ta_shared.global_complete_ticket;
	report->flags = packet.ta_shared.report_flags;
	report->context = packet.context;

	return 0;
}

static int poulsbo_sgx_build_submit_live_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_live_words *live)
{
	struct psb_sgx_lab_submit_exec_packet packet;
	int ret;

	if (!live)
		return -EINVAL;

	memset(live, 0, sizeof(*live));
	ret = poulsbo_sgx_build_submit_exec_packet_locked(
		dev_priv, submit, PSB_SGX_LAB_EXEC_SOURCE_LIVE, &packet);
	if (ret)
		return ret;

	live->stage = packet.stage;
	live->stage_count = packet.stage_count;
	live->prepare_count = submit->prepare_count;
	live->arm_count = submit->arm_count;
	live->emit_count = submit->emit_count;
	live->launch_count = submit->launch_count;
	live->dispatch_count = submit->dispatch_count;
	live->retired_count = submit->retired_count;
	live->context = packet.context;
	live->cmd_type = packet.cmd_type;
	live->owner_state = packet.ta_shared.owner_state;
	live->issued_packets = packet.issued_packets;
	live->completed_packets = packet.completed_packets;
	live->dependency_active_count =
		packet.ta_shared.dependency_active_count;
	live->refilled_from_complete = packet.refilled_from_complete;
	live->queue_packet_count = packet.queue_packet_count;
	live->queue_epoch = packet.queue_epoch;
	live->queue_base_offset = packet.queue_base_offset;
	live->queue_size = packet.queue_size;
	live->queue_packet_size = packet.queue_packet_size;
	live->ccb_offset = packet.ccb_offset;
	live->ccb_write_offset = packet.ccb_write_offset;
	live->ccb_read_offset = packet.ccb_read_offset;
	live->sync_offset = packet.ta_shared.sync_offset;
	live->sync_complete_seq = packet.ta_shared.sync_complete_seq;
	live->sync_pending_seq = packet.ta_shared.sync_pending_seq;
	live->sync_flags = packet.ta_shared.sync_flags;
	live->owner_offset = packet.ta_shared.owner_offset;
	live->owner_issue_cookie = packet.ta_shared.owner_issue_cookie;
	live->owner_complete_cookie =
		packet.ta_shared.owner_complete_cookie;
	live->global_issue_ticket = packet.ta_shared.global_issue_ticket;
	live->global_running_ticket =
		packet.ta_shared.global_running_ticket;
	live->global_complete_ticket =
		packet.ta_shared.global_complete_ticket;
	live->ctrl_flags = packet.ta_shared.ctrl_flags;
	live->report_flags = packet.ta_shared.report_flags;
	live->num_ta_status_vals = packet.ta_shared.num_ta_status_vals;
	live->num_3d_status_vals = packet.ta_shared.num_3d_status_vals;
	live->ta_dependency = packet.ta_shared.ta_dependency;
	live->status_ta_offset = packet.ta_shared.status_ta_offset;
	live->status_3d_offset = packet.ta_shared.status_3d_offset;
	live->status_stride = packet.ta_shared.status_stride;
	live->status_value_seed = packet.ta_shared.status_value_seed;
	live->control_object_id = packet.control_object_id;
	live->status_object_id = packet.status_object_id;
	live->ccb_service_address = packet.command.service_address;
	live->ccb_cache_control = packet.command.cache_control;
	live->ccb_data0 = packet.command.data0;
	live->ccb_data1 = packet.command.data1;
	live->control_gpu_va32 = packet.control_gpu_va32;
	live->status_gpu_va32 = packet.status_gpu_va32;
	live->pds_gpu_va32 = packet.pds_gpu_va32;
	live->use_gpu_va32 = packet.use_gpu_va32;
	live->target_gpu_va32 = packet.target_gpu_va32;
	live->ccb_status_gpu_va32 = packet.ccb_status_gpu_va32;
	return 0;
}

static int poulsbo_sgx_write_submit_live_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 live_offset,
	const struct psb_sgx_lab_live_words *live)
{
	return poulsbo_sgx_object_write_words_locked(
		control_obj, live_offset, (const u32 *)live,
		sizeof(*live) / sizeof(u32));
}

static int poulsbo_sgx_read_submit_live_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 live_offset,
	struct psb_sgx_lab_live_words *live)
{
	if (!live)
		return -EINVAL;

	memset(live, 0, sizeof(*live));
	return poulsbo_sgx_object_read_words_locked(
		control_obj, live_offset, (u32 *)live,
		sizeof(*live) / sizeof(u32));
}

static int poulsbo_sgx_refresh_submit_live_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_refresh_views *views)
{
	int ret;

	if (!submit->live_config_count)
		return 0;
	if (!views || !views->control_obj || !views->have_live)
		return -EINVAL;

	ret = poulsbo_sgx_write_submit_live_locked(views->control_obj,
						   submit->live_offset,
						   &views->live);
	if (ret)
		return ret;

	submit->last_live_jiffies = jiffies;
	return 0;
}

static int poulsbo_sgx_write_submit_report_locked(
	struct psb_sgx_lab_object *status_obj,
	u32 report_offset,
	const struct psb_sgx_lab_report_words *report)
{
	return poulsbo_sgx_object_write_words_locked(
		status_obj, report_offset, (const u32 *)report,
		sizeof(*report) / sizeof(u32));
}

static int poulsbo_sgx_read_submit_report_locked(
	struct psb_sgx_lab_object *status_obj,
	u32 report_offset,
	struct psb_sgx_lab_report_words *report)
{
	if (!report)
		return -EINVAL;

	memset(report, 0, sizeof(*report));
	return poulsbo_sgx_object_read_words_locked(
		status_obj, report_offset, (u32 *)report,
		sizeof(*report) / sizeof(u32));
}

static int poulsbo_sgx_report_region_end(u32 base_offset, u32 *end_out);
static int poulsbo_sgx_live_region_end(u32 base_offset, u32 *end_out);
static int poulsbo_sgx_manifest_region_end(u32 base_offset, u32 *end_out);
static int poulsbo_sgx_descriptor_region_end(u32 base_offset, u32 *end_out);
static int poulsbo_sgx_plan_region_end(u32 base_offset, u32 *end_out);
static int poulsbo_sgx_issue_plan_region_end(u32 base_offset, u32 *end_out);
static int poulsbo_sgx_result_region_end(u32 base_offset, u32 *end_out);
struct psb_sgx_lab_control_layout;
static int poulsbo_sgx_validate_submit_control_layout_locked(
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_control_layout *layout_out);
struct psb_sgx_lab_status_layout;
static int poulsbo_sgx_validate_submit_status_layout_locked(
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *status_obj,
	struct psb_sgx_lab_status_layout *layout_out);
static u32 *poulsbo_sgx_dependency_offset_ptr(
	struct psb_sgx_lab_submit *submit, unsigned int slot);
static u32 *poulsbo_sgx_dependency_config_count_ptr(
	struct psb_sgx_lab_submit *submit, unsigned int slot);
static int poulsbo_sgx_write_submit_result_locked(
	struct psb_sgx_lab_object *status_obj,
	u32 result_offset,
	const struct psb_sgx_lab_result_words *result);

static bool
poulsbo_sgx_submit_has_control_views(struct psb_sgx_lab_submit *submit)
{
	return submit->packet_config_count || submit->ccb_config_count ||
	       submit->queue_config_count || submit->live_config_count ||
	       submit->manifest_config_count ||
	       submit->descriptor_config_count || submit->plan_config_count ||
	       submit->issue_plan_config_count;
}

static bool
poulsbo_sgx_submit_has_status_views(struct psb_sgx_lab_submit *submit)
{
	/*
	 * Kick config sets the TA/3D status value counts before the later
	 * status op defines the backing status-memory layout. Refresh preflight
	 * should only validate real status-backed views here, not those raw
	 * counts alone, or kick will fail before status layout exists.
	 */
	return submit->status_config_count || submit->sync_config_count ||
	       submit->owner_config_count ||
	       submit->report_config_count ||
	       submit->result_config_count ||
	       submit->dependency_config_count ||
	       submit->dependency_config_count_secondary ||
	       submit->dependency_config_count_tertiary ||
	       submit->dependency_config_count_quaternary;
}

static int poulsbo_sgx_refresh_submit_report_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_refresh_views *views)
{
	int ret;

	if (!submit->report_config_count)
		return 0;
	if (!views || !views->status_obj || !views->have_report)
		return -EINVAL;

	ret = poulsbo_sgx_write_submit_report_locked(views->status_obj,
						    submit->report_offset,
						    &views->report);
	if (ret)
		return ret;

	submit->report_stage = views->report.stage;
	submit->report_stage_count = views->report.stage_count;
	submit->report_owner_state = views->report.owner_state;
	submit->report_queue_packet_count = views->report.queue_packet_count;
	submit->report_completed_packets = views->report.completed_packets;
	submit->report_dependency_active_count =
		views->report.dependency_active_count;
	submit->report_queue_epoch = views->report.queue_epoch;
	submit->report_global_issue_ticket = views->report.global_issue_ticket;
	submit->report_global_running_ticket = views->report.global_running_ticket;
	submit->report_global_complete_ticket =
		views->report.global_complete_ticket;
	submit->report_flags = views->report.flags;
	submit->last_report_jiffies = jiffies;

	return 0;
}

static int poulsbo_sgx_refresh_submit_result_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_refresh_views *views)
{
	int ret;

	if (!submit->result_config_count)
		return 0;
	if (!views || !views->status_obj || !views->have_result)
		return -EINVAL;

	ret = poulsbo_sgx_write_submit_result_locked(views->status_obj,
						     submit->result_offset,
						     &views->result);
	if (ret)
		return ret;

	submit->last_result_jiffies = jiffies;
	return 0;
}

static int poulsbo_sgx_build_submit_manifest_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_manifest_words *manifest)
{
	struct psb_sgx_lab_submit_exec_packet packet;
	int ret;

	if (!manifest)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_exec_packet_locked(
		dev_priv, submit, PSB_SGX_LAB_EXEC_SOURCE_LIVE, &packet);
	if (ret)
		return ret;

	memset(manifest, 0, sizeof(*manifest));
	manifest->stage = packet.stage;
	manifest->stage_count = packet.stage_count;
	manifest->context = packet.context;
	manifest->cmd_type = packet.cmd_type;
	manifest->owner_state = packet.ta_shared.owner_state;
	manifest->ctrl_flags = packet.ta_shared.ctrl_flags;
	manifest->queue_base_offset = packet.queue_base_offset;
	manifest->queue_size = packet.queue_size;
	manifest->queue_packet_size = packet.queue_packet_size;
	manifest->queue_packet_count = packet.queue_packet_count;
	manifest->queue_epoch = packet.queue_epoch;
	manifest->ccb_offset = packet.ccb_offset;
	manifest->ccb_write_offset = packet.ccb_write_offset;
	manifest->ccb_read_offset = packet.ccb_read_offset;
	manifest->dependency_active_count =
		packet.ta_shared.dependency_active_count;
	manifest->global_issue_ticket = packet.ta_shared.global_issue_ticket;
	manifest->global_running_ticket =
		packet.ta_shared.global_running_ticket;
	manifest->global_complete_ticket =
		packet.ta_shared.global_complete_ticket;
	manifest->pds_gpu_va32 = packet.pds_gpu_va32;
	manifest->use_gpu_va32 = packet.use_gpu_va32;
	manifest->target_gpu_va32 = packet.target_gpu_va32;
	manifest->status_gpu_va32 = packet.ccb_status_gpu_va32;
	manifest->sync_complete_seq = packet.ta_shared.sync_complete_seq;
	manifest->sync_pending_seq = packet.ta_shared.sync_pending_seq;
	manifest->sync_flags = packet.ta_shared.sync_flags;
	manifest->owner_issue_cookie = packet.ta_shared.owner_issue_cookie;
	manifest->owner_complete_cookie =
		packet.ta_shared.owner_complete_cookie;
	return 0;
}

static int poulsbo_sgx_write_submit_manifest_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 manifest_offset,
	const struct psb_sgx_lab_manifest_words *manifest)
{
	int ret;

	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj, manifest_offset + PSB_SGX_LAB_MANIFEST_STAGE_OFFSET,
		manifest->stage);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_STAGE_COUNT_OFFSET,
		manifest->stage_count);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_CONTEXT_OFFSET,
		manifest->context);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_CMD_TYPE_OFFSET,
		manifest->cmd_type);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_OWNER_STATE_OFFSET,
		manifest->owner_state);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_CTRL_FLAGS_OFFSET,
		manifest->ctrl_flags);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_QUEUE_BASE_OFFSET,
		manifest->queue_base_offset);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_QUEUE_SIZE_OFFSET,
		manifest->queue_size);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_QUEUE_PACKET_SIZE_OFFSET,
		manifest->queue_packet_size);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_QUEUE_PACKET_COUNT_OFFSET,
		manifest->queue_packet_count);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_QUEUE_EPOCH_OFFSET,
		manifest->queue_epoch);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_CCB_OFFSET,
		manifest->ccb_offset);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_CCB_WRITE_OFFSET,
		manifest->ccb_write_offset);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_CCB_READ_OFFSET,
		manifest->ccb_read_offset);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset +
			PSB_SGX_LAB_MANIFEST_DEPENDENCY_ACTIVE_COUNT_OFFSET,
		manifest->dependency_active_count);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset +
			PSB_SGX_LAB_MANIFEST_GLOBAL_ISSUE_TICKET_OFFSET,
		manifest->global_issue_ticket);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset +
			PSB_SGX_LAB_MANIFEST_GLOBAL_RUNNING_TICKET_OFFSET,
		manifest->global_running_ticket);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset +
			PSB_SGX_LAB_MANIFEST_GLOBAL_COMPLETE_TICKET_OFFSET,
		manifest->global_complete_ticket);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_PDS_GPU_VA32_OFFSET,
		manifest->pds_gpu_va32);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_USE_GPU_VA32_OFFSET,
		manifest->use_gpu_va32);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_TARGET_GPU_VA32_OFFSET,
		manifest->target_gpu_va32);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_STATUS_GPU_VA32_OFFSET,
		manifest->status_gpu_va32);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset +
			PSB_SGX_LAB_MANIFEST_SYNC_COMPLETE_SEQ_OFFSET,
		manifest->sync_complete_seq);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_SYNC_PENDING_SEQ_OFFSET,
		manifest->sync_pending_seq);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_SYNC_FLAGS_OFFSET,
		manifest->sync_flags);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset +
			PSB_SGX_LAB_MANIFEST_OWNER_ISSUE_COOKIE_OFFSET,
		manifest->owner_issue_cookie);
	if (ret)
		return ret;
	return poulsbo_sgx_object_write_u32_locked(
		control_obj,
		manifest_offset +
			PSB_SGX_LAB_MANIFEST_OWNER_COMPLETE_COOKIE_OFFSET,
		manifest->owner_complete_cookie);
}

static int poulsbo_sgx_read_submit_manifest_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 manifest_offset,
	struct psb_sgx_lab_manifest_words *manifest)
{
	int ret;

	if (!manifest)
		return -EINVAL;

	memset(manifest, 0, sizeof(*manifest));
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj, manifest_offset + PSB_SGX_LAB_MANIFEST_STAGE_OFFSET,
		&manifest->stage);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_STAGE_COUNT_OFFSET,
		&manifest->stage_count);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_CONTEXT_OFFSET,
		&manifest->context);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_CMD_TYPE_OFFSET,
		&manifest->cmd_type);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_OWNER_STATE_OFFSET,
		&manifest->owner_state);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_CTRL_FLAGS_OFFSET,
		&manifest->ctrl_flags);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_QUEUE_BASE_OFFSET,
		&manifest->queue_base_offset);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_QUEUE_SIZE_OFFSET,
		&manifest->queue_size);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_QUEUE_PACKET_SIZE_OFFSET,
		&manifest->queue_packet_size);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_QUEUE_PACKET_COUNT_OFFSET,
		&manifest->queue_packet_count);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_QUEUE_EPOCH_OFFSET,
		&manifest->queue_epoch);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_CCB_OFFSET,
		&manifest->ccb_offset);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_CCB_WRITE_OFFSET,
		&manifest->ccb_write_offset);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_CCB_READ_OFFSET,
		&manifest->ccb_read_offset);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset +
			PSB_SGX_LAB_MANIFEST_DEPENDENCY_ACTIVE_COUNT_OFFSET,
		&manifest->dependency_active_count);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset +
			PSB_SGX_LAB_MANIFEST_GLOBAL_ISSUE_TICKET_OFFSET,
		&manifest->global_issue_ticket);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset +
			PSB_SGX_LAB_MANIFEST_GLOBAL_RUNNING_TICKET_OFFSET,
		&manifest->global_running_ticket);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset +
			PSB_SGX_LAB_MANIFEST_GLOBAL_COMPLETE_TICKET_OFFSET,
		&manifest->global_complete_ticket);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_PDS_GPU_VA32_OFFSET,
		&manifest->pds_gpu_va32);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_USE_GPU_VA32_OFFSET,
		&manifest->use_gpu_va32);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_TARGET_GPU_VA32_OFFSET,
		&manifest->target_gpu_va32);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_STATUS_GPU_VA32_OFFSET,
		&manifest->status_gpu_va32);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset +
			PSB_SGX_LAB_MANIFEST_SYNC_COMPLETE_SEQ_OFFSET,
		&manifest->sync_complete_seq);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_SYNC_PENDING_SEQ_OFFSET,
		&manifest->sync_pending_seq);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset + PSB_SGX_LAB_MANIFEST_SYNC_FLAGS_OFFSET,
		&manifest->sync_flags);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset +
			PSB_SGX_LAB_MANIFEST_OWNER_ISSUE_COOKIE_OFFSET,
		&manifest->owner_issue_cookie);
	if (ret)
		return ret;
	return poulsbo_sgx_object_read_u32_locked(
		control_obj,
		manifest_offset +
			PSB_SGX_LAB_MANIFEST_OWNER_COMPLETE_COOKIE_OFFSET,
		&manifest->owner_complete_cookie);
}

static int poulsbo_sgx_refresh_submit_manifest_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_refresh_views *views)
{
	int ret;

	if (!submit->manifest_config_count)
		return 0;
	if (!views || !views->control_obj || !views->have_manifest)
		return -EINVAL;

	ret = poulsbo_sgx_write_submit_manifest_locked(views->control_obj,
						       submit->manifest_offset,
						       &views->manifest);
	if (ret)
		return ret;

	submit->last_manifest_jiffies = jiffies;
	return 0;
}

static int poulsbo_sgx_build_submit_descriptor_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_descriptor_words *descriptor)
{
	struct psb_sgx_lab_submit_exec_packet packet;
	int ret;

	if (!descriptor)
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_exec_packet_locked(
		dev_priv, submit, PSB_SGX_LAB_EXEC_SOURCE_LIVE, &packet);
	if (ret)
		return ret;

	memset(descriptor, 0, sizeof(*descriptor));
	descriptor->stage = packet.stage;
	descriptor->stage_count = packet.stage_count;
	descriptor->context = packet.context;
	descriptor->cmd_type = packet.cmd_type;
	descriptor->owner_state = packet.ta_shared.owner_state;
	descriptor->issued_packets = packet.issued_packets;
	descriptor->completed_packets = packet.completed_packets;
	descriptor->dependency_active_count =
		packet.ta_shared.dependency_active_count;
	descriptor->queue_packet_count = packet.queue_packet_count;
	descriptor->queue_epoch = packet.queue_epoch;
	descriptor->ctrl_flags = packet.ta_shared.ctrl_flags;
	descriptor->queue_base_offset = packet.queue_base_offset;
	descriptor->queue_size = packet.queue_size;
	descriptor->queue_packet_size = packet.queue_packet_size;
	descriptor->ccb_offset = packet.ccb_offset;
	descriptor->sync_offset = packet.ta_shared.sync_offset;
	descriptor->owner_offset = packet.ta_shared.owner_offset;
	descriptor->report_offset = submit->report_offset;
	descriptor->manifest_offset = submit->manifest_offset;
	descriptor->global_issue_ticket =
		packet.ta_shared.global_issue_ticket;
	descriptor->global_running_ticket =
		packet.ta_shared.global_running_ticket;
	descriptor->global_complete_ticket =
		packet.ta_shared.global_complete_ticket;
	descriptor->report_flags = packet.ta_shared.report_flags;
	descriptor->status_ta_offset = packet.ta_shared.status_ta_offset;
	descriptor->status_3d_offset = packet.ta_shared.status_3d_offset;
	descriptor->status_stride = packet.ta_shared.status_stride;
	descriptor->status_value_seed = packet.ta_shared.status_value_seed;
	descriptor->control_gpu_va32 = packet.control_gpu_va32;
	descriptor->pds_gpu_va32 = packet.pds_gpu_va32;
	descriptor->use_gpu_va32 = packet.use_gpu_va32;
	descriptor->target_gpu_va32 = packet.target_gpu_va32;
	descriptor->status_gpu_va32 = packet.ccb_status_gpu_va32;
	return 0;
}

static int poulsbo_sgx_write_submit_descriptor_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 descriptor_offset,
	const struct psb_sgx_lab_descriptor_words *descriptor)
{
	return poulsbo_sgx_object_write_words_locked(
		control_obj, descriptor_offset, (const u32 *)descriptor,
		sizeof(*descriptor) / sizeof(u32));
}

static int poulsbo_sgx_read_submit_descriptor_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 descriptor_offset,
	struct psb_sgx_lab_descriptor_words *descriptor)
{
	if (!descriptor)
		return -EINVAL;

	memset(descriptor, 0, sizeof(*descriptor));
	return poulsbo_sgx_object_read_words_locked(
		control_obj, descriptor_offset, (u32 *)descriptor,
		sizeof(*descriptor) / sizeof(u32));
}

static int poulsbo_sgx_refresh_submit_descriptor_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_refresh_views *views)
{
	int ret;

	if (!submit->descriptor_config_count)
		return 0;
	if (!views || !views->control_obj || !views->have_descriptor)
		return -EINVAL;

	ret = poulsbo_sgx_write_submit_descriptor_locked(
		views->control_obj, submit->descriptor_offset,
		&views->descriptor);
	if (ret)
		return ret;

	submit->last_descriptor_jiffies = jiffies;
	return 0;
}

static int poulsbo_sgx_build_submit_plan_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_plan_words *plan)
{
	enum psb_sgx_lab_exec_source source;
	struct psb_sgx_lab_submit_exec_packet packet;
	u32 stage;
	int ret;

	if (!plan)
		return -EINVAL;

	memset(plan, 0, sizeof(*plan));
	stage = poulsbo_sgx_submit_report_stage_locked(submit);
	source = poulsbo_sgx_submit_plan_exec_source_locked(stage);
	ret = poulsbo_sgx_build_submit_exec_packet_locked(
		dev_priv, submit, source, &packet);
	if (ret)
		return ret;

	plan->stage = packet.stage;
	plan->stage_count = packet.stage_count;
	plan->source = poulsbo_sgx_submit_plan_source_locked(source);
	plan->report_offset = submit->report_offset;
	plan->manifest_offset = submit->manifest_offset;
	plan->descriptor_offset = submit->descriptor_offset;

	plan->context = packet.context;
	plan->cmd_type = packet.cmd_type;
	plan->owner_state = packet.ta_shared.owner_state;
	plan->issued_packets = packet.issued_packets;
	plan->completed_packets = packet.completed_packets;
	plan->dependency_active_count = packet.ta_shared.dependency_active_count;
	plan->refilled_from_complete = packet.refilled_from_complete;
	plan->queue_packet_count = packet.queue_packet_count;
	plan->queue_epoch = packet.queue_epoch;
	plan->queue_base_offset = packet.queue_base_offset;
	plan->queue_size = packet.queue_size;
	plan->queue_packet_size = packet.queue_packet_size;
	plan->ccb_offset = packet.ccb_offset;
	plan->ccb_write_offset = packet.ccb_write_offset;
	plan->ccb_read_offset = packet.ccb_read_offset;
	plan->sync_offset = packet.ta_shared.sync_offset;
	plan->sync_complete_seq = packet.ta_shared.sync_complete_seq;
	plan->sync_pending_seq = packet.ta_shared.sync_pending_seq;
	plan->sync_flags = packet.ta_shared.sync_flags;
	plan->owner_offset = packet.ta_shared.owner_offset;
	plan->owner_issue_cookie = packet.ta_shared.owner_issue_cookie;
	plan->owner_complete_cookie =
		packet.ta_shared.owner_complete_cookie;
	plan->global_issue_ticket = packet.ta_shared.global_issue_ticket;
	plan->global_running_ticket =
		packet.ta_shared.global_running_ticket;
	plan->global_complete_ticket =
		packet.ta_shared.global_complete_ticket;
	plan->ctrl_flags = packet.ta_shared.ctrl_flags;
	plan->ccb_service_address = packet.command.service_address;
	plan->ccb_cache_control = packet.command.cache_control;
	plan->ccb_data0 = packet.command.data0;
	plan->ccb_data1 = packet.command.data1;
	plan->control_gpu_va32 = packet.control_gpu_va32;
	plan->pds_gpu_va32 = packet.pds_gpu_va32;
	plan->use_gpu_va32 = packet.use_gpu_va32;
	plan->target_gpu_va32 = packet.target_gpu_va32;
	plan->status_gpu_va32 = packet.ccb_status_gpu_va32;
	return 0;
}

static int poulsbo_sgx_build_submit_issue_plan_view_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_issue_plan_words *issue_plan)
{
	struct psb_sgx_lab_submit_exec_packet live_packet;
	struct psb_sgx_lab_submit_cmdta_kick launch_kick;
	int ret;

	if (!issue_plan)
		return -EINVAL;

	memset(issue_plan, 0, sizeof(*issue_plan));
	ret = poulsbo_sgx_build_submit_exec_packet_locked(
		dev_priv, submit, PSB_SGX_LAB_EXEC_SOURCE_LIVE, &live_packet);
	if (ret)
		return ret;

	issue_plan->stage = live_packet.stage;
	issue_plan->stage_count = live_packet.stage_count;

	if (live_packet.stage != PSB_SGX_LAB_REPORT_STAGE_LAUNCHED ||
	    !submit->launch_valid || submit->launch_consumed ||
	    !submit->issue_plan_packet_count)
		return 0;

	ret = poulsbo_sgx_build_submit_cmdta_kick_locked(
		dev_priv, submit, PSB_SGX_LAB_EXEC_SOURCE_LAUNCH, true,
		&launch_kick);
	if (ret)
		return ret;

	issue_plan->valid = 1;
	issue_plan->owner_state = launch_kick.issue_plan.owner_state;
	issue_plan->issued_packets = launch_kick.issue_plan.issued_packets;
	issue_plan->refilled_from_complete =
		launch_kick.issue_plan.refilled_from_complete ? 1 : 0;
	issue_plan->queue_epoch = launch_kick.issue_plan.new_queue_epoch;
	issue_plan->queue_base_offset = launch_kick.packet.queue_base_offset;
	issue_plan->queue_size = launch_kick.packet.queue_size;
	issue_plan->queue_packet_size =
		launch_kick.packet.queue_packet_size;
	issue_plan->queue_packet_count =
		launch_kick.issue_plan.new_packet_count;
	issue_plan->queue_tail_packet_count =
		launch_kick.issue_plan.new_tail_packets;
	issue_plan->queue_head_packet_count =
		launch_kick.issue_plan.new_head_packets;
	issue_plan->ccb_offset = launch_kick.packet.ccb_offset;
	issue_plan->ccb_write_offset =
		launch_kick.issue_plan.new_write_offset;
	issue_plan->ccb_read_offset = launch_kick.packet.ccb_read_offset;
	issue_plan->sync_offset = launch_kick.packet.ta_shared.sync_offset;
	issue_plan->sync_complete_seq =
		launch_kick.packet.ta_shared.sync_complete_seq;
	issue_plan->sync_pending_seq =
		launch_kick.issue_plan.new_sync_pending_seq;
	issue_plan->sync_flags = launch_kick.issue_plan.new_sync_flags;
	issue_plan->owner_offset = launch_kick.packet.ta_shared.owner_offset;
	issue_plan->owner_issue_cookie =
		launch_kick.issue_plan.new_owner_issue_cookie;
	issue_plan->owner_complete_cookie =
		launch_kick.packet.ta_shared.owner_complete_cookie;
	issue_plan->global_issue_ticket =
		launch_kick.issue_plan.new_global_issue_ticket;
	issue_plan->global_running_ticket =
		launch_kick.packet.ta_shared.global_running_ticket;
	issue_plan->global_complete_ticket =
		launch_kick.packet.ta_shared.global_complete_ticket;
	issue_plan->dependency_active_count =
		launch_kick.packet.ta_shared.dependency_active_count;
	issue_plan->ccb_service_address =
		launch_kick.packet.command.service_address;
	issue_plan->ccb_cache_control =
		launch_kick.packet.command.cache_control;
	issue_plan->ccb_data0 = launch_kick.packet.command.data0;
	issue_plan->ccb_data1 = launch_kick.packet.command.data1;
	issue_plan->control_object_id = launch_kick.packet.control_object_id;
	issue_plan->status_object_id = launch_kick.packet.status_object_id;
	issue_plan->control_gpu_va32 = launch_kick.packet.control_gpu_va32;
	issue_plan->pds_gpu_va32 = launch_kick.packet.pds_gpu_va32;
	issue_plan->use_gpu_va32 = launch_kick.packet.use_gpu_va32;
	issue_plan->target_gpu_va32 = launch_kick.packet.target_gpu_va32;
	issue_plan->status_gpu_va32 = launch_kick.packet.status_gpu_va32;
	return 0;
}

static int poulsbo_sgx_build_submit_result_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_result_words *result)
{
	enum psb_sgx_lab_exec_source source;
	struct psb_sgx_lab_submit_exec_packet packet;
	u32 stage;
	int ret;

	if (!result)
		return -EINVAL;

	memset(result, 0, sizeof(*result));
	stage = poulsbo_sgx_submit_report_stage_locked(submit);
	source = poulsbo_sgx_submit_result_exec_source_locked(stage);
	ret = poulsbo_sgx_build_submit_exec_packet_locked(
		dev_priv, submit, source, &packet);
	if (ret)
		return ret;

	result->stage = packet.stage;
	result->stage_count = packet.stage_count;
	result->source = poulsbo_sgx_submit_result_source_locked(source);
	result->context = packet.context;
	result->cmd_type = packet.cmd_type;
	result->owner_state = packet.ta_shared.owner_state;
	result->issued_packets = packet.issued_packets;
	result->completed_packets = packet.completed_packets;
	result->dependency_active_count =
		packet.ta_shared.dependency_active_count;
	result->queue_packet_count = packet.queue_packet_count;
	result->queue_epoch = packet.queue_epoch;
	result->sync_complete_seq = packet.ta_shared.sync_complete_seq;
	result->sync_pending_seq = packet.ta_shared.sync_pending_seq;
	result->sync_flags = packet.ta_shared.sync_flags;
	result->owner_issue_cookie = packet.ta_shared.owner_issue_cookie;
	result->owner_complete_cookie =
		packet.ta_shared.owner_complete_cookie;
	result->global_issue_ticket = packet.ta_shared.global_issue_ticket;
	result->global_running_ticket =
		packet.ta_shared.global_running_ticket;
	result->global_complete_ticket =
		packet.ta_shared.global_complete_ticket;
	result->ctrl_flags = packet.ta_shared.ctrl_flags;
	result->num_ta_status_vals = packet.ta_shared.num_ta_status_vals;
	result->num_3d_status_vals = packet.ta_shared.num_3d_status_vals;
	result->ta_dependency = packet.ta_shared.ta_dependency;
	result->status_ta_offset = packet.ta_shared.status_ta_offset;
	result->status_3d_offset = packet.ta_shared.status_3d_offset;
	result->status_stride = packet.ta_shared.status_stride;
	result->status_value_seed = packet.ta_shared.status_value_seed;
	result->control_object_id = packet.control_object_id;
	result->status_object_id = packet.status_object_id;
	result->control_gpu_va32 = packet.control_gpu_va32;
	result->status_gpu_va32 = packet.status_gpu_va32;
	result->report_offset = submit->report_offset;
	result->manifest_offset = submit->manifest_offset;
	result->descriptor_offset = submit->descriptor_offset;
	result->plan_offset = submit->plan_offset;
	return 0;
}

static int poulsbo_sgx_write_submit_result_locked(
	struct psb_sgx_lab_object *status_obj,
	u32 result_offset,
	const struct psb_sgx_lab_result_words *result)
{
	return poulsbo_sgx_object_write_words_locked(
		status_obj, result_offset, (const u32 *)result,
		sizeof(*result) / sizeof(u32));
}

static int poulsbo_sgx_read_submit_result_locked(
	struct psb_sgx_lab_object *status_obj,
	u32 result_offset,
	struct psb_sgx_lab_result_words *result)
{
	if (!result)
		return -EINVAL;

	memset(result, 0, sizeof(*result));
	return poulsbo_sgx_object_read_words_locked(
		status_obj, result_offset, (u32 *)result,
		sizeof(*result) / sizeof(u32));
}

static int poulsbo_sgx_write_submit_plan_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 plan_offset,
	const struct psb_sgx_lab_plan_words *plan)
{
	return poulsbo_sgx_object_write_words_locked(
		control_obj, plan_offset, (const u32 *)plan,
		sizeof(*plan) / sizeof(u32));
}

static int poulsbo_sgx_read_submit_plan_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 plan_offset,
	struct psb_sgx_lab_plan_words *plan)
{
	if (!plan)
		return -EINVAL;

	memset(plan, 0, sizeof(*plan));
	return poulsbo_sgx_object_read_words_locked(
		control_obj, plan_offset, (u32 *)plan,
		sizeof(*plan) / sizeof(u32));
}

static int poulsbo_sgx_write_submit_issue_plan_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 issue_plan_offset,
	const struct psb_sgx_lab_issue_plan_words *issue_plan)
{
	return poulsbo_sgx_object_write_words_locked(
		control_obj, issue_plan_offset, (const u32 *)issue_plan,
		sizeof(*issue_plan) / sizeof(u32));
}

static int poulsbo_sgx_read_submit_issue_plan_locked(
	struct psb_sgx_lab_object *control_obj,
	u32 issue_plan_offset,
	struct psb_sgx_lab_issue_plan_words *issue_plan)
{
	if (!issue_plan)
		return -EINVAL;

	memset(issue_plan, 0, sizeof(*issue_plan));
	return poulsbo_sgx_object_read_words_locked(
		control_obj, issue_plan_offset, (u32 *)issue_plan,
		sizeof(*issue_plan) / sizeof(u32));
}

static int poulsbo_sgx_refresh_submit_plan_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_refresh_views *views)
{
	int ret;

	if (!submit->plan_config_count)
		return 0;
	if (!views || !views->control_obj || !views->have_plan)
		return -EINVAL;

	ret = poulsbo_sgx_write_submit_plan_locked(views->control_obj,
						   submit->plan_offset,
						   &views->plan);
	if (ret)
		return ret;

	submit->last_plan_jiffies = jiffies;
	return 0;
}

static int poulsbo_sgx_refresh_submit_issue_plan_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_refresh_views *views)
{
	int ret;

	if (!submit->issue_plan_config_count)
		return 0;
	if (!views || !views->control_obj || !views->have_issue_plan)
		return -EINVAL;

	ret = poulsbo_sgx_write_submit_issue_plan_locked(
		views->control_obj, submit->issue_plan_offset,
		&views->issue_plan);
	if (ret)
		return ret;

	submit->last_issue_plan_jiffies = jiffies;
	return 0;
}

static int poulsbo_sgx_preflight_submit_views_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_refresh_views *views)
{
	struct psb_sgx_lab_submit_view_records records;
	int ret;

	if (!views)
		return -EINVAL;

	memset(views, 0, sizeof(*views));

	if (poulsbo_sgx_submit_has_control_views(submit)) {
		if (!submit->control_object_id)
			return -EINVAL;
		views->control_obj = poulsbo_sgx_find_object_locked(
			dev_priv, submit->control_object_id);
		if (!views->control_obj)
			return -ENOENT;
		ret = poulsbo_sgx_validate_submit_control_layout_locked(
			submit, views->control_obj, NULL);
		if (ret)
			return ret;
	}

	if (poulsbo_sgx_submit_has_status_views(submit)) {
		if (!submit->status_object_id)
			return -EINVAL;
		views->status_obj = poulsbo_sgx_find_object_locked(
			dev_priv, submit->status_object_id);
		if (!views->status_obj)
			return -ENOENT;
		ret = poulsbo_sgx_validate_submit_status_layout_locked(
			submit, views->status_obj, NULL);
		if (ret)
			return ret;
	}

	if (submit->live_config_count || submit->report_config_count ||
	    submit->manifest_config_count ||
	    submit->descriptor_config_count || submit->plan_config_count ||
	    submit->issue_plan_config_count || submit->result_config_count) {
		ret = poulsbo_sgx_collect_submit_view_records_locked(
			dev_priv, submit, &records);
		if (ret)
			return ret;
		if (submit->live_config_count) {
			ret = poulsbo_sgx_build_submit_live_locked(
				dev_priv, submit, &views->live);
			if (ret)
				return ret;
			views->have_live = true;
		}
		if (submit->report_config_count) {
			memset(&views->report, 0, sizeof(views->report));
			views->report.stage = records.live.stage;
			views->report.stage_count = records.live.stage_count;
			views->report.prepare_count = records.live.prepare_count;
			views->report.arm_count = records.live.arm_count;
			views->report.emit_count = records.live.emit_count;
			views->report.launch_count = records.live.launch_count;
			views->report.dispatch_count = records.live.dispatch_count;
			views->report.retired_count = records.live.retired_count;
			views->report.owner_state = records.live.owner_state;
			views->report.queue_packet_count =
				records.live.queue_packet_count;
			views->report.completed_packets =
				records.live.completed_packets;
			views->report.dependency_active_count =
				records.live.dependency_active_count;
			views->report.queue_epoch = records.live.queue_epoch;
			views->report.global_issue_ticket =
				records.live.global_issue_ticket;
			views->report.global_running_ticket =
				records.live.global_running_ticket;
			views->report.global_complete_ticket =
				records.live.global_complete_ticket;
			views->report.flags = records.live.report_flags;
			views->report.context = records.live.context;
			views->have_report = true;
		}

		if (submit->manifest_config_count) {
			memset(&views->manifest, 0, sizeof(views->manifest));
			views->manifest.stage = records.live.stage;
			views->manifest.stage_count = records.live.stage_count;
			views->manifest.context = records.live.context;
			views->manifest.cmd_type = records.live.cmd_type;
			views->manifest.owner_state = records.live.owner_state;
			views->manifest.ctrl_flags = records.live.ctrl_flags;
			views->manifest.queue_base_offset =
				records.live.queue_base_offset;
			views->manifest.queue_size = records.live.queue_size;
			views->manifest.queue_packet_size =
				records.live.queue_packet_size;
			views->manifest.queue_packet_count =
				records.live.queue_packet_count;
			views->manifest.queue_epoch = records.live.queue_epoch;
			views->manifest.ccb_offset = records.live.ccb_offset;
			views->manifest.ccb_write_offset =
				records.live.ccb_write_offset;
			views->manifest.ccb_read_offset =
				records.live.ccb_read_offset;
			views->manifest.dependency_active_count =
				records.live.dependency_active_count;
			views->manifest.global_issue_ticket =
				records.live.global_issue_ticket;
			views->manifest.global_running_ticket =
				records.live.global_running_ticket;
			views->manifest.global_complete_ticket =
				records.live.global_complete_ticket;
			views->manifest.pds_gpu_va32 = records.live.pds_gpu_va32;
			views->manifest.use_gpu_va32 = records.live.use_gpu_va32;
			views->manifest.target_gpu_va32 =
				records.live.target_gpu_va32;
			views->manifest.status_gpu_va32 =
				records.live.ccb_status_gpu_va32;
			views->manifest.sync_complete_seq =
				records.live.sync_complete_seq;
			views->manifest.sync_pending_seq =
				records.live.sync_pending_seq;
			views->manifest.sync_flags = records.live.sync_flags;
			views->manifest.owner_issue_cookie =
				records.live.owner_issue_cookie;
			views->manifest.owner_complete_cookie =
				records.live.owner_complete_cookie;
			views->have_manifest = true;
		}

		if (submit->descriptor_config_count) {
			memset(&views->descriptor, 0, sizeof(views->descriptor));
			views->descriptor.stage = records.live.stage;
			views->descriptor.stage_count = records.live.stage_count;
			views->descriptor.context = records.live.context;
			views->descriptor.cmd_type = records.live.cmd_type;
			views->descriptor.owner_state = records.live.owner_state;
			views->descriptor.issued_packets =
				records.live.issued_packets;
			views->descriptor.completed_packets =
				records.live.completed_packets;
			views->descriptor.dependency_active_count =
				records.live.dependency_active_count;
			views->descriptor.queue_packet_count =
				records.live.queue_packet_count;
			views->descriptor.queue_epoch =
				records.live.queue_epoch;
			views->descriptor.ctrl_flags = records.live.ctrl_flags;
			views->descriptor.queue_base_offset =
				records.live.queue_base_offset;
			views->descriptor.queue_size = records.live.queue_size;
			views->descriptor.queue_packet_size =
				records.live.queue_packet_size;
			views->descriptor.ccb_offset = records.live.ccb_offset;
			views->descriptor.sync_offset = records.live.sync_offset;
			views->descriptor.owner_offset = records.live.owner_offset;
			views->descriptor.report_offset = submit->report_offset;
			views->descriptor.manifest_offset =
				submit->manifest_offset;
			views->descriptor.global_issue_ticket =
				records.live.global_issue_ticket;
			views->descriptor.global_running_ticket =
				records.live.global_running_ticket;
			views->descriptor.global_complete_ticket =
				records.live.global_complete_ticket;
			views->descriptor.report_flags =
				records.live.report_flags;
			views->descriptor.status_ta_offset =
				records.live.status_ta_offset;
			views->descriptor.status_3d_offset =
				records.live.status_3d_offset;
			views->descriptor.status_stride =
				records.live.status_stride;
			views->descriptor.status_value_seed =
				records.live.status_value_seed;
			views->descriptor.control_gpu_va32 =
				records.live.control_gpu_va32;
			views->descriptor.pds_gpu_va32 = records.live.pds_gpu_va32;
			views->descriptor.use_gpu_va32 = records.live.use_gpu_va32;
			views->descriptor.target_gpu_va32 =
				records.live.target_gpu_va32;
			views->descriptor.status_gpu_va32 =
				records.live.ccb_status_gpu_va32;
			views->have_descriptor = true;
		}

		if (submit->plan_config_count) {
			memset(&views->plan, 0, sizeof(views->plan));
			views->plan.stage = records.plan.stage;
			views->plan.stage_count = records.plan.stage_count;
			views->plan.source = poulsbo_sgx_submit_plan_source_locked(
				records.plan_source);
			views->plan.report_offset = submit->report_offset;
			views->plan.manifest_offset = submit->manifest_offset;
			views->plan.descriptor_offset = submit->descriptor_offset;
			views->plan.context = records.plan.context;
			views->plan.cmd_type = records.plan.cmd_type;
			views->plan.owner_state = records.plan.owner_state;
			views->plan.issued_packets = records.plan.issued_packets;
			views->plan.completed_packets =
				records.plan.completed_packets;
			views->plan.dependency_active_count =
				records.plan.dependency_active_count;
			views->plan.refilled_from_complete =
				records.plan.refilled_from_complete;
			views->plan.queue_packet_count =
				records.plan.queue_packet_count;
			views->plan.queue_epoch = records.plan.queue_epoch;
			views->plan.queue_base_offset =
				records.plan.queue_base_offset;
			views->plan.queue_size = records.plan.queue_size;
			views->plan.queue_packet_size =
				records.plan.queue_packet_size;
			views->plan.ccb_offset = records.plan.ccb_offset;
			views->plan.ccb_write_offset =
				records.plan.ccb_write_offset;
			views->plan.ccb_read_offset =
				records.plan.ccb_read_offset;
			views->plan.sync_offset = records.plan.sync_offset;
			views->plan.sync_complete_seq =
				records.plan.sync_complete_seq;
			views->plan.sync_pending_seq =
				records.plan.sync_pending_seq;
			views->plan.sync_flags = records.plan.sync_flags;
			views->plan.owner_offset = records.plan.owner_offset;
			views->plan.owner_issue_cookie =
				records.plan.owner_issue_cookie;
			views->plan.owner_complete_cookie =
				records.plan.owner_complete_cookie;
			views->plan.global_issue_ticket =
				records.plan.global_issue_ticket;
			views->plan.global_running_ticket =
				records.plan.global_running_ticket;
			views->plan.global_complete_ticket =
				records.plan.global_complete_ticket;
			views->plan.ctrl_flags = records.plan.ctrl_flags;
			views->plan.ccb_service_address =
				records.plan.ccb_service_address;
			views->plan.ccb_cache_control =
				records.plan.ccb_cache_control;
			views->plan.ccb_data0 = records.plan.ccb_data0;
			views->plan.ccb_data1 = records.plan.ccb_data1;
			views->plan.control_gpu_va32 =
				records.plan.control_gpu_va32;
			views->plan.pds_gpu_va32 = records.plan.pds_gpu_va32;
			views->plan.use_gpu_va32 = records.plan.use_gpu_va32;
			views->plan.target_gpu_va32 = records.plan.target_gpu_va32;
			views->plan.status_gpu_va32 =
				records.plan.ccb_status_gpu_va32;
			views->have_plan = true;
		}

		if (submit->issue_plan_config_count) {
			ret = poulsbo_sgx_build_submit_issue_plan_view_locked(
				dev_priv, submit, &views->issue_plan);
			if (ret)
				return ret;
			views->have_issue_plan = true;
		}

		if (submit->result_config_count) {
			memset(&views->result, 0, sizeof(views->result));
			views->result.stage = records.result.stage;
			views->result.stage_count = records.result.stage_count;
			views->result.source =
				poulsbo_sgx_submit_result_source_locked(
					records.result_source);
			views->result.context = records.result.context;
			views->result.cmd_type = records.result.cmd_type;
			views->result.owner_state = records.result.owner_state;
			views->result.issued_packets = records.result.issued_packets;
			views->result.completed_packets =
				records.result.completed_packets;
			views->result.dependency_active_count =
				records.result.dependency_active_count;
			views->result.queue_packet_count =
				records.result.queue_packet_count;
			views->result.queue_epoch = records.result.queue_epoch;
			views->result.sync_complete_seq =
				records.result.sync_complete_seq;
			views->result.sync_pending_seq =
				records.result.sync_pending_seq;
			views->result.sync_flags = records.result.sync_flags;
			views->result.owner_issue_cookie =
				records.result.owner_issue_cookie;
			views->result.owner_complete_cookie =
				records.result.owner_complete_cookie;
			views->result.global_issue_ticket =
				records.result.global_issue_ticket;
			views->result.global_running_ticket =
				records.result.global_running_ticket;
			views->result.global_complete_ticket =
				records.result.global_complete_ticket;
			views->result.ctrl_flags = records.result.ctrl_flags;
			views->result.num_ta_status_vals =
				records.result.num_ta_status_vals;
			views->result.num_3d_status_vals =
				records.result.num_3d_status_vals;
			views->result.ta_dependency = records.result.ta_dependency;
			views->result.status_ta_offset =
				records.result.status_ta_offset;
			views->result.status_3d_offset =
				records.result.status_3d_offset;
			views->result.status_stride =
				records.result.status_stride;
			views->result.status_value_seed =
				records.result.status_value_seed;
			views->result.control_object_id =
				records.result.control_object_id;
			views->result.status_object_id =
				records.result.status_object_id;
			views->result.control_gpu_va32 =
				records.result.control_gpu_va32;
			views->result.status_gpu_va32 =
				records.result.status_gpu_va32;
			views->result.report_offset = submit->report_offset;
			views->result.manifest_offset = submit->manifest_offset;
			views->result.descriptor_offset =
				submit->descriptor_offset;
			views->result.plan_offset = submit->plan_offset;
			views->have_result = true;
		}
	}

	return 0;
}

static int poulsbo_sgx_refresh_submit_views_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit)
{
	struct psb_sgx_lab_refresh_views views;
	int ret;

	ret = poulsbo_sgx_preflight_submit_views_locked(dev_priv, submit,
							 &views);
	if (ret)
		return ret;

	ret = poulsbo_sgx_refresh_submit_live_locked(submit, &views);
	if (ret)
		return ret;
	ret = poulsbo_sgx_refresh_submit_report_locked(submit, &views);
	if (ret)
		return ret;
	ret = poulsbo_sgx_refresh_submit_result_locked(submit, &views);
	if (ret)
		return ret;
	ret = poulsbo_sgx_refresh_submit_manifest_locked(submit, &views);
	if (ret)
		return ret;
	ret = poulsbo_sgx_refresh_submit_descriptor_locked(submit, &views);
	if (ret)
		return ret;
	ret = poulsbo_sgx_refresh_submit_plan_locked(submit, &views);
	if (ret)
		return ret;
	return poulsbo_sgx_refresh_submit_issue_plan_locked(submit, &views);
}

static bool
poulsbo_sgx_submit_emit_matches_locked(struct drm_psb_private *dev_priv,
				       struct psb_sgx_lab_submit *submit)
{
	struct psb_sgx_lab_object *control_obj = NULL;
	struct psb_sgx_lab_object *pds_obj = NULL;
	struct psb_sgx_lab_object *use_obj = NULL;
	struct psb_sgx_lab_object *target_obj = NULL;
	struct psb_sgx_lab_object *status_obj = NULL;
	u32 active_count;
	u64 control_gpu_va = 0;
	u64 pds_gpu_va = 0;
	u64 use_gpu_va = 0;
	u64 target_gpu_va = 0;
	u64 status_gpu_va = 0;

	if (!submit->emit_valid || !poulsbo_sgx_submit_arm_matches_locked(submit))
		return false;

	if (submit->emit_prepare_count != submit->prepare_count ||
	    submit->emit_arm_count != submit->arm_count ||
	    submit->emit_context != submit->context ||
	    submit->emit_cmd_type != submit->cmd_type ||
	    submit->emit_owner_state != submit->owner_state ||
	    submit->emit_ctrl_flags != submit->ctrl_flags ||
	    submit->emit_num_ta_status_vals != submit->num_ta_status_vals ||
	    submit->emit_num_3d_status_vals != submit->num_3d_status_vals ||
	    submit->emit_ta_dependency != submit->ta_dependency ||
	    submit->emit_control_object_id != submit->control_object_id ||
	    submit->emit_pds_object_id != submit->pds_object_id ||
	    submit->emit_use_object_id != submit->use_object_id ||
	    submit->emit_target_object_id != submit->target_object_id ||
	    submit->emit_status_object_id != submit->status_object_id ||
	    submit->emit_ccb_offset != submit->ccb_offset ||
	    submit->emit_ccb_write_offset != submit->ccb_write_offset ||
	    submit->emit_ccb_read_offset != submit->ccb_read_offset ||
	    submit->emit_ccb_service_address != submit->ccb_service_address ||
	    submit->emit_ccb_cache_control != submit->ccb_cache_control ||
	    submit->emit_ccb_data0 != submit->ccb_data0 ||
	    submit->emit_ccb_data1 != submit->ccb_data1 ||
	    submit->emit_queue_base_offset != submit->queue_base_offset ||
	    submit->emit_queue_size != submit->queue_size ||
	    submit->emit_queue_packet_size != submit->queue_packet_size ||
	    submit->emit_queue_packet_count != submit->queue_packet_count ||
	    submit->emit_queue_tail_packet_count !=
		    submit->queue_tail_packet_count ||
	    submit->emit_queue_head_packet_count !=
		    submit->queue_head_packet_count ||
	    submit->emit_sync_complete_seq != submit->sync_complete_seq ||
	    submit->emit_sync_pending_seq != submit->sync_pending_seq ||
	    submit->emit_sync_flags != submit->sync_flags ||
	    submit->emit_owner_issue_cookie != submit->owner_issue_cookie ||
	    submit->emit_owner_complete_cookie !=
		    submit->owner_complete_cookie ||
	    submit->emit_queue_epoch != submit->queue_epoch ||
	    submit->emit_global_issue_ticket != submit->global_issue_ticket)
		return false;

	if (submit->control_object_id) {
		control_obj = poulsbo_sgx_find_object_locked(
			dev_priv, submit->control_object_id);
		if (!control_obj)
			return false;
		control_gpu_va = poulsbo_sgx_object_gpu_va(dev_priv,
							    control_obj);
	}
	if (submit->pds_object_id) {
		pds_obj = poulsbo_sgx_find_object_locked(dev_priv,
							 submit->pds_object_id);
		if (!pds_obj)
			return false;
		pds_gpu_va = poulsbo_sgx_object_gpu_va(dev_priv, pds_obj);
	}
	if (submit->use_object_id) {
		use_obj = poulsbo_sgx_find_object_locked(dev_priv,
							 submit->use_object_id);
		if (!use_obj)
			return false;
		use_gpu_va = poulsbo_sgx_object_gpu_va(dev_priv, use_obj);
	}
	if (submit->target_object_id) {
		target_obj = poulsbo_sgx_find_object_locked(
			dev_priv, submit->target_object_id);
		if (!target_obj)
			return false;
		target_gpu_va = poulsbo_sgx_object_gpu_va(dev_priv,
							   target_obj);
	}
	if (submit->status_object_id) {
		status_obj = poulsbo_sgx_find_object_locked(
			dev_priv, submit->status_object_id);
		if (!status_obj)
			return false;
		status_gpu_va = poulsbo_sgx_object_gpu_va(dev_priv,
							   status_obj);
	}

	if (submit->emit_control_gpu_va != control_gpu_va ||
	    submit->emit_pds_gpu_va != pds_gpu_va ||
	    submit->emit_use_gpu_va != use_gpu_va ||
	    submit->emit_target_gpu_va != target_gpu_va ||
	    submit->emit_status_gpu_va != status_gpu_va)
		return false;

	active_count = poulsbo_sgx_submit_dependency_active_count_locked(submit);
	return submit->emit_dependency_active_count == active_count;
}

static bool
poulsbo_sgx_submit_launch_base_matches_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit)
{
	if (!submit->launch_valid || submit->launch_consumed)
		return false;
	if (!submit->emit_valid ||
	    !poulsbo_sgx_submit_emit_matches_locked(dev_priv, submit))
		return false;
	if (submit->launch_prepare_count != submit->prepare_count ||
	    submit->launch_arm_count != submit->arm_count ||
	    submit->launch_emit_count != submit->emit_count ||
	    submit->launch_context != submit->emit_context ||
	    submit->launch_cmd_type != submit->emit_cmd_type ||
	    submit->launch_ctrl_flags != submit->emit_ctrl_flags ||
	    submit->launch_num_ta_status_vals !=
		    submit->emit_num_ta_status_vals ||
	    submit->launch_num_3d_status_vals !=
		    submit->emit_num_3d_status_vals ||
	    submit->launch_ta_dependency != submit->emit_ta_dependency ||
	    submit->launch_control_object_id != submit->emit_control_object_id ||
	    submit->launch_pds_object_id != submit->emit_pds_object_id ||
	    submit->launch_use_object_id != submit->emit_use_object_id ||
	    submit->launch_target_object_id != submit->emit_target_object_id ||
	    submit->launch_status_object_id != submit->emit_status_object_id ||
	    submit->launch_ccb_offset != submit->emit_ccb_offset ||
	    submit->launch_ccb_write_offset != submit->emit_ccb_write_offset ||
	    submit->launch_ccb_read_offset != submit->emit_ccb_read_offset ||
	    submit->launch_ccb_service_address !=
		    submit->emit_ccb_service_address ||
	    submit->launch_ccb_cache_control !=
		    submit->emit_ccb_cache_control ||
	    submit->launch_ccb_data0 != submit->emit_ccb_data0 ||
	    submit->launch_ccb_data1 != submit->emit_ccb_data1 ||
	    submit->launch_queue_base_offset != submit->emit_queue_base_offset ||
	    submit->launch_queue_size != submit->emit_queue_size ||
	    submit->launch_queue_packet_size !=
		    submit->emit_queue_packet_size ||
	    submit->launch_sync_offset != submit->sync_offset ||
	    submit->launch_sync_complete_seq != submit->emit_sync_complete_seq ||
	    submit->launch_owner_offset != submit->owner_offset ||
	    submit->launch_owner_complete_cookie !=
		    submit->emit_owner_complete_cookie ||
	    submit->launch_global_running_ticket !=
		    submit->global_running_ticket ||
	    submit->launch_global_complete_ticket !=
		    submit->global_complete_ticket ||
	    submit->launch_control_gpu_va != submit->emit_control_gpu_va ||
	    submit->launch_pds_gpu_va != submit->emit_pds_gpu_va ||
	    submit->launch_use_gpu_va != submit->emit_use_gpu_va ||
	    submit->launch_target_gpu_va != submit->emit_target_gpu_va ||
	    submit->launch_status_gpu_va != submit->emit_status_gpu_va)
		return false;

	return true;
}

static bool poulsbo_sgx_submit_issue_plan_matches_launch_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_issue_plan *plan)
{
	if (!submit || !plan)
		return false;
	if (submit->launch_owner_state != plan->owner_state ||
	    submit->launch_issued_packets != plan->issued_packets ||
	    submit->launch_refilled_from_complete !=
		    (plan->refilled_from_complete ? 1 : 0) ||
	    submit->launch_new_write_offset != plan->new_write_offset ||
	    submit->launch_queue_packet_count != plan->new_packet_count ||
	    submit->launch_queue_tail_packet_count != plan->new_tail_packets ||
	    submit->launch_queue_head_packet_count != plan->new_head_packets ||
	    submit->launch_sync_pending_seq != plan->new_sync_pending_seq ||
	    submit->launch_sync_flags != plan->new_sync_flags ||
	    submit->launch_owner_issue_cookie != plan->new_owner_issue_cookie ||
	    submit->launch_queue_epoch != plan->new_queue_epoch ||
	    submit->launch_global_issue_ticket != plan->new_global_issue_ticket ||
	    submit->launch_dependency_active_count !=
		    submit->emit_dependency_active_count)
		return false;

	return true;
}

static const char *poulsbo_sgx_lab_submit_slot_name(u32 slot)
{
	switch (slot) {
	case DRM_PSB_SGX_LAB_SUBMIT_SLOT_CONTROL:
		return "control";
	case DRM_PSB_SGX_LAB_SUBMIT_SLOT_PDS:
		return "pds";
	case DRM_PSB_SGX_LAB_SUBMIT_SLOT_USE:
		return "use";
	case DRM_PSB_SGX_LAB_SUBMIT_SLOT_TARGET:
		return "target";
	case DRM_PSB_SGX_LAB_SUBMIT_SLOT_STATUS:
		return "status";
	default:
		return NULL;
	}
}

static u32 poulsbo_sgx_lab_status_value(u32 seed, bool is_3d, u32 index)
{
	return seed + (is_3d ? 0x200 : 0x100) + index;
}

static int poulsbo_sgx_status_region_end(u32 base_offset, u32 count, u32 stride,
					 u32 *end_out)
{
	u64 end;

	if (!count) {
		*end_out = base_offset;
		return 0;
	}
	if (!stride)
		return -EINVAL;

	end = (u64)base_offset + (u64)(count - 1) * stride + sizeof(u32);
	if (end > U32_MAX)
		return -EINVAL;

	*end_out = (u32)end;
	return 0;
}

static int poulsbo_sgx_queue_region_end(u32 base_offset, u32 queue_size,
					u32 *end_out)
{
	u64 end;

	if (!queue_size)
		return -EINVAL;

	end = (u64)base_offset + (u64)queue_size;
	if (end > U32_MAX)
		return -EINVAL;

	*end_out = (u32)end;
	return 0;
}

static int poulsbo_sgx_sync_region_end(u32 base_offset, u32 *end_out)
{
	u64 end;

	end = (u64)base_offset +
	      (u64)PSB_SGX_LAB_SYNC_DWORDS * sizeof(u32);
	if (end > U32_MAX)
		return -EINVAL;

	*end_out = (u32)end;
	return 0;
}

static int poulsbo_sgx_owner_region_end(u32 base_offset, u32 *end_out)
{
	u64 end;

	end = (u64)base_offset +
	      (u64)PSB_SGX_LAB_OWNER_DWORDS * sizeof(u32);
	if (end > U32_MAX)
		return -EINVAL;

	*end_out = (u32)end;
	return 0;
}

static int poulsbo_sgx_report_region_end(u32 base_offset, u32 *end_out)
{
	u64 end;

	end = (u64)base_offset +
	      (u64)PSB_SGX_LAB_REPORT_DWORDS * sizeof(u32);
	if (end > U32_MAX)
		return -EINVAL;

	*end_out = (u32)end;
	return 0;
}

static int poulsbo_sgx_live_region_end(u32 base_offset, u32 *end_out)
{
	u64 end;

	end = (u64)base_offset +
	      (u64)(PSB_SGX_LAB_LIVE_DWORDS * sizeof(u32));
	if (end > U32_MAX)
		return -EINVAL;

	*end_out = (u32)end;
	return 0;
}

static int poulsbo_sgx_manifest_region_end(u32 base_offset, u32 *end_out)
{
	u64 end;

	end = (u64)base_offset +
	      (u64)PSB_SGX_LAB_MANIFEST_DWORDS * sizeof(u32);
	if (end > U32_MAX)
		return -EINVAL;

	*end_out = (u32)end;
	return 0;
}

static int poulsbo_sgx_descriptor_region_end(u32 base_offset, u32 *end_out)
{
	u64 end;

	end = (u64)base_offset +
	      (u64)PSB_SGX_LAB_DESCRIPTOR_DWORDS * sizeof(u32);
	if (end > U32_MAX)
		return -EINVAL;

	*end_out = (u32)end;
	return 0;
}

static int poulsbo_sgx_plan_region_end(u32 base_offset, u32 *end_out)
{
	u64 end;

	end = (u64)base_offset +
	      (u64)PSB_SGX_LAB_PLAN_DWORDS * sizeof(u32);
	if (end > U32_MAX)
		return -EINVAL;

	*end_out = (u32)end;
	return 0;
}

static int poulsbo_sgx_issue_plan_region_end(u32 base_offset, u32 *end_out)
{
	u64 end;

	end = (u64)base_offset +
	      (u64)PSB_SGX_LAB_ISSUE_PLAN_DWORDS * sizeof(u32);
	if (end > U32_MAX)
		return -EINVAL;

	*end_out = (u32)end;
	return 0;
}

static int poulsbo_sgx_result_region_end(u32 base_offset, u32 *end_out)
{
	u64 end;

	end = (u64)base_offset +
	      (u64)PSB_SGX_LAB_RESULT_DWORDS * sizeof(u32);
	if (end > U32_MAX)
		return -EINVAL;

	*end_out = (u32)end;
	return 0;
}

static int poulsbo_sgx_dependency_region_end(u32 base_offset, u32 *end_out)
{
	u64 end;

	end = (u64)base_offset +
	      (u64)PSB_SGX_LAB_DEPENDENCY_DWORDS * sizeof(u32);
	if (end > U32_MAX)
		return -EINVAL;

	*end_out = (u32)end;
	return 0;
}

static bool poulsbo_sgx_regions_overlap(u32 first_start, u32 first_end,
					u32 second_start, u32 second_end)
{
	return first_start < second_end && second_start < first_end;
}

struct psb_sgx_lab_control_layout {
	bool queue_active;
	bool ccb_active;
	bool live_active;
	bool manifest_active;
	bool descriptor_active;
	bool plan_active;
	bool issue_plan_active;
	u32 queue_region_end;
	u32 ccb_region_end;
	u32 live_region_end;
	u32 manifest_region_end;
	u32 descriptor_region_end;
	u32 plan_region_end;
	u32 issue_plan_region_end;
};

static int poulsbo_sgx_ccb_region_end(u32 base_offset, u32 *end_out)
{
	u64 end;

	end = (u64)base_offset + (u64)PSB_SGX_LAB_CCB_RECORD_BYTES;
	if (end > U32_MAX)
		return -EINVAL;

	*end_out = (u32)end;
	return 0;
}

static int poulsbo_sgx_collect_submit_control_layout_locked(
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_control_layout *layout)
{
	memset(layout, 0, sizeof(*layout));

	if (submit->queue_config_count) {
		int ret;

		ret = poulsbo_sgx_queue_region_end(submit->queue_base_offset,
						   submit->queue_size,
						   &layout->queue_region_end);
		if (ret)
			return ret;
		layout->queue_active = true;
		if (poulsbo_sgx_regions_overlap(
			    submit->queue_base_offset, layout->queue_region_end,
			    0, PSB_SGX_LAB_CONTROL_QUEUE_HEADER_BYTES))
			return -EINVAL;
	}

	if (submit->packet_config_count || submit->ccb_config_count) {
		int ret;

		if (submit->ccb_offset & 0xf)
			return -EINVAL;
		ret = poulsbo_sgx_ccb_region_end(submit->ccb_offset,
						 &layout->ccb_region_end);
		if (ret)
			return ret;
		layout->ccb_active = true;
		if (poulsbo_sgx_regions_overlap(
			    submit->ccb_offset, layout->ccb_region_end, 0,
			    PSB_SGX_LAB_CONTROL_QUEUE_HEADER_BYTES))
			return -EINVAL;
	}

	if (layout->queue_active && layout->ccb_active &&
	    (submit->ccb_offset < submit->queue_base_offset ||
	     layout->ccb_region_end > layout->queue_region_end))
		return -EINVAL;

	if (submit->live_config_count) {
		int ret;

		ret = poulsbo_sgx_live_region_end(submit->live_offset,
						  &layout->live_region_end);
		if (ret)
			return ret;
		layout->live_active = true;
		if (poulsbo_sgx_regions_overlap(submit->live_offset,
						layout->live_region_end,
						0,
						PSB_SGX_LAB_CONTROL_QUEUE_HEADER_BYTES))
			return -EINVAL;
		if (layout->queue_active &&
		    poulsbo_sgx_regions_overlap(submit->live_offset,
						layout->live_region_end,
						submit->queue_base_offset,
						layout->queue_region_end))
			return -EINVAL;
		if (layout->ccb_active &&
		    poulsbo_sgx_regions_overlap(submit->live_offset,
						layout->live_region_end,
						submit->ccb_offset,
						layout->ccb_region_end))
			return -EINVAL;
	}

	if (submit->manifest_config_count) {
		int ret;

		ret = poulsbo_sgx_manifest_region_end(submit->manifest_offset,
						      &layout->manifest_region_end);
		if (ret)
			return ret;
		layout->manifest_active = true;
		if (poulsbo_sgx_regions_overlap(
			    submit->manifest_offset, layout->manifest_region_end,
			    0, PSB_SGX_LAB_CONTROL_QUEUE_HEADER_BYTES))
			return -EINVAL;
		if (layout->queue_active &&
		    poulsbo_sgx_regions_overlap(
			    submit->manifest_offset, layout->manifest_region_end,
			    submit->queue_base_offset, layout->queue_region_end))
			return -EINVAL;
		if (layout->ccb_active &&
		    poulsbo_sgx_regions_overlap(submit->manifest_offset,
						layout->manifest_region_end,
						submit->ccb_offset,
						layout->ccb_region_end))
			return -EINVAL;
		if (layout->live_active &&
		    poulsbo_sgx_regions_overlap(submit->manifest_offset,
						layout->manifest_region_end,
						submit->live_offset,
						layout->live_region_end))
			return -EINVAL;
	}

	if (submit->descriptor_config_count) {
		int ret;

		ret = poulsbo_sgx_descriptor_region_end(
			submit->descriptor_offset,
			&layout->descriptor_region_end);
		if (ret)
			return ret;
		layout->descriptor_active = true;
		if (poulsbo_sgx_regions_overlap(
			    submit->descriptor_offset,
			    layout->descriptor_region_end, 0,
			    PSB_SGX_LAB_CONTROL_QUEUE_HEADER_BYTES))
			return -EINVAL;
		if (layout->queue_active &&
		    poulsbo_sgx_regions_overlap(
			    submit->descriptor_offset,
			    layout->descriptor_region_end,
			    submit->queue_base_offset, layout->queue_region_end))
			return -EINVAL;
		if (layout->ccb_active &&
		    poulsbo_sgx_regions_overlap(submit->descriptor_offset,
						layout->descriptor_region_end,
						submit->ccb_offset,
						layout->ccb_region_end))
			return -EINVAL;
		if (layout->manifest_active &&
		    poulsbo_sgx_regions_overlap(
			    submit->descriptor_offset,
			    layout->descriptor_region_end,
			    submit->manifest_offset,
			    layout->manifest_region_end))
			return -EINVAL;
		if (layout->live_active &&
		    poulsbo_sgx_regions_overlap(submit->descriptor_offset,
						layout->descriptor_region_end,
						submit->live_offset,
						layout->live_region_end))
			return -EINVAL;
	}

	if (submit->plan_config_count) {
		int ret;

		ret = poulsbo_sgx_plan_region_end(submit->plan_offset,
						  &layout->plan_region_end);
		if (ret)
			return ret;
		layout->plan_active = true;
		if (poulsbo_sgx_regions_overlap(
			    submit->plan_offset, layout->plan_region_end, 0,
			    PSB_SGX_LAB_CONTROL_QUEUE_HEADER_BYTES))
			return -EINVAL;
		if (layout->queue_active &&
		    poulsbo_sgx_regions_overlap(
			    submit->plan_offset, layout->plan_region_end,
			    submit->queue_base_offset, layout->queue_region_end))
			return -EINVAL;
		if (layout->ccb_active &&
		    poulsbo_sgx_regions_overlap(submit->plan_offset,
						layout->plan_region_end,
						submit->ccb_offset,
						layout->ccb_region_end))
			return -EINVAL;
		if (layout->manifest_active &&
		    poulsbo_sgx_regions_overlap(submit->plan_offset,
						layout->plan_region_end,
						submit->manifest_offset,
						layout->manifest_region_end))
			return -EINVAL;
		if (layout->descriptor_active &&
		    poulsbo_sgx_regions_overlap(submit->plan_offset,
						layout->plan_region_end,
						submit->descriptor_offset,
						layout->descriptor_region_end))
			return -EINVAL;
		if (layout->live_active &&
		    poulsbo_sgx_regions_overlap(submit->plan_offset,
						layout->plan_region_end,
						submit->live_offset,
						layout->live_region_end))
			return -EINVAL;
	}

	if (submit->issue_plan_config_count) {
		int ret;

		ret = poulsbo_sgx_issue_plan_region_end(
			submit->issue_plan_offset,
			&layout->issue_plan_region_end);
		if (ret)
			return ret;
		layout->issue_plan_active = true;
		if (poulsbo_sgx_regions_overlap(
			    submit->issue_plan_offset,
			    layout->issue_plan_region_end, 0,
			    PSB_SGX_LAB_CONTROL_QUEUE_HEADER_BYTES))
			return -EINVAL;
		if (layout->queue_active &&
		    poulsbo_sgx_regions_overlap(
			    submit->issue_plan_offset,
			    layout->issue_plan_region_end,
			    submit->queue_base_offset,
			    layout->queue_region_end))
			return -EINVAL;
		if (layout->ccb_active &&
		    poulsbo_sgx_regions_overlap(
			    submit->issue_plan_offset,
			    layout->issue_plan_region_end,
			    submit->ccb_offset, layout->ccb_region_end))
			return -EINVAL;
		if (layout->manifest_active &&
		    poulsbo_sgx_regions_overlap(
			    submit->issue_plan_offset,
			    layout->issue_plan_region_end,
			    submit->manifest_offset,
			    layout->manifest_region_end))
			return -EINVAL;
		if (layout->descriptor_active &&
		    poulsbo_sgx_regions_overlap(
			    submit->issue_plan_offset,
			    layout->issue_plan_region_end,
			    submit->descriptor_offset,
			    layout->descriptor_region_end))
			return -EINVAL;
		if (layout->plan_active &&
		    poulsbo_sgx_regions_overlap(
			    submit->issue_plan_offset,
			    layout->issue_plan_region_end, submit->plan_offset,
			    layout->plan_region_end))
			return -EINVAL;
		if (layout->live_active &&
		    poulsbo_sgx_regions_overlap(
			    submit->issue_plan_offset,
			    layout->issue_plan_region_end, submit->live_offset,
			    layout->live_region_end))
			return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_validate_submit_control_layout_access_locked(
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	const struct psb_sgx_lab_control_layout *layout)
{
	int ret;

	if (!control_obj)
		return -EINVAL;

	if (layout->queue_active) {
		ret = poulsbo_sgx_object_access_ok(
			control_obj, submit->queue_base_offset,
			layout->queue_region_end - submit->queue_base_offset);
		if (ret)
			return ret;
	}

	if (layout->ccb_active) {
		ret = poulsbo_sgx_object_access_ok(
			control_obj, submit->ccb_offset,
			layout->ccb_region_end - submit->ccb_offset);
		if (ret)
			return ret;
	}

	if (layout->live_active) {
		ret = poulsbo_sgx_object_access_ok(
			control_obj, submit->live_offset,
			layout->live_region_end - submit->live_offset);
		if (ret)
			return ret;
	}

	if (layout->manifest_active) {
		ret = poulsbo_sgx_object_access_ok(
			control_obj, submit->manifest_offset,
			layout->manifest_region_end - submit->manifest_offset);
		if (ret)
			return ret;
	}

	if (layout->descriptor_active) {
		ret = poulsbo_sgx_object_access_ok(
			control_obj, submit->descriptor_offset,
			layout->descriptor_region_end -
				submit->descriptor_offset);
		if (ret)
			return ret;
	}

	if (layout->plan_active) {
		ret = poulsbo_sgx_object_access_ok(
			control_obj, submit->plan_offset,
			layout->plan_region_end - submit->plan_offset);
		if (ret)
			return ret;
	}

	if (layout->issue_plan_active) {
		ret = poulsbo_sgx_object_access_ok(
			control_obj, submit->issue_plan_offset,
			layout->issue_plan_region_end -
				submit->issue_plan_offset);
		if (ret)
			return ret;
	}

	return 0;
}

static int poulsbo_sgx_validate_submit_control_layout_locked(
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_control_layout *layout_out)
{
	struct psb_sgx_lab_control_layout layout;
	int ret;

	ret = poulsbo_sgx_collect_submit_control_layout_locked(submit, &layout);
	if (ret)
		return ret;

	ret = poulsbo_sgx_validate_submit_control_layout_access_locked(
		submit, control_obj, &layout);
	if (ret)
		return ret;

	if (layout_out)
		*layout_out = layout;

	return 0;
}

struct psb_sgx_lab_status_layout {
	bool ta_active;
	bool threed_active;
	bool sync_active;
	bool owner_active;
	bool report_active;
	bool result_active;
	bool dependency_active[PSB_SGX_LAB_DEPENDENCY_SLOTS];
	u32 ta_region_end;
	u32 threed_region_end;
	u32 sync_region_end;
	u32 owner_region_end;
	u32 report_region_end;
	u32 result_region_end;
	u32 dependency_region_end[PSB_SGX_LAB_DEPENDENCY_SLOTS];
};

static int poulsbo_sgx_collect_submit_status_layout_locked(
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_status_layout *layout)
{
	unsigned int slot;

	memset(layout, 0, sizeof(*layout));

	if (submit->num_ta_status_vals) {
		int ret;

		ret = poulsbo_sgx_status_region_end(submit->status_ta_offset,
						    submit->num_ta_status_vals,
						    submit->status_stride,
						    &layout->ta_region_end);
		if (ret)
			return ret;
		layout->ta_active = true;
	}

	if (submit->num_3d_status_vals) {
		int ret;

		ret = poulsbo_sgx_status_region_end(submit->status_3d_offset,
						    submit->num_3d_status_vals,
						    submit->status_stride,
						    &layout->threed_region_end);
		if (ret)
			return ret;
		layout->threed_active = true;
	}

	if (layout->ta_active && layout->threed_active &&
	    poulsbo_sgx_regions_overlap(submit->status_ta_offset,
					layout->ta_region_end,
					submit->status_3d_offset,
					layout->threed_region_end))
		return -EINVAL;

	if (submit->sync_config_count) {
		int ret;

		ret = poulsbo_sgx_sync_region_end(submit->sync_offset,
						  &layout->sync_region_end);
		if (ret)
			return ret;
		layout->sync_active = true;
		if (layout->ta_active &&
		    poulsbo_sgx_regions_overlap(submit->sync_offset,
						layout->sync_region_end,
						submit->status_ta_offset,
						layout->ta_region_end))
			return -EINVAL;
		if (layout->threed_active &&
		    poulsbo_sgx_regions_overlap(submit->sync_offset,
						layout->sync_region_end,
						submit->status_3d_offset,
						layout->threed_region_end))
			return -EINVAL;
	}

	if (submit->owner_config_count) {
		int ret;

		ret = poulsbo_sgx_owner_region_end(submit->owner_offset,
						   &layout->owner_region_end);
		if (ret)
			return ret;
		layout->owner_active = true;
		if (layout->sync_active &&
		    poulsbo_sgx_regions_overlap(submit->owner_offset,
						layout->owner_region_end,
						submit->sync_offset,
						layout->sync_region_end))
			return -EINVAL;
		if (layout->ta_active &&
		    poulsbo_sgx_regions_overlap(submit->owner_offset,
						layout->owner_region_end,
						submit->status_ta_offset,
						layout->ta_region_end))
			return -EINVAL;
		if (layout->threed_active &&
		    poulsbo_sgx_regions_overlap(submit->owner_offset,
						layout->owner_region_end,
						submit->status_3d_offset,
						layout->threed_region_end))
			return -EINVAL;
	}

	if (submit->report_config_count) {
		int ret;

		ret = poulsbo_sgx_report_region_end(submit->report_offset,
						    &layout->report_region_end);
		if (ret)
			return ret;
		layout->report_active = true;
		if (layout->sync_active &&
		    poulsbo_sgx_regions_overlap(submit->report_offset,
						layout->report_region_end,
						submit->sync_offset,
						layout->sync_region_end))
			return -EINVAL;
		if (layout->owner_active &&
		    poulsbo_sgx_regions_overlap(submit->report_offset,
						layout->report_region_end,
						submit->owner_offset,
						layout->owner_region_end))
			return -EINVAL;
		if (layout->ta_active &&
		    poulsbo_sgx_regions_overlap(submit->report_offset,
						layout->report_region_end,
						submit->status_ta_offset,
						layout->ta_region_end))
			return -EINVAL;
		if (layout->threed_active &&
		    poulsbo_sgx_regions_overlap(submit->report_offset,
						layout->report_region_end,
						submit->status_3d_offset,
						layout->threed_region_end))
			return -EINVAL;
	}

	if (submit->result_config_count) {
		int ret;

		ret = poulsbo_sgx_result_region_end(submit->result_offset,
						    &layout->result_region_end);
		if (ret)
			return ret;
		layout->result_active = true;
		if (layout->sync_active &&
		    poulsbo_sgx_regions_overlap(submit->result_offset,
						layout->result_region_end,
						submit->sync_offset,
						layout->sync_region_end))
			return -EINVAL;
		if (layout->owner_active &&
		    poulsbo_sgx_regions_overlap(submit->result_offset,
						layout->result_region_end,
						submit->owner_offset,
						layout->owner_region_end))
			return -EINVAL;
		if (layout->report_active &&
		    poulsbo_sgx_regions_overlap(submit->result_offset,
						layout->result_region_end,
						submit->report_offset,
						layout->report_region_end))
			return -EINVAL;
		if (layout->ta_active &&
		    poulsbo_sgx_regions_overlap(submit->result_offset,
						layout->result_region_end,
						submit->status_ta_offset,
						layout->ta_region_end))
			return -EINVAL;
		if (layout->threed_active &&
		    poulsbo_sgx_regions_overlap(submit->result_offset,
						layout->result_region_end,
						submit->status_3d_offset,
						layout->threed_region_end))
			return -EINVAL;
	}

	for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
		u32 *dependency_offset =
			poulsbo_sgx_dependency_offset_ptr(submit, slot);
		u32 *dependency_config_count =
			poulsbo_sgx_dependency_config_count_ptr(submit, slot);
		unsigned int other_slot;
		int ret;

		if (!dependency_offset || !dependency_config_count)
			return -EINVAL;
		if (!*dependency_config_count)
			continue;
		ret = poulsbo_sgx_dependency_region_end(
			*dependency_offset,
			&layout->dependency_region_end[slot]);
		if (ret)
			return ret;
		layout->dependency_active[slot] = true;
		if (layout->sync_active &&
		    poulsbo_sgx_regions_overlap(
			    *dependency_offset,
			    layout->dependency_region_end[slot],
			    submit->sync_offset, layout->sync_region_end))
			return -EINVAL;
		if (layout->owner_active &&
		    poulsbo_sgx_regions_overlap(
			    *dependency_offset,
			    layout->dependency_region_end[slot],
			    submit->owner_offset, layout->owner_region_end))
			return -EINVAL;
		if (layout->report_active &&
		    poulsbo_sgx_regions_overlap(
			    *dependency_offset,
			    layout->dependency_region_end[slot],
			    submit->report_offset, layout->report_region_end))
			return -EINVAL;
		if (layout->result_active &&
		    poulsbo_sgx_regions_overlap(
			    *dependency_offset,
			    layout->dependency_region_end[slot],
			    submit->result_offset, layout->result_region_end))
			return -EINVAL;
		if (layout->ta_active &&
		    poulsbo_sgx_regions_overlap(
			    *dependency_offset,
			    layout->dependency_region_end[slot],
			    submit->status_ta_offset, layout->ta_region_end))
			return -EINVAL;
		if (layout->threed_active &&
		    poulsbo_sgx_regions_overlap(
			    *dependency_offset,
			    layout->dependency_region_end[slot],
			    submit->status_3d_offset,
			    layout->threed_region_end))
			return -EINVAL;
		for (other_slot = slot + 1;
		     other_slot < PSB_SGX_LAB_DEPENDENCY_SLOTS;
		     other_slot++) {
			u32 *other_dependency_offset =
				poulsbo_sgx_dependency_offset_ptr(submit,
								   other_slot);
			u32 *other_dependency_config_count =
				poulsbo_sgx_dependency_config_count_ptr(
					submit, other_slot);
			u32 other_dependency_region_end;

			if (!other_dependency_offset ||
			    !other_dependency_config_count)
				return -EINVAL;
			if (!*other_dependency_config_count)
				continue;
			ret = poulsbo_sgx_dependency_region_end(
				*other_dependency_offset,
				&other_dependency_region_end);
			if (ret)
				return ret;
			if (poulsbo_sgx_regions_overlap(
				    *dependency_offset,
				    layout->dependency_region_end[slot],
				    *other_dependency_offset,
				    other_dependency_region_end))
				return -EINVAL;
		}
	}

	return 0;
}

static int poulsbo_sgx_validate_submit_status_layout_access_locked(
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *status_obj,
	const struct psb_sgx_lab_status_layout *layout)
{
	unsigned int slot;
	int ret;
	bool any_active = false;

	any_active = layout->ta_active || layout->threed_active ||
		     layout->sync_active || layout->owner_active ||
		     layout->report_active || layout->result_active;
	for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS && !any_active;
	     slot++)
		any_active = layout->dependency_active[slot];

	if (!any_active)
		return 0;
	if (!status_obj)
		return -EINVAL;

	if (layout->ta_active) {
		ret = poulsbo_sgx_object_access_ok(
			status_obj, submit->status_ta_offset,
			layout->ta_region_end - submit->status_ta_offset);
		if (ret)
			return ret;
	}
	if (layout->threed_active) {
		ret = poulsbo_sgx_object_access_ok(
			status_obj, submit->status_3d_offset,
			layout->threed_region_end - submit->status_3d_offset);
		if (ret)
			return ret;
	}
	if (layout->sync_active) {
		ret = poulsbo_sgx_object_access_ok(
			status_obj, submit->sync_offset,
			layout->sync_region_end - submit->sync_offset);
		if (ret)
			return ret;
	}
	if (layout->owner_active) {
		ret = poulsbo_sgx_object_access_ok(
			status_obj, submit->owner_offset,
			layout->owner_region_end - submit->owner_offset);
		if (ret)
			return ret;
	}
	if (layout->report_active) {
		ret = poulsbo_sgx_object_access_ok(
			status_obj, submit->report_offset,
			layout->report_region_end - submit->report_offset);
		if (ret)
			return ret;
	}
	if (layout->result_active) {
		ret = poulsbo_sgx_object_access_ok(
			status_obj, submit->result_offset,
			layout->result_region_end - submit->result_offset);
		if (ret)
			return ret;
	}
	for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
		u32 *dependency_offset;

		if (!layout->dependency_active[slot])
			continue;
		dependency_offset =
			poulsbo_sgx_dependency_offset_ptr(submit, slot);
		if (!dependency_offset)
			return -EINVAL;
		ret = poulsbo_sgx_object_access_ok(
			status_obj, *dependency_offset,
			layout->dependency_region_end[slot] - *dependency_offset);
		if (ret)
			return ret;
	}

	return 0;
}

static int poulsbo_sgx_validate_submit_status_layout_locked(
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *status_obj,
	struct psb_sgx_lab_status_layout *layout_out)
{
	struct psb_sgx_lab_status_layout layout;
	int ret;

	ret = poulsbo_sgx_collect_submit_status_layout_locked(submit, &layout);
	if (ret)
		return ret;

	ret = poulsbo_sgx_validate_submit_status_layout_access_locked(
		submit, status_obj, &layout);
	if (ret)
		return ret;

	if (layout_out)
		*layout_out = layout;

	return 0;
}

static u32 *poulsbo_sgx_dependency_submit_id_ptr(
	struct psb_sgx_lab_submit *submit, unsigned int slot)
{
	switch (slot) {
	case 0:
		return &submit->dependency_submit_id;
	case 1:
		return &submit->dependency_submit_id_secondary;
	case 2:
		return &submit->dependency_submit_id_tertiary;
	case 3:
		return &submit->dependency_submit_id_quaternary;
	default:
		return NULL;
	}
}

static u32 *poulsbo_sgx_dependency_offset_ptr(struct psb_sgx_lab_submit *submit,
					      unsigned int slot)
{
	switch (slot) {
	case 0:
		return &submit->dependency_offset;
	case 1:
		return &submit->dependency_offset_secondary;
	case 2:
		return &submit->dependency_offset_tertiary;
	case 3:
		return &submit->dependency_offset_quaternary;
	default:
		return NULL;
	}
}

static u32 *poulsbo_sgx_dependency_snapshot_source_submit_id_ptr(
	struct psb_sgx_lab_submit *submit, unsigned int slot)
{
	switch (slot) {
	case 0:
		return &submit->dependency_snapshot_source_submit_id;
	case 1:
		return &submit->dependency_snapshot_source_submit_id_secondary;
	case 2:
		return &submit->dependency_snapshot_source_submit_id_tertiary;
	case 3:
		return &submit->dependency_snapshot_source_submit_id_quaternary;
	default:
		return NULL;
	}
}

static u32 *poulsbo_sgx_dependency_snapshot_required_ticket_ptr(
	struct psb_sgx_lab_submit *submit, unsigned int slot)
{
	switch (slot) {
	case 0:
		return &submit->dependency_snapshot_required_ticket;
	case 1:
		return &submit->dependency_snapshot_required_ticket_secondary;
	case 2:
		return &submit->dependency_snapshot_required_ticket_tertiary;
	case 3:
		return &submit->dependency_snapshot_required_ticket_quaternary;
	default:
		return NULL;
	}
}

static u32 *poulsbo_sgx_dependency_snapshot_observed_complete_ticket_ptr(
	struct psb_sgx_lab_submit *submit, unsigned int slot)
{
	switch (slot) {
	case 0:
		return &submit->dependency_snapshot_observed_complete_ticket;
	case 1:
		return &submit->dependency_snapshot_observed_complete_ticket_secondary;
	case 2:
		return &submit->dependency_snapshot_observed_complete_ticket_tertiary;
	case 3:
		return &submit->dependency_snapshot_observed_complete_ticket_quaternary;
	default:
		return NULL;
	}
}

static u32 *poulsbo_sgx_dependency_snapshot_flags_ptr(
	struct psb_sgx_lab_submit *submit, unsigned int slot)
{
	switch (slot) {
	case 0:
		return &submit->dependency_snapshot_flags;
	case 1:
		return &submit->dependency_snapshot_flags_secondary;
	case 2:
		return &submit->dependency_snapshot_flags_tertiary;
	case 3:
		return &submit->dependency_snapshot_flags_quaternary;
	default:
		return NULL;
	}
}

static u32 *poulsbo_sgx_dependency_config_count_ptr(
	struct psb_sgx_lab_submit *submit, unsigned int slot)
{
	switch (slot) {
	case 0:
		return &submit->dependency_config_count;
	case 1:
		return &submit->dependency_config_count_secondary;
	case 2:
		return &submit->dependency_config_count_tertiary;
	case 3:
		return &submit->dependency_config_count_quaternary;
	default:
		return NULL;
	}
}

static int poulsbo_sgx_store_submit_dependency_snapshot_locked(
	struct psb_sgx_lab_submit *submit,
	unsigned int slot,
	const struct psb_sgx_lab_dependency_words *dependency)
{
	u32 *dependency_snapshot_source_submit_id =
		poulsbo_sgx_dependency_snapshot_source_submit_id_ptr(submit,
								      slot);
	u32 *dependency_snapshot_required_ticket =
		poulsbo_sgx_dependency_snapshot_required_ticket_ptr(submit,
								   slot);
	u32 *dependency_snapshot_observed_complete_ticket =
		poulsbo_sgx_dependency_snapshot_observed_complete_ticket_ptr(
			submit, slot);
	u32 *dependency_snapshot_flags =
		poulsbo_sgx_dependency_snapshot_flags_ptr(submit, slot);

	if (!dependency_snapshot_source_submit_id ||
	    !dependency_snapshot_required_ticket ||
	    !dependency_snapshot_observed_complete_ticket ||
	    !dependency_snapshot_flags || !dependency)
		return -EINVAL;

	*dependency_snapshot_source_submit_id =
		dependency->source_submit_id;
	*dependency_snapshot_required_ticket =
		dependency->required_ticket;
	*dependency_snapshot_observed_complete_ticket =
		dependency->observed_complete_ticket;
	*dependency_snapshot_flags = dependency->flags;
	return 0;
}

static int poulsbo_sgx_load_submit_dependency_snapshot_locked(
	struct psb_sgx_lab_submit *submit,
	unsigned int slot,
	struct psb_sgx_lab_dependency_words *dependency)
{
	u32 *dependency_snapshot_source_submit_id =
		poulsbo_sgx_dependency_snapshot_source_submit_id_ptr(submit,
								      slot);
	u32 *dependency_snapshot_required_ticket =
		poulsbo_sgx_dependency_snapshot_required_ticket_ptr(submit,
								   slot);
	u32 *dependency_snapshot_observed_complete_ticket =
		poulsbo_sgx_dependency_snapshot_observed_complete_ticket_ptr(
			submit, slot);
	u32 *dependency_snapshot_flags =
		poulsbo_sgx_dependency_snapshot_flags_ptr(submit, slot);

	if (!dependency_snapshot_source_submit_id ||
	    !dependency_snapshot_required_ticket ||
	    !dependency_snapshot_observed_complete_ticket ||
	    !dependency_snapshot_flags || !dependency)
		return -EINVAL;

	memset(dependency, 0, sizeof(*dependency));
	dependency->source_submit_id =
		*dependency_snapshot_source_submit_id;
	dependency->required_ticket =
		*dependency_snapshot_required_ticket;
	dependency->observed_complete_ticket =
		*dependency_snapshot_observed_complete_ticket;
	dependency->flags = *dependency_snapshot_flags;
	return 0;
}

static unsigned long *poulsbo_sgx_last_dependency_jiffies_ptr(
	struct psb_sgx_lab_submit *submit, unsigned int slot)
{
	switch (slot) {
	case 0:
		return &submit->last_dependency_jiffies;
	case 1:
		return &submit->last_dependency_secondary_jiffies;
	case 2:
		return &submit->last_dependency_tertiary_jiffies;
	case 3:
		return &submit->last_dependency_quaternary_jiffies;
	default:
		return NULL;
	}
}

static unsigned long *poulsbo_sgx_last_dependency_block_jiffies_ptr(
	struct psb_sgx_lab_submit *submit, unsigned int slot)
{
	switch (slot) {
	case 0:
		return &submit->last_dependency_block_jiffies;
	case 1:
		return &submit->last_dependency_block_secondary_jiffies;
	case 2:
		return &submit->last_dependency_block_tertiary_jiffies;
	case 3:
		return &submit->last_dependency_block_quaternary_jiffies;
	default:
		return NULL;
	}
}

static u32 poulsbo_sgx_dependency_required_ticket(struct psb_sgx_lab_submit *submit,
						  unsigned int slot)
{
	switch (slot) {
	case 0:
		return submit->ta_dependency;
	case 1:
		if (submit->dependency_ticket_secondary)
			return submit->dependency_ticket_secondary;
		return submit->ta_dependency;
	case 2:
		if (submit->dependency_ticket_tertiary)
			return submit->dependency_ticket_tertiary;
		return submit->ta_dependency;
	case 3:
		if (submit->dependency_ticket_quaternary)
			return submit->dependency_ticket_quaternary;
		return submit->ta_dependency;
	default:
		return 0;
	}
}

static bool poulsbo_sgx_dependency_slot_active(struct psb_sgx_lab_submit *submit,
					       unsigned int slot)
{
	u32 *dependency_submit_id = poulsbo_sgx_dependency_submit_id_ptr(submit,
									 slot);
	u32 *dependency_config_count =
		poulsbo_sgx_dependency_config_count_ptr(submit, slot);

	if (!dependency_submit_id || !dependency_config_count)
		return false;
	if (slot == 0 && submit->ta_dependency)
		return true;
	if (slot == 1 && submit->dependency_ticket_secondary)
		return true;
	if (slot == 2 && submit->dependency_ticket_tertiary)
		return true;
	if (slot == 3 && submit->dependency_ticket_quaternary)
		return true;

	return *dependency_submit_id || *dependency_config_count;
}

static int poulsbo_sgx_dependency_snapshot_slot_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	unsigned int slot,
	u32 *source_submit_id_out,
	u32 *required_ticket_out,
	u32 *observed_complete_ticket_out,
	u32 *flags_out)
{
	struct psb_sgx_lab_submit *dependency_submit = NULL;
	u32 *dependency_submit_id = poulsbo_sgx_dependency_submit_id_ptr(submit,
									 slot);
	u32 required_ticket =
		poulsbo_sgx_dependency_required_ticket(submit, slot);
	u32 observed_complete_ticket = dev_priv->sgx_lab_global_complete_ticket;
	u32 flags = PSB_SGX_LAB_DEPENDENCY_FLAG_READY;
	u32 source_submit_id;

	if (!dependency_submit_id)
		return -EINVAL;
	source_submit_id = *dependency_submit_id;

	if (source_submit_id) {
		dependency_submit = poulsbo_sgx_find_submit_locked(
			dev_priv, source_submit_id);
		flags |= PSB_SGX_LAB_DEPENDENCY_FLAG_SOURCE_BOUND;
		if (dependency_submit)
			observed_complete_ticket =
				dependency_submit->global_complete_ticket;
		else
			flags |=
				PSB_SGX_LAB_DEPENDENCY_FLAG_SOURCE_MISSING;
	}
	if (required_ticket)
		flags |= PSB_SGX_LAB_DEPENDENCY_FLAG_GATED;
	if (!required_ticket || observed_complete_ticket >= required_ticket)
		flags |= PSB_SGX_LAB_DEPENDENCY_FLAG_SATISFIED;

	if (source_submit_id_out)
		*source_submit_id_out = source_submit_id;
	if (required_ticket_out)
		*required_ticket_out = required_ticket;
	if (observed_complete_ticket_out)
		*observed_complete_ticket_out = observed_complete_ticket;
	if (flags_out)
		*flags_out = flags;

	return 0;
}

static int poulsbo_sgx_build_submit_dependency_slot_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	unsigned int slot,
	struct psb_sgx_lab_dependency_words *dependency)
{
	int ret;

	if (!dependency)
		return -EINVAL;

	memset(dependency, 0, sizeof(*dependency));
	ret = poulsbo_sgx_dependency_snapshot_slot_locked(
		dev_priv, submit, slot, &dependency->source_submit_id,
		&dependency->required_ticket,
		&dependency->observed_complete_ticket, &dependency->flags);
	if (ret)
		return ret;

	return 0;
}

static int poulsbo_sgx_write_submit_dependency_block_locked(
	struct psb_sgx_lab_object *status_obj,
	u32 dependency_offset,
	const struct psb_sgx_lab_dependency_words *dependency)
{
	return poulsbo_sgx_object_write_words_locked(
		status_obj, dependency_offset, (const u32 *)dependency,
		sizeof(*dependency) / sizeof(u32));
}

static int poulsbo_sgx_read_submit_dependency_block_locked(
	struct psb_sgx_lab_object *status_obj,
	u32 dependency_offset,
	struct psb_sgx_lab_dependency_words *dependency)
{
	if (!dependency)
		return -EINVAL;

	memset(dependency, 0, sizeof(*dependency));
	return poulsbo_sgx_object_read_words_locked(
		status_obj, dependency_offset, (u32 *)dependency,
		sizeof(*dependency) / sizeof(u32));
}

static int poulsbo_sgx_refresh_dependency_block_slot_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	unsigned int slot)
{
	struct psb_sgx_lab_object *status_obj;
	u32 *dependency_offset = poulsbo_sgx_dependency_offset_ptr(submit, slot);
	u32 *dependency_config_count =
		poulsbo_sgx_dependency_config_count_ptr(submit, slot);
	unsigned long *last_dependency_block_jiffies =
		poulsbo_sgx_last_dependency_block_jiffies_ptr(submit, slot);
	struct psb_sgx_lab_dependency_words dependency;
	int ret;

	if (!dependency_offset || !dependency_config_count ||
	    !last_dependency_block_jiffies)
		return -EINVAL;
	if (!*dependency_config_count)
		return 0;
	if (!submit->status_object_id ||
	    !poulsbo_sgx_dependency_required_ticket(submit, slot))
		return -EINVAL;

	ret = poulsbo_sgx_build_submit_dependency_slot_locked(
		dev_priv, submit, slot, &dependency);
	if (ret)
		return ret;

	status_obj = poulsbo_sgx_find_object_locked(dev_priv,
						    submit->status_object_id);
	if (!status_obj)
		return -ENOENT;
	ret = poulsbo_sgx_validate_submit_status_layout_locked(
		submit, status_obj, NULL);
	if (ret)
		return ret;

	ret = poulsbo_sgx_write_submit_dependency_block_locked(
		status_obj, *dependency_offset, &dependency);
	if (ret)
		return ret;

	ret = poulsbo_sgx_store_submit_dependency_snapshot_locked(
		submit, slot, &dependency);
	if (ret)
		return ret;
	*last_dependency_block_jiffies = jiffies;

	return 0;
}

static int poulsbo_sgx_refresh_dependency_blocks_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit)
{
	unsigned int slot;
	int ret;

	for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
		ret = poulsbo_sgx_refresh_dependency_block_slot_locked(
			dev_priv, submit, slot);
		if (ret)
			return ret;
	}

	return 0;
}

static int poulsbo_sgx_refresh_dependency_dependents_locked(
	struct drm_psb_private *dev_priv,
	u32 completed_submit_id)
{
	struct psb_sgx_lab_submit *submit;
	unsigned int slot;
	int ret;

	list_for_each_entry(submit, &dev_priv->sgx_lab_submit_list, head) {
		bool matched = false;
		bool any_active = false;

		for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
			if (!poulsbo_sgx_dependency_slot_active(submit, slot))
				continue;
			any_active = true;
			break;
		}
		if (!any_active)
			continue;
		if (!completed_submit_id) {
			continue;
		}
		for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
			u32 *dependency_submit_id;
			u32 *dependency_config_count;

			dependency_submit_id =
				poulsbo_sgx_dependency_submit_id_ptr(submit,
								      slot);
			dependency_config_count =
				poulsbo_sgx_dependency_config_count_ptr(
					submit, slot);
			if (!dependency_submit_id || !dependency_config_count)
				continue;
			if (!*dependency_config_count && !*dependency_submit_id)
				continue;
			if (*dependency_submit_id != completed_submit_id)
				continue;
			matched = true;
			break;
		}
		if (!matched)
			continue;
		ret = poulsbo_sgx_refresh_dependency_blocks_locked(dev_priv,
								   submit);
		if (ret)
			return ret;
	}

	return 0;
}

static int poulsbo_sgx_dependency_observed_ticket_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	unsigned int slot,
	u32 *observed_complete_ticket_out,
	bool *source_missing_out)
{
	struct psb_sgx_lab_submit *dependency_submit = NULL;
	u32 *dependency_submit_id = poulsbo_sgx_dependency_submit_id_ptr(submit,
									 slot);
	u32 *snapshot_observed_complete_ticket =
		poulsbo_sgx_dependency_snapshot_observed_complete_ticket_ptr(
			submit, slot);
	u32 observed_complete_ticket = dev_priv->sgx_lab_global_complete_ticket;
	bool source_missing = false;

	if (!dependency_submit_id || !snapshot_observed_complete_ticket)
		return -EINVAL;

	if (*dependency_submit_id) {
		dependency_submit = poulsbo_sgx_find_submit_locked(
			dev_priv, *dependency_submit_id);
		if (dependency_submit) {
			observed_complete_ticket =
				dependency_submit->global_complete_ticket;
		} else {
			source_missing = true;
			observed_complete_ticket =
				*snapshot_observed_complete_ticket;
		}
	}

	if (observed_complete_ticket_out)
		*observed_complete_ticket_out = observed_complete_ticket;
	if (source_missing_out)
		*source_missing_out = source_missing;

	return 0;
}

static int poulsbo_sgx_queue_progression(u32 read_offset, u32 queue_size,
					 u32 queue_packet_size,
					 u32 queue_packet_count,
					 u32 *tail_packets_out,
					 u32 *head_packets_out,
					 u32 *write_offset_out)
{
	u64 total_bytes;
	u32 remaining;
	u32 tail_packets;
	u32 head_packets;
	u32 write_offset;

	if (!queue_size || !queue_packet_size)
		return -EINVAL;
	if (read_offset >= queue_size)
		return -EINVAL;
	if (read_offset % queue_packet_size)
		return -EINVAL;

	total_bytes = (u64)queue_packet_size * queue_packet_count;
	if (total_bytes > queue_size)
		return -EINVAL;

	remaining = queue_size - read_offset;
	tail_packets = min_t(u32, queue_packet_count,
			     remaining / queue_packet_size);
	head_packets = queue_packet_count - tail_packets;

	if (!queue_packet_count)
		write_offset = read_offset;
	else if (total_bytes <= remaining)
		write_offset = read_offset + (u32)total_bytes;
	else
		write_offset = (u32)(total_bytes - remaining);

	if (write_offset == queue_size)
		write_offset = 0;

	if (tail_packets_out)
		*tail_packets_out = tail_packets;
	if (head_packets_out)
		*head_packets_out = head_packets;
	if (write_offset_out)
		*write_offset_out = write_offset;

	return 0;
}

static int poulsbo_sgx_validate_submit_queue_header_image_locked(
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_queue_header_words *expected_queue_header)
{
	struct psb_sgx_lab_queue_header_words built_queue_header;
	struct psb_sgx_lab_queue_header_words ctl_queue_header;
	const struct psb_sgx_lab_queue_header_words *expected;
	int ret;

	ret = poulsbo_sgx_read_queue_header_locked(control_obj, 0,
						   &ctl_queue_header);
	if (ret)
		return ret;
	if (expected_queue_header) {
		expected = expected_queue_header;
	} else {
		ret = poulsbo_sgx_build_submit_queue_header_locked(
			submit, &built_queue_header);
		if (ret)
			return ret;
		expected = &built_queue_header;
	}
	if (memcmp(&ctl_queue_header, expected, sizeof(*expected)) != 0)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_validate_submit_ccb_record_image_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_ccb_record_words *expected_ccb_record)
{
	struct psb_sgx_lab_ccb_record_words built_ccb_record;
	struct psb_sgx_lab_ccb_record_words ctl_ccb_record;
	const struct psb_sgx_lab_ccb_record_words *expected;
	int ret;

	ret = poulsbo_sgx_read_submit_ccb_record_locked(control_obj,
							submit->ccb_offset,
							&ctl_ccb_record);
	if (ret)
		return ret;
	if (expected_ccb_record) {
		expected = expected_ccb_record;
	} else {
		ret = poulsbo_sgx_build_submit_ccb_record_locked(
			dev_priv, submit, &built_ccb_record);
		if (ret)
			return ret;
		expected = &built_ccb_record;
	}
	if (memcmp(&ctl_ccb_record, expected, sizeof(*expected)) != 0)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_validate_submit_manifest_image_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_manifest_words *expected_manifest)
{
	struct psb_sgx_lab_manifest_words built_manifest;
	struct psb_sgx_lab_manifest_words ctl_manifest;
	const struct psb_sgx_lab_manifest_words *expected;
	int ret;

	ret = poulsbo_sgx_read_submit_manifest_locked(control_obj,
						      submit->manifest_offset,
						      &ctl_manifest);
	if (ret)
		return ret;
	if (expected_manifest) {
		expected = expected_manifest;
	} else {
		ret = poulsbo_sgx_build_submit_manifest_locked(
			dev_priv, submit, &built_manifest);
		if (ret)
			return ret;
		expected = &built_manifest;
	}
	if (memcmp(&ctl_manifest, expected, sizeof(*expected)) != 0)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_validate_submit_live_image_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_live_words *expected_live)
{
	struct psb_sgx_lab_live_words built_live;
	struct psb_sgx_lab_live_words ctl_live;
	const struct psb_sgx_lab_live_words *expected;
	int ret;

	ret = poulsbo_sgx_read_submit_live_locked(control_obj,
						  submit->live_offset,
						  &ctl_live);
	if (ret)
		return ret;
	if (expected_live) {
		expected = expected_live;
	} else {
		ret = poulsbo_sgx_build_submit_live_locked(
			dev_priv, submit, &built_live);
		if (ret)
			return ret;
		expected = &built_live;
	}
	if (memcmp(&ctl_live, expected, sizeof(*expected)) != 0)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_validate_submit_descriptor_image_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_descriptor_words *expected_descriptor)
{
	struct psb_sgx_lab_descriptor_words built_descriptor;
	struct psb_sgx_lab_descriptor_words ctl_descriptor;
	const struct psb_sgx_lab_descriptor_words *expected;
	int ret;

	ret = poulsbo_sgx_read_submit_descriptor_locked(control_obj,
							submit->descriptor_offset,
							&ctl_descriptor);
	if (ret)
		return ret;
	if (expected_descriptor) {
		expected = expected_descriptor;
	} else {
		ret = poulsbo_sgx_build_submit_descriptor_locked(
			dev_priv, submit, &built_descriptor);
		if (ret)
			return ret;
		expected = &built_descriptor;
	}
	if (memcmp(&ctl_descriptor, expected, sizeof(*expected)) != 0)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_validate_submit_plan_image_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_plan_words *expected_plan)
{
	struct psb_sgx_lab_plan_words built_plan;
	struct psb_sgx_lab_plan_words ctl_plan;
	const struct psb_sgx_lab_plan_words *expected;
	int ret;

	ret = poulsbo_sgx_read_submit_plan_locked(control_obj,
						  submit->plan_offset,
						  &ctl_plan);
	if (ret)
		return ret;
	if (expected_plan) {
		expected = expected_plan;
	} else {
		ret = poulsbo_sgx_build_submit_plan_locked(
			dev_priv, submit, &built_plan);
		if (ret)
			return ret;
		expected = &built_plan;
	}
	if (memcmp(&ctl_plan, expected, sizeof(*expected)) != 0)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_validate_submit_issue_plan_image_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_issue_plan_words *expected_issue_plan)
{
	struct psb_sgx_lab_issue_plan_words built_issue_plan;
	struct psb_sgx_lab_issue_plan_words ctl_issue_plan;
	const struct psb_sgx_lab_issue_plan_words *expected;
	int ret;

	ret = poulsbo_sgx_read_submit_issue_plan_locked(
		control_obj, submit->issue_plan_offset, &ctl_issue_plan);
	if (ret)
		return ret;
	if (expected_issue_plan) {
		expected = expected_issue_plan;
	} else {
		ret = poulsbo_sgx_build_submit_issue_plan_view_locked(
			dev_priv, submit, &built_issue_plan);
		if (ret)
			return ret;
		expected = &built_issue_plan;
	}
	if (memcmp(&ctl_issue_plan, expected, sizeof(*expected)) != 0)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_validate_submit_sync_image_locked(
	struct psb_sgx_lab_object *status_obj,
	struct psb_sgx_lab_submit *submit)
{
	struct psb_sgx_lab_sync_words expected_sync;
	struct psb_sgx_lab_sync_words ctl_sync;
	int ret;

	ret = poulsbo_sgx_read_submit_sync_locked(status_obj, submit->sync_offset,
						  &ctl_sync);
	if (ret)
		return ret;
	ret = poulsbo_sgx_build_submit_sync_locked(submit, &expected_sync);
	if (ret)
		return ret;
	if (memcmp(&ctl_sync, &expected_sync, sizeof(expected_sync)) != 0)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_validate_submit_owner_image_locked(
	struct psb_sgx_lab_object *status_obj,
	struct psb_sgx_lab_submit *submit)
{
	struct psb_sgx_lab_owner_words expected_owner;
	struct psb_sgx_lab_owner_words ctl_owner;
	int ret;

	ret = poulsbo_sgx_read_submit_owner_locked(status_obj, submit->owner_offset,
						   &ctl_owner);
	if (ret)
		return ret;
	ret = poulsbo_sgx_build_submit_owner_locked(submit, &expected_owner);
	if (ret)
		return ret;
	if (memcmp(&ctl_owner, &expected_owner, sizeof(expected_owner)) != 0)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_validate_submit_report_image_locked(
	struct psb_sgx_lab_object *status_obj,
	struct psb_sgx_lab_submit *submit)
{
	struct psb_sgx_lab_report_words expected_report;
	struct psb_sgx_lab_report_words ctl_report;
	int ret;

	ret = poulsbo_sgx_read_submit_report_locked(status_obj,
						    submit->report_offset,
						    &ctl_report);
	if (ret)
		return ret;
	ret = poulsbo_sgx_build_submit_report_locked(submit, &expected_report);
	if (ret)
		return ret;
	if (memcmp(&ctl_report, &expected_report, sizeof(expected_report)) != 0)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_validate_submit_result_image_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_object *status_obj,
	struct psb_sgx_lab_submit *submit)
{
	struct psb_sgx_lab_result_words expected_result;
	struct psb_sgx_lab_result_words ctl_result;
	int ret;

	ret = poulsbo_sgx_read_submit_result_locked(status_obj,
						    submit->result_offset,
						    &ctl_result);
	if (ret)
		return ret;
	ret = poulsbo_sgx_build_submit_result_locked(dev_priv, submit,
						     &expected_result);
	if (ret)
		return ret;
	if (memcmp(&ctl_result, &expected_result, sizeof(expected_result)) != 0)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_validate_submit_control_side_images_locked(
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	const struct psb_sgx_lab_submit_sgx_ta_submission *ta_submission)
{
	u32 expected_write_offset;
	u32 expected_tail_packets;
	u32 expected_head_packets;
	int ret;

	if (!submit || !control_obj || !ta_submission)
		return -EINVAL;

	ret = poulsbo_sgx_validate_submit_queue_header_image_locked(
		control_obj, submit,
		&ta_submission->queued_ta_command.queue_header);
	if (ret)
		return ret;
	ret = poulsbo_sgx_validate_submit_ccb_record_image_locked(
		NULL, control_obj, submit,
		&ta_submission->queued_ta_command.ta_command.ccb_record);
	if (ret)
		return ret;

	ret = poulsbo_sgx_queue_progression(submit->ccb_read_offset,
					    submit->queue_size,
					    submit->queue_packet_size,
					    submit->queue_packet_count,
					    &expected_tail_packets,
					    &expected_head_packets,
					    &expected_write_offset);
	if (ret)
		return ret;

	if (submit->ccb_write_offset & 0x3 || submit->ccb_read_offset & 0x3)
		return -EINVAL;
	if (submit->ccb_read_offset >= control_obj->gt->gem.size)
		return -EINVAL;
	if (submit->ccb_read_offset >= submit->queue_size ||
	    submit->ccb_write_offset >= submit->queue_size)
		return -EINVAL;
	if (submit->queue_base_offset + submit->ccb_write_offset >
	    control_obj->gt->gem.size)
		return -EINVAL;
	if (submit->queue_base_offset + submit->ccb_read_offset >
	    control_obj->gt->gem.size)
		return -EINVAL;
	if (submit->ccb_offset !=
	    submit->queue_base_offset + submit->ccb_read_offset)
		return -EINVAL;
	if (submit->queue_tail_packet_count != expected_tail_packets ||
	    submit->queue_head_packet_count != expected_head_packets)
		return -EINVAL;
	if (submit->ccb_write_offset != expected_write_offset)
		return -EINVAL;
	if (!submit->queue_packet_count) {
		if (submit->queue_tail_packet_count ||
		    submit->queue_head_packet_count)
			return -EINVAL;
	} else if (!submit->queue_tail_packet_count) {
		return -EINVAL;
	}

	if (submit->manifest_config_count) {
		ret = poulsbo_sgx_validate_submit_manifest_image_locked(
			NULL, control_obj, submit, &ta_submission->manifest);
		if (ret)
			return ret;
	}

	if (submit->live_config_count) {
		ret = poulsbo_sgx_validate_submit_live_image_locked(
			NULL, control_obj, submit, &ta_submission->live);
		if (ret)
			return ret;
	}

	if (submit->descriptor_config_count) {
		ret = poulsbo_sgx_validate_submit_descriptor_image_locked(
			NULL, control_obj, submit, &ta_submission->descriptor);
		if (ret)
			return ret;
	}

	if (submit->plan_config_count) {
		ret = poulsbo_sgx_validate_submit_plan_image_locked(
			NULL, control_obj, submit, &ta_submission->plan);
		if (ret)
			return ret;
	}

	if (submit->issue_plan_config_count) {
		ret = poulsbo_sgx_validate_submit_issue_plan_image_locked(
			NULL, control_obj, submit, &ta_submission->issue_plan);
		if (ret)
			return ret;
	}

	return 0;
}

static int poulsbo_sgx_validate_submit_status_side_images_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *status_obj,
	const struct psb_sgx_lab_submit_sgx_ta_submission *ta_submission)
{
	struct psb_sgx_lab_dependency_words expected_dependency;
	struct psb_sgx_lab_dependency_words ctl_dependency;
	const struct psb_sgx_lab_submit_sgx_cmdta_shared *live_cmdta_shared;
	const struct psb_sgx_lab_submit_status_update *status_info;
	unsigned int slot;
	u32 index;
	int ret;

	if (!status_obj)
		return 0;

	if (submit->sync_config_count) {
		ret = poulsbo_sgx_validate_submit_sync_image_locked(
			status_obj, submit);
		if (ret)
			return ret;
	}

	if (submit->owner_config_count) {
		ret = poulsbo_sgx_validate_submit_owner_image_locked(
			status_obj, submit);
		if (ret)
			return ret;
	}

	if (submit->report_config_count) {
		ret = poulsbo_sgx_validate_submit_report_image_locked(
			status_obj, submit);
		if (ret)
			return ret;
	}

	if (submit->result_config_count) {
		ret = poulsbo_sgx_validate_submit_result_image_locked(
			dev_priv, status_obj, submit);
		if (ret)
			return ret;
	}

	for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
		u32 *dependency_offset =
			poulsbo_sgx_dependency_offset_ptr(submit, slot);
		u32 *dependency_config_count =
			poulsbo_sgx_dependency_config_count_ptr(submit, slot);

		if (!dependency_offset || !dependency_config_count)
			return -EINVAL;
		if (!*dependency_config_count)
			continue;
		ret = poulsbo_sgx_read_submit_dependency_block_locked(
			status_obj, *dependency_offset, &ctl_dependency);
		if (ret)
			return ret;
		ret = poulsbo_sgx_load_submit_dependency_snapshot_locked(
			submit, slot, &expected_dependency);
		if (ret)
			return ret;
		if (memcmp(&ctl_dependency, &expected_dependency,
			   sizeof(expected_dependency)) != 0)
			return -EINVAL;
	}

	if (!ta_submission || !ta_submission->kick)
		return -EINVAL;

	live_cmdta_shared = &ta_submission->queued_ta_command.ta_command
				    .ta_kick.cmdta.shared;
	if (!live_cmdta_shared->kick)
		return -EINVAL;

	for (index = 0; index < live_cmdta_shared->num_ta_status_vals; index++) {
		u32 value;

		status_info = &live_cmdta_shared->ta_status_info[index];
		ret = poulsbo_sgx_object_read_u32_locked(
			status_obj, status_info->value_offset, &value);
		if (ret)
			return ret;
		if (value != status_info->value)
			return -EINVAL;
	}

	for (index = 0; index < live_cmdta_shared->num_3d_status_vals; index++) {
		u32 value;

		status_info = &live_cmdta_shared->threed_status_info[index];
		ret = poulsbo_sgx_object_read_u32_locked(
			status_obj, status_info->value_offset, &value);
		if (ret)
			return ret;
		if (value != status_info->value)
			return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_validate_submit_sgx_ta_command_locked(
	struct psb_sgx_lab_submit_ccb_kick *ccb_kick,
	struct psb_sgx_lab_submit_sgx_ta_command *ta_command)
{
	struct psb_sgx_lab_submit_sgx_ta_kick *ta_kick;
	u32 index;

	if (!ccb_kick || !ta_command)
		return -EINVAL;

	ta_kick = &ta_command->ta_kick;
	if (ta_command->kick != ccb_kick || ta_kick->kick != ccb_kick ||
	    ta_kick->cmdta.kick != ccb_kick ||
	    ta_kick->cmdta.shared.kick != ccb_kick ||
	    ta_kick->command.service_address !=
		    ccb_kick->cmdta.packet.command.service_address ||
	    ta_kick->command.cache_control !=
		    ccb_kick->cmdta.packet.command.cache_control ||
	    ta_kick->command.data0 != ccb_kick->cmdta.packet.command.data0 ||
	    ta_kick->command.data1 != ccb_kick->cmdta.packet.command.data1 ||
	    ta_command->ccb_record.service_address !=
		    ta_kick->command.service_address ||
	    ta_command->ccb_record.cache_control !=
		    ta_kick->command.cache_control ||
	    ta_command->ccb_record.data0 != ta_kick->command.data0 ||
	    ta_command->ccb_record.data1 != ta_kick->command.data1 ||
	    ta_command->ccb_record.pds_gpu_va32 !=
		    ccb_kick->cmdta.packet.pds_gpu_va32 ||
	    ta_command->ccb_record.use_gpu_va32 !=
		    ccb_kick->cmdta.packet.use_gpu_va32 ||
	    ta_command->ccb_record.target_gpu_va32 !=
		    ccb_kick->cmdta.packet.target_gpu_va32 ||
	    ta_command->ccb_record.status_gpu_va32 !=
		    ccb_kick->cmdta.packet.ccb_status_gpu_va32 ||
	    ta_kick->control_gpu_va32 != ccb_kick->cmdta.packet.control_gpu_va32 ||
	    ta_kick->ccb_gpu_va32 != ccb_kick->cmdta.ccb_gpu_va32 ||
	    ta_kick->cmdta_gpu_va32 != ccb_kick->cmdta.ccb_gpu_va32 ||
	    ta_kick->queue_gpu_va32 != ccb_kick->cmdta.queue_gpu_va32 ||
	    ta_kick->sync_gpu_va32 != ccb_kick->cmdta.sync_gpu_va32 ||
	    ta_kick->owner_gpu_va32 != ccb_kick->cmdta.owner_gpu_va32 ||
	    ta_kick->ta_status_gpu_va32 != ccb_kick->cmdta.ta_status_gpu_va32 ||
	    ta_kick->status_3d_gpu_va32 != ccb_kick->cmdta.status_3d_gpu_va32 ||
	    ta_kick->cmdta.shared.num_ta_status_vals !=
		    ccb_kick->num_ta_status_updates ||
	    ta_kick->cmdta.shared.num_3d_status_vals !=
		    ccb_kick->num_3d_status_updates ||
	    ta_kick->cmdta.shared.ccb_offset != ccb_kick->ccb_offset ||
	    ta_kick->cmdta.shared.ta_dependency != ccb_kick->ta_dependency ||
	    ta_kick->cmdta.shared.hwdevice_sync_list.num_sync_objects !=
		    ta_kick->cmdta.shared.num_dst_sync_objects ||
	    ta_kick->cmdta.shared.num_ta_dst_syncs !=
		    ta_kick->cmdta.shared.num_dst_sync_objects) {
		return -EINVAL;
	}
	if (!ta_kick->cmdta.shared.num_dst_sync_objects &&
	    (!poulsbo_sgx_submit_sgx_device_sync_object_is_zero(
		     &ta_kick->cmdta.shared.ta_tq_sync) ||
	     !poulsbo_sgx_submit_sgx_device_sync_object_is_zero(
		     &ta_kick->cmdta.shared.threed_tq_sync) ||
	     ta_kick->cmdta.shared.hwdevice_sync_list.access_gpu_va32 ||
	     !poulsbo_sgx_submit_sgx_device_sync_object_is_zero(
		     &ta_kick->cmdta.shared.hwdevice_sync_list.sync_data[0]) ||
	     !poulsbo_sgx_submit_sgx_device_sync_object_is_zero(
		     &ta_kick->cmdta.ta_dst_syncs[0]) ||
	     !poulsbo_sgx_submit_sgx_device_sync_object_is_zero(
		     &ta_kick->cmdta.threed_src_syncs[0]))) {
		return -EINVAL;
	}
	if (ta_kick->cmdta.shared.num_3d_src_syncs &&
	    (!ta_kick->cmdta.shared.ta3d_dependency.submit_id ||
	     !ta_kick->cmdta.shared.ta3d_dependency.required_complete_ticket)) {
		return -EINVAL;
	}
	if (!ta_kick->cmdta.shared.num_ta_dst_syncs &&
	    (!poulsbo_sgx_submit_sgx_device_sync_object_is_zero(
		     &ta_kick->cmdta.shared.ta_tq_sync) ||
	     !poulsbo_sgx_submit_sgx_device_sync_object_is_zero(
		     &ta_kick->cmdta.shared.threed_tq_sync))) {
		return -EINVAL;
	}
	if (ta_kick->cmdta.shared.num_dst_sync_objects &&
	    !poulsbo_sgx_submit_sgx_device_sync_object_equal(
		    &ta_kick->cmdta.shared.hwdevice_sync_list.sync_data[0],
		    &ta_kick->cmdta.shared.ta_tq_sync)) {
		return -EINVAL;
	}
	if (ta_kick->cmdta.shared.num_ta_src_syncs)
		return -EINVAL;
	if (!ta_kick->cmdta.shared.num_ta_dst_syncs &&
	    !poulsbo_sgx_submit_sgx_device_sync_object_is_zero(
		    &ta_kick->cmdta.ta_dst_syncs[0])) {
		return -EINVAL;
	}
	if (ta_kick->cmdta.shared.num_ta_dst_syncs &&
	    !poulsbo_sgx_submit_sgx_device_sync_object_equal(
		    &ta_kick->cmdta.ta_dst_syncs[0],
		    &ta_kick->cmdta.shared.ta_tq_sync)) {
		return -EINVAL;
	}
	if (!ta_kick->cmdta.shared.num_3d_src_syncs &&
	    !poulsbo_sgx_submit_sgx_device_sync_object_is_zero(
		    &ta_kick->cmdta.shared.ta3d_dependency_sync)) {
		return -EINVAL;
	}
	if (!ta_kick->cmdta.shared.num_3d_src_syncs &&
	    !poulsbo_sgx_submit_sgx_device_sync_object_is_zero(
		    &ta_kick->cmdta.threed_src_syncs[0])) {
		return -EINVAL;
	}
	if (ta_kick->cmdta.shared.num_3d_src_syncs &&
	    (!ta_kick->cmdta.shared.ta3d_dependency_sync.write_complete_gpu_va32 ||
	     !poulsbo_sgx_submit_sgx_device_sync_object_equal(
		     &ta_kick->cmdta.threed_src_syncs[0],
		     &ta_kick->cmdta.shared.ta3d_dependency_sync))) {
		return -EINVAL;
	}
	for (index = 0; index < ta_kick->cmdta.shared.num_ta_status_vals; index++) {
		if (ta_kick->cmdta.shared.ta_status_info[index].value_offset !=
			    ccb_kick->ta_status_updates[index].value_offset ||
		    ta_kick->cmdta.shared.ta_status_info[index].value_gpu_va32 !=
			    ccb_kick->ta_status_updates[index].value_gpu_va32 ||
		    ta_kick->cmdta.shared.ta_status_info[index].value !=
			    ccb_kick->ta_status_updates[index].value) {
			return -EINVAL;
		}
	}
	for (index = 0; index < ta_kick->cmdta.shared.num_3d_status_vals; index++) {
		if (ta_kick->cmdta.shared.threed_status_info[index].value_offset !=
			    ccb_kick->threed_status_updates[index].value_offset ||
		    ta_kick->cmdta.shared.threed_status_info[index].value_gpu_va32 !=
			    ccb_kick->threed_status_updates[index].value_gpu_va32 ||
		    ta_kick->cmdta.shared.threed_status_info[index].value !=
			    ccb_kick->threed_status_updates[index].value) {
			return -EINVAL;
		}
	}

	return 0;
}

static int poulsbo_sgx_validate_submit_sgx_queued_ta_command_locked(
	struct psb_sgx_lab_submit_ccb_kick *ccb_kick,
	struct psb_sgx_lab_submit_sgx_queued_ta_command *queued_ta_command)
{
	int ret;

	if (!ccb_kick || !queued_ta_command)
		return -EINVAL;

	ret = poulsbo_sgx_validate_submit_sgx_ta_command_locked(
		ccb_kick, &queued_ta_command->ta_command);
	if (ret)
		return ret;

	if (queued_ta_command->kick != ccb_kick ||
	    queued_ta_command->queue_header.write_offset !=
		    ccb_kick->cmdta.packet.ccb_write_offset ||
	    queued_ta_command->queue_header.read_offset !=
		    ccb_kick->cmdta.packet.ccb_read_offset ||
	    queued_ta_command->queue_header.queue_base_offset !=
		    ccb_kick->cmdta.packet.queue_base_offset ||
	    queued_ta_command->queue_header.queue_size !=
		    ccb_kick->cmdta.packet.queue_size ||
	    queued_ta_command->queue_header.queue_packet_size !=
		    ccb_kick->cmdta.packet.queue_packet_size ||
	    queued_ta_command->queue_header.queue_packet_count !=
		    ccb_kick->cmdta.packet.queue_packet_count ||
	    queued_ta_command->queue_header.queue_tail_packet_count !=
		    ccb_kick->cmdta.packet.queue_tail_packet_count ||
	    queued_ta_command->queue_header.queue_head_packet_count !=
		    ccb_kick->cmdta.packet.queue_head_packet_count ||
	    queued_ta_command->queue_header.queue_base_offset +
			    queued_ta_command->queue_header.read_offset !=
		    ccb_kick->cmdta.packet.ccb_offset) {
		return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_validate_submit_sgx_queued_ta_command_issue_plan_locked(
	struct psb_sgx_lab_submit_ccb_kick *ccb_kick,
	struct psb_sgx_lab_submit_sgx_queued_ta_command *queued_ta_command)
{
	if (!ccb_kick || !queued_ta_command)
		return -EINVAL;

	if (queued_ta_command->have_issue_plan) {
		if (!ccb_kick->cmdta.have_issue_plan ||
		    queued_ta_command->issue_plan.owner_state !=
			    ccb_kick->cmdta.issue_plan.owner_state ||
		    queued_ta_command->issue_plan.issued_packets !=
			    ccb_kick->cmdta.issue_plan.issued_packets ||
		    queued_ta_command->issue_plan.refilled_from_complete !=
			    ccb_kick->cmdta.issue_plan.refilled_from_complete ||
		    queued_ta_command->issue_plan.new_queue_epoch !=
			    ccb_kick->cmdta.issue_plan.new_queue_epoch ||
		    queued_ta_command->issue_plan.new_packet_count !=
			    ccb_kick->cmdta.issue_plan.new_packet_count ||
		    queued_ta_command->issue_plan.new_tail_packets !=
			    ccb_kick->cmdta.issue_plan.new_tail_packets ||
		    queued_ta_command->issue_plan.new_head_packets !=
			    ccb_kick->cmdta.issue_plan.new_head_packets ||
		    queued_ta_command->issue_plan.new_write_offset !=
			    ccb_kick->cmdta.issue_plan.new_write_offset ||
		    queued_ta_command->issue_plan.new_sync_pending_seq !=
			    ccb_kick->cmdta.issue_plan.new_sync_pending_seq ||
		    queued_ta_command->issue_plan.new_owner_issue_cookie !=
			    ccb_kick->cmdta.issue_plan.new_owner_issue_cookie ||
		    queued_ta_command->issue_plan.new_global_issue_ticket !=
			    ccb_kick->cmdta.issue_plan.new_global_issue_ticket ||
		    queued_ta_command->issue_plan.new_sync_flags !=
			    ccb_kick->cmdta.issue_plan.new_sync_flags) {
			return -EINVAL;
		}
	} else if (ccb_kick->cmdta.have_issue_plan ||
		   queued_ta_command->issue_plan.issued_packets ||
		   queued_ta_command->issue_plan.new_packet_count ||
		   queued_ta_command->issue_plan.new_write_offset ||
		   queued_ta_command->issue_plan.new_global_issue_ticket) {
		return -EINVAL;
	}

	return 0;
}

static bool poulsbo_sgx_submit_sgx_queued_ta_command_identity_equal(
	const struct psb_sgx_lab_submit_sgx_queued_ta_command *left,
	const struct psb_sgx_lab_submit_sgx_queued_ta_command *right)
{
	const struct psb_sgx_lab_submit_sgx_ta_command *left_command;
	const struct psb_sgx_lab_submit_sgx_ta_command *right_command;
	const struct psb_sgx_lab_submit_sgx_ta_kick *left_kick;
	const struct psb_sgx_lab_submit_sgx_ta_kick *right_kick;

	if (!left || !right)
		return false;

	left_command = &left->ta_command;
	right_command = &right->ta_command;
	left_kick = &left_command->ta_kick;
	right_kick = &right_command->ta_kick;

	return left_command->ccb_record.service_address ==
		       right_command->ccb_record.service_address &&
	       left_command->ccb_record.cache_control ==
		       right_command->ccb_record.cache_control &&
	       left_command->ccb_record.data0 ==
		       right_command->ccb_record.data0 &&
	       left_command->ccb_record.data1 ==
		       right_command->ccb_record.data1 &&
	       left_command->ccb_record.pds_gpu_va32 ==
		       right_command->ccb_record.pds_gpu_va32 &&
	       left_command->ccb_record.use_gpu_va32 ==
		       right_command->ccb_record.use_gpu_va32 &&
	       left_command->ccb_record.target_gpu_va32 ==
		       right_command->ccb_record.target_gpu_va32 &&
	       left_command->ccb_record.status_gpu_va32 ==
		       right_command->ccb_record.status_gpu_va32 &&
	       left_kick->command.service_address ==
		       right_kick->command.service_address &&
	       left_kick->command.cache_control ==
		       right_kick->command.cache_control &&
	       left_kick->command.data0 == right_kick->command.data0 &&
	       left_kick->command.data1 == right_kick->command.data1 &&
	       left_kick->control_gpu_va32 == right_kick->control_gpu_va32 &&
	       left_kick->sync_gpu_va32 == right_kick->sync_gpu_va32 &&
	       left_kick->owner_gpu_va32 == right_kick->owner_gpu_va32 &&
	       left_kick->ta_status_gpu_va32 == right_kick->ta_status_gpu_va32 &&
	       left_kick->status_3d_gpu_va32 == right_kick->status_3d_gpu_va32 &&
	       left->queue_header.queue_base_offset ==
		       right->queue_header.queue_base_offset &&
	       left->queue_header.queue_size == right->queue_header.queue_size &&
	       left->queue_header.queue_packet_size ==
		       right->queue_header.queue_packet_size;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_submission_locked(
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_submit_ccb_kick *ccb_kick,
	struct psb_sgx_lab_submit_sgx_ta_submission *ta_submission)
{
	struct psb_sgx_lab_submit_sgx_queued_ta_command *queued_ta_command;
	struct psb_sgx_lab_submit_sgx_ta_command *ta_command;
	struct psb_sgx_lab_submit_sgx_ta_kick *ta_kick;
	int ret;

	if (!submit || !ccb_kick || !ta_submission)
		return -EINVAL;

	queued_ta_command = &ta_submission->queued_ta_command;
	ta_command = &queued_ta_command->ta_command;
	ta_kick = &ta_command->ta_kick;
	if (ta_submission->kick != ccb_kick) {
		ret = -EINVAL;
		goto out;
	}
	ret = poulsbo_sgx_validate_submit_sgx_queued_ta_command_locked(
		ccb_kick, queued_ta_command);
	if (ret)
		goto out;
	ret = poulsbo_sgx_validate_submit_sgx_queued_ta_command_issue_plan_locked(
		ccb_kick, queued_ta_command);
	if (ret)
		goto out;
	if (!!submit->live_config_count != !!ta_submission->have_live ||
	    !!submit->manifest_config_count != !!ta_submission->have_manifest ||
	    !!submit->descriptor_config_count !=
		    !!ta_submission->have_descriptor ||
	    !!submit->plan_config_count != !!ta_submission->have_plan ||
	    !!submit->issue_plan_config_count !=
		    !!ta_submission->have_issue_plan_view) {
		ret = -EINVAL;
		goto out;
	}
	if (ta_submission->have_live &&
	    (ta_submission->live.queue_base_offset !=
		     queued_ta_command->queue_header.queue_base_offset ||
	     ta_submission->live.queue_size !=
		     queued_ta_command->queue_header.queue_size ||
	     ta_submission->live.queue_packet_size !=
		     queued_ta_command->queue_header.queue_packet_size ||
	     ta_submission->live.queue_packet_count !=
		     queued_ta_command->queue_header.queue_packet_count ||
	     ta_submission->live.ccb_offset !=
		     queued_ta_command->queue_header.queue_base_offset +
			     queued_ta_command->queue_header.read_offset ||
	     ta_submission->live.ccb_write_offset !=
		     queued_ta_command->queue_header.write_offset ||
	     ta_submission->live.ccb_read_offset !=
		     queued_ta_command->queue_header.read_offset ||
	     ta_submission->live.ccb_service_address !=
		     ta_command->ccb_record.service_address ||
	     ta_submission->live.ccb_cache_control !=
		     ta_command->ccb_record.cache_control ||
	     ta_submission->live.ccb_data0 != ta_command->ccb_record.data0 ||
	     ta_submission->live.ccb_data1 != ta_command->ccb_record.data1 ||
	     ta_submission->live.pds_gpu_va32 !=
		     ta_command->ccb_record.pds_gpu_va32 ||
	     ta_submission->live.use_gpu_va32 !=
		     ta_command->ccb_record.use_gpu_va32 ||
	     ta_submission->live.target_gpu_va32 !=
		     ta_command->ccb_record.target_gpu_va32 ||
	     ta_submission->live.ccb_status_gpu_va32 !=
		     ta_command->ccb_record.status_gpu_va32)) {
		ret = -EINVAL;
		goto out;
	}
	if (ta_submission->have_manifest &&
	    (ta_submission->manifest.queue_base_offset !=
		     queued_ta_command->queue_header.queue_base_offset ||
	     ta_submission->manifest.queue_size !=
		     queued_ta_command->queue_header.queue_size ||
	     ta_submission->manifest.queue_packet_size !=
		     queued_ta_command->queue_header.queue_packet_size ||
	     ta_submission->manifest.queue_packet_count !=
		     queued_ta_command->queue_header.queue_packet_count ||
	     ta_submission->manifest.ccb_offset !=
		     queued_ta_command->queue_header.queue_base_offset +
			     queued_ta_command->queue_header.read_offset ||
	     ta_submission->manifest.ccb_write_offset !=
		     queued_ta_command->queue_header.write_offset ||
	     ta_submission->manifest.ccb_read_offset !=
		     queued_ta_command->queue_header.read_offset ||
	     ta_submission->manifest.pds_gpu_va32 !=
		     ta_command->ccb_record.pds_gpu_va32 ||
	     ta_submission->manifest.use_gpu_va32 !=
		     ta_command->ccb_record.use_gpu_va32 ||
	     ta_submission->manifest.target_gpu_va32 !=
		     ta_command->ccb_record.target_gpu_va32 ||
	     ta_submission->manifest.status_gpu_va32 !=
		     ta_command->ccb_record.status_gpu_va32)) {
		ret = -EINVAL;
		goto out;
	}
	if (ta_submission->have_descriptor &&
	    (ta_submission->descriptor.queue_packet_count !=
		     queued_ta_command->queue_header.queue_packet_count ||
	     ta_submission->descriptor.queue_packet_size !=
		     queued_ta_command->queue_header.queue_packet_size ||
	     ta_submission->descriptor.ccb_offset !=
		     queued_ta_command->queue_header.queue_base_offset +
			     queued_ta_command->queue_header.read_offset ||
	     ta_submission->descriptor.control_gpu_va32 !=
		     ta_kick->control_gpu_va32 ||
	     ta_submission->descriptor.pds_gpu_va32 !=
		     ta_command->ccb_record.pds_gpu_va32 ||
	     ta_submission->descriptor.use_gpu_va32 !=
		     ta_command->ccb_record.use_gpu_va32 ||
	     ta_submission->descriptor.target_gpu_va32 !=
		     ta_command->ccb_record.target_gpu_va32 ||
	     ta_submission->descriptor.status_gpu_va32 !=
		     ta_command->ccb_record.status_gpu_va32)) {
		ret = -EINVAL;
		goto out;
	}
	if (ta_submission->have_plan &&
	    (ta_submission->plan.ccb_service_address !=
		     ta_command->ccb_record.service_address ||
	     ta_submission->plan.ccb_cache_control !=
		     ta_command->ccb_record.cache_control ||
	     ta_submission->plan.ccb_data0 != ta_command->ccb_record.data0 ||
	     ta_submission->plan.ccb_data1 != ta_command->ccb_record.data1 ||
	     ta_submission->plan.control_gpu_va32 !=
		     ta_kick->control_gpu_va32 ||
	     ta_submission->plan.pds_gpu_va32 !=
		     ta_command->ccb_record.pds_gpu_va32 ||
	     ta_submission->plan.use_gpu_va32 !=
		     ta_command->ccb_record.use_gpu_va32 ||
	     ta_submission->plan.target_gpu_va32 !=
		     ta_command->ccb_record.target_gpu_va32 ||
	     ta_submission->plan.status_gpu_va32 !=
		     ta_command->ccb_record.status_gpu_va32)) {
		ret = -EINVAL;
		goto out;
	}
	if (ta_submission->have_issue_plan_view &&
	    ta_submission->issue_plan.valid &&
	    (ta_submission->issue_plan.ccb_service_address !=
		     ta_command->ccb_record.service_address ||
	     ta_submission->issue_plan.ccb_cache_control !=
		     ta_command->ccb_record.cache_control ||
	     ta_submission->issue_plan.ccb_data0 != ta_command->ccb_record.data0 ||
	     ta_submission->issue_plan.ccb_data1 != ta_command->ccb_record.data1 ||
	     ta_submission->issue_plan.control_gpu_va32 !=
		     ta_kick->control_gpu_va32 ||
	     ta_submission->issue_plan.pds_gpu_va32 !=
		     ta_command->ccb_record.pds_gpu_va32 ||
	     ta_submission->issue_plan.use_gpu_va32 !=
		     ta_command->ccb_record.use_gpu_va32 ||
	     ta_submission->issue_plan.target_gpu_va32 !=
		     ta_command->ccb_record.target_gpu_va32 ||
	     ta_submission->issue_plan.status_gpu_va32 !=
		     ta_command->ccb_record.status_gpu_va32)) {
		ret = -EINVAL;
		goto out;
	}
	ret = 0;
out:
	return ret;
}

static int poulsbo_sgx_validate_launch_submit_sgx_ta_submission_locked(
	struct psb_sgx_lab_submit_ccb_kick *ccb_kick,
	struct psb_sgx_lab_submit_sgx_ta_submission *ta_submission)
{
	struct psb_sgx_lab_submit_sgx_queued_ta_command *queued_ta_command;
	int ret;

	if (!ccb_kick || !ta_submission)
		return -EINVAL;

	queued_ta_command = &ta_submission->queued_ta_command;
	if (ta_submission->kick != ccb_kick || !queued_ta_command->have_issue_plan ||
	    !ccb_kick->cmdta.have_issue_plan) {
		return -EINVAL;
	}

	ret = poulsbo_sgx_validate_submit_sgx_queued_ta_command_locked(
		ccb_kick, queued_ta_command);
	if (ret)
		return ret;

	return poulsbo_sgx_validate_submit_sgx_queued_ta_command_issue_plan_locked(
		ccb_kick, queued_ta_command);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_execution_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_object *status_obj,
	struct psb_sgx_lab_submit_sgx_ta_execution *execution)
{
	int ret;

	if (!execution)
		return -EINVAL;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submission_locked(
		submit, &execution->live_kick, &execution->live_submission);
	if (ret)
		return ret;

	ret = poulsbo_sgx_validate_submit_control_side_images_locked(
		submit, control_obj, &execution->live_submission);
	if (ret)
		return ret;

	ret = poulsbo_sgx_validate_submit_status_side_images_locked(
		dev_priv, submit, status_obj, &execution->live_submission);
	if (ret)
		return ret;

	if (!execution->have_launch_submission)
		return 0;

	return poulsbo_sgx_validate_launch_submit_sgx_ta_submission_locked(
		&execution->launch_kick, &execution->launch_submission);
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_timeline_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_object *status_obj,
	struct psb_sgx_lab_submit_sgx_ta_timeline *timeline)
{
	struct psb_sgx_lab_submit_sgx_queued_ta_command *launch_submission_command;
	int ret;

	if (!timeline)
		return -EINVAL;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_execution_locked(
		dev_priv, submit, control_obj, status_obj, &timeline->execution);
	if (ret)
		return ret;

	if (timeline->report_stage != poulsbo_sgx_submit_report_stage_locked(submit) ||
	    timeline->report_stage_count !=
		    poulsbo_sgx_submit_report_stage_count_locked(
			    submit, timeline->report_stage) ||
	    timeline->issue_count != submit->issue_count ||
	    timeline->transition_count != submit->transition_count ||
	    timeline->global_issue_ticket != submit->global_issue_ticket ||
	    timeline->global_running_ticket != submit->global_running_ticket ||
	    timeline->global_complete_ticket != submit->global_complete_ticket ||
	    timeline->arm_valid != submit->arm_valid ||
	    timeline->emit_valid != submit->emit_valid ||
	    timeline->launch_valid != submit->launch_valid ||
	    timeline->launch_consumed != submit->launch_consumed ||
	    timeline->dispatch_valid != submit->dispatch_valid ||
	    timeline->retired_valid != submit->retired_valid ||
	    !!timeline->have_launch_command != !!submit->launch_valid ||
	    !!timeline->have_dispatch_command != !!submit->dispatch_valid ||
	    !!timeline->have_retired_command != !!submit->retired_valid) {
		return -EINVAL;
	}

	if (timeline->have_retired_command && !timeline->have_dispatch_command)
		return -EINVAL;

	if (timeline->have_launch_command) {
		ret = poulsbo_sgx_validate_submit_sgx_queued_ta_command_locked(
			&timeline->launch_kick, &timeline->launch_command);
		if (ret)
			return ret;
	}
	if (timeline->have_dispatch_command) {
		ret = poulsbo_sgx_validate_submit_sgx_queued_ta_command_locked(
			&timeline->dispatch_kick, &timeline->dispatch_command);
		if (ret)
			return ret;
		if (timeline->have_launch_command &&
		    !poulsbo_sgx_submit_sgx_queued_ta_command_identity_equal(
			    &timeline->launch_command,
			    &timeline->dispatch_command))
			return -EINVAL;
	}
	if (timeline->have_retired_command) {
		ret = poulsbo_sgx_validate_submit_sgx_queued_ta_command_locked(
			&timeline->retired_kick, &timeline->retired_command);
		if (ret)
			return ret;
		if (!poulsbo_sgx_submit_sgx_queued_ta_command_identity_equal(
			    &timeline->dispatch_command,
			    &timeline->retired_command))
			return -EINVAL;
	}

	if (!timeline->execution.have_launch_submission)
		return 0;

	launch_submission_command =
		&timeline->execution.launch_submission.queued_ta_command;
	if (!timeline->have_launch_command ||
	    !poulsbo_sgx_submit_sgx_queued_ta_command_identity_equal(
		    launch_submission_command, &timeline->launch_command) ||
	    launch_submission_command->queue_header.write_offset !=
		    timeline->launch_command.queue_header.write_offset ||
	    launch_submission_command->queue_header.read_offset !=
		    timeline->launch_command.queue_header.read_offset ||
	    launch_submission_command->queue_header.queue_base_offset !=
		    timeline->launch_command.queue_header.queue_base_offset ||
	    launch_submission_command->queue_header.queue_size !=
		    timeline->launch_command.queue_header.queue_size ||
	    launch_submission_command->queue_header.queue_packet_size !=
		    timeline->launch_command.queue_header.queue_packet_size ||
	    launch_submission_command->queue_header.queue_packet_count !=
		    timeline->launch_command.queue_header.queue_packet_count ||
	    launch_submission_command->queue_header.queue_tail_packet_count !=
		    timeline->launch_command.queue_header.queue_tail_packet_count ||
	    launch_submission_command->queue_header.queue_head_packet_count !=
		    timeline->launch_command.queue_header.queue_head_packet_count)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_gate_locked(
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_runtime *runtime,
	const struct psb_sgx_lab_submit_sgx_ta_gate *gate)
{
	u32 active_dependency_count = 0;
	u32 source_bound_dependency_count = 0;
	u32 gated_dependency_count = 0;
	u32 satisfied_dependency_count = 0;
	u32 blocked_dependency_count = 0;
	u32 missing_dependency_count = 0;
	bool running_slot_busy;
	bool can_enter_running;
	unsigned int slot;

	if (!submit || !runtime || !gate)
		return -EINVAL;

	for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
		const struct psb_sgx_lab_submit_sgx_dependency_state *dependency;
		const struct psb_sgx_lab_submit_sgx_gate_slot *gate_dependency;
		bool source_bound;
		bool gated;
		bool satisfied;
		bool blocked;
		bool ready;

		dependency = &runtime->dependencies[slot];
		gate_dependency = &gate->dependencies[slot];
		source_bound = !!dependency->submit_id;
		gated = !!dependency->required_ticket;
		satisfied = dependency->active &&
			    (!gated ||
			     dependency->observed_complete_ticket >=
				     dependency->required_ticket);
		blocked = dependency->active && gated && !satisfied;
		ready = dependency->active && !dependency->source_missing &&
			satisfied;

		if (gate_dependency->active != dependency->active ||
		    gate_dependency->source_bound != source_bound ||
		    gate_dependency->source_missing !=
			    dependency->source_missing ||
		    gate_dependency->gated != gated ||
		    gate_dependency->satisfied != satisfied ||
		    gate_dependency->blocked != blocked ||
		    gate_dependency->ready != ready ||
		    gate_dependency->submit_id != dependency->submit_id ||
		    gate_dependency->required_ticket !=
			    dependency->required_ticket ||
		    gate_dependency->observed_complete_ticket !=
			    dependency->observed_complete_ticket ||
		    gate_dependency->flags != dependency->flags) {
			return -EINVAL;
		}

		if (!dependency->active)
			continue;

		active_dependency_count++;
		if (source_bound)
			source_bound_dependency_count++;
		if (gated)
			gated_dependency_count++;
		if (satisfied)
			satisfied_dependency_count++;
		if (blocked)
			blocked_dependency_count++;
		if (dependency->source_missing)
			missing_dependency_count++;
	}

	running_slot_busy =
		runtime->coordination.active_running_valid &&
		runtime->coordination.active_running_submit_id !=
			submit->submit_id;
	can_enter_running =
		!running_slot_busy &&
		!blocked_dependency_count &&
		!missing_dependency_count;

	if (gate->active_dependency_count != active_dependency_count ||
	    gate->source_bound_dependency_count !=
		    source_bound_dependency_count ||
	    gate->gated_dependency_count != gated_dependency_count ||
	    gate->satisfied_dependency_count != satisfied_dependency_count ||
	    gate->blocked_dependency_count != blocked_dependency_count ||
	    gate->missing_dependency_count != missing_dependency_count ||
	    gate->running_slot_busy != running_slot_busy ||
	    gate->can_enter_running != can_enter_running) {
		return -EINVAL;
	}

	if ((submit->issue_count || submit->transition_count ||
	     submit->owner_state == PSB_SGX_LAB_OWNER_RUNNING ||
	     submit->owner_state == PSB_SGX_LAB_OWNER_COMPLETE) &&
	    (gate->blocked_dependency_count ||
	     gate->missing_dependency_count)) {
		return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_control_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_control *control)
{
	const struct psb_sgx_lab_submit_sgx_issue_request *issue_request;
	struct psb_sgx_lab_issue_plan plan;
	bool emit_matches_live;
	bool launch_ready;
	bool running_owned_by_self;
	bool plan_buildable = false;
	bool plan_matches_launch = false;
	bool gate_ready;
	int ret;

	if (!dev_priv || !submit || !control)
		return -EINVAL;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_gate_locked(
		submit, &control->runtime, &control->gate);
	if (ret)
		return ret;

	emit_matches_live =
		submit->emit_valid &&
		poulsbo_sgx_submit_emit_matches_locked(dev_priv, submit);
	launch_ready =
		poulsbo_sgx_submit_launch_base_matches_locked(dev_priv, submit);
	running_owned_by_self =
		control->runtime.coordination.active_running_valid &&
		control->runtime.coordination.active_running_submit_id ==
			submit->submit_id;
	if (control->emit_matches_live != emit_matches_live ||
	    control->have_launch_snapshot != submit->launch_valid ||
	    control->launch_consumed != submit->launch_consumed ||
	    control->launch_ready != launch_ready ||
	    control->running_owned_by_self != running_owned_by_self) {
		return -EINVAL;
	}

	issue_request = &control->issue_request;
	if (!issue_request->active)
		return 0;

	gate_ready =
		!issue_request->issue_to_running ||
		control->gate.can_enter_running;
	if (issue_request->issue_to_running !=
		    (issue_request->owner_state ==
		     PSB_SGX_LAB_OWNER_RUNNING) ||
	    issue_request->gate_ready != gate_ready) {
		return -EINVAL;
	}
	if (launch_ready) {
		plan_buildable = poulsbo_sgx_build_issue_plan_locked(
			dev_priv, submit, issue_request->owner_state,
			issue_request->issued_packets, &plan);
	}
	if (plan_buildable) {
		plan_matches_launch =
			poulsbo_sgx_submit_issue_plan_matches_launch_locked(
				submit, &plan);
	}
	if (issue_request->plan_buildable != plan_buildable ||
	    issue_request->plan_matches_launch != plan_matches_launch ||
	    issue_request->can_issue !=
		    (plan_matches_launch && gate_ready)) {
		return -EINVAL;
	}
	if (!plan_buildable)
		return 0;
	if (issue_request->plan.owner_state != plan.owner_state ||
	    issue_request->plan.issued_packets != plan.issued_packets ||
	    issue_request->plan.refilled_from_complete !=
		    plan.refilled_from_complete ||
	    issue_request->plan.new_queue_epoch != plan.new_queue_epoch ||
	    issue_request->plan.new_packet_count != plan.new_packet_count ||
	    issue_request->plan.new_tail_packets != plan.new_tail_packets ||
	    issue_request->plan.new_head_packets != plan.new_head_packets ||
	    issue_request->plan.new_write_offset != plan.new_write_offset ||
	    issue_request->plan.new_sync_pending_seq !=
		    plan.new_sync_pending_seq ||
	    issue_request->plan.new_owner_issue_cookie !=
		    plan.new_owner_issue_cookie ||
	    issue_request->plan.new_global_issue_ticket !=
		    plan.new_global_issue_ticket ||
	    issue_request->plan.new_sync_flags != plan.new_sync_flags) {
		return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_validate_submit_sgx_ta_runtime_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_object *status_obj)
{
	struct psb_sgx_lab_submit_sgx_ta_runtime *runtime;
	struct psb_sgx_lab_submit_sgx_ta_gate gate;
	bool any_dependency_active = false;
	bool any_dependency_required = false;
	unsigned int slot;
	int ret;

	runtime = kmalloc(sizeof(*runtime), GFP_KERNEL);
	if (!runtime)
		return -ENOMEM;

	ret = poulsbo_sgx_build_submit_sgx_ta_runtime_locked(
		dev_priv, submit, runtime);
	if (ret)
		goto out;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_timeline_locked(
		dev_priv, submit, control_obj, status_obj, &runtime->timeline);
	if (ret)
		goto out;

	ret = poulsbo_sgx_build_submit_sgx_ta_gate_locked(submit, runtime,
							  &gate);
	if (ret)
		goto out;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_gate_locked(
		submit, runtime, &gate);
	if (ret)
		goto out;

	if (runtime->coordination.issue_ticket !=
		    dev_priv->sgx_lab_global_issue_ticket ||
	    runtime->coordination.running_ticket !=
		    dev_priv->sgx_lab_global_running_ticket ||
	    runtime->coordination.complete_ticket !=
		    dev_priv->sgx_lab_global_complete_ticket ||
	    runtime->coordination.issue_submit_id !=
		    dev_priv->sgx_lab_global_issue_submit_id ||
	    runtime->coordination.running_submit_id !=
		    dev_priv->sgx_lab_global_running_submit_id ||
	    runtime->coordination.complete_submit_id !=
		    dev_priv->sgx_lab_global_complete_submit_id ||
	    runtime->coordination.active_running_valid !=
		    dev_priv->sgx_lab_running_active ||
	    runtime->coordination.active_running_submit_id !=
		    dev_priv->sgx_lab_active_running_submit_id) {
		ret = -EINVAL;
		goto out;
	}

	for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
		struct psb_sgx_lab_submit_sgx_dependency_state *dependency;
		u32 *dependency_submit_id;
		u32 *dependency_snapshot_flags;

		dependency = &runtime->dependencies[slot];
		dependency_submit_id =
			poulsbo_sgx_dependency_submit_id_ptr(submit, slot);
		dependency_snapshot_flags =
			poulsbo_sgx_dependency_snapshot_flags_ptr(submit, slot);
		if (!dependency_submit_id || !dependency_snapshot_flags) {
			ret = -EINVAL;
			goto out;
		}
		if (*dependency_submit_id == submit->submit_id) {
			ret = -EINVAL;
			goto out;
		}
		if (dependency->active !=
		    poulsbo_sgx_dependency_slot_active(submit, slot) ||
		    dependency->submit_id != *dependency_submit_id ||
		    dependency->required_ticket !=
			    poulsbo_sgx_dependency_required_ticket(
				    submit, slot) ||
		    dependency->flags != *dependency_snapshot_flags) {
			ret = -EINVAL;
			goto out;
		}
		if (!dependency->active)
			continue;

		any_dependency_active = true;
		if (dependency->required_ticket)
			any_dependency_required = true;
		/*
		 * Validation is allowed to observe a still-blocked dependency.
		 * The launch/issue path enforces the gate when execution tries
		 * to consume the submission.
		 */
		if (dependency->submit_id &&
		    dependency->source_missing &&
		    !(dependency->flags &
		      PSB_SGX_LAB_DEPENDENCY_FLAG_SOURCE_MISSING)) {
			ret = -EINVAL;
			goto out;
		}
	}

	if (submit->sync_config_count) {
		if (any_dependency_active && !any_dependency_required) {
			ret = -EINVAL;
			goto out;
		}
		if (submit->sync_pending_seq < submit->sync_complete_seq ||
		    runtime->sync_outstanding_count !=
			    submit->sync_pending_seq -
				    submit->sync_complete_seq ||
		    runtime->sync_outstanding_count !=
			    submit->queue_packet_count ||
		    !!submit->issue_count != !!submit->global_issue_ticket ||
		    submit->global_issue_ticket >
			    runtime->coordination.issue_ticket) {
			ret = -EINVAL;
			goto out;
		}
	}

	if (submit->owner_config_count) {
		if (submit->owner_issue_cookie < submit->owner_complete_cookie ||
		    runtime->owner_outstanding_count !=
			    submit->owner_issue_cookie -
				    submit->owner_complete_cookie ||
		    runtime->owner_outstanding_count !=
			    runtime->sync_outstanding_count ||
		    (!submit->transition_count &&
		     (submit->global_complete_ticket ||
		      submit->global_running_ticket)) ||
		    submit->global_running_ticket >
			    runtime->coordination.running_ticket ||
		    submit->global_complete_ticket >
			    runtime->coordination.complete_ticket) {
			ret = -EINVAL;
			goto out;
		}

		if (runtime->coordination.running_owner_submit_count > 1) {
			ret = -EINVAL;
			goto out;
		}
		if (runtime->coordination.running_owner_submit_count) {
			if (!runtime->coordination.active_running_valid ||
			    runtime->coordination.active_running_submit_id !=
				    runtime->coordination.running_owner_submit_id) {
				ret = -EINVAL;
				goto out;
			}
		} else if (runtime->coordination.active_running_valid) {
			ret = -EINVAL;
			goto out;
		}

		if (submit->owner_state == PSB_SGX_LAB_OWNER_RUNNING) {
			if (!submit->global_running_ticket ||
			    !runtime->coordination.active_running_valid ||
			    runtime->coordination.active_running_submit_id !=
				    submit->submit_id ||
			    runtime->coordination.running_ticket !=
				    submit->global_running_ticket) {
				ret = -EINVAL;
				goto out;
			}
		} else if (runtime->coordination.active_running_valid &&
			   runtime->coordination.active_running_submit_id ==
				   submit->submit_id) {
			ret = -EINVAL;
			goto out;
		}

		if (!runtime->owner_outstanding_count) {
			if (submit->owner_state != PSB_SGX_LAB_OWNER_COMPLETE ||
			    !submit->global_complete_ticket) {
				ret = -EINVAL;
				goto out;
			}
		} else if (submit->owner_state ==
			   PSB_SGX_LAB_OWNER_COMPLETE) {
			ret = -EINVAL;
			goto out;
		}
	}

	ret = 0;

out:
	kfree(runtime);
	return ret;
}

static int poulsbo_sgx_validate_submit_sgx_ta_control_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_object *status_obj)
{
	struct psb_sgx_lab_submit_sgx_ta_control *control;
	int ret;

	control = kmalloc(sizeof(*control), GFP_KERNEL);
	if (!control)
		return -ENOMEM;

	ret = poulsbo_sgx_validate_submit_sgx_ta_runtime_locked(
		dev_priv, submit, control_obj, status_obj);
	if (ret)
		goto out;

	ret = poulsbo_sgx_build_submit_sgx_ta_control_locked(
		dev_priv, submit, false, 0, 0, control);
	if (ret)
		goto out;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_control_locked(
		dev_priv, submit, control);

out:
	kfree(control);
	return ret;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_schedule_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_schedule *schedule)
{
	int ret;
	bool can_issue_queued;
	bool can_issue_running;
	bool can_enter_running;
	bool can_stay_running;
	bool can_complete;

	if (!dev_priv || !submit || !schedule)
		return -EINVAL;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_control_locked(
		dev_priv, submit, &schedule->control);
	if (ret)
		return ret;

	can_issue_queued =
		schedule->control.launch_ready &&
		!schedule->control.launch_consumed &&
		submit->owner_state == PSB_SGX_LAB_OWNER_QUEUED;
	can_issue_running =
		schedule->control.launch_ready &&
		!schedule->control.launch_consumed &&
		schedule->control.running_owned_by_self &&
		submit->owner_state == PSB_SGX_LAB_OWNER_RUNNING;
	can_enter_running =
		submit->owner_state == PSB_SGX_LAB_OWNER_QUEUED &&
		submit->queue_packet_count &&
		schedule->control.gate.can_enter_running;
	can_stay_running =
		submit->owner_state == PSB_SGX_LAB_OWNER_RUNNING &&
		submit->global_running_ticket &&
		submit->queue_packet_count;
	can_complete =
		submit->owner_state == PSB_SGX_LAB_OWNER_RUNNING &&
		submit->queue_packet_count;

	if (schedule->can_issue_queued != can_issue_queued ||
	    schedule->can_issue_running != can_issue_running ||
	    schedule->can_enter_running != can_enter_running ||
	    schedule->can_stay_running != can_stay_running ||
	    schedule->can_complete != can_complete) {
		return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_validate_submit_sgx_ta_schedule_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_object *status_obj)
{
	struct psb_sgx_lab_submit_sgx_ta_schedule *schedule;
	int ret;

	schedule = kmalloc(sizeof(*schedule), GFP_KERNEL);
	if (!schedule)
		return -ENOMEM;

	ret = poulsbo_sgx_validate_submit_sgx_ta_control_locked(
		dev_priv, submit, control_obj, status_obj);
	if (ret)
		goto out;

	ret = poulsbo_sgx_build_submit_sgx_ta_schedule_locked(
		dev_priv, submit, false, 0, 0, schedule);
	if (ret)
		goto out;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_schedule_locked(
		dev_priv, submit, schedule);

out:
	kfree(schedule);
	return ret;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_dispatch_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_dispatch *dispatch)
{
	u32 queue_region_end;
	u32 sync_region_end;
	u32 owner_region_end;
	int ret;

	if (!dev_priv || !submit || !dispatch)
		return -EINVAL;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_schedule_locked(
		dev_priv, submit, &dispatch->schedule);
	if (ret)
		return ret;

	if (dispatch->issue_image.active != !!submit->launch_valid)
		return -EINVAL;
	if (!dispatch->issue_image.active)
		return 0;

	ret = poulsbo_sgx_queue_region_end(submit->launch_queue_base_offset,
					   submit->launch_queue_size,
					   &queue_region_end);
	if (ret)
		return ret;
	ret = poulsbo_sgx_sync_region_end(submit->launch_sync_offset,
					  &sync_region_end);
	if (ret)
		return ret;
	ret = poulsbo_sgx_owner_region_end(submit->launch_owner_offset,
					   &owner_region_end);
	if (ret)
		return ret;

	if (dispatch->issue_image.issue_write_offset !=
		    submit->launch_ccb_write_offset ||
	    dispatch->issue_image.final_write_offset !=
		    submit->launch_new_write_offset ||
	    dispatch->issue_image.queue_region_end != queue_region_end ||
	    dispatch->issue_image.sync_region_end != sync_region_end ||
	    dispatch->issue_image.owner_region_end != owner_region_end)
		return -EINVAL;

	if (dispatch->issue_image.queue_header.write_offset !=
		    submit->launch_new_write_offset ||
	    dispatch->issue_image.queue_header.read_offset !=
		    submit->launch_ccb_read_offset ||
	    dispatch->issue_image.queue_header.queue_base_offset !=
		    submit->launch_queue_base_offset ||
	    dispatch->issue_image.queue_header.queue_size !=
		    submit->launch_queue_size ||
	    dispatch->issue_image.queue_header.queue_packet_size !=
		    submit->launch_queue_packet_size ||
	    dispatch->issue_image.queue_header.queue_packet_count !=
		    submit->launch_queue_packet_count ||
	    dispatch->issue_image.queue_header.queue_tail_packet_count !=
		    submit->launch_queue_tail_packet_count ||
	    dispatch->issue_image.queue_header.queue_head_packet_count !=
		    submit->launch_queue_head_packet_count)
		return -EINVAL;

	if (dispatch->issue_image.ccb_record.service_address !=
		    submit->launch_ccb_service_address ||
	    dispatch->issue_image.ccb_record.cache_control !=
		    submit->launch_ccb_cache_control ||
	    dispatch->issue_image.ccb_record.data0 !=
		    submit->launch_ccb_data0 ||
	    dispatch->issue_image.ccb_record.data1 !=
		    submit->launch_ccb_data1 ||
	    dispatch->issue_image.ccb_record.pds_gpu_va32 !=
		    (u32)submit->launch_pds_gpu_va ||
	    dispatch->issue_image.ccb_record.use_gpu_va32 !=
		    (u32)submit->launch_use_gpu_va ||
	    dispatch->issue_image.ccb_record.target_gpu_va32 !=
		    (u32)submit->launch_target_gpu_va ||
	    dispatch->issue_image.ccb_record.status_gpu_va32 !=
		    (u32)submit->launch_status_gpu_va)
		return -EINVAL;

	if (dispatch->issue_image.sync.read_offset !=
		    submit->launch_ccb_read_offset ||
	    dispatch->issue_image.sync.write_offset !=
		    submit->launch_new_write_offset ||
	    dispatch->issue_image.sync.complete_seq !=
		    submit->launch_sync_complete_seq ||
	    dispatch->issue_image.sync.pending_seq !=
		    submit->launch_sync_pending_seq ||
	    dispatch->issue_image.sync.flags !=
		    submit->launch_sync_flags ||
	    dispatch->issue_image.sync.epoch !=
		    submit->launch_queue_epoch ||
	    dispatch->issue_image.sync.global_issue_ticket !=
		    submit->launch_global_issue_ticket)
		return -EINVAL;

	if (dispatch->issue_image.owner.owner_state !=
		    submit->launch_owner_state ||
	    dispatch->issue_image.owner.owner_issue_cookie !=
		    submit->launch_owner_issue_cookie ||
	    dispatch->issue_image.owner.owner_complete_cookie !=
		    submit->launch_owner_complete_cookie ||
	    dispatch->issue_image.owner.epoch !=
		    submit->launch_queue_epoch ||
	    dispatch->issue_image.owner.global_complete_ticket !=
		    submit->launch_global_complete_ticket ||
	    dispatch->issue_image.owner.global_running_ticket !=
		    submit->launch_global_running_ticket)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_validate_submit_sgx_ta_dispatch_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_object *status_obj)
{
	struct psb_sgx_lab_submit_sgx_ta_dispatch *dispatch;
	int ret;

	dispatch = kmalloc(sizeof(*dispatch), GFP_KERNEL);
	if (!dispatch)
		return -ENOMEM;

	ret = poulsbo_sgx_validate_submit_sgx_ta_schedule_locked(
		dev_priv, submit, control_obj, status_obj);
	if (ret)
		goto out;

	ret = poulsbo_sgx_build_submit_sgx_ta_dispatch_locked(
		dev_priv, submit, false, 0, 0, dispatch);
	if (ret)
		goto out;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_dispatch_locked(
		dev_priv, submit, dispatch);

out:
	kfree(dispatch);
	return ret;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_path_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_path *path)
{
	int ret;

	if (!dev_priv || !submit || !path)
		return -EINVAL;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_dispatch_locked(
		dev_priv, submit, &path->dispatch);
	if (ret)
		return ret;

	return path->transition_image.active ? -EINVAL : 0;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_commit_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_commit *commit)
{
	int ret;

	if (!dev_priv || !submit || !commit)
		return -EINVAL;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_path_locked(
		dev_priv, submit, &commit->path);
	if (ret)
		return ret;

	return (commit->issue_active || commit->transition_active) ? -EINVAL : 0;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_event_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_event *event)
{
	int ret;

	if (!dev_priv || !submit || !event)
		return -EINVAL;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_commit_locked(
		dev_priv, submit, &event->commit);
	if (ret)
		return ret;

	return (event->refresh_views || event->increment_issue_count ||
		event->increment_transition_count ||
		event->update_last_epoch_jiffies ||
		event->update_last_issue_jiffies ||
		event->update_last_transition_jiffies) ? -EINVAL : 0;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_work_item_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_work_item *work_item)
{
	int ret;

	if (!dev_priv || !submit || !work_item)
		return -EINVAL;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_event_locked(
		dev_priv, submit, &work_item->event);
	if (ret)
		return ret;
	if (work_item->submit_id != submit->submit_id)
		return -EINVAL;

	return (work_item->validate_before_apply ||
		work_item->disarm_on_build_failure ||
		work_item->owner_state || work_item->issued_packets ||
		work_item->completed_packets) ? -EINVAL : 0;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_op_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_op *ta_op)
{
	int ret;

	if (!dev_priv || !submit || !ta_op)
		return -EINVAL;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_work_item_locked(
		dev_priv, submit, &ta_op->work_item);
	if (ret)
		return ret;
	if (ta_op->submit_id != submit->submit_id)
		return -EINVAL;

	return (ta_op->op_kind || ta_op->owner_state ||
		ta_op->packet_count) ? -EINVAL : 0;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_scheduler_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_scheduler *scheduler)
{
	const struct psb_sgx_lab_submit_sgx_ta_commit *commit;
	const struct psb_sgx_lab_submit_sgx_coordination *coordination;
	struct psb_sgx_lab_submit_sgx_scheduler_state next_state;
	bool active;
	bool issue_advanced = false;
	bool running_advanced = false;
	bool complete_advanced = false;
	bool active_running_changed = false;
	int ret;

	if (!dev_priv || !submit || !scheduler)
		return -EINVAL;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_op_locked(
		dev_priv, submit, &scheduler->ta_op);
	if (ret)
		return ret;

	active = scheduler->ta_op.op_kind != PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE;
	if (scheduler->active != active)
		return -EINVAL;
	if (!active) {
		const struct psb_sgx_lab_submit_sgx_scheduler_state zero_state = {};

		if (scheduler->issue_advanced || scheduler->running_advanced ||
		    scheduler->complete_advanced ||
		    scheduler->active_running_changed)
			return -EINVAL;
		return memcmp(&scheduler->next_state, &zero_state,
			      sizeof(zero_state)) ? -EINVAL : 0;
	}

	commit = &scheduler->ta_op.work_item.event.commit;
	coordination =
		&commit->path.dispatch.schedule.control.runtime.coordination;
	poulsbo_sgx_fill_submit_sgx_scheduler_state_from_coordination(
		coordination, &next_state);

	switch (scheduler->ta_op.op_kind) {
	case PSB_SGX_LAB_SUBMIT_SGX_TA_OP_ISSUE:
		if (!commit->issue_active || commit->transition_active)
			return -EINVAL;
		issue_advanced = true;
		next_state.issue_ticket = commit->next_state.global_issue_ticket;
		next_state.issue_submit_id = submit->submit_id;
		break;
	case PSB_SGX_LAB_SUBMIT_SGX_TA_OP_TRANSITION:
		if (!commit->transition_active || commit->issue_active)
			return -EINVAL;
		if (commit->next_state.owner_state == PSB_SGX_LAB_OWNER_RUNNING &&
		    commit->entering_running) {
			running_advanced = true;
			active_running_changed = true;
			next_state.running_ticket =
				commit->next_state.global_running_ticket;
			next_state.running_submit_id = submit->submit_id;
			next_state.active_running_valid = true;
			next_state.active_running_submit_id = submit->submit_id;
		} else if (commit->next_state.owner_state !=
				   PSB_SGX_LAB_OWNER_RUNNING &&
			   next_state.active_running_valid &&
			   next_state.active_running_submit_id ==
				   submit->submit_id) {
			active_running_changed = true;
			next_state.active_running_valid = false;
			next_state.active_running_submit_id = 0;
		}
		if (commit->entering_complete) {
			complete_advanced = true;
			next_state.complete_ticket =
				commit->next_state.global_complete_ticket;
			next_state.complete_submit_id = submit->submit_id;
		}
		break;
	default:
		return -EINVAL;
	}

	if (scheduler->issue_advanced != issue_advanced ||
	    scheduler->running_advanced != running_advanced ||
	    scheduler->complete_advanced != complete_advanced ||
	    scheduler->active_running_changed != active_running_changed)
		return -EINVAL;

	if (scheduler->next_state.issue_ticket != next_state.issue_ticket ||
	    scheduler->next_state.running_ticket != next_state.running_ticket ||
	    scheduler->next_state.complete_ticket != next_state.complete_ticket ||
	    scheduler->next_state.issue_submit_id !=
		    next_state.issue_submit_id ||
	    scheduler->next_state.running_submit_id !=
		    next_state.running_submit_id ||
	    scheduler->next_state.complete_submit_id !=
		    next_state.complete_submit_id ||
	    scheduler->next_state.active_running_valid !=
		    next_state.active_running_valid ||
	    scheduler->next_state.active_running_submit_id !=
		    next_state.active_running_submit_id) {
		return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_sequence_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_sequence *sequence)
{
	const struct psb_sgx_lab_submit_sgx_ta_event *event;
	bool active;
	u32 next_issue_count;
	u32 next_transition_count;
	int ret;

	if (!dev_priv || !submit || !sequence)
		return -EINVAL;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_scheduler_locked(
		dev_priv, submit, &sequence->scheduler);
	if (ret)
		return ret;

	active = sequence->scheduler.ta_op.op_kind !=
		 PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE;
	if (sequence->active != active)
		return -EINVAL;
	if (!active) {
		if (sequence->scheduler.active ||
		    sequence->scheduler.issue_advanced ||
		    sequence->scheduler.running_advanced ||
		    sequence->scheduler.complete_advanced ||
		    sequence->scheduler.active_running_changed ||
		    sequence->scheduler.next_state.issue_ticket ||
		    sequence->scheduler.next_state.running_ticket ||
		    sequence->scheduler.next_state.complete_ticket ||
		    sequence->scheduler.next_state.issue_submit_id ||
		    sequence->scheduler.next_state.running_submit_id ||
		    sequence->scheduler.next_state.complete_submit_id ||
		    sequence->scheduler.next_state.active_running_valid ||
		    sequence->scheduler.next_state.active_running_submit_id ||
		    sequence->refresh_views ||
		    sequence->increment_issue_count ||
		    sequence->increment_transition_count ||
		    sequence->update_last_epoch_jiffies ||
		    sequence->update_last_issue_jiffies ||
		    sequence->update_last_transition_jiffies ||
		    sequence->next_issue_count ||
		    sequence->next_transition_count) {
			return -EINVAL;
		}
		return 0;
	}

	event = &sequence->scheduler.ta_op.work_item.event;
	next_issue_count =
		submit->issue_count + (event->increment_issue_count ? 1 : 0);
	next_transition_count =
		submit->transition_count +
		(event->increment_transition_count ? 1 : 0);

	if (sequence->refresh_views != event->refresh_views ||
	    sequence->increment_issue_count != event->increment_issue_count ||
	    sequence->increment_transition_count !=
		    event->increment_transition_count ||
	    sequence->update_last_epoch_jiffies !=
		    event->update_last_epoch_jiffies ||
	    sequence->update_last_issue_jiffies !=
		    event->update_last_issue_jiffies ||
	    sequence->update_last_transition_jiffies !=
		    event->update_last_transition_jiffies ||
	    sequence->next_issue_count != next_issue_count ||
	    sequence->next_transition_count != next_transition_count)
		return -EINVAL;

	return 0;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_work_queue_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_work_queue *work_queue)
{
	const struct psb_sgx_lab_submit_sgx_ta_commit *commit;
	const struct psb_sgx_lab_submit_sgx_coordination *coordination;
	struct psb_sgx_lab_submit_sgx_scheduler_state base_state;
	bool active;
	int ret;

	if (!dev_priv || !submit || !work_queue)
		return -EINVAL;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_sequence_locked(
		dev_priv, submit, &work_queue->sequence);
	if (ret)
		return ret;

	active = work_queue->sequence.scheduler.ta_op.op_kind !=
		 PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE;
	if (work_queue->active != active)
		return -EINVAL;
	if (!active) {
		if (work_queue->submit_id || work_queue->op_kind ||
		    work_queue->owner_state || work_queue->packet_count ||
		    work_queue->base_state.issue_ticket ||
		    work_queue->base_state.running_ticket ||
		    work_queue->base_state.complete_ticket ||
		    work_queue->base_state.issue_submit_id ||
		    work_queue->base_state.running_submit_id ||
		    work_queue->base_state.complete_submit_id ||
		    work_queue->base_state.active_running_valid ||
		    work_queue->base_state.active_running_submit_id ||
		    work_queue->base_issue_count ||
		    work_queue->base_transition_count) {
			return -EINVAL;
		}
		return 0;
	}

	commit = &work_queue->sequence.scheduler.ta_op.work_item.event.commit;
	coordination =
		&commit->path.dispatch.schedule.control.runtime.coordination;
	poulsbo_sgx_fill_submit_sgx_scheduler_state_from_coordination(
		coordination, &base_state);

	if (work_queue->submit_id != submit->submit_id ||
	    work_queue->op_kind != work_queue->sequence.scheduler.ta_op.op_kind ||
	    work_queue->owner_state !=
		    work_queue->sequence.scheduler.ta_op.owner_state ||
	    work_queue->packet_count !=
		    work_queue->sequence.scheduler.ta_op.packet_count ||
	    work_queue->base_issue_count != submit->issue_count ||
	    work_queue->base_transition_count != submit->transition_count)
		return -EINVAL;

	if (work_queue->base_state.issue_ticket != base_state.issue_ticket ||
	    work_queue->base_state.running_ticket != base_state.running_ticket ||
	    work_queue->base_state.complete_ticket !=
		    base_state.complete_ticket ||
	    work_queue->base_state.issue_submit_id !=
		    base_state.issue_submit_id ||
	    work_queue->base_state.running_submit_id !=
		    base_state.running_submit_id ||
	    work_queue->base_state.complete_submit_id !=
		    base_state.complete_submit_id ||
	    work_queue->base_state.active_running_valid !=
		    base_state.active_running_valid ||
	    work_queue->base_state.active_running_submit_id !=
		    base_state.active_running_submit_id) {
		return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_job_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_job *job)
{
	const struct psb_sgx_lab_submit_sgx_ta_control *control;
	const struct psb_sgx_lab_submit_sgx_ta_runtime *runtime;
	const struct psb_sgx_lab_submit_sgx_ta_gate *gate;
	bool active;
	unsigned int slot;
	int ret;

	if (!dev_priv || !submit || !job)
		return -EINVAL;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_work_queue_locked(
		dev_priv, submit, &job->work_queue);
	if (ret)
		return ret;

	active = job->work_queue.sequence.scheduler.ta_op.op_kind !=
		 PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE;
	if (job->active != active)
		return -EINVAL;
	if (!active) {
		if (job->submit_id || job->context || job->cmd_type ||
		    job->ctrl_flags || job->control_object_id ||
		    job->pds_object_id || job->use_object_id ||
		    job->target_object_id || job->status_object_id ||
		    job->num_ta_status_vals || job->num_3d_status_vals ||
		    job->ta_dependency || job->active_dependency_count ||
		    job->running_slot_busy || job->can_enter_running) {
			return -EINVAL;
		}
		for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
			if (job->dependencies[slot].active ||
			    job->dependencies[slot].source_missing ||
			    job->dependencies[slot].submit_id ||
			    job->dependencies[slot].required_ticket ||
			    job->dependencies[slot].observed_complete_ticket ||
			    job->dependencies[slot].flags) {
				return -EINVAL;
			}
		}
		return 0;
	}

	control =
		&job->work_queue.sequence.scheduler.ta_op.work_item.event.commit.path.dispatch.schedule.control;
	runtime = &control->runtime;
	gate = &control->gate;

	if (job->submit_id != submit->submit_id ||
	    job->context != submit->context ||
	    job->cmd_type != submit->cmd_type ||
	    job->ctrl_flags != submit->ctrl_flags ||
	    job->control_object_id != submit->control_object_id ||
	    job->pds_object_id != submit->pds_object_id ||
	    job->use_object_id != submit->use_object_id ||
	    job->target_object_id != submit->target_object_id ||
	    job->status_object_id != submit->status_object_id ||
	    job->num_ta_status_vals != submit->num_ta_status_vals ||
	    job->num_3d_status_vals != submit->num_3d_status_vals ||
	    job->ta_dependency != submit->ta_dependency ||
	    job->active_dependency_count != gate->active_dependency_count ||
	    job->running_slot_busy != gate->running_slot_busy ||
	    job->can_enter_running != gate->can_enter_running) {
		return -EINVAL;
	}

	for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
		if (job->dependencies[slot].active !=
			    runtime->dependencies[slot].active ||
		    job->dependencies[slot].source_missing !=
			    runtime->dependencies[slot].source_missing ||
		    job->dependencies[slot].submit_id !=
			    runtime->dependencies[slot].submit_id ||
		    job->dependencies[slot].required_ticket !=
			    runtime->dependencies[slot].required_ticket ||
		    job->dependencies[slot].observed_complete_ticket !=
			    runtime->dependencies[slot]
				    .observed_complete_ticket ||
		    job->dependencies[slot].flags !=
			    runtime->dependencies[slot].flags) {
			return -EINVAL;
		}
	}

	return 0;
}

static int poulsbo_sgx_validate_built_submit_sgx_ta_request_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_submit_sgx_ta_request *request)
{
	const struct psb_sgx_lab_submit_sgx_ta_sequence *sequence;
	const struct psb_sgx_lab_submit_sgx_ta_scheduler *scheduler;
	const struct psb_sgx_lab_submit_sgx_ta_commit *commit;
	const struct psb_sgx_lab_submit_sgx_scheduler_state zero_state = {};
	bool active;
	int ret;

	if (!dev_priv || !submit || !request)
		return -EINVAL;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_job_locked(
		dev_priv, submit, &request->job);
	if (ret)
		return ret;

	active = request->job.work_queue.sequence.scheduler.ta_op.op_kind !=
		 PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE;
	if (request->active != active)
		return -EINVAL;
	if (!active) {
		if (request->submit_id || request->op_kind ||
		    request->owner_state || request->packet_count ||
		    request->issue_active || request->transition_active ||
		    request->refresh_views ||
		    request->increment_issue_count ||
		    request->increment_transition_count ||
		    request->issue_advanced || request->running_advanced ||
		    request->complete_advanced ||
		    request->active_running_changed) {
			return -EINVAL;
		}
		return memcmp(&request->next_state, &zero_state,
			      sizeof(zero_state)) ? -EINVAL : 0;
	}

	sequence = &request->job.work_queue.sequence;
	scheduler = &sequence->scheduler;
	commit = &scheduler->ta_op.work_item.event.commit;

	if (request->submit_id != submit->submit_id ||
	    request->op_kind != request->job.work_queue.op_kind ||
	    request->owner_state != request->job.work_queue.owner_state ||
	    request->packet_count != request->job.work_queue.packet_count ||
	    request->issue_active != commit->issue_active ||
	    request->transition_active != commit->transition_active ||
	    request->refresh_views != sequence->refresh_views ||
	    request->increment_issue_count !=
		    sequence->increment_issue_count ||
	    request->increment_transition_count !=
		    sequence->increment_transition_count ||
	    request->issue_advanced != scheduler->issue_advanced ||
	    request->running_advanced != scheduler->running_advanced ||
	    request->complete_advanced != scheduler->complete_advanced ||
	    request->active_running_changed !=
		    scheduler->active_running_changed) {
		return -EINVAL;
	}

	if (request->next_state.issue_ticket !=
		    scheduler->next_state.issue_ticket ||
	    request->next_state.running_ticket !=
		    scheduler->next_state.running_ticket ||
	    request->next_state.complete_ticket !=
		    scheduler->next_state.complete_ticket ||
	    request->next_state.issue_submit_id !=
		    scheduler->next_state.issue_submit_id ||
	    request->next_state.running_submit_id !=
		    scheduler->next_state.running_submit_id ||
	    request->next_state.complete_submit_id !=
		    scheduler->next_state.complete_submit_id ||
	    request->next_state.active_running_valid !=
		    scheduler->next_state.active_running_valid ||
	    request->next_state.active_running_submit_id !=
		    scheduler->next_state.active_running_submit_id) {
		return -EINVAL;
	}

	return 0;
}

static int poulsbo_sgx_validate_submit_sgx_ta_work_item_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_object *status_obj)
{
	struct psb_sgx_lab_submit_sgx_ta_work_item *work_item;
	int ret;

	work_item = kmalloc(sizeof(*work_item), GFP_KERNEL);
	if (!work_item)
		return -ENOMEM;

	ret = poulsbo_sgx_validate_submit_sgx_ta_dispatch_locked(
		dev_priv, submit, control_obj, status_obj);
	if (ret)
		goto out;

	ret = poulsbo_sgx_build_submit_sgx_ta_work_item_locked(
		dev_priv, submit, false, 0, 0, false, 0, 0, work_item);
	if (ret)
		goto out;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_work_item_locked(
		dev_priv, submit, work_item);

out:
	kfree(work_item);
	return ret;
}

static int poulsbo_sgx_validate_submit_sgx_ta_op_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_object *status_obj)
{
	struct psb_sgx_lab_submit_sgx_ta_op *ta_op;
	int ret;

	ta_op = kmalloc(sizeof(*ta_op), GFP_KERNEL);
	if (!ta_op)
		return -ENOMEM;

	ret = poulsbo_sgx_validate_submit_sgx_ta_work_item_locked(
		dev_priv, submit, control_obj, status_obj);
	if (ret)
		goto out;

	ret = poulsbo_sgx_build_submit_sgx_ta_op_locked(
		dev_priv, submit, PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE, 0, 0,
		ta_op);
	if (ret)
		goto out;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_op_locked(
		dev_priv, submit, ta_op);

out:
	kfree(ta_op);
	return ret;
}

static int poulsbo_sgx_validate_submit_sgx_ta_scheduler_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_object *status_obj)
{
	struct psb_sgx_lab_submit_sgx_ta_scheduler *scheduler;
	int ret;

	scheduler = kmalloc(sizeof(*scheduler), GFP_KERNEL);
	if (!scheduler)
		return -ENOMEM;

	ret = poulsbo_sgx_validate_submit_sgx_ta_op_locked(
		dev_priv, submit, control_obj, status_obj);
	if (ret)
		goto out;

	ret = poulsbo_sgx_build_submit_sgx_ta_scheduler_locked(
		dev_priv, submit, PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE, 0, 0,
		scheduler);
	if (ret)
		goto out;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_scheduler_locked(
		dev_priv, submit, scheduler);

out:
	kfree(scheduler);
	return ret;
}

static int poulsbo_sgx_validate_submit_sgx_ta_sequence_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_object *status_obj)
{
	struct psb_sgx_lab_submit_sgx_ta_sequence *sequence;
	int ret;

	sequence = kmalloc(sizeof(*sequence), GFP_KERNEL);
	if (!sequence)
		return -ENOMEM;

	ret = poulsbo_sgx_validate_submit_sgx_ta_scheduler_locked(
		dev_priv, submit, control_obj, status_obj);
	if (ret)
		goto out;

	ret = poulsbo_sgx_build_submit_sgx_ta_sequence_locked(
		dev_priv, submit, PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE, 0, 0,
		sequence);
	if (ret)
		goto out;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_sequence_locked(
		dev_priv, submit, sequence);

out:
	kfree(sequence);
	return ret;
}

static int poulsbo_sgx_validate_submit_sgx_ta_work_queue_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_object *status_obj)
{
	struct psb_sgx_lab_submit_sgx_ta_work_queue *work_queue;
	int ret;

	work_queue = kmalloc(sizeof(*work_queue), GFP_KERNEL);
	if (!work_queue)
		return -ENOMEM;

	ret = poulsbo_sgx_validate_submit_sgx_ta_sequence_locked(
		dev_priv, submit, control_obj, status_obj);
	if (ret)
		goto out;

	ret = poulsbo_sgx_build_submit_sgx_ta_work_queue_locked(
		dev_priv, submit, PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE, 0, 0,
		work_queue);
	if (ret)
		goto out;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_work_queue_locked(
		dev_priv, submit, work_queue);

out:
	kfree(work_queue);
	return ret;
}

static int poulsbo_sgx_validate_submit_sgx_ta_job_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_object *status_obj)
{
	struct psb_sgx_lab_submit_sgx_ta_job *job;
	int ret;

	job = kmalloc(sizeof(*job), GFP_KERNEL);
	if (!job)
		return -ENOMEM;

	ret = poulsbo_sgx_validate_submit_sgx_ta_work_queue_locked(
		dev_priv, submit, control_obj, status_obj);
	if (ret)
		goto out;

	ret = poulsbo_sgx_build_submit_sgx_ta_job_locked(
		dev_priv, submit, PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE, 0, 0,
		job);
	if (ret)
		goto out;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_job_locked(
		dev_priv, submit, job);

out:
	kfree(job);
	return ret;
}

static int poulsbo_sgx_validate_submit_sgx_ta_request_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_object *status_obj)
{
	struct psb_sgx_lab_submit_sgx_ta_request *request;
	int ret;

	request = kmalloc(sizeof(*request), GFP_KERNEL);
	if (!request)
		return -ENOMEM;

	ret = poulsbo_sgx_validate_submit_sgx_ta_job_locked(
		dev_priv, submit, control_obj, status_obj);
	if (ret)
		goto out;

	ret = poulsbo_sgx_build_submit_sgx_ta_request_locked(
		dev_priv, submit, PSB_SGX_LAB_SUBMIT_SGX_TA_OP_NONE, 0, 0,
		request);
	if (ret)
		goto out;

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_request_locked(
		dev_priv, submit, request);

out:
	kfree(request);
	return ret;
}

static int poulsbo_sgx_validate_submit_locked(struct drm_psb_private *dev_priv,
					      struct psb_sgx_lab_submit *submit)
{
	struct psb_sgx_lab_object *obj;
	struct psb_sgx_lab_object *control_obj = NULL;
	struct psb_sgx_lab_object *status_obj = NULL;
	u32 object_ids[5];
	u32 expected_roles[5];
	u32 expected_classes[5];
	u32 expected_gpu_uses[5];
	unsigned int i, slot;
	int ret;

	if (submit->context == PSB_SGX_LAB_CONTEXT_AUTO)
		return -EINVAL;
	if (submit->cmd_type == PSB_SGX_LAB_CMD_INVALID)
		return -EINVAL;
	for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
		u32 *dependency_submit_id =
			poulsbo_sgx_dependency_submit_id_ptr(submit, slot);

		if (!dependency_submit_id)
			return -EINVAL;
	}
	if (submit->cmd_type != PSB_SGX_LAB_CMD_TA)
		return -EOPNOTSUPP;
	if (submit->ccb_offset & 0xf)
		return -EINVAL;
	if (!(submit->ctrl_flags & PSB_SGX_LAB_CMDTA_CTRLFLAGS_READY))
		return -EINVAL;
	if (submit->ctrl_flags & ~PSB_SGX_LAB_CMDTA_CTRLFLAGS_READY)
		return -EINVAL;
	if (submit->num_ta_status_vals > PSB_SGX_LAB_MAX_STATUS_VALS ||
	    submit->num_3d_status_vals > PSB_SGX_LAB_MAX_STATUS_VALS)
		return -EINVAL;
	if (!submit->queue_config_count)
		return -EINVAL;
	if (!submit->ccb_config_count)
		return -EINVAL;
	if ((submit->num_ta_status_vals || submit->num_3d_status_vals) &&
	    !submit->status_config_count)
		return -EINVAL;

	object_ids[0] = submit->control_object_id;
	expected_roles[0] = PSB_SGX_LAB_ROLE_CONTROL;
	expected_classes[0] = PSB_SGX_LAB_CLASS_PACKET;
	expected_gpu_uses[0] = PSB_SGX_LAB_GPU_USE_CONTROL_CCB;
	object_ids[1] = submit->pds_object_id;
	expected_roles[1] = PSB_SGX_LAB_ROLE_PDS;
	expected_classes[1] = PSB_SGX_LAB_CLASS_CODE;
	expected_gpu_uses[1] = PSB_SGX_LAB_GPU_USE_PDS_CODE;
	object_ids[2] = submit->use_object_id;
	expected_roles[2] = PSB_SGX_LAB_ROLE_USE;
	expected_classes[2] = PSB_SGX_LAB_CLASS_CODE;
	expected_gpu_uses[2] = PSB_SGX_LAB_GPU_USE_USE_CODE;
	object_ids[3] = submit->target_object_id;
	expected_roles[3] = PSB_SGX_LAB_ROLE_TARGET;
	expected_classes[3] = PSB_SGX_LAB_CLASS_SURFACE;
	expected_gpu_uses[3] = PSB_SGX_LAB_GPU_USE_RENDER_TARGET;
	object_ids[4] = submit->status_object_id;
	expected_roles[4] = PSB_SGX_LAB_ROLE_STATUS;
	expected_classes[4] = PSB_SGX_LAB_CLASS_STATE;
	expected_gpu_uses[4] = PSB_SGX_LAB_GPU_USE_STATUS_BLOCK;

	if (!submit->control_object_id || !submit->pds_object_id ||
	    !submit->use_object_id || !submit->target_object_id ||
	    ((submit->num_ta_status_vals || submit->num_3d_status_vals) &&
	     !submit->status_object_id))
		return -EINVAL;
	if ((submit->num_ta_status_vals || submit->num_3d_status_vals) &&
	    (!submit->status_stride || (submit->status_stride & 0x3)))
		return -EINVAL;
	if (submit->report_config_count && (submit->report_offset & 0x3))
		return -EINVAL;
	if (submit->result_config_count && (submit->result_offset & 0x3))
		return -EINVAL;
	if (submit->manifest_config_count && (submit->manifest_offset & 0x3))
		return -EINVAL;
	if (submit->descriptor_config_count &&
	    (submit->descriptor_offset & 0x3))
		return -EINVAL;
	if (submit->plan_config_count && (submit->plan_offset & 0x3))
		return -EINVAL;
	if (submit->issue_plan_config_count &&
	    (submit->issue_plan_offset & 0x3))
		return -EINVAL;
	if ((submit->queue_base_offset & 0xf) || !submit->queue_size ||
	    !submit->queue_packet_size)
		return -EINVAL;
	if ((submit->queue_packet_size & 0xf) ||
	    submit->queue_packet_size < PSB_SGX_LAB_CCB_RECORD_BYTES)
		return -EINVAL;
	if (submit->queue_size < submit->queue_packet_size ||
	    (submit->queue_size % submit->queue_packet_size))
		return -EINVAL;
	if (submit->queue_packet_count >
	    submit->queue_size / submit->queue_packet_size)
		return -EINVAL;
	if (submit->sync_config_count) {
		if (!(submit->sync_flags & PSB_SGX_LAB_SYNC_FLAG_READY))
			return -EINVAL;
		if (submit->sync_flags &
		    ~(PSB_SGX_LAB_SYNC_FLAG_READY |
		      PSB_SGX_LAB_SYNC_FLAG_WRAPPED))
			return -EINVAL;
		if (!!(submit->sync_flags & PSB_SGX_LAB_SYNC_FLAG_WRAPPED) !=
		    !!submit->queue_head_packet_count)
			return -EINVAL;
		if (submit->sync_pending_seq < submit->sync_complete_seq)
			return -EINVAL;
	}
	if (submit->owner_config_count) {
		if (!submit->sync_config_count)
			return -EINVAL;
		if (submit->owner_state != PSB_SGX_LAB_OWNER_QUEUED &&
		    submit->owner_state != PSB_SGX_LAB_OWNER_RUNNING &&
		    submit->owner_state != PSB_SGX_LAB_OWNER_COMPLETE)
			return -EINVAL;
		if (submit->owner_state == PSB_SGX_LAB_OWNER_RUNNING ||
		    submit->owner_state == PSB_SGX_LAB_OWNER_COMPLETE) {
			for (slot = 0;
			     slot < PSB_SGX_LAB_DEPENDENCY_SLOTS;
			     slot++) {
				if (!poulsbo_sgx_dependency_slot_active(
					    submit, slot))
					continue;
				break;
			}
		}
	}
	if (submit->report_config_count) {
		if (!submit->status_object_id)
			return -EINVAL;
	}
	if (submit->result_config_count) {
		if (!submit->status_object_id)
			return -EINVAL;
	}
	if ((submit->num_ta_status_vals && (submit->status_ta_offset & 0x3)) ||
	    (submit->num_3d_status_vals && (submit->status_3d_offset & 0x3)))
		return -EINVAL;
	for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
		u32 *dependency_offset =
			poulsbo_sgx_dependency_offset_ptr(submit, slot);
		u32 *dependency_config_count =
			poulsbo_sgx_dependency_config_count_ptr(submit, slot);

		if (!dependency_offset || !dependency_config_count)
			return -EINVAL;
		if (!*dependency_config_count)
			continue;
		if (!poulsbo_sgx_dependency_required_ticket(submit, slot))
			return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(object_ids); i++) {
		unsigned int j;

		if (!object_ids[i]) {
			if (i == 4)
				continue;
			return -EINVAL;
		}

		obj = poulsbo_sgx_find_object_locked(dev_priv, object_ids[i]);
		if (!obj)
			return -ENOENT;
		if (i == 0)
			control_obj = obj;
		if (i == 4)
			status_obj = obj;
		if (obj->role != expected_roles[i])
			return -EINVAL;
		if (obj->object_class != expected_classes[i])
			return -EINVAL;
		if (obj->gpu_use != expected_gpu_uses[i])
			return -EINVAL;
		if (obj->expected_context != PSB_SGX_LAB_CONTEXT_AUTO &&
		    obj->expected_context != submit->context)
			return -EINVAL;
		if (submit->context == PSB_SGX_LAB_CONTEXT_PF &&
		    obj->gt->sgx_pf_bind_count == 0)
			return -EINVAL;
		if (submit->context == PSB_SGX_LAB_CONTEXT_DEFAULT &&
		    obj->gt->in_gart == 0)
			return -EINVAL;

		for (j = i + 1; j < ARRAY_SIZE(object_ids); j++) {
			if (object_ids[j] && object_ids[i] == object_ids[j])
				return -EINVAL;
		}
	}

	if (!control_obj)
		return -EINVAL;
	ret = poulsbo_sgx_validate_submit_control_layout_locked(
		submit, control_obj, NULL);
		if (ret)
			return ret;
	if ((submit->num_ta_status_vals || submit->num_3d_status_vals ||
	     submit->sync_config_count || submit->owner_config_count ||
	     submit->report_config_count || submit->result_config_count ||
	     submit->dependency_config_count ||
	     submit->dependency_config_count_secondary ||
	     submit->dependency_config_count_tertiary ||
	     submit->dependency_config_count_quaternary) && !status_obj)
		return -EINVAL;
	ret = poulsbo_sgx_validate_submit_status_layout_locked(
		submit, status_obj, NULL);
	if (ret)
		return ret;
	return poulsbo_sgx_validate_submit_sgx_ta_request_locked(
		dev_priv, submit, control_obj, status_obj);
}

static int poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_operation(
	struct drm_device *dev,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_operation
		**return_out)
{
	struct drm_psb_private *dev_priv;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_operation
		*operation;
	int ret;

	if (!dev || !op || !return_out)
		return -EINVAL;

	dev_priv = dev->dev_private;
	if (!dev_priv)
		return -ENODEV;

	*return_out = NULL;
	operation = kmalloc(sizeof(*operation), GFP_KERNEL);
	if (!operation)
		return -ENOMEM;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	ret = poulsbo_sgx_build_submit_sgx_ta_submit_op_request_operation_locked(
		dev_priv, op, dependency_fields_present, operation);
	if (ret)
		goto out_unlock;

	submit = poulsbo_sgx_find_submit_locked(dev_priv,
						operation->resolved_submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}

	ret = poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_operation_locked(
		dev_priv, submit, operation);
	if (ret)
		goto out_unlock;

	ret = poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_operation_locked(
		dev_priv, submit, operation);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (ret) {
		kfree(operation);
		return ret;
	}

	*return_out = operation;
	return 0;
}

static int poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_control(
	struct drm_device *dev,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_control
		**return_out)
{
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_operation
		*operation = NULL;
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_control
		*control;
	int ret;

	if (!dev || !op || !return_out)
		return -EINVAL;

	*return_out = NULL;
	control = kmalloc(sizeof(*control), GFP_KERNEL);
	if (!control)
		return -ENOMEM;

	ret = poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_operation(
		dev, op, dependency_fields_present, &operation);
	if (ret)
		goto out_free_control;

	control->submit_op_request_operation = *operation;
	control->request_op = operation->request_op;
	control->response_op = operation->response_op;
	control->outcome = operation->outcome;
	control->active = operation->active;
	control->dependency_fields_present =
		operation->dependency_fields_present;
	control->issue_request_control =
		operation->issue_request_operation;
	control->transition_request_control =
		operation->transition_request_operation;
	control->resolved_submit_id = operation->resolved_submit_id;
	control->request_control_op = op->op;

	kfree(operation);

	*return_out = control;
	return 0;

out_free_control:
	kfree(control);
	return ret;
}

static int poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_path(
	struct drm_device *dev,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_path **return_out)
{
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_control
		*control = NULL;
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_path *path;
	int ret;

	if (!dev || !op || !return_out)
		return -EINVAL;

	*return_out = NULL;
	path = kmalloc(sizeof(*path), GFP_KERNEL);
	if (!path)
		return -ENOMEM;

	ret = poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_control(
		dev, op, dependency_fields_present, &control);
	if (ret)
		goto out_free_path;

	path->submit_op_request_control = *control;
	path->request_op = control->request_op;
	path->response_op = control->response_op;
	path->outcome = control->outcome;
	path->active = control->active;
	path->dependency_fields_present = control->dependency_fields_present;
	path->issue_request_path = control->issue_request_control;
	path->transition_request_path = control->transition_request_control;
	path->resolved_submit_id = control->resolved_submit_id;
	path->request_path_op = op->op;

	kfree(control);
	*return_out = path;
	return 0;

out_free_path:
	kfree(path);
	return ret;
}

static int poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_report(
	struct drm_device *dev,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_report
		**return_out)
{
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_path *path = NULL;
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_report *report;
	int ret;

	if (!dev || !op || !return_out)
		return -EINVAL;

	*return_out = NULL;
	report = kmalloc(sizeof(*report), GFP_KERNEL);
	if (!report)
		return -ENOMEM;

	ret = poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_path(
		dev, op, dependency_fields_present, &path);
	if (ret)
		goto out_free_report;

	report->submit_op_request_path = *path;
	report->request_op = path->request_op;
	report->response_op = path->response_op;
	report->outcome = path->outcome;
	report->active = path->active;
	report->dependency_fields_present =
		path->dependency_fields_present;
	report->issue_request_report = path->issue_request_path;
	report->transition_request_report =
		path->transition_request_path;
	report->resolved_submit_id = path->resolved_submit_id;
	report->request_report_op = op->op;

	kfree(path);
	*return_out = report;
	return 0;

out_free_report:
	kfree(report);
	return ret;
}

static int poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_trace(
	struct drm_device *dev,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_trace **return_out)
{
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_report
		*report = NULL;
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_trace *trace;
	int ret;

	if (!dev || !op || !return_out)
		return -EINVAL;

	*return_out = NULL;
	trace = kmalloc(sizeof(*trace), GFP_KERNEL);
	if (!trace)
		return -ENOMEM;

	ret = poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_report(
		dev, op, dependency_fields_present, &report);
	if (ret)
		goto out_free_trace;

	trace->submit_op_request_report = *report;
	trace->request_op = report->request_op;
	trace->response_op = report->response_op;
	trace->outcome = report->outcome;
	trace->active = report->active;
	trace->dependency_fields_present =
		report->dependency_fields_present;
	trace->issue_request_trace = report->issue_request_report;
	trace->transition_request_trace =
		report->transition_request_report;
	trace->resolved_submit_id = report->resolved_submit_id;
	trace->request_trace_op = op->op;

	kfree(report);
	*return_out = trace;
	return 0;

out_free_trace:
	kfree(trace);
	return ret;
}

static int poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_frontend(
	struct drm_device *dev,
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_frontend **return_out)
{
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_trace
		*trace = NULL;
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_frontend *frontend;
	int ret;

	if (!dev || !op || !return_out)
		return -EINVAL;

	*return_out = NULL;
	frontend = kmalloc(sizeof(*frontend), GFP_KERNEL);
	if (!frontend)
		return -ENOMEM;

	ret = poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_trace(
		dev, op, dependency_fields_present, &trace);
	if (ret)
		goto out_free_frontend;

	frontend->submit_op_request_trace = *trace;
	frontend->request_op = trace->request_op;
	frontend->response_op = trace->response_op;
	frontend->outcome = trace->outcome;
	frontend->active = trace->active;
	frontend->dependency_fields_present =
		trace->dependency_fields_present;
	frontend->issue_request_frontend = trace->issue_request_trace;
	frontend->transition_request_frontend =
		trace->transition_request_trace;
	frontend->resolved_submit_id = trace->resolved_submit_id;
	frontend->request_frontend_op = op->op;

	kfree(trace);
	*return_out = frontend;
	return 0;

out_free_frontend:
	kfree(frontend);
	return ret;
}

static int poulsbo_sgx_prepare_queue_packets_locked(
	struct psb_sgx_lab_object *control_obj,
	struct psb_sgx_lab_submit *submit,
	const struct psb_sgx_lab_ccb_record_words *ccb_record)
{
	u32 packet_index;
	u32 write_offset = submit->ccb_read_offset;
	int ret;

	for (packet_index = 0; packet_index < submit->queue_packet_count;
	     packet_index++) {
		u32 slot_offset = submit->queue_base_offset + write_offset;

		ret = poulsbo_sgx_object_access_ok(control_obj, slot_offset,
					   PSB_SGX_LAB_CCB_RECORD_BYTES);
		if (ret)
			return ret;
		ret = poulsbo_sgx_write_submit_ccb_record_locked(
			control_obj, slot_offset, ccb_record);
		if (ret)
			return ret;

		write_offset += submit->queue_packet_size;
		if (write_offset >= submit->queue_size)
			write_offset -= submit->queue_size;
	}

	return 0;
}

static int poulsbo_sgx_prepare_status_values_locked(
	struct psb_sgx_lab_object *status_obj,
	struct psb_sgx_lab_submit *submit)
{
	u32 index;
	int ret;

	for (index = 0; index < submit->num_ta_status_vals; index++) {
		ret = poulsbo_sgx_object_write_u32_locked(
			status_obj, submit->status_ta_offset +
			index * submit->status_stride,
			poulsbo_sgx_lab_status_value(submit->status_value_seed,
						     false, index));
		if (ret)
			return ret;
	}

	for (index = 0; index < submit->num_3d_status_vals; index++) {
		ret = poulsbo_sgx_object_write_u32_locked(
			status_obj, submit->status_3d_offset +
			index * submit->status_stride,
			poulsbo_sgx_lab_status_value(submit->status_value_seed,
						     true, index));
		if (ret)
			return ret;
	}

	return 0;
}

static int poulsbo_sgx_prepare_sync_block_locked(
	struct psb_sgx_lab_object *status_obj,
	struct psb_sgx_lab_submit *submit)
{
	struct psb_sgx_lab_sync_words sync;
	u32 sync_region_end;
	int ret;

	ret = poulsbo_sgx_sync_region_end(submit->sync_offset, &sync_region_end);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_access_ok(status_obj, submit->sync_offset,
					   sync_region_end - submit->sync_offset);
	if (ret)
		return ret;

	ret = poulsbo_sgx_build_submit_sync_locked(submit, &sync);
	if (ret)
		return ret;
	ret = poulsbo_sgx_write_submit_sync_locked(status_obj,
						   submit->sync_offset,
						   &sync);
	if (ret)
		return ret;

	submit->sync_flags = sync.flags;
	return 0;
}

static int poulsbo_sgx_prepare_owner_block_locked(
	struct psb_sgx_lab_object *status_obj,
	struct psb_sgx_lab_submit *submit)
{
	struct psb_sgx_lab_owner_words owner;
	u32 owner_region_end;
	int ret;

	ret = poulsbo_sgx_owner_region_end(submit->owner_offset, &owner_region_end);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_access_ok(status_obj, submit->owner_offset,
					   owner_region_end -
					   submit->owner_offset);
	if (ret)
		return ret;

	ret = poulsbo_sgx_build_submit_owner_locked(submit, &owner);
	if (ret)
		return ret;
	return poulsbo_sgx_write_submit_owner_locked(status_obj,
						     submit->owner_offset,
						     &owner);
}

static int poulsbo_sgx_prepare_submit_locked(struct drm_psb_private *dev_priv,
					     struct psb_sgx_lab_submit *submit)
{
	struct psb_sgx_lab_object *control_obj;
	struct psb_sgx_lab_object *status_obj = NULL;
	struct psb_sgx_lab_queue_header_words queue_header;
	struct psb_sgx_lab_ccb_record_words ccb_record;
	u32 queue_region_end;
	u32 expected_tail_packets;
	u32 expected_head_packets;
	u32 expected_write_offset;
	int ret;

	poulsbo_sgx_disarm_submit_locked(submit);

	if (!submit->control_object_id || !submit->packet_config_count ||
	    !submit->ccb_config_count || !submit->queue_config_count)
		return -EINVAL;
	if ((submit->status_config_count || submit->sync_config_count ||
	     submit->owner_config_count || submit->dependency_config_count ||
	     submit->dependency_config_count_secondary ||
	     submit->dependency_config_count_tertiary ||
	     submit->dependency_config_count_quaternary) &&
	    !submit->status_object_id)
		return -EINVAL;

	poulsbo_sgx_disarm_submit_locked(submit);

	control_obj = poulsbo_sgx_find_object_locked(dev_priv,
						     submit->control_object_id);
	if (!control_obj)
		return -ENOENT;
	if (submit->status_object_id) {
		status_obj = poulsbo_sgx_find_object_locked(
			dev_priv, submit->status_object_id);
		if (!status_obj)
			return -ENOENT;
	}

	ret = poulsbo_sgx_queue_progression(submit->ccb_read_offset,
					    submit->queue_size,
					    submit->queue_packet_size,
					    submit->queue_packet_count,
					    &expected_tail_packets,
					    &expected_head_packets,
					    &expected_write_offset);
	if (ret)
		return ret;
	if (submit->ccb_offset !=
	    submit->queue_base_offset + submit->ccb_read_offset)
		return -EINVAL;
	ret = poulsbo_sgx_queue_region_end(submit->queue_base_offset,
					   submit->queue_size,
					   &queue_region_end);
	if (ret)
		return ret;
	if (queue_region_end > control_obj->gt->gem.size ||
	    submit->ccb_offset + PSB_SGX_LAB_CCB_RECORD_BYTES >
		    queue_region_end)
		return -EINVAL;

	submit->ccb_write_offset = expected_write_offset;
	submit->queue_tail_packet_count = expected_tail_packets;
	submit->queue_head_packet_count = expected_head_packets;

	ret = poulsbo_sgx_object_access_ok(control_obj, 0, 32);
	if (ret)
		return ret;
	ret = poulsbo_sgx_object_access_ok(control_obj, submit->ccb_offset,
					   PSB_SGX_LAB_CCB_RECORD_BYTES);
	if (ret)
		return ret;

	ret = poulsbo_sgx_build_submit_queue_header_locked(submit,
							    &queue_header);
	if (ret)
		return ret;
	ret = poulsbo_sgx_build_submit_ccb_record_locked(dev_priv, submit,
							 &ccb_record);
	if (ret)
		return ret;
	ret = poulsbo_sgx_write_queue_header_locked(control_obj, 0,
						    &queue_header);
	if (ret)
		return ret;
	ret = poulsbo_sgx_write_submit_ccb_record_locked(
		control_obj, submit->ccb_offset, &ccb_record);
	if (ret)
		return ret;

	ret = poulsbo_sgx_prepare_queue_packets_locked(control_obj, submit,
							 &ccb_record);
	if (ret)
		return ret;

	if (submit->status_config_count) {
		if (!status_obj)
			return -EINVAL;
		ret = poulsbo_sgx_prepare_status_values_locked(status_obj, submit);
		if (ret)
			return ret;
	}
	if (submit->sync_config_count) {
		if (!status_obj)
			return -EINVAL;
		ret = poulsbo_sgx_prepare_sync_block_locked(status_obj, submit);
		if (ret)
			return ret;
	}
	if (submit->owner_config_count) {
		if (!status_obj)
			return -EINVAL;
		ret = poulsbo_sgx_prepare_owner_block_locked(status_obj, submit);
		if (ret)
			return ret;
	}
	if (submit->dependency_config_count ||
	    submit->dependency_config_count_secondary ||
	    submit->dependency_config_count_tertiary ||
	    submit->dependency_config_count_quaternary) {
		ret = poulsbo_sgx_refresh_dependency_blocks_locked(dev_priv,
								   submit);
		if (ret)
			return ret;
	}

	ret = poulsbo_sgx_validate_submit_locked(dev_priv, submit);
	if (ret)
		return ret;

	submit->prepare_count++;
	submit->last_prepare_jiffies = jiffies;
	return poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);
}

static int poulsbo_sgx_arm_submit_locked(struct drm_psb_private *dev_priv,
					 struct psb_sgx_lab_submit *submit)
{
	int ret;

	if (!submit->prepare_count)
		return -EINVAL;
	if (!submit->control_object_id || !submit->status_object_id ||
	    !submit->queue_config_count || !submit->ccb_config_count ||
	    !submit->sync_config_count || !submit->owner_config_count)
		return -EINVAL;

	ret = poulsbo_sgx_validate_submit_locked(dev_priv, submit);
	if (ret)
		return ret;

	submit->arm_valid = true;
	submit->arm_prepare_count = submit->prepare_count;
	submit->arm_queue_packet_count = submit->queue_packet_count;
	submit->arm_queue_tail_packet_count = submit->queue_tail_packet_count;
	submit->arm_queue_head_packet_count = submit->queue_head_packet_count;
	submit->arm_ccb_read_offset = submit->ccb_read_offset;
	submit->arm_ccb_write_offset = submit->ccb_write_offset;
	submit->arm_sync_complete_seq = submit->sync_complete_seq;
	submit->arm_sync_pending_seq = submit->sync_pending_seq;
	submit->arm_owner_state = submit->owner_state;
	submit->arm_owner_issue_cookie = submit->owner_issue_cookie;
	submit->arm_owner_complete_cookie = submit->owner_complete_cookie;
	submit->arm_queue_epoch = submit->queue_epoch;
	submit->arm_global_issue_ticket = submit->global_issue_ticket;
	submit->arm_count++;
	submit->last_arm_jiffies = jiffies;
	return poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);
}

static int poulsbo_sgx_emit_submit_locked(struct drm_psb_private *dev_priv,
					  struct psb_sgx_lab_submit *submit)
{
	struct psb_sgx_lab_object *control_obj;
	struct psb_sgx_lab_object *pds_obj = NULL;
	struct psb_sgx_lab_object *use_obj = NULL;
	struct psb_sgx_lab_object *target_obj = NULL;
	struct psb_sgx_lab_object *status_obj;
	int ret;

	if (!submit->arm_valid)
		return -EINVAL;
	if (!submit->control_object_id || !submit->status_object_id ||
	    !submit->queue_config_count || !submit->ccb_config_count ||
	    !submit->sync_config_count || !submit->owner_config_count)
		return -EINVAL;
	if (!poulsbo_sgx_submit_arm_matches_locked(submit)) {
		poulsbo_sgx_disarm_submit_locked(submit);
		return -ESTALE;
	}

	ret = poulsbo_sgx_validate_submit_locked(dev_priv, submit);
	if (ret) {
		poulsbo_sgx_disarm_submit_locked(submit);
		return ret;
	}

	control_obj = poulsbo_sgx_find_object_locked(dev_priv,
						     submit->control_object_id);
	status_obj = poulsbo_sgx_find_object_locked(dev_priv,
						    submit->status_object_id);
	if (!control_obj || !status_obj) {
		poulsbo_sgx_disarm_submit_locked(submit);
		return -ENOENT;
	}

	if (submit->pds_object_id) {
		pds_obj = poulsbo_sgx_find_object_locked(dev_priv,
							 submit->pds_object_id);
		if (!pds_obj) {
			poulsbo_sgx_disarm_submit_locked(submit);
			return -ENOENT;
		}
	}
	if (submit->use_object_id) {
		use_obj = poulsbo_sgx_find_object_locked(dev_priv,
							 submit->use_object_id);
		if (!use_obj) {
			poulsbo_sgx_disarm_submit_locked(submit);
			return -ENOENT;
		}
	}
	if (submit->target_object_id) {
		target_obj = poulsbo_sgx_find_object_locked(
			dev_priv, submit->target_object_id);
		if (!target_obj) {
			poulsbo_sgx_disarm_submit_locked(submit);
			return -ENOENT;
		}
	}

	poulsbo_sgx_unemit_submit_locked(submit);

	submit->emit_prepare_count = submit->prepare_count;
	submit->emit_arm_count = submit->arm_count;
	submit->emit_context = submit->context;
	submit->emit_cmd_type = submit->cmd_type;
	submit->emit_owner_state = submit->owner_state;
	submit->emit_ctrl_flags = submit->ctrl_flags;
	submit->emit_num_ta_status_vals = submit->num_ta_status_vals;
	submit->emit_num_3d_status_vals = submit->num_3d_status_vals;
	submit->emit_ta_dependency = submit->ta_dependency;
	submit->emit_control_object_id = submit->control_object_id;
	submit->emit_pds_object_id = submit->pds_object_id;
	submit->emit_use_object_id = submit->use_object_id;
	submit->emit_target_object_id = submit->target_object_id;
	submit->emit_status_object_id = submit->status_object_id;
	submit->emit_ccb_offset = submit->ccb_offset;
	submit->emit_ccb_write_offset = submit->ccb_write_offset;
	submit->emit_ccb_read_offset = submit->ccb_read_offset;
	submit->emit_ccb_service_address = submit->ccb_service_address;
	submit->emit_ccb_cache_control = submit->ccb_cache_control;
	submit->emit_ccb_data0 = submit->ccb_data0;
	submit->emit_ccb_data1 = submit->ccb_data1;
	submit->emit_queue_base_offset = submit->queue_base_offset;
	submit->emit_queue_size = submit->queue_size;
	submit->emit_queue_packet_size = submit->queue_packet_size;
	submit->emit_queue_packet_count = submit->queue_packet_count;
	submit->emit_queue_tail_packet_count = submit->queue_tail_packet_count;
	submit->emit_queue_head_packet_count = submit->queue_head_packet_count;
	submit->emit_sync_complete_seq = submit->sync_complete_seq;
	submit->emit_sync_pending_seq = submit->sync_pending_seq;
	submit->emit_sync_flags = submit->sync_flags;
	submit->emit_owner_issue_cookie = submit->owner_issue_cookie;
	submit->emit_owner_complete_cookie = submit->owner_complete_cookie;
	submit->emit_queue_epoch = submit->queue_epoch;
	submit->emit_global_issue_ticket = submit->global_issue_ticket;
	submit->emit_dependency_active_count =
		poulsbo_sgx_submit_dependency_active_count_locked(submit);
	submit->emit_control_gpu_va =
		poulsbo_sgx_object_gpu_va(dev_priv, control_obj);
	submit->emit_pds_gpu_va = pds_obj ?
		poulsbo_sgx_object_gpu_va(dev_priv, pds_obj) : 0;
	submit->emit_use_gpu_va = use_obj ?
		poulsbo_sgx_object_gpu_va(dev_priv, use_obj) : 0;
	submit->emit_target_gpu_va = target_obj ?
		poulsbo_sgx_object_gpu_va(dev_priv, target_obj) : 0;
	submit->emit_status_gpu_va =
		poulsbo_sgx_object_gpu_va(dev_priv, status_obj);
	submit->emit_valid = true;
	submit->emit_count++;
	submit->last_emit_jiffies = jiffies;
	return poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);
}

static int poulsbo_sgx_launch_submit_locked(struct drm_psb_private *dev_priv,
					    struct psb_sgx_lab_submit *submit,
					    u32 owner_state,
					    u32 issued_packets)
{
	struct psb_sgx_lab_issue_plan plan;

	if (!submit->emit_valid)
		return -EINVAL;
	if (!poulsbo_sgx_submit_emit_matches_locked(dev_priv, submit)) {
		poulsbo_sgx_disarm_submit_locked(submit);
		return -ESTALE;
	}

	if (!poulsbo_sgx_build_issue_plan_locked(dev_priv, submit, owner_state,
						 issued_packets, &plan))
		return -EINVAL;

	poulsbo_sgx_unlaunch_submit_locked(submit);

	submit->launch_prepare_count = submit->prepare_count;
	submit->launch_arm_count = submit->arm_count;
	submit->launch_emit_count = submit->emit_count;
	submit->launch_context = submit->emit_context;
	submit->launch_cmd_type = submit->emit_cmd_type;
	submit->launch_owner_state = plan.owner_state;
	submit->launch_issued_packets = plan.issued_packets;
	submit->launch_refilled_from_complete =
		plan.refilled_from_complete ? 1 : 0;
	submit->launch_ctrl_flags = submit->emit_ctrl_flags;
	submit->launch_num_ta_status_vals = submit->emit_num_ta_status_vals;
	submit->launch_num_3d_status_vals = submit->emit_num_3d_status_vals;
	submit->launch_ta_dependency = submit->emit_ta_dependency;
	submit->launch_control_object_id = submit->emit_control_object_id;
	submit->launch_pds_object_id = submit->emit_pds_object_id;
	submit->launch_use_object_id = submit->emit_use_object_id;
	submit->launch_target_object_id = submit->emit_target_object_id;
	submit->launch_status_object_id = submit->emit_status_object_id;
	submit->launch_ccb_offset = submit->emit_ccb_offset;
	submit->launch_ccb_write_offset = submit->emit_ccb_write_offset;
	submit->launch_ccb_read_offset = submit->emit_ccb_read_offset;
	submit->launch_new_write_offset = plan.new_write_offset;
	submit->launch_ccb_service_address = submit->emit_ccb_service_address;
	submit->launch_ccb_cache_control = submit->emit_ccb_cache_control;
	submit->launch_ccb_data0 = submit->emit_ccb_data0;
	submit->launch_ccb_data1 = submit->emit_ccb_data1;
	submit->launch_queue_base_offset = submit->emit_queue_base_offset;
	submit->launch_queue_size = submit->emit_queue_size;
	submit->launch_queue_packet_size = submit->emit_queue_packet_size;
	submit->launch_queue_packet_count = plan.new_packet_count;
	submit->launch_queue_tail_packet_count = plan.new_tail_packets;
	submit->launch_queue_head_packet_count = plan.new_head_packets;
	submit->launch_sync_offset = submit->sync_offset;
	submit->launch_sync_complete_seq = submit->emit_sync_complete_seq;
	submit->launch_sync_pending_seq = plan.new_sync_pending_seq;
	submit->launch_sync_flags = plan.new_sync_flags;
	submit->launch_owner_offset = submit->owner_offset;
	submit->launch_owner_issue_cookie = plan.new_owner_issue_cookie;
	submit->launch_owner_complete_cookie =
		submit->emit_owner_complete_cookie;
	submit->launch_queue_epoch = plan.new_queue_epoch;
	submit->launch_global_issue_ticket = plan.new_global_issue_ticket;
	submit->launch_global_running_ticket = submit->global_running_ticket;
	submit->launch_global_complete_ticket = submit->global_complete_ticket;
	submit->launch_dependency_active_count =
		submit->emit_dependency_active_count;
	submit->launch_control_gpu_va = submit->emit_control_gpu_va;
	submit->launch_pds_gpu_va = submit->emit_pds_gpu_va;
	submit->launch_use_gpu_va = submit->emit_use_gpu_va;
	submit->launch_target_gpu_va = submit->emit_target_gpu_va;
	submit->launch_status_gpu_va = submit->emit_status_gpu_va;
	submit->launch_valid = true;
	submit->launch_consumed = false;
	submit->launch_count++;
	submit->last_launch_jiffies = jiffies;
	return poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);
}

static int poulsbo_sgx_prepare_submit(struct drm_device *dev, u32 submit_id)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	u32 prepare_count;
	u32 queue_packet_count;
	u32 ccb_write_offset;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}

	ret = poulsbo_sgx_prepare_submit_locked(dev_priv, submit);
	if (ret)
		goto out_unlock;

	queue_packet_count = submit->queue_packet_count;
	ccb_write_offset = submit->ccb_write_offset;
	prepare_count = submit->prepare_count;
	ret = 0;

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit prepare submit_id=%u packet_count=%u write_offset=%#x prepare_count=%u\n",
			 submit_id, queue_packet_count, ccb_write_offset,
			 prepare_count);
	return ret;
}

static int poulsbo_sgx_arm_submit(struct drm_device *dev, u32 submit_id)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	u32 arm_prepare_count;
	u32 arm_queue_packet_count;
	u32 arm_ccb_write_offset;
	u32 arm_global_issue_ticket;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}

	ret = poulsbo_sgx_arm_submit_locked(dev_priv, submit);
	if (ret)
		goto out_unlock;

	arm_prepare_count = submit->arm_prepare_count;
	arm_queue_packet_count = submit->arm_queue_packet_count;
	arm_ccb_write_offset = submit->arm_ccb_write_offset;
	arm_global_issue_ticket = submit->arm_global_issue_ticket;
	ret = 0;

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit arm submit_id=%u prepare_count=%u packet_count=%u write_offset=%#x global_issue_ticket=%u\n",
			 submit_id, arm_prepare_count, arm_queue_packet_count,
			 arm_ccb_write_offset, arm_global_issue_ticket);
	return ret;
}

static int poulsbo_sgx_emit_submit(struct drm_device *dev, u32 submit_id)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	u32 emit_arm_count;
	u32 emit_queue_packet_count;
	u32 emit_global_issue_ticket;
	u32 emit_dependency_active_count;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}

	ret = poulsbo_sgx_emit_submit_locked(dev_priv, submit);
	if (ret)
		goto out_unlock;

	emit_arm_count = submit->emit_arm_count;
	emit_queue_packet_count = submit->emit_queue_packet_count;
	emit_global_issue_ticket = submit->emit_global_issue_ticket;
	emit_dependency_active_count = submit->emit_dependency_active_count;
	ret = 0;

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit emit submit_id=%u arm_count=%u packet_count=%u global_issue_ticket=%u dependency_active_count=%u\n",
			 submit_id, emit_arm_count, emit_queue_packet_count,
			 emit_global_issue_ticket, emit_dependency_active_count);
	return ret;
}

static int poulsbo_sgx_launch_submit(struct drm_device *dev, u32 submit_id,
				     u32 owner_state, u32 issued_packets)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	u32 launch_emit_count;
	u32 launch_queue_packet_count;
	u32 launch_global_issue_ticket;
	u32 launch_dependency_active_count;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}

	ret = poulsbo_sgx_launch_submit_locked(dev_priv, submit, owner_state,
					       issued_packets);
	if (ret)
		goto out_unlock;

	launch_emit_count = submit->launch_emit_count;
	launch_queue_packet_count = submit->launch_queue_packet_count;
	launch_global_issue_ticket = submit->launch_global_issue_ticket;
	launch_dependency_active_count =
		submit->launch_dependency_active_count;
	ret = 0;

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit launch submit_id=%u emit_count=%u packet_count=%u global_issue_ticket=%u dependency_active_count=%u\n",
			 submit_id, launch_emit_count, launch_queue_packet_count,
			 launch_global_issue_ticket,
			 launch_dependency_active_count);
	return ret;
}

static int poulsbo_sgx_configure_submit_kick(struct drm_device *dev, u32 submit_id,
					     u32 cmd_type, u32 ccb_offset,
					     u32 ctrl_flags,
					     u32 num_ta_status_vals,
					     u32 num_3d_status_vals,
					     u32 ta_dependency)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *control_obj = NULL;
	u32 old_cmd_type;
	u32 old_ccb_offset;
	u32 old_ctrl_flags;
	u32 old_num_ta_status_vals;
	u32 old_num_3d_status_vals;
	u32 old_ta_dependency;
	u32 old_packet_config_count;
	unsigned int slot;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);
	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	for (slot = 0; slot < PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
		u32 *dependency_config_count =
			poulsbo_sgx_dependency_config_count_ptr(submit, slot);
		u32 required_ticket;

		if (!dependency_config_count)
			continue;
		if (!*dependency_config_count)
			continue;
		required_ticket = slot == 0 ? ta_dependency :
			poulsbo_sgx_dependency_required_ticket(submit, slot);
		if (!required_ticket || !submit->status_object_id) {
			ret = -EINVAL;
			goto out_unlock;
		}
	}
	if (submit->control_object_id) {
		control_obj = poulsbo_sgx_find_object_locked(
			dev_priv, submit->control_object_id);
		if (!control_obj) {
			ret = -ENOENT;
			goto out_unlock;
		}
		old_cmd_type = submit->cmd_type;
		old_ccb_offset = submit->ccb_offset;
		old_ctrl_flags = submit->ctrl_flags;
		old_num_ta_status_vals = submit->num_ta_status_vals;
		old_num_3d_status_vals = submit->num_3d_status_vals;
		old_ta_dependency = submit->ta_dependency;
		old_packet_config_count = submit->packet_config_count;
		submit->cmd_type = cmd_type;
		submit->ccb_offset = ccb_offset;
		submit->ctrl_flags = ctrl_flags;
		submit->num_ta_status_vals = num_ta_status_vals;
		submit->num_3d_status_vals = num_3d_status_vals;
		submit->ta_dependency = ta_dependency;
		submit->packet_config_count++;
		ret = poulsbo_sgx_validate_submit_control_layout_locked(
			submit, control_obj, NULL);
		submit->cmd_type = old_cmd_type;
		submit->ccb_offset = old_ccb_offset;
		submit->ctrl_flags = old_ctrl_flags;
		submit->num_ta_status_vals = old_num_ta_status_vals;
		submit->num_3d_status_vals = old_num_3d_status_vals;
		submit->ta_dependency = old_ta_dependency;
		submit->packet_config_count = old_packet_config_count;
		if (ret)
			goto out_unlock;
	}

	poulsbo_sgx_disarm_submit_locked(submit);
	submit->cmd_type = cmd_type;
	submit->ccb_offset = ccb_offset;
	submit->ctrl_flags = ctrl_flags;
	submit->num_ta_status_vals = num_ta_status_vals;
	submit->num_3d_status_vals = num_3d_status_vals;
	submit->ta_dependency = ta_dependency;
	submit->packet_config_count++;
	submit->last_packet_jiffies = jiffies;
	ret = poulsbo_sgx_refresh_dependency_blocks_locked(dev_priv, submit);
	if (!ret)
		ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
	if (ret)
		return ret;

	dev_info(dev->dev,
		 "lab submit kick submit_id=%u cmd=%s ccb_offset=%#x ctrl_flags=%#x ta_status=%u 3d_status=%u ta_dependency=%u\n",
		 submit_id, poulsbo_sgx_lab_cmd_type_name(cmd_type),
		 ccb_offset, ctrl_flags, num_ta_status_vals,
		 num_3d_status_vals, ta_dependency);
	return 0;
}

static int poulsbo_sgx_configure_submit_dependency_slot(
	struct drm_device *dev,
	u32 submit_id,
	unsigned int slot,
	u32 dependency_submit_id)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_submit *dependency_submit = NULL;
	u32 *submit_dependency_submit_id;
	u32 *dependency_config_count;
	unsigned long *last_dependency_jiffies;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);
	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	submit_dependency_submit_id =
		poulsbo_sgx_dependency_submit_id_ptr(submit, slot);
	dependency_config_count =
		poulsbo_sgx_dependency_config_count_ptr(submit, slot);
	last_dependency_jiffies =
		poulsbo_sgx_last_dependency_jiffies_ptr(submit, slot);
	if (!submit_dependency_submit_id || !dependency_config_count ||
	    !last_dependency_jiffies) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (*dependency_config_count && !submit->status_object_id) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (dependency_submit_id) {
		if (dependency_submit_id == submit_id) {
			ret = -EINVAL;
			goto out_unlock;
		}
		dependency_submit =
			poulsbo_sgx_find_submit_locked(dev_priv,
						       dependency_submit_id);
		if (!dependency_submit) {
			ret = -ENOENT;
			goto out_unlock;
		}
	}
	poulsbo_sgx_disarm_submit_locked(submit);
	*submit_dependency_submit_id = dependency_submit_id;
	*last_dependency_jiffies = jiffies;
	ret = poulsbo_sgx_refresh_dependency_blocks_locked(dev_priv, submit);
	if (!ret)
		ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
	if (ret)
		return ret;

	dev_info(dev->dev,
		 "lab submit dependency%u submit_id=%u dependency_submit_id=%u\n",
		 slot + 1, submit_id, dependency_submit_id);
	return 0;
}

static int poulsbo_sgx_configure_submit_dependency(struct drm_device *dev,
						   u32 submit_id,
						   u32 dependency_submit_id)
{
	return poulsbo_sgx_configure_submit_dependency_slot(dev, submit_id, 0,
							    dependency_submit_id);
}

static int poulsbo_sgx_configure_submit_dependency2(struct drm_device *dev,
						    u32 submit_id,
						    u32 dependency_submit_id)
{
	return poulsbo_sgx_configure_submit_dependency_slot(dev, submit_id, 1,
							    dependency_submit_id);
}

static int poulsbo_sgx_configure_submit_dependency3(struct drm_device *dev,
						    u32 submit_id,
						    u32 dependency_submit_id)
{
	return poulsbo_sgx_configure_submit_dependency_slot(dev, submit_id, 2,
							    dependency_submit_id);
}

static int poulsbo_sgx_dependency_slot_from_user(u32 slot_index,
						 unsigned int *slot_out)
{
	if (!slot_index || slot_index > PSB_SGX_LAB_DEPENDENCY_SLOTS)
		return -EINVAL;
	if (slot_out)
		*slot_out = slot_index - 1;
	return 0;
}

static int poulsbo_sgx_configure_submit_dependency_slot_user(
	struct drm_device *dev,
	u32 submit_id,
	u32 slot_index,
	u32 dependency_submit_id)
{
	unsigned int slot;
	int ret;

	ret = poulsbo_sgx_dependency_slot_from_user(slot_index, &slot);
	if (ret)
		return ret;

	return poulsbo_sgx_configure_submit_dependency_slot(dev, submit_id, slot,
							    dependency_submit_id);
}

static int poulsbo_sgx_configure_submit_dependency_ticket2(
	struct drm_device *dev,
	u32 submit_id,
	u32 dependency_ticket_secondary)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);
	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	if (submit->dependency_config_count_secondary &&
	    !submit->status_object_id) {
		ret = -EINVAL;
		goto out_unlock;
	}
	poulsbo_sgx_disarm_submit_locked(submit);
	submit->dependency_ticket_secondary = dependency_ticket_secondary;
	submit->last_dependency_secondary_jiffies = jiffies;
	ret = poulsbo_sgx_refresh_dependency_blocks_locked(dev_priv, submit);
	if (!ret)
		ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
	if (ret)
		return ret;

	dev_info(dev->dev,
		 "lab submit depticket2 submit_id=%u dependency2_ticket=%u\n",
		 submit_id, dependency_ticket_secondary);
	return 0;
}

static int poulsbo_sgx_configure_submit_dependency_ticket3(
	struct drm_device *dev,
	u32 submit_id,
	u32 dependency_ticket_tertiary)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);
	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	if (submit->dependency_config_count_tertiary &&
	    !submit->status_object_id) {
		ret = -EINVAL;
		goto out_unlock;
	}
	poulsbo_sgx_disarm_submit_locked(submit);
	submit->dependency_ticket_tertiary = dependency_ticket_tertiary;
	submit->last_dependency_tertiary_jiffies = jiffies;
	ret = poulsbo_sgx_refresh_dependency_blocks_locked(dev_priv, submit);
	if (!ret)
		ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
	if (ret)
		return ret;

	dev_info(dev->dev,
		 "lab submit depticket3 submit_id=%u dependency3_ticket=%u\n",
		 submit_id, dependency_ticket_tertiary);
	return 0;
}

static int poulsbo_sgx_configure_submit_dependency_ticket_slot(
	struct drm_device *dev,
	u32 submit_id,
	u32 slot_index,
	u32 dependency_ticket)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	u32 *dependency_config_count;
	unsigned long *last_dependency_jiffies;
	unsigned int slot;
	int ret;

	ret = poulsbo_sgx_dependency_slot_from_user(slot_index, &slot);
	if (ret)
		return ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);
	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	dependency_config_count =
		poulsbo_sgx_dependency_config_count_ptr(submit, slot);
	last_dependency_jiffies =
		poulsbo_sgx_last_dependency_jiffies_ptr(submit, slot);
	if (!dependency_config_count || !last_dependency_jiffies) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (*dependency_config_count &&
	    (!dependency_ticket || !submit->status_object_id)) {
		ret = -EINVAL;
		goto out_unlock;
	}
	switch (slot) {
	case 0:
		submit->ta_dependency = dependency_ticket;
		break;
	case 1:
		submit->dependency_ticket_secondary = dependency_ticket;
		break;
	case 2:
		submit->dependency_ticket_tertiary = dependency_ticket;
		break;
	case 3:
		submit->dependency_ticket_quaternary = dependency_ticket;
		break;
	default:
		ret = -EINVAL;
		goto out_unlock;
	}
	*last_dependency_jiffies = jiffies;
	poulsbo_sgx_disarm_submit_locked(submit);
	ret = poulsbo_sgx_refresh_dependency_blocks_locked(dev_priv, submit);
	if (!ret)
		ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
	if (ret)
		return ret;

	dev_info(dev->dev,
		 "lab submit depticket_slot submit_id=%u slot=%u required_ticket=%u\n",
		 submit_id, slot_index, dependency_ticket);
	return 0;
}

static int poulsbo_sgx_configure_submit_dependency_state_slot(
	struct drm_device *dev,
	u32 submit_id,
	unsigned int slot,
	u32 dependency_offset)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *status_obj;
	u32 *submit_dependency_submit_id;
	u32 *submit_dependency_offset;
	u32 *dependency_config_count;
	u32 *snapshot_source_submit_id;
	u32 *snapshot_required_ticket;
	u32 *snapshot_observed_complete_ticket;
	u32 *snapshot_flags;
	u32 old_dependency_offset;
	u32 old_dependency_config_count;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	submit_dependency_submit_id =
		poulsbo_sgx_dependency_submit_id_ptr(submit, slot);
	submit_dependency_offset =
		poulsbo_sgx_dependency_offset_ptr(submit, slot);
	dependency_config_count =
		poulsbo_sgx_dependency_config_count_ptr(submit, slot);
	snapshot_source_submit_id =
		poulsbo_sgx_dependency_snapshot_source_submit_id_ptr(submit,
								      slot);
	snapshot_required_ticket =
		poulsbo_sgx_dependency_snapshot_required_ticket_ptr(submit,
								   slot);
	snapshot_observed_complete_ticket =
		poulsbo_sgx_dependency_snapshot_observed_complete_ticket_ptr(
			submit, slot);
	snapshot_flags =
		poulsbo_sgx_dependency_snapshot_flags_ptr(submit, slot);
	if (!submit_dependency_submit_id || !submit_dependency_offset ||
	    !dependency_config_count || !snapshot_source_submit_id ||
	    !snapshot_required_ticket ||
	    !snapshot_observed_complete_ticket || !snapshot_flags) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (!submit->status_object_id ||
	    !poulsbo_sgx_dependency_required_ticket(submit, slot)) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (*submit_dependency_submit_id &&
	    !poulsbo_sgx_dependency_required_ticket(submit, slot)) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (dependency_offset & 0x3) {
		ret = -EINVAL;
		goto out_unlock;
	}

	status_obj = poulsbo_sgx_find_object_locked(dev_priv,
						    submit->status_object_id);
	if (!status_obj) {
		ret = -ENOENT;
		goto out_unlock;
	}
	old_dependency_offset = *submit_dependency_offset;
	old_dependency_config_count = *dependency_config_count;
	*submit_dependency_offset = dependency_offset;
	(*dependency_config_count)++;
	ret = poulsbo_sgx_validate_submit_status_layout_locked(
		submit, status_obj, NULL);
	*submit_dependency_offset = old_dependency_offset;
	*dependency_config_count = old_dependency_config_count;
	if (ret)
		goto out_unlock;
	poulsbo_sgx_disarm_submit_locked(submit);
	*submit_dependency_offset = dependency_offset;
	(*dependency_config_count)++;
	ret = poulsbo_sgx_refresh_dependency_blocks_locked(dev_priv, submit);
	if (!ret)
		ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit depstate%u submit_id=%u offset=%#x source_submit_id=%u required_ticket=%u observed_complete_ticket=%u flags=%#x\n",
			 slot + 1, submit_id, dependency_offset,
			 *snapshot_source_submit_id, *snapshot_required_ticket,
			 *snapshot_observed_complete_ticket, *snapshot_flags);
	return ret;
}

static int poulsbo_sgx_configure_submit_dependency_state(
	struct drm_device *dev,
	u32 submit_id,
	u32 dependency_offset)
{
	return poulsbo_sgx_configure_submit_dependency_state_slot(dev,
								  submit_id,
								  0,
								  dependency_offset);
}

static int poulsbo_sgx_configure_submit_dependency_state2(
	struct drm_device *dev,
	u32 submit_id,
	u32 dependency_offset)
{
	return poulsbo_sgx_configure_submit_dependency_state_slot(dev,
								  submit_id,
								  1,
								  dependency_offset);
}

static int poulsbo_sgx_configure_submit_dependency_state3(
	struct drm_device *dev,
	u32 submit_id,
	u32 dependency_offset)
{
	return poulsbo_sgx_configure_submit_dependency_state_slot(dev,
								  submit_id,
								  2,
								  dependency_offset);
}

static int poulsbo_sgx_configure_submit_dependency_state_slot_user(
	struct drm_device *dev,
	u32 submit_id,
	u32 slot_index,
	u32 dependency_offset)
{
	unsigned int slot;
	int ret;

	ret = poulsbo_sgx_dependency_slot_from_user(slot_index, &slot);
	if (ret)
		return ret;

	return poulsbo_sgx_configure_submit_dependency_state_slot(dev, submit_id,
								  slot,
								  dependency_offset);
}

static int poulsbo_sgx_clear_submit_dependency_slot_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	unsigned int slot)
{
	struct psb_sgx_lab_object *status_obj = NULL;
	u32 *dependency_submit_id = poulsbo_sgx_dependency_submit_id_ptr(submit,
									 slot);
	u32 *dependency_offset = poulsbo_sgx_dependency_offset_ptr(submit, slot);
	u32 *dependency_config_count =
		poulsbo_sgx_dependency_config_count_ptr(submit, slot);
	unsigned long *last_dependency_jiffies =
		poulsbo_sgx_last_dependency_jiffies_ptr(submit, slot);
	unsigned long *last_dependency_block_jiffies =
		poulsbo_sgx_last_dependency_block_jiffies_ptr(submit, slot);
	struct psb_sgx_lab_dependency_words cleared_dependency;
	u32 old_dependency_offset;
	u32 old_dependency_config_count;
	u32 dependency_region_end;
	int ret;

	if (!dependency_submit_id || !dependency_offset ||
	    !dependency_config_count || !last_dependency_jiffies ||
	    !last_dependency_block_jiffies)
		return -EINVAL;

	old_dependency_offset = *dependency_offset;
	old_dependency_config_count = *dependency_config_count;
	poulsbo_sgx_disarm_submit_locked(submit);
	memset(&cleared_dependency, 0, sizeof(cleared_dependency));

	if (old_dependency_config_count && old_dependency_offset &&
	    submit->status_object_id) {
		ret = poulsbo_sgx_dependency_region_end(old_dependency_offset,
							 &dependency_region_end);
		if (ret)
			return ret;
		status_obj = poulsbo_sgx_find_object_locked(
			dev_priv, submit->status_object_id);
		if (!status_obj)
			return -ENOENT;
		ret = poulsbo_sgx_object_access_ok(status_obj,
						   old_dependency_offset,
						   dependency_region_end -
						   old_dependency_offset);
		if (ret)
			return ret;
		ret = poulsbo_sgx_write_submit_dependency_block_locked(
			status_obj, old_dependency_offset, &cleared_dependency);
		if (ret)
			return ret;
	}

	switch (slot) {
	case 0:
		submit->ta_dependency = 0;
		break;
	case 1:
		submit->dependency_ticket_secondary = 0;
		break;
	case 2:
		submit->dependency_ticket_tertiary = 0;
		break;
	case 3:
		submit->dependency_ticket_quaternary = 0;
		break;
	default:
		return -EINVAL;
	}

	*dependency_submit_id = 0;
	*dependency_offset = 0;
	ret = poulsbo_sgx_store_submit_dependency_snapshot_locked(
		submit, slot, &cleared_dependency);
	if (ret)
		return ret;
	*dependency_config_count = 0;
	*last_dependency_jiffies = jiffies;
	*last_dependency_block_jiffies = jiffies;

	ret = poulsbo_sgx_refresh_dependency_blocks_locked(dev_priv, submit);
	if (!ret)
		ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);
	return ret;
}

static int poulsbo_sgx_clear_submit_dependency_slot_user(
	struct drm_device *dev,
	u32 submit_id,
	u32 slot_index)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	unsigned int slot;
	int ret;

	ret = poulsbo_sgx_dependency_slot_from_user(slot_index, &slot);
	if (ret)
		return ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);
	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}

	ret = poulsbo_sgx_clear_submit_dependency_slot_locked(dev_priv, submit,
								     slot);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
	if (ret)
		return ret;

	dev_info(dev->dev,
		 "lab submit depclear_slot submit_id=%u slot=%u\n",
		 submit_id, slot_index);
	return 0;
}

static int poulsbo_sgx_configure_submit_ccb(struct drm_device *dev, u32 submit_id,
					    u32 write_offset, u32 read_offset,
					    u32 service_address,
					    u32 cache_control,
					    u32 data0, u32 data1)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *control_obj;
	struct psb_sgx_lab_queue_header_words queue_header;
	struct psb_sgx_lab_ccb_record_words ccb_record;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}

	if (!submit->control_object_id || !submit->packet_config_count) {
		ret = -EINVAL;
		goto out_unlock;
	}

	control_obj = poulsbo_sgx_find_object_locked(dev_priv,
						    submit->control_object_id);
	if (!control_obj) {
		ret = -ENOENT;
		goto out_unlock;
	}

	if (write_offset & 0x3 || read_offset & 0x3) {
		ret = -EINVAL;
		goto out_unlock;
	}

	ret = poulsbo_sgx_validate_submit_control_layout_locked(
		submit, control_obj, NULL);
	if (ret)
		goto out_unlock;

	ret = poulsbo_sgx_object_access_ok(control_obj, 0, 8);
	if (ret)
		goto out_unlock;

	ret = poulsbo_sgx_object_access_ok(control_obj, submit->ccb_offset,
					   PSB_SGX_LAB_CCB_RECORD_BYTES);
	if (ret)
		goto out_unlock;

	ret = poulsbo_sgx_build_submit_queue_header_locked(submit,
							    &queue_header);
	if (ret)
		goto out_unlock;
	queue_header.write_offset = write_offset;
	queue_header.read_offset = read_offset;
	ret = poulsbo_sgx_write_queue_header_locked(control_obj, 0,
						    &queue_header);
	if (ret)
		goto out_unlock;

	memset(&ccb_record, 0, sizeof(ccb_record));
	ccb_record.service_address = service_address;
	ccb_record.cache_control = cache_control;
	ccb_record.data0 = data0;
	ccb_record.data1 = data1;
	ret = poulsbo_sgx_resolve_submit_ccb_gpu_va32_locked(
		dev_priv, submit, &ccb_record.pds_gpu_va32,
		&ccb_record.use_gpu_va32, &ccb_record.target_gpu_va32,
		&ccb_record.status_gpu_va32);
	if (ret && ret != -ENOENT)
		goto out_unlock;
	if (ret == -ENOENT) {
		memset(&ccb_record.pds_gpu_va32, 0,
		       4 * sizeof(ccb_record.pds_gpu_va32));
		ret = 0;
	}

	ret = poulsbo_sgx_write_submit_ccb_record_locked(
		control_obj, submit->ccb_offset, &ccb_record);
	if (ret)
		goto out_unlock;

	poulsbo_sgx_disarm_submit_locked(submit);
	submit->ccb_write_offset = write_offset;
	submit->ccb_read_offset = read_offset;
	submit->ccb_service_address = service_address;
	submit->ccb_cache_control = cache_control;
	submit->ccb_data0 = data0;
	submit->ccb_data1 = data1;
	submit->ccb_config_count++;
	submit->last_ccb_jiffies = jiffies;
	ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit ccb submit_id=%u write_offset=%#x read_offset=%#x service=%#x cache=%#x data0=%#x data1=%#x\n",
			 submit_id, write_offset, read_offset,
			 service_address, cache_control, data0, data1);
	return ret;
}

static int poulsbo_sgx_configure_submit_queue(struct drm_device *dev,
					      u32 submit_id,
					      u32 queue_base_offset,
					      u32 queue_size,
					      u32 queue_packet_size,
					      u32 queue_packet_count)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *control_obj;
	struct psb_sgx_lab_queue_header_words queue_header;
	u32 queue_tail_packet_count;
	u32 queue_head_packet_count;
	u32 expected_write_offset;
	u32 old_queue_base_offset;
	u32 old_queue_size;
	u32 old_queue_packet_size;
	u32 old_queue_packet_count;
	u32 old_queue_tail_packet_count;
	u32 old_queue_head_packet_count;
	u32 old_queue_config_count;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	if (!submit->control_object_id || !submit->packet_config_count ||
	    !submit->ccb_config_count) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if ((queue_base_offset & 0xf) || !queue_size || !queue_packet_size ||
	    !queue_packet_count) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if ((queue_packet_size & 0xf) ||
	    queue_packet_size < PSB_SGX_LAB_CCB_RECORD_BYTES) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (queue_size < queue_packet_size ||
	    (queue_size % queue_packet_size)) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (queue_packet_count > queue_size / queue_packet_size) {
		ret = -EINVAL;
		goto out_unlock;
	}
	ret = poulsbo_sgx_queue_progression(submit->ccb_read_offset,
					    queue_size,
					    queue_packet_size,
					    queue_packet_count,
					    &queue_tail_packet_count,
					    &queue_head_packet_count,
					    &expected_write_offset);
	if (ret)
		goto out_unlock;
	if (submit->ccb_write_offset != expected_write_offset) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (submit->ccb_offset != queue_base_offset + submit->ccb_read_offset) {
		ret = -EINVAL;
		goto out_unlock;
	}

	control_obj = poulsbo_sgx_find_object_locked(dev_priv,
						    submit->control_object_id);
	if (!control_obj) {
		ret = -ENOENT;
		goto out_unlock;
	}

	old_queue_base_offset = submit->queue_base_offset;
	old_queue_size = submit->queue_size;
	old_queue_packet_size = submit->queue_packet_size;
	old_queue_packet_count = submit->queue_packet_count;
	old_queue_tail_packet_count = submit->queue_tail_packet_count;
	old_queue_head_packet_count = submit->queue_head_packet_count;
	old_queue_config_count = submit->queue_config_count;
	submit->queue_base_offset = queue_base_offset;
	submit->queue_size = queue_size;
	submit->queue_packet_size = queue_packet_size;
	submit->queue_packet_count = queue_packet_count;
	submit->queue_tail_packet_count = queue_tail_packet_count;
	submit->queue_head_packet_count = queue_head_packet_count;
	submit->queue_config_count++;
	ret = poulsbo_sgx_validate_submit_control_layout_locked(
		submit, control_obj, NULL);
	submit->queue_base_offset = old_queue_base_offset;
	submit->queue_size = old_queue_size;
	submit->queue_packet_size = old_queue_packet_size;
	submit->queue_packet_count = old_queue_packet_count;
	submit->queue_tail_packet_count = old_queue_tail_packet_count;
	submit->queue_head_packet_count = old_queue_head_packet_count;
	submit->queue_config_count = old_queue_config_count;
	if (ret)
		goto out_unlock;

	ret = poulsbo_sgx_object_access_ok(control_obj, 0,
					   PSB_SGX_LAB_CONTROL_QUEUE_HEADER_BYTES);
	if (ret)
		goto out_unlock;
	ret = poulsbo_sgx_build_submit_queue_header_locked(submit,
							    &queue_header);
	if (ret)
		goto out_unlock;
	queue_header.queue_base_offset = queue_base_offset;
	queue_header.queue_size = queue_size;
	queue_header.queue_packet_size = queue_packet_size;
	queue_header.queue_packet_count = queue_packet_count;
	queue_header.queue_tail_packet_count = queue_tail_packet_count;
	queue_header.queue_head_packet_count = queue_head_packet_count;
	ret = poulsbo_sgx_write_queue_header_locked(control_obj, 0,
						    &queue_header);
	if (ret)
		goto out_unlock;

	poulsbo_sgx_disarm_submit_locked(submit);
	submit->queue_base_offset = queue_base_offset;
	submit->queue_size = queue_size;
	submit->queue_packet_size = queue_packet_size;
	submit->queue_packet_count = queue_packet_count;
	submit->queue_tail_packet_count = queue_tail_packet_count;
	submit->queue_head_packet_count = queue_head_packet_count;
	submit->queue_config_count++;
	submit->last_queue_jiffies = jiffies;
	ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit queue submit_id=%u base_offset=%#x size=%#x packet_size=%#x packet_count=%u read_offset=%#x write_offset=%#x tail_packets=%u head_packets=%u wrapped=%u\n",
			 submit_id, queue_base_offset, queue_size,
			 queue_packet_size, queue_packet_count,
			 submit->ccb_read_offset, submit->ccb_write_offset,
			 queue_tail_packet_count, queue_head_packet_count,
			 queue_head_packet_count ? 1 : 0);
	return ret;
}

static int poulsbo_sgx_configure_submit_status(struct drm_device *dev,
					       u32 submit_id,
					       u32 ta_offset,
					       u32 threed_offset,
					       u32 stride,
					       u32 value_seed)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *status_obj;
	u32 old_status_ta_offset;
	u32 old_status_3d_offset;
	u32 old_status_stride;
	u32 old_status_config_count;
	u32 ta_count = 0;
	u32 threed_count = 0;
	u32 index;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	if (!submit->status_object_id) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (!submit->packet_config_count) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (!submit->num_ta_status_vals && !submit->num_3d_status_vals) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (!stride || (stride & 0x3)) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if ((submit->num_ta_status_vals && (ta_offset & 0x3)) ||
	    (submit->num_3d_status_vals && (threed_offset & 0x3))) {
		ret = -EINVAL;
		goto out_unlock;
	}

	status_obj = poulsbo_sgx_find_object_locked(dev_priv,
						    submit->status_object_id);
	if (!status_obj) {
		ret = -ENOENT;
		goto out_unlock;
	}
	old_status_ta_offset = submit->status_ta_offset;
	old_status_3d_offset = submit->status_3d_offset;
	old_status_stride = submit->status_stride;
	old_status_config_count = submit->status_config_count;
	submit->status_ta_offset = ta_offset;
	submit->status_3d_offset = threed_offset;
	submit->status_stride = stride;
	submit->status_config_count++;
	ret = poulsbo_sgx_validate_submit_status_layout_locked(
		submit, status_obj, NULL);
	submit->status_ta_offset = old_status_ta_offset;
	submit->status_3d_offset = old_status_3d_offset;
	submit->status_stride = old_status_stride;
	submit->status_config_count = old_status_config_count;
	if (ret)
		goto out_unlock;

	for (index = 0; index < submit->num_ta_status_vals; index++) {
		ret = poulsbo_sgx_object_write_u32_locked(
			status_obj, ta_offset + index * stride,
			poulsbo_sgx_lab_status_value(value_seed, false, index));
		if (ret)
			goto out_unlock;
	}
	for (index = 0; index < submit->num_3d_status_vals; index++) {
		ret = poulsbo_sgx_object_write_u32_locked(
			status_obj, threed_offset + index * stride,
			poulsbo_sgx_lab_status_value(value_seed, true, index));
		if (ret)
			goto out_unlock;
	}

	poulsbo_sgx_disarm_submit_locked(submit);
	submit->status_ta_offset = ta_offset;
	submit->status_3d_offset = threed_offset;
	submit->status_stride = stride;
	submit->status_value_seed = value_seed;
	submit->status_config_count++;
	submit->last_status_jiffies = jiffies;
	ta_count = submit->num_ta_status_vals;
	threed_count = submit->num_3d_status_vals;
	ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit status submit_id=%u ta_offset=%#x 3d_offset=%#x stride=%#x seed=%#x ta_count=%u 3d_count=%u\n",
			 submit_id, ta_offset, threed_offset, stride,
			 value_seed, ta_count, threed_count);
	return ret;
}

static int poulsbo_sgx_configure_submit_sync(struct drm_device *dev,
					     u32 submit_id,
					     u32 sync_offset,
					     u32 complete_seq,
					     u32 pending_seq,
					     u32 flags)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *status_obj;
	struct psb_sgx_lab_sync_words sync;
	u32 old_sync_offset;
	u32 old_sync_config_count;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	if (!submit->status_object_id || !submit->queue_config_count) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (flags & ~(PSB_SGX_LAB_SYNC_FLAG_READY |
		      PSB_SGX_LAB_SYNC_FLAG_WRAPPED)) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (!(flags & PSB_SGX_LAB_SYNC_FLAG_READY)) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (!!(flags & PSB_SGX_LAB_SYNC_FLAG_WRAPPED) !=
	    !!submit->queue_head_packet_count) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (pending_seq < complete_seq ||
	    pending_seq - complete_seq != submit->queue_packet_count) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (sync_offset & 0x3) {
		ret = -EINVAL;
		goto out_unlock;
	}

	status_obj = poulsbo_sgx_find_object_locked(dev_priv,
						    submit->status_object_id);
	if (!status_obj) {
		ret = -ENOENT;
		goto out_unlock;
	}
	old_sync_offset = submit->sync_offset;
	old_sync_config_count = submit->sync_config_count;
	submit->sync_offset = sync_offset;
	submit->sync_config_count++;
	ret = poulsbo_sgx_validate_submit_status_layout_locked(
		submit, status_obj, NULL);
	submit->sync_offset = old_sync_offset;
	submit->sync_config_count = old_sync_config_count;
	if (ret)
		goto out_unlock;

	memset(&sync, 0, sizeof(sync));
	sync.read_offset = submit->ccb_read_offset;
	sync.write_offset = submit->ccb_write_offset;
	sync.complete_seq = complete_seq;
	sync.pending_seq = pending_seq;
	sync.flags = flags | PSB_SGX_LAB_SYNC_FLAG_READY;
	sync.epoch = submit->queue_epoch;
	sync.global_issue_ticket = submit->global_issue_ticket;
	if (submit->queue_head_packet_count)
		sync.flags |= PSB_SGX_LAB_SYNC_FLAG_WRAPPED;
	else
		sync.flags &= ~PSB_SGX_LAB_SYNC_FLAG_WRAPPED;
	ret = poulsbo_sgx_write_submit_sync_locked(status_obj, sync_offset,
						   &sync);
	if (ret)
		goto out_unlock;

	poulsbo_sgx_disarm_submit_locked(submit);
	submit->sync_offset = sync_offset;
	submit->sync_complete_seq = complete_seq;
	submit->sync_pending_seq = pending_seq;
	submit->sync_flags = sync.flags;
	submit->sync_config_count++;
	submit->last_sync_jiffies = jiffies;
	ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit sync submit_id=%u offset=%#x read_offset=%#x write_offset=%#x complete_seq=%#x pending_seq=%#x flags=%#x wrapped=%u epoch=%u global_issue_ticket=%u\n",
			 submit_id, sync_offset, submit->ccb_read_offset,
			 submit->ccb_write_offset, complete_seq, pending_seq,
			 flags, !!(flags & PSB_SGX_LAB_SYNC_FLAG_WRAPPED),
			 submit->queue_epoch, submit->global_issue_ticket);
	return ret;
}

static int poulsbo_sgx_configure_submit_owner(struct drm_device *dev,
					      u32 submit_id,
					      u32 owner_offset,
					      u32 owner_state,
					      u32 issue_cookie,
					      u32 complete_cookie)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *status_obj;
	struct psb_sgx_lab_owner_words owner;
	u32 old_owner_offset;
	u32 old_owner_config_count;
	u32 expected_sync_outstanding;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	if (!submit->status_object_id || !submit->queue_config_count ||
	    !submit->sync_config_count) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (owner_state != PSB_SGX_LAB_OWNER_QUEUED &&
	    owner_state != PSB_SGX_LAB_OWNER_RUNNING &&
	    owner_state != PSB_SGX_LAB_OWNER_COMPLETE) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (owner_offset & 0x3) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (issue_cookie < complete_cookie) {
		ret = -EINVAL;
		goto out_unlock;
	}
	expected_sync_outstanding = submit->sync_pending_seq -
				    submit->sync_complete_seq;
	if (issue_cookie - complete_cookie != expected_sync_outstanding) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (!expected_sync_outstanding) {
		if (owner_state != PSB_SGX_LAB_OWNER_COMPLETE) {
			ret = -EINVAL;
			goto out_unlock;
		}
	} else if (owner_state == PSB_SGX_LAB_OWNER_COMPLETE) {
		ret = -EINVAL;
		goto out_unlock;
	}

	status_obj = poulsbo_sgx_find_object_locked(dev_priv,
						    submit->status_object_id);
	if (!status_obj) {
		ret = -ENOENT;
		goto out_unlock;
	}
	old_owner_offset = submit->owner_offset;
	old_owner_config_count = submit->owner_config_count;
	submit->owner_offset = owner_offset;
	submit->owner_config_count++;
	ret = poulsbo_sgx_validate_submit_status_layout_locked(
		submit, status_obj, NULL);
	submit->owner_offset = old_owner_offset;
	submit->owner_config_count = old_owner_config_count;
	if (ret)
		goto out_unlock;

	memset(&owner, 0, sizeof(owner));
	owner.owner_state = owner_state;
	owner.owner_issue_cookie = issue_cookie;
	owner.owner_complete_cookie = complete_cookie;
	owner.epoch = submit->queue_epoch;
	owner.global_complete_ticket = submit->global_complete_ticket;
	owner.global_running_ticket = submit->global_running_ticket;
	ret = poulsbo_sgx_write_submit_owner_locked(status_obj, owner_offset,
						    &owner);
	if (ret)
		goto out_unlock;

	poulsbo_sgx_disarm_submit_locked(submit);
	submit->owner_offset = owner_offset;
	submit->owner_state = owner_state;
	submit->owner_issue_cookie = issue_cookie;
	submit->owner_complete_cookie = complete_cookie;
	submit->owner_config_count++;
	submit->last_owner_jiffies = jiffies;
	ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit owner submit_id=%u offset=%#x state=%s issue_cookie=%#x complete_cookie=%#x outstanding=%#x epoch=%u global_complete_ticket=%u global_running_ticket=%u\n",
			 submit_id, owner_offset,
			 poulsbo_sgx_lab_owner_state_name(owner_state),
			 issue_cookie, complete_cookie,
			 issue_cookie - complete_cookie,
			 submit->queue_epoch, submit->global_complete_ticket,
			 submit->global_running_ticket);
	return ret;
}

static int poulsbo_sgx_configure_submit_report(struct drm_device *dev,
					       u32 submit_id,
					       u32 report_offset)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *status_obj;
	u32 old_report_offset;
	u32 old_report_config_count;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	if (!submit->status_object_id) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (report_offset & 0x3) {
		ret = -EINVAL;
		goto out_unlock;
	}

	status_obj = poulsbo_sgx_find_object_locked(dev_priv,
						    submit->status_object_id);
	if (!status_obj) {
		ret = -ENOENT;
		goto out_unlock;
	}
	old_report_offset = submit->report_offset;
	old_report_config_count = submit->report_config_count;
	submit->report_offset = report_offset;
	submit->report_config_count++;
	ret = poulsbo_sgx_validate_submit_status_layout_locked(
		submit, status_obj, NULL);
	submit->report_offset = old_report_offset;
	submit->report_config_count = old_report_config_count;
	if (ret)
		goto out_unlock;

	poulsbo_sgx_disarm_submit_locked(submit);
	submit->report_offset = report_offset;
	submit->report_config_count++;
	ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit report submit_id=%u offset=%#x stage=%s stage_count=%u owner_state=%s queue_packet_count=%u completed_packets=%u dependency_active_count=%u flags=%#x\n",
			 submit_id, report_offset,
			 poulsbo_sgx_lab_report_stage_name(submit->report_stage),
			 submit->report_stage_count,
			 poulsbo_sgx_lab_owner_state_name(
				 submit->report_owner_state),
			 submit->report_queue_packet_count,
			 submit->report_completed_packets,
			 submit->report_dependency_active_count,
			 submit->report_flags);
	return ret;
}

static int poulsbo_sgx_configure_submit_live(struct drm_device *dev,
					     u32 submit_id,
					     u32 live_offset)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *control_obj;
	u32 old_live_offset;
	u32 old_live_config_count;
	u32 stage;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	if (!submit->control_object_id) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (live_offset & 0x3) {
		ret = -EINVAL;
		goto out_unlock;
	}

	control_obj = poulsbo_sgx_find_object_locked(dev_priv,
						     submit->control_object_id);
	if (!control_obj) {
		ret = -ENOENT;
		goto out_unlock;
	}
	old_live_offset = submit->live_offset;
	old_live_config_count = submit->live_config_count;
	submit->live_offset = live_offset;
	submit->live_config_count++;
	ret = poulsbo_sgx_validate_submit_control_layout_locked(
		submit, control_obj, NULL);
	submit->live_offset = old_live_offset;
	submit->live_config_count = old_live_config_count;
	if (ret)
		goto out_unlock;

	poulsbo_sgx_disarm_submit_locked(submit);
	submit->live_offset = live_offset;
	submit->live_config_count++;
	ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);
	stage = poulsbo_sgx_submit_report_stage_locked(submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit live submit_id=%u offset=%#x stage=%s stage_count=%u owner_state=%s issued_packets=%u completed_packets=%u dependency_active_count=%u\n",
			 submit_id, live_offset,
			 poulsbo_sgx_lab_report_stage_name(stage),
			 poulsbo_sgx_submit_report_stage_count_locked(submit,
								      stage),
			 poulsbo_sgx_lab_owner_state_name(submit->owner_state),
			 poulsbo_sgx_submit_report_issued_packets_locked(submit,
								 stage),
			 poulsbo_sgx_submit_report_completed_packets_locked(
				 submit, stage),
			 poulsbo_sgx_submit_report_dependency_count_locked(
				 submit, stage));
	return ret;
}

static int poulsbo_sgx_configure_submit_result(struct drm_device *dev,
					       u32 submit_id,
					       u32 result_offset)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *status_obj;
	u32 old_result_offset;
	u32 old_result_config_count;
	u32 stage;
	u32 source;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	if (!submit->status_object_id) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (result_offset & 0x3) {
		ret = -EINVAL;
		goto out_unlock;
	}

	status_obj = poulsbo_sgx_find_object_locked(dev_priv,
						    submit->status_object_id);
	if (!status_obj) {
		ret = -ENOENT;
		goto out_unlock;
	}
	old_result_offset = submit->result_offset;
	old_result_config_count = submit->result_config_count;
	submit->result_offset = result_offset;
	submit->result_config_count++;
	ret = poulsbo_sgx_validate_submit_status_layout_locked(
		submit, status_obj, NULL);
	submit->result_offset = old_result_offset;
	submit->result_config_count = old_result_config_count;
	if (ret)
		goto out_unlock;

	poulsbo_sgx_disarm_submit_locked(submit);
	submit->result_offset = result_offset;
	submit->result_config_count++;
	ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);
	stage = poulsbo_sgx_submit_report_stage_locked(submit);
	source = poulsbo_sgx_submit_result_source_locked(
		poulsbo_sgx_submit_result_exec_source_locked(stage));

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit result submit_id=%u offset=%#x source=%s stage=%s stage_count=%u owner_state=%s issued_packets=%u completed_packets=%u dependency_active_count=%u\n",
			 submit_id, result_offset,
			 poulsbo_sgx_lab_result_source_name(source),
			 poulsbo_sgx_lab_report_stage_name(stage),
			 poulsbo_sgx_submit_report_stage_count_locked(submit, stage),
			 poulsbo_sgx_lab_owner_state_name(submit->owner_state),
			 poulsbo_sgx_submit_report_issued_packets_locked(submit,
									 stage),
			 poulsbo_sgx_submit_report_completed_packets_locked(
				 submit, stage),
			 poulsbo_sgx_submit_report_dependency_count_locked(
				 submit, stage));
	return ret;
}

static int poulsbo_sgx_configure_submit_manifest(struct drm_device *dev,
						 u32 submit_id,
						 u32 manifest_offset)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *control_obj;
	u32 old_manifest_offset;
	u32 old_manifest_config_count;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	if (!submit->control_object_id) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (manifest_offset & 0x3) {
		ret = -EINVAL;
		goto out_unlock;
	}

	control_obj = poulsbo_sgx_find_object_locked(dev_priv,
						     submit->control_object_id);
	if (!control_obj) {
		ret = -ENOENT;
		goto out_unlock;
	}

	old_manifest_offset = submit->manifest_offset;
	old_manifest_config_count = submit->manifest_config_count;
	submit->manifest_offset = manifest_offset;
	submit->manifest_config_count++;
	ret = poulsbo_sgx_validate_submit_control_layout_locked(
		submit, control_obj, NULL);
	submit->manifest_offset = old_manifest_offset;
	submit->manifest_config_count = old_manifest_config_count;
	if (ret)
		goto out_unlock;

	poulsbo_sgx_disarm_submit_locked(submit);
	submit->manifest_offset = manifest_offset;
	submit->manifest_config_count++;
	ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit manifest submit_id=%u offset=%#x stage=%s stage_count=%u owner_state=%s queue_packet_count=%u dependency_active_count=%u\n",
			 submit_id, manifest_offset,
			 poulsbo_sgx_lab_report_stage_name(
				 poulsbo_sgx_submit_report_stage_locked(submit)),
			 poulsbo_sgx_submit_report_stage_count_locked(
				 submit,
				 poulsbo_sgx_submit_report_stage_locked(
					 submit)),
			 poulsbo_sgx_lab_owner_state_name(submit->owner_state),
			 submit->queue_packet_count,
			 poulsbo_sgx_submit_report_dependency_count_locked(
				 submit,
				 poulsbo_sgx_submit_report_stage_locked(
					 submit)));
	return ret;
}

static int poulsbo_sgx_configure_submit_descriptor(struct drm_device *dev,
						   u32 submit_id,
						   u32 descriptor_offset)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *control_obj;
	u32 old_descriptor_offset;
	u32 old_descriptor_config_count;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	if (!submit->control_object_id) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (descriptor_offset & 0x3) {
		ret = -EINVAL;
		goto out_unlock;
	}

	control_obj = poulsbo_sgx_find_object_locked(dev_priv,
						     submit->control_object_id);
	if (!control_obj) {
		ret = -ENOENT;
		goto out_unlock;
	}

	old_descriptor_offset = submit->descriptor_offset;
	old_descriptor_config_count = submit->descriptor_config_count;
	submit->descriptor_offset = descriptor_offset;
	submit->descriptor_config_count++;
	ret = poulsbo_sgx_validate_submit_control_layout_locked(
		submit, control_obj, NULL);
	submit->descriptor_offset = old_descriptor_offset;
	submit->descriptor_config_count = old_descriptor_config_count;
	if (ret)
		goto out_unlock;

	poulsbo_sgx_disarm_submit_locked(submit);
	submit->descriptor_offset = descriptor_offset;
	submit->descriptor_config_count++;
	ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit descriptor submit_id=%u offset=%#x stage=%s stage_count=%u owner_state=%s issued_packets=%u completed_packets=%u dependency_active_count=%u\n",
			 submit_id, descriptor_offset,
			 poulsbo_sgx_lab_report_stage_name(
				 poulsbo_sgx_submit_report_stage_locked(submit)),
			 poulsbo_sgx_submit_report_stage_count_locked(
				 submit,
				 poulsbo_sgx_submit_report_stage_locked(
					 submit)),
			 poulsbo_sgx_lab_owner_state_name(submit->owner_state),
			 poulsbo_sgx_submit_report_issued_packets_locked(
				 submit,
				 poulsbo_sgx_submit_report_stage_locked(
					 submit)),
			 poulsbo_sgx_submit_report_completed_packets_locked(
				 submit,
				 poulsbo_sgx_submit_report_stage_locked(
					 submit)),
			 poulsbo_sgx_submit_report_dependency_count_locked(
				 submit,
				 poulsbo_sgx_submit_report_stage_locked(
					 submit)));
	return ret;
}

static int poulsbo_sgx_configure_submit_plan(struct drm_device *dev,
					     u32 submit_id,
					     u32 plan_offset)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *control_obj;
	u32 old_plan_offset;
	u32 old_plan_config_count;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	if (!submit->control_object_id) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (plan_offset & 0x3) {
		ret = -EINVAL;
		goto out_unlock;
	}

	control_obj = poulsbo_sgx_find_object_locked(dev_priv,
						     submit->control_object_id);
	if (!control_obj) {
		ret = -ENOENT;
		goto out_unlock;
	}

	old_plan_offset = submit->plan_offset;
	old_plan_config_count = submit->plan_config_count;
	submit->plan_offset = plan_offset;
	submit->plan_config_count++;
	ret = poulsbo_sgx_validate_submit_control_layout_locked(
		submit, control_obj, NULL);
	submit->plan_offset = old_plan_offset;
	submit->plan_config_count = old_plan_config_count;
	if (ret)
		goto out_unlock;

	poulsbo_sgx_disarm_submit_locked(submit);
	submit->plan_offset = plan_offset;
	submit->plan_config_count++;
	ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit plan submit_id=%u offset=%#x source=%s stage=%s stage_count=%u owner_state=%s queue_packet_count=%u issued_packets=%u completed_packets=%u dependency_active_count=%u\n",
			 submit_id, plan_offset,
			 poulsbo_sgx_lab_plan_source_name(
				 poulsbo_sgx_submit_plan_source_locked(
					 poulsbo_sgx_submit_plan_exec_source_locked(
						 poulsbo_sgx_submit_report_stage_locked(
							 submit)))),
			 poulsbo_sgx_lab_report_stage_name(
				 poulsbo_sgx_submit_report_stage_locked(submit)),
			 poulsbo_sgx_submit_report_stage_count_locked(
				 submit,
				 poulsbo_sgx_submit_report_stage_locked(
					 submit)),
			 poulsbo_sgx_lab_owner_state_name(submit->owner_state),
			 submit->queue_packet_count,
			 poulsbo_sgx_submit_report_issued_packets_locked(
				 submit,
				 poulsbo_sgx_submit_report_stage_locked(
					 submit)),
			 poulsbo_sgx_submit_report_completed_packets_locked(
				 submit,
				 poulsbo_sgx_submit_report_stage_locked(
					 submit)),
			 poulsbo_sgx_submit_report_dependency_count_locked(
				 submit,
				 poulsbo_sgx_submit_report_stage_locked(
					 submit)));
	return ret;
}

static int poulsbo_sgx_configure_submit_issue_plan(struct drm_device *dev,
						   u32 submit_id,
						   u32 issue_plan_offset,
						   u32 owner_state,
						   u32 issued_packets)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *control_obj;
	u32 old_issue_plan_offset;
	u32 old_issue_plan_config_count;
	u32 old_issue_plan_owner_state;
	u32 old_issue_plan_packet_count;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		ret = -ENOENT;
		goto out_unlock;
	}
	if (!submit->control_object_id) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (issue_plan_offset & 0x3 ||
	    (owner_state != PSB_SGX_LAB_OWNER_QUEUED &&
	     owner_state != PSB_SGX_LAB_OWNER_RUNNING) ||
	    !issued_packets) {
		ret = -EINVAL;
		goto out_unlock;
	}

	control_obj = poulsbo_sgx_find_object_locked(dev_priv,
						     submit->control_object_id);
	if (!control_obj) {
		ret = -ENOENT;
		goto out_unlock;
	}

	old_issue_plan_offset = submit->issue_plan_offset;
	old_issue_plan_config_count = submit->issue_plan_config_count;
	old_issue_plan_owner_state = submit->issue_plan_owner_state;
	old_issue_plan_packet_count = submit->issue_plan_packet_count;
	submit->issue_plan_offset = issue_plan_offset;
	submit->issue_plan_config_count++;
	submit->issue_plan_owner_state = owner_state;
	submit->issue_plan_packet_count = issued_packets;
	ret = poulsbo_sgx_validate_submit_control_layout_locked(
		submit, control_obj, NULL);
	submit->issue_plan_offset = old_issue_plan_offset;
	submit->issue_plan_config_count = old_issue_plan_config_count;
	submit->issue_plan_owner_state = old_issue_plan_owner_state;
	submit->issue_plan_packet_count = old_issue_plan_packet_count;
	if (ret)
		goto out_unlock;

	poulsbo_sgx_disarm_submit_locked(submit);
	submit->issue_plan_offset = issue_plan_offset;
	submit->issue_plan_config_count++;
	submit->issue_plan_owner_state = owner_state;
	submit->issue_plan_packet_count = issued_packets;
	ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	if (!ret)
		dev_info(dev->dev,
			 "lab submit issue-plan submit_id=%u offset=%#x owner_state=%s issued_packets=%u stage=%s stage_count=%u\n",
			 submit_id, issue_plan_offset,
			 poulsbo_sgx_lab_owner_state_name(owner_state),
			 issued_packets,
			 poulsbo_sgx_lab_report_stage_name(
				 poulsbo_sgx_submit_report_stage_locked(submit)),
			 poulsbo_sgx_submit_report_stage_count_locked(
				 submit,
				 poulsbo_sgx_submit_report_stage_locked(
					 submit)));
	return ret;
}

static int poulsbo_sgx_validate_submit_sgx_ta_ioctl_op_request(
	const struct drm_psb_sgx_lab_submit_op *op,
	bool dependency_fields_present,
	u32 op_kind)
{
	if (!op)
		return -EINVAL;

	if (op->context || op->slot || op->object_id ||
	    op->cmd_type || op->ccb_offset || op->ctrl_flags ||
	    op->num_ta_status_vals || op->num_3d_status_vals ||
	    op->ta_dependency || op->ccb_write_offset ||
	    op->ccb_read_offset || op->ccb_service_address ||
	    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
	    op->queue_base_offset || op->queue_size ||
	    op->queue_packet_size || op->queue_packet_count ||
	    op->status_ta_offset || op->status_3d_offset ||
	    op->status_stride || op->status_value_seed ||
	    op->sync_offset || op->sync_complete_seq ||
	    op->sync_pending_seq || op->sync_flags ||
	    op->owner_offset || op->owner_issue_cookie ||
	    op->owner_complete_cookie || op->report_offset ||
	    op->live_offset || op->manifest_offset ||
	    op->descriptor_offset || op->plan_offset ||
	    op->issue_plan_offset || op->result_offset ||
	    dependency_fields_present)
		return -EINVAL;

	switch (op_kind) {
	case PSB_SGX_LAB_SUBMIT_SGX_TA_OP_ISSUE:
		if (op->owner_state != DRM_PSB_SGX_LAB_OWNER_QUEUED &&
		    op->owner_state != DRM_PSB_SGX_LAB_OWNER_RUNNING)
			return -EINVAL;
		return op->packet_count ? 0 : -EINVAL;
	case PSB_SGX_LAB_SUBMIT_SGX_TA_OP_TRANSITION:
		if (op->owner_state != DRM_PSB_SGX_LAB_OWNER_QUEUED &&
		    op->owner_state != DRM_PSB_SGX_LAB_OWNER_RUNNING &&
		    op->owner_state != DRM_PSB_SGX_LAB_OWNER_COMPLETE)
			return -EINVAL;
		return 0;
	default:
		return -EINVAL;
	}
}

static int poulsbo_sgx_create_submit(struct drm_device *dev, u32 context,
				     u32 *submit_id_out)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;

	if (context != PSB_SGX_LAB_CONTEXT_DEFAULT &&
	    context != PSB_SGX_LAB_CONTEXT_PF)
		return -EINVAL;

	submit = kzalloc(sizeof(*submit), GFP_KERNEL);
	if (!submit)
		return -ENOMEM;

	submit->submit_id = atomic_inc_return(&dev_priv->sgx_lab_submit_id_next);
	submit->context = context;
	submit->created_jiffies = jiffies;
	submit->last_validate_status = -EINVAL;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	list_add_tail(&submit->head, &dev_priv->sgx_lab_submit_list);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	dev_info(dev->dev, "lab submit create submit_id=%u context=%s\n",
		 submit->submit_id, poulsbo_sgx_lab_context_name(context));
	if (submit_id_out)
		*submit_id_out = submit->submit_id;
	return 0;
}

static int poulsbo_sgx_drop_submit(struct drm_device *dev, u32 submit_id)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);
	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		mutex_unlock(&dev_priv->gtt_mutex);
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
		return -ENOENT;
	}

	list_del(&submit->head);
	if (dev_priv->sgx_lab_running_active &&
	    dev_priv->sgx_lab_active_running_submit_id == submit_id) {
		dev_priv->sgx_lab_running_active = false;
		dev_priv->sgx_lab_active_running_submit_id = 0;
	}
	if (dev_priv->sgx_lab_submit_focus_valid &&
	    dev_priv->sgx_lab_submit_focus_id == submit_id) {
		dev_priv->sgx_lab_submit_focus_valid = false;
		dev_priv->sgx_lab_submit_focus_id = 0;
		dev_priv->sgx_lab_submit_focus_jiffies = jiffies;
	}
	ret = poulsbo_sgx_refresh_dependency_dependents_locked(dev_priv,
							       submit_id);
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
	if (ret)
		return ret;

	dev_info(dev->dev, "lab submit drop submit_id=%u context=%s\n",
		 submit->submit_id,
		 poulsbo_sgx_lab_context_name(submit->context));
	kfree(submit);
	return 0;
}

static int poulsbo_sgx_focus_submit(struct drm_device *dev, u32 submit_id)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
		return -ENOENT;
	}
	dev_priv->sgx_lab_submit_focus_valid = true;
	dev_priv->sgx_lab_submit_focus_id = submit_id;
	dev_priv->sgx_lab_submit_focus_jiffies = jiffies;
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
	return 0;
}

static void poulsbo_sgx_clear_submit_focus(struct drm_psb_private *dev_priv)
{
	dev_priv->sgx_lab_submit_focus_valid = false;
	dev_priv->sgx_lab_submit_focus_id = 0;
	dev_priv->sgx_lab_submit_focus_jiffies = jiffies;
}

static int poulsbo_sgx_attach_submit_object(struct drm_device *dev, u32 submit_id,
					    const char *slot_name, u32 object_id)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *obj;
	u32 expected_role;
	u32 *slot;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);

	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
		return -ENOENT;
	}

	slot = poulsbo_sgx_submit_slot_ptr(submit, slot_name, &expected_role);
	if (!slot) {
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
		return -EINVAL;
	}

	obj = poulsbo_sgx_find_object_locked(dev_priv, object_id);
	if (!obj) {
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
		return -ENOENT;
	}

	if (obj->role != expected_role) {
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
		return -EINVAL;
	}

	*slot = object_id;
	poulsbo_sgx_disarm_submit_locked(submit);
	ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
	if (ret)
		return ret;

	dev_info(dev->dev,
		 "lab submit attach submit_id=%u slot=%s object_id=%u\n",
		 submit_id, slot_name, object_id);
	return 0;
}

static int poulsbo_sgx_detach_submit_object(struct drm_device *dev, u32 submit_id,
					    const char *slot_name)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	u32 *slot;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
		return -ENOENT;
	}

	slot = poulsbo_sgx_submit_slot_ptr(submit, slot_name, NULL);
	if (!slot) {
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
		return -EINVAL;
	}

	*slot = 0;
	poulsbo_sgx_disarm_submit_locked(submit);
	ret = poulsbo_sgx_refresh_submit_views_locked(dev_priv, submit);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
	if (ret)
		return ret;

	dev_info(dev->dev, "lab submit detach submit_id=%u slot=%s\n",
		 submit_id, slot_name);
	return 0;
}

static int poulsbo_sgx_validate_submit(struct drm_device *dev, u32 submit_id)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_submit *submit;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	submit = poulsbo_sgx_find_submit_locked(dev_priv, submit_id);
	if (!submit) {
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
		return -ENOENT;
	}

	ret = poulsbo_sgx_validate_submit_locked(dev_priv, submit);
	submit->last_validate_status = ret;
	submit->validate_count++;
	submit->last_validate_jiffies = jiffies;
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	dev_info(dev->dev,
		 "lab submit validate submit_id=%u context=%s status=%d count=%u\n",
		 submit_id,
		 poulsbo_sgx_lab_context_name(submit->context),
		 ret, submit->validate_count);
	return ret;
}

static u32 poulsbo_sgx_drop_all_submits_count(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	u32 dropped = 0;

	while (true) {
		struct psb_sgx_lab_submit *submit;

		mutex_lock(&dev_priv->sgx_lab_submit_mutex);
		if (list_empty(&dev_priv->sgx_lab_submit_list)) {
			dev_priv->sgx_lab_running_active = false;
			dev_priv->sgx_lab_active_running_submit_id = 0;
			dev_priv->sgx_lab_submit_focus_valid = false;
			dev_priv->sgx_lab_submit_focus_id = 0;
			dev_priv->sgx_lab_submit_focus_jiffies = jiffies;
			mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
			break;
		}
		submit = list_first_entry(&dev_priv->sgx_lab_submit_list,
					  struct psb_sgx_lab_submit, head);
		list_del(&submit->head);
		if (dev_priv->sgx_lab_running_active &&
		    dev_priv->sgx_lab_active_running_submit_id ==
			submit->submit_id) {
			dev_priv->sgx_lab_running_active = false;
			dev_priv->sgx_lab_active_running_submit_id = 0;
		}
		if (dev_priv->sgx_lab_submit_focus_valid &&
		    dev_priv->sgx_lab_submit_focus_id == submit->submit_id) {
			dev_priv->sgx_lab_submit_focus_valid = false;
			dev_priv->sgx_lab_submit_focus_id = 0;
			dev_priv->sgx_lab_submit_focus_jiffies = jiffies;
		}
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
		dev_info(dev->dev, "lab submit drop submit_id=%u during cleanup\n",
			 submit->submit_id);
		kfree(submit);
		dropped++;
	}

	return dropped;
}

static void poulsbo_sgx_drop_all_submits(struct drm_device *dev)
{
	(void)poulsbo_sgx_drop_all_submits_count(dev);
}

static int poulsbo_sgx_adopt_range(struct drm_device *dev, u32 range_id,
				   u32 *object_id_out)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct gtt_range *gt;
	struct psb_sgx_lab_object *obj;
	unsigned long gpu_va;
	int ret = 0;

	gt = psb_gtt_lab_get_range(dev, range_id);
	if (!gt)
		return -ENOENT;

	if (gt->stolen) {
		ret = -EINVAL;
		goto out_put;
	}

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		ret = -ENOMEM;
		goto out_put;
	}

	obj->object_id = atomic_inc_return(&dev_priv->sgx_lab_object_id_next);
	obj->gt = gt;
	obj->role = PSB_SGX_LAB_ROLE_GENERIC;
	obj->object_class = PSB_SGX_LAB_CLASS_GENERIC;
	obj->gpu_use = PSB_SGX_LAB_GPU_USE_GENERIC;
	obj->expected_context = PSB_SGX_LAB_CONTEXT_AUTO;
	obj->created_jiffies = jiffies;

	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);
	if (gt->sgx_lab_object_id) {
		ret = -EEXIST;
		mutex_unlock(&dev_priv->gtt_mutex);
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		goto out_free;
	}
	gt->sgx_lab_object_id = obj->object_id;
	gt->last_sgx_lab_adopt_jiffies = jiffies;
	mutex_unlock(&dev_priv->gtt_mutex);

	list_add_tail(&obj->head, &dev_priv->sgx_lab_object_list);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);

	gpu_va = dev_priv->gtt.gatt_start + gt->offset;
	dev_info(dev->dev,
		 "lab object adopt object_id=%u range_id=%u name=%s gpu_va=%#08lx size=%#llx\n",
		 obj->object_id, gt->debug_id,
		 gt->resource.name ? gt->resource.name : "?",
		 gpu_va, (unsigned long long)gt->gem.size);
	if (object_id_out)
		*object_id_out = obj->object_id;

	return 0;

out_free:
	kfree(obj);
out_put:
	psb_gtt_lab_put_range(gt);
	return ret;
}

static int poulsbo_sgx_drop_object(struct drm_device *dev, u32 object_id)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_object *obj;
	struct psb_sgx_lab_submit *submit;
	struct gtt_range *gt;
	int ret = 0;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	obj = poulsbo_sgx_find_object_locked(dev_priv, object_id);
	if (!obj) {
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
		return -ENOENT;
	}
	list_for_each_entry(submit, &dev_priv->sgx_lab_submit_list, head) {
		if (poulsbo_sgx_submit_references_object(submit, object_id)) {
			mutex_unlock(&dev_priv->sgx_lab_object_mutex);
			mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
			return -EBUSY;
		}
	}

	gt = obj->gt;
	while (true) {
		u32 active_binds;

		mutex_lock(&dev_priv->gtt_mutex);
		active_binds = gt->sgx_pf_bind_count;
		mutex_unlock(&dev_priv->gtt_mutex);
		if (!active_binds)
			break;

		ret = psb_gtt_lab_unbind_pf(dev, gt->debug_id);
		if (ret) {
			mutex_unlock(&dev_priv->sgx_lab_object_mutex);
			mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
			return ret;
		}
		obj->last_pf_unbind_jiffies = jiffies;
	}

	list_del(&obj->head);
	mutex_lock(&dev_priv->gtt_mutex);
	if (gt->sgx_lab_object_id == object_id) {
		gt->sgx_lab_object_id = 0;
		gt->last_sgx_lab_drop_jiffies = jiffies;
	}
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	dev_info(dev->dev,
		 "lab object drop object_id=%u range_id=%u name=%s gpu_va=%#08lx\n",
		 object_id, gt->debug_id,
		 gt->resource.name ? gt->resource.name : "?",
		 (unsigned long)(dev_priv->gtt.gatt_start + gt->offset));
	psb_gtt_lab_put_range(gt);
	kfree(obj);

	return 0;
}

static int poulsbo_sgx_bind_object(struct drm_device *dev, u32 object_id)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_object *obj;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	obj = poulsbo_sgx_find_object_locked(dev_priv, object_id);
	if (!obj) {
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		return -ENOENT;
	}

	ret = psb_gtt_lab_bind_pf(dev, obj->gt->debug_id);
	if (!ret)
		obj->last_pf_bind_jiffies = jiffies;
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);

	return ret;
}

static int poulsbo_sgx_unbind_object(struct drm_device *dev, u32 object_id)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_sgx_lab_object *obj;
	int ret;

	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	obj = poulsbo_sgx_find_object_locked(dev_priv, object_id);
	if (!obj) {
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		return -ENOENT;
	}

	ret = psb_gtt_lab_unbind_pf(dev, obj->gt->debug_id);
	if (!ret)
		obj->last_pf_unbind_jiffies = jiffies;
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);

	return ret;
}

static u32 poulsbo_sgx_drop_all_objects_count(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	u32 dropped = 0;

	while (true) {
		struct psb_sgx_lab_object *obj;
		int ret;

		mutex_lock(&dev_priv->sgx_lab_object_mutex);
		if (list_empty(&dev_priv->sgx_lab_object_list)) {
			mutex_unlock(&dev_priv->sgx_lab_object_mutex);
			break;
		}
		obj = list_first_entry(&dev_priv->sgx_lab_object_list,
				       struct psb_sgx_lab_object, head);
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);

		ret = poulsbo_sgx_drop_object(dev, obj->object_id);
		if (ret) {
			dev_warn(dev->dev,
				 "failed to drop SGX lab object %u during unload: %d\n",
				 obj->object_id, ret);
			break;
		}
		dropped++;
	}

	return dropped;
}

static void poulsbo_sgx_drop_all_objects(struct drm_device *dev)
{
	(void)poulsbo_sgx_drop_all_objects_count(dev);
}

static ssize_t poulsbo_lab_2d_show(struct device *kdev,
				   struct device_attribute *attr, char *buf)
{
	struct pci_dev *pdev = to_pci_dev(kdev);
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_psb_private *dev_priv;
	struct psb_fbdev *fbdev;
	struct drm_framebuffer *fb = NULL;
	struct fb_info *info = NULL;
	const char *path = "unavailable";

	if (!dev || !dev->dev_private)
		return scnprintf(buf, PAGE_SIZE, "driver not ready\n");

	dev_priv = dev->dev_private;
	fbdev = dev_priv->fbdev;
	if (fbdev) {
		info = fbdev->pfb.fbdev;
		fb = fbdev->psb_fb_helper.fb;
	}

	if (info && info->fbops) {
		if (info->fbops->fb_fillrect == psbfb_fillrect &&
		    info->fbops->fb_copyarea == psbfb_copyarea &&
		    info->fbops->fb_imageblit == psbfb_imageblit)
			path = "accel_2d";
		else
			path = "software";
	}

	if (!fb || !info)
		return scnprintf(buf, PAGE_SIZE,
				 "echo 'fill dx dy w h color [copy|xor]' > %s\n"
				 "echo 'copy sx sy dx dy w h' > %s\n"
				 "echo 'mono dx dy w h fg bg [checker|vertical|horizontal|diagonal|solid]' > %s\n"
				 "echo 'clear color' > %s\n"
				 "echo 'sync' > %s\n"
				 "echo 'pf_bind range_id' > poulsbo_sgx_bind\n"
				 "echo 'pf_unbind range_id' > poulsbo_sgx_bind\n"
				 "echo 'adopt range_id' > poulsbo_sgx_object\n"
				 "echo 'role object_id role_name' > poulsbo_sgx_object\n"
				 "echo 'class object_id class_name' > poulsbo_sgx_object\n"
				 "echo 'gpu_use object_id gpu_use_name' > poulsbo_sgx_object\n"
				 "echo 'context object_id context_name' > poulsbo_sgx_object\n"
				 "echo 'pf_bind object_id' > poulsbo_sgx_object\n"
				 "echo 'pf_unbind object_id' > poulsbo_sgx_object\n"
				 "echo 'drop object_id' > poulsbo_sgx_object\n"
				 "cat poulsbo_sgx_objects\n"
				 "echo 'create context_name' > poulsbo_sgx_submit\n"
				 "echo 'attach submit_id slot_name object_id' > poulsbo_sgx_submit\n"
				 "echo 'detach submit_id slot_name' > poulsbo_sgx_submit\n"
				 "echo 'kick submit_id cmd_type ccb_offset ctrl_flags num_ta_status num_3d_status ta_dependency' > poulsbo_sgx_submit\n"
				 "echo 'depend submit_id dependency_submit_id' > poulsbo_sgx_submit\n"
				 "echo 'depend2 submit_id dependency_submit_id' > poulsbo_sgx_submit\n"
				 "echo 'depticket2 submit_id required_ticket' > poulsbo_sgx_submit\n"
				 "echo 'depend3 submit_id dependency_submit_id' > poulsbo_sgx_submit\n"
				 "echo 'depticket3 submit_id required_ticket' > poulsbo_sgx_submit\n"
				 "echo 'depend_slot submit_id slot dependency_submit_id' > poulsbo_sgx_submit\n"
				 "echo 'depticket_slot submit_id slot required_ticket' > poulsbo_sgx_submit\n"
				 "echo 'depstate submit_id offset' > poulsbo_sgx_submit\n"
				 "echo 'depstate2 submit_id offset' > poulsbo_sgx_submit\n"
				 "echo 'depstate3 submit_id offset' > poulsbo_sgx_submit\n"
				 "echo 'depstate_slot submit_id slot offset' > poulsbo_sgx_submit\n"
			 "echo 'depclear_slot submit_id slot' > poulsbo_sgx_submit\n"
				 "echo 'ccb submit_id write_offset read_offset service cache data0 data1' > poulsbo_sgx_submit\n"
				 "echo 'queue submit_id base_offset size packet_size packet_count' > poulsbo_sgx_submit\n"
				 "echo 'status submit_id ta_offset 3d_offset stride seed' > poulsbo_sgx_submit\n"
				 "echo 'sync submit_id offset complete_seq pending_seq flags' > poulsbo_sgx_submit\n"
				 "echo 'owner submit_id offset owner_state issue_cookie complete_cookie' > poulsbo_sgx_submit\n"
				 "echo 'report submit_id offset' > poulsbo_sgx_submit\n"
				 "echo 'issue submit_id owner_state issued_packets' > poulsbo_sgx_submit\n"
				 "echo 'transition submit_id owner_state completed_packets' > poulsbo_sgx_submit\n"
				 "echo 'validate submit_id' > poulsbo_sgx_submit\n"
				 "echo 'drop submit_id' > poulsbo_sgx_submit\n"
				 "cat poulsbo_sgx_submits\n"
				 "cat poulsbo_sgx_state\n"
				 "cat poulsbo_gtt_state\n"
				 "fb=unavailable path=%s irq_count=%u fault_count=%u last_blit=%#x\n",
				 attr->attr.name, attr->attr.name,
				 attr->attr.name, attr->attr.name,
				 attr->attr.name,
				 path, dev_priv->sgx_irq.irq_count,
				 dev_priv->sgx_irq.fault_count,
				 dev_priv->sgx_irq.last_2d_blit_status);

	return scnprintf(buf, PAGE_SIZE,
			 "echo 'fill dx dy w h color [copy|xor]' > %s\n"
			 "echo 'copy sx sy dx dy w h' > %s\n"
			 "echo 'mono dx dy w h fg bg [checker|vertical|horizontal|diagonal|solid]' > %s\n"
			 "echo 'clear color' > %s\n"
			 "echo 'sync' > %s\n"
			 "echo 'pf_bind range_id' > poulsbo_sgx_bind\n"
			 "echo 'pf_unbind range_id' > poulsbo_sgx_bind\n"
			 "echo 'adopt range_id' > poulsbo_sgx_object\n"
			 "echo 'role object_id role_name' > poulsbo_sgx_object\n"
			 "echo 'class object_id class_name' > poulsbo_sgx_object\n"
			 "echo 'gpu_use object_id gpu_use_name' > poulsbo_sgx_object\n"
			 "echo 'context object_id context_name' > poulsbo_sgx_object\n"
			 "echo 'pf_bind object_id' > poulsbo_sgx_object\n"
			 "echo 'pf_unbind object_id' > poulsbo_sgx_object\n"
			 "echo 'drop object_id' > poulsbo_sgx_object\n"
			 "cat poulsbo_sgx_objects\n"
			 "echo 'create context_name' > poulsbo_sgx_submit\n"
			 "echo 'attach submit_id slot_name object_id' > poulsbo_sgx_submit\n"
			 "echo 'detach submit_id slot_name' > poulsbo_sgx_submit\n"
			 "echo 'kick submit_id cmd_type ccb_offset ctrl_flags num_ta_status num_3d_status ta_dependency' > poulsbo_sgx_submit\n"
			 "echo 'depend submit_id dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depend2 submit_id dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depticket2 submit_id required_ticket' > poulsbo_sgx_submit\n"
			 "echo 'depend3 submit_id dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depticket3 submit_id required_ticket' > poulsbo_sgx_submit\n"
			 "echo 'depend_slot submit_id slot dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depticket_slot submit_id slot required_ticket' > poulsbo_sgx_submit\n"
			 "echo 'depstate submit_id offset' > poulsbo_sgx_submit\n"
			 "echo 'depstate2 submit_id offset' > poulsbo_sgx_submit\n"
			 "echo 'depstate3 submit_id offset' > poulsbo_sgx_submit\n"
			 "echo 'depstate_slot submit_id slot offset' > poulsbo_sgx_submit\n"
			 "echo 'depclear_slot submit_id slot' > poulsbo_sgx_submit\n"
			 "echo 'ccb submit_id write_offset read_offset service cache data0 data1' > poulsbo_sgx_submit\n"
			 "echo 'queue submit_id base_offset size packet_size packet_count' > poulsbo_sgx_submit\n"
			 "echo 'status submit_id ta_offset 3d_offset stride seed' > poulsbo_sgx_submit\n"
			 "echo 'sync submit_id offset complete_seq pending_seq flags' > poulsbo_sgx_submit\n"
			 "echo 'owner submit_id offset owner_state issue_cookie complete_cookie' > poulsbo_sgx_submit\n"
			 "echo 'report submit_id offset' > poulsbo_sgx_submit\n"
			 "echo 'issue submit_id owner_state issued_packets' > poulsbo_sgx_submit\n"
			 "echo 'transition submit_id owner_state completed_packets' > poulsbo_sgx_submit\n"
			 "echo 'validate submit_id' > poulsbo_sgx_submit\n"
			 "echo 'drop submit_id' > poulsbo_sgx_submit\n"
			 "cat poulsbo_sgx_submits\n"
			 "cat poulsbo_sgx_state\n"
			 "cat poulsbo_gtt_state\n"
			 "fb=%ux%u bpp=%u pitch=%u path=%s irq_count=%u fault_count=%u last_blit=%#x\n",
			 attr->attr.name, attr->attr.name,
			 attr->attr.name, attr->attr.name,
			 attr->attr.name,
			 fb->width, fb->height, info->var.bits_per_pixel,
			 fb->pitches[0], path, dev_priv->sgx_irq.irq_count,
			 dev_priv->sgx_irq.fault_count,
			 dev_priv->sgx_irq.last_2d_blit_status);
}

static ssize_t poulsbo_lab_2d_store(struct device *kdev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct pci_dev *pdev = to_pci_dev(kdev);
	struct drm_device *dev = pci_get_drvdata(pdev);
	int ret;

	if (!dev || !dev->dev_private)
		return -ENODEV;

	ret = psbfb_run_lab_command(dev, buf, count);
	if (ret)
		return ret;

	return count;
}

static DEVICE_ATTR_RW(poulsbo_lab_2d);

static ssize_t poulsbo_sgx_bind_show(struct device *kdev,
				     struct device_attribute *attr, char *buf)
{
	struct pci_dev *pdev = to_pci_dev(kdev);
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_psb_private *dev_priv;
	int pf_pd_context = -1;

	if (!dev || !dev->dev_private)
		return scnprintf(buf, PAGE_SIZE, "driver not ready\n");

	dev_priv = dev->dev_private;
	if (dev_priv->pf_pd)
		pf_pd_context = dev_priv->pf_pd->hw_context;

	return scnprintf(buf, PAGE_SIZE,
			 "echo 'pf_bind range_id' > %s\n"
			 "echo 'pf_unbind range_id' > %s\n"
			 "echo 'adopt range_id' > poulsbo_sgx_object\n"
			 "echo 'role object_id role_name' > poulsbo_sgx_object\n"
			 "echo 'class object_id class_name' > poulsbo_sgx_object\n"
			 "echo 'gpu_use object_id gpu_use_name' > poulsbo_sgx_object\n"
			 "echo 'context object_id context_name' > poulsbo_sgx_object\n"
			 "echo 'pf_bind object_id' > poulsbo_sgx_object\n"
			 "echo 'pf_unbind object_id' > poulsbo_sgx_object\n"
			 "echo 'drop object_id' > poulsbo_sgx_object\n"
			 "cat poulsbo_sgx_objects\n"
			 "echo 'create context_name' > poulsbo_sgx_submit\n"
			 "echo 'attach submit_id slot_name object_id' > poulsbo_sgx_submit\n"
			 "echo 'detach submit_id slot_name' > poulsbo_sgx_submit\n"
			 "echo 'kick submit_id cmd_type ccb_offset ctrl_flags num_ta_status num_3d_status ta_dependency' > poulsbo_sgx_submit\n"
			 "echo 'depend submit_id dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depend2 submit_id dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depticket2 submit_id required_ticket' > poulsbo_sgx_submit\n"
			 "echo 'depend3 submit_id dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depticket3 submit_id required_ticket' > poulsbo_sgx_submit\n"
			 "echo 'depend_slot submit_id slot dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depticket_slot submit_id slot required_ticket' > poulsbo_sgx_submit\n"
			 "echo 'depstate submit_id offset' > poulsbo_sgx_submit\n"
			 "echo 'depstate2 submit_id offset' > poulsbo_sgx_submit\n"
			 "echo 'depstate3 submit_id offset' > poulsbo_sgx_submit\n"
			 "echo 'depstate_slot submit_id slot offset' > poulsbo_sgx_submit\n"
			 "echo 'depclear_slot submit_id slot' > poulsbo_sgx_submit\n"
			 "echo 'ccb submit_id write_offset read_offset service cache data0 data1' > poulsbo_sgx_submit\n"
			 "echo 'queue submit_id base_offset size packet_size packet_count' > poulsbo_sgx_submit\n"
			 "echo 'status submit_id ta_offset 3d_offset stride seed' > poulsbo_sgx_submit\n"
			 "echo 'sync submit_id offset complete_seq pending_seq flags' > poulsbo_sgx_submit\n"
			 "echo 'owner submit_id offset owner_state issue_cookie complete_cookie' > poulsbo_sgx_submit\n"
			 "echo 'issue submit_id owner_state issued_packets' > poulsbo_sgx_submit\n"
			 "echo 'transition submit_id owner_state completed_packets' > poulsbo_sgx_submit\n"
			 "echo 'validate submit_id' > poulsbo_sgx_submit\n"
			 "echo 'drop submit_id' > poulsbo_sgx_submit\n"
			 "cat poulsbo_sgx_submits\n"
			 "pf_pd_context=%d attr=%s\n",
			 attr->attr.name, attr->attr.name,
			 pf_pd_context, attr->attr.name);
}

static ssize_t poulsbo_sgx_bind_store(struct device *kdev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct pci_dev *pdev = to_pci_dev(kdev);
	struct drm_device *dev = pci_get_drvdata(pdev);
	char op[16];
	u32 range_id;
	int ret;

	if (!dev || !dev->dev_private)
		return -ENODEV;

	if (sscanf(buf, "%15s %u", op, &range_id) != 2)
		return -EINVAL;

	if (sysfs_streq(op, "pf_bind"))
		ret = psb_gtt_lab_bind_pf(dev, range_id);
	else if (sysfs_streq(op, "pf_unbind"))
		ret = psb_gtt_lab_unbind_pf(dev, range_id);
	else
		return -EINVAL;

	if (ret)
		return ret;

	return count;
}

static DEVICE_ATTR_RW(poulsbo_sgx_bind);

static ssize_t poulsbo_sgx_object_show(struct device *kdev,
				       struct device_attribute *attr, char *buf)
{
	struct pci_dev *pdev = to_pci_dev(kdev);
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_psb_private *dev_priv;
	unsigned int object_count = 0;
	struct psb_sgx_lab_object *obj;

	if (!dev || !dev->dev_private)
		return scnprintf(buf, PAGE_SIZE, "driver not ready\n");

	dev_priv = dev->dev_private;

	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	list_for_each_entry(obj, &dev_priv->sgx_lab_object_list, head)
		object_count++;
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);

	return scnprintf(buf, PAGE_SIZE,
			 "echo 'adopt range_id' > %s\n"
			 "echo 'role object_id role_name' > %s\n"
			 "echo 'class object_id class_name' > %s\n"
			 "echo 'gpu_use object_id gpu_use_name' > %s\n"
			 "echo 'context object_id context_name' > %s\n"
			 "echo 'pf_bind object_id' > %s\n"
			 "echo 'pf_unbind object_id' > %s\n"
			 "echo 'drop object_id' > %s\n"
			 "cat poulsbo_sgx_objects\n"
			 "echo 'create context_name' > poulsbo_sgx_submit\n"
			 "echo 'attach submit_id slot_name object_id' > poulsbo_sgx_submit\n"
			 "echo 'detach submit_id slot_name' > poulsbo_sgx_submit\n"
			 "echo 'kick submit_id cmd_type ccb_offset ctrl_flags num_ta_status num_3d_status ta_dependency' > poulsbo_sgx_submit\n"
			 "echo 'depend submit_id dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depend2 submit_id dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depticket2 submit_id required_ticket' > poulsbo_sgx_submit\n"
			 "echo 'depend3 submit_id dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depticket3 submit_id required_ticket' > poulsbo_sgx_submit\n"
			 "echo 'depend_slot submit_id slot dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depticket_slot submit_id slot required_ticket' > poulsbo_sgx_submit\n"
			 "echo 'depstate submit_id offset' > poulsbo_sgx_submit\n"
			 "echo 'depstate2 submit_id offset' > poulsbo_sgx_submit\n"
			 "echo 'depstate3 submit_id offset' > poulsbo_sgx_submit\n"
			 "echo 'depstate_slot submit_id slot offset' > poulsbo_sgx_submit\n"
			 "echo 'depclear_slot submit_id slot' > poulsbo_sgx_submit\n"
			 "echo 'ccb submit_id write_offset read_offset service cache data0 data1' > poulsbo_sgx_submit\n"
			 "echo 'queue submit_id base_offset size packet_size packet_count' > poulsbo_sgx_submit\n"
			 "echo 'status submit_id ta_offset 3d_offset stride seed' > poulsbo_sgx_submit\n"
			 "echo 'sync submit_id offset complete_seq pending_seq flags' > poulsbo_sgx_submit\n"
			 "echo 'owner submit_id offset owner_state issue_cookie complete_cookie' > poulsbo_sgx_submit\n"
			 "echo 'issue submit_id owner_state issued_packets' > poulsbo_sgx_submit\n"
			 "echo 'transition submit_id owner_state completed_packets' > poulsbo_sgx_submit\n"
			 "echo 'validate submit_id' > poulsbo_sgx_submit\n"
			 "echo 'drop submit_id' > poulsbo_sgx_submit\n"
			 "cat poulsbo_sgx_submits\n"
			 "object_count=%u attr=%s\n",
			 attr->attr.name, attr->attr.name,
			 attr->attr.name, attr->attr.name,
			 attr->attr.name, attr->attr.name,
			 attr->attr.name, attr->attr.name,
			 object_count, attr->attr.name);
}

static ssize_t poulsbo_sgx_object_store(struct device *kdev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct pci_dev *pdev = to_pci_dev(kdev);
	struct drm_device *dev = pci_get_drvdata(pdev);
	char op[16];
	char arg[16];
	u32 id;
	u32 object_id = 0;
	int ret;

	if (!dev || !dev->dev_private)
		return -ENODEV;

	if (sscanf(buf, "%15s %u %15s", op, &id, arg) < 2)
		return -EINVAL;

	if (sysfs_streq(op, "adopt"))
		ret = poulsbo_sgx_adopt_range(dev, id, &object_id);
	else if (sysfs_streq(op, "role")) {
		u32 role;

		ret = poulsbo_sgx_lab_parse_role(arg, &role);
		if (!ret)
			ret = poulsbo_sgx_set_object_role(dev, id, role);
	} else if (sysfs_streq(op, "class")) {
		u32 object_class;

		ret = poulsbo_sgx_lab_parse_class(arg, &object_class);
		if (!ret)
			ret = poulsbo_sgx_set_object_class(dev, id, object_class);
	} else if (sysfs_streq(op, "gpu_use")) {
		u32 gpu_use;

		ret = poulsbo_sgx_lab_parse_gpu_use(arg, &gpu_use);
		if (!ret)
			ret = poulsbo_sgx_set_object_gpu_use(dev, id, gpu_use);
	} else if (sysfs_streq(op, "context")) {
		u32 context;

		ret = poulsbo_sgx_lab_parse_context(arg, &context);
		if (!ret)
			ret = poulsbo_sgx_set_object_context(dev, id, context);
	}
	else if (sysfs_streq(op, "pf_bind"))
		ret = poulsbo_sgx_bind_object(dev, id);
	else if (sysfs_streq(op, "pf_unbind"))
		ret = poulsbo_sgx_unbind_object(dev, id);
	else if (sysfs_streq(op, "drop"))
		ret = poulsbo_sgx_drop_object(dev, id);
	else
		return -EINVAL;

	if (ret)
		return ret;

	if (object_id)
		dev_info(dev->dev,
			 "lab object adopt command completed object_id=%u range_id=%u attr=%s\n",
			 object_id, id, attr->attr.name);

	return count;
}

static DEVICE_ATTR_RW(poulsbo_sgx_object);

static ssize_t poulsbo_sgx_objects_show(struct device *kdev,
					struct device_attribute *attr,
					char *buf)
{
	struct pci_dev *pdev = to_pci_dev(kdev);
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_psb_private *dev_priv;
	struct psb_sgx_lab_object *obj;
	ssize_t pos = 0;
	unsigned int shown = 0;
	unsigned int total = 0;
	bool truncated = false;

	if (!dev || !dev->dev_private)
		return scnprintf(buf, PAGE_SIZE, "driver not ready\n");

	dev_priv = dev->dev_private;

	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);
	list_for_each_entry(obj, &dev_priv->sgx_lab_object_list, head) {
		struct gtt_range *gt = obj->gt;
		unsigned long gpu_va = poulsbo_sgx_object_gpu_va(dev_priv, obj);

		total++;
		if (PAGE_SIZE - pos < 288) {
			truncated = true;
			break;
		}

		pos += scnprintf(buf + pos, PAGE_SIZE - pos,
				 "object id=%u range_id=%u name=%s role=%s class=%s gpu_use=%s expected_context=%s size=%#llx gpu_va=%#08lx stolen=%u in_gart=%d mmapping=%u npage=%d pf_bind_count=%u pf_bind_total=%u pf_unbind_total=%u created_jiffies=%lu last_pf_bind_jiffies=%lu last_pf_unbind_jiffies=%lu last_role_jiffies=%lu last_class_jiffies=%lu last_gpu_use_jiffies=%lu last_context_jiffies=%lu\n",
				 obj->object_id, gt->debug_id,
				 gt->resource.name ? gt->resource.name : "?",
				 poulsbo_sgx_lab_role_name(obj->role),
				 poulsbo_sgx_lab_class_name(obj->object_class),
				 poulsbo_sgx_lab_gpu_use_name(obj->gpu_use),
				 poulsbo_sgx_lab_context_name(obj->expected_context),
				 (unsigned long long)gt->gem.size, gpu_va,
				 gt->stolen, gt->in_gart, gt->mmapping,
				 gt->npage, gt->sgx_pf_bind_count,
				 gt->sgx_pf_bind_total,
				 gt->sgx_pf_unbind_total,
				 obj->created_jiffies,
				 obj->last_pf_bind_jiffies,
				 obj->last_pf_unbind_jiffies,
				 obj->last_role_jiffies,
				 obj->last_class_jiffies,
				 obj->last_gpu_use_jiffies,
				 obj->last_context_jiffies);
		shown++;
	}
	mutex_unlock(&dev_priv->gtt_mutex);

	if (truncated)
		pos += scnprintf(buf + pos, PAGE_SIZE - pos,
				 "objects_shown=%u objects_total_at_least=%u truncated=1 attr=%s\n",
				 shown, total, attr->attr.name);
	else
		pos += scnprintf(buf + pos, PAGE_SIZE - pos,
				 "objects_shown=%u objects_total=%u truncated=0 attr=%s\n",
				 shown, total, attr->attr.name);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);

	return pos;
}

static DEVICE_ATTR_RO(poulsbo_sgx_objects);

static ssize_t poulsbo_sgx_submit_show(struct device *kdev,
				       struct device_attribute *attr, char *buf)
{
	struct pci_dev *pdev = to_pci_dev(kdev);
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_psb_private *dev_priv;
	unsigned int submit_count = 0;
	struct psb_sgx_lab_submit *submit;

	if (!dev || !dev->dev_private)
		return scnprintf(buf, PAGE_SIZE, "driver not ready\n");

	dev_priv = dev->dev_private;
	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	list_for_each_entry(submit, &dev_priv->sgx_lab_submit_list, head)
		submit_count++;
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	return scnprintf(buf, PAGE_SIZE,
			 "echo 'create context_name' > poulsbo_sgx_submit\n"
			 "echo 'attach submit_id slot_name object_id' > poulsbo_sgx_submit\n"
			 "echo 'detach submit_id slot_name' > poulsbo_sgx_submit\n"
			 "echo 'kick submit_id cmd_type ccb_offset ctrl_flags num_ta_status num_3d_status ta_dependency' > poulsbo_sgx_submit\n"
			 "echo 'depend submit_id dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depend2 submit_id dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depticket2 submit_id required_ticket' > poulsbo_sgx_submit\n"
			 "echo 'depend3 submit_id dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depticket3 submit_id required_ticket' > poulsbo_sgx_submit\n"
			 "echo 'depend_slot submit_id slot dependency_submit_id' > poulsbo_sgx_submit\n"
			 "echo 'depticket_slot submit_id slot required_ticket' > poulsbo_sgx_submit\n"
			 "echo 'depstate submit_id offset' > poulsbo_sgx_submit\n"
			 "echo 'depstate2 submit_id offset' > poulsbo_sgx_submit\n"
			 "echo 'depstate3 submit_id offset' > poulsbo_sgx_submit\n"
			 "echo 'depstate_slot submit_id slot offset' > poulsbo_sgx_submit\n"
			 "echo 'depclear_slot submit_id slot' > poulsbo_sgx_submit\n"
			 "echo 'ccb submit_id write_offset read_offset service cache data0 data1' > poulsbo_sgx_submit\n"
			 "echo 'queue submit_id base_offset size packet_size packet_count' > poulsbo_sgx_submit\n"
			 "echo 'status submit_id ta_offset 3d_offset stride seed' > poulsbo_sgx_submit\n"
			 "echo 'sync submit_id offset complete_seq pending_seq flags' > poulsbo_sgx_submit\n"
			 "echo 'owner submit_id offset owner_state issue_cookie complete_cookie' > poulsbo_sgx_submit\n"
			 "echo 'issue submit_id owner_state issued_packets' > poulsbo_sgx_submit\n"
			 "echo 'transition submit_id owner_state completed_packets' > poulsbo_sgx_submit\n"
			 "echo 'validate submit_id' > poulsbo_sgx_submit\n"
			 "echo 'focus submit_id' > poulsbo_sgx_submit\n"
			 "echo 'focus clear' > poulsbo_sgx_submit\n"
			 "echo 'drop submit_id' > poulsbo_sgx_submit\n"
			 "cat poulsbo_sgx_submits\n"
			 "cat poulsbo_sgx_submit_state\n"
			 "cat poulsbo_sgx_coord_state\n"
			 "submit_count=%u attr=%s\n",
			 submit_count, attr->attr.name);
}

static ssize_t poulsbo_sgx_submit_store(struct device *kdev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct pci_dev *pdev = to_pci_dev(kdev);
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_psb_private *dev_priv = dev ? dev->dev_private : NULL;
	char op[16];
	char arg0[16];
	char arg1[16];
	char arg2[16];
	char arg3[16];
	char arg4[16];
	char arg5[16];
	u32 id;
	u32 id2;
	u32 submit_id = 0;
	u32 context;
	u32 cmd_type;
	u32 ccb_offset;
	u32 ctrl_flags;
	u32 num_ta_status_vals;
	u32 num_3d_status_vals;
	u32 ta_dependency;
	u32 dependency_submit_id;
	u32 slot_index;
	u32 dependency_ticket;
	u32 dependency_ticket_secondary;
	u32 dependency_ticket_tertiary;
	u32 dependency_offset;
	u32 write_offset;
	u32 read_offset;
	u32 service_address;
	u32 cache_control;
	u32 data0;
	u32 data1;
	u32 queue_base_offset;
	u32 queue_size;
	u32 queue_packet_size;
	u32 queue_packet_count;
	u32 status_ta_offset;
	u32 status_3d_offset;
	u32 status_stride;
	u32 status_seed;
	u32 sync_offset;
	u32 sync_complete_seq;
	u32 sync_pending_seq;
	u32 sync_flags;
	u32 owner_offset;
	u32 owner_state;
	u32 owner_issue_cookie;
	u32 owner_complete_cookie;
	u32 issue_owner_state;
	u32 issue_issued_packets;
	u32 transition_completed_packets;
	u32 transition_owner_state;
	int ret = -EINVAL;

	if (!dev || !dev->dev_private)
		return -ENODEV;

	memset(arg0, 0, sizeof(arg0));
	memset(arg1, 0, sizeof(arg1));
	memset(arg2, 0, sizeof(arg2));
	memset(arg3, 0, sizeof(arg3));
	memset(arg4, 0, sizeof(arg4));
	memset(arg5, 0, sizeof(arg5));
	if (sscanf(buf, "%15s", op) != 1)
		return -EINVAL;

	if (sysfs_streq(op, "create")) {
		if (sscanf(buf, "%15s %15s", op, arg0) != 2)
			return -EINVAL;
		ret = poulsbo_sgx_lab_parse_context(arg0, &context);
		if (!ret)
			ret = poulsbo_sgx_create_submit(dev, context, &submit_id);
	} else if (sysfs_streq(op, "validate")) {
		if (sscanf(buf, "%15s %u", op, &id) != 2)
			return -EINVAL;
		ret = poulsbo_sgx_validate_submit(dev, id);
	} else if (sysfs_streq(op, "arm")) {
		if (sscanf(buf, "%15s %u", op, &id) != 2)
			return -EINVAL;
		ret = poulsbo_sgx_arm_submit(dev, id);
	} else if (sysfs_streq(op, "drop")) {
		if (sscanf(buf, "%15s %u", op, &id) != 2)
			return -EINVAL;
		ret = poulsbo_sgx_drop_submit(dev, id);
	} else if (sysfs_streq(op, "attach")) {
		if (sscanf(buf, "%15s %u %15s %u", op, &id, arg0, &id2) != 4)
			return -EINVAL;
		ret = poulsbo_sgx_attach_submit_object(dev, id, arg0, id2);
	} else if (sysfs_streq(op, "detach")) {
		if (sscanf(buf, "%15s %u %15s", op, &id, arg0) != 3)
			return -EINVAL;
		ret = poulsbo_sgx_detach_submit_object(dev, id, arg0);
	} else if (sysfs_streq(op, "kick")) {
		if (sscanf(buf, "%15s %u %15s %15s %15s %15s %15s %15s",
			   op, &id, arg0, arg1, arg2, arg3, arg4, arg5) != 8)
			return -EINVAL;
		ret = poulsbo_sgx_lab_parse_cmd_type(arg0, &cmd_type);
		if (ret)
			return ret;
		ret = kstrtou32(arg1, 0, &ccb_offset);
		if (ret)
			return ret;
		ret = kstrtou32(arg2, 0, &ctrl_flags);
		if (ret)
			return ret;
		ret = kstrtou32(arg3, 0, &num_ta_status_vals);
		if (ret)
			return ret;
		ret = kstrtou32(arg4, 0, &num_3d_status_vals);
		if (ret)
			return ret;
		ret = kstrtou32(arg5, 0, &ta_dependency);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_kick(dev, id, cmd_type,
							ccb_offset, ctrl_flags,
							num_ta_status_vals,
							num_3d_status_vals,
							ta_dependency);
	} else if (sysfs_streq(op, "depend")) {
		if (sscanf(buf, "%15s %u %15s", op, &id, arg0) != 3)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &dependency_submit_id);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_dependency(dev, id,
							      dependency_submit_id);
	} else if (sysfs_streq(op, "depend2")) {
		if (sscanf(buf, "%15s %u %15s", op, &id, arg0) != 3)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &dependency_submit_id);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_dependency2(dev, id,
							       dependency_submit_id);
	} else if (sysfs_streq(op, "depticket2")) {
		if (sscanf(buf, "%15s %u %15s", op, &id, arg0) != 3)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &dependency_ticket_secondary);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_dependency_ticket2(
			dev, id, dependency_ticket_secondary);
	} else if (sysfs_streq(op, "depend3")) {
		if (sscanf(buf, "%15s %u %15s", op, &id, arg0) != 3)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &dependency_submit_id);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_dependency3(dev, id,
							       dependency_submit_id);
	} else if (sysfs_streq(op, "depend_slot")) {
		if (sscanf(buf, "%15s %u %15s %15s", op, &id, arg0, arg1) != 4)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &slot_index);
		if (ret)
			return ret;
		ret = kstrtou32(arg1, 0, &dependency_submit_id);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_dependency_slot_user(
			dev, id, slot_index, dependency_submit_id);
	} else if (sysfs_streq(op, "depticket3")) {
		if (sscanf(buf, "%15s %u %15s", op, &id, arg0) != 3)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &dependency_ticket_tertiary);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_dependency_ticket3(
			dev, id, dependency_ticket_tertiary);
	} else if (sysfs_streq(op, "depticket_slot")) {
		if (sscanf(buf, "%15s %u %15s %15s", op, &id, arg0, arg1) != 4)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &slot_index);
		if (ret)
			return ret;
		ret = kstrtou32(arg1, 0, &dependency_ticket);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_dependency_ticket_slot(
			dev, id, slot_index, dependency_ticket);
	} else if (sysfs_streq(op, "depstate")) {
		if (sscanf(buf, "%15s %u %15s", op, &id, arg0) != 3)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &dependency_offset);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_dependency_state(
			dev, id, dependency_offset);
	} else if (sysfs_streq(op, "depstate2")) {
		if (sscanf(buf, "%15s %u %15s", op, &id, arg0) != 3)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &dependency_offset);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_dependency_state2(
			dev, id, dependency_offset);
	} else if (sysfs_streq(op, "depstate3")) {
		if (sscanf(buf, "%15s %u %15s", op, &id, arg0) != 3)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &dependency_offset);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_dependency_state3(
			dev, id, dependency_offset);
	} else if (sysfs_streq(op, "depstate_slot")) {
		if (sscanf(buf, "%15s %u %15s %15s", op, &id, arg0, arg1) != 4)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &slot_index);
		if (ret)
			return ret;
		ret = kstrtou32(arg1, 0, &dependency_offset);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_dependency_state_slot_user(
			dev, id, slot_index, dependency_offset);
	} else if (sysfs_streq(op, "depclear_slot")) {
		if (sscanf(buf, "%15s %u %15s", op, &id, arg0) != 3)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &slot_index);
		if (ret)
			return ret;
		ret = poulsbo_sgx_clear_submit_dependency_slot_user(
			dev, id, slot_index);
	} else if (sysfs_streq(op, "ccb")) {
		if (sscanf(buf, "%15s %u %15s %15s %15s %15s %15s %15s",
			   op, &id, arg0, arg1, arg2, arg3, arg4, arg5) != 8)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &write_offset);
		if (ret)
			return ret;
		ret = kstrtou32(arg1, 0, &read_offset);
		if (ret)
			return ret;
		ret = kstrtou32(arg2, 0, &service_address);
		if (ret)
			return ret;
		ret = kstrtou32(arg3, 0, &cache_control);
		if (ret)
			return ret;
		ret = kstrtou32(arg4, 0, &data0);
		if (ret)
			return ret;
		ret = kstrtou32(arg5, 0, &data1);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_ccb(dev, id,
						       write_offset, read_offset,
						       service_address,
						       cache_control,
						       data0, data1);
	} else if (sysfs_streq(op, "status")) {
		if (sscanf(buf, "%15s %u %15s %15s %15s %15s",
			   op, &id, arg0, arg1, arg2, arg3) != 6)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &status_ta_offset);
		if (ret)
			return ret;
		ret = kstrtou32(arg1, 0, &status_3d_offset);
		if (ret)
			return ret;
		ret = kstrtou32(arg2, 0, &status_stride);
		if (ret)
			return ret;
		ret = kstrtou32(arg3, 0, &status_seed);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_status(dev, id,
							  status_ta_offset,
							  status_3d_offset,
							  status_stride,
							  status_seed);
	} else if (sysfs_streq(op, "queue")) {
		if (sscanf(buf, "%15s %u %15s %15s %15s %15s",
			   op, &id, arg0, arg1, arg2, arg3) != 6)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &queue_base_offset);
		if (ret)
			return ret;
		ret = kstrtou32(arg1, 0, &queue_size);
		if (ret)
			return ret;
		ret = kstrtou32(arg2, 0, &queue_packet_size);
		if (ret)
			return ret;
		ret = kstrtou32(arg3, 0, &queue_packet_count);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_queue(dev, id,
							 queue_base_offset,
							 queue_size,
							 queue_packet_size,
							 queue_packet_count);
	} else if (sysfs_streq(op, "sync")) {
		if (sscanf(buf, "%15s %u %15s %15s %15s %15s",
			   op, &id, arg0, arg1, arg2, arg3) != 6)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &sync_offset);
		if (ret)
			return ret;
		ret = kstrtou32(arg1, 0, &sync_complete_seq);
		if (ret)
			return ret;
		ret = kstrtou32(arg2, 0, &sync_pending_seq);
		if (ret)
			return ret;
		ret = kstrtou32(arg3, 0, &sync_flags);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_sync(dev, id,
							sync_offset,
							sync_complete_seq,
							sync_pending_seq,
							sync_flags);
	} else if (sysfs_streq(op, "owner")) {
		if (sscanf(buf, "%15s %u %15s %15s %15s %15s",
			   op, &id, arg0, arg1, arg2, arg3) != 6)
			return -EINVAL;
		ret = kstrtou32(arg0, 0, &owner_offset);
		if (ret)
			return ret;
		ret = poulsbo_sgx_lab_parse_owner_state(arg1, &owner_state);
		if (ret)
			return ret;
		ret = kstrtou32(arg2, 0, &owner_issue_cookie);
		if (ret)
			return ret;
		ret = kstrtou32(arg3, 0, &owner_complete_cookie);
		if (ret)
			return ret;
		ret = poulsbo_sgx_configure_submit_owner(dev, id,
							 owner_offset,
							 owner_state,
							 owner_issue_cookie,
							 owner_complete_cookie);
		} else if (sysfs_streq(op, "report")) {
			if (sscanf(buf, "%15s %u %15s", op, &id, arg0) != 3)
				return -EINVAL;
			ret = kstrtou32(arg0, 0, &owner_offset);
			if (ret)
				return ret;
			ret = poulsbo_sgx_configure_submit_report(dev, id,
								  owner_offset);
		} else if (sysfs_streq(op, "result")) {
			if (sscanf(buf, "%15s %u %15s", op, &id, arg0) != 3)
				return -EINVAL;
			ret = kstrtou32(arg0, 0, &owner_offset);
			if (ret)
				return ret;
			ret = poulsbo_sgx_configure_submit_result(dev, id,
								  owner_offset);
	} else if (sysfs_streq(op, "issue")) {
		struct drm_psb_sgx_lab_submit_op request_op = {};
		struct psb_sgx_lab_submit_sgx_ta_submit_op_request_frontend
			*frontend = NULL;

			if (sscanf(buf, "%15s %u %15s %15s",
				   op, &id, arg0, arg1) != 4)
			return -EINVAL;
		ret = poulsbo_sgx_lab_parse_owner_state(arg0, &issue_owner_state);
		if (ret)
			return ret;
		ret = kstrtou32(arg1, 0, &issue_issued_packets);
		if (ret)
			return ret;
		request_op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE;
		request_op.submit_id = id;
		request_op.owner_state = issue_owner_state;
		request_op.packet_count = issue_issued_packets;
		ret = poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_frontend(
			dev, &request_op, false, &frontend);
		if (!ret && frontend) {
			const struct psb_sgx_lab_submit_sgx_ta_request_work_outcome
				*outcome = &frontend->outcome;

			dev_info(dev->dev,
				 "lab submit issue submit_id=%u state=%s issued_packets=%u read_offset=%#x new_write_offset=%#x outstanding=%u wrapped=%u pending_seq=%#x issue_cookie=%#x epoch=%u refill=%u global_issue_ticket=%u global_running_ticket=%u refresh_views=%u next_issue_count=%u issue_advanced=%u next_issue_ticket=%u next_issue_submit_id=%u\n",
				 outcome->submit_id,
				 poulsbo_sgx_lab_owner_state_name(outcome->owner_state),
				 outcome->packet_count,
				 outcome->selected_read_offset,
				 outcome->selected_write_offset,
				 outcome->selected_outstanding_packets,
				 outcome->selected_wrapped,
				 outcome->selected_seq,
				 outcome->selected_cookie,
				 outcome->epoch,
				 outcome->refilled_from_complete,
				 outcome->global_issue_ticket,
				 outcome->global_running_ticket,
				 outcome->refresh_views,
				 outcome->next_issue_count,
				 outcome->issue_advanced,
				 outcome->next_state.issue_ticket,
				 outcome->next_state.issue_submit_id);
		}
		kfree(frontend);
	} else if (sysfs_streq(op, "transition")) {
		struct drm_psb_sgx_lab_submit_op request_op = {};
		struct psb_sgx_lab_submit_sgx_ta_submit_op_request_frontend
			*frontend = NULL;

		if (sscanf(buf, "%15s %u %15s %15s",
			   op, &id, arg0, arg1) != 4)
			return -EINVAL;
		ret = poulsbo_sgx_lab_parse_owner_state(arg0,
							&transition_owner_state);
		if (ret)
			return ret;
		ret = kstrtou32(arg1, 0, &transition_completed_packets);
		if (ret)
			return ret;
		request_op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION;
		request_op.submit_id = id;
		request_op.owner_state = transition_owner_state;
		request_op.packet_count = transition_completed_packets;
		ret = poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_frontend(
			dev, &request_op, false, &frontend);
		if (!ret && frontend) {
			const struct psb_sgx_lab_submit_sgx_ta_request_work_outcome
				*outcome = &frontend->outcome;

			dev_info(dev->dev,
				 "lab submit transition submit_id=%u state=%s completed_packets=%u new_read_offset=%#x new_write_offset=%#x new_ccb_offset=%#x outstanding=%u wrapped=%u complete_seq=%#x complete_cookie=%#x epoch=%u global_running_ticket=%u global_complete_ticket=%u refresh_views=%u next_transition_count=%u running_advanced=%u complete_advanced=%u next_running_ticket=%u next_complete_ticket=%u active_running_changed=%u\n",
				 outcome->submit_id,
				 poulsbo_sgx_lab_owner_state_name(outcome->owner_state),
				 outcome->packet_count,
				 outcome->selected_read_offset,
				 outcome->selected_write_offset,
				 outcome->selected_ccb_offset,
				 outcome->selected_outstanding_packets,
				 outcome->selected_wrapped,
				 outcome->selected_seq,
				 outcome->selected_cookie,
				 outcome->epoch,
				 outcome->global_running_ticket,
				 outcome->global_complete_ticket,
				 outcome->refresh_views,
				 outcome->next_transition_count,
				 outcome->running_advanced,
				 outcome->complete_advanced,
				 outcome->next_state.running_ticket,
				 outcome->next_state.complete_ticket,
				 outcome->active_running_changed);
		}
		kfree(frontend);
	} else if (sysfs_streq(op, "focus")) {
		if (sscanf(buf, "%15s %15s", op, arg0) != 2)
			return -EINVAL;
		mutex_lock(&dev_priv->sgx_lab_submit_mutex);
		if (sysfs_streq(arg0, "clear")) {
			poulsbo_sgx_clear_submit_focus(dev_priv);
			ret = 0;
		} else {
			ret = kstrtou32(arg0, 0, &id);
			if (!ret && !poulsbo_sgx_find_submit_locked(dev_priv, id))
				ret = -ENOENT;
			if (!ret) {
				dev_priv->sgx_lab_submit_focus_valid = true;
				dev_priv->sgx_lab_submit_focus_id = id;
				dev_priv->sgx_lab_submit_focus_jiffies = jiffies;
			}
		}
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
	}

	if (ret)
		return ret;

	if (submit_id)
		dev_info(dev->dev,
			 "lab submit create command completed submit_id=%u attr=%s\n",
			 submit_id, attr->attr.name);

	return count;
}

static DEVICE_ATTR_RW(poulsbo_sgx_submit);

static ssize_t poulsbo_sgx_submit_format_locked(struct drm_psb_private *dev_priv,
						struct psb_sgx_lab_submit *submit,
						char *buf, size_t size)
{
	struct psb_sgx_lab_object *control_obj;
	struct psb_sgx_lab_object *pds_obj;
	struct psb_sgx_lab_object *use_obj;
	struct psb_sgx_lab_object *target_obj;
	struct psb_sgx_lab_object *status_obj;
	unsigned long control_gpu_va = 0;
	unsigned long pds_gpu_va = 0;
	unsigned long use_gpu_va = 0;
	unsigned long target_gpu_va = 0;
	unsigned long status_gpu_va = 0;
	ssize_t len;

	control_obj = poulsbo_sgx_find_object_locked(dev_priv,
					     submit->control_object_id);
	pds_obj = poulsbo_sgx_find_object_locked(dev_priv,
					 submit->pds_object_id);
	use_obj = poulsbo_sgx_find_object_locked(dev_priv,
					 submit->use_object_id);
	target_obj = poulsbo_sgx_find_object_locked(dev_priv,
					    submit->target_object_id);
	status_obj = poulsbo_sgx_find_object_locked(dev_priv,
					    submit->status_object_id);
	if (control_obj)
		control_gpu_va = poulsbo_sgx_object_gpu_va(dev_priv,
							   control_obj);
	if (pds_obj)
		pds_gpu_va = poulsbo_sgx_object_gpu_va(dev_priv, pds_obj);
	if (use_obj)
		use_gpu_va = poulsbo_sgx_object_gpu_va(dev_priv, use_obj);
	if (target_obj)
		target_gpu_va = poulsbo_sgx_object_gpu_va(dev_priv,
							   target_obj);
	if (status_obj)
		status_gpu_va = poulsbo_sgx_object_gpu_va(dev_priv, status_obj);

	len = scnprintf(buf, size,
			"submit id=%u context=%s cmd_type=%s ccb_offset=%#x ctrl_flags=%#x ta_status=%u 3d_status=%u ta_dependency=%u dependency_submit_id=%u dependency_offset=%#x dependency_snapshot_source_submit_id=%u dependency_required_ticket=%u dependency_observed_complete_ticket=%u dependency_flags=%#x dependency2_submit_id=%u dependency2_offset=%#x dependency2_snapshot_source_submit_id=%u dependency2_required_ticket=%u dependency2_observed_complete_ticket=%u dependency2_flags=%#x dependency3_submit_id=%u dependency3_offset=%#x dependency3_snapshot_source_submit_id=%u dependency3_required_ticket=%u dependency3_observed_complete_ticket=%u dependency3_flags=%#x dependency4_submit_id=%u dependency4_offset=%#x dependency4_snapshot_source_submit_id=%u dependency4_required_ticket=%u dependency4_observed_complete_ticket=%u dependency4_flags=%#x ccb_write_offset=%#x ccb_read_offset=%#x ccb_service=%#x ccb_cache=%#x ccb_data0=%#x ccb_data1=%#x queue_base_offset=%#x queue_size=%#x queue_packet_size=%#x queue_packet_count=%u queue_tail_packet_count=%u queue_head_packet_count=%u queue_wrap=%u sync_offset=%#x sync_complete_seq=%#x sync_pending_seq=%#x sync_flags=%#x owner_offset=%#x owner_state=%s owner_issue_cookie=%#x owner_complete_cookie=%#x status_ta_offset=%#x status_3d_offset=%#x status_stride=%#x status_seed=%#x control_object_id=%u control_gpu_va=%#08lx pds_object_id=%u pds_gpu_va=%#08lx use_object_id=%u use_gpu_va=%#08lx target_object_id=%u target_gpu_va=%#08lx status_object_id=%u status_gpu_va=%#08lx validate_count=%u last_validate_status=%d packet_config_count=%u ccb_config_count=%u queue_config_count=%u status_config_count=%u sync_config_count=%u dependency_config_count=%u dependency2_config_count=%u dependency3_config_count=%u dependency4_config_count=%u owner_config_count=%u queue_epoch=%u global_issue_ticket=%u global_running_ticket=%u global_complete_ticket=%u prepare_count=%u arm_valid=%u arm_count=%u arm_prepare_count=%u arm_queue_packet_count=%u arm_queue_tail_packet_count=%u arm_queue_head_packet_count=%u arm_ccb_read_offset=%#x arm_ccb_write_offset=%#x arm_sync_complete_seq=%#x arm_sync_pending_seq=%#x arm_owner_state=%s arm_owner_issue_cookie=%#x arm_owner_complete_cookie=%#x arm_queue_epoch=%u arm_global_issue_ticket=%u issue_count=%u transition_count=%u created_jiffies=%lu last_validate_jiffies=%lu last_packet_jiffies=%lu last_ccb_jiffies=%lu last_queue_jiffies=%lu last_status_jiffies=%lu last_sync_jiffies=%lu last_dependency_jiffies=%lu last_dependency2_jiffies=%lu last_dependency3_jiffies=%lu last_dependency4_jiffies=%lu last_dependency_block_jiffies=%lu last_dependency2_block_jiffies=%lu last_dependency3_block_jiffies=%lu last_dependency4_block_jiffies=%lu last_owner_jiffies=%lu last_epoch_jiffies=%lu last_prepare_jiffies=%lu last_arm_jiffies=%lu last_issue_jiffies=%lu last_transition_jiffies=%lu",
			 submit->submit_id,
			 poulsbo_sgx_lab_context_name(submit->context),
			 poulsbo_sgx_lab_cmd_type_name(submit->cmd_type),
			 submit->ccb_offset,
			 submit->ctrl_flags,
			 submit->num_ta_status_vals,
			 submit->num_3d_status_vals,
			 submit->ta_dependency,
			 submit->dependency_submit_id,
			 submit->dependency_offset,
			 submit->dependency_snapshot_source_submit_id,
			 submit->dependency_snapshot_required_ticket,
			 submit->dependency_snapshot_observed_complete_ticket,
			 submit->dependency_snapshot_flags,
			 submit->dependency_submit_id_secondary,
			 submit->dependency_offset_secondary,
			 submit->dependency_snapshot_source_submit_id_secondary,
			 submit->dependency_snapshot_required_ticket_secondary,
			 submit->dependency_snapshot_observed_complete_ticket_secondary,
			 submit->dependency_snapshot_flags_secondary,
			 submit->dependency_submit_id_tertiary,
			 submit->dependency_offset_tertiary,
			 submit->dependency_snapshot_source_submit_id_tertiary,
			 submit->dependency_snapshot_required_ticket_tertiary,
			 submit->dependency_snapshot_observed_complete_ticket_tertiary,
			 submit->dependency_snapshot_flags_tertiary,
			 submit->dependency_submit_id_quaternary,
			 submit->dependency_offset_quaternary,
			 submit->dependency_snapshot_source_submit_id_quaternary,
			 submit->dependency_snapshot_required_ticket_quaternary,
			 submit->dependency_snapshot_observed_complete_ticket_quaternary,
			 submit->dependency_snapshot_flags_quaternary,
			 submit->ccb_write_offset,
			 submit->ccb_read_offset,
			 submit->ccb_service_address,
			 submit->ccb_cache_control,
			 submit->ccb_data0,
			 submit->ccb_data1,
			 submit->queue_base_offset,
			 submit->queue_size,
			 submit->queue_packet_size,
			 submit->queue_packet_count,
			 submit->queue_tail_packet_count,
			 submit->queue_head_packet_count,
			 submit->queue_head_packet_count ? 1 : 0,
			 submit->sync_offset,
			 submit->sync_complete_seq,
			 submit->sync_pending_seq,
			 submit->sync_flags,
			 submit->owner_offset,
			 poulsbo_sgx_lab_owner_state_name(submit->owner_state),
			 submit->owner_issue_cookie,
			 submit->owner_complete_cookie,
			 submit->status_ta_offset,
			 submit->status_3d_offset,
			 submit->status_stride,
			 submit->status_value_seed,
			 submit->control_object_id,
			 control_gpu_va,
			 submit->pds_object_id,
			 pds_gpu_va,
			 submit->use_object_id,
			 use_gpu_va,
			 submit->target_object_id,
			 target_gpu_va,
			 submit->status_object_id,
			 status_gpu_va,
			 submit->validate_count,
			 submit->last_validate_status,
			 submit->packet_config_count,
			 submit->ccb_config_count,
			 submit->queue_config_count,
			 submit->status_config_count,
			 submit->sync_config_count,
			 submit->dependency_config_count,
			 submit->dependency_config_count_secondary,
			 submit->dependency_config_count_tertiary,
			 submit->dependency_config_count_quaternary,
			 submit->owner_config_count,
			 submit->queue_epoch,
			 submit->global_issue_ticket,
			 submit->global_running_ticket,
			 submit->global_complete_ticket,
			 submit->prepare_count,
			 submit->arm_valid ? 1 : 0,
			 submit->arm_count,
			 submit->arm_prepare_count,
			 submit->arm_queue_packet_count,
			 submit->arm_queue_tail_packet_count,
			 submit->arm_queue_head_packet_count,
			 submit->arm_ccb_read_offset,
			 submit->arm_ccb_write_offset,
			 submit->arm_sync_complete_seq,
			 submit->arm_sync_pending_seq,
			 poulsbo_sgx_lab_owner_state_name(submit->arm_owner_state),
			 submit->arm_owner_issue_cookie,
			 submit->arm_owner_complete_cookie,
			 submit->arm_queue_epoch,
			 submit->arm_global_issue_ticket,
			 submit->issue_count,
			 submit->transition_count,
			 submit->created_jiffies,
			 submit->last_validate_jiffies,
			 submit->last_packet_jiffies,
			 submit->last_ccb_jiffies,
			 submit->last_queue_jiffies,
			 submit->last_status_jiffies,
			 submit->last_sync_jiffies,
			 submit->last_dependency_jiffies,
			 submit->last_dependency_secondary_jiffies,
			 submit->last_dependency_tertiary_jiffies,
			 submit->last_dependency_quaternary_jiffies,
			 submit->last_dependency_block_jiffies,
			 submit->last_dependency_block_secondary_jiffies,
			 submit->last_dependency_block_tertiary_jiffies,
			 submit->last_dependency_block_quaternary_jiffies,
			 submit->last_owner_jiffies,
			 submit->last_epoch_jiffies,
			 submit->last_prepare_jiffies,
			 submit->last_arm_jiffies,
			 submit->last_issue_jiffies,
			 submit->last_transition_jiffies);

	len += scnprintf(buf + len, size > len ? size - len : 0,
			 " emit_valid=%u emit_count=%u emit_prepare_count=%u emit_arm_count=%u emit_context=%s emit_cmd_type=%s emit_ctrl_flags=%#x emit_ta_status=%u emit_3d_status=%u emit_ta_dependency=%u emit_control_object_id=%u emit_control_gpu_va=%#08llx emit_pds_object_id=%u emit_pds_gpu_va=%#08llx emit_use_object_id=%u emit_use_gpu_va=%#08llx emit_target_object_id=%u emit_target_gpu_va=%#08llx emit_status_object_id=%u emit_status_gpu_va=%#08llx emit_ccb_offset=%#x emit_ccb_write_offset=%#x emit_ccb_read_offset=%#x emit_ccb_service=%#x emit_ccb_cache=%#x emit_ccb_data0=%#x emit_ccb_data1=%#x emit_queue_base_offset=%#x emit_queue_size=%#x emit_queue_packet_size=%#x emit_queue_packet_count=%u emit_queue_tail_packet_count=%u emit_queue_head_packet_count=%u emit_sync_complete_seq=%#x emit_sync_pending_seq=%#x emit_sync_flags=%#x emit_owner_state=%s emit_owner_issue_cookie=%#x emit_owner_complete_cookie=%#x emit_queue_epoch=%u emit_global_issue_ticket=%u emit_dependency_active_count=%u last_emit_jiffies=%lu\n",
			 submit->emit_valid ? 1 : 0,
			 submit->emit_count,
			 submit->emit_prepare_count,
			 submit->emit_arm_count,
			 poulsbo_sgx_lab_context_name(submit->emit_context),
			 poulsbo_sgx_lab_cmd_type_name(submit->emit_cmd_type),
			 submit->emit_ctrl_flags,
			 submit->emit_num_ta_status_vals,
			 submit->emit_num_3d_status_vals,
			 submit->emit_ta_dependency,
			 submit->emit_control_object_id,
			 (unsigned long long)submit->emit_control_gpu_va,
			 submit->emit_pds_object_id,
			 (unsigned long long)submit->emit_pds_gpu_va,
			 submit->emit_use_object_id,
			 (unsigned long long)submit->emit_use_gpu_va,
			 submit->emit_target_object_id,
			 (unsigned long long)submit->emit_target_gpu_va,
			 submit->emit_status_object_id,
			 (unsigned long long)submit->emit_status_gpu_va,
			 submit->emit_ccb_offset,
			 submit->emit_ccb_write_offset,
			 submit->emit_ccb_read_offset,
			 submit->emit_ccb_service_address,
			 submit->emit_ccb_cache_control,
			 submit->emit_ccb_data0,
			 submit->emit_ccb_data1,
			 submit->emit_queue_base_offset,
			 submit->emit_queue_size,
			 submit->emit_queue_packet_size,
			 submit->emit_queue_packet_count,
			 submit->emit_queue_tail_packet_count,
			 submit->emit_queue_head_packet_count,
			 submit->emit_sync_complete_seq,
			 submit->emit_sync_pending_seq,
			 submit->emit_sync_flags,
			 poulsbo_sgx_lab_owner_state_name(submit->emit_owner_state),
			 submit->emit_owner_issue_cookie,
			 submit->emit_owner_complete_cookie,
			 submit->emit_queue_epoch,
			 submit->emit_global_issue_ticket,
			 submit->emit_dependency_active_count,
			 submit->last_emit_jiffies);

	len += scnprintf(buf + len, size > len ? size - len : 0,
			 " retired_valid=%u retired_count=%u retired_prepare_count=%u retired_arm_count=%u retired_emit_count=%u retired_launch_count=%u retired_dispatch_count=%u retired_context=%s retired_cmd_type=%s retired_owner_state=%s retired_issued_packets=%u retired_completed_packets=%u retired_refilled_from_complete=%u retired_queue_packet_count=%u retired_sync_complete_seq=%#x retired_sync_pending_seq=%#x retired_owner_issue_cookie=%#x retired_owner_complete_cookie=%#x retired_queue_epoch=%u retired_global_issue_ticket=%u retired_global_running_ticket=%u retired_global_complete_ticket=%u retired_dependency_active_count=%u retired_control_object_id=%u retired_status_object_id=%u retired_control_gpu_va=%#08llx retired_status_gpu_va=%#08llx last_retired_jiffies=%lu\n",
			 submit->retired_valid ? 1 : 0,
			 submit->retired_count,
			 submit->retired_prepare_count,
			 submit->retired_arm_count,
			 submit->retired_emit_count,
			 submit->retired_launch_count,
			 submit->retired_dispatch_count,
			 poulsbo_sgx_lab_context_name(submit->retired_context),
			 poulsbo_sgx_lab_cmd_type_name(submit->retired_cmd_type),
			 poulsbo_sgx_lab_owner_state_name(
				 submit->retired_owner_state),
			 submit->retired_issued_packets,
			 submit->retired_completed_packets,
			 submit->retired_refilled_from_complete,
			 submit->retired_queue_packet_count,
			 submit->retired_sync_complete_seq,
			 submit->retired_sync_pending_seq,
			 submit->retired_owner_issue_cookie,
			 submit->retired_owner_complete_cookie,
			 submit->retired_queue_epoch,
			 submit->retired_global_issue_ticket,
			 submit->retired_global_running_ticket,
			 submit->retired_global_complete_ticket,
			 submit->retired_dependency_active_count,
			 submit->retired_control_object_id,
			 submit->retired_status_object_id,
			 (unsigned long long)submit->retired_control_gpu_va,
			 (unsigned long long)submit->retired_status_gpu_va,
			 submit->last_retired_jiffies);

	return len;
}

static void poulsbo_sgx_fill_submit_query_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_submit *submit,
	u32 resolved_index,
	u32 total_submit_count,
	u32 matching_submit_count,
	u32 resolved_match_index,
	u32 resolved_slot,
	u32 resolved_object_id,
	u32 resolved_range_id,
	u32 resolved_range_type,
	u64 query_gpu_va_offset,
	struct drm_psb_sgx_lab_submit_query *query)
{
	struct psb_sgx_lab_object *control_obj;
	struct psb_sgx_lab_object *pds_obj;
	struct psb_sgx_lab_object *use_obj;
	struct psb_sgx_lab_object *target_obj;
	struct psb_sgx_lab_object *status_obj;
	struct psb_sgx_lab_object *resolved_obj = NULL;
	unsigned int slot;
	struct psb_sgx_lab_ccb_record_words ccb_record;
	u32 live_query_end = 0;
	u32 manifest_query_end = 0;
	u32 descriptor_query_end = 0;
	u32 plan_query_end = 0;
	u32 issue_plan_query_end = 0;
	u32 result_query_end = 0;
	struct psb_sgx_lab_live_words live;
	struct psb_sgx_lab_manifest_words manifest;
	struct psb_sgx_lab_descriptor_words descriptor;
	struct psb_sgx_lab_plan_words plan;
	struct psb_sgx_lab_issue_plan_words issue_plan;
	struct psb_sgx_lab_result_words result;
	u32 request_flags = query->flags;
	u32 request_submit_id = query->submit_id;
	u32 request_query_index = query->query_index;
	u64 request_query_gpu_va = query->query_gpu_va;
	u32 request_query_match_index = query->query_match_index;
	u32 active_count = 0;

	memset(query, 0, sizeof(*query));
	query->size = sizeof(*query);
	query->flags = request_flags;
	query->submit_id = request_submit_id;
	query->query_index = request_query_index;
	query->query_gpu_va = request_query_gpu_va;
	query->query_match_index = request_query_match_index;
	query->resolved_submit_id = submit->submit_id;
	query->resolved_index = resolved_index;
	query->total_submit_count = total_submit_count;
	query->matching_submit_count = matching_submit_count;
	query->resolved_match_index = resolved_match_index;
	query->resolved_slot = resolved_slot;
	query->resolved_object_id = resolved_object_id;
	query->resolved_range_id = resolved_range_id;
	query->resolved_range_type = resolved_range_type;
	query->query_gpu_va_offset = query_gpu_va_offset;
	query->context = submit->context;
	query->cmd_type = submit->cmd_type;
	query->owner_state = submit->owner_state;
	query->last_validate_status = submit->last_validate_status;
	query->num_ta_status_vals = submit->num_ta_status_vals;
	query->num_3d_status_vals = submit->num_3d_status_vals;
	query->ta_dependency = submit->ta_dependency;
	query->ctrl_flags = submit->ctrl_flags;
	query->control_object_id = submit->control_object_id;
	query->pds_object_id = submit->pds_object_id;
	query->use_object_id = submit->use_object_id;
	query->target_object_id = submit->target_object_id;
	query->status_object_id = submit->status_object_id;
	query->ccb_offset = submit->ccb_offset;
	query->ccb_write_offset = submit->ccb_write_offset;
	query->ccb_read_offset = submit->ccb_read_offset;
	query->ccb_service_address = submit->ccb_service_address;
	query->ccb_cache_control = submit->ccb_cache_control;
	query->ccb_data0 = submit->ccb_data0;
	query->ccb_data1 = submit->ccb_data1;
	query->ccb_pds_gpu_va32 = 0;
	query->ccb_use_gpu_va32 = 0;
	query->ccb_target_gpu_va32 = 0;
	query->ccb_status_gpu_va32 = 0;
	query->queue_base_offset = submit->queue_base_offset;
	query->queue_size = submit->queue_size;
	query->queue_packet_size = submit->queue_packet_size;
	query->queue_packet_count = submit->queue_packet_count;
	query->queue_tail_packet_count = submit->queue_tail_packet_count;
	query->queue_head_packet_count = submit->queue_head_packet_count;
	query->queue_epoch = submit->queue_epoch;
	query->status_ta_offset = submit->status_ta_offset;
	query->status_3d_offset = submit->status_3d_offset;
	query->status_stride = submit->status_stride;
	query->status_value_seed = submit->status_value_seed;
	query->sync_offset = submit->sync_offset;
	query->sync_complete_seq = submit->sync_complete_seq;
	query->sync_pending_seq = submit->sync_pending_seq;
	query->sync_flags = submit->sync_flags;
	query->owner_offset = submit->owner_offset;
	query->owner_issue_cookie = submit->owner_issue_cookie;
	query->owner_complete_cookie = submit->owner_complete_cookie;
	query->global_issue_ticket = submit->global_issue_ticket;
	query->global_running_ticket = submit->global_running_ticket;
	query->global_complete_ticket = submit->global_complete_ticket;
	query->validate_count = submit->validate_count;
	query->prepare_count = submit->prepare_count;
	query->arm_valid = submit->arm_valid ? 1 : 0;
	query->arm_count = submit->arm_count;
	query->arm_prepare_count = submit->arm_prepare_count;
	query->arm_queue_packet_count = submit->arm_queue_packet_count;
	query->arm_queue_tail_packet_count = submit->arm_queue_tail_packet_count;
	query->arm_queue_head_packet_count = submit->arm_queue_head_packet_count;
	query->arm_ccb_read_offset = submit->arm_ccb_read_offset;
	query->arm_ccb_write_offset = submit->arm_ccb_write_offset;
	query->arm_sync_complete_seq = submit->arm_sync_complete_seq;
	query->arm_sync_pending_seq = submit->arm_sync_pending_seq;
	query->arm_owner_state = submit->arm_owner_state;
	query->arm_owner_issue_cookie = submit->arm_owner_issue_cookie;
	query->arm_owner_complete_cookie = submit->arm_owner_complete_cookie;
	query->arm_queue_epoch = submit->arm_queue_epoch;
	query->arm_global_issue_ticket = submit->arm_global_issue_ticket;
	query->emit_valid = submit->emit_valid ? 1 : 0;
	query->emit_count = submit->emit_count;
	query->emit_prepare_count = submit->emit_prepare_count;
	query->emit_arm_count = submit->emit_arm_count;
	query->emit_context = submit->emit_context;
	query->emit_cmd_type = submit->emit_cmd_type;
	query->emit_owner_state = submit->emit_owner_state;
	query->emit_ctrl_flags = submit->emit_ctrl_flags;
	query->emit_num_ta_status_vals = submit->emit_num_ta_status_vals;
	query->emit_num_3d_status_vals = submit->emit_num_3d_status_vals;
	query->emit_ta_dependency = submit->emit_ta_dependency;
	query->emit_control_object_id = submit->emit_control_object_id;
	query->emit_pds_object_id = submit->emit_pds_object_id;
	query->emit_use_object_id = submit->emit_use_object_id;
	query->emit_target_object_id = submit->emit_target_object_id;
	query->emit_status_object_id = submit->emit_status_object_id;
	query->emit_ccb_offset = submit->emit_ccb_offset;
	query->emit_ccb_write_offset = submit->emit_ccb_write_offset;
	query->emit_ccb_read_offset = submit->emit_ccb_read_offset;
	query->emit_ccb_service_address = submit->emit_ccb_service_address;
	query->emit_ccb_cache_control = submit->emit_ccb_cache_control;
	query->emit_ccb_data0 = submit->emit_ccb_data0;
	query->emit_ccb_data1 = submit->emit_ccb_data1;
	query->emit_queue_base_offset = submit->emit_queue_base_offset;
	query->emit_queue_size = submit->emit_queue_size;
	query->emit_queue_packet_size = submit->emit_queue_packet_size;
	query->emit_queue_packet_count = submit->emit_queue_packet_count;
	query->emit_queue_tail_packet_count = submit->emit_queue_tail_packet_count;
	query->emit_queue_head_packet_count = submit->emit_queue_head_packet_count;
	query->emit_sync_complete_seq = submit->emit_sync_complete_seq;
	query->emit_sync_pending_seq = submit->emit_sync_pending_seq;
	query->emit_sync_flags = submit->emit_sync_flags;
	query->emit_owner_issue_cookie = submit->emit_owner_issue_cookie;
	query->emit_owner_complete_cookie = submit->emit_owner_complete_cookie;
	query->emit_queue_epoch = submit->emit_queue_epoch;
	query->emit_global_issue_ticket = submit->emit_global_issue_ticket;
	query->emit_dependency_active_count =
		submit->emit_dependency_active_count;
	query->emit_control_gpu_va = submit->emit_control_gpu_va;
	query->emit_pds_gpu_va = submit->emit_pds_gpu_va;
	query->emit_use_gpu_va = submit->emit_use_gpu_va;
	query->emit_target_gpu_va = submit->emit_target_gpu_va;
	query->emit_status_gpu_va = submit->emit_status_gpu_va;
	query->launch_valid = submit->launch_valid ? 1 : 0;
	query->launch_consumed = submit->launch_consumed ? 1 : 0;
	query->launch_count = submit->launch_count;
	query->launch_prepare_count = submit->launch_prepare_count;
	query->launch_arm_count = submit->launch_arm_count;
	query->launch_emit_count = submit->launch_emit_count;
	query->launch_context = submit->launch_context;
	query->launch_cmd_type = submit->launch_cmd_type;
	query->launch_owner_state = submit->launch_owner_state;
	query->launch_issued_packets = submit->launch_issued_packets;
	query->launch_refilled_from_complete =
		submit->launch_refilled_from_complete;
	query->launch_ctrl_flags = submit->launch_ctrl_flags;
	query->launch_num_ta_status_vals =
		submit->launch_num_ta_status_vals;
	query->launch_num_3d_status_vals =
		submit->launch_num_3d_status_vals;
	query->launch_ta_dependency = submit->launch_ta_dependency;
	query->launch_control_object_id = submit->launch_control_object_id;
	query->launch_pds_object_id = submit->launch_pds_object_id;
	query->launch_use_object_id = submit->launch_use_object_id;
	query->launch_target_object_id = submit->launch_target_object_id;
	query->launch_status_object_id = submit->launch_status_object_id;
	query->launch_ccb_offset = submit->launch_ccb_offset;
	query->launch_ccb_write_offset = submit->launch_ccb_write_offset;
	query->launch_ccb_read_offset = submit->launch_ccb_read_offset;
	query->launch_new_write_offset = submit->launch_new_write_offset;
	query->launch_ccb_service_address =
		submit->launch_ccb_service_address;
	query->launch_ccb_cache_control = submit->launch_ccb_cache_control;
	query->launch_ccb_data0 = submit->launch_ccb_data0;
	query->launch_ccb_data1 = submit->launch_ccb_data1;
	query->launch_queue_base_offset = submit->launch_queue_base_offset;
	query->launch_queue_size = submit->launch_queue_size;
	query->launch_queue_packet_size = submit->launch_queue_packet_size;
	query->launch_queue_packet_count = submit->launch_queue_packet_count;
	query->launch_queue_tail_packet_count =
		submit->launch_queue_tail_packet_count;
	query->launch_queue_head_packet_count =
		submit->launch_queue_head_packet_count;
	query->launch_sync_offset = submit->launch_sync_offset;
	query->launch_sync_complete_seq = submit->launch_sync_complete_seq;
	query->launch_sync_pending_seq = submit->launch_sync_pending_seq;
	query->launch_sync_flags = submit->launch_sync_flags;
	query->launch_owner_offset = submit->launch_owner_offset;
	query->launch_owner_issue_cookie = submit->launch_owner_issue_cookie;
	query->launch_owner_complete_cookie =
		submit->launch_owner_complete_cookie;
	query->launch_queue_epoch = submit->launch_queue_epoch;
	query->launch_global_issue_ticket =
		submit->launch_global_issue_ticket;
	query->launch_global_running_ticket =
		submit->launch_global_running_ticket;
	query->launch_global_complete_ticket =
		submit->launch_global_complete_ticket;
	query->launch_dependency_active_count =
		submit->launch_dependency_active_count;
	query->launch_control_gpu_va = submit->launch_control_gpu_va;
	query->launch_pds_gpu_va = submit->launch_pds_gpu_va;
	query->launch_use_gpu_va = submit->launch_use_gpu_va;
	query->launch_target_gpu_va = submit->launch_target_gpu_va;
	query->launch_status_gpu_va = submit->launch_status_gpu_va;
	query->dispatch_valid = submit->dispatch_valid ? 1 : 0;
	query->dispatch_count = submit->dispatch_count;
	query->dispatch_prepare_count = submit->dispatch_prepare_count;
	query->dispatch_arm_count = submit->dispatch_arm_count;
	query->dispatch_emit_count = submit->dispatch_emit_count;
	query->dispatch_launch_count = submit->dispatch_launch_count;
	query->dispatch_context = submit->dispatch_context;
	query->dispatch_cmd_type = submit->dispatch_cmd_type;
	query->dispatch_owner_state = submit->dispatch_owner_state;
	query->dispatch_issued_packets = submit->dispatch_issued_packets;
	query->dispatch_completed_packets = submit->dispatch_completed_packets;
	query->dispatch_refilled_from_complete =
		submit->dispatch_refilled_from_complete;
	query->dispatch_ctrl_flags = submit->dispatch_ctrl_flags;
	query->dispatch_num_ta_status_vals =
		submit->dispatch_num_ta_status_vals;
	query->dispatch_num_3d_status_vals =
		submit->dispatch_num_3d_status_vals;
	query->dispatch_ta_dependency = submit->dispatch_ta_dependency;
	query->dispatch_control_object_id = submit->dispatch_control_object_id;
	query->dispatch_pds_object_id = submit->dispatch_pds_object_id;
	query->dispatch_use_object_id = submit->dispatch_use_object_id;
	query->dispatch_target_object_id = submit->dispatch_target_object_id;
	query->dispatch_status_object_id = submit->dispatch_status_object_id;
	query->dispatch_ccb_offset = submit->dispatch_ccb_offset;
	query->dispatch_ccb_write_offset = submit->dispatch_ccb_write_offset;
	query->dispatch_ccb_read_offset = submit->dispatch_ccb_read_offset;
	query->dispatch_ccb_service_address =
		submit->dispatch_ccb_service_address;
	query->dispatch_ccb_cache_control =
		submit->dispatch_ccb_cache_control;
	query->dispatch_ccb_data0 = submit->dispatch_ccb_data0;
	query->dispatch_ccb_data1 = submit->dispatch_ccb_data1;
	query->dispatch_queue_base_offset = submit->dispatch_queue_base_offset;
	query->dispatch_queue_size = submit->dispatch_queue_size;
	query->dispatch_queue_packet_size = submit->dispatch_queue_packet_size;
	query->dispatch_queue_packet_count = submit->dispatch_queue_packet_count;
	query->dispatch_queue_tail_packet_count =
		submit->dispatch_queue_tail_packet_count;
	query->dispatch_queue_head_packet_count =
		submit->dispatch_queue_head_packet_count;
	query->dispatch_sync_offset = submit->dispatch_sync_offset;
	query->dispatch_sync_complete_seq = submit->dispatch_sync_complete_seq;
	query->dispatch_sync_pending_seq = submit->dispatch_sync_pending_seq;
	query->dispatch_sync_flags = submit->dispatch_sync_flags;
	query->dispatch_owner_offset = submit->dispatch_owner_offset;
	query->dispatch_owner_issue_cookie =
		submit->dispatch_owner_issue_cookie;
	query->dispatch_owner_complete_cookie =
		submit->dispatch_owner_complete_cookie;
	query->dispatch_queue_epoch = submit->dispatch_queue_epoch;
	query->dispatch_global_issue_ticket =
		submit->dispatch_global_issue_ticket;
	query->dispatch_global_running_ticket =
		submit->dispatch_global_running_ticket;
	query->dispatch_global_complete_ticket =
		submit->dispatch_global_complete_ticket;
	query->dispatch_dependency_active_count =
		submit->dispatch_dependency_active_count;
	query->dispatch_control_gpu_va = submit->dispatch_control_gpu_va;
	query->dispatch_pds_gpu_va = submit->dispatch_pds_gpu_va;
	query->dispatch_use_gpu_va = submit->dispatch_use_gpu_va;
	query->dispatch_target_gpu_va = submit->dispatch_target_gpu_va;
	query->dispatch_status_gpu_va = submit->dispatch_status_gpu_va;
	query->retired_valid = submit->retired_valid ? 1 : 0;
	query->retired_count = submit->retired_count;
	query->retired_prepare_count = submit->retired_prepare_count;
	query->retired_arm_count = submit->retired_arm_count;
	query->retired_emit_count = submit->retired_emit_count;
	query->retired_launch_count = submit->retired_launch_count;
	query->retired_dispatch_count = submit->retired_dispatch_count;
	query->retired_context = submit->retired_context;
	query->retired_cmd_type = submit->retired_cmd_type;
	query->retired_owner_state = submit->retired_owner_state;
	query->retired_issued_packets = submit->retired_issued_packets;
	query->retired_completed_packets = submit->retired_completed_packets;
	query->retired_refilled_from_complete =
		submit->retired_refilled_from_complete;
	query->retired_queue_packet_count = submit->retired_queue_packet_count;
	query->retired_sync_complete_seq = submit->retired_sync_complete_seq;
	query->retired_sync_pending_seq = submit->retired_sync_pending_seq;
	query->retired_owner_issue_cookie = submit->retired_owner_issue_cookie;
	query->retired_owner_complete_cookie =
		submit->retired_owner_complete_cookie;
	query->retired_queue_epoch = submit->retired_queue_epoch;
	query->retired_global_issue_ticket =
		submit->retired_global_issue_ticket;
	query->retired_global_running_ticket =
		submit->retired_global_running_ticket;
	query->retired_global_complete_ticket =
		submit->retired_global_complete_ticket;
	query->retired_dependency_active_count =
		submit->retired_dependency_active_count;
	query->retired_control_object_id = submit->retired_control_object_id;
	query->retired_status_object_id = submit->retired_status_object_id;
	query->retired_control_gpu_va = submit->retired_control_gpu_va;
	query->retired_status_gpu_va = submit->retired_status_gpu_va;
	query->report_offset = submit->report_offset;
	query->report_stage = submit->report_stage;
	query->report_stage_count = submit->report_stage_count;
	query->report_owner_state = submit->report_owner_state;
	query->report_queue_packet_count = submit->report_queue_packet_count;
	query->report_completed_packets = submit->report_completed_packets;
	query->report_dependency_active_count =
		submit->report_dependency_active_count;
	query->report_queue_epoch = submit->report_queue_epoch;
	query->report_global_issue_ticket =
		submit->report_global_issue_ticket;
	query->report_global_running_ticket =
		submit->report_global_running_ticket;
	query->report_global_complete_ticket =
		submit->report_global_complete_ticket;
	query->report_flags = submit->report_flags;
	query->report_config_count = submit->report_config_count;
	query->live_offset = submit->live_offset;
	query->live_stage = 0;
	query->live_stage_count = 0;
	query->live_prepare_count = 0;
	query->live_arm_count = 0;
	query->live_emit_count = 0;
	query->live_launch_count = 0;
	query->live_dispatch_count = 0;
	query->live_retired_count = 0;
	query->live_context = 0;
	query->live_cmd_type = 0;
	query->live_owner_state = 0;
	query->live_issued_packets = 0;
	query->live_completed_packets = 0;
	query->live_dependency_active_count = 0;
	query->live_refilled_from_complete = 0;
	query->live_queue_packet_count = 0;
	query->live_queue_epoch = 0;
	query->live_queue_base_offset = 0;
	query->live_queue_size = 0;
	query->live_queue_packet_size = 0;
	query->live_ccb_offset = 0;
	query->live_ccb_write_offset = 0;
	query->live_ccb_read_offset = 0;
	query->live_sync_offset = 0;
	query->live_sync_complete_seq = 0;
	query->live_sync_pending_seq = 0;
	query->live_sync_flags = 0;
	query->live_owner_offset = 0;
	query->live_owner_issue_cookie = 0;
	query->live_owner_complete_cookie = 0;
	query->live_global_issue_ticket = 0;
	query->live_global_running_ticket = 0;
	query->live_global_complete_ticket = 0;
	query->live_ctrl_flags = 0;
	query->live_report_flags = 0;
	query->live_num_ta_status_vals = 0;
	query->live_num_3d_status_vals = 0;
	query->live_ta_dependency = 0;
	query->live_status_ta_offset = 0;
	query->live_status_3d_offset = 0;
	query->live_status_stride = 0;
	query->live_status_value_seed = 0;
	query->live_control_object_id = 0;
	query->live_status_object_id = 0;
	query->live_ccb_service_address = 0;
	query->live_ccb_cache_control = 0;
	query->live_ccb_data0 = 0;
	query->live_ccb_data1 = 0;
	query->live_control_gpu_va32 = 0;
	query->live_status_gpu_va32 = 0;
	query->live_pds_gpu_va32 = 0;
	query->live_use_gpu_va32 = 0;
	query->live_target_gpu_va32 = 0;
	query->live_ccb_status_gpu_va32 = 0;
	query->live_config_count = submit->live_config_count;
	query->result_offset = submit->result_offset;
	query->result_source = DRM_PSB_SGX_LAB_RESULT_SOURCE_NONE;
	query->result_config_count = submit->result_config_count;
	query->manifest_offset = submit->manifest_offset;
	query->manifest_stage = 0;
	query->manifest_stage_count = 0;
	query->manifest_context = 0;
	query->manifest_cmd_type = 0;
	query->manifest_owner_state = 0;
	query->manifest_ctrl_flags = 0;
	query->manifest_queue_base_offset = 0;
	query->manifest_queue_size = 0;
	query->manifest_queue_packet_size = 0;
	query->manifest_queue_packet_count = 0;
	query->manifest_queue_epoch = 0;
	query->manifest_ccb_offset = 0;
	query->manifest_ccb_write_offset = 0;
	query->manifest_ccb_read_offset = 0;
	query->manifest_dependency_active_count = 0;
	query->manifest_global_issue_ticket = 0;
	query->manifest_global_running_ticket = 0;
	query->manifest_global_complete_ticket = 0;
	query->manifest_pds_gpu_va32 = 0;
	query->manifest_use_gpu_va32 = 0;
	query->manifest_target_gpu_va32 = 0;
	query->manifest_status_gpu_va32 = 0;
	query->manifest_sync_complete_seq = 0;
	query->manifest_sync_pending_seq = 0;
	query->manifest_sync_flags = 0;
	query->manifest_owner_issue_cookie = 0;
	query->manifest_owner_complete_cookie = 0;
	query->manifest_config_count = submit->manifest_config_count;
	query->descriptor_offset = submit->descriptor_offset;
	query->descriptor_stage = 0;
	query->descriptor_stage_count = 0;
	query->descriptor_context = 0;
	query->descriptor_cmd_type = 0;
	query->descriptor_owner_state = 0;
	query->descriptor_issued_packets = 0;
	query->descriptor_completed_packets = 0;
	query->descriptor_dependency_active_count = 0;
	query->descriptor_queue_packet_count = 0;
	query->descriptor_queue_epoch = 0;
	query->descriptor_ctrl_flags = 0;
	query->descriptor_queue_base_offset = 0;
	query->descriptor_queue_size = 0;
	query->descriptor_queue_packet_size = 0;
	query->descriptor_ccb_offset = 0;
	query->descriptor_sync_offset = 0;
	query->descriptor_owner_offset = 0;
	query->descriptor_report_offset = 0;
	query->descriptor_manifest_offset = 0;
	query->descriptor_global_issue_ticket = 0;
	query->descriptor_global_running_ticket = 0;
	query->descriptor_global_complete_ticket = 0;
	query->descriptor_report_flags = 0;
	query->descriptor_status_ta_offset = 0;
	query->descriptor_status_3d_offset = 0;
	query->descriptor_status_stride = 0;
	query->descriptor_status_value_seed = 0;
	query->descriptor_control_gpu_va32 = 0;
	query->descriptor_pds_gpu_va32 = 0;
	query->descriptor_use_gpu_va32 = 0;
	query->descriptor_target_gpu_va32 = 0;
	query->descriptor_status_gpu_va32 = 0;
	query->descriptor_config_count = submit->descriptor_config_count;
	query->plan_offset = submit->plan_offset;
	query->plan_stage = 0;
	query->plan_stage_count = 0;
	query->plan_source = DRM_PSB_SGX_LAB_PLAN_SOURCE_NONE;
	query->plan_context = 0;
	query->plan_cmd_type = 0;
	query->plan_owner_state = 0;
	query->plan_issued_packets = 0;
	query->plan_completed_packets = 0;
	query->plan_dependency_active_count = 0;
	query->plan_refilled_from_complete = 0;
	query->plan_queue_packet_count = 0;
	query->plan_queue_epoch = 0;
	query->plan_queue_base_offset = 0;
	query->plan_queue_size = 0;
	query->plan_queue_packet_size = 0;
	query->plan_ccb_offset = 0;
	query->plan_ccb_write_offset = 0;
	query->plan_ccb_read_offset = 0;
	query->plan_sync_offset = 0;
	query->plan_sync_complete_seq = 0;
	query->plan_sync_pending_seq = 0;
	query->plan_sync_flags = 0;
	query->plan_owner_offset = 0;
	query->plan_owner_issue_cookie = 0;
	query->plan_owner_complete_cookie = 0;
	query->plan_global_issue_ticket = 0;
	query->plan_global_running_ticket = 0;
	query->plan_global_complete_ticket = 0;
	query->plan_report_offset = 0;
	query->plan_manifest_offset = 0;
	query->plan_descriptor_offset = 0;
	query->plan_ctrl_flags = 0;
	query->plan_ccb_service_address = 0;
	query->plan_ccb_cache_control = 0;
	query->plan_ccb_data0 = 0;
	query->plan_ccb_data1 = 0;
	query->plan_control_gpu_va32 = 0;
	query->plan_pds_gpu_va32 = 0;
	query->plan_use_gpu_va32 = 0;
	query->plan_target_gpu_va32 = 0;
	query->plan_status_gpu_va32 = 0;
	query->plan_config_count = submit->plan_config_count;
	query->issue_plan_offset = submit->issue_plan_offset;
	query->issue_plan_stage = 0;
	query->issue_plan_stage_count = 0;
	query->issue_plan_valid = 0;
	query->issue_plan_owner_state = 0;
	query->issue_plan_issued_packets = 0;
	query->issue_plan_refilled_from_complete = 0;
	query->issue_plan_queue_epoch = 0;
	query->issue_plan_queue_base_offset = 0;
	query->issue_plan_queue_size = 0;
	query->issue_plan_queue_packet_size = 0;
	query->issue_plan_queue_packet_count = 0;
	query->issue_plan_queue_tail_packet_count = 0;
	query->issue_plan_queue_head_packet_count = 0;
	query->issue_plan_ccb_offset = 0;
	query->issue_plan_ccb_write_offset = 0;
	query->issue_plan_ccb_read_offset = 0;
	query->issue_plan_sync_offset = 0;
	query->issue_plan_sync_complete_seq = 0;
	query->issue_plan_sync_pending_seq = 0;
	query->issue_plan_sync_flags = 0;
	query->issue_plan_owner_offset = 0;
	query->issue_plan_owner_issue_cookie = 0;
	query->issue_plan_owner_complete_cookie = 0;
	query->issue_plan_global_issue_ticket = 0;
	query->issue_plan_global_running_ticket = 0;
	query->issue_plan_global_complete_ticket = 0;
	query->issue_plan_dependency_active_count = 0;
	query->issue_plan_ccb_service_address = 0;
	query->issue_plan_ccb_cache_control = 0;
	query->issue_plan_ccb_data0 = 0;
	query->issue_plan_ccb_data1 = 0;
	query->issue_plan_control_object_id = 0;
	query->issue_plan_status_object_id = 0;
	query->issue_plan_control_gpu_va32 = 0;
	query->issue_plan_pds_gpu_va32 = 0;
	query->issue_plan_use_gpu_va32 = 0;
	query->issue_plan_target_gpu_va32 = 0;
	query->issue_plan_status_gpu_va32 = 0;
	query->issue_plan_config_count = submit->issue_plan_config_count;
	query->issue_count = submit->issue_count;
	query->transition_count = submit->transition_count;
	query->dependency_slot_count = DRM_PSB_SGX_LAB_DEPENDENCY_SLOTS;
	query->last_prepare_jiffies = submit->last_prepare_jiffies;
	query->last_arm_jiffies = submit->last_arm_jiffies;
	query->last_emit_jiffies = submit->last_emit_jiffies;
	query->last_launch_jiffies = submit->last_launch_jiffies;
	query->last_dispatch_jiffies = submit->last_dispatch_jiffies;
	query->last_retired_jiffies = submit->last_retired_jiffies;
	query->last_live_jiffies = submit->last_live_jiffies;
	query->last_report_jiffies = submit->last_report_jiffies;

	for (slot = 0; slot < DRM_PSB_SGX_LAB_DEPENDENCY_SLOTS; slot++) {
		u32 *source_submit_id =
			poulsbo_sgx_dependency_snapshot_source_submit_id_ptr(
				submit, slot);
		u32 *required_ticket =
			poulsbo_sgx_dependency_snapshot_required_ticket_ptr(
				submit, slot);
		u32 *observed_complete_ticket =
			poulsbo_sgx_dependency_snapshot_observed_complete_ticket_ptr(
				submit, slot);
		u32 *dep_flags =
			poulsbo_sgx_dependency_snapshot_flags_ptr(submit, slot);

		if (!source_submit_id || !required_ticket ||
		    !observed_complete_ticket || !dep_flags)
			continue;

		query->dependencies[slot].submit_id = *source_submit_id;
		query->dependencies[slot].required_ticket = *required_ticket;
		query->dependencies[slot].observed_complete_ticket =
			*observed_complete_ticket;
		query->dependencies[slot].flags = *dep_flags;
		if (poulsbo_sgx_dependency_slot_active(submit, slot))
			active_count++;
	}
	query->dependency_active_count = active_count;

	control_obj = poulsbo_sgx_find_object_locked(dev_priv,
						      submit->control_object_id);
	pds_obj = poulsbo_sgx_find_object_locked(dev_priv,
					 submit->pds_object_id);
	use_obj = poulsbo_sgx_find_object_locked(dev_priv,
					 submit->use_object_id);
	target_obj = poulsbo_sgx_find_object_locked(dev_priv,
					    submit->target_object_id);
	status_obj = poulsbo_sgx_find_object_locked(dev_priv,
						    submit->status_object_id);
	if (resolved_object_id)
		resolved_obj = poulsbo_sgx_find_object_locked(dev_priv,
						       resolved_object_id);
	if (resolved_obj) {
		query->resolved_object_role = resolved_obj->role;
		query->resolved_object_class = resolved_obj->object_class;
		query->resolved_object_gpu_use = resolved_obj->gpu_use;
		query->resolved_object_expected_context =
			resolved_obj->expected_context;
	}

	if (control_obj)
		query->control_gpu_va = poulsbo_sgx_object_gpu_va(dev_priv,
								  control_obj);
	if (pds_obj)
		query->pds_gpu_va = poulsbo_sgx_object_gpu_va(dev_priv,
							      pds_obj);
	if (use_obj)
		query->use_gpu_va = poulsbo_sgx_object_gpu_va(dev_priv,
							      use_obj);
	if (target_obj)
		query->target_gpu_va = poulsbo_sgx_object_gpu_va(dev_priv,
								 target_obj);
	if (status_obj)
	query->status_gpu_va = poulsbo_sgx_object_gpu_va(dev_priv,
							 status_obj);
	if (control_obj &&
	    !poulsbo_sgx_object_access_ok(control_obj, submit->ccb_offset,
					  PSB_SGX_LAB_CCB_RECORD_BYTES) &&
	    !poulsbo_sgx_read_submit_ccb_record_locked(control_obj,
						       submit->ccb_offset,
						       &ccb_record)) {
		/* Actual record readback succeeded; live config fields stay above. */
		query->ccb_pds_gpu_va32 = ccb_record.pds_gpu_va32;
		query->ccb_use_gpu_va32 = ccb_record.use_gpu_va32;
			query->ccb_target_gpu_va32 = ccb_record.target_gpu_va32;
			query->ccb_status_gpu_va32 = ccb_record.status_gpu_va32;
	}
	if (control_obj && submit->live_config_count &&
	    !poulsbo_sgx_live_region_end(submit->live_offset,
					 &live_query_end) &&
	    !poulsbo_sgx_object_access_ok(control_obj, submit->live_offset,
					  live_query_end -
						  submit->live_offset) &&
	    !poulsbo_sgx_read_submit_live_locked(control_obj,
						submit->live_offset,
						&live)) {
		query->live_stage = live.stage;
		query->live_stage_count = live.stage_count;
		query->live_prepare_count = live.prepare_count;
		query->live_arm_count = live.arm_count;
		query->live_emit_count = live.emit_count;
		query->live_launch_count = live.launch_count;
		query->live_dispatch_count = live.dispatch_count;
		query->live_retired_count = live.retired_count;
		query->live_context = live.context;
		query->live_cmd_type = live.cmd_type;
		query->live_owner_state = live.owner_state;
		query->live_issued_packets = live.issued_packets;
		query->live_completed_packets = live.completed_packets;
		query->live_dependency_active_count =
			live.dependency_active_count;
		query->live_refilled_from_complete =
			live.refilled_from_complete;
		query->live_queue_packet_count = live.queue_packet_count;
		query->live_queue_epoch = live.queue_epoch;
		query->live_queue_base_offset = live.queue_base_offset;
		query->live_queue_size = live.queue_size;
		query->live_queue_packet_size = live.queue_packet_size;
		query->live_ccb_offset = live.ccb_offset;
		query->live_ccb_write_offset = live.ccb_write_offset;
		query->live_ccb_read_offset = live.ccb_read_offset;
		query->live_sync_offset = live.sync_offset;
		query->live_sync_complete_seq = live.sync_complete_seq;
		query->live_sync_pending_seq = live.sync_pending_seq;
		query->live_sync_flags = live.sync_flags;
		query->live_owner_offset = live.owner_offset;
		query->live_owner_issue_cookie = live.owner_issue_cookie;
		query->live_owner_complete_cookie = live.owner_complete_cookie;
		query->live_global_issue_ticket = live.global_issue_ticket;
		query->live_global_running_ticket = live.global_running_ticket;
		query->live_global_complete_ticket =
			live.global_complete_ticket;
		query->live_ctrl_flags = live.ctrl_flags;
		query->live_report_flags = live.report_flags;
		query->live_num_ta_status_vals = live.num_ta_status_vals;
		query->live_num_3d_status_vals = live.num_3d_status_vals;
		query->live_ta_dependency = live.ta_dependency;
		query->live_status_ta_offset = live.status_ta_offset;
		query->live_status_3d_offset = live.status_3d_offset;
		query->live_status_stride = live.status_stride;
		query->live_status_value_seed = live.status_value_seed;
		query->live_control_object_id = live.control_object_id;
		query->live_status_object_id = live.status_object_id;
		query->live_ccb_service_address = live.ccb_service_address;
		query->live_ccb_cache_control = live.ccb_cache_control;
		query->live_ccb_data0 = live.ccb_data0;
		query->live_ccb_data1 = live.ccb_data1;
		query->live_control_gpu_va32 = live.control_gpu_va32;
		query->live_status_gpu_va32 = live.status_gpu_va32;
		query->live_pds_gpu_va32 = live.pds_gpu_va32;
		query->live_use_gpu_va32 = live.use_gpu_va32;
		query->live_target_gpu_va32 = live.target_gpu_va32;
		query->live_ccb_status_gpu_va32 = live.ccb_status_gpu_va32;
	}
	if (status_obj && submit->result_config_count &&
	    !poulsbo_sgx_result_region_end(submit->result_offset,
					   &result_query_end) &&
	    !poulsbo_sgx_object_access_ok(status_obj, submit->result_offset,
					  result_query_end -
						  submit->result_offset) &&
	    !poulsbo_sgx_read_submit_result_locked(status_obj,
						  submit->result_offset,
						  &result)) {
		query->result_stage = result.stage;
		query->result_stage_count = result.stage_count;
		query->result_source = result.source;
		query->result_context = result.context;
		query->result_cmd_type = result.cmd_type;
		query->result_owner_state = result.owner_state;
		query->result_issued_packets = result.issued_packets;
		query->result_completed_packets = result.completed_packets;
		query->result_dependency_active_count =
			result.dependency_active_count;
		query->result_queue_packet_count = result.queue_packet_count;
		query->result_queue_epoch = result.queue_epoch;
		query->result_sync_complete_seq = result.sync_complete_seq;
		query->result_sync_pending_seq = result.sync_pending_seq;
		query->result_sync_flags = result.sync_flags;
		query->result_owner_issue_cookie = result.owner_issue_cookie;
		query->result_owner_complete_cookie = result.owner_complete_cookie;
		query->result_global_issue_ticket = result.global_issue_ticket;
		query->result_global_running_ticket = result.global_running_ticket;
		query->result_global_complete_ticket =
			result.global_complete_ticket;
		query->result_ctrl_flags = result.ctrl_flags;
		query->result_num_ta_status_vals = result.num_ta_status_vals;
		query->result_num_3d_status_vals = result.num_3d_status_vals;
		query->result_ta_dependency = result.ta_dependency;
		query->result_status_ta_offset = result.status_ta_offset;
		query->result_status_3d_offset = result.status_3d_offset;
		query->result_status_stride = result.status_stride;
		query->result_status_value_seed = result.status_value_seed;
		query->result_control_object_id = result.control_object_id;
		query->result_status_object_id = result.status_object_id;
		query->result_control_gpu_va32 = result.control_gpu_va32;
		query->result_status_gpu_va32 = result.status_gpu_va32;
		query->result_report_offset = result.report_offset;
		query->result_manifest_offset = result.manifest_offset;
		query->result_descriptor_offset = result.descriptor_offset;
		query->result_plan_offset = result.plan_offset;
	}
	query->last_result_jiffies = submit->last_result_jiffies;
	if (control_obj && submit->manifest_config_count &&
	    !poulsbo_sgx_manifest_region_end(submit->manifest_offset,
						     &manifest_query_end) &&
	    !poulsbo_sgx_object_access_ok(control_obj, submit->manifest_offset,
					  manifest_query_end -
					  submit->manifest_offset) &&
	    !poulsbo_sgx_read_submit_manifest_locked(control_obj,
						    submit->manifest_offset,
						    &manifest)) {
		query->manifest_stage = manifest.stage;
		query->manifest_stage_count = manifest.stage_count;
		query->manifest_context = manifest.context;
		query->manifest_cmd_type = manifest.cmd_type;
		query->manifest_owner_state = manifest.owner_state;
		query->manifest_ctrl_flags = manifest.ctrl_flags;
		query->manifest_queue_base_offset = manifest.queue_base_offset;
		query->manifest_queue_size = manifest.queue_size;
		query->manifest_queue_packet_size = manifest.queue_packet_size;
		query->manifest_queue_packet_count = manifest.queue_packet_count;
		query->manifest_queue_epoch = manifest.queue_epoch;
		query->manifest_ccb_offset = manifest.ccb_offset;
		query->manifest_ccb_write_offset = manifest.ccb_write_offset;
		query->manifest_ccb_read_offset = manifest.ccb_read_offset;
		query->manifest_dependency_active_count =
			manifest.dependency_active_count;
		query->manifest_global_issue_ticket =
			manifest.global_issue_ticket;
		query->manifest_global_running_ticket =
			manifest.global_running_ticket;
		query->manifest_global_complete_ticket =
			manifest.global_complete_ticket;
		query->manifest_pds_gpu_va32 = manifest.pds_gpu_va32;
		query->manifest_use_gpu_va32 = manifest.use_gpu_va32;
		query->manifest_target_gpu_va32 = manifest.target_gpu_va32;
		query->manifest_status_gpu_va32 = manifest.status_gpu_va32;
		query->manifest_sync_complete_seq =
			manifest.sync_complete_seq;
		query->manifest_sync_pending_seq =
			manifest.sync_pending_seq;
		query->manifest_sync_flags = manifest.sync_flags;
		query->manifest_owner_issue_cookie =
			manifest.owner_issue_cookie;
		query->manifest_owner_complete_cookie =
			manifest.owner_complete_cookie;
	}
	query->last_manifest_jiffies = submit->last_manifest_jiffies;
	if (control_obj && submit->descriptor_config_count &&
	    !poulsbo_sgx_descriptor_region_end(submit->descriptor_offset,
					       &descriptor_query_end) &&
	    !poulsbo_sgx_object_access_ok(control_obj,
					  submit->descriptor_offset,
					  descriptor_query_end -
						  submit->descriptor_offset) &&
	    !poulsbo_sgx_read_submit_descriptor_locked(
		    control_obj, submit->descriptor_offset, &descriptor)) {
		query->descriptor_stage = descriptor.stage;
		query->descriptor_stage_count = descriptor.stage_count;
		query->descriptor_context = descriptor.context;
		query->descriptor_cmd_type = descriptor.cmd_type;
		query->descriptor_owner_state = descriptor.owner_state;
		query->descriptor_issued_packets = descriptor.issued_packets;
		query->descriptor_completed_packets =
			descriptor.completed_packets;
		query->descriptor_dependency_active_count =
			descriptor.dependency_active_count;
		query->descriptor_queue_packet_count =
			descriptor.queue_packet_count;
		query->descriptor_queue_epoch = descriptor.queue_epoch;
		query->descriptor_ctrl_flags = descriptor.ctrl_flags;
		query->descriptor_queue_base_offset =
			descriptor.queue_base_offset;
		query->descriptor_queue_size = descriptor.queue_size;
		query->descriptor_queue_packet_size =
			descriptor.queue_packet_size;
		query->descriptor_ccb_offset = descriptor.ccb_offset;
		query->descriptor_sync_offset = descriptor.sync_offset;
		query->descriptor_owner_offset = descriptor.owner_offset;
		query->descriptor_report_offset = descriptor.report_offset;
		query->descriptor_manifest_offset = descriptor.manifest_offset;
		query->descriptor_global_issue_ticket =
			descriptor.global_issue_ticket;
		query->descriptor_global_running_ticket =
			descriptor.global_running_ticket;
		query->descriptor_global_complete_ticket =
			descriptor.global_complete_ticket;
		query->descriptor_report_flags = descriptor.report_flags;
		query->descriptor_status_ta_offset =
			descriptor.status_ta_offset;
		query->descriptor_status_3d_offset =
			descriptor.status_3d_offset;
		query->descriptor_status_stride = descriptor.status_stride;
		query->descriptor_status_value_seed =
			descriptor.status_value_seed;
		query->descriptor_control_gpu_va32 =
			descriptor.control_gpu_va32;
		query->descriptor_pds_gpu_va32 = descriptor.pds_gpu_va32;
		query->descriptor_use_gpu_va32 = descriptor.use_gpu_va32;
		query->descriptor_target_gpu_va32 =
			descriptor.target_gpu_va32;
		query->descriptor_status_gpu_va32 =
			descriptor.status_gpu_va32;
	}
	query->last_descriptor_jiffies = submit->last_descriptor_jiffies;
	if (control_obj && submit->plan_config_count &&
	    !poulsbo_sgx_plan_region_end(submit->plan_offset,
					 &plan_query_end) &&
	    !poulsbo_sgx_object_access_ok(control_obj, submit->plan_offset,
					  plan_query_end -
					  submit->plan_offset) &&
	    !poulsbo_sgx_read_submit_plan_locked(control_obj,
						 submit->plan_offset,
						 &plan)) {
		query->plan_stage = plan.stage;
		query->plan_stage_count = plan.stage_count;
		query->plan_source = plan.source;
		query->plan_context = plan.context;
		query->plan_cmd_type = plan.cmd_type;
		query->plan_owner_state = plan.owner_state;
		query->plan_issued_packets = plan.issued_packets;
		query->plan_completed_packets = plan.completed_packets;
		query->plan_dependency_active_count =
			plan.dependency_active_count;
		query->plan_refilled_from_complete =
			plan.refilled_from_complete;
		query->plan_queue_packet_count = plan.queue_packet_count;
		query->plan_queue_epoch = plan.queue_epoch;
		query->plan_queue_base_offset = plan.queue_base_offset;
		query->plan_queue_size = plan.queue_size;
		query->plan_queue_packet_size = plan.queue_packet_size;
		query->plan_ccb_offset = plan.ccb_offset;
		query->plan_ccb_write_offset = plan.ccb_write_offset;
		query->plan_ccb_read_offset = plan.ccb_read_offset;
		query->plan_sync_offset = plan.sync_offset;
		query->plan_sync_complete_seq = plan.sync_complete_seq;
		query->plan_sync_pending_seq = plan.sync_pending_seq;
		query->plan_sync_flags = plan.sync_flags;
		query->plan_owner_offset = plan.owner_offset;
		query->plan_owner_issue_cookie = plan.owner_issue_cookie;
		query->plan_owner_complete_cookie =
			plan.owner_complete_cookie;
		query->plan_global_issue_ticket = plan.global_issue_ticket;
		query->plan_global_running_ticket =
			plan.global_running_ticket;
		query->plan_global_complete_ticket =
			plan.global_complete_ticket;
		query->plan_report_offset = plan.report_offset;
		query->plan_manifest_offset = plan.manifest_offset;
		query->plan_descriptor_offset = plan.descriptor_offset;
		query->plan_ctrl_flags = plan.ctrl_flags;
		query->plan_ccb_service_address =
			plan.ccb_service_address;
		query->plan_ccb_cache_control = plan.ccb_cache_control;
		query->plan_ccb_data0 = plan.ccb_data0;
		query->plan_ccb_data1 = plan.ccb_data1;
		query->plan_control_gpu_va32 = plan.control_gpu_va32;
		query->plan_pds_gpu_va32 = plan.pds_gpu_va32;
		query->plan_use_gpu_va32 = plan.use_gpu_va32;
		query->plan_target_gpu_va32 = plan.target_gpu_va32;
		query->plan_status_gpu_va32 = plan.status_gpu_va32;
	}
	query->last_plan_jiffies = submit->last_plan_jiffies;
	if (control_obj && submit->issue_plan_config_count &&
	    !poulsbo_sgx_issue_plan_region_end(submit->issue_plan_offset,
					       &issue_plan_query_end) &&
	    !poulsbo_sgx_object_access_ok(control_obj,
					  submit->issue_plan_offset,
					  issue_plan_query_end -
						  submit->issue_plan_offset) &&
	    !poulsbo_sgx_read_submit_issue_plan_locked(
		    control_obj, submit->issue_plan_offset, &issue_plan)) {
		query->issue_plan_stage = issue_plan.stage;
		query->issue_plan_stage_count = issue_plan.stage_count;
		query->issue_plan_valid = issue_plan.valid;
		query->issue_plan_owner_state = issue_plan.owner_state;
		query->issue_plan_issued_packets = issue_plan.issued_packets;
		query->issue_plan_refilled_from_complete =
			issue_plan.refilled_from_complete;
		query->issue_plan_queue_epoch = issue_plan.queue_epoch;
		query->issue_plan_queue_base_offset =
			issue_plan.queue_base_offset;
		query->issue_plan_queue_size = issue_plan.queue_size;
		query->issue_plan_queue_packet_size =
			issue_plan.queue_packet_size;
		query->issue_plan_queue_packet_count =
			issue_plan.queue_packet_count;
		query->issue_plan_queue_tail_packet_count =
			issue_plan.queue_tail_packet_count;
		query->issue_plan_queue_head_packet_count =
			issue_plan.queue_head_packet_count;
		query->issue_plan_ccb_offset = issue_plan.ccb_offset;
		query->issue_plan_ccb_write_offset =
			issue_plan.ccb_write_offset;
		query->issue_plan_ccb_read_offset =
			issue_plan.ccb_read_offset;
		query->issue_plan_sync_offset = issue_plan.sync_offset;
		query->issue_plan_sync_complete_seq =
			issue_plan.sync_complete_seq;
		query->issue_plan_sync_pending_seq =
			issue_plan.sync_pending_seq;
		query->issue_plan_sync_flags = issue_plan.sync_flags;
		query->issue_plan_owner_offset = issue_plan.owner_offset;
		query->issue_plan_owner_issue_cookie =
			issue_plan.owner_issue_cookie;
		query->issue_plan_owner_complete_cookie =
			issue_plan.owner_complete_cookie;
		query->issue_plan_global_issue_ticket =
			issue_plan.global_issue_ticket;
		query->issue_plan_global_running_ticket =
			issue_plan.global_running_ticket;
		query->issue_plan_global_complete_ticket =
			issue_plan.global_complete_ticket;
		query->issue_plan_dependency_active_count =
			issue_plan.dependency_active_count;
		query->issue_plan_ccb_service_address =
			issue_plan.ccb_service_address;
		query->issue_plan_ccb_cache_control =
			issue_plan.ccb_cache_control;
		query->issue_plan_ccb_data0 = issue_plan.ccb_data0;
		query->issue_plan_ccb_data1 = issue_plan.ccb_data1;
		query->issue_plan_control_object_id =
			issue_plan.control_object_id;
		query->issue_plan_status_object_id =
			issue_plan.status_object_id;
		query->issue_plan_control_gpu_va32 =
			issue_plan.control_gpu_va32;
		query->issue_plan_pds_gpu_va32 = issue_plan.pds_gpu_va32;
		query->issue_plan_use_gpu_va32 = issue_plan.use_gpu_va32;
		query->issue_plan_target_gpu_va32 =
			issue_plan.target_gpu_va32;
		query->issue_plan_status_gpu_va32 =
			issue_plan.status_gpu_va32;
	}
	query->last_issue_plan_jiffies = submit->last_issue_plan_jiffies;
}

static void poulsbo_sgx_fill_coord_query_locked(
	struct drm_psb_private *dev_priv,
	struct drm_psb_sgx_lab_coord_query *query)
{
	struct psb_sgx_lab_submit *submit;
	struct psb_sgx_lab_object *obj;
	u32 request_flags = query->flags;
	u32 submit_count = 0;
	u32 object_count = 0;

	memset(query, 0, sizeof(*query));
	query->size = sizeof(*query);
	query->flags = request_flags;
	query->focus_valid = dev_priv->sgx_lab_submit_focus_valid ? 1 : 0;
	query->focused_submit_id = dev_priv->sgx_lab_submit_focus_id;
	list_for_each_entry(submit, &dev_priv->sgx_lab_submit_list, head)
		submit_count++;
	list_for_each_entry(obj, &dev_priv->sgx_lab_object_list, head)
		object_count++;
	query->live_submit_count = submit_count;
	query->live_object_count = object_count;
	query->issue_ticket = dev_priv->sgx_lab_global_issue_ticket;
	query->running_ticket = dev_priv->sgx_lab_global_running_ticket;
	query->complete_ticket = dev_priv->sgx_lab_global_complete_ticket;
	query->issue_submit_id = dev_priv->sgx_lab_global_issue_submit_id;
	query->running_submit_id = dev_priv->sgx_lab_global_running_submit_id;
	query->complete_submit_id = dev_priv->sgx_lab_global_complete_submit_id;
	query->active_running_valid = dev_priv->sgx_lab_running_active ? 1 : 0;
	query->active_running_submit_id = dev_priv->sgx_lab_active_running_submit_id;
}

static void poulsbo_sgx_fill_range_query_locked(
	struct drm_psb_private *dev_priv,
	struct gtt_range *gt, u32 resolved_index,
	u32 total_range_count, u32 matching_range_count,
	struct drm_psb_sgx_lab_range_query *query)
{
	struct psb_sgx_lab_object *obj = NULL;
	u32 request_flags = query->flags;
	u64 request_query_gpu_va = query->query_gpu_va;
	u32 request_query_submit_match_index = query->query_submit_match_index;
	u64 request_query_gpu_va_offset = 0;
	u32 submit_ref_count = 0;
	u32 first_submit_id = 0;
	u32 first_submit_slot = 0;
	u32 resolved_submit_found = 0;
	u32 resolved_submit_id = 0;
	u32 resolved_submit_slot = 0;
	u32 resolved_submit_match_index = 0;

	if (gt->sgx_lab_object_id)
		obj = poulsbo_sgx_find_object_locked(dev_priv,
						     gt->sgx_lab_object_id);
	poulsbo_sgx_range_submit_refs_locked(dev_priv, gt, &submit_ref_count,
					     &first_submit_id,
					     &first_submit_slot,
					     request_flags &
					     DRM_PSB_SGX_LAB_QUERY_RANGE_USE_SUBMIT_MATCH_INDEX,
					     request_query_submit_match_index,
					     &resolved_submit_found,
					     &resolved_submit_id,
					     &resolved_submit_slot,
					     &resolved_submit_match_index);

	memset(query, 0, sizeof(*query));
	query->size = sizeof(*query);
	query->flags = request_flags;
	query->query_gpu_va = request_query_gpu_va;
	query->query_submit_match_index = request_query_submit_match_index;
	query->resolved_range_id = gt->debug_id;
	query->resolved_index = resolved_index;
	query->range_type = poulsbo_sgx_lab_range_type(gt);
	query->total_range_count = total_range_count;
	query->matching_range_count = matching_range_count;
	query->offset = gt->offset;
	query->in_gart = gt->in_gart;
	query->stolen = gt->stolen ? 1 : 0;
	query->mmapping = gt->mmapping ? 1 : 0;
	query->npage = gt->npage;
	query->roll = gt->roll;
	query->sgx_object_id = gt->sgx_lab_object_id;
	if (obj) {
		query->sgx_object_role = obj->role;
		query->sgx_object_class = obj->object_class;
		query->sgx_object_gpu_use = obj->gpu_use;
		query->sgx_object_expected_context = obj->expected_context;
	}
	query->sgx_submit_ref_count = submit_ref_count;
	query->sgx_first_submit_id = first_submit_id;
	query->sgx_first_submit_slot = first_submit_slot;
	query->sgx_resolved_submit_found = resolved_submit_found;
	query->sgx_resolved_submit_id = resolved_submit_id;
	query->sgx_resolved_submit_slot = resolved_submit_slot;
	query->sgx_resolved_submit_match_index = resolved_submit_match_index;
	query->pin_count_total = gt->pin_count_total;
	query->unpin_count_total = gt->unpin_count_total;
	query->pf_bind_count = gt->sgx_pf_bind_count;
	query->pf_bind_total = gt->sgx_pf_bind_total;
	query->pf_unbind_total = gt->sgx_pf_unbind_total;
	query->start = gt->resource.start;
	query->end = gt->resource.end;
	query->len = resource_size(&gt->resource);
	query->gpu_va = dev_priv->gtt.gatt_start + gt->offset;
	if (request_flags & DRM_PSB_SGX_LAB_QUERY_RANGE_USE_GPU_VA)
		poulsbo_sgx_range_contains_gpu_va(dev_priv, gt,
						  request_query_gpu_va,
						  &request_query_gpu_va_offset);
	query->query_gpu_va_offset = request_query_gpu_va_offset;
}

static void poulsbo_sgx_fill_state_query_locked(
	struct drm_psb_private *dev_priv,
	struct drm_psb_sgx_lab_sgx_state_query *query)
{
	struct psb_sgx_snapshot snapshot;
	unsigned long flags;
	u32 request_flags = query->flags;

	spin_lock_irqsave(&dev_priv->sgx_state_lock, flags);
	snapshot = dev_priv->sgx_state;
	spin_unlock_irqrestore(&dev_priv->sgx_state_lock, flags);

	memset(query, 0, sizeof(*query));
	query->size = sizeof(*query);
	query->flags = request_flags;
	query->sequence = snapshot.sequence;
	query->capture_failed = snapshot.capture_failed;
	query->captured_jiffies = snapshot.captured_jiffies;
	memcpy(query->reason, snapshot.reason, sizeof(query->reason));
	query->irq_count = snapshot.irq_count;
	query->fault_count = snapshot.fault_count;
	query->vdc_irq_mask = snapshot.vdc_irq_mask;
	query->gatt_start = snapshot.gatt_start;
	query->mmu_gatt_start = snapshot.mmu_gatt_start;
	query->gatt_free_offset = snapshot.gatt_free_offset;
	query->mmu_driver_bif_ctrl = snapshot.mmu_driver_bif_ctrl;
	query->mmu_needs_tlbflush = snapshot.mmu_needs_tlbflush;
	query->mmu_default_pd_context = snapshot.mmu_default_pd_context;
	query->core_id = snapshot.core_id;
	query->core_revision = snapshot.core_revision;
	query->clkgatectl = snapshot.clkgatectl;
	query->sgx_soft_reset = snapshot.sgx_soft_reset;
	query->event_status = snapshot.event_status;
	query->event_status2 = snapshot.event_status2;
	query->event_host_enable = snapshot.event_host_enable;
	query->event_host_enable2 = snapshot.event_host_enable2;
	query->event_kicker = snapshot.event_kicker;
	query->event_kick = snapshot.event_kick;
	query->pds_exec_base = snapshot.pds_exec_base;
	query->use_code_base0 = snapshot.use_code_base0;
	query->use_code_base1 = snapshot.use_code_base1;
	query->bif_ctrl = snapshot.bif_ctrl;
	query->bif_int_stat = snapshot.bif_int_stat;
	query->bif_fault = snapshot.bif_fault;
	query->bif_dir_list_base0 = snapshot.bif_dir_list_base0;
	query->bif_dir_list_base1 = snapshot.bif_dir_list_base1;
	query->bif_bank0 = snapshot.bif_bank0;
	query->bif_bank1 = snapshot.bif_bank1;
	query->bif_twod_req_base = snapshot.bif_twod_req_base;
	query->bif_3d_req_base = snapshot.bif_3d_req_base;
	query->twod_socif = snapshot.twod_socif;
	query->twod_blit_status = snapshot.twod_blit_status;
}

static void poulsbo_sgx_fill_gtt_state_query_locked(
	struct drm_psb_private *dev_priv,
	struct drm_psb_sgx_lab_gtt_state_query *query)
{
	struct psb_mmu_driver *mmu;
	struct psb_mmu_debug_state mmu_debug = { 0 };
	struct resource *res;
	int default_pd_context = -1;
	int pf_pd_context = -1;
	u32 request_flags = query->flags;

	memset(query, 0, sizeof(*query));
	query->size = sizeof(*query);
	query->flags = request_flags;
	query->gtt_start = dev_priv->gtt.gtt_start;
	query->gatt_start = dev_priv->gtt.gatt_start;
	query->mmu_gatt_start = dev_priv->gtt.mmu_gatt_start;
	query->gatt_free_offset = dev_priv->gatt_free_offset;
	query->stolen_size = dev_priv->gtt.stolen_size;
	query->vram_stolen_size = dev_priv->vram_stolen_size;

	mmu = dev_priv->mmu;
	if (mmu) {
		down_read(&mmu->sem);
		mmu_debug = mmu->debug;
		if (mmu->default_pd)
			default_pd_context = mmu->default_pd->hw_context;
		if (dev_priv->pf_pd)
			pf_pd_context = dev_priv->pf_pd->hw_context;
		up_read(&mmu->sem);
	}

	query->mmu_last_op = mmu_debug.last_op;
	query->mmu_last_num_pages = mmu_debug.last_num_pages;
	query->mmu_last_type = mmu_debug.last_type;
	query->mmu_last_hw_context = mmu_debug.last_hw_context;
	query->mmu_last_address = mmu_debug.last_address;
	query->mmu_last_jiffies = mmu_debug.last_jiffies;
	query->mmu_flush_count = mmu_debug.flush_count;
	query->mmu_set_pd_context_count = mmu_debug.set_pd_context_count;
	query->mmu_insert_pfn_sequence_count =
		mmu_debug.insert_pfn_sequence_count;
	query->mmu_insert_pages_count = mmu_debug.insert_pages_count;
	query->mmu_remove_pfn_sequence_count =
		mmu_debug.remove_pfn_sequence_count;
	query->mmu_remove_pages_count = mmu_debug.remove_pages_count;
	query->mmu_default_pd_context = default_pd_context;
	query->mmu_pf_pd_context = pf_pd_context;

	res = dev_priv->gtt_mem ? dev_priv->gtt_mem->child : NULL;
	while (res) {
		struct gtt_range *gt = container_of(res, struct gtt_range,
						    resource);

		query->range_total_count++;
		switch (poulsbo_sgx_lab_range_type(gt)) {
		case DRM_PSB_SGX_LAB_RANGE_GEM:
			query->range_gem_count++;
			break;
		case DRM_PSB_SGX_LAB_RANGE_FB:
			query->range_fb_count++;
			break;
		case DRM_PSB_SGX_LAB_RANGE_CURSOR:
			query->range_cursor_count++;
			break;
		case DRM_PSB_SGX_LAB_RANGE_OTHER:
		case DRM_PSB_SGX_LAB_RANGE_UNKNOWN:
		default:
			query->range_other_count++;
			break;
		}
		if (gt->in_gart)
			query->range_in_gart_count++;
		if (gt->mmapping)
			query->range_mmapping_count++;
		if (gt->sgx_lab_object_id)
			query->range_sgx_object_count++;
		if (gt->sgx_pf_bind_count)
			query->range_pf_bound_count++;
		res = res->sibling;
	}
}

static void poulsbo_sgx_fill_object_query_locked(
	struct drm_psb_private *dev_priv,
	struct psb_sgx_lab_object *obj,
	u32 resolved_index,
	u32 total_object_count,
	struct drm_psb_sgx_lab_object_query *query)
{
	struct gtt_range *gt = obj->gt;
	u32 request_flags = query->flags;

	memset(query, 0, sizeof(*query));
	query->size = sizeof(*query);
	query->flags = request_flags;
	query->resolved_object_id = obj->object_id;
	query->resolved_index = resolved_index;
	query->total_object_count = total_object_count;
	query->range_id = gt->debug_id;
	query->range_type = poulsbo_sgx_lab_range_type(gt);
	query->role = obj->role;
	query->object_class = obj->object_class;
	query->gpu_use = obj->gpu_use;
	query->expected_context = obj->expected_context;
	query->offset = gt->offset;
	query->in_gart = gt->in_gart;
	query->stolen = gt->stolen;
	query->mmapping = gt->mmapping;
	query->npage = gt->npage;
	query->pf_bind_count = gt->sgx_pf_bind_count;
	query->pf_bind_total = gt->sgx_pf_bind_total;
	query->pf_unbind_total = gt->sgx_pf_unbind_total;
	query->size_bytes = gt->gem.size;
	query->gpu_va = poulsbo_sgx_object_gpu_va(dev_priv, obj);
	query->created_jiffies = obj->created_jiffies;
	query->last_pf_bind_jiffies = obj->last_pf_bind_jiffies;
	query->last_pf_unbind_jiffies = obj->last_pf_unbind_jiffies;
	query->last_role_jiffies = obj->last_role_jiffies;
	query->last_class_jiffies = obj->last_class_jiffies;
	query->last_gpu_use_jiffies = obj->last_gpu_use_jiffies;
	query->last_context_jiffies = obj->last_context_jiffies;
}

static ssize_t poulsbo_sgx_submits_show(struct device *kdev,
					struct device_attribute *attr,
					char *buf)
{
	struct pci_dev *pdev = to_pci_dev(kdev);
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_psb_private *dev_priv;
	struct psb_sgx_lab_submit *submit;
	ssize_t pos = 0;
	unsigned int shown = 0;
	unsigned int total = 0;
	bool truncated = false;

	if (!dev || !dev->dev_private)
		return scnprintf(buf, PAGE_SIZE, "driver not ready\n");

	dev_priv = dev->dev_private;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);
	list_for_each_entry(submit, &dev_priv->sgx_lab_submit_list, head) {
		total++;
		if (PAGE_SIZE - pos < 2260) {
			truncated = true;
			break;
		}

		pos += poulsbo_sgx_submit_format_locked(dev_priv, submit,
							 buf + pos,
							 PAGE_SIZE - pos);
		shown++;
	}
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);

	if (truncated)
		pos += scnprintf(buf + pos, PAGE_SIZE - pos,
				 "submits_shown=%u submits_total_at_least=%u truncated=1 attr=%s\n",
				 shown, total, attr->attr.name);
	else
		pos += scnprintf(buf + pos, PAGE_SIZE - pos,
				 "submits_shown=%u submits_total=%u truncated=0 attr=%s\n",
				 shown, total, attr->attr.name);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	return pos;
}

static DEVICE_ATTR_RO(poulsbo_sgx_submits);

static ssize_t poulsbo_sgx_submit_state_show(struct device *kdev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct pci_dev *pdev = to_pci_dev(kdev);
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_psb_private *dev_priv;
	struct psb_sgx_lab_submit *submit;
	ssize_t pos = 0;
	u32 focused_submit_id;
	unsigned long focus_jiffies;

	if (!dev || !dev->dev_private)
		return scnprintf(buf, PAGE_SIZE, "driver not ready\n");

	dev_priv = dev->dev_private;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);
	if (!dev_priv->sgx_lab_submit_focus_valid) {
		pos = scnprintf(buf, PAGE_SIZE,
				"focused_submit=none attr=%s\n",
				attr->attr.name);
		goto out_unlock;
	}

	focused_submit_id = dev_priv->sgx_lab_submit_focus_id;
	focus_jiffies = dev_priv->sgx_lab_submit_focus_jiffies;
	submit = poulsbo_sgx_find_submit_locked(dev_priv, focused_submit_id);
	if (!submit) {
		pos = scnprintf(buf, PAGE_SIZE,
				"focused_submit_id=%u state=missing last_focus_jiffies=%lu attr=%s\n",
				focused_submit_id, focus_jiffies,
				attr->attr.name);
		goto out_unlock;
	}

	pos += scnprintf(buf + pos, PAGE_SIZE - pos,
			 "focused_submit_id=%u state=present last_focus_jiffies=%lu attr=%s\n",
			 focused_submit_id, focus_jiffies, attr->attr.name);
	pos += poulsbo_sgx_submit_format_locked(dev_priv, submit, buf + pos,
						 PAGE_SIZE - pos);

out_unlock:
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	return pos;
}

static DEVICE_ATTR_RO(poulsbo_sgx_submit_state);

static ssize_t poulsbo_sgx_coord_state_show(struct device *kdev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct pci_dev *pdev = to_pci_dev(kdev);
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_psb_private *dev_priv;
	unsigned int submit_count = 0;
	struct psb_sgx_lab_submit *submit;
	ssize_t pos;

	if (!dev || !dev->dev_private)
		return scnprintf(buf, PAGE_SIZE, "driver not ready\n");

	dev_priv = dev->dev_private;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	list_for_each_entry(submit, &dev_priv->sgx_lab_submit_list, head)
		submit_count++;

	pos = scnprintf(buf, PAGE_SIZE,
			"issue_ticket_counter=%u running_ticket_counter=%u complete_ticket_counter=%u last_issue_submit_id=%u last_running_submit_id=%u last_complete_submit_id=%u active_running_valid=%u active_running_submit_id=%u active_running_ticket=%u last_issue_jiffies=%lu last_running_jiffies=%lu last_complete_jiffies=%lu focused_submit_valid=%u focused_submit_id=%u live_submit_count=%u attr=%s\n",
			dev_priv->sgx_lab_global_issue_ticket,
			dev_priv->sgx_lab_global_running_ticket,
			dev_priv->sgx_lab_global_complete_ticket,
			dev_priv->sgx_lab_global_issue_submit_id,
			dev_priv->sgx_lab_global_running_submit_id,
			dev_priv->sgx_lab_global_complete_submit_id,
			dev_priv->sgx_lab_running_active ? 1 : 0,
			dev_priv->sgx_lab_active_running_submit_id,
			dev_priv->sgx_lab_running_active ?
				dev_priv->sgx_lab_global_running_ticket : 0,
			dev_priv->sgx_lab_global_issue_jiffies,
			dev_priv->sgx_lab_global_running_jiffies,
			dev_priv->sgx_lab_global_complete_jiffies,
			dev_priv->sgx_lab_submit_focus_valid ? 1 : 0,
			dev_priv->sgx_lab_submit_focus_id,
			submit_count, attr->attr.name);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);

	return pos;
}

static DEVICE_ATTR_RO(poulsbo_sgx_coord_state);

static int psb_ioctl_sgx_lab_query_submit(struct drm_device *dev, void *data,
					  struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_psb_sgx_lab_submit_query *query = data;
	struct psb_sgx_lab_submit *submit = NULL;
	struct psb_sgx_lab_submit *iter;
	u32 resolved_index = 0;
	u32 total_submit_count = 0;
	u32 matching_submit_count = 0;
	u32 resolved_match_index = 0;
	u32 resolved_slot = 0;
	u32 resolved_object_id = 0;
	u32 resolved_range_id = 0;
	u32 resolved_range_type = 0;
	u64 query_gpu_va_offset = 0;
	u32 query_match_index = 0;
	u32 match_submit_id = 0;
	bool use_focus;
	bool use_index;
	bool use_gpu_va;
	bool use_gpu_va_match_index;
	unsigned int selector_count = 0;

	if (!dev_priv)
		return -ENODEV;
	if (!query || query->size < sizeof(*query))
		return -EINVAL;
	use_focus = query->flags & DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_FOCUS;
	use_index = query->flags & DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_INDEX;
	use_gpu_va = query->flags & DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_GPU_VA;
	use_gpu_va_match_index =
		query->flags & DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_GPU_VA_MATCH_INDEX;
	selector_count += use_focus ? 1 : 0;
	selector_count += use_index ? 1 : 0;
	selector_count += use_gpu_va ? 1 : 0;
	if (selector_count > 1)
		return -EINVAL;
	if (use_gpu_va_match_index && !use_gpu_va)
		return -EINVAL;
	if (use_gpu_va && !query->query_gpu_va)
		return -EINVAL;
	if (use_gpu_va_match_index)
		query_match_index = query->query_match_index;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);

	if (use_focus) {
		if (!dev_priv->sgx_lab_submit_focus_valid) {
			mutex_unlock(&dev_priv->gtt_mutex);
			mutex_unlock(&dev_priv->sgx_lab_object_mutex);
			mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
			return -ENOENT;
		}
		match_submit_id = dev_priv->sgx_lab_submit_focus_id;
	}

	list_for_each_entry(iter, &dev_priv->sgx_lab_submit_list, head) {
		bool gpu_va_match = false;
		u32 iter_resolved_slot = 0;
		u32 iter_resolved_object_id = 0;
		u32 iter_resolved_range_id = 0;
		u32 iter_resolved_range_type = 0;
		u64 iter_query_gpu_va_offset = 0;

		if (use_gpu_va)
			gpu_va_match =
				poulsbo_sgx_submit_resolve_gpu_va_locked(
					dev_priv, iter, query->query_gpu_va,
					&iter_resolved_slot,
					&iter_resolved_object_id,
					&iter_resolved_range_id,
					&iter_resolved_range_type,
					&iter_query_gpu_va_offset);

		if (!submit) {
			if (use_index) {
				if (total_submit_count == query->query_index) {
					submit = iter;
					resolved_index = total_submit_count;
				}
			} else if (use_focus) {
				if (iter->submit_id == match_submit_id) {
					submit = iter;
					resolved_index = total_submit_count;
				}
			} else if (use_gpu_va) {
				if (gpu_va_match) {
					if (matching_submit_count ==
					    query_match_index) {
						submit = iter;
						resolved_index =
							total_submit_count;
						resolved_match_index =
							matching_submit_count;
						resolved_slot =
							iter_resolved_slot;
						resolved_object_id =
							iter_resolved_object_id;
						resolved_range_id =
							iter_resolved_range_id;
						resolved_range_type =
							iter_resolved_range_type;
						query_gpu_va_offset =
							iter_query_gpu_va_offset;
					}
					matching_submit_count++;
				}
			} else if (iter->submit_id == query->submit_id) {
				submit = iter;
				resolved_index = total_submit_count;
			}
		} else if (use_gpu_va && gpu_va_match) {
			matching_submit_count++;
		}
		total_submit_count++;
	}

	if (!submit) {
		mutex_unlock(&dev_priv->gtt_mutex);
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
		return -ENOENT;
	}

	if (!use_gpu_va)
		matching_submit_count = 1;

	poulsbo_sgx_fill_submit_query_locked(dev_priv, submit, resolved_index,
					     total_submit_count,
					     matching_submit_count,
					     resolved_match_index,
					     resolved_slot,
					     resolved_object_id,
					     resolved_range_id,
					     resolved_range_type,
					     query_gpu_va_offset, query);

	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
	return 0;
}

static int psb_ioctl_sgx_lab_query_coord(struct drm_device *dev, void *data,
					 struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_psb_sgx_lab_coord_query *query = data;

	if (!dev_priv)
		return -ENODEV;
	if (!query || query->size < sizeof(*query))
		return -EINVAL;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	poulsbo_sgx_fill_coord_query_locked(dev_priv, query);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
	return 0;
}

static int psb_ioctl_sgx_lab_query_range(struct drm_device *dev, void *data,
					 struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_psb_sgx_lab_range_query *query = data;
	struct resource *res;
	struct gtt_range *found = NULL;
	u32 request_flags;
	u32 total_range_count = 0;
	u32 matching_range_count = 0;
	u32 resolved_index = 0;
	bool use_id;
	bool use_gpu_va;
	bool use_submit_match_index;

	if (!dev_priv)
		return -ENODEV;
	if (!query || query->size < sizeof(*query))
		return -EINVAL;

	request_flags = query->flags;
	if (request_flags & ~(DRM_PSB_SGX_LAB_QUERY_RANGE_USE_ID |
			      DRM_PSB_SGX_LAB_QUERY_RANGE_FILTER_GEM |
			      DRM_PSB_SGX_LAB_QUERY_RANGE_USE_GPU_VA |
			      DRM_PSB_SGX_LAB_QUERY_RANGE_USE_SUBMIT_MATCH_INDEX))
		return -EINVAL;
	use_id = request_flags & DRM_PSB_SGX_LAB_QUERY_RANGE_USE_ID;
	use_gpu_va = request_flags & DRM_PSB_SGX_LAB_QUERY_RANGE_USE_GPU_VA;
	use_submit_match_index =
		request_flags & DRM_PSB_SGX_LAB_QUERY_RANGE_USE_SUBMIT_MATCH_INDEX;
	if (use_id && use_gpu_va)
		return -EINVAL;
	if (use_id && !query->range_id)
		return -EINVAL;
	if (use_gpu_va && !query->query_gpu_va)
		return -EINVAL;

	mutex_lock(&dev_priv->sgx_lab_submit_mutex);
	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);
	res = dev_priv->gtt_mem ? dev_priv->gtt_mem->child : NULL;
	while (res) {
		struct gtt_range *gt = container_of(res, struct gtt_range,
						    resource);

		total_range_count++;
		if (poulsbo_sgx_range_matches_query_flags(gt, request_flags)) {
			if (!found) {
				if (use_id) {
					if (gt->debug_id == query->range_id) {
						found = gt;
						resolved_index =
							matching_range_count;
					}
				} else if (use_gpu_va) {
					if (poulsbo_sgx_range_contains_gpu_va(
						    dev_priv, gt,
						    query->query_gpu_va,
						    NULL)) {
						found = gt;
						resolved_index =
							matching_range_count;
					}
				} else if (matching_range_count ==
					   query->query_index) {
					found = gt;
					resolved_index = matching_range_count;
				}
			}
			matching_range_count++;
		}
		res = res->sibling;
	}

	if (!found) {
		mutex_unlock(&dev_priv->gtt_mutex);
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
		return -ENOENT;
	}

	poulsbo_sgx_fill_range_query_locked(dev_priv, found, resolved_index,
					    total_range_count,
					    matching_range_count, query);
	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
	return 0;
}

static int psb_ioctl_sgx_lab_query_sgx_state(struct drm_device *dev, void *data,
					     struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_psb_sgx_lab_sgx_state_query *query = data;
	u32 request_flags;

	if (!dev_priv)
		return -ENODEV;
	if (!query || query->size < sizeof(*query))
		return -EINVAL;

	request_flags = query->flags;
	if (request_flags & ~DRM_PSB_SGX_LAB_QUERY_SGX_STATE_CAPTURE)
		return -EINVAL;

	if (request_flags & DRM_PSB_SGX_LAB_QUERY_SGX_STATE_CAPTURE)
		psb_sgx_capture_state(dev, "ioctl-sgx-state", true);

	poulsbo_sgx_fill_state_query_locked(dev_priv, query);
	return 0;
}

static int psb_ioctl_sgx_lab_query_gtt_state(struct drm_device *dev, void *data,
					     struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_psb_sgx_lab_gtt_state_query *query = data;

	if (!dev_priv)
		return -ENODEV;
	if (!query || query->size < sizeof(*query))
		return -EINVAL;
	if (query->flags)
		return -EINVAL;

	mutex_lock(&dev_priv->gtt_mutex);
	poulsbo_sgx_fill_gtt_state_query_locked(dev_priv, query);
	mutex_unlock(&dev_priv->gtt_mutex);
	return 0;
}

static int psb_ioctl_sgx_lab_query_object(struct drm_device *dev, void *data,
					  struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_psb_sgx_lab_object_query *query = data;
	struct psb_sgx_lab_object *obj;
	struct psb_sgx_lab_object *found = NULL;
	u32 request_flags;
	u32 total_object_count = 0;
	u32 resolved_index = 0;

	if (!dev_priv)
		return -ENODEV;
	if (!query || query->size < sizeof(*query))
		return -EINVAL;

	request_flags = query->flags;
	if (request_flags & ~DRM_PSB_SGX_LAB_QUERY_OBJECT_USE_ID)
		return -EINVAL;
	if ((request_flags & DRM_PSB_SGX_LAB_QUERY_OBJECT_USE_ID) &&
	    !query->object_id)
		return -EINVAL;

	mutex_lock(&dev_priv->sgx_lab_object_mutex);
	mutex_lock(&dev_priv->gtt_mutex);
	list_for_each_entry(obj, &dev_priv->sgx_lab_object_list, head) {
		if ((request_flags & DRM_PSB_SGX_LAB_QUERY_OBJECT_USE_ID) &&
		    obj->object_id == query->object_id) {
			found = obj;
			resolved_index = total_object_count;
		} else if (!(request_flags &
			     DRM_PSB_SGX_LAB_QUERY_OBJECT_USE_ID) &&
			   total_object_count == query->query_index) {
			found = obj;
			resolved_index = total_object_count;
		}
		total_object_count++;
		if (found)
			break;
	}

	if (!found) {
		mutex_unlock(&dev_priv->gtt_mutex);
		mutex_unlock(&dev_priv->sgx_lab_object_mutex);
		return -ENOENT;
	}

	poulsbo_sgx_fill_object_query_locked(dev_priv, found, resolved_index,
					     total_object_count, query);
	query->total_object_count = total_object_count;

	list_for_each_entry_continue(obj, &dev_priv->sgx_lab_object_list, head)
		query->total_object_count++;

	mutex_unlock(&dev_priv->gtt_mutex);
	mutex_unlock(&dev_priv->sgx_lab_object_mutex);
	return 0;
}

static int psb_ioctl_sgx_lab_object_op(struct drm_device *dev, void *data,
				       struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_psb_sgx_lab_object_op *op = data;
	u32 object_id = 0;
	int ret;

	if (!dev_priv)
		return -ENODEV;
	if (!op || op->size < sizeof(*op))
		return -EINVAL;

	switch (op->op) {
	case DRM_PSB_SGX_LAB_OBJECT_OP_ADOPT:
		if (op->flags || op->object_id || op->gpu_use || op->context)
			return -EINVAL;
		ret = poulsbo_sgx_adopt_range(dev, op->range_id, &object_id);
		if (ret)
			return ret;
		op->resolved_object_id = object_id;
		return 0;
	case DRM_PSB_SGX_LAB_OBJECT_OP_GPU_USE:
		if (op->flags || !op->object_id || op->range_id || op->context)
			return -EINVAL;
		switch (op->gpu_use) {
		case DRM_PSB_SGX_LAB_GPU_USE_GENERIC:
		case DRM_PSB_SGX_LAB_GPU_USE_CONTROL_CCB:
		case DRM_PSB_SGX_LAB_GPU_USE_PDS_CODE:
		case DRM_PSB_SGX_LAB_GPU_USE_USE_CODE:
		case DRM_PSB_SGX_LAB_GPU_USE_RENDER_TARGET:
		case DRM_PSB_SGX_LAB_GPU_USE_STATUS_BLOCK:
			break;
		default:
			return -EINVAL;
		}
		ret = poulsbo_sgx_set_object_gpu_use(dev, op->object_id,
						     op->gpu_use);
		break;
	case DRM_PSB_SGX_LAB_OBJECT_OP_ROLE:
		if (op->flags || !op->object_id || op->range_id || op->gpu_use ||
		    op->context || op->object_class)
			return -EINVAL;
		switch (op->role) {
		case DRM_PSB_SGX_LAB_ROLE_GENERIC:
		case DRM_PSB_SGX_LAB_ROLE_PDS:
		case DRM_PSB_SGX_LAB_ROLE_USE:
		case DRM_PSB_SGX_LAB_ROLE_CONTROL:
		case DRM_PSB_SGX_LAB_ROLE_TARGET:
		case DRM_PSB_SGX_LAB_ROLE_STATUS:
			break;
		default:
			return -EINVAL;
		}
		ret = poulsbo_sgx_set_object_role(dev, op->object_id, op->role);
		break;
	case DRM_PSB_SGX_LAB_OBJECT_OP_CLASS:
		if (op->flags || !op->object_id || op->range_id || op->gpu_use ||
		    op->context || op->role)
			return -EINVAL;
		switch (op->object_class) {
		case DRM_PSB_SGX_LAB_CLASS_GENERIC:
		case DRM_PSB_SGX_LAB_CLASS_PACKET:
		case DRM_PSB_SGX_LAB_CLASS_CODE:
		case DRM_PSB_SGX_LAB_CLASS_STATE:
		case DRM_PSB_SGX_LAB_CLASS_SURFACE:
			break;
		default:
			return -EINVAL;
		}
		ret = poulsbo_sgx_set_object_class(dev, op->object_id,
						    op->object_class);
		break;
	case DRM_PSB_SGX_LAB_OBJECT_OP_CONTEXT:
		if (op->flags || !op->object_id || op->range_id || op->gpu_use)
			return -EINVAL;
		if (op->context != DRM_PSB_SGX_LAB_CONTEXT_AUTO &&
		    op->context != DRM_PSB_SGX_LAB_CONTEXT_DEFAULT &&
		    op->context != DRM_PSB_SGX_LAB_CONTEXT_PF)
			return -EINVAL;
		ret = poulsbo_sgx_set_object_context(dev, op->object_id,
						     op->context);
		break;
	case DRM_PSB_SGX_LAB_OBJECT_OP_PF_BIND:
		if (op->flags || !op->object_id || op->range_id || op->gpu_use ||
		    op->context)
			return -EINVAL;
		ret = poulsbo_sgx_bind_object(dev, op->object_id);
		break;
	case DRM_PSB_SGX_LAB_OBJECT_OP_PF_UNBIND:
		if (op->flags || !op->object_id || op->range_id || op->gpu_use ||
		    op->context)
			return -EINVAL;
		ret = poulsbo_sgx_unbind_object(dev, op->object_id);
		break;
	case DRM_PSB_SGX_LAB_OBJECT_OP_DROP:
		if (op->flags || !op->object_id || op->range_id || op->gpu_use ||
		    op->context)
			return -EINVAL;
		ret = poulsbo_sgx_drop_object(dev, op->object_id);
		break;
	case DRM_PSB_SGX_LAB_OBJECT_OP_CLEANUP:
		if (op->flags || op->object_id || op->range_id || op->role ||
		    op->object_class || op->gpu_use || op->context)
			return -EINVAL;
		op->result_count = poulsbo_sgx_drop_all_objects_count(dev);
		op->resolved_object_id = 0;
		return 0;
	default:
		return -EINVAL;
	}
	if (ret)
		return ret;

	op->resolved_object_id = op->object_id;
	return 0;
}

static int psb_ioctl_sgx_lab_submit_op(struct drm_device *dev, void *data,
				       struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_psb_sgx_lab_submit_op *op = data;
	struct psb_sgx_lab_submit_sgx_ta_submit_op_request_frontend *request_frontend =
		NULL;
	const char *slot_name = NULL;
	bool dependency_fields_present;
	u32 submit_id;
	int ret;

	if (!dev_priv)
		return -ENODEV;
	if (!op || op->size < sizeof(*op))
		return -EINVAL;
	if (op->report_offset &&
	    op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_REPORT)
		return -EINVAL;
	if (op->manifest_offset &&
	    op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_MANIFEST)
		return -EINVAL;
	if (op->descriptor_offset &&
	    op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_DESCRIPTOR)
		return -EINVAL;
	if (op->plan_offset &&
	    op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_PLAN)
		return -EINVAL;
	if (op->issue_plan_offset &&
	    op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE_PLAN)
		return -EINVAL;
	if (op->result_offset &&
	    op->op != DRM_PSB_SGX_LAB_SUBMIT_OP_RESULT)
		return -EINVAL;
	dependency_fields_present = op->dependency_slot ||
		op->dependency_submit_id || op->dependency_ticket ||
		op->dependency_offset;

	switch (op->op) {
	case DRM_PSB_SGX_LAB_SUBMIT_OP_CREATE:
		if (op->flags || op->submit_id || op->owner_state ||
		    op->packet_count || op->slot || op->object_id ||
		    op->cmd_type || op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->plan_offset ||
		    dependency_fields_present)
			return -EINVAL;
		ret = poulsbo_sgx_create_submit(dev, op->context, &submit_id);
		if (ret)
			return ret;
		op->resolved_submit_id = submit_id;
		return 0;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_FOCUS:
		if (op->flags || op->owner_state || op->packet_count ||
		    op->slot || op->object_id || op->cmd_type ||
		    op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->plan_offset ||
		    dependency_fields_present)
			return -EINVAL;
		ret = poulsbo_sgx_focus_submit(dev, op->submit_id);
		if (ret)
			return ret;
		op->resolved_submit_id = op->submit_id;
		return 0;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_FOCUS_CLEAR:
		if (op->flags || op->submit_id || op->owner_state ||
		    op->packet_count || op->slot || op->object_id ||
		    op->cmd_type || op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->plan_offset ||
		    dependency_fields_present)
			return -EINVAL;
		mutex_lock(&dev_priv->sgx_lab_submit_mutex);
		poulsbo_sgx_clear_submit_focus(dev_priv);
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
		op->resolved_submit_id = 0;
		return 0;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_CLEANUP:
		if (op->flags || op->submit_id || op->owner_state ||
		    op->packet_count || op->context || op->slot ||
		    op->object_id || op->cmd_type || op->ccb_offset ||
		    op->ctrl_flags || op->num_ta_status_vals ||
		    op->num_3d_status_vals || op->ta_dependency ||
		    op->ccb_write_offset || op->ccb_read_offset ||
		    op->ccb_service_address || op->ccb_cache_control ||
		    op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->plan_offset ||
		    dependency_fields_present)
			return -EINVAL;
		op->result_count = poulsbo_sgx_drop_all_submits_count(dev);
		op->resolved_submit_id = 0;
		return 0;
	default:
		break;
	}

	submit_id = op->submit_id;
	if (op->flags & DRM_PSB_SGX_LAB_SUBMIT_OP_USE_FOCUS) {
		mutex_lock(&dev_priv->sgx_lab_submit_mutex);
		if (!dev_priv->sgx_lab_submit_focus_valid) {
			mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
			return -ENOENT;
		}
		submit_id = dev_priv->sgx_lab_submit_focus_id;
		mutex_unlock(&dev_priv->sgx_lab_submit_mutex);
	}

	if (op->slot) {
		slot_name = poulsbo_sgx_lab_submit_slot_name(op->slot);
		if (!slot_name)
			return -EINVAL;
	}

	switch (op->op) {
	case DRM_PSB_SGX_LAB_SUBMIT_OP_VALIDATE:
		if (op->owner_state || op->packet_count ||
		    op->context || op->slot || op->object_id ||
		    op->cmd_type || op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->plan_offset ||
		    dependency_fields_present)
			return -EINVAL;
		ret = poulsbo_sgx_validate_submit(dev, submit_id);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_PREPARE:
		if (op->owner_state || op->packet_count ||
		    op->context || op->slot || op->object_id ||
		    op->cmd_type || op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->plan_offset ||
		    dependency_fields_present)
			return -EINVAL;
		ret = poulsbo_sgx_prepare_submit(dev, submit_id);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_ARM:
		if (op->owner_state || op->packet_count ||
		    op->context || op->slot || op->object_id ||
		    op->cmd_type || op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->plan_offset ||
		    dependency_fields_present)
			return -EINVAL;
		ret = poulsbo_sgx_arm_submit(dev, submit_id);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_EMIT:
		if (op->owner_state || op->packet_count ||
		    op->context || op->slot || op->object_id ||
		    op->cmd_type || op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->plan_offset ||
		    dependency_fields_present)
			return -EINVAL;
		ret = poulsbo_sgx_emit_submit(dev, submit_id);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_LAUNCH:
		if (op->context || op->slot || op->object_id ||
		    op->cmd_type || op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->plan_offset ||
		    dependency_fields_present)
			return -EINVAL;
		if (op->owner_state != DRM_PSB_SGX_LAB_OWNER_QUEUED &&
		    op->owner_state != DRM_PSB_SGX_LAB_OWNER_RUNNING)
			return -EINVAL;
		ret = poulsbo_sgx_launch_submit(dev, submit_id, op->owner_state,
						op->packet_count);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE:
		ret = poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_frontend(
			dev, op, dependency_fields_present, &request_frontend);
		if (!ret)
			submit_id = request_frontend->response_op.resolved_submit_id;
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION:
		ret = poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_frontend(
			dev, op, dependency_fields_present, &request_frontend);
		if (!ret)
			submit_id = request_frontend->response_op.resolved_submit_id;
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_ATTACH:
		if (op->owner_state || op->packet_count || op->context ||
		    !slot_name || !op->object_id || op->cmd_type ||
		    op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->plan_offset ||
		    dependency_fields_present)
			return -EINVAL;
		ret = poulsbo_sgx_attach_submit_object(dev, submit_id,
						       slot_name,
						       op->object_id);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DETACH:
		if (op->owner_state || op->packet_count || op->context ||
		    !slot_name || op->object_id || op->cmd_type ||
		    op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->plan_offset ||
		    dependency_fields_present)
			return -EINVAL;
		ret = poulsbo_sgx_detach_submit_object(dev, submit_id,
						       slot_name);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_KICK:
		if (op->owner_state || op->packet_count || op->context ||
		    op->slot || op->object_id || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->plan_offset ||
		    dependency_fields_present)
			return -EINVAL;
		ret = poulsbo_sgx_configure_submit_kick(dev, submit_id,
							op->cmd_type,
							op->ccb_offset,
							op->ctrl_flags,
							op->num_ta_status_vals,
							op->num_3d_status_vals,
							op->ta_dependency);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_CCB:
		if (op->owner_state || op->packet_count || op->context ||
		    op->slot || op->object_id || op->cmd_type ||
		    op->ctrl_flags || op->num_ta_status_vals ||
		    op->num_3d_status_vals || op->ta_dependency ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || dependency_fields_present)
			return -EINVAL;
		ret = poulsbo_sgx_configure_submit_ccb(dev, submit_id,
						       op->ccb_write_offset,
						       op->ccb_read_offset,
						       op->ccb_service_address,
						       op->ccb_cache_control,
						       op->ccb_data0,
						       op->ccb_data1);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_QUEUE:
		if (op->owner_state || op->packet_count || op->context ||
		    op->slot || op->object_id || op->cmd_type ||
		    op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    dependency_fields_present)
			return -EINVAL;
		ret = poulsbo_sgx_configure_submit_queue(dev, submit_id,
							 op->queue_base_offset,
							 op->queue_size,
							 op->queue_packet_size,
							 op->queue_packet_count);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_STATUS:
		if (op->owner_state || op->packet_count || op->context ||
		    op->slot || op->object_id || op->cmd_type ||
		    op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    dependency_fields_present)
			return -EINVAL;
		ret = poulsbo_sgx_configure_submit_status(dev, submit_id,
							  op->status_ta_offset,
							  op->status_3d_offset,
							  op->status_stride,
							  op->status_value_seed);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_SYNC:
		if (op->owner_state || op->packet_count || op->context ||
		    op->slot || op->object_id || op->cmd_type ||
		    op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || dependency_fields_present)
			return -EINVAL;
		ret = poulsbo_sgx_configure_submit_sync(dev, submit_id,
							op->sync_offset,
							op->sync_complete_seq,
							op->sync_pending_seq,
							op->sync_flags);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_OWNER:
		if (op->packet_count || op->context || op->slot ||
		    op->object_id || op->cmd_type || op->ccb_offset ||
		    op->ctrl_flags || op->num_ta_status_vals ||
		    op->num_3d_status_vals || op->ta_dependency ||
		    op->ccb_write_offset || op->ccb_read_offset ||
		    op->ccb_service_address || op->ccb_cache_control ||
		    op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    dependency_fields_present)
			return -EINVAL;
		if (op->owner_state != DRM_PSB_SGX_LAB_OWNER_QUEUED &&
		    op->owner_state != DRM_PSB_SGX_LAB_OWNER_RUNNING &&
		    op->owner_state != DRM_PSB_SGX_LAB_OWNER_COMPLETE)
			return -EINVAL;
		ret = poulsbo_sgx_configure_submit_owner(dev, submit_id,
							 op->owner_offset,
							 op->owner_state,
							 op->owner_issue_cookie,
							 op->owner_complete_cookie);
		break;
		case DRM_PSB_SGX_LAB_SUBMIT_OP_REPORT:
			if (op->owner_state || op->packet_count || op->context ||
			    op->slot || op->object_id || op->cmd_type ||
		    op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->plan_offset ||
		    dependency_fields_present ||
		    !op->report_offset)
			return -EINVAL;
			ret = poulsbo_sgx_configure_submit_report(dev, submit_id,
								  op->report_offset);
			break;
		case DRM_PSB_SGX_LAB_SUBMIT_OP_LIVE:
			if (op->owner_state || op->packet_count || op->context ||
			    op->slot || op->object_id || op->cmd_type ||
			    op->ccb_offset || op->ctrl_flags ||
			    op->num_ta_status_vals || op->num_3d_status_vals ||
			    op->ta_dependency || op->ccb_write_offset ||
			    op->ccb_read_offset || op->ccb_service_address ||
			    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
			    op->queue_base_offset || op->queue_size ||
			    op->queue_packet_size || op->queue_packet_count ||
			    op->status_ta_offset || op->status_3d_offset ||
			    op->status_stride || op->status_value_seed ||
			    op->sync_offset || op->sync_complete_seq ||
			    op->sync_pending_seq || op->sync_flags ||
			    op->owner_offset || op->owner_issue_cookie ||
			    op->owner_complete_cookie || op->plan_offset ||
			    dependency_fields_present || !op->live_offset)
				return -EINVAL;
			ret = poulsbo_sgx_configure_submit_live(dev, submit_id,
								op->live_offset);
			break;
		case DRM_PSB_SGX_LAB_SUBMIT_OP_RESULT:
			if (op->owner_state || op->packet_count || op->context ||
			    op->slot || op->object_id || op->cmd_type ||
			    op->ccb_offset || op->ctrl_flags ||
			    op->num_ta_status_vals || op->num_3d_status_vals ||
			    op->ta_dependency || op->ccb_write_offset ||
			    op->ccb_read_offset || op->ccb_service_address ||
			    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
			    op->queue_base_offset || op->queue_size ||
			    op->queue_packet_size || op->queue_packet_count ||
			    op->status_ta_offset || op->status_3d_offset ||
			    op->status_stride || op->status_value_seed ||
			    op->sync_offset || op->sync_complete_seq ||
			    op->sync_pending_seq || op->sync_flags ||
			    op->owner_offset || op->owner_issue_cookie ||
			    op->owner_complete_cookie || op->plan_offset ||
			    dependency_fields_present ||
			    !op->result_offset)
				return -EINVAL;
			ret = poulsbo_sgx_configure_submit_result(dev, submit_id,
								  op->result_offset);
			break;
		case DRM_PSB_SGX_LAB_SUBMIT_OP_MANIFEST:
			if (op->owner_state || op->packet_count || op->context ||
			    op->slot || op->object_id || op->cmd_type ||
		    op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->plan_offset ||
		    dependency_fields_present ||
		    !op->manifest_offset)
			return -EINVAL;
		ret = poulsbo_sgx_configure_submit_manifest(dev, submit_id,
							    op->manifest_offset);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DESCRIPTOR:
		if (op->owner_state || op->packet_count || op->context ||
		    op->slot || op->object_id || op->cmd_type ||
		    op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->plan_offset ||
		    dependency_fields_present ||
		    !op->descriptor_offset)
			return -EINVAL;
		ret = poulsbo_sgx_configure_submit_descriptor(
			dev, submit_id, op->descriptor_offset);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_PLAN:
		if (op->owner_state || op->packet_count || op->context ||
		    op->slot || op->object_id || op->cmd_type ||
		    op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || dependency_fields_present ||
		    !op->plan_offset)
			return -EINVAL;
		ret = poulsbo_sgx_configure_submit_plan(dev, submit_id,
							op->plan_offset);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE_PLAN:
		if (op->context || op->slot || op->object_id ||
		    op->cmd_type || op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || dependency_fields_present ||
		    !op->issue_plan_offset)
			return -EINVAL;
		if (op->owner_state != DRM_PSB_SGX_LAB_OWNER_QUEUED &&
		    op->owner_state != DRM_PSB_SGX_LAB_OWNER_RUNNING)
			return -EINVAL;
		ret = poulsbo_sgx_configure_submit_issue_plan(
			dev, submit_id, op->issue_plan_offset,
			op->owner_state, op->packet_count);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DEPEND_SLOT:
		if (op->owner_state || op->packet_count || op->context ||
		    op->slot || op->object_id || op->cmd_type ||
		    op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->dependency_ticket ||
		    op->dependency_offset)
			return -EINVAL;
		ret = poulsbo_sgx_configure_submit_dependency_slot_user(
			dev, submit_id, op->dependency_slot,
			op->dependency_submit_id);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DEPTICKET_SLOT:
		if (op->owner_state || op->packet_count || op->context ||
		    op->slot || op->object_id || op->cmd_type ||
		    op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->dependency_submit_id ||
		    op->dependency_offset)
			return -EINVAL;
		ret = poulsbo_sgx_configure_submit_dependency_ticket_slot(
			dev, submit_id, op->dependency_slot,
			op->dependency_ticket);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DEPSTATE_SLOT:
		if (op->owner_state || op->packet_count || op->context ||
		    op->slot || op->object_id || op->cmd_type ||
		    op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->dependency_submit_id ||
		    op->dependency_ticket)
			return -EINVAL;
		ret = poulsbo_sgx_configure_submit_dependency_state_slot_user(
			dev, submit_id, op->dependency_slot,
			op->dependency_offset);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DEPCLEAR_SLOT:
		if (op->owner_state || op->packet_count || op->context ||
		    op->slot || op->object_id || op->cmd_type ||
		    op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || op->dependency_submit_id ||
		    op->dependency_ticket || op->dependency_offset)
			return -EINVAL;
		ret = poulsbo_sgx_clear_submit_dependency_slot_user(
			dev, submit_id, op->dependency_slot);
		break;
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DROP:
		if (op->owner_state || op->packet_count ||
		    op->context || op->slot || op->object_id ||
		    op->cmd_type || op->ccb_offset || op->ctrl_flags ||
		    op->num_ta_status_vals || op->num_3d_status_vals ||
		    op->ta_dependency || op->ccb_write_offset ||
		    op->ccb_read_offset || op->ccb_service_address ||
		    op->ccb_cache_control || op->ccb_data0 || op->ccb_data1 ||
		    op->queue_base_offset || op->queue_size ||
		    op->queue_packet_size || op->queue_packet_count ||
		    op->status_ta_offset || op->status_3d_offset ||
		    op->status_stride || op->status_value_seed ||
		    op->sync_offset || op->sync_complete_seq ||
		    op->sync_pending_seq || op->sync_flags ||
		    op->owner_offset || op->owner_issue_cookie ||
		    op->owner_complete_cookie || dependency_fields_present)
			return -EINVAL;
		ret = poulsbo_sgx_drop_submit(dev, submit_id);
		break;
	default:
		return -EINVAL;
	}
	if (ret)
		goto out_free_request_frontend;
	if (request_frontend) {
		kfree(request_frontend);
		request_frontend = NULL;
	}

	op->resolved_submit_id = submit_id;
	return 0;

out_free_request_frontend:
	kfree(request_frontend);
	return ret;
}

static ssize_t poulsbo_sgx_state_show(struct device *kdev,
				      struct device_attribute *attr, char *buf)
{
	struct pci_dev *pdev = to_pci_dev(kdev);
	struct drm_device *dev = pci_get_drvdata(pdev);

	if (!dev || !dev->dev_private)
		return scnprintf(buf, PAGE_SIZE, "driver not ready\n");

	psb_sgx_capture_state(dev, attr->attr.name, true);
	return psb_sgx_dump_state(dev, buf, PAGE_SIZE);
}

static DEVICE_ATTR_RO(poulsbo_sgx_state);

static ssize_t poulsbo_gtt_state_show(struct device *kdev,
				      struct device_attribute *attr, char *buf)
{
	struct pci_dev *pdev = to_pci_dev(kdev);
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_psb_private *dev_priv;
	struct psb_mmu_driver *mmu;
	struct psb_mmu_debug_state mmu_debug = { 0 };
	struct resource *res;
	ssize_t pos = 0;
	unsigned int total_ranges = 0;
	unsigned int shown_ranges = 0;
	bool truncated = false;
	int default_pd_context = -1;
	int pf_pd_context = -1;

	if (!dev || !dev->dev_private)
		return scnprintf(buf, PAGE_SIZE, "driver not ready\n");

	dev_priv = dev->dev_private;

	mutex_lock(&dev_priv->gtt_mutex);

	mmu = dev_priv->mmu;
	if (mmu) {
		down_read(&mmu->sem);
		mmu_debug = mmu->debug;
		if (mmu->default_pd)
			default_pd_context = mmu->default_pd->hw_context;
		if (dev_priv->pf_pd)
			pf_pd_context = dev_priv->pf_pd->hw_context;
		up_read(&mmu->sem);
	}

	pos += scnprintf(buf + pos, PAGE_SIZE - pos,
			 "attr=%s gtt_start=%#08x gatt_start=%#08x mmu_gatt_start=%#08x stolen_size=%#lx vram_stolen_size=%#lx gatt_free_offset=%#08x\n",
			 attr->attr.name, dev_priv->gtt.gtt_start,
			 dev_priv->gtt.gatt_start, dev_priv->gtt.mmu_gatt_start,
			 dev_priv->gtt.stolen_size, dev_priv->vram_stolen_size,
			 dev_priv->gatt_free_offset);
	pos += scnprintf(buf + pos, PAGE_SIZE - pos,
			 "mmu_last_op=%s mmu_last_address=%#08lx mmu_last_num_pages=%u mmu_last_type=%d mmu_last_hw_context=%d mmu_last_jiffies=%lu\n",
			 poulsbo_mmu_debug_op_name(mmu_debug.last_op),
			 mmu_debug.last_address, mmu_debug.last_num_pages,
			 mmu_debug.last_type, mmu_debug.last_hw_context,
			 mmu_debug.last_jiffies);
	pos += scnprintf(buf + pos, PAGE_SIZE - pos,
			 "mmu_counts flush=%u set_pd_context=%u insert_pfn_sequence=%u insert_pages=%u remove_pfn_sequence=%u remove_pages=%u default_pd_context=%d pf_pd_context=%d\n",
			 mmu_debug.flush_count, mmu_debug.set_pd_context_count,
			 mmu_debug.insert_pfn_sequence_count,
			 mmu_debug.insert_pages_count,
			 mmu_debug.remove_pfn_sequence_count,
			 mmu_debug.remove_pages_count,
			 default_pd_context, pf_pd_context);

	res = dev_priv->gtt_mem ? dev_priv->gtt_mem->child : NULL;
	while (res) {
		struct gtt_range *gt = container_of(res, struct gtt_range,
						    resource);
		unsigned long len = resource_size(&gt->resource);
		unsigned long gpu_va = dev_priv->gtt.gatt_start + gt->offset;

		total_ranges++;
		if (PAGE_SIZE - pos < 192) {
			truncated = true;
			break;
		}

		pos += scnprintf(buf + pos, PAGE_SIZE - pos,
				 "range id=%u name=%s start=%#08llx end=%#08llx len=%#08lx offset=%#08x gpu_va=%#08lx stolen=%u in_gart=%d mmapping=%u npage=%d roll=%d pin_total=%u unpin_total=%u sgx_object_id=%u pf_bind_count=%u pf_bind_total=%u pf_unbind_total=%u last_pin_jiffies=%lu last_unpin_jiffies=%lu last_pf_bind_jiffies=%lu last_pf_unbind_jiffies=%lu last_lab_adopt_jiffies=%lu last_lab_drop_jiffies=%lu\n",
				 gt->debug_id,
				 gt->resource.name ? gt->resource.name : "?",
				 (unsigned long long)gt->resource.start,
				 (unsigned long long)gt->resource.end,
				 len, gt->offset, gpu_va, gt->stolen,
				 gt->in_gart, gt->mmapping, gt->npage,
				 gt->roll, gt->pin_count_total,
				 gt->unpin_count_total,
				 gt->sgx_lab_object_id,
				 gt->sgx_pf_bind_count,
				 gt->sgx_pf_bind_total,
				 gt->sgx_pf_unbind_total,
				 gt->last_pin_jiffies,
				 gt->last_unpin_jiffies,
				 gt->last_sgx_pf_bind_jiffies,
				 gt->last_sgx_pf_unbind_jiffies,
				 gt->last_sgx_lab_adopt_jiffies,
				 gt->last_sgx_lab_drop_jiffies);
		shown_ranges++;
		res = res->sibling;
	}

	if (truncated)
		pos += scnprintf(buf + pos, PAGE_SIZE - pos,
				 "ranges_shown=%u ranges_total_at_least=%u truncated=1\n",
				 shown_ranges, total_ranges);
	else
		pos += scnprintf(buf + pos, PAGE_SIZE - pos,
				 "ranges_shown=%u ranges_total=%u truncated=0\n",
				 shown_ranges, total_ranges);

	mutex_unlock(&dev_priv->gtt_mutex);
	return pos;
}

static DEVICE_ATTR_RO(poulsbo_gtt_state);

/*
 * The table below contains a mapping of the PCI vendor ID and the PCI Device ID
 * to the different groups of PowerVR 5-series chip designs
 *
 * 0x8086 = Intel Corporation
 *
 * PowerVR SGX535    - Poulsbo    - Intel GMA 500, Intel Atom Z5xx
 * PowerVR SGX535    - Moorestown - Intel GMA 600
 * PowerVR SGX535    - Oaktrail   - Intel GMA 600, Intel Atom Z6xx, E6xx
 * PowerVR SGX540    - Medfield   - Intel Atom Z2460
 * PowerVR SGX544MP2 - Medfield   -
 * PowerVR SGX545    - Cedartrail - Intel GMA 3600, Intel Atom D2500, N2600
 * PowerVR SGX545    - Cedartrail - Intel GMA 3650, Intel Atom D2550, D2700,
 *                                  N2800
 */
static const struct pci_device_id pciidlist[] = {
	{ 0x8086, 0x8108, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &psb_chip_ops },
	{ 0x8086, 0x8109, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &psb_chip_ops },
#if defined(CONFIG_DRM_GMA600)
	{ 0x8086, 0x4100, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &oaktrail_chip_ops },
	{ 0x8086, 0x4101, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &oaktrail_chip_ops },
	{ 0x8086, 0x4102, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &oaktrail_chip_ops },
	{ 0x8086, 0x4103, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &oaktrail_chip_ops },
	{ 0x8086, 0x4104, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &oaktrail_chip_ops },
	{ 0x8086, 0x4105, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &oaktrail_chip_ops },
	{ 0x8086, 0x4106, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &oaktrail_chip_ops },
	{ 0x8086, 0x4107, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &oaktrail_chip_ops },
	{ 0x8086, 0x4108, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &oaktrail_chip_ops },
#endif
#if defined(CONFIG_DRM_MEDFIELD)
	{ 0x8086, 0x0130, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &mdfld_chip_ops },
	{ 0x8086, 0x0131, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &mdfld_chip_ops },
	{ 0x8086, 0x0132, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &mdfld_chip_ops },
	{ 0x8086, 0x0133, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &mdfld_chip_ops },
	{ 0x8086, 0x0134, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &mdfld_chip_ops },
	{ 0x8086, 0x0135, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &mdfld_chip_ops },
	{ 0x8086, 0x0136, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &mdfld_chip_ops },
	{ 0x8086, 0x0137, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &mdfld_chip_ops },
#endif
#if defined(CONFIG_DRM_GMA3600)
	{ 0x8086, 0x0be0, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
	{ 0x8086, 0x0be1, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
	{ 0x8086, 0x0be2, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
	{ 0x8086, 0x0be3, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
	{ 0x8086, 0x0be4, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
	{ 0x8086, 0x0be5, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
	{ 0x8086, 0x0be6, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
	{ 0x8086, 0x0be7, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
	{ 0x8086, 0x0be8, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
	{ 0x8086, 0x0be9, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
	{ 0x8086, 0x0bea, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
	{ 0x8086, 0x0beb, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
	{ 0x8086, 0x0bec, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
	{ 0x8086, 0x0bed, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
	{ 0x8086, 0x0bee, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
	{ 0x8086, 0x0bef, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (long) &cdv_chip_ops },
#endif
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, pciidlist);

/*
 * Standard IOCTLs.
 */
/*
 * Minimal handler for the PVR "unprivileged" DRM command. The closed userspace
 * may probe it during init; the services bridge itself goes through the SRVKM
 * command, so a benign success is sufficient for bring-up.
 */
static int psb_pvr_unpriv_stub(struct drm_device *dev, void *arg,
			       struct drm_file *file)
{
	(void)dev; (void)arg; (void)file;
	return 0;
}

static const struct drm_ioctl_desc psb_ioctls[] = {
	DRM_IOCTL_DEF_DRV(PSB_SGX_LAB_QUERY_SUBMIT,
			  psb_ioctl_sgx_lab_query_submit, 0),
	DRM_IOCTL_DEF_DRV(PSB_SGX_LAB_QUERY_COORD,
			  psb_ioctl_sgx_lab_query_coord, 0),
	DRM_IOCTL_DEF_DRV(PSB_SGX_LAB_QUERY_RANGE,
			  psb_ioctl_sgx_lab_query_range, 0),
	DRM_IOCTL_DEF_DRV(PSB_SGX_LAB_QUERY_SGX_STATE,
			  psb_ioctl_sgx_lab_query_sgx_state, 0),
	DRM_IOCTL_DEF_DRV(PSB_SGX_LAB_QUERY_GTT_STATE,
			  psb_ioctl_sgx_lab_query_gtt_state, 0),
	DRM_IOCTL_DEF_DRV(PSB_SGX_LAB_QUERY_OBJECT,
			  psb_ioctl_sgx_lab_query_object, 0),
	DRM_IOCTL_DEF_DRV(PSB_SGX_LAB_OBJECT_OP,
			  psb_ioctl_sgx_lab_object_op, 0),
	DRM_IOCTL_DEF_DRV(PSB_SGX_LAB_SUBMIT_OP,
			  psb_ioctl_sgx_lab_submit_op, 0),
	DRM_IOCTL_DEF_DRV(PSB_BLT_SUBMIT, psb_blt_submit_ioctl, 0),
	/*
	 * PVR/SGX services bridge, hosted here so the closed EMGD userspace (which
	 * opens this node by busid PCI:00:02:00) can reach pvrsrvkm. Handlers are
	 * exported by pvrsrvkm (pvr_gma_bridge.c). Offsets 0x12/0x15/0x16 match the
	 * historical EMGD DRM_PVR_RESERVED reservations and don't collide with the
	 * PSB ioctls (0x00-0x08).
	 */
	DRM_IOCTL_DEF_DRV(GMA_PVR_SRVKM, gma_pvr_srvkm_dispatch, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GMA_PVR_ISMASTER, gma_pvr_is_master, DRM_MASTER),
	DRM_IOCTL_DEF_DRV(GMA_PVR_UNPRIV, psb_pvr_unpriv_stub, DRM_RENDER_ALLOW),
};

static int psb_do_init(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_gtt *pg = &dev_priv->gtt;

	uint32_t stolen_gtt;

	if (pg->mmu_gatt_start & 0x0FFFFFFF) {
		dev_err(dev->dev, "Gatt must be 256M aligned. This is a bug.\n");
		return -EINVAL;
	}

	stolen_gtt = (pg->stolen_size >> PAGE_SHIFT) * 4;
	stolen_gtt = (stolen_gtt + PAGE_SIZE - 1) >> PAGE_SHIFT;
	stolen_gtt = (stolen_gtt < pg->gtt_pages) ? stolen_gtt : pg->gtt_pages;

	dev_priv->gatt_free_offset = pg->mmu_gatt_start +
	    (stolen_gtt << PAGE_SHIFT) * 1024;

	spin_lock_init(&dev_priv->irqmask_lock);
	spin_lock_init(&dev_priv->sgx_state_lock);
	spin_lock_init(&dev_priv->lock_2d);

	PSB_WSGX32(0x00000000, PSB_CR_BIF_BANK0);
	PSB_WSGX32(0x00000000, PSB_CR_BIF_BANK1);
	PSB_RSGX32(PSB_CR_BIF_BANK1);

	/* Do not bypass any MMU access, let them pagefault instead */
	PSB_WSGX32((PSB_RSGX32(PSB_CR_BIF_CTRL) & ~_PSB_MMU_ER_MASK),
		   PSB_CR_BIF_CTRL);
	PSB_RSGX32(PSB_CR_BIF_CTRL);

	psb_spank(dev_priv);

	/* mmu_gatt ?? */
	PSB_WSGX32(pg->gatt_start, PSB_CR_BIF_TWOD_REQ_BASE);
	PSB_RSGX32(PSB_CR_BIF_TWOD_REQ_BASE); /* Post */

	return 0;
}

/* sgx_3d.c - SGX535 3D bring-up (gated behind module param sgx3d_enable). */
extern int sgx3d_sysfs_register(struct drm_device *dev);
extern void sgx3d_sysfs_unregister(struct drm_device *dev);
extern void sgx3d_fini(struct drm_device *dev);

static void psb_driver_unload(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;

	/* TODO: Kill vblank etc here */

	if (dev_priv) {
		sgx3d_sysfs_unregister(dev);
		sgx3d_fini(dev);
		device_remove_file(dev->dev, &dev_attr_poulsbo_lab_2d);
		device_remove_file(dev->dev, &dev_attr_poulsbo_sgx_bind);
		device_remove_file(dev->dev, &dev_attr_poulsbo_sgx_object);
		device_remove_file(dev->dev, &dev_attr_poulsbo_sgx_objects);
		device_remove_file(dev->dev, &dev_attr_poulsbo_sgx_submit);
		device_remove_file(dev->dev, &dev_attr_poulsbo_sgx_submits);
		device_remove_file(dev->dev, &dev_attr_poulsbo_sgx_submit_state);
		device_remove_file(dev->dev, &dev_attr_poulsbo_sgx_coord_state);
		device_remove_file(dev->dev, &dev_attr_poulsbo_sgx_state);
		device_remove_file(dev->dev, &dev_attr_poulsbo_gtt_state);
		poulsbo_sgx_drop_all_submits(dev);
		poulsbo_sgx_drop_all_objects(dev);

		if (dev_priv->backlight_device)
			gma_backlight_exit(dev);
		psb_modeset_cleanup(dev);

		if (dev_priv->ops->chip_teardown)
			dev_priv->ops->chip_teardown(dev);

		psb_intel_opregion_fini(dev);

		if (dev_priv->pf_pd) {
			psb_mmu_free_pagedir(dev_priv->pf_pd);
			dev_priv->pf_pd = NULL;
		}
		if (dev_priv->mmu) {
			struct psb_gtt *pg = &dev_priv->gtt;

			down_read(&pg->sem);
			psb_mmu_remove_pfn_sequence(
				psb_mmu_get_default_pd
				(dev_priv->mmu),
				pg->mmu_gatt_start,
				dev_priv->vram_stolen_size >> PAGE_SHIFT);
			up_read(&pg->sem);
			psb_mmu_driver_takedown(dev_priv->mmu);
			dev_priv->mmu = NULL;
		}
		psb_gtt_takedown(dev);
		if (dev_priv->scratch_page) {
			set_pages_wb(dev_priv->scratch_page, 1);
			__free_page(dev_priv->scratch_page);
			dev_priv->scratch_page = NULL;
		}
		if (dev_priv->vdc_reg) {
			iounmap(dev_priv->vdc_reg);
			dev_priv->vdc_reg = NULL;
		}
		if (dev_priv->sgx_reg) {
			iounmap(dev_priv->sgx_reg);
			dev_priv->sgx_reg = NULL;
		}
		if (dev_priv->aux_reg) {
			iounmap(dev_priv->aux_reg);
			dev_priv->aux_reg = NULL;
		}
		pci_dev_put(dev_priv->aux_pdev);
		pci_dev_put(dev_priv->lpc_pdev);

		/* Destroy VBT data */
		psb_intel_destroy_bios(dev);

		kfree(dev_priv);
		dev->dev_private = NULL;
	}
	gma_power_uninit(dev);
}

static int psb_driver_load(struct drm_device *dev, unsigned long flags)
{
	struct drm_psb_private *dev_priv;
	unsigned long resource_start, resource_len;
	unsigned long irqflags;
	int ret = -ENOMEM;
	struct drm_connector *connector;
	struct gma_encoder *gma_encoder;
	struct psb_gtt *pg;

	/* allocating and initializing driver private data */
	dev_priv = kzalloc(sizeof(*dev_priv), GFP_KERNEL);
	if (dev_priv == NULL)
		return -ENOMEM;

	dev_priv->ops = (struct psb_ops *)flags;
	dev_priv->dev = dev;
	/* Publish the bound device for the EXPORT_SYMBOL'd flip helpers
	 * (gma500_flip.c / gma500_get_drm()), used by the gma500dc DC module. */
	gma500_drm_dev = dev;
	/*
	 * With legacy .load, minors are registered before psb_driver_load
	 * finishes and an ioctl only checks dev->dev_private for NULL, so
	 * mutex_2d must be initialized before dev_private is published.
	 */
	mutex_init(&dev_priv->mutex_2d);
	dev->dev_private = (void *) dev_priv;
	mutex_init(&dev_priv->sgx_lab_object_mutex);
	INIT_LIST_HEAD(&dev_priv->sgx_lab_object_list);
	atomic_set(&dev_priv->sgx_lab_object_id_next, 0);
	mutex_init(&dev_priv->sgx_lab_submit_mutex);
	INIT_LIST_HEAD(&dev_priv->sgx_lab_submit_list);
	atomic_set(&dev_priv->sgx_lab_submit_id_next, 0);

	pg = &dev_priv->gtt;

	pci_set_master(dev->pdev);

	dev_priv->num_pipe = dev_priv->ops->pipes;

	resource_start = pci_resource_start(dev->pdev, PSB_MMIO_RESOURCE);

	dev_priv->vdc_reg =
	    ioremap(resource_start + PSB_VDC_OFFSET, PSB_VDC_SIZE);
	if (!dev_priv->vdc_reg)
		goto out_err;

	dev_priv->sgx_reg = ioremap(resource_start + dev_priv->ops->sgx_offset,
							PSB_SGX_SIZE);
	if (!dev_priv->sgx_reg)
		goto out_err;

	if (IS_MRST(dev)) {
		int domain = pci_domain_nr(dev->pdev->bus);

		dev_priv->aux_pdev =
			pci_get_domain_bus_and_slot(domain, 0,
						    PCI_DEVFN(3, 0));

		if (dev_priv->aux_pdev) {
			resource_start = pci_resource_start(dev_priv->aux_pdev,
							    PSB_AUX_RESOURCE);
			resource_len = pci_resource_len(dev_priv->aux_pdev,
							PSB_AUX_RESOURCE);
			dev_priv->aux_reg = ioremap_nocache(resource_start,
							    resource_len);
			if (!dev_priv->aux_reg)
				goto out_err;

			DRM_DEBUG_KMS("Found aux vdc");
		} else {
			/* Couldn't find the aux vdc so map to primary vdc */
			dev_priv->aux_reg = dev_priv->vdc_reg;
			DRM_DEBUG_KMS("Couldn't find aux pci device");
		}
		dev_priv->gmbus_reg = dev_priv->aux_reg;

		dev_priv->lpc_pdev =
			pci_get_domain_bus_and_slot(domain, 0,
						    PCI_DEVFN(31, 0));
		if (dev_priv->lpc_pdev) {
			pci_read_config_word(dev_priv->lpc_pdev, PSB_LPC_GBA,
				&dev_priv->lpc_gpio_base);
			pci_write_config_dword(dev_priv->lpc_pdev, PSB_LPC_GBA,
				(u32)dev_priv->lpc_gpio_base | (1L<<31));
			pci_read_config_word(dev_priv->lpc_pdev, PSB_LPC_GBA,
				&dev_priv->lpc_gpio_base);
			dev_priv->lpc_gpio_base &= 0xffc0;
			if (dev_priv->lpc_gpio_base)
				DRM_DEBUG_KMS("Found LPC GPIO at 0x%04x\n",
						dev_priv->lpc_gpio_base);
			else {
				pci_dev_put(dev_priv->lpc_pdev);
				dev_priv->lpc_pdev = NULL;
			}
		}
	} else {
		dev_priv->gmbus_reg = dev_priv->vdc_reg;
	}

	psb_intel_opregion_setup(dev);

	ret = dev_priv->ops->chip_setup(dev);
	if (ret)
		goto out_err;

	/* Init OSPM support */
	gma_power_init(dev);

	ret = -ENOMEM;

	dev_priv->scratch_page = alloc_page(GFP_DMA32 | __GFP_ZERO);
	if (!dev_priv->scratch_page)
		goto out_err;

	set_pages_uc(dev_priv->scratch_page, 1);

	ret = psb_gtt_init(dev, 0);
	if (ret)
		goto out_err;

	ret = -ENOMEM;

	dev_priv->mmu = psb_mmu_driver_init(dev, 1, 0, 0);
	if (!dev_priv->mmu)
		goto out_err;

	dev_priv->pf_pd = psb_mmu_alloc_pd(dev_priv->mmu, 1, 0);
	if (!dev_priv->pf_pd)
		goto out_err;

	ret = psb_do_init(dev);
	if (ret)
		return ret;

	/* Add stolen memory to SGX MMU */
	down_read(&pg->sem);
	ret = psb_mmu_insert_pfn_sequence(psb_mmu_get_default_pd(dev_priv->mmu),
					  dev_priv->stolen_base >> PAGE_SHIFT,
					  pg->gatt_start,
					  pg->stolen_size >> PAGE_SHIFT, 0);
	up_read(&pg->sem);

	psb_mmu_set_pd_context(psb_mmu_get_default_pd(dev_priv->mmu), 0);
	psb_mmu_set_pd_context(dev_priv->pf_pd, 1);

	PSB_WSGX32(0x20000000, PSB_CR_PDS_EXEC_BASE);
	PSB_WSGX32(0x30000000, PSB_CR_BIF_3D_REQ_BASE);

	acpi_video_register();

	/* Setup vertical blanking handling */
	ret = drm_vblank_init(dev, dev_priv->num_pipe);
	if (ret)
		goto out_err;

	/*
	 * Install interrupt handlers prior to powering off SGX or else we will
	 * crash.
	 */
	dev_priv->vdc_irq_mask = 0;
	dev_priv->pipestat[0] = 0;
	dev_priv->pipestat[1] = 0;
	dev_priv->pipestat[2] = 0;
	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);
	PSB_WVDC32(0xFFFFFFFF, PSB_HWSTAM);
	PSB_WVDC32(0x00000000, PSB_INT_ENABLE_R);
	PSB_WVDC32(0xFFFFFFFF, PSB_INT_MASK_R);
	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);

	drm_irq_install(dev, dev->pdev->irq);

	dev->max_vblank_count = 0xffffff; /* only 24 bits of frame count */
	dev->driver->get_vblank_counter = psb_get_vblank_counter;

	psb_modeset_init(dev);
	psb_fbdev_init(dev);
	ret = device_create_file(dev->dev, &dev_attr_poulsbo_lab_2d);
	if (ret) {
		dev_warn(dev->dev,
			 "failed to create poulsbo_lab_2d sysfs control: %d\n",
			 ret);
		ret = 0;
	}
	ret = device_create_file(dev->dev, &dev_attr_poulsbo_sgx_bind);
	if (ret) {
		dev_warn(dev->dev,
			 "failed to create poulsbo_sgx_bind sysfs control: %d\n",
			 ret);
		ret = 0;
	}
	ret = device_create_file(dev->dev, &dev_attr_poulsbo_sgx_object);
	if (ret) {
		dev_warn(dev->dev,
			 "failed to create poulsbo_sgx_object sysfs control: %d\n",
			 ret);
		ret = 0;
	}
	ret = device_create_file(dev->dev, &dev_attr_poulsbo_sgx_objects);
	if (ret) {
		dev_warn(dev->dev,
			 "failed to create poulsbo_sgx_objects sysfs view: %d\n",
			 ret);
		ret = 0;
	}
	ret = device_create_file(dev->dev, &dev_attr_poulsbo_sgx_submit);
	if (ret) {
		dev_warn(dev->dev,
			 "failed to create poulsbo_sgx_submit sysfs control: %d\n",
			 ret);
		ret = 0;
	}
	ret = device_create_file(dev->dev, &dev_attr_poulsbo_sgx_submits);
	if (ret) {
		dev_warn(dev->dev,
			 "failed to create poulsbo_sgx_submits sysfs view: %d\n",
			 ret);
		ret = 0;
	}
	ret = device_create_file(dev->dev, &dev_attr_poulsbo_sgx_submit_state);
	if (ret) {
		dev_warn(dev->dev,
			 "failed to create poulsbo_sgx_submit_state sysfs view: %d\n",
			 ret);
		ret = 0;
	}
	ret = device_create_file(dev->dev, &dev_attr_poulsbo_sgx_coord_state);
	if (ret) {
		dev_warn(dev->dev,
			 "failed to create poulsbo_sgx_coord_state sysfs view: %d\n",
			 ret);
		ret = 0;
	}
	ret = device_create_file(dev->dev, &dev_attr_poulsbo_sgx_state);
	if (ret) {
		dev_warn(dev->dev,
			 "failed to create poulsbo_sgx_state sysfs view: %d\n",
			 ret);
		ret = 0;
	}
	ret = device_create_file(dev->dev, &dev_attr_poulsbo_gtt_state);
	if (ret) {
		dev_warn(dev->dev,
			 "failed to create poulsbo_gtt_state sysfs view: %d\n",
			 ret);
		ret = 0;
	}
	sgx3d_sysfs_register(dev);	/* SGX 3D bring-up trigger (gated) */
	drm_kms_helper_poll_init(dev);

	/* Only add backlight support if we have LVDS output */
	list_for_each_entry(connector, &dev->mode_config.connector_list,
			    head) {
		gma_encoder = gma_attached_encoder(connector);

		switch (gma_encoder->type) {
		case INTEL_OUTPUT_LVDS:
		case INTEL_OUTPUT_MIPI:
			ret = gma_backlight_init(dev);
			break;
		}
	}

	if (ret)
		return ret;
	psb_intel_opregion_enable_asle(dev);
#if 0
	/* Enable runtime pm at last */
	pm_runtime_enable(&dev->pdev->dev);
	pm_runtime_set_active(&dev->pdev->dev);
#endif
	/* Intel drm driver load is done, continue doing pvr load */
	return 0;
out_err:
	psb_driver_unload(dev);
	return ret;
}

static inline void get_brightness(struct backlight_device *bd)
{
#ifdef CONFIG_BACKLIGHT_CLASS_DEVICE
	if (bd) {
		bd->props.brightness = bd->ops->get_brightness(bd);
		backlight_update_status(bd);
	}
#endif
}

static long psb_unlocked_ioctl(struct file *filp, unsigned int cmd,
			       unsigned long arg)
{
	struct drm_file *file_priv = filp->private_data;
	struct drm_device *dev = file_priv->minor->dev;
	struct drm_psb_private *dev_priv = dev->dev_private;
	static unsigned int runtime_allowed;

	if (runtime_allowed == 1 && dev_priv->is_lvds_on) {
		runtime_allowed++;
		pm_runtime_allow(&dev->pdev->dev);
		dev_priv->rpm_enabled = 1;
	}
	return drm_ioctl(filp, cmd, arg);
	/* FIXME: do we need to wrap the other side of this */
}

static int psb_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	return drm_get_pci_dev(pdev, ent, &driver);
}


static void psb_pci_remove(struct pci_dev *pdev)
{
	struct drm_device *dev = pci_get_drvdata(pdev);
	drm_put_dev(dev);
}

static const struct dev_pm_ops psb_pm_ops = {
	.resume = gma_power_resume,
	.suspend = gma_power_suspend,
	.thaw = gma_power_thaw,
	.freeze = gma_power_freeze,
	.restore = gma_power_restore,
	.runtime_suspend = psb_runtime_suspend,
	.runtime_resume = psb_runtime_resume,
	.runtime_idle = psb_runtime_idle,
};

static const struct vm_operations_struct psb_gem_vm_ops = {
	.fault = psb_gem_fault,
	.open = drm_gem_vm_open,
	.close = drm_gem_vm_close,
};

/*
 * mmap wrapper: the closed EMGD userspace mmap()s PVR device memory through
 * this node (it opened us by busid for the PVR bridge). Give pvrsrvkm first
 * refusal via gma_pvr_try_mmap(); if the offset isn't a PVR area it returns 1
 * and we fall back to the normal GEM mmap.
 */
static int psb_pvr_gem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret = gma_pvr_try_mmap(filp, vma);

	if (ret == 1)
		return drm_gem_mmap(filp, vma);
	return ret;
}

static const struct file_operations psb_gem_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.release = drm_release,
	.unlocked_ioctl = psb_unlocked_ioctl,
	.compat_ioctl = drm_compat_ioctl,
	.mmap = psb_pvr_gem_mmap,
	.poll = drm_poll,
	.read = drm_read,
};

static struct drm_driver driver = {
	.driver_features = DRIVER_MODESET | DRIVER_GEM,
	.load = psb_driver_load,
	.unload = psb_driver_unload,
	.lastclose = drm_fb_helper_lastclose,

	/* Create/destroy the per-client PVR services connection so the closed
	 * EMGD userspace (opening this node by busid) can use the SGX via the
	 * hosted PVR bridge ioctls. Handlers exported by pvrsrvkm. */
	.open = gma_pvr_srvkm_open,
	.postclose = gma_pvr_srvkm_release,

	.num_ioctls = ARRAY_SIZE(psb_ioctls),
	.irq_preinstall = psb_irq_preinstall,
	.irq_postinstall = psb_irq_postinstall,
	.irq_uninstall = psb_irq_uninstall,
	.irq_handler = psb_irq_handler,
	.enable_vblank = psb_enable_vblank,
	.disable_vblank = psb_disable_vblank,
	.get_vblank_counter = psb_get_vblank_counter,

	.gem_free_object = psb_gem_free_object,
	.gem_vm_ops = &psb_gem_vm_ops,

	.dumb_create = psb_gem_dumb_create,
	.ioctls = psb_ioctls,
	.fops = &psb_gem_fops,
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL
};

static struct pci_driver psb_pci_driver = {
	.name = DRIVER_NAME,
	.id_table = pciidlist,
	.probe = psb_pci_probe,
	.remove = psb_pci_remove,
	.driver.pm = &psb_pm_ops,
};

static int __init psb_init(void)
{
	return pci_register_driver(&psb_pci_driver);
}

static void __exit psb_exit(void)
{
	pci_unregister_driver(&psb_pci_driver);
}

late_initcall(psb_init);
module_exit(psb_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
