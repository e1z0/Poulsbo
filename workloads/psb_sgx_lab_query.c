#include <drm/drm.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "psb_drm_lab.h"

static const char *context_name(uint32_t context)
{
	switch (context) {
	case DRM_PSB_SGX_LAB_CONTEXT_AUTO:
		return "auto";
	case DRM_PSB_SGX_LAB_CONTEXT_DEFAULT:
		return "default";
	case DRM_PSB_SGX_LAB_CONTEXT_PF:
		return "pf";
	default:
		return "unknown";
	}
}

static const char *cmd_type_name(uint32_t cmd_type)
{
	switch (cmd_type) {
	case DRM_PSB_SGX_LAB_CMD_INVALID:
		return "invalid";
	case DRM_PSB_SGX_LAB_CMD_TA:
		return "ta";
	case DRM_PSB_SGX_LAB_CMD_TRANSFER:
		return "transfer";
	case DRM_PSB_SGX_LAB_CMD_2D:
		return "2d";
	case DRM_PSB_SGX_LAB_CMD_POWER:
		return "power";
	case DRM_PSB_SGX_LAB_CMD_CLEANUP:
		return "cleanup";
	case DRM_PSB_SGX_LAB_CMD_GETMISCINFO:
		return "getmiscinfo";
	case DRM_PSB_SGX_LAB_CMD_PROCESS_QUEUES:
		return "process_queues";
	default:
		return "unknown";
	}
}

static const char *owner_state_name(uint32_t state)
{
	switch (state) {
	case DRM_PSB_SGX_LAB_OWNER_NONE:
		return "none";
	case DRM_PSB_SGX_LAB_OWNER_QUEUED:
		return "queued";
	case DRM_PSB_SGX_LAB_OWNER_RUNNING:
		return "running";
	case DRM_PSB_SGX_LAB_OWNER_COMPLETE:
		return "complete";
	default:
		return "unknown";
	}
}

static const char *report_stage_name(uint32_t stage)
{
	switch (stage) {
	case DRM_PSB_SGX_LAB_REPORT_STAGE_NONE:
		return "none";
	case DRM_PSB_SGX_LAB_REPORT_STAGE_PREPARED:
		return "prepared";
	case DRM_PSB_SGX_LAB_REPORT_STAGE_ARMED:
		return "armed";
	case DRM_PSB_SGX_LAB_REPORT_STAGE_EMITTED:
		return "emitted";
	case DRM_PSB_SGX_LAB_REPORT_STAGE_LAUNCHED:
		return "launched";
	case DRM_PSB_SGX_LAB_REPORT_STAGE_DISPATCHED:
		return "dispatched";
	case DRM_PSB_SGX_LAB_REPORT_STAGE_RETIRED:
		return "retired";
	default:
		return "unknown";
	}
}

static const char *plan_source_name(uint32_t source)
{
	switch (source) {
	case DRM_PSB_SGX_LAB_PLAN_SOURCE_NONE:
		return "none";
	case DRM_PSB_SGX_LAB_PLAN_SOURCE_LIVE:
		return "live";
	case DRM_PSB_SGX_LAB_PLAN_SOURCE_LAUNCH:
		return "launch";
	case DRM_PSB_SGX_LAB_PLAN_SOURCE_DISPATCH:
		return "dispatch";
	case DRM_PSB_SGX_LAB_PLAN_SOURCE_RETIRED:
		return "retired";
	default:
		return "unknown";
	}
}

static const char *result_source_name(uint32_t source)
{
	switch (source) {
	case DRM_PSB_SGX_LAB_RESULT_SOURCE_NONE:
		return "none";
	case DRM_PSB_SGX_LAB_RESULT_SOURCE_LIVE:
		return "live";
	case DRM_PSB_SGX_LAB_RESULT_SOURCE_DISPATCH:
		return "dispatch";
	case DRM_PSB_SGX_LAB_RESULT_SOURCE_RETIRED:
		return "retired";
	default:
		return "unknown";
	}
}

static const char *slot_name(uint32_t slot)
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
		return "none";
	}
}

static const char *role_name(uint32_t role)
{
	switch (role) {
	case DRM_PSB_SGX_LAB_ROLE_PDS:
		return "pds";
	case DRM_PSB_SGX_LAB_ROLE_USE:
		return "use";
	case DRM_PSB_SGX_LAB_ROLE_CONTROL:
		return "control";
	case DRM_PSB_SGX_LAB_ROLE_TARGET:
		return "target";
	case DRM_PSB_SGX_LAB_ROLE_STATUS:
		return "status";
	case DRM_PSB_SGX_LAB_ROLE_GENERIC:
	default:
		return "generic";
	}
}

static const char *class_name(uint32_t object_class)
{
	switch (object_class) {
	case DRM_PSB_SGX_LAB_CLASS_PACKET:
		return "packet";
	case DRM_PSB_SGX_LAB_CLASS_CODE:
		return "code";
	case DRM_PSB_SGX_LAB_CLASS_STATE:
		return "state";
	case DRM_PSB_SGX_LAB_CLASS_SURFACE:
		return "surface";
	case DRM_PSB_SGX_LAB_CLASS_GENERIC:
	default:
		return "generic";
	}
}

static const char *gpu_use_name(uint32_t gpu_use)
{
	switch (gpu_use) {
	case DRM_PSB_SGX_LAB_GPU_USE_CONTROL_CCB:
		return "control_ccb";
	case DRM_PSB_SGX_LAB_GPU_USE_PDS_CODE:
		return "pds_code";
	case DRM_PSB_SGX_LAB_GPU_USE_USE_CODE:
		return "use_code";
	case DRM_PSB_SGX_LAB_GPU_USE_RENDER_TARGET:
		return "render_target";
	case DRM_PSB_SGX_LAB_GPU_USE_STATUS_BLOCK:
		return "status_block";
	case DRM_PSB_SGX_LAB_GPU_USE_GENERIC:
	default:
		return "generic";
	}
}

static const char *range_type_name(uint32_t range_type)
{
	switch (range_type) {
	case DRM_PSB_SGX_LAB_RANGE_GEM:
		return "gem";
	case DRM_PSB_SGX_LAB_RANGE_FB:
		return "fb";
	case DRM_PSB_SGX_LAB_RANGE_CURSOR:
		return "cursor";
	case DRM_PSB_SGX_LAB_RANGE_OTHER:
		return "other";
	case DRM_PSB_SGX_LAB_RANGE_UNKNOWN:
	default:
		return "unknown";
	}
}

static void print_dep_flags(uint32_t flags)
{
	int first = 1;

	printf("dep_flags_names=");
	if (flags & DRM_PSB_SGX_LAB_DEP_FLAG_READY) {
		printf("%sready", first ? "" : ",");
		first = 0;
	}
	if (flags & DRM_PSB_SGX_LAB_DEP_FLAG_SOURCE_BOUND) {
		printf("%ssource_bound", first ? "" : ",");
		first = 0;
	}
	if (flags & DRM_PSB_SGX_LAB_DEP_FLAG_SATISFIED) {
		printf("%ssatisfied", first ? "" : ",");
		first = 0;
	}
	if (flags & DRM_PSB_SGX_LAB_DEP_FLAG_GATED) {
		printf("%sgated", first ? "" : ",");
		first = 0;
	}
	if (flags & DRM_PSB_SGX_LAB_DEP_FLAG_SOURCE_MISSING) {
		printf("%ssource_missing", first ? "" : ",");
		first = 0;
	}
	if (first)
		printf("none");
	printf("\n");
}

static int parse_submit_selector(const char *arg,
				 const char *next_arg,
				 const char *next_arg2,
				 struct drm_psb_sgx_lab_submit_query *query)
{
	if (strcmp(arg, "focus") == 0) {
		query->flags = DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_FOCUS;
		return 0;
	}

	if (strcmp(arg, "index") == 0) {
		char *end = NULL;
		unsigned long value;

		if (!next_arg)
			return -1;
		errno = 0;
		value = strtoul(next_arg, &end, 0);
		if (errno || !end || *end != '\0' || value > UINT32_MAX)
			return -1;
		query->flags = DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_INDEX;
		query->query_index = (uint32_t)value;
		return 1;
	}

	if (strcmp(arg, "gpuva") == 0) {
		char *end = NULL;
		unsigned long long value;

		if (!next_arg)
			return -1;
		errno = 0;
		value = strtoull(next_arg, &end, 0);
		if (errno || !end || *end != '\0')
			return -1;
		query->flags = DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_GPU_VA;
		query->query_gpu_va = (uint64_t)value;
		return 1;
	}

	if (strcmp(arg, "gpuva-match") == 0) {
		char *end = NULL;
		unsigned long long value;
		unsigned long match_index;

		if (!next_arg || !next_arg2)
			return -1;
		errno = 0;
		value = strtoull(next_arg, &end, 0);
		if (errno || !end || *end != '\0')
			return -1;
		errno = 0;
		match_index = strtoul(next_arg2, &end, 0);
		if (errno || !end || *end != '\0' || match_index > UINT32_MAX)
			return -1;
		query->flags = DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_GPU_VA |
			       DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_GPU_VA_MATCH_INDEX;
		query->query_gpu_va = (uint64_t)value;
		query->query_match_index = (uint32_t)match_index;
		return 2;
	}

	{
		char *end = NULL;
		unsigned long value;

		errno = 0;
		value = strtoul(arg, &end, 0);
		if (errno || !end || *end != '\0' || value > UINT32_MAX)
			return -1;
		query->submit_id = (uint32_t)value;
	}

	return 0;
}

int main(int argc, char **argv)
{
	const char *path = argc > 1 ? argv[1] : "/dev/dri/card0";
	const char *selector = argc > 2 ? argv[2] : "focus";
	const char *selector_arg = argc > 3 ? argv[3] : NULL;
	const char *selector_arg2 = argc > 4 ? argv[4] : NULL;
	struct drm_psb_sgx_lab_coord_query coord;
	struct drm_psb_sgx_lab_submit_query submit;
	int coord_only = 0;
	int fd;
	int ret;
	int selector_parse_ret;
	uint32_t i;

	fd = open(path, O_RDWR);
	printf("drm_path=%s\n", path);
	printf("drm_open_result=%d\n", fd);
	if (fd < 0) {
		printf("drm_open_errno=%d\n", errno);
		return 1;
	}

	memset(&coord, 0, sizeof(coord));
	coord.size = sizeof(coord);
	errno = 0;
	/*
	 * Use raw ioctl() so custom lab-query error returns are preserved. The
	 * object and submit mutation helpers already avoid libdrm retry behavior;
	 * keep the typed query helpers aligned with that lab-only contract.
	 */
	ret = ioctl(fd, DRM_IOCTL_PSB_SGX_LAB_QUERY_COORD, &coord);
	printf("coord_ioctl_ret=%d errno=%d\n", ret, errno);
	if (ret == 0) {
		printf("coord_focus_valid=%u\n", coord.focus_valid);
		printf("coord_focused_submit_id=%u\n", coord.focused_submit_id);
		printf("coord_live_submit_count=%u\n", coord.live_submit_count);
		printf("coord_live_object_count=%u\n", coord.live_object_count);
		printf("coord_issue_ticket=%u\n", coord.issue_ticket);
		printf("coord_running_ticket=%u\n", coord.running_ticket);
		printf("coord_complete_ticket=%u\n", coord.complete_ticket);
		printf("coord_issue_submit_id=%u\n", coord.issue_submit_id);
		printf("coord_running_submit_id=%u\n", coord.running_submit_id);
		printf("coord_complete_submit_id=%u\n", coord.complete_submit_id);
		printf("coord_active_running_valid=%u\n",
		       coord.active_running_valid);
		printf("coord_active_running_submit_id=%u\n",
		       coord.active_running_submit_id);
	}

	if (strcmp(selector, "coord") == 0) {
		printf("submit_selector=coord\n");
		printf("submit_query_skipped=1\n");
		close(fd);
		return ret ? 3 : 0;
	}

	memset(&submit, 0, sizeof(submit));
	submit.size = sizeof(submit);
	selector_parse_ret = parse_submit_selector(selector, selector_arg,
						       selector_arg2,
						       &submit);
	if (selector_parse_ret < 0) {
		fprintf(stderr, "invalid submit selector: %s%s%s\n", selector,
			selector_arg ? " " : "",
			selector_arg ? selector_arg : "");
		close(fd);
		return 2;
	}

	errno = 0;
	ret = ioctl(fd, DRM_IOCTL_PSB_SGX_LAB_QUERY_SUBMIT, &submit);
	printf("submit_selector=%s\n", selector);
	printf("submit_query_skipped=%u\n", coord_only);
	if (submit.flags & DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_INDEX)
		printf("submit_query_index=%u\n", submit.query_index);
	if (submit.flags & DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_GPU_VA)
		printf("submit_query_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.query_gpu_va);
	if (submit.flags & DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_GPU_VA_MATCH_INDEX)
		printf("submit_query_match_index=%u\n",
		       submit.query_match_index);
	printf("submit_ioctl_ret=%d errno=%d\n", ret, errno);
	if (ret == 0) {
		printf("submit_resolved_id=%u\n", submit.resolved_submit_id);
		printf("submit_resolved_index=%u\n", submit.resolved_index);
		printf("submit_total_count=%u\n", submit.total_submit_count);
		printf("submit_matching_submit_count=%u\n",
		       submit.matching_submit_count);
		printf("submit_resolved_match_index=%u\n",
		       submit.resolved_match_index);
		printf("submit_resolved_slot=%u\n", submit.resolved_slot);
		printf("submit_resolved_slot_name=%s\n",
		       slot_name(submit.resolved_slot));
		printf("submit_resolved_object_id=%u\n",
		       submit.resolved_object_id);
		printf("submit_resolved_range_id=%u\n",
		       submit.resolved_range_id);
		printf("submit_resolved_range_type=%u\n",
		       submit.resolved_range_type);
		printf("submit_resolved_range_type_name=%s\n",
		       range_type_name(submit.resolved_range_type));
		printf("submit_resolved_object_role=%u\n",
		       submit.resolved_object_role);
		printf("submit_resolved_object_role_name=%s\n",
		       role_name(submit.resolved_object_role));
		printf("submit_resolved_object_class=%u\n",
		       submit.resolved_object_class);
		printf("submit_resolved_object_class_name=%s\n",
		       class_name(submit.resolved_object_class));
		printf("submit_resolved_object_gpu_use=%u\n",
		       submit.resolved_object_gpu_use);
		printf("submit_resolved_object_gpu_use_name=%s\n",
		       gpu_use_name(submit.resolved_object_gpu_use));
		printf("submit_resolved_object_expected_context=%u\n",
		       submit.resolved_object_expected_context);
		printf("submit_resolved_object_expected_context_name=%s\n",
		       context_name(submit.resolved_object_expected_context));
		printf("submit_query_gpu_va_offset=0x%08" PRIx64 "\n",
		       (uint64_t)submit.query_gpu_va_offset);
		printf("submit_context=%u\n", submit.context);
		printf("submit_context_name=%s\n",
		       context_name(submit.context));
		printf("submit_cmd_type=%u\n", submit.cmd_type);
		printf("submit_cmd_type_name=%s\n",
		       cmd_type_name(submit.cmd_type));
		printf("submit_owner_state=%u\n", submit.owner_state);
		printf("submit_owner_state_name=%s\n",
		       owner_state_name(submit.owner_state));
		printf("submit_last_validate_status=%d\n",
		       submit.last_validate_status);
		printf("submit_validate_count=%u\n", submit.validate_count);
		printf("submit_prepare_count=%u\n", submit.prepare_count);
		printf("submit_arm_valid=%u\n", submit.arm_valid);
		printf("submit_arm_count=%u\n", submit.arm_count);
		printf("submit_arm_prepare_count=%u\n",
		       submit.arm_prepare_count);
		printf("submit_arm_queue_packet_count=%u\n",
		       submit.arm_queue_packet_count);
		printf("submit_arm_queue_tail_packet_count=%u\n",
		       submit.arm_queue_tail_packet_count);
		printf("submit_arm_queue_head_packet_count=%u\n",
		       submit.arm_queue_head_packet_count);
		printf("submit_arm_ccb_read_offset=0x%x\n",
		       submit.arm_ccb_read_offset);
		printf("submit_arm_ccb_write_offset=0x%x\n",
		       submit.arm_ccb_write_offset);
		printf("submit_arm_sync_complete_seq=0x%x\n",
		       submit.arm_sync_complete_seq);
		printf("submit_arm_sync_pending_seq=0x%x\n",
		       submit.arm_sync_pending_seq);
		printf("submit_arm_owner_state=%u\n", submit.arm_owner_state);
		printf("submit_arm_owner_state_name=%s\n",
		       owner_state_name(submit.arm_owner_state));
		printf("submit_arm_owner_issue_cookie=0x%x\n",
		       submit.arm_owner_issue_cookie);
		printf("submit_arm_owner_complete_cookie=0x%x\n",
		       submit.arm_owner_complete_cookie);
		printf("submit_arm_queue_epoch=%u\n", submit.arm_queue_epoch);
		printf("submit_arm_global_issue_ticket=%u\n",
		       submit.arm_global_issue_ticket);
		printf("submit_emit_valid=%u\n", submit.emit_valid);
		printf("submit_emit_count=%u\n", submit.emit_count);
		printf("submit_emit_prepare_count=%u\n",
		       submit.emit_prepare_count);
		printf("submit_emit_arm_count=%u\n", submit.emit_arm_count);
		printf("submit_emit_context=%u\n", submit.emit_context);
		printf("submit_emit_context_name=%s\n",
		       context_name(submit.emit_context));
		printf("submit_emit_cmd_type=%u\n", submit.emit_cmd_type);
		printf("submit_emit_cmd_type_name=%s\n",
		       cmd_type_name(submit.emit_cmd_type));
		printf("submit_emit_owner_state=%u\n", submit.emit_owner_state);
		printf("submit_emit_owner_state_name=%s\n",
		       owner_state_name(submit.emit_owner_state));
		printf("submit_emit_ctrl_flags=0x%x\n", submit.emit_ctrl_flags);
		printf("submit_emit_num_ta_status_vals=%u\n",
		       submit.emit_num_ta_status_vals);
		printf("submit_emit_num_3d_status_vals=%u\n",
		       submit.emit_num_3d_status_vals);
		printf("submit_emit_ta_dependency=%u\n",
		       submit.emit_ta_dependency);
		printf("submit_emit_control_object_id=%u\n",
		       submit.emit_control_object_id);
		printf("submit_emit_pds_object_id=%u\n",
		       submit.emit_pds_object_id);
		printf("submit_emit_use_object_id=%u\n",
		       submit.emit_use_object_id);
		printf("submit_emit_target_object_id=%u\n",
		       submit.emit_target_object_id);
		printf("submit_emit_status_object_id=%u\n",
		       submit.emit_status_object_id);
		printf("submit_emit_ccb_offset=0x%x\n", submit.emit_ccb_offset);
		printf("submit_emit_ccb_write_offset=0x%x\n",
		       submit.emit_ccb_write_offset);
		printf("submit_emit_ccb_read_offset=0x%x\n",
		       submit.emit_ccb_read_offset);
		printf("submit_emit_ccb_service_address=0x%x\n",
		       submit.emit_ccb_service_address);
		printf("submit_emit_ccb_cache_control=0x%x\n",
		       submit.emit_ccb_cache_control);
		printf("submit_emit_ccb_data0=0x%x\n", submit.emit_ccb_data0);
		printf("submit_emit_ccb_data1=0x%x\n", submit.emit_ccb_data1);
		printf("submit_emit_queue_base_offset=0x%x\n",
		       submit.emit_queue_base_offset);
		printf("submit_emit_queue_size=0x%x\n", submit.emit_queue_size);
		printf("submit_emit_queue_packet_size=0x%x\n",
		       submit.emit_queue_packet_size);
		printf("submit_emit_queue_packet_count=%u\n",
		       submit.emit_queue_packet_count);
		printf("submit_emit_queue_tail_packet_count=%u\n",
		       submit.emit_queue_tail_packet_count);
		printf("submit_emit_queue_head_packet_count=%u\n",
		       submit.emit_queue_head_packet_count);
		printf("submit_emit_sync_complete_seq=0x%x\n",
		       submit.emit_sync_complete_seq);
		printf("submit_emit_sync_pending_seq=0x%x\n",
		       submit.emit_sync_pending_seq);
		printf("submit_emit_sync_flags=0x%x\n", submit.emit_sync_flags);
		printf("submit_emit_owner_issue_cookie=0x%x\n",
		       submit.emit_owner_issue_cookie);
		printf("submit_emit_owner_complete_cookie=0x%x\n",
		       submit.emit_owner_complete_cookie);
		printf("submit_emit_queue_epoch=%u\n",
		       submit.emit_queue_epoch);
		printf("submit_emit_global_issue_ticket=%u\n",
		       submit.emit_global_issue_ticket);
		printf("submit_emit_dependency_active_count=%u\n",
		       submit.emit_dependency_active_count);
		printf("submit_emit_control_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.emit_control_gpu_va);
		printf("submit_emit_pds_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.emit_pds_gpu_va);
		printf("submit_emit_use_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.emit_use_gpu_va);
		printf("submit_emit_target_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.emit_target_gpu_va);
		printf("submit_emit_status_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.emit_status_gpu_va);
		printf("submit_launch_valid=%u\n", submit.launch_valid);
		printf("submit_launch_consumed=%u\n",
		       submit.launch_consumed);
		printf("submit_launch_count=%u\n", submit.launch_count);
		printf("submit_launch_prepare_count=%u\n",
		       submit.launch_prepare_count);
		printf("submit_launch_arm_count=%u\n",
		       submit.launch_arm_count);
		printf("submit_launch_emit_count=%u\n",
		       submit.launch_emit_count);
		printf("submit_launch_context=%u\n", submit.launch_context);
		printf("submit_launch_context_name=%s\n",
		       context_name(submit.launch_context));
		printf("submit_launch_cmd_type=%u\n",
		       submit.launch_cmd_type);
		printf("submit_launch_cmd_type_name=%s\n",
		       cmd_type_name(submit.launch_cmd_type));
		printf("submit_launch_owner_state=%u\n",
		       submit.launch_owner_state);
		printf("submit_launch_owner_state_name=%s\n",
		       owner_state_name(submit.launch_owner_state));
		printf("submit_launch_issued_packets=%u\n",
		       submit.launch_issued_packets);
		printf("submit_launch_refilled_from_complete=%u\n",
		       submit.launch_refilled_from_complete);
		printf("submit_launch_ctrl_flags=0x%x\n",
		       submit.launch_ctrl_flags);
		printf("submit_launch_num_ta_status_vals=%u\n",
		       submit.launch_num_ta_status_vals);
		printf("submit_launch_num_3d_status_vals=%u\n",
		       submit.launch_num_3d_status_vals);
		printf("submit_launch_ta_dependency=%u\n",
		       submit.launch_ta_dependency);
		printf("submit_launch_control_object_id=%u\n",
		       submit.launch_control_object_id);
		printf("submit_launch_pds_object_id=%u\n",
		       submit.launch_pds_object_id);
		printf("submit_launch_use_object_id=%u\n",
		       submit.launch_use_object_id);
		printf("submit_launch_target_object_id=%u\n",
		       submit.launch_target_object_id);
		printf("submit_launch_status_object_id=%u\n",
		       submit.launch_status_object_id);
		printf("submit_launch_ccb_offset=0x%x\n",
		       submit.launch_ccb_offset);
		printf("submit_launch_ccb_write_offset=0x%x\n",
		       submit.launch_ccb_write_offset);
		printf("submit_launch_ccb_read_offset=0x%x\n",
		       submit.launch_ccb_read_offset);
		printf("submit_launch_new_write_offset=0x%x\n",
		       submit.launch_new_write_offset);
		printf("submit_launch_ccb_service_address=0x%x\n",
		       submit.launch_ccb_service_address);
		printf("submit_launch_ccb_cache_control=0x%x\n",
		       submit.launch_ccb_cache_control);
		printf("submit_launch_ccb_data0=0x%x\n",
		       submit.launch_ccb_data0);
		printf("submit_launch_ccb_data1=0x%x\n",
		       submit.launch_ccb_data1);
		printf("submit_launch_queue_base_offset=0x%x\n",
		       submit.launch_queue_base_offset);
		printf("submit_launch_queue_size=0x%x\n",
		       submit.launch_queue_size);
		printf("submit_launch_queue_packet_size=0x%x\n",
		       submit.launch_queue_packet_size);
		printf("submit_launch_queue_packet_count=%u\n",
		       submit.launch_queue_packet_count);
		printf("submit_launch_queue_tail_packet_count=%u\n",
		       submit.launch_queue_tail_packet_count);
		printf("submit_launch_queue_head_packet_count=%u\n",
		       submit.launch_queue_head_packet_count);
		printf("submit_launch_sync_offset=0x%x\n",
		       submit.launch_sync_offset);
		printf("submit_launch_sync_complete_seq=0x%x\n",
		       submit.launch_sync_complete_seq);
		printf("submit_launch_sync_pending_seq=0x%x\n",
		       submit.launch_sync_pending_seq);
		printf("submit_launch_sync_flags=0x%x\n",
		       submit.launch_sync_flags);
		printf("submit_launch_owner_offset=0x%x\n",
		       submit.launch_owner_offset);
		printf("submit_launch_owner_issue_cookie=0x%x\n",
		       submit.launch_owner_issue_cookie);
		printf("submit_launch_owner_complete_cookie=0x%x\n",
		       submit.launch_owner_complete_cookie);
		printf("submit_launch_queue_epoch=%u\n",
		       submit.launch_queue_epoch);
		printf("submit_launch_global_issue_ticket=%u\n",
		       submit.launch_global_issue_ticket);
		printf("submit_launch_global_running_ticket=%u\n",
		       submit.launch_global_running_ticket);
		printf("submit_launch_global_complete_ticket=%u\n",
		       submit.launch_global_complete_ticket);
		printf("submit_launch_dependency_active_count=%u\n",
		       submit.launch_dependency_active_count);
		printf("submit_launch_control_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.launch_control_gpu_va);
		printf("submit_launch_pds_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.launch_pds_gpu_va);
		printf("submit_launch_use_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.launch_use_gpu_va);
		printf("submit_launch_target_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.launch_target_gpu_va);
		printf("submit_launch_status_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.launch_status_gpu_va);
		printf("submit_dispatch_valid=%u\n", submit.dispatch_valid);
		printf("submit_dispatch_count=%u\n", submit.dispatch_count);
		printf("submit_dispatch_prepare_count=%u\n",
		       submit.dispatch_prepare_count);
		printf("submit_dispatch_arm_count=%u\n",
		       submit.dispatch_arm_count);
		printf("submit_dispatch_emit_count=%u\n",
		       submit.dispatch_emit_count);
		printf("submit_dispatch_launch_count=%u\n",
		       submit.dispatch_launch_count);
		printf("submit_dispatch_context=%u\n", submit.dispatch_context);
		printf("submit_dispatch_context_name=%s\n",
		       context_name(submit.dispatch_context));
		printf("submit_dispatch_cmd_type=%u\n", submit.dispatch_cmd_type);
		printf("submit_dispatch_cmd_type_name=%s\n",
		       cmd_type_name(submit.dispatch_cmd_type));
		printf("submit_dispatch_owner_state=%u\n",
		       submit.dispatch_owner_state);
		printf("submit_dispatch_owner_state_name=%s\n",
		       owner_state_name(submit.dispatch_owner_state));
		printf("submit_dispatch_issued_packets=%u\n",
		       submit.dispatch_issued_packets);
		printf("submit_dispatch_completed_packets=%u\n",
		       submit.dispatch_completed_packets);
		printf("submit_dispatch_refilled_from_complete=%u\n",
		       submit.dispatch_refilled_from_complete);
		printf("submit_dispatch_ctrl_flags=0x%x\n",
		       submit.dispatch_ctrl_flags);
		printf("submit_dispatch_num_ta_status_vals=%u\n",
		       submit.dispatch_num_ta_status_vals);
		printf("submit_dispatch_num_3d_status_vals=%u\n",
		       submit.dispatch_num_3d_status_vals);
		printf("submit_dispatch_ta_dependency=%u\n",
		       submit.dispatch_ta_dependency);
		printf("submit_dispatch_control_object_id=%u\n",
		       submit.dispatch_control_object_id);
		printf("submit_dispatch_pds_object_id=%u\n",
		       submit.dispatch_pds_object_id);
		printf("submit_dispatch_use_object_id=%u\n",
		       submit.dispatch_use_object_id);
		printf("submit_dispatch_target_object_id=%u\n",
		       submit.dispatch_target_object_id);
		printf("submit_dispatch_status_object_id=%u\n",
		       submit.dispatch_status_object_id);
		printf("submit_dispatch_ccb_offset=0x%x\n",
		       submit.dispatch_ccb_offset);
		printf("submit_dispatch_ccb_write_offset=0x%x\n",
		       submit.dispatch_ccb_write_offset);
		printf("submit_dispatch_ccb_read_offset=0x%x\n",
		       submit.dispatch_ccb_read_offset);
		printf("submit_dispatch_ccb_service_address=0x%x\n",
		       submit.dispatch_ccb_service_address);
		printf("submit_dispatch_ccb_cache_control=0x%x\n",
		       submit.dispatch_ccb_cache_control);
		printf("submit_dispatch_ccb_data0=0x%x\n",
		       submit.dispatch_ccb_data0);
		printf("submit_dispatch_ccb_data1=0x%x\n",
		       submit.dispatch_ccb_data1);
		printf("submit_dispatch_queue_base_offset=0x%x\n",
		       submit.dispatch_queue_base_offset);
		printf("submit_dispatch_queue_size=0x%x\n",
		       submit.dispatch_queue_size);
		printf("submit_dispatch_queue_packet_size=0x%x\n",
		       submit.dispatch_queue_packet_size);
		printf("submit_dispatch_queue_packet_count=%u\n",
		       submit.dispatch_queue_packet_count);
		printf("submit_dispatch_queue_tail_packet_count=%u\n",
		       submit.dispatch_queue_tail_packet_count);
		printf("submit_dispatch_queue_head_packet_count=%u\n",
		       submit.dispatch_queue_head_packet_count);
		printf("submit_dispatch_sync_offset=0x%x\n",
		       submit.dispatch_sync_offset);
		printf("submit_dispatch_sync_complete_seq=0x%x\n",
		       submit.dispatch_sync_complete_seq);
		printf("submit_dispatch_sync_pending_seq=0x%x\n",
		       submit.dispatch_sync_pending_seq);
		printf("submit_dispatch_sync_flags=0x%x\n",
		       submit.dispatch_sync_flags);
		printf("submit_dispatch_owner_offset=0x%x\n",
		       submit.dispatch_owner_offset);
		printf("submit_dispatch_owner_issue_cookie=0x%x\n",
		       submit.dispatch_owner_issue_cookie);
		printf("submit_dispatch_owner_complete_cookie=0x%x\n",
		       submit.dispatch_owner_complete_cookie);
		printf("submit_dispatch_queue_epoch=%u\n",
		       submit.dispatch_queue_epoch);
		printf("submit_dispatch_global_issue_ticket=%u\n",
		       submit.dispatch_global_issue_ticket);
		printf("submit_dispatch_global_running_ticket=%u\n",
		       submit.dispatch_global_running_ticket);
		printf("submit_dispatch_global_complete_ticket=%u\n",
		       submit.dispatch_global_complete_ticket);
		printf("submit_dispatch_dependency_active_count=%u\n",
		       submit.dispatch_dependency_active_count);
		printf("submit_dispatch_control_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.dispatch_control_gpu_va);
		printf("submit_dispatch_pds_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.dispatch_pds_gpu_va);
		printf("submit_dispatch_use_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.dispatch_use_gpu_va);
		printf("submit_dispatch_target_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.dispatch_target_gpu_va);
		printf("submit_dispatch_status_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.dispatch_status_gpu_va);
		printf("submit_retired_valid=%u\n", submit.retired_valid);
		printf("submit_retired_count=%u\n", submit.retired_count);
		printf("submit_retired_prepare_count=%u\n",
		       submit.retired_prepare_count);
		printf("submit_retired_arm_count=%u\n", submit.retired_arm_count);
		printf("submit_retired_emit_count=%u\n",
		       submit.retired_emit_count);
		printf("submit_retired_launch_count=%u\n",
		       submit.retired_launch_count);
		printf("submit_retired_dispatch_count=%u\n",
		       submit.retired_dispatch_count);
		printf("submit_retired_context=%u\n", submit.retired_context);
		printf("submit_retired_context_name=%s\n",
		       context_name(submit.retired_context));
		printf("submit_retired_cmd_type=%u\n", submit.retired_cmd_type);
		printf("submit_retired_cmd_type_name=%s\n",
		       cmd_type_name(submit.retired_cmd_type));
		printf("submit_retired_owner_state=%u\n",
		       submit.retired_owner_state);
		printf("submit_retired_owner_state_name=%s\n",
		       owner_state_name(submit.retired_owner_state));
		printf("submit_retired_issued_packets=%u\n",
		       submit.retired_issued_packets);
		printf("submit_retired_completed_packets=%u\n",
		       submit.retired_completed_packets);
		printf("submit_retired_refilled_from_complete=%u\n",
		       submit.retired_refilled_from_complete);
		printf("submit_retired_queue_packet_count=%u\n",
		       submit.retired_queue_packet_count);
		printf("submit_retired_sync_complete_seq=0x%x\n",
		       submit.retired_sync_complete_seq);
		printf("submit_retired_sync_pending_seq=0x%x\n",
		       submit.retired_sync_pending_seq);
		printf("submit_retired_owner_issue_cookie=0x%x\n",
		       submit.retired_owner_issue_cookie);
		printf("submit_retired_owner_complete_cookie=0x%x\n",
		       submit.retired_owner_complete_cookie);
		printf("submit_retired_queue_epoch=%u\n",
		       submit.retired_queue_epoch);
		printf("submit_retired_global_issue_ticket=%u\n",
		       submit.retired_global_issue_ticket);
		printf("submit_retired_global_running_ticket=%u\n",
		       submit.retired_global_running_ticket);
		printf("submit_retired_global_complete_ticket=%u\n",
		       submit.retired_global_complete_ticket);
		printf("submit_retired_dependency_active_count=%u\n",
		       submit.retired_dependency_active_count);
		printf("submit_retired_control_object_id=%u\n",
		       submit.retired_control_object_id);
		printf("submit_retired_status_object_id=%u\n",
		       submit.retired_status_object_id);
		printf("submit_retired_control_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.retired_control_gpu_va);
		printf("submit_retired_status_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.retired_status_gpu_va);
		printf("submit_report_offset=0x%x\n", submit.report_offset);
		printf("submit_report_stage=%u\n", submit.report_stage);
		printf("submit_report_stage_name=%s\n",
		       report_stage_name(submit.report_stage));
		printf("submit_report_stage_count=%u\n",
		       submit.report_stage_count);
		printf("submit_report_owner_state=%u\n",
		       submit.report_owner_state);
		printf("submit_report_owner_state_name=%s\n",
		       owner_state_name(submit.report_owner_state));
		printf("submit_report_queue_packet_count=%u\n",
		       submit.report_queue_packet_count);
		printf("submit_report_completed_packets=%u\n",
		       submit.report_completed_packets);
		printf("submit_report_dependency_active_count=%u\n",
		       submit.report_dependency_active_count);
		printf("submit_report_queue_epoch=%u\n",
		       submit.report_queue_epoch);
		printf("submit_report_global_issue_ticket=%u\n",
		       submit.report_global_issue_ticket);
		printf("submit_report_global_running_ticket=%u\n",
		       submit.report_global_running_ticket);
		printf("submit_report_global_complete_ticket=%u\n",
		       submit.report_global_complete_ticket);
		printf("submit_report_flags=0x%x\n", submit.report_flags);
		printf("submit_report_config_count=%u\n",
		       submit.report_config_count);
		printf("submit_result_offset=0x%x\n", submit.result_offset);
		printf("submit_result_stage=%u\n", submit.result_stage);
		printf("submit_result_stage_name=%s\n",
		       report_stage_name(submit.result_stage));
		printf("submit_result_stage_count=%u\n",
		       submit.result_stage_count);
		printf("submit_result_source=%u\n", submit.result_source);
		printf("submit_result_source_name=%s\n",
		       result_source_name(submit.result_source));
		printf("submit_result_context=%u\n", submit.result_context);
		printf("submit_result_context_name=%s\n",
		       context_name(submit.result_context));
		printf("submit_result_cmd_type=%u\n", submit.result_cmd_type);
		printf("submit_result_cmd_type_name=%s\n",
		       cmd_type_name(submit.result_cmd_type));
		printf("submit_result_owner_state=%u\n",
		       submit.result_owner_state);
		printf("submit_result_owner_state_name=%s\n",
		       owner_state_name(submit.result_owner_state));
		printf("submit_result_issued_packets=%u\n",
		       submit.result_issued_packets);
		printf("submit_result_completed_packets=%u\n",
		       submit.result_completed_packets);
		printf("submit_result_dependency_active_count=%u\n",
		       submit.result_dependency_active_count);
		printf("submit_result_queue_packet_count=%u\n",
		       submit.result_queue_packet_count);
		printf("submit_result_queue_epoch=%u\n",
		       submit.result_queue_epoch);
		printf("submit_result_sync_complete_seq=0x%x\n",
		       submit.result_sync_complete_seq);
		printf("submit_result_sync_pending_seq=0x%x\n",
		       submit.result_sync_pending_seq);
		printf("submit_result_sync_flags=0x%x\n",
		       submit.result_sync_flags);
		printf("submit_result_owner_issue_cookie=0x%x\n",
		       submit.result_owner_issue_cookie);
		printf("submit_result_owner_complete_cookie=0x%x\n",
		       submit.result_owner_complete_cookie);
		printf("submit_result_global_issue_ticket=%u\n",
		       submit.result_global_issue_ticket);
		printf("submit_result_global_running_ticket=%u\n",
		       submit.result_global_running_ticket);
		printf("submit_result_global_complete_ticket=%u\n",
		       submit.result_global_complete_ticket);
		printf("submit_result_ctrl_flags=0x%x\n",
		       submit.result_ctrl_flags);
		printf("submit_result_num_ta_status_vals=%u\n",
		       submit.result_num_ta_status_vals);
		printf("submit_result_num_3d_status_vals=%u\n",
		       submit.result_num_3d_status_vals);
		printf("submit_result_ta_dependency=%u\n",
		       submit.result_ta_dependency);
		printf("submit_result_status_ta_offset=0x%x\n",
		       submit.result_status_ta_offset);
		printf("submit_result_status_3d_offset=0x%x\n",
		       submit.result_status_3d_offset);
		printf("submit_result_status_stride=0x%x\n",
		       submit.result_status_stride);
		printf("submit_result_status_value_seed=0x%x\n",
		       submit.result_status_value_seed);
		printf("submit_result_control_object_id=%u\n",
		       submit.result_control_object_id);
		printf("submit_result_status_object_id=%u\n",
		       submit.result_status_object_id);
		printf("submit_result_control_gpu_va32=0x%x\n",
		       submit.result_control_gpu_va32);
		printf("submit_result_status_gpu_va32=0x%x\n",
		       submit.result_status_gpu_va32);
		printf("submit_result_report_offset=0x%x\n",
		       submit.result_report_offset);
		printf("submit_result_manifest_offset=0x%x\n",
		       submit.result_manifest_offset);
		printf("submit_result_descriptor_offset=0x%x\n",
		       submit.result_descriptor_offset);
		printf("submit_result_plan_offset=0x%x\n",
		       submit.result_plan_offset);
		printf("submit_result_config_count=%u\n",
		       submit.result_config_count);
		printf("submit_manifest_offset=0x%x\n",
		       submit.manifest_offset);
		printf("submit_manifest_stage=%u\n", submit.manifest_stage);
		printf("submit_manifest_stage_name=%s\n",
		       report_stage_name(submit.manifest_stage));
		printf("submit_manifest_stage_count=%u\n",
		       submit.manifest_stage_count);
		printf("submit_manifest_context=%u\n",
		       submit.manifest_context);
		printf("submit_manifest_context_name=%s\n",
		       context_name(submit.manifest_context));
		printf("submit_manifest_cmd_type=%u\n",
		       submit.manifest_cmd_type);
		printf("submit_manifest_cmd_type_name=%s\n",
		       cmd_type_name(submit.manifest_cmd_type));
		printf("submit_manifest_owner_state=%u\n",
		       submit.manifest_owner_state);
		printf("submit_manifest_owner_state_name=%s\n",
		       owner_state_name(submit.manifest_owner_state));
		printf("submit_manifest_ctrl_flags=0x%x\n",
		       submit.manifest_ctrl_flags);
		printf("submit_manifest_queue_base_offset=0x%x\n",
		       submit.manifest_queue_base_offset);
		printf("submit_manifest_queue_size=0x%x\n",
		       submit.manifest_queue_size);
		printf("submit_manifest_queue_packet_size=0x%x\n",
		       submit.manifest_queue_packet_size);
		printf("submit_manifest_queue_packet_count=%u\n",
		       submit.manifest_queue_packet_count);
		printf("submit_manifest_queue_epoch=%u\n",
		       submit.manifest_queue_epoch);
		printf("submit_manifest_ccb_offset=0x%x\n",
		       submit.manifest_ccb_offset);
		printf("submit_manifest_ccb_write_offset=0x%x\n",
		       submit.manifest_ccb_write_offset);
		printf("submit_manifest_ccb_read_offset=0x%x\n",
		       submit.manifest_ccb_read_offset);
		printf("submit_manifest_dependency_active_count=%u\n",
		       submit.manifest_dependency_active_count);
		printf("submit_manifest_global_issue_ticket=%u\n",
		       submit.manifest_global_issue_ticket);
		printf("submit_manifest_global_running_ticket=%u\n",
		       submit.manifest_global_running_ticket);
		printf("submit_manifest_global_complete_ticket=%u\n",
		       submit.manifest_global_complete_ticket);
		printf("submit_manifest_pds_gpu_va32=0x%x\n",
		       submit.manifest_pds_gpu_va32);
		printf("submit_manifest_use_gpu_va32=0x%x\n",
		       submit.manifest_use_gpu_va32);
		printf("submit_manifest_target_gpu_va32=0x%x\n",
		       submit.manifest_target_gpu_va32);
		printf("submit_manifest_status_gpu_va32=0x%x\n",
		       submit.manifest_status_gpu_va32);
		printf("submit_manifest_sync_complete_seq=0x%x\n",
		       submit.manifest_sync_complete_seq);
		printf("submit_manifest_sync_pending_seq=0x%x\n",
		       submit.manifest_sync_pending_seq);
		printf("submit_manifest_sync_flags=0x%x\n",
		       submit.manifest_sync_flags);
		printf("submit_manifest_owner_issue_cookie=0x%x\n",
		       submit.manifest_owner_issue_cookie);
		printf("submit_manifest_owner_complete_cookie=0x%x\n",
		       submit.manifest_owner_complete_cookie);
		printf("submit_manifest_config_count=%u\n",
		       submit.manifest_config_count);
		printf("submit_descriptor_offset=0x%x\n",
		       submit.descriptor_offset);
		printf("submit_descriptor_stage=%u\n",
		       submit.descriptor_stage);
		printf("submit_descriptor_stage_name=%s\n",
		       report_stage_name(submit.descriptor_stage));
		printf("submit_descriptor_stage_count=%u\n",
		       submit.descriptor_stage_count);
		printf("submit_descriptor_context=%u\n",
		       submit.descriptor_context);
		printf("submit_descriptor_context_name=%s\n",
		       context_name(submit.descriptor_context));
		printf("submit_descriptor_cmd_type=%u\n",
		       submit.descriptor_cmd_type);
		printf("submit_descriptor_cmd_type_name=%s\n",
		       cmd_type_name(submit.descriptor_cmd_type));
		printf("submit_descriptor_owner_state=%u\n",
		       submit.descriptor_owner_state);
		printf("submit_descriptor_owner_state_name=%s\n",
		       owner_state_name(submit.descriptor_owner_state));
		printf("submit_descriptor_issued_packets=%u\n",
		       submit.descriptor_issued_packets);
		printf("submit_descriptor_completed_packets=%u\n",
		       submit.descriptor_completed_packets);
		printf("submit_descriptor_dependency_active_count=%u\n",
		       submit.descriptor_dependency_active_count);
		printf("submit_descriptor_queue_packet_count=%u\n",
		       submit.descriptor_queue_packet_count);
		printf("submit_descriptor_queue_epoch=%u\n",
		       submit.descriptor_queue_epoch);
		printf("submit_descriptor_ctrl_flags=0x%x\n",
		       submit.descriptor_ctrl_flags);
		printf("submit_descriptor_queue_base_offset=0x%x\n",
		       submit.descriptor_queue_base_offset);
		printf("submit_descriptor_queue_size=0x%x\n",
		       submit.descriptor_queue_size);
		printf("submit_descriptor_queue_packet_size=0x%x\n",
		       submit.descriptor_queue_packet_size);
		printf("submit_descriptor_ccb_offset=0x%x\n",
		       submit.descriptor_ccb_offset);
		printf("submit_descriptor_sync_offset=0x%x\n",
		       submit.descriptor_sync_offset);
		printf("submit_descriptor_owner_offset=0x%x\n",
		       submit.descriptor_owner_offset);
		printf("submit_descriptor_report_offset=0x%x\n",
		       submit.descriptor_report_offset);
		printf("submit_descriptor_manifest_offset=0x%x\n",
		       submit.descriptor_manifest_offset);
		printf("submit_descriptor_global_issue_ticket=%u\n",
		       submit.descriptor_global_issue_ticket);
		printf("submit_descriptor_global_running_ticket=%u\n",
		       submit.descriptor_global_running_ticket);
		printf("submit_descriptor_global_complete_ticket=%u\n",
		       submit.descriptor_global_complete_ticket);
		printf("submit_descriptor_report_flags=0x%x\n",
		       submit.descriptor_report_flags);
		printf("submit_descriptor_status_ta_offset=0x%x\n",
		       submit.descriptor_status_ta_offset);
		printf("submit_descriptor_status_3d_offset=0x%x\n",
		       submit.descriptor_status_3d_offset);
		printf("submit_descriptor_status_stride=0x%x\n",
		       submit.descriptor_status_stride);
		printf("submit_descriptor_status_value_seed=0x%x\n",
		       submit.descriptor_status_value_seed);
		printf("submit_descriptor_control_gpu_va32=0x%x\n",
		       submit.descriptor_control_gpu_va32);
		printf("submit_descriptor_pds_gpu_va32=0x%x\n",
		       submit.descriptor_pds_gpu_va32);
		printf("submit_descriptor_use_gpu_va32=0x%x\n",
		       submit.descriptor_use_gpu_va32);
		printf("submit_descriptor_target_gpu_va32=0x%x\n",
		       submit.descriptor_target_gpu_va32);
		printf("submit_descriptor_status_gpu_va32=0x%x\n",
		       submit.descriptor_status_gpu_va32);
		printf("submit_descriptor_config_count=%u\n",
		       submit.descriptor_config_count);
		printf("submit_plan_offset=0x%x\n", submit.plan_offset);
		printf("submit_plan_stage=%u\n", submit.plan_stage);
		printf("submit_plan_stage_name=%s\n",
		       report_stage_name(submit.plan_stage));
		printf("submit_plan_stage_count=%u\n",
		       submit.plan_stage_count);
		printf("submit_plan_source=%u\n", submit.plan_source);
		printf("submit_plan_source_name=%s\n",
		       plan_source_name(submit.plan_source));
		printf("submit_plan_context=%u\n", submit.plan_context);
		printf("submit_plan_context_name=%s\n",
		       context_name(submit.plan_context));
		printf("submit_plan_cmd_type=%u\n", submit.plan_cmd_type);
		printf("submit_plan_cmd_type_name=%s\n",
		       cmd_type_name(submit.plan_cmd_type));
		printf("submit_plan_owner_state=%u\n", submit.plan_owner_state);
		printf("submit_plan_owner_state_name=%s\n",
		       owner_state_name(submit.plan_owner_state));
		printf("submit_plan_issued_packets=%u\n",
		       submit.plan_issued_packets);
		printf("submit_plan_completed_packets=%u\n",
		       submit.plan_completed_packets);
		printf("submit_plan_dependency_active_count=%u\n",
		       submit.plan_dependency_active_count);
		printf("submit_plan_refilled_from_complete=%u\n",
		       submit.plan_refilled_from_complete);
		printf("submit_plan_queue_packet_count=%u\n",
		       submit.plan_queue_packet_count);
		printf("submit_plan_queue_epoch=%u\n", submit.plan_queue_epoch);
		printf("submit_plan_queue_base_offset=0x%x\n",
		       submit.plan_queue_base_offset);
		printf("submit_plan_queue_size=0x%x\n",
		       submit.plan_queue_size);
		printf("submit_plan_queue_packet_size=0x%x\n",
		       submit.plan_queue_packet_size);
		printf("submit_plan_ccb_offset=0x%x\n",
		       submit.plan_ccb_offset);
		printf("submit_plan_ccb_write_offset=0x%x\n",
		       submit.plan_ccb_write_offset);
		printf("submit_plan_ccb_read_offset=0x%x\n",
		       submit.plan_ccb_read_offset);
		printf("submit_plan_sync_offset=0x%x\n",
		       submit.plan_sync_offset);
		printf("submit_plan_sync_complete_seq=0x%x\n",
		       submit.plan_sync_complete_seq);
		printf("submit_plan_sync_pending_seq=0x%x\n",
		       submit.plan_sync_pending_seq);
		printf("submit_plan_sync_flags=0x%x\n",
		       submit.plan_sync_flags);
		printf("submit_plan_owner_offset=0x%x\n",
		       submit.plan_owner_offset);
		printf("submit_plan_owner_issue_cookie=0x%x\n",
		       submit.plan_owner_issue_cookie);
		printf("submit_plan_owner_complete_cookie=0x%x\n",
		       submit.plan_owner_complete_cookie);
		printf("submit_plan_global_issue_ticket=%u\n",
		       submit.plan_global_issue_ticket);
		printf("submit_plan_global_running_ticket=%u\n",
		       submit.plan_global_running_ticket);
		printf("submit_plan_global_complete_ticket=%u\n",
		       submit.plan_global_complete_ticket);
		printf("submit_plan_report_offset=0x%x\n",
		       submit.plan_report_offset);
		printf("submit_plan_manifest_offset=0x%x\n",
		       submit.plan_manifest_offset);
		printf("submit_plan_descriptor_offset=0x%x\n",
		       submit.plan_descriptor_offset);
		printf("submit_plan_ctrl_flags=0x%x\n",
		       submit.plan_ctrl_flags);
		printf("submit_plan_ccb_service_address=0x%x\n",
		       submit.plan_ccb_service_address);
		printf("submit_plan_ccb_cache_control=0x%x\n",
		       submit.plan_ccb_cache_control);
		printf("submit_plan_ccb_data0=0x%x\n",
		       submit.plan_ccb_data0);
		printf("submit_plan_ccb_data1=0x%x\n",
		       submit.plan_ccb_data1);
		printf("submit_plan_control_gpu_va32=0x%x\n",
		       submit.plan_control_gpu_va32);
		printf("submit_plan_pds_gpu_va32=0x%x\n",
		       submit.plan_pds_gpu_va32);
		printf("submit_plan_use_gpu_va32=0x%x\n",
		       submit.plan_use_gpu_va32);
		printf("submit_plan_target_gpu_va32=0x%x\n",
		       submit.plan_target_gpu_va32);
		printf("submit_plan_status_gpu_va32=0x%x\n",
		       submit.plan_status_gpu_va32);
		printf("submit_plan_config_count=%u\n",
		       submit.plan_config_count);
		printf("submit_issue_count=%u\n", submit.issue_count);
		printf("submit_transition_count=%u\n", submit.transition_count);
		printf("submit_last_prepare_jiffies=%" PRIu64 "\n",
		       (uint64_t)submit.last_prepare_jiffies);
		printf("submit_last_arm_jiffies=%" PRIu64 "\n",
		       (uint64_t)submit.last_arm_jiffies);
		printf("submit_last_emit_jiffies=%" PRIu64 "\n",
		       (uint64_t)submit.last_emit_jiffies);
		printf("submit_last_launch_jiffies=%" PRIu64 "\n",
		       (uint64_t)submit.last_launch_jiffies);
		printf("submit_last_dispatch_jiffies=%" PRIu64 "\n",
		       (uint64_t)submit.last_dispatch_jiffies);
		printf("submit_last_retired_jiffies=%" PRIu64 "\n",
		       (uint64_t)submit.last_retired_jiffies);
		printf("submit_last_report_jiffies=%" PRIu64 "\n",
		       (uint64_t)submit.last_report_jiffies);
		printf("submit_last_result_jiffies=%" PRIu64 "\n",
		       (uint64_t)submit.last_result_jiffies);
		printf("submit_last_manifest_jiffies=%" PRIu64 "\n",
		       (uint64_t)submit.last_manifest_jiffies);
		printf("submit_last_descriptor_jiffies=%" PRIu64 "\n",
		       (uint64_t)submit.last_descriptor_jiffies);
		printf("submit_last_plan_jiffies=%" PRIu64 "\n",
		       (uint64_t)submit.last_plan_jiffies);
		printf("submit_ta_dependency=%u\n", submit.ta_dependency);
		printf("submit_num_ta_status_vals=%u\n",
		       submit.num_ta_status_vals);
		printf("submit_num_3d_status_vals=%u\n",
		       submit.num_3d_status_vals);
		printf("submit_ctrl_flags=0x%x\n", submit.ctrl_flags);
		printf("submit_ccb_offset=0x%x\n", submit.ccb_offset);
		printf("submit_ccb_write_offset=0x%x\n",
		       submit.ccb_write_offset);
		printf("submit_ccb_read_offset=0x%x\n",
		       submit.ccb_read_offset);
		printf("submit_ccb_service_address=0x%x\n",
		       submit.ccb_service_address);
		printf("submit_ccb_cache_control=0x%x\n",
		       submit.ccb_cache_control);
		printf("submit_ccb_data0=0x%x\n", submit.ccb_data0);
		printf("submit_ccb_data1=0x%x\n", submit.ccb_data1);
		printf("submit_ccb_pds_gpu_va32=0x%x\n",
		       submit.ccb_pds_gpu_va32);
		printf("submit_ccb_use_gpu_va32=0x%x\n",
		       submit.ccb_use_gpu_va32);
		printf("submit_ccb_target_gpu_va32=0x%x\n",
		       submit.ccb_target_gpu_va32);
		printf("submit_ccb_status_gpu_va32=0x%x\n",
		       submit.ccb_status_gpu_va32);
		printf("submit_queue_base_offset=0x%x\n",
		       submit.queue_base_offset);
		printf("submit_queue_size=0x%x\n", submit.queue_size);
		printf("submit_queue_packet_size=0x%x\n",
		       submit.queue_packet_size);
		printf("submit_queue_packet_count=%u\n",
		       submit.queue_packet_count);
		printf("submit_queue_tail_packet_count=%u\n",
		       submit.queue_tail_packet_count);
		printf("submit_queue_head_packet_count=%u\n",
		       submit.queue_head_packet_count);
		printf("submit_queue_epoch=%u\n", submit.queue_epoch);
		printf("submit_status_ta_offset=0x%x\n",
		       submit.status_ta_offset);
		printf("submit_status_3d_offset=0x%x\n",
		       submit.status_3d_offset);
		printf("submit_status_stride=0x%x\n", submit.status_stride);
		printf("submit_status_value_seed=0x%x\n",
		       submit.status_value_seed);
		printf("submit_sync_offset=0x%x\n", submit.sync_offset);
		printf("submit_sync_complete_seq=0x%x\n",
		       submit.sync_complete_seq);
		printf("submit_sync_pending_seq=0x%x\n",
		       submit.sync_pending_seq);
		printf("submit_sync_flags=0x%x\n", submit.sync_flags);
		printf("submit_owner_offset=0x%x\n", submit.owner_offset);
		printf("submit_owner_issue_cookie=0x%x\n",
		       submit.owner_issue_cookie);
		printf("submit_owner_complete_cookie=0x%x\n",
		       submit.owner_complete_cookie);
		printf("submit_global_issue_ticket=%u\n",
		       submit.global_issue_ticket);
		printf("submit_global_running_ticket=%u\n",
		       submit.global_running_ticket);
		printf("submit_global_complete_ticket=%u\n",
		       submit.global_complete_ticket);
		printf("submit_control_object_id=%u\n",
		       submit.control_object_id);
		printf("submit_pds_object_id=%u\n", submit.pds_object_id);
		printf("submit_use_object_id=%u\n", submit.use_object_id);
		printf("submit_target_object_id=%u\n", submit.target_object_id);
		printf("submit_status_object_id=%u\n", submit.status_object_id);
		printf("submit_control_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.control_gpu_va);
		printf("submit_pds_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.pds_gpu_va);
		printf("submit_use_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.use_gpu_va);
		printf("submit_target_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.target_gpu_va);
		printf("submit_status_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)submit.status_gpu_va);
		printf("submit_dependency_slot_count=%u\n",
		       submit.dependency_slot_count);
		printf("submit_dependency_active_count=%u\n",
		       submit.dependency_active_count);
		for (i = 0; i < submit.dependency_slot_count &&
			    i < DRM_PSB_SGX_LAB_DEPENDENCY_SLOTS; i++) {
			printf("dependency[%u]_submit_id=%u\n", i + 1,
			       submit.dependencies[i].submit_id);
			printf("dependency[%u]_required_ticket=%u\n", i + 1,
			       submit.dependencies[i].required_ticket);
			printf("dependency[%u]_observed_complete_ticket=%u\n",
			       i + 1,
			       submit.dependencies[i].observed_complete_ticket);
			printf("dependency[%u]_flags=0x%x\n", i + 1,
			       submit.dependencies[i].flags);
			print_dep_flags(submit.dependencies[i].flags);
		}
	}

	close(fd);
	return ret ? 3 : 0;
}
