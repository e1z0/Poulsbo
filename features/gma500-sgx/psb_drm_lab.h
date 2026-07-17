/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _PSB_DRM_LAB_H_
#define _PSB_DRM_LAB_H_

#include <drm/drm.h>

#define DRM_PSB_SGX_LAB_QUERY_SUBMIT 0x00
#define DRM_PSB_SGX_LAB_QUERY_COORD  0x01
#define DRM_PSB_SGX_LAB_SUBMIT_OP    0x02
#define DRM_PSB_SGX_LAB_OBJECT_OP    0x03
#define DRM_PSB_SGX_LAB_QUERY_RANGE  0x04
#define DRM_PSB_SGX_LAB_QUERY_SGX_STATE 0x05
#define DRM_PSB_SGX_LAB_QUERY_GTT_STATE 0x06
#define DRM_PSB_SGX_LAB_QUERY_OBJECT 0x07

#define DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_FOCUS 0x00000001
#define DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_INDEX 0x00000002
#define DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_GPU_VA 0x00000004
#define DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_GPU_VA_MATCH_INDEX 0x00000008
#define DRM_PSB_SGX_LAB_SUBMIT_OP_USE_FOCUS 0x00000001
#define DRM_PSB_SGX_LAB_QUERY_RANGE_USE_ID 0x00000001
#define DRM_PSB_SGX_LAB_QUERY_RANGE_FILTER_GEM 0x00000002
#define DRM_PSB_SGX_LAB_QUERY_RANGE_USE_GPU_VA 0x00000004
#define DRM_PSB_SGX_LAB_QUERY_RANGE_USE_SUBMIT_MATCH_INDEX 0x00000008
#define DRM_PSB_SGX_LAB_QUERY_SGX_STATE_CAPTURE 0x00000001
#define DRM_PSB_SGX_LAB_QUERY_OBJECT_USE_ID 0x00000001
#define DRM_PSB_SGX_LAB_DEPENDENCY_SLOTS 4
#define DRM_PSB_SGX_LAB_SGX_REASON_LEN 32

#define DRM_PSB_SGX_LAB_RANGE_UNKNOWN 0
#define DRM_PSB_SGX_LAB_RANGE_GEM 1
#define DRM_PSB_SGX_LAB_RANGE_FB 2
#define DRM_PSB_SGX_LAB_RANGE_CURSOR 3
#define DRM_PSB_SGX_LAB_RANGE_OTHER 4

#define DRM_PSB_SGX_LAB_CONTEXT_AUTO 0
#define DRM_PSB_SGX_LAB_CONTEXT_DEFAULT 1
#define DRM_PSB_SGX_LAB_CONTEXT_PF 2

#define DRM_PSB_SGX_LAB_SUBMIT_SLOT_CONTROL 1
#define DRM_PSB_SGX_LAB_SUBMIT_SLOT_PDS 2
#define DRM_PSB_SGX_LAB_SUBMIT_SLOT_USE 3
#define DRM_PSB_SGX_LAB_SUBMIT_SLOT_TARGET 4
#define DRM_PSB_SGX_LAB_SUBMIT_SLOT_STATUS 5

#define DRM_PSB_SGX_LAB_ROLE_GENERIC 0
#define DRM_PSB_SGX_LAB_ROLE_PDS 1
#define DRM_PSB_SGX_LAB_ROLE_USE 2
#define DRM_PSB_SGX_LAB_ROLE_CONTROL 3
#define DRM_PSB_SGX_LAB_ROLE_TARGET 4
#define DRM_PSB_SGX_LAB_ROLE_STATUS 5

#define DRM_PSB_SGX_LAB_CLASS_GENERIC 0
#define DRM_PSB_SGX_LAB_CLASS_PACKET 1
#define DRM_PSB_SGX_LAB_CLASS_CODE 2
#define DRM_PSB_SGX_LAB_CLASS_STATE 3
#define DRM_PSB_SGX_LAB_CLASS_SURFACE 4

#define DRM_PSB_SGX_LAB_GPU_USE_GENERIC 0
#define DRM_PSB_SGX_LAB_GPU_USE_CONTROL_CCB 1
#define DRM_PSB_SGX_LAB_GPU_USE_PDS_CODE 2
#define DRM_PSB_SGX_LAB_GPU_USE_USE_CODE 3
#define DRM_PSB_SGX_LAB_GPU_USE_RENDER_TARGET 4
#define DRM_PSB_SGX_LAB_GPU_USE_STATUS_BLOCK 5

#define DRM_PSB_SGX_LAB_CMD_INVALID 0
#define DRM_PSB_SGX_LAB_CMD_TA 1
#define DRM_PSB_SGX_LAB_CMD_TRANSFER 2
#define DRM_PSB_SGX_LAB_CMD_2D 3
#define DRM_PSB_SGX_LAB_CMD_POWER 4
#define DRM_PSB_SGX_LAB_CMD_CLEANUP 5
#define DRM_PSB_SGX_LAB_CMD_GETMISCINFO 6
#define DRM_PSB_SGX_LAB_CMD_PROCESS_QUEUES 7

#define DRM_PSB_SGX_LAB_OWNER_NONE 0
#define DRM_PSB_SGX_LAB_OWNER_QUEUED 1
#define DRM_PSB_SGX_LAB_OWNER_RUNNING 2
#define DRM_PSB_SGX_LAB_OWNER_COMPLETE 3

#define DRM_PSB_SGX_LAB_REPORT_STAGE_NONE 0
#define DRM_PSB_SGX_LAB_REPORT_STAGE_PREPARED 1
#define DRM_PSB_SGX_LAB_REPORT_STAGE_ARMED 2
#define DRM_PSB_SGX_LAB_REPORT_STAGE_EMITTED 3
#define DRM_PSB_SGX_LAB_REPORT_STAGE_LAUNCHED 4
#define DRM_PSB_SGX_LAB_REPORT_STAGE_DISPATCHED 5
#define DRM_PSB_SGX_LAB_REPORT_STAGE_RETIRED 6

#define DRM_PSB_SGX_LAB_REPORT_FLAG_ARM_VALID 0x00000001
#define DRM_PSB_SGX_LAB_REPORT_FLAG_EMIT_VALID 0x00000002
#define DRM_PSB_SGX_LAB_REPORT_FLAG_LAUNCH_VALID 0x00000004
#define DRM_PSB_SGX_LAB_REPORT_FLAG_LAUNCH_CONSUMED 0x00000008
#define DRM_PSB_SGX_LAB_REPORT_FLAG_DISPATCH_VALID 0x00000010
#define DRM_PSB_SGX_LAB_REPORT_FLAG_RETIRED_VALID 0x00000020

#define DRM_PSB_SGX_LAB_PLAN_SOURCE_NONE 0
#define DRM_PSB_SGX_LAB_PLAN_SOURCE_LIVE 1
#define DRM_PSB_SGX_LAB_PLAN_SOURCE_LAUNCH 2
#define DRM_PSB_SGX_LAB_PLAN_SOURCE_DISPATCH 3
#define DRM_PSB_SGX_LAB_PLAN_SOURCE_RETIRED 4

#define DRM_PSB_SGX_LAB_RESULT_SOURCE_NONE 0
#define DRM_PSB_SGX_LAB_RESULT_SOURCE_LIVE 1
#define DRM_PSB_SGX_LAB_RESULT_SOURCE_DISPATCH 2
#define DRM_PSB_SGX_LAB_RESULT_SOURCE_RETIRED 3

#define DRM_PSB_SGX_LAB_DEP_FLAG_READY 0x00000001
#define DRM_PSB_SGX_LAB_DEP_FLAG_SOURCE_BOUND 0x00000002
#define DRM_PSB_SGX_LAB_DEP_FLAG_SATISFIED 0x00000004
#define DRM_PSB_SGX_LAB_DEP_FLAG_GATED 0x00000008
#define DRM_PSB_SGX_LAB_DEP_FLAG_SOURCE_MISSING 0x00000010

#define DRM_PSB_SGX_LAB_SUBMIT_OP_VALIDATE 1
#define DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE 2
#define DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION 3
#define DRM_PSB_SGX_LAB_SUBMIT_OP_CREATE 4
#define DRM_PSB_SGX_LAB_SUBMIT_OP_DROP 5
#define DRM_PSB_SGX_LAB_SUBMIT_OP_FOCUS 6
#define DRM_PSB_SGX_LAB_SUBMIT_OP_FOCUS_CLEAR 7
#define DRM_PSB_SGX_LAB_SUBMIT_OP_ATTACH 8
#define DRM_PSB_SGX_LAB_SUBMIT_OP_DETACH 9
#define DRM_PSB_SGX_LAB_SUBMIT_OP_KICK 10
#define DRM_PSB_SGX_LAB_SUBMIT_OP_CCB 11
#define DRM_PSB_SGX_LAB_SUBMIT_OP_QUEUE 12
#define DRM_PSB_SGX_LAB_SUBMIT_OP_STATUS 13
#define DRM_PSB_SGX_LAB_SUBMIT_OP_SYNC 14
#define DRM_PSB_SGX_LAB_SUBMIT_OP_OWNER 15
#define DRM_PSB_SGX_LAB_SUBMIT_OP_DEPEND_SLOT 16
#define DRM_PSB_SGX_LAB_SUBMIT_OP_DEPTICKET_SLOT 17
#define DRM_PSB_SGX_LAB_SUBMIT_OP_DEPSTATE_SLOT 18
#define DRM_PSB_SGX_LAB_SUBMIT_OP_DEPCLEAR_SLOT 19
#define DRM_PSB_SGX_LAB_SUBMIT_OP_CLEANUP 20
#define DRM_PSB_SGX_LAB_SUBMIT_OP_PREPARE 21
#define DRM_PSB_SGX_LAB_SUBMIT_OP_ARM 22
#define DRM_PSB_SGX_LAB_SUBMIT_OP_EMIT 23
#define DRM_PSB_SGX_LAB_SUBMIT_OP_LAUNCH 24
#define DRM_PSB_SGX_LAB_SUBMIT_OP_REPORT 25
#define DRM_PSB_SGX_LAB_SUBMIT_OP_MANIFEST 26
#define DRM_PSB_SGX_LAB_SUBMIT_OP_DESCRIPTOR 27
#define DRM_PSB_SGX_LAB_SUBMIT_OP_PLAN 28
#define DRM_PSB_SGX_LAB_SUBMIT_OP_RESULT 29
#define DRM_PSB_SGX_LAB_SUBMIT_OP_LIVE 30
#define DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE_PLAN 31

#define DRM_PSB_SGX_LAB_OBJECT_OP_ADOPT 1
#define DRM_PSB_SGX_LAB_OBJECT_OP_DROP 2
#define DRM_PSB_SGX_LAB_OBJECT_OP_PF_BIND 3
#define DRM_PSB_SGX_LAB_OBJECT_OP_PF_UNBIND 4
#define DRM_PSB_SGX_LAB_OBJECT_OP_GPU_USE 5
#define DRM_PSB_SGX_LAB_OBJECT_OP_CONTEXT 6
#define DRM_PSB_SGX_LAB_OBJECT_OP_ROLE 7
#define DRM_PSB_SGX_LAB_OBJECT_OP_CLASS 8
#define DRM_PSB_SGX_LAB_OBJECT_OP_CLEANUP 9

struct drm_psb_sgx_lab_dependency_query {
	__u32 submit_id;
	__u32 required_ticket;
	__u32 observed_complete_ticket;
	__u32 flags;
};

struct drm_psb_sgx_lab_submit_query {
	__u32 size;
	__u32 flags;
	__u32 submit_id;
	__u32 query_index;
	__u64 query_gpu_va;
	__u32 query_match_index;
	__u32 resolved_submit_id;
	__u32 resolved_index;
	__u32 total_submit_count;
	__u32 matching_submit_count;
	__u32 resolved_match_index;
	__u32 resolved_slot;
	__u32 resolved_object_id;
	__u32 resolved_range_id;
	__u32 resolved_range_type;
	__u32 resolved_object_role;
	__u32 resolved_object_class;
	__u32 resolved_object_gpu_use;
	__u32 resolved_object_expected_context;
	__u64 query_gpu_va_offset;
	__u32 context;
	__u32 cmd_type;
	__u32 owner_state;
	__s32 last_validate_status;
	__u32 num_ta_status_vals;
	__u32 num_3d_status_vals;
	__u32 ta_dependency;
	__u32 ctrl_flags;
	__u32 control_object_id;
	__u32 pds_object_id;
	__u32 use_object_id;
	__u32 target_object_id;
	__u32 status_object_id;
	__u32 ccb_offset;
	__u32 ccb_write_offset;
	__u32 ccb_read_offset;
	__u32 ccb_service_address;
	__u32 ccb_cache_control;
	__u32 ccb_data0;
	__u32 ccb_data1;
	__u32 ccb_pds_gpu_va32;
	__u32 ccb_use_gpu_va32;
	__u32 ccb_target_gpu_va32;
	__u32 ccb_status_gpu_va32;
	__u32 queue_base_offset;
	__u32 queue_size;
	__u32 queue_packet_size;
	__u32 queue_packet_count;
	__u32 queue_tail_packet_count;
	__u32 queue_head_packet_count;
	__u32 queue_epoch;
	__u32 status_ta_offset;
	__u32 status_3d_offset;
	__u32 status_stride;
	__u32 status_value_seed;
	__u32 sync_offset;
	__u32 sync_complete_seq;
	__u32 sync_pending_seq;
	__u32 sync_flags;
	__u32 owner_offset;
	__u32 owner_issue_cookie;
	__u32 owner_complete_cookie;
	__u32 global_issue_ticket;
	__u32 global_running_ticket;
	__u32 global_complete_ticket;
	__u32 validate_count;
	__u32 prepare_count;
	__u32 arm_valid;
	__u32 arm_count;
	__u32 arm_prepare_count;
	__u32 arm_queue_packet_count;
	__u32 arm_queue_tail_packet_count;
	__u32 arm_queue_head_packet_count;
	__u32 arm_ccb_read_offset;
	__u32 arm_ccb_write_offset;
	__u32 arm_sync_complete_seq;
	__u32 arm_sync_pending_seq;
	__u32 arm_owner_state;
	__u32 arm_owner_issue_cookie;
	__u32 arm_owner_complete_cookie;
	__u32 arm_queue_epoch;
	__u32 arm_global_issue_ticket;
	__u32 emit_valid;
	__u32 emit_count;
	__u32 emit_prepare_count;
	__u32 emit_arm_count;
	__u32 emit_context;
	__u32 emit_cmd_type;
	__u32 emit_owner_state;
	__u32 emit_ctrl_flags;
	__u32 emit_num_ta_status_vals;
	__u32 emit_num_3d_status_vals;
	__u32 emit_ta_dependency;
	__u32 emit_control_object_id;
	__u32 emit_pds_object_id;
	__u32 emit_use_object_id;
	__u32 emit_target_object_id;
	__u32 emit_status_object_id;
	__u32 emit_ccb_offset;
	__u32 emit_ccb_write_offset;
	__u32 emit_ccb_read_offset;
	__u32 emit_ccb_service_address;
	__u32 emit_ccb_cache_control;
	__u32 emit_ccb_data0;
	__u32 emit_ccb_data1;
	__u32 emit_queue_base_offset;
	__u32 emit_queue_size;
	__u32 emit_queue_packet_size;
	__u32 emit_queue_packet_count;
	__u32 emit_queue_tail_packet_count;
	__u32 emit_queue_head_packet_count;
	__u32 emit_sync_complete_seq;
	__u32 emit_sync_pending_seq;
	__u32 emit_sync_flags;
	__u32 emit_owner_issue_cookie;
	__u32 emit_owner_complete_cookie;
	__u32 emit_queue_epoch;
	__u32 emit_global_issue_ticket;
	__u32 emit_dependency_active_count;
	__u64 emit_control_gpu_va;
	__u64 emit_pds_gpu_va;
	__u64 emit_use_gpu_va;
	__u64 emit_target_gpu_va;
	__u64 emit_status_gpu_va;
	__u32 launch_valid;
	__u32 launch_consumed;
	__u32 launch_count;
	__u32 launch_prepare_count;
	__u32 launch_arm_count;
	__u32 launch_emit_count;
	__u32 launch_context;
	__u32 launch_cmd_type;
	__u32 launch_owner_state;
	__u32 launch_issued_packets;
	__u32 launch_refilled_from_complete;
	__u32 launch_ctrl_flags;
	__u32 launch_num_ta_status_vals;
	__u32 launch_num_3d_status_vals;
	__u32 launch_ta_dependency;
	__u32 launch_control_object_id;
	__u32 launch_pds_object_id;
	__u32 launch_use_object_id;
	__u32 launch_target_object_id;
	__u32 launch_status_object_id;
	__u32 launch_ccb_offset;
	__u32 launch_ccb_write_offset;
	__u32 launch_ccb_read_offset;
	__u32 launch_new_write_offset;
	__u32 launch_ccb_service_address;
	__u32 launch_ccb_cache_control;
	__u32 launch_ccb_data0;
	__u32 launch_ccb_data1;
	__u32 launch_queue_base_offset;
	__u32 launch_queue_size;
	__u32 launch_queue_packet_size;
	__u32 launch_queue_packet_count;
	__u32 launch_queue_tail_packet_count;
	__u32 launch_queue_head_packet_count;
	__u32 launch_sync_offset;
	__u32 launch_sync_complete_seq;
	__u32 launch_sync_pending_seq;
	__u32 launch_sync_flags;
	__u32 launch_owner_offset;
	__u32 launch_owner_issue_cookie;
	__u32 launch_owner_complete_cookie;
	__u32 launch_queue_epoch;
	__u32 launch_global_issue_ticket;
	__u32 launch_global_running_ticket;
	__u32 launch_global_complete_ticket;
	__u32 launch_dependency_active_count;
	__u64 launch_control_gpu_va;
	__u64 launch_pds_gpu_va;
	__u64 launch_use_gpu_va;
	__u64 launch_target_gpu_va;
	__u64 launch_status_gpu_va;
	__u32 dispatch_valid;
	__u32 dispatch_count;
	__u32 dispatch_prepare_count;
	__u32 dispatch_arm_count;
	__u32 dispatch_emit_count;
	__u32 dispatch_launch_count;
	__u32 dispatch_context;
	__u32 dispatch_cmd_type;
	__u32 dispatch_owner_state;
	__u32 dispatch_issued_packets;
	__u32 dispatch_completed_packets;
	__u32 dispatch_refilled_from_complete;
	__u32 dispatch_ctrl_flags;
	__u32 dispatch_num_ta_status_vals;
	__u32 dispatch_num_3d_status_vals;
	__u32 dispatch_ta_dependency;
	__u32 dispatch_control_object_id;
	__u32 dispatch_pds_object_id;
	__u32 dispatch_use_object_id;
	__u32 dispatch_target_object_id;
	__u32 dispatch_status_object_id;
	__u32 dispatch_ccb_offset;
	__u32 dispatch_ccb_write_offset;
	__u32 dispatch_ccb_read_offset;
	__u32 dispatch_ccb_service_address;
	__u32 dispatch_ccb_cache_control;
	__u32 dispatch_ccb_data0;
	__u32 dispatch_ccb_data1;
	__u32 dispatch_queue_base_offset;
	__u32 dispatch_queue_size;
	__u32 dispatch_queue_packet_size;
	__u32 dispatch_queue_packet_count;
	__u32 dispatch_queue_tail_packet_count;
	__u32 dispatch_queue_head_packet_count;
	__u32 dispatch_sync_offset;
	__u32 dispatch_sync_complete_seq;
	__u32 dispatch_sync_pending_seq;
	__u32 dispatch_sync_flags;
	__u32 dispatch_owner_offset;
	__u32 dispatch_owner_issue_cookie;
	__u32 dispatch_owner_complete_cookie;
	__u32 dispatch_queue_epoch;
	__u32 dispatch_global_issue_ticket;
	__u32 dispatch_global_running_ticket;
	__u32 dispatch_global_complete_ticket;
	__u32 dispatch_dependency_active_count;
	__u64 dispatch_control_gpu_va;
	__u64 dispatch_pds_gpu_va;
	__u64 dispatch_use_gpu_va;
	__u64 dispatch_target_gpu_va;
	__u64 dispatch_status_gpu_va;
	__u32 retired_valid;
	__u32 retired_count;
	__u32 retired_prepare_count;
	__u32 retired_arm_count;
	__u32 retired_emit_count;
	__u32 retired_launch_count;
	__u32 retired_dispatch_count;
	__u32 retired_context;
	__u32 retired_cmd_type;
	__u32 retired_owner_state;
	__u32 retired_issued_packets;
	__u32 retired_completed_packets;
	__u32 retired_refilled_from_complete;
	__u32 retired_queue_packet_count;
	__u32 retired_sync_complete_seq;
	__u32 retired_sync_pending_seq;
	__u32 retired_owner_issue_cookie;
	__u32 retired_owner_complete_cookie;
	__u32 retired_queue_epoch;
	__u32 retired_global_issue_ticket;
	__u32 retired_global_running_ticket;
	__u32 retired_global_complete_ticket;
	__u32 retired_dependency_active_count;
	__u32 retired_control_object_id;
	__u32 retired_status_object_id;
	__u64 retired_control_gpu_va;
	__u64 retired_status_gpu_va;
	__u32 report_offset;
	__u32 report_stage;
	__u32 report_stage_count;
	__u32 report_owner_state;
	__u32 report_queue_packet_count;
	__u32 report_completed_packets;
	__u32 report_dependency_active_count;
	__u32 report_queue_epoch;
	__u32 report_global_issue_ticket;
	__u32 report_global_running_ticket;
	__u32 report_global_complete_ticket;
	__u32 report_flags;
	__u32 report_config_count;
	__u32 live_offset;
	__u32 live_stage;
	__u32 live_stage_count;
	__u32 live_prepare_count;
	__u32 live_arm_count;
	__u32 live_emit_count;
	__u32 live_launch_count;
	__u32 live_dispatch_count;
	__u32 live_retired_count;
	__u32 live_context;
	__u32 live_cmd_type;
	__u32 live_owner_state;
	__u32 live_issued_packets;
	__u32 live_completed_packets;
	__u32 live_dependency_active_count;
	__u32 live_refilled_from_complete;
	__u32 live_queue_packet_count;
	__u32 live_queue_epoch;
	__u32 live_queue_base_offset;
	__u32 live_queue_size;
	__u32 live_queue_packet_size;
	__u32 live_ccb_offset;
	__u32 live_ccb_write_offset;
	__u32 live_ccb_read_offset;
	__u32 live_sync_offset;
	__u32 live_sync_complete_seq;
	__u32 live_sync_pending_seq;
	__u32 live_sync_flags;
	__u32 live_owner_offset;
	__u32 live_owner_issue_cookie;
	__u32 live_owner_complete_cookie;
	__u32 live_global_issue_ticket;
	__u32 live_global_running_ticket;
	__u32 live_global_complete_ticket;
	__u32 live_ctrl_flags;
	__u32 live_report_flags;
	__u32 live_num_ta_status_vals;
	__u32 live_num_3d_status_vals;
	__u32 live_ta_dependency;
	__u32 live_status_ta_offset;
	__u32 live_status_3d_offset;
	__u32 live_status_stride;
	__u32 live_status_value_seed;
	__u32 live_control_object_id;
	__u32 live_status_object_id;
	__u32 live_ccb_service_address;
	__u32 live_ccb_cache_control;
	__u32 live_ccb_data0;
	__u32 live_ccb_data1;
	__u32 live_control_gpu_va32;
	__u32 live_status_gpu_va32;
	__u32 live_pds_gpu_va32;
	__u32 live_use_gpu_va32;
	__u32 live_target_gpu_va32;
	__u32 live_ccb_status_gpu_va32;
	__u32 live_config_count;
	__u32 manifest_offset;
	__u32 manifest_stage;
	__u32 manifest_stage_count;
	__u32 manifest_context;
	__u32 manifest_cmd_type;
	__u32 manifest_owner_state;
	__u32 manifest_ctrl_flags;
	__u32 manifest_queue_base_offset;
	__u32 manifest_queue_size;
	__u32 manifest_queue_packet_size;
	__u32 manifest_queue_packet_count;
	__u32 manifest_queue_epoch;
	__u32 manifest_ccb_offset;
	__u32 manifest_ccb_write_offset;
	__u32 manifest_ccb_read_offset;
	__u32 manifest_dependency_active_count;
	__u32 manifest_global_issue_ticket;
	__u32 manifest_global_running_ticket;
	__u32 manifest_global_complete_ticket;
	__u32 manifest_pds_gpu_va32;
	__u32 manifest_use_gpu_va32;
	__u32 manifest_target_gpu_va32;
	__u32 manifest_status_gpu_va32;
	__u32 manifest_sync_complete_seq;
	__u32 manifest_sync_pending_seq;
	__u32 manifest_sync_flags;
	__u32 manifest_owner_issue_cookie;
	__u32 manifest_owner_complete_cookie;
	__u32 manifest_config_count;
	__u32 descriptor_offset;
	__u32 descriptor_stage;
	__u32 descriptor_stage_count;
	__u32 descriptor_context;
	__u32 descriptor_cmd_type;
	__u32 descriptor_owner_state;
	__u32 descriptor_issued_packets;
	__u32 descriptor_completed_packets;
	__u32 descriptor_dependency_active_count;
	__u32 descriptor_queue_packet_count;
	__u32 descriptor_queue_epoch;
	__u32 descriptor_ctrl_flags;
	__u32 descriptor_queue_base_offset;
	__u32 descriptor_queue_size;
	__u32 descriptor_queue_packet_size;
	__u32 descriptor_ccb_offset;
	__u32 descriptor_sync_offset;
	__u32 descriptor_owner_offset;
	__u32 descriptor_report_offset;
	__u32 descriptor_manifest_offset;
	__u32 descriptor_global_issue_ticket;
	__u32 descriptor_global_running_ticket;
	__u32 descriptor_global_complete_ticket;
	__u32 descriptor_report_flags;
	__u32 descriptor_status_ta_offset;
	__u32 descriptor_status_3d_offset;
	__u32 descriptor_status_stride;
	__u32 descriptor_status_value_seed;
	__u32 descriptor_control_gpu_va32;
	__u32 descriptor_pds_gpu_va32;
	__u32 descriptor_use_gpu_va32;
	__u32 descriptor_target_gpu_va32;
	__u32 descriptor_status_gpu_va32;
	__u32 descriptor_config_count;
	__u32 plan_offset;
	__u32 plan_stage;
	__u32 plan_stage_count;
	__u32 plan_source;
	__u32 plan_context;
	__u32 plan_cmd_type;
	__u32 plan_owner_state;
	__u32 plan_issued_packets;
	__u32 plan_completed_packets;
	__u32 plan_dependency_active_count;
	__u32 plan_refilled_from_complete;
	__u32 plan_queue_packet_count;
	__u32 plan_queue_epoch;
	__u32 plan_queue_base_offset;
	__u32 plan_queue_size;
	__u32 plan_queue_packet_size;
	__u32 plan_ccb_offset;
	__u32 plan_ccb_write_offset;
	__u32 plan_ccb_read_offset;
	__u32 plan_sync_offset;
	__u32 plan_sync_complete_seq;
	__u32 plan_sync_pending_seq;
	__u32 plan_sync_flags;
	__u32 plan_owner_offset;
	__u32 plan_owner_issue_cookie;
	__u32 plan_owner_complete_cookie;
	__u32 plan_global_issue_ticket;
	__u32 plan_global_running_ticket;
	__u32 plan_global_complete_ticket;
	__u32 plan_report_offset;
	__u32 plan_manifest_offset;
	__u32 plan_descriptor_offset;
	__u32 plan_ctrl_flags;
	__u32 plan_ccb_service_address;
	__u32 plan_ccb_cache_control;
	__u32 plan_ccb_data0;
	__u32 plan_ccb_data1;
	__u32 plan_control_gpu_va32;
	__u32 plan_pds_gpu_va32;
	__u32 plan_use_gpu_va32;
	__u32 plan_target_gpu_va32;
	__u32 plan_status_gpu_va32;
	__u32 plan_config_count;
	__u32 issue_plan_offset;
	__u32 issue_plan_stage;
	__u32 issue_plan_stage_count;
	__u32 issue_plan_valid;
	__u32 issue_plan_owner_state;
	__u32 issue_plan_issued_packets;
	__u32 issue_plan_refilled_from_complete;
	__u32 issue_plan_queue_epoch;
	__u32 issue_plan_queue_base_offset;
	__u32 issue_plan_queue_size;
	__u32 issue_plan_queue_packet_size;
	__u32 issue_plan_queue_packet_count;
	__u32 issue_plan_queue_tail_packet_count;
	__u32 issue_plan_queue_head_packet_count;
	__u32 issue_plan_ccb_offset;
	__u32 issue_plan_ccb_write_offset;
	__u32 issue_plan_ccb_read_offset;
	__u32 issue_plan_sync_offset;
	__u32 issue_plan_sync_complete_seq;
	__u32 issue_plan_sync_pending_seq;
	__u32 issue_plan_sync_flags;
	__u32 issue_plan_owner_offset;
	__u32 issue_plan_owner_issue_cookie;
	__u32 issue_plan_owner_complete_cookie;
	__u32 issue_plan_global_issue_ticket;
	__u32 issue_plan_global_running_ticket;
	__u32 issue_plan_global_complete_ticket;
	__u32 issue_plan_dependency_active_count;
	__u32 issue_plan_ccb_service_address;
	__u32 issue_plan_ccb_cache_control;
	__u32 issue_plan_ccb_data0;
	__u32 issue_plan_ccb_data1;
	__u32 issue_plan_control_object_id;
	__u32 issue_plan_status_object_id;
	__u32 issue_plan_control_gpu_va32;
	__u32 issue_plan_pds_gpu_va32;
	__u32 issue_plan_use_gpu_va32;
	__u32 issue_plan_target_gpu_va32;
	__u32 issue_plan_status_gpu_va32;
	__u32 issue_plan_config_count;
	__u32 result_offset;
	__u32 result_stage;
	__u32 result_stage_count;
	__u32 result_source;
	__u32 result_context;
	__u32 result_cmd_type;
	__u32 result_owner_state;
	__u32 result_issued_packets;
	__u32 result_completed_packets;
	__u32 result_dependency_active_count;
	__u32 result_queue_packet_count;
	__u32 result_queue_epoch;
	__u32 result_sync_complete_seq;
	__u32 result_sync_pending_seq;
	__u32 result_sync_flags;
	__u32 result_owner_issue_cookie;
	__u32 result_owner_complete_cookie;
	__u32 result_global_issue_ticket;
	__u32 result_global_running_ticket;
	__u32 result_global_complete_ticket;
	__u32 result_ctrl_flags;
	__u32 result_num_ta_status_vals;
	__u32 result_num_3d_status_vals;
	__u32 result_ta_dependency;
	__u32 result_status_ta_offset;
	__u32 result_status_3d_offset;
	__u32 result_status_stride;
	__u32 result_status_value_seed;
	__u32 result_control_object_id;
	__u32 result_status_object_id;
	__u32 result_control_gpu_va32;
	__u32 result_status_gpu_va32;
	__u32 result_report_offset;
	__u32 result_manifest_offset;
	__u32 result_descriptor_offset;
	__u32 result_plan_offset;
	__u32 result_config_count;
	__u32 issue_count;
	__u32 transition_count;
	__u32 dependency_slot_count;
	__u32 dependency_active_count;
	__u64 last_prepare_jiffies;
	__u64 last_arm_jiffies;
	__u64 last_emit_jiffies;
	__u64 last_launch_jiffies;
	__u64 last_dispatch_jiffies;
	__u64 last_retired_jiffies;
	__u64 last_live_jiffies;
	struct drm_psb_sgx_lab_dependency_query
		dependencies[DRM_PSB_SGX_LAB_DEPENDENCY_SLOTS];
	__u64 control_gpu_va;
	__u64 pds_gpu_va;
	__u64 use_gpu_va;
	__u64 target_gpu_va;
	__u64 status_gpu_va;
	__u64 last_report_jiffies;
	__u64 last_manifest_jiffies;
	__u64 last_descriptor_jiffies;
	__u64 last_plan_jiffies;
	__u64 last_issue_plan_jiffies;
	__u64 last_result_jiffies;
	__u32 reserved[1];
};

struct drm_psb_sgx_lab_coord_query {
	__u32 size;
	__u32 flags;
	__u32 focus_valid;
	__u32 focused_submit_id;
	__u32 live_submit_count;
	__u32 live_object_count;
	__u32 issue_ticket;
	__u32 running_ticket;
	__u32 complete_ticket;
	__u32 issue_submit_id;
	__u32 running_submit_id;
	__u32 complete_submit_id;
	__u32 active_running_valid;
	__u32 active_running_submit_id;
	__u32 reserved[10];
};

struct drm_psb_sgx_lab_range_query {
	__u32 size;
	__u32 flags;
	__u32 range_id;
	__u32 query_index;
	__u64 query_gpu_va;
	__u32 query_submit_match_index;
	__u32 resolved_range_id;
	__u32 resolved_index;
	__u32 range_type;
	__u32 total_range_count;
	__u32 matching_range_count;
	__u32 offset;
	__s32 in_gart;
	__u32 stolen;
	__u32 mmapping;
	__s32 npage;
	__s32 roll;
	__u32 sgx_object_id;
	__u32 sgx_object_role;
	__u32 sgx_object_class;
	__u32 sgx_object_gpu_use;
	__u32 sgx_object_expected_context;
	__u32 sgx_submit_ref_count;
	__u32 sgx_first_submit_id;
	__u32 sgx_first_submit_slot;
	__u32 sgx_resolved_submit_found;
	__u32 sgx_resolved_submit_id;
	__u32 sgx_resolved_submit_slot;
	__u32 sgx_resolved_submit_match_index;
	__u32 pin_count_total;
	__u32 unpin_count_total;
	__u32 pf_bind_count;
	__u32 pf_bind_total;
	__u32 pf_unbind_total;
	__u64 start;
	__u64 end;
	__u64 len;
	__u64 gpu_va;
	__u64 query_gpu_va_offset;
	__u32 reserved[1];
};

struct drm_psb_sgx_lab_sgx_state_query {
	__u32 size;
	__u32 flags;
	__u32 sequence;
	__u32 capture_failed;
	__u64 captured_jiffies;
	__u8 reason[DRM_PSB_SGX_LAB_SGX_REASON_LEN];
	__u32 irq_count;
	__u32 fault_count;
	__u32 vdc_irq_mask;
	__u32 gatt_start;
	__u32 mmu_gatt_start;
	__u32 gatt_free_offset;
	__u32 mmu_driver_bif_ctrl;
	__u32 mmu_needs_tlbflush;
	__u32 mmu_default_pd_context;
	__u32 core_id;
	__u32 core_revision;
	__u32 clkgatectl;
	__u32 sgx_soft_reset;
	__u32 event_status;
	__u32 event_status2;
	__u32 event_host_enable;
	__u32 event_host_enable2;
	__u32 event_kicker;
	__u32 event_kick;
	__u32 pds_exec_base;
	__u32 use_code_base0;
	__u32 use_code_base1;
	__u32 bif_ctrl;
	__u32 bif_int_stat;
	__u32 bif_fault;
	__u32 bif_dir_list_base0;
	__u32 bif_dir_list_base1;
	__u32 bif_bank0;
	__u32 bif_bank1;
	__u32 bif_twod_req_base;
	__u32 bif_3d_req_base;
	__u32 twod_socif;
	__u32 twod_blit_status;
	__u32 reserved[8];
};

struct drm_psb_sgx_lab_gtt_state_query {
	__u32 size;
	__u32 flags;
	__u32 gtt_start;
	__u32 gatt_start;
	__u32 mmu_gatt_start;
	__u32 gatt_free_offset;
	__u64 stolen_size;
	__u64 vram_stolen_size;
	__u32 mmu_last_op;
	__u32 mmu_last_num_pages;
	__s32 mmu_last_type;
	__s32 mmu_last_hw_context;
	__u64 mmu_last_address;
	__u64 mmu_last_jiffies;
	__u32 mmu_flush_count;
	__u32 mmu_set_pd_context_count;
	__u32 mmu_insert_pfn_sequence_count;
	__u32 mmu_insert_pages_count;
	__u32 mmu_remove_pfn_sequence_count;
	__u32 mmu_remove_pages_count;
	__s32 mmu_default_pd_context;
	__s32 mmu_pf_pd_context;
	__u32 range_total_count;
	__u32 range_gem_count;
	__u32 range_fb_count;
	__u32 range_cursor_count;
	__u32 range_other_count;
	__u32 range_in_gart_count;
	__u32 range_mmapping_count;
	__u32 range_sgx_object_count;
	__u32 range_pf_bound_count;
	__u32 reserved[8];
};

struct drm_psb_sgx_lab_object_query {
	__u32 size;
	__u32 flags;
	__u32 object_id;
	__u32 query_index;
	__u32 resolved_object_id;
	__u32 resolved_index;
	__u32 total_object_count;
	__u32 range_id;
	__u32 range_type;
	__u32 role;
	__u32 object_class;
	__u32 gpu_use;
	__u32 expected_context;
	__u32 offset;
	__s32 in_gart;
	__u32 stolen;
	__u32 mmapping;
	__s32 npage;
	__u32 pf_bind_count;
	__u32 pf_bind_total;
	__u32 pf_unbind_total;
	__u64 size_bytes;
	__u64 gpu_va;
	__u64 created_jiffies;
	__u64 last_pf_bind_jiffies;
	__u64 last_pf_unbind_jiffies;
	__u64 last_role_jiffies;
	__u64 last_class_jiffies;
	__u64 last_gpu_use_jiffies;
	__u64 last_context_jiffies;
	__u32 reserved[8];
};

struct drm_psb_sgx_lab_submit_op {
	__u32 size;
	__u32 flags;
	__u32 submit_id;
	__u32 resolved_submit_id;
	__u32 result_count;
	__u32 op;
	__u32 owner_state;
	__u32 packet_count;
	__u32 context;
	__u32 slot;
	__u32 object_id;
	__u32 cmd_type;
	__u32 ccb_offset;
	__u32 ctrl_flags;
	__u32 num_ta_status_vals;
	__u32 num_3d_status_vals;
	__u32 ta_dependency;
	__u32 ccb_write_offset;
	__u32 ccb_read_offset;
	__u32 ccb_service_address;
	__u32 ccb_cache_control;
	__u32 ccb_data0;
	__u32 ccb_data1;
	__u32 queue_base_offset;
	__u32 queue_size;
	__u32 queue_packet_size;
	__u32 queue_packet_count;
	__u32 status_ta_offset;
	__u32 status_3d_offset;
	__u32 status_stride;
	__u32 status_value_seed;
	__u32 sync_offset;
	__u32 sync_complete_seq;
	__u32 sync_pending_seq;
	__u32 sync_flags;
	__u32 owner_offset;
	__u32 owner_issue_cookie;
	__u32 owner_complete_cookie;
	__u32 dependency_slot;
	__u32 dependency_submit_id;
	__u32 dependency_ticket;
	__u32 dependency_offset;
	__u32 report_offset;
	__u32 live_offset;
	__u32 manifest_offset;
	__u32 descriptor_offset;
	__u32 plan_offset;
	__u32 issue_plan_offset;
	__u32 result_offset;
};

struct drm_psb_sgx_lab_object_op {
	__u32 size;
	__u32 flags;
	__u32 object_id;
	__u32 resolved_object_id;
	__u32 result_count;
	__u32 op;
	__u32 range_id;
	__u32 role;
	__u32 object_class;
	__u32 gpu_use;
	__u32 context;
	__u32 reserved[6];
};

#define DRM_IOCTL_PSB_SGX_LAB_QUERY_SUBMIT \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_SGX_LAB_QUERY_SUBMIT, \
		 struct drm_psb_sgx_lab_submit_query)
#define DRM_IOCTL_PSB_SGX_LAB_QUERY_COORD \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_SGX_LAB_QUERY_COORD, \
		 struct drm_psb_sgx_lab_coord_query)
#define DRM_IOCTL_PSB_SGX_LAB_QUERY_RANGE \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_SGX_LAB_QUERY_RANGE, \
		 struct drm_psb_sgx_lab_range_query)
#define DRM_IOCTL_PSB_SGX_LAB_QUERY_SGX_STATE \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_SGX_LAB_QUERY_SGX_STATE, \
		 struct drm_psb_sgx_lab_sgx_state_query)
#define DRM_IOCTL_PSB_SGX_LAB_QUERY_GTT_STATE \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_SGX_LAB_QUERY_GTT_STATE, \
		 struct drm_psb_sgx_lab_gtt_state_query)
#define DRM_IOCTL_PSB_SGX_LAB_QUERY_OBJECT \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_SGX_LAB_QUERY_OBJECT, \
		 struct drm_psb_sgx_lab_object_query)
#define DRM_IOCTL_PSB_SGX_LAB_SUBMIT_OP \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_SGX_LAB_SUBMIT_OP, \
		 struct drm_psb_sgx_lab_submit_op)
#define DRM_IOCTL_PSB_SGX_LAB_OBJECT_OP \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_SGX_LAB_OBJECT_OP, \
		 struct drm_psb_sgx_lab_object_op)

#endif
