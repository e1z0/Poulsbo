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

static const char *role_name(uint32_t role)
{
	switch (role) {
	case DRM_PSB_SGX_LAB_ROLE_GENERIC:
		return "generic";
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
	default:
		return "unknown";
	}
}

static const char *class_name(uint32_t object_class)
{
	switch (object_class) {
	case DRM_PSB_SGX_LAB_CLASS_GENERIC:
		return "generic";
	case DRM_PSB_SGX_LAB_CLASS_PACKET:
		return "packet";
	case DRM_PSB_SGX_LAB_CLASS_CODE:
		return "code";
	case DRM_PSB_SGX_LAB_CLASS_STATE:
		return "state";
	case DRM_PSB_SGX_LAB_CLASS_SURFACE:
		return "surface";
	default:
		return "unknown";
	}
}

static const char *gpu_use_name(uint32_t gpu_use)
{
	switch (gpu_use) {
	case DRM_PSB_SGX_LAB_GPU_USE_GENERIC:
		return "generic";
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
	default:
		return "unknown";
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

static const char *mmu_op_name(uint32_t op)
{
	switch (op) {
	case 0:
		return "none";
	case 1:
		return "flush";
	case 2:
		return "set_pd_context";
	case 3:
		return "insert_pfn_sequence";
	case 4:
		return "insert_pages";
	case 5:
		return "remove_pfn_sequence";
	case 6:
		return "remove_pages";
	default:
		return "unknown";
	}
}

static void usage(const char *argv0)
{
	fprintf(stderr,
		"usage: %s [/dev/dri/card0] [cached|capture|json|capture-json]\n",
		argv0);
}

static int query_coord(int fd, struct drm_psb_sgx_lab_coord_query *coord)
{
	memset(coord, 0, sizeof(*coord));
	coord->size = sizeof(*coord);
	return ioctl(fd, DRM_IOCTL_PSB_SGX_LAB_QUERY_COORD, coord);
}

static int query_gtt_state(int fd, struct drm_psb_sgx_lab_gtt_state_query *gtt)
{
	memset(gtt, 0, sizeof(*gtt));
	gtt->size = sizeof(*gtt);
	return ioctl(fd, DRM_IOCTL_PSB_SGX_LAB_QUERY_GTT_STATE, gtt);
}

static int query_sgx_state(int fd, int capture,
			   struct drm_psb_sgx_lab_sgx_state_query *sgx)
{
	memset(sgx, 0, sizeof(*sgx));
	sgx->size = sizeof(*sgx);
	if (capture)
		sgx->flags |= DRM_PSB_SGX_LAB_QUERY_SGX_STATE_CAPTURE;
	return ioctl(fd, DRM_IOCTL_PSB_SGX_LAB_QUERY_SGX_STATE, sgx);
}

static int query_object_index(int fd, uint32_t index,
			      struct drm_psb_sgx_lab_object_query *object)
{
	memset(object, 0, sizeof(*object));
	object->size = sizeof(*object);
	object->query_index = index;
	return ioctl(fd, DRM_IOCTL_PSB_SGX_LAB_QUERY_OBJECT, object);
}

static int query_submit_index(int fd, uint32_t index,
			      struct drm_psb_sgx_lab_submit_query *submit)
{
	memset(submit, 0, sizeof(*submit));
	submit->size = sizeof(*submit);
	submit->flags = DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_INDEX;
	submit->query_index = index;
	return ioctl(fd, DRM_IOCTL_PSB_SGX_LAB_QUERY_SUBMIT, submit);
}

static void print_json_string(const char *value)
{
	const unsigned char *p = (const unsigned char *)value;

	putchar('"');
	while (*p) {
		switch (*p) {
		case '\\':
			fputs("\\\\", stdout);
			break;
		case '"':
			fputs("\\\"", stdout);
			break;
		case '\n':
			fputs("\\n", stdout);
			break;
		case '\r':
			fputs("\\r", stdout);
			break;
		case '\t':
			fputs("\\t", stdout);
			break;
		default:
			if (*p < 0x20)
				printf("\\u%04x", *p);
			else
				putchar(*p);
			break;
		}
		p++;
	}
	putchar('"');
}

static void print_dump_text(
	int fd,
	const struct drm_psb_sgx_lab_coord_query *coord,
	const struct drm_psb_sgx_lab_gtt_state_query *gtt,
	const struct drm_psb_sgx_lab_sgx_state_query *sgx)
{
	uint32_t i;

	printf("coord_focus_valid=%u\n", coord->focus_valid);
	printf("coord_focused_submit_id=%u\n", coord->focused_submit_id);
	printf("coord_live_submit_count=%u\n", coord->live_submit_count);
	printf("coord_live_object_count=%u\n", coord->live_object_count);
	printf("coord_issue_ticket=%u\n", coord->issue_ticket);
	printf("coord_running_ticket=%u\n", coord->running_ticket);
	printf("coord_complete_ticket=%u\n", coord->complete_ticket);
	printf("coord_issue_submit_id=%u\n", coord->issue_submit_id);
	printf("coord_running_submit_id=%u\n", coord->running_submit_id);
	printf("coord_complete_submit_id=%u\n", coord->complete_submit_id);

	printf("gtt_state_range_total_count=%u\n", gtt->range_total_count);
	printf("gtt_state_range_gem_count=%u\n", gtt->range_gem_count);
	printf("gtt_state_range_sgx_object_count=%u\n",
	       gtt->range_sgx_object_count);
	printf("gtt_state_range_pf_bound_count=%u\n",
	       gtt->range_pf_bound_count);
	printf("gtt_state_mmu_last_op_name=%s\n",
	       mmu_op_name(gtt->mmu_last_op));
	printf("gtt_state_mmu_insert_pages_count=%u\n",
	       gtt->mmu_insert_pages_count);
	printf("gtt_state_mmu_remove_pages_count=%u\n",
	       gtt->mmu_remove_pages_count);

	printf("sgx_state_reason=%s\n", sgx->reason[0] ?
	       (const char *)sgx->reason : "none");
	printf("sgx_state_sequence=%u\n", sgx->sequence);
	printf("sgx_state_capture_failed=%u\n", sgx->capture_failed);
	printf("sgx_state_irq_count=%u\n", sgx->irq_count);
	printf("sgx_state_fault_count=%u\n", sgx->fault_count);
	printf("sgx_state_bif_fault=0x%x\n", sgx->bif_fault);
	printf("sgx_state_twod_blit_status=0x%x\n", sgx->twod_blit_status);

	for (i = 0; i < coord->live_object_count; i++) {
		struct drm_psb_sgx_lab_object_query object;
		int ret;
		int saved_errno;

		errno = 0;
		ret = query_object_index(fd, i, &object);
		saved_errno = errno;
		printf("object[%u]_ioctl_ret=%d errno=%d\n", i, ret, saved_errno);
		if (ret != 0)
			continue;

		printf("object[%u]_id=%u\n", i, object.resolved_object_id);
		printf("object[%u]_range_id=%u\n", i, object.range_id);
		printf("object[%u]_range_type_name=%s\n", i,
		       range_type_name(object.range_type));
		printf("object[%u]_role_name=%s\n", i, role_name(object.role));
		printf("object[%u]_class_name=%s\n", i,
		       class_name(object.object_class));
		printf("object[%u]_gpu_use_name=%s\n", i,
		       gpu_use_name(object.gpu_use));
		printf("object[%u]_context_name=%s\n", i,
		       context_name(object.expected_context));
		printf("object[%u]_gpu_va=0x%08" PRIx64 "\n", i,
		       (uint64_t)object.gpu_va);
		printf("object[%u]_pf_bind_count=%u\n", i,
		       object.pf_bind_count);
	}

	for (i = 0; i < coord->live_submit_count; i++) {
		struct drm_psb_sgx_lab_submit_query submit;
		int ret;
		int saved_errno;

		errno = 0;
		ret = query_submit_index(fd, i, &submit);
		saved_errno = errno;
		printf("submit[%u]_ioctl_ret=%d errno=%d\n", i, ret, saved_errno);
		if (ret != 0)
			continue;

		printf("submit[%u]_id=%u\n", i, submit.resolved_submit_id);
		printf("submit[%u]_context_name=%s\n", i,
		       context_name(submit.context));
		printf("submit[%u]_cmd_type_name=%s\n", i,
		       cmd_type_name(submit.cmd_type));
		printf("submit[%u]_owner_state_name=%s\n", i,
		       owner_state_name(submit.owner_state));
		printf("submit[%u]_prepare_count=%u\n", i, submit.prepare_count);
		printf("submit[%u]_arm_valid=%u\n", i, submit.arm_valid);
		printf("submit[%u]_arm_count=%u\n", i, submit.arm_count);
		printf("submit[%u]_arm_owner_state_name=%s\n", i,
		       owner_state_name(submit.arm_owner_state));
		printf("submit[%u]_arm_queue_packet_count=%u\n", i,
		       submit.arm_queue_packet_count);
		printf("submit[%u]_arm_sync_pending_seq=0x%x\n", i,
		       submit.arm_sync_pending_seq);
		printf("submit[%u]_emit_valid=%u\n", i, submit.emit_valid);
		printf("submit[%u]_emit_count=%u\n", i, submit.emit_count);
		printf("submit[%u]_emit_cmd_type_name=%s\n", i,
		       cmd_type_name(submit.emit_cmd_type));
		printf("submit[%u]_emit_owner_state_name=%s\n", i,
		       owner_state_name(submit.emit_owner_state));
		printf("submit[%u]_emit_queue_packet_count=%u\n", i,
		       submit.emit_queue_packet_count);
		printf("submit[%u]_emit_sync_pending_seq=0x%x\n", i,
		       submit.emit_sync_pending_seq);
		printf("submit[%u]_emit_global_issue_ticket=%u\n", i,
		       submit.emit_global_issue_ticket);
		printf("submit[%u]_emit_dependency_active_count=%u\n", i,
		       submit.emit_dependency_active_count);
		printf("submit[%u]_emit_control_object_id=%u\n", i,
		       submit.emit_control_object_id);
		printf("submit[%u]_emit_status_object_id=%u\n", i,
		       submit.emit_status_object_id);
		printf("submit[%u]_emit_control_gpu_va=0x%08" PRIx64 "\n", i,
		       (uint64_t)submit.emit_control_gpu_va);
		printf("submit[%u]_emit_status_gpu_va=0x%08" PRIx64 "\n", i,
		       (uint64_t)submit.emit_status_gpu_va);
		printf("submit[%u]_launch_valid=%u\n", i, submit.launch_valid);
		printf("submit[%u]_launch_consumed=%u\n", i,
		       submit.launch_consumed);
		printf("submit[%u]_launch_count=%u\n", i, submit.launch_count);
		printf("submit[%u]_launch_owner_state_name=%s\n", i,
		       owner_state_name(submit.launch_owner_state));
		printf("submit[%u]_launch_issued_packets=%u\n", i,
		       submit.launch_issued_packets);
		printf("submit[%u]_launch_queue_packet_count=%u\n", i,
		       submit.launch_queue_packet_count);
		printf("submit[%u]_launch_new_write_offset=0x%x\n", i,
		       submit.launch_new_write_offset);
		printf("submit[%u]_launch_global_issue_ticket=%u\n", i,
		       submit.launch_global_issue_ticket);
		printf("submit[%u]_launch_dependency_active_count=%u\n", i,
		       submit.launch_dependency_active_count);
		printf("submit[%u]_launch_control_object_id=%u\n", i,
		       submit.launch_control_object_id);
		printf("submit[%u]_launch_status_object_id=%u\n", i,
		       submit.launch_status_object_id);
		printf("submit[%u]_launch_control_gpu_va=0x%08" PRIx64 "\n", i,
		       (uint64_t)submit.launch_control_gpu_va);
		printf("submit[%u]_launch_status_gpu_va=0x%08" PRIx64 "\n", i,
		       (uint64_t)submit.launch_status_gpu_va);
		printf("submit[%u]_dispatch_valid=%u\n", i,
		       submit.dispatch_valid);
		printf("submit[%u]_dispatch_count=%u\n", i,
		       submit.dispatch_count);
		printf("submit[%u]_dispatch_owner_state_name=%s\n", i,
		       owner_state_name(submit.dispatch_owner_state));
		printf("submit[%u]_dispatch_issued_packets=%u\n", i,
		       submit.dispatch_issued_packets);
		printf("submit[%u]_dispatch_completed_packets=%u\n", i,
		       submit.dispatch_completed_packets);
		printf("submit[%u]_dispatch_queue_packet_count=%u\n", i,
		       submit.dispatch_queue_packet_count);
		printf("submit[%u]_dispatch_sync_complete_seq=0x%x\n", i,
		       submit.dispatch_sync_complete_seq);
		printf("submit[%u]_dispatch_sync_pending_seq=0x%x\n", i,
		       submit.dispatch_sync_pending_seq);
		printf("submit[%u]_dispatch_global_issue_ticket=%u\n", i,
		       submit.dispatch_global_issue_ticket);
		printf("submit[%u]_dispatch_global_running_ticket=%u\n", i,
		       submit.dispatch_global_running_ticket);
		printf("submit[%u]_dispatch_global_complete_ticket=%u\n", i,
		       submit.dispatch_global_complete_ticket);
		printf("submit[%u]_dispatch_dependency_active_count=%u\n", i,
		       submit.dispatch_dependency_active_count);
		printf("submit[%u]_dispatch_control_object_id=%u\n", i,
		       submit.dispatch_control_object_id);
		printf("submit[%u]_dispatch_status_object_id=%u\n", i,
		       submit.dispatch_status_object_id);
		printf("submit[%u]_dispatch_control_gpu_va=0x%08" PRIx64 "\n", i,
		       (uint64_t)submit.dispatch_control_gpu_va);
		printf("submit[%u]_dispatch_status_gpu_va=0x%08" PRIx64 "\n", i,
		       (uint64_t)submit.dispatch_status_gpu_va);
		printf("submit[%u]_retired_valid=%u\n", i,
		       submit.retired_valid);
		printf("submit[%u]_retired_count=%u\n", i,
		       submit.retired_count);
		printf("submit[%u]_retired_dispatch_count=%u\n", i,
		       submit.retired_dispatch_count);
		printf("submit[%u]_retired_owner_state_name=%s\n", i,
		       owner_state_name(submit.retired_owner_state));
		printf("submit[%u]_retired_issued_packets=%u\n", i,
		       submit.retired_issued_packets);
		printf("submit[%u]_retired_completed_packets=%u\n", i,
		       submit.retired_completed_packets);
		printf("submit[%u]_retired_queue_packet_count=%u\n", i,
		       submit.retired_queue_packet_count);
		printf("submit[%u]_retired_sync_complete_seq=0x%x\n", i,
		       submit.retired_sync_complete_seq);
		printf("submit[%u]_retired_sync_pending_seq=0x%x\n", i,
		       submit.retired_sync_pending_seq);
		printf("submit[%u]_retired_global_issue_ticket=%u\n", i,
		       submit.retired_global_issue_ticket);
		printf("submit[%u]_retired_global_running_ticket=%u\n", i,
		       submit.retired_global_running_ticket);
		printf("submit[%u]_retired_global_complete_ticket=%u\n", i,
		       submit.retired_global_complete_ticket);
		printf("submit[%u]_retired_dependency_active_count=%u\n", i,
		       submit.retired_dependency_active_count);
		printf("submit[%u]_retired_control_object_id=%u\n", i,
		       submit.retired_control_object_id);
		printf("submit[%u]_retired_status_object_id=%u\n", i,
		       submit.retired_status_object_id);
		printf("submit[%u]_retired_control_gpu_va=0x%08" PRIx64 "\n", i,
		       (uint64_t)submit.retired_control_gpu_va);
		printf("submit[%u]_retired_status_gpu_va=0x%08" PRIx64 "\n", i,
		       (uint64_t)submit.retired_status_gpu_va);
		printf("submit[%u]_report_offset=0x%x\n", i,
		       submit.report_offset);
		printf("submit[%u]_report_stage_name=%s\n", i,
		       report_stage_name(submit.report_stage));
		printf("submit[%u]_report_stage_count=%u\n", i,
		       submit.report_stage_count);
		printf("submit[%u]_report_owner_state_name=%s\n", i,
		       owner_state_name(submit.report_owner_state));
		printf("submit[%u]_report_queue_packet_count=%u\n", i,
		       submit.report_queue_packet_count);
		printf("submit[%u]_report_completed_packets=%u\n", i,
		       submit.report_completed_packets);
		printf("submit[%u]_report_dependency_active_count=%u\n", i,
		       submit.report_dependency_active_count);
		printf("submit[%u]_report_queue_epoch=%u\n", i,
		       submit.report_queue_epoch);
		printf("submit[%u]_report_global_issue_ticket=%u\n", i,
		       submit.report_global_issue_ticket);
		printf("submit[%u]_report_global_running_ticket=%u\n", i,
		       submit.report_global_running_ticket);
		printf("submit[%u]_report_global_complete_ticket=%u\n", i,
		       submit.report_global_complete_ticket);
			printf("submit[%u]_report_flags=0x%x\n", i,
			       submit.report_flags);
			printf("submit[%u]_report_config_count=%u\n", i,
			       submit.report_config_count);
			printf("submit[%u]_result_offset=0x%x\n", i,
			       submit.result_offset);
			printf("submit[%u]_result_stage_name=%s\n", i,
			       report_stage_name(submit.result_stage));
			printf("submit[%u]_result_stage_count=%u\n", i,
			       submit.result_stage_count);
			printf("submit[%u]_result_source_name=%s\n", i,
			       result_source_name(submit.result_source));
			printf("submit[%u]_result_context_name=%s\n", i,
			       context_name(submit.result_context));
			printf("submit[%u]_result_cmd_type_name=%s\n", i,
			       cmd_type_name(submit.result_cmd_type));
			printf("submit[%u]_result_owner_state_name=%s\n", i,
			       owner_state_name(submit.result_owner_state));
			printf("submit[%u]_result_issued_packets=%u\n", i,
			       submit.result_issued_packets);
			printf("submit[%u]_result_completed_packets=%u\n", i,
			       submit.result_completed_packets);
			printf("submit[%u]_result_dependency_active_count=%u\n", i,
			       submit.result_dependency_active_count);
			printf("submit[%u]_result_queue_packet_count=%u\n", i,
			       submit.result_queue_packet_count);
			printf("submit[%u]_result_queue_epoch=%u\n", i,
			       submit.result_queue_epoch);
			printf("submit[%u]_result_sync_complete_seq=0x%x\n", i,
			       submit.result_sync_complete_seq);
			printf("submit[%u]_result_sync_pending_seq=0x%x\n", i,
			       submit.result_sync_pending_seq);
			printf("submit[%u]_result_sync_flags=0x%x\n", i,
			       submit.result_sync_flags);
			printf("submit[%u]_result_owner_issue_cookie=0x%x\n", i,
			       submit.result_owner_issue_cookie);
			printf("submit[%u]_result_owner_complete_cookie=0x%x\n", i,
			       submit.result_owner_complete_cookie);
			printf("submit[%u]_result_global_issue_ticket=%u\n", i,
			       submit.result_global_issue_ticket);
			printf("submit[%u]_result_global_running_ticket=%u\n", i,
			       submit.result_global_running_ticket);
			printf("submit[%u]_result_global_complete_ticket=%u\n", i,
			       submit.result_global_complete_ticket);
			printf("submit[%u]_result_ctrl_flags=0x%x\n", i,
			       submit.result_ctrl_flags);
			printf("submit[%u]_result_num_ta_status_vals=%u\n", i,
			       submit.result_num_ta_status_vals);
			printf("submit[%u]_result_num_3d_status_vals=%u\n", i,
			       submit.result_num_3d_status_vals);
			printf("submit[%u]_result_ta_dependency=%u\n", i,
			       submit.result_ta_dependency);
			printf("submit[%u]_result_status_ta_offset=0x%x\n", i,
			       submit.result_status_ta_offset);
			printf("submit[%u]_result_status_3d_offset=0x%x\n", i,
			       submit.result_status_3d_offset);
			printf("submit[%u]_result_status_stride=0x%x\n", i,
			       submit.result_status_stride);
			printf("submit[%u]_result_status_value_seed=0x%x\n", i,
			       submit.result_status_value_seed);
			printf("submit[%u]_result_control_object_id=%u\n", i,
			       submit.result_control_object_id);
			printf("submit[%u]_result_status_object_id=%u\n", i,
			       submit.result_status_object_id);
			printf("submit[%u]_result_control_gpu_va32=0x%x\n", i,
			       submit.result_control_gpu_va32);
			printf("submit[%u]_result_status_gpu_va32=0x%x\n", i,
			       submit.result_status_gpu_va32);
			printf("submit[%u]_result_report_offset=0x%x\n", i,
			       submit.result_report_offset);
			printf("submit[%u]_result_manifest_offset=0x%x\n", i,
			       submit.result_manifest_offset);
			printf("submit[%u]_result_descriptor_offset=0x%x\n", i,
			       submit.result_descriptor_offset);
			printf("submit[%u]_result_plan_offset=0x%x\n", i,
			       submit.result_plan_offset);
			printf("submit[%u]_result_config_count=%u\n", i,
			       submit.result_config_count);
			printf("submit[%u]_manifest_offset=0x%x\n", i,
			       submit.manifest_offset);
		printf("submit[%u]_manifest_stage_name=%s\n", i,
		       report_stage_name(submit.manifest_stage));
		printf("submit[%u]_manifest_stage_count=%u\n", i,
		       submit.manifest_stage_count);
		printf("submit[%u]_manifest_context_name=%s\n", i,
		       context_name(submit.manifest_context));
		printf("submit[%u]_manifest_cmd_type_name=%s\n", i,
		       cmd_type_name(submit.manifest_cmd_type));
		printf("submit[%u]_manifest_owner_state_name=%s\n", i,
		       owner_state_name(submit.manifest_owner_state));
		printf("submit[%u]_manifest_ctrl_flags=0x%x\n", i,
		       submit.manifest_ctrl_flags);
		printf("submit[%u]_manifest_queue_packet_count=%u\n", i,
		       submit.manifest_queue_packet_count);
		printf("submit[%u]_manifest_queue_epoch=%u\n", i,
		       submit.manifest_queue_epoch);
		printf("submit[%u]_manifest_ccb_offset=0x%x\n", i,
		       submit.manifest_ccb_offset);
		printf("submit[%u]_manifest_ccb_write_offset=0x%x\n", i,
		       submit.manifest_ccb_write_offset);
		printf("submit[%u]_manifest_ccb_read_offset=0x%x\n", i,
		       submit.manifest_ccb_read_offset);
		printf("submit[%u]_manifest_dependency_active_count=%u\n", i,
		       submit.manifest_dependency_active_count);
		printf("submit[%u]_manifest_global_issue_ticket=%u\n", i,
		       submit.manifest_global_issue_ticket);
		printf("submit[%u]_manifest_global_running_ticket=%u\n", i,
		       submit.manifest_global_running_ticket);
		printf("submit[%u]_manifest_global_complete_ticket=%u\n", i,
		       submit.manifest_global_complete_ticket);
		printf("submit[%u]_manifest_pds_gpu_va32=0x%x\n", i,
		       submit.manifest_pds_gpu_va32);
		printf("submit[%u]_manifest_use_gpu_va32=0x%x\n", i,
		       submit.manifest_use_gpu_va32);
		printf("submit[%u]_manifest_target_gpu_va32=0x%x\n", i,
		       submit.manifest_target_gpu_va32);
		printf("submit[%u]_manifest_status_gpu_va32=0x%x\n", i,
		       submit.manifest_status_gpu_va32);
		printf("submit[%u]_manifest_sync_complete_seq=0x%x\n", i,
		       submit.manifest_sync_complete_seq);
		printf("submit[%u]_manifest_sync_pending_seq=0x%x\n", i,
		       submit.manifest_sync_pending_seq);
		printf("submit[%u]_manifest_owner_issue_cookie=0x%x\n", i,
		       submit.manifest_owner_issue_cookie);
		printf("submit[%u]_manifest_owner_complete_cookie=0x%x\n", i,
		       submit.manifest_owner_complete_cookie);
		printf("submit[%u]_manifest_config_count=%u\n", i,
		       submit.manifest_config_count);
		printf("submit[%u]_descriptor_offset=0x%x\n", i,
		       submit.descriptor_offset);
		printf("submit[%u]_descriptor_stage_name=%s\n", i,
		       report_stage_name(submit.descriptor_stage));
		printf("submit[%u]_descriptor_stage_count=%u\n", i,
		       submit.descriptor_stage_count);
		printf("submit[%u]_descriptor_context_name=%s\n", i,
		       context_name(submit.descriptor_context));
		printf("submit[%u]_descriptor_cmd_type_name=%s\n", i,
		       cmd_type_name(submit.descriptor_cmd_type));
		printf("submit[%u]_descriptor_owner_state_name=%s\n", i,
		       owner_state_name(submit.descriptor_owner_state));
		printf("submit[%u]_descriptor_issued_packets=%u\n", i,
		       submit.descriptor_issued_packets);
		printf("submit[%u]_descriptor_completed_packets=%u\n", i,
		       submit.descriptor_completed_packets);
		printf("submit[%u]_descriptor_dependency_active_count=%u\n", i,
		       submit.descriptor_dependency_active_count);
		printf("submit[%u]_descriptor_queue_packet_count=%u\n", i,
		       submit.descriptor_queue_packet_count);
		printf("submit[%u]_descriptor_queue_epoch=%u\n", i,
		       submit.descriptor_queue_epoch);
		printf("submit[%u]_descriptor_ctrl_flags=0x%x\n", i,
		       submit.descriptor_ctrl_flags);
		printf("submit[%u]_descriptor_queue_base_offset=0x%x\n", i,
		       submit.descriptor_queue_base_offset);
		printf("submit[%u]_descriptor_queue_size=0x%x\n", i,
		       submit.descriptor_queue_size);
		printf("submit[%u]_descriptor_queue_packet_size=0x%x\n", i,
		       submit.descriptor_queue_packet_size);
		printf("submit[%u]_descriptor_ccb_offset=0x%x\n", i,
		       submit.descriptor_ccb_offset);
		printf("submit[%u]_descriptor_sync_offset=0x%x\n", i,
		       submit.descriptor_sync_offset);
		printf("submit[%u]_descriptor_owner_offset=0x%x\n", i,
		       submit.descriptor_owner_offset);
		printf("submit[%u]_descriptor_report_offset=0x%x\n", i,
		       submit.descriptor_report_offset);
		printf("submit[%u]_descriptor_manifest_offset=0x%x\n", i,
		       submit.descriptor_manifest_offset);
		printf("submit[%u]_descriptor_global_issue_ticket=%u\n", i,
		       submit.descriptor_global_issue_ticket);
		printf("submit[%u]_descriptor_global_running_ticket=%u\n", i,
		       submit.descriptor_global_running_ticket);
		printf("submit[%u]_descriptor_global_complete_ticket=%u\n", i,
		       submit.descriptor_global_complete_ticket);
		printf("submit[%u]_descriptor_report_flags=0x%x\n", i,
		       submit.descriptor_report_flags);
		printf("submit[%u]_descriptor_status_ta_offset=0x%x\n", i,
		       submit.descriptor_status_ta_offset);
		printf("submit[%u]_descriptor_status_3d_offset=0x%x\n", i,
		       submit.descriptor_status_3d_offset);
		printf("submit[%u]_descriptor_status_stride=0x%x\n", i,
		       submit.descriptor_status_stride);
		printf("submit[%u]_descriptor_status_value_seed=0x%x\n", i,
		       submit.descriptor_status_value_seed);
		printf("submit[%u]_descriptor_control_gpu_va32=0x%x\n", i,
		       submit.descriptor_control_gpu_va32);
		printf("submit[%u]_descriptor_pds_gpu_va32=0x%x\n", i,
		       submit.descriptor_pds_gpu_va32);
		printf("submit[%u]_descriptor_use_gpu_va32=0x%x\n", i,
		       submit.descriptor_use_gpu_va32);
		printf("submit[%u]_descriptor_target_gpu_va32=0x%x\n", i,
		       submit.descriptor_target_gpu_va32);
		printf("submit[%u]_descriptor_status_gpu_va32=0x%x\n", i,
		       submit.descriptor_status_gpu_va32);
		printf("submit[%u]_descriptor_config_count=%u\n", i,
		       submit.descriptor_config_count);
		printf("submit[%u]_plan_offset=0x%x\n", i, submit.plan_offset);
		printf("submit[%u]_plan_stage_name=%s\n", i,
		       report_stage_name(submit.plan_stage));
		printf("submit[%u]_plan_stage_count=%u\n", i,
		       submit.plan_stage_count);
		printf("submit[%u]_plan_source_name=%s\n", i,
		       plan_source_name(submit.plan_source));
		printf("submit[%u]_plan_context_name=%s\n", i,
		       context_name(submit.plan_context));
		printf("submit[%u]_plan_cmd_type_name=%s\n", i,
		       cmd_type_name(submit.plan_cmd_type));
		printf("submit[%u]_plan_owner_state_name=%s\n", i,
		       owner_state_name(submit.plan_owner_state));
		printf("submit[%u]_plan_issued_packets=%u\n", i,
		       submit.plan_issued_packets);
		printf("submit[%u]_plan_completed_packets=%u\n", i,
		       submit.plan_completed_packets);
		printf("submit[%u]_plan_dependency_active_count=%u\n", i,
		       submit.plan_dependency_active_count);
		printf("submit[%u]_plan_refilled_from_complete=%u\n", i,
		       submit.plan_refilled_from_complete);
		printf("submit[%u]_plan_queue_packet_count=%u\n", i,
		       submit.plan_queue_packet_count);
		printf("submit[%u]_plan_queue_epoch=%u\n", i,
		       submit.plan_queue_epoch);
		printf("submit[%u]_plan_queue_base_offset=0x%x\n", i,
		       submit.plan_queue_base_offset);
		printf("submit[%u]_plan_queue_size=0x%x\n", i,
		       submit.plan_queue_size);
		printf("submit[%u]_plan_queue_packet_size=0x%x\n", i,
		       submit.plan_queue_packet_size);
		printf("submit[%u]_plan_ccb_offset=0x%x\n", i,
		       submit.plan_ccb_offset);
		printf("submit[%u]_plan_ccb_write_offset=0x%x\n", i,
		       submit.plan_ccb_write_offset);
		printf("submit[%u]_plan_ccb_read_offset=0x%x\n", i,
		       submit.plan_ccb_read_offset);
		printf("submit[%u]_plan_sync_offset=0x%x\n", i,
		       submit.plan_sync_offset);
		printf("submit[%u]_plan_sync_complete_seq=0x%x\n", i,
		       submit.plan_sync_complete_seq);
		printf("submit[%u]_plan_sync_pending_seq=0x%x\n", i,
		       submit.plan_sync_pending_seq);
		printf("submit[%u]_plan_sync_flags=0x%x\n", i,
		       submit.plan_sync_flags);
		printf("submit[%u]_plan_owner_offset=0x%x\n", i,
		       submit.plan_owner_offset);
		printf("submit[%u]_plan_owner_issue_cookie=0x%x\n", i,
		       submit.plan_owner_issue_cookie);
		printf("submit[%u]_plan_owner_complete_cookie=0x%x\n", i,
		       submit.plan_owner_complete_cookie);
		printf("submit[%u]_plan_global_issue_ticket=%u\n", i,
		       submit.plan_global_issue_ticket);
		printf("submit[%u]_plan_global_running_ticket=%u\n", i,
		       submit.plan_global_running_ticket);
		printf("submit[%u]_plan_global_complete_ticket=%u\n", i,
		       submit.plan_global_complete_ticket);
		printf("submit[%u]_plan_report_offset=0x%x\n", i,
		       submit.plan_report_offset);
		printf("submit[%u]_plan_manifest_offset=0x%x\n", i,
		       submit.plan_manifest_offset);
		printf("submit[%u]_plan_descriptor_offset=0x%x\n", i,
		       submit.plan_descriptor_offset);
		printf("submit[%u]_plan_ctrl_flags=0x%x\n", i,
		       submit.plan_ctrl_flags);
		printf("submit[%u]_plan_ccb_service_address=0x%x\n", i,
		       submit.plan_ccb_service_address);
		printf("submit[%u]_plan_ccb_cache_control=0x%x\n", i,
		       submit.plan_ccb_cache_control);
		printf("submit[%u]_plan_ccb_data0=0x%x\n", i,
		       submit.plan_ccb_data0);
		printf("submit[%u]_plan_ccb_data1=0x%x\n", i,
		       submit.plan_ccb_data1);
		printf("submit[%u]_plan_control_gpu_va32=0x%x\n", i,
		       submit.plan_control_gpu_va32);
		printf("submit[%u]_plan_pds_gpu_va32=0x%x\n", i,
		       submit.plan_pds_gpu_va32);
		printf("submit[%u]_plan_use_gpu_va32=0x%x\n", i,
		       submit.plan_use_gpu_va32);
		printf("submit[%u]_plan_target_gpu_va32=0x%x\n", i,
		       submit.plan_target_gpu_va32);
		printf("submit[%u]_plan_status_gpu_va32=0x%x\n", i,
		       submit.plan_status_gpu_va32);
		printf("submit[%u]_plan_config_count=%u\n", i,
		       submit.plan_config_count);
		printf("submit[%u]_issue_count=%u\n", i, submit.issue_count);
		printf("submit[%u]_validate_count=%u\n", i, submit.validate_count);
		printf("submit[%u]_transition_count=%u\n", i,
		       submit.transition_count);
		printf("submit[%u]_queue_packet_count=%u\n", i,
		       submit.queue_packet_count);
		printf("submit[%u]_sync_pending_seq=0x%x\n", i,
		       submit.sync_pending_seq);
		printf("submit[%u]_owner_issue_cookie=0x%x\n", i,
		       submit.owner_issue_cookie);
		printf("submit[%u]_global_issue_ticket=%u\n", i,
		       submit.global_issue_ticket);
		printf("submit[%u]_ccb_pds_gpu_va32=0x%x\n", i,
		       submit.ccb_pds_gpu_va32);
		printf("submit[%u]_ccb_use_gpu_va32=0x%x\n", i,
		       submit.ccb_use_gpu_va32);
		printf("submit[%u]_ccb_target_gpu_va32=0x%x\n", i,
		       submit.ccb_target_gpu_va32);
		printf("submit[%u]_ccb_status_gpu_va32=0x%x\n", i,
		       submit.ccb_status_gpu_va32);
		printf("submit[%u]_control_object_id=%u\n", i,
		       submit.control_object_id);
		printf("submit[%u]_status_object_id=%u\n", i,
		       submit.status_object_id);
		printf("submit[%u]_control_gpu_va=0x%08" PRIx64 "\n", i,
		       (uint64_t)submit.control_gpu_va);
		printf("submit[%u]_status_gpu_va=0x%08" PRIx64 "\n", i,
		       (uint64_t)submit.status_gpu_va);
	}
}

static void print_dump_json(
	int fd,
	const char *path,
	const char *mode,
	const struct drm_psb_sgx_lab_coord_query *coord,
	const struct drm_psb_sgx_lab_gtt_state_query *gtt,
	const struct drm_psb_sgx_lab_sgx_state_query *sgx)
{
	uint32_t i;

	printf("{\n");
	printf("  \"drm_path\": ");
	print_json_string(path);
	printf(",\n");
	printf("  \"dump_mode\": ");
	print_json_string(mode);
	printf(",\n");
	printf("  \"coord\": {\n");
	printf("    \"focus_valid\": %u,\n", coord->focus_valid);
	printf("    \"focused_submit_id\": %u,\n", coord->focused_submit_id);
	printf("    \"live_submit_count\": %u,\n", coord->live_submit_count);
	printf("    \"live_object_count\": %u,\n", coord->live_object_count);
	printf("    \"issue_ticket\": %u,\n", coord->issue_ticket);
	printf("    \"running_ticket\": %u,\n", coord->running_ticket);
	printf("    \"complete_ticket\": %u,\n", coord->complete_ticket);
	printf("    \"issue_submit_id\": %u,\n", coord->issue_submit_id);
	printf("    \"running_submit_id\": %u,\n", coord->running_submit_id);
	printf("    \"complete_submit_id\": %u\n", coord->complete_submit_id);
	printf("  },\n");
	printf("  \"gtt_state\": {\n");
	printf("    \"range_total_count\": %u,\n", gtt->range_total_count);
	printf("    \"range_gem_count\": %u,\n", gtt->range_gem_count);
	printf("    \"range_sgx_object_count\": %u,\n",
	       gtt->range_sgx_object_count);
	printf("    \"range_pf_bound_count\": %u,\n",
	       gtt->range_pf_bound_count);
	printf("    \"mmu_last_op_name\": ");
	print_json_string(mmu_op_name(gtt->mmu_last_op));
	printf(",\n");
	printf("    \"mmu_insert_pages_count\": %u,\n",
	       gtt->mmu_insert_pages_count);
	printf("    \"mmu_remove_pages_count\": %u\n",
	       gtt->mmu_remove_pages_count);
	printf("  },\n");
	printf("  \"sgx_state\": {\n");
	printf("    \"reason\": ");
	print_json_string(sgx->reason[0] ? (const char *)sgx->reason : "none");
	printf(",\n");
	printf("    \"sequence\": %u,\n", sgx->sequence);
	printf("    \"capture_failed\": %u,\n", sgx->capture_failed);
	printf("    \"irq_count\": %u,\n", sgx->irq_count);
	printf("    \"fault_count\": %u,\n", sgx->fault_count);
	printf("    \"bif_fault\": %u,\n", sgx->bif_fault);
	printf("    \"twod_blit_status\": %u\n", sgx->twod_blit_status);
	printf("  },\n");
	printf("  \"objects\": [\n");
	for (i = 0; i < coord->live_object_count; i++) {
		struct drm_psb_sgx_lab_object_query object;
		int ret;
		int saved_errno;

		errno = 0;
		ret = query_object_index(fd, i, &object);
		saved_errno = errno;
		printf("    {\n");
		printf("      \"index\": %u,\n", i);
		printf("      \"ioctl_ret\": %d,\n", ret);
		printf("      \"errno\": %d", saved_errno);
		if (ret == 0) {
			printf(",\n      \"id\": %u,\n", object.resolved_object_id);
			printf("      \"range_id\": %u,\n", object.range_id);
			printf("      \"range_type_name\": ");
			print_json_string(range_type_name(object.range_type));
			printf(",\n      \"role_name\": ");
			print_json_string(role_name(object.role));
			printf(",\n      \"class_name\": ");
			print_json_string(class_name(object.object_class));
			printf(",\n      \"gpu_use_name\": ");
			print_json_string(gpu_use_name(object.gpu_use));
			printf(",\n      \"context_name\": ");
			print_json_string(context_name(object.expected_context));
			printf(",\n      \"gpu_va\": %" PRIu64 ",\n",
			       (uint64_t)object.gpu_va);
			printf("      \"pf_bind_count\": %u\n",
			       object.pf_bind_count);
		} else {
			printf("\n");
		}
		printf("    }%s\n",
		       (i + 1 < coord->live_object_count) ? "," : "");
	}
	printf("  ],\n");
	printf("  \"submits\": [\n");
	for (i = 0; i < coord->live_submit_count; i++) {
		struct drm_psb_sgx_lab_submit_query submit;
		int ret;
		int saved_errno;

		errno = 0;
		ret = query_submit_index(fd, i, &submit);
		saved_errno = errno;
		printf("    {\n");
		printf("      \"index\": %u,\n", i);
		printf("      \"ioctl_ret\": %d,\n", ret);
		printf("      \"errno\": %d", saved_errno);
		if (ret == 0) {
			printf(",\n      \"id\": %u,\n", submit.resolved_submit_id);
			printf("      \"context_name\": ");
			print_json_string(context_name(submit.context));
			printf(",\n      \"cmd_type_name\": ");
			print_json_string(cmd_type_name(submit.cmd_type));
			printf(",\n      \"owner_state_name\": ");
			print_json_string(owner_state_name(submit.owner_state));
			printf(",\n      \"prepare_count\": %u,\n",
			       submit.prepare_count);
			printf("      \"arm_valid\": %u,\n", submit.arm_valid);
			printf("      \"arm_count\": %u,\n", submit.arm_count);
			printf("      \"arm_owner_state_name\": ");
			print_json_string(owner_state_name(
				submit.arm_owner_state));
			printf(",\n      \"arm_queue_packet_count\": %u,\n",
			       submit.arm_queue_packet_count);
			printf("      \"arm_sync_pending_seq\": %u,\n",
			       submit.arm_sync_pending_seq);
			printf("      \"emit_valid\": %u,\n", submit.emit_valid);
			printf("      \"emit_count\": %u,\n", submit.emit_count);
			printf("      \"emit_cmd_type_name\": ");
			print_json_string(cmd_type_name(submit.emit_cmd_type));
			printf(",\n      \"emit_owner_state_name\": ");
			print_json_string(owner_state_name(
				submit.emit_owner_state));
			printf(",\n      \"emit_queue_packet_count\": %u,\n",
			       submit.emit_queue_packet_count);
			printf("      \"emit_sync_pending_seq\": %u,\n",
			       submit.emit_sync_pending_seq);
			printf("      \"emit_global_issue_ticket\": %u,\n",
			       submit.emit_global_issue_ticket);
			printf("      \"emit_dependency_active_count\": %u,\n",
			       submit.emit_dependency_active_count);
			printf("      \"emit_control_object_id\": %u,\n",
			       submit.emit_control_object_id);
			printf("      \"emit_status_object_id\": %u,\n",
			       submit.emit_status_object_id);
			printf("      \"emit_control_gpu_va\": %" PRIu64 ",\n",
			       (uint64_t)submit.emit_control_gpu_va);
			printf("      \"emit_status_gpu_va\": %" PRIu64 ",\n",
			       (uint64_t)submit.emit_status_gpu_va);
			printf("      \"launch_valid\": %u,\n",
			       submit.launch_valid);
			printf("      \"launch_consumed\": %u,\n",
			       submit.launch_consumed);
			printf("      \"launch_count\": %u,\n",
			       submit.launch_count);
			printf("      \"launch_owner_state_name\": ");
			print_json_string(owner_state_name(
				submit.launch_owner_state));
			printf(",\n      \"launch_issued_packets\": %u,\n",
			       submit.launch_issued_packets);
			printf("      \"launch_queue_packet_count\": %u,\n",
			       submit.launch_queue_packet_count);
			printf("      \"launch_new_write_offset\": %u,\n",
			       submit.launch_new_write_offset);
			printf("      \"launch_global_issue_ticket\": %u,\n",
			       submit.launch_global_issue_ticket);
			printf("      \"launch_dependency_active_count\": %u,\n",
			       submit.launch_dependency_active_count);
			printf("      \"launch_control_object_id\": %u,\n",
			       submit.launch_control_object_id);
			printf("      \"launch_status_object_id\": %u,\n",
			       submit.launch_status_object_id);
			printf("      \"launch_control_gpu_va\": %" PRIu64 ",\n",
			       (uint64_t)submit.launch_control_gpu_va);
			printf("      \"launch_status_gpu_va\": %" PRIu64 ",\n",
			       (uint64_t)submit.launch_status_gpu_va);
			printf("      \"dispatch_valid\": %u,\n",
			       submit.dispatch_valid);
			printf("      \"dispatch_count\": %u,\n",
			       submit.dispatch_count);
			printf("      \"dispatch_owner_state_name\": ");
			print_json_string(owner_state_name(
				submit.dispatch_owner_state));
			printf(",\n      \"dispatch_issued_packets\": %u,\n",
			       submit.dispatch_issued_packets);
			printf("      \"dispatch_completed_packets\": %u,\n",
			       submit.dispatch_completed_packets);
			printf("      \"dispatch_queue_packet_count\": %u,\n",
			       submit.dispatch_queue_packet_count);
			printf("      \"dispatch_sync_complete_seq\": %u,\n",
			       submit.dispatch_sync_complete_seq);
			printf("      \"dispatch_sync_pending_seq\": %u,\n",
			       submit.dispatch_sync_pending_seq);
			printf("      \"dispatch_global_issue_ticket\": %u,\n",
			       submit.dispatch_global_issue_ticket);
			printf("      \"dispatch_global_running_ticket\": %u,\n",
			       submit.dispatch_global_running_ticket);
			printf("      \"dispatch_global_complete_ticket\": %u,\n",
			       submit.dispatch_global_complete_ticket);
			printf("      \"dispatch_dependency_active_count\": %u,\n",
			       submit.dispatch_dependency_active_count);
			printf("      \"dispatch_control_object_id\": %u,\n",
			       submit.dispatch_control_object_id);
			printf("      \"dispatch_status_object_id\": %u,\n",
			       submit.dispatch_status_object_id);
			printf("      \"dispatch_control_gpu_va\": %" PRIu64 ",\n",
			       (uint64_t)submit.dispatch_control_gpu_va);
			printf("      \"dispatch_status_gpu_va\": %" PRIu64 ",\n",
			       (uint64_t)submit.dispatch_status_gpu_va);
			printf("      \"retired_valid\": %u,\n",
			       submit.retired_valid);
			printf("      \"retired_count\": %u,\n",
			       submit.retired_count);
			printf("      \"retired_dispatch_count\": %u,\n",
			       submit.retired_dispatch_count);
			printf("      \"retired_owner_state_name\": ");
			print_json_string(owner_state_name(
				submit.retired_owner_state));
			printf(",\n      \"retired_issued_packets\": %u,\n",
			       submit.retired_issued_packets);
			printf("      \"retired_completed_packets\": %u,\n",
			       submit.retired_completed_packets);
			printf("      \"retired_queue_packet_count\": %u,\n",
			       submit.retired_queue_packet_count);
			printf("      \"retired_sync_complete_seq\": %u,\n",
			       submit.retired_sync_complete_seq);
			printf("      \"retired_sync_pending_seq\": %u,\n",
			       submit.retired_sync_pending_seq);
			printf("      \"retired_global_issue_ticket\": %u,\n",
			       submit.retired_global_issue_ticket);
			printf("      \"retired_global_running_ticket\": %u,\n",
			       submit.retired_global_running_ticket);
			printf("      \"retired_global_complete_ticket\": %u,\n",
			       submit.retired_global_complete_ticket);
			printf("      \"retired_dependency_active_count\": %u,\n",
			       submit.retired_dependency_active_count);
			printf("      \"retired_control_object_id\": %u,\n",
			       submit.retired_control_object_id);
			printf("      \"retired_status_object_id\": %u,\n",
			       submit.retired_status_object_id);
			printf("      \"retired_control_gpu_va\": %" PRIu64 ",\n",
			       (uint64_t)submit.retired_control_gpu_va);
			printf("      \"retired_status_gpu_va\": %" PRIu64 ",\n",
			       (uint64_t)submit.retired_status_gpu_va);
			printf("      \"report_offset\": %u,\n",
			       submit.report_offset);
			printf("      \"report_stage_name\": ");
			print_json_string(report_stage_name(submit.report_stage));
			printf(",\n      \"report_stage_count\": %u,\n",
			       submit.report_stage_count);
			printf("      \"report_owner_state_name\": ");
			print_json_string(owner_state_name(
				submit.report_owner_state));
			printf(",\n      \"report_queue_packet_count\": %u,\n",
			       submit.report_queue_packet_count);
			printf("      \"report_completed_packets\": %u,\n",
			       submit.report_completed_packets);
			printf("      \"report_dependency_active_count\": %u,\n",
			       submit.report_dependency_active_count);
			printf("      \"report_queue_epoch\": %u,\n",
			       submit.report_queue_epoch);
			printf("      \"report_global_issue_ticket\": %u,\n",
			       submit.report_global_issue_ticket);
			printf("      \"report_global_running_ticket\": %u,\n",
			       submit.report_global_running_ticket);
			printf("      \"report_global_complete_ticket\": %u,\n",
			       submit.report_global_complete_ticket);
			printf("      \"report_flags\": %u,\n",
			       submit.report_flags);
			printf("      \"report_config_count\": %u,\n",
			       submit.report_config_count);
			printf("      \"result_offset\": %u,\n",
			       submit.result_offset);
			printf("      \"result_stage_name\": ");
			print_json_string(report_stage_name(submit.result_stage));
			printf(",\n      \"result_stage_count\": %u,\n",
			       submit.result_stage_count);
			printf("      \"result_source_name\": ");
			print_json_string(result_source_name(submit.result_source));
			printf(",\n      \"result_context_name\": ");
			print_json_string(context_name(submit.result_context));
			printf(",\n      \"result_cmd_type_name\": ");
			print_json_string(cmd_type_name(submit.result_cmd_type));
			printf(",\n      \"result_owner_state_name\": ");
			print_json_string(owner_state_name(
				submit.result_owner_state));
			printf(",\n      \"result_issued_packets\": %u,\n",
			       submit.result_issued_packets);
			printf("      \"result_completed_packets\": %u,\n",
			       submit.result_completed_packets);
			printf("      \"result_dependency_active_count\": %u,\n",
			       submit.result_dependency_active_count);
			printf("      \"result_queue_packet_count\": %u,\n",
			       submit.result_queue_packet_count);
			printf("      \"result_queue_epoch\": %u,\n",
			       submit.result_queue_epoch);
			printf("      \"result_sync_complete_seq\": %u,\n",
			       submit.result_sync_complete_seq);
			printf("      \"result_sync_pending_seq\": %u,\n",
			       submit.result_sync_pending_seq);
			printf("      \"result_sync_flags\": %u,\n",
			       submit.result_sync_flags);
			printf("      \"result_owner_issue_cookie\": %u,\n",
			       submit.result_owner_issue_cookie);
			printf("      \"result_owner_complete_cookie\": %u,\n",
			       submit.result_owner_complete_cookie);
			printf("      \"result_global_issue_ticket\": %u,\n",
			       submit.result_global_issue_ticket);
			printf("      \"result_global_running_ticket\": %u,\n",
			       submit.result_global_running_ticket);
			printf("      \"result_global_complete_ticket\": %u,\n",
			       submit.result_global_complete_ticket);
			printf("      \"result_ctrl_flags\": %u,\n",
			       submit.result_ctrl_flags);
			printf("      \"result_num_ta_status_vals\": %u,\n",
			       submit.result_num_ta_status_vals);
			printf("      \"result_num_3d_status_vals\": %u,\n",
			       submit.result_num_3d_status_vals);
			printf("      \"result_ta_dependency\": %u,\n",
			       submit.result_ta_dependency);
			printf("      \"result_status_ta_offset\": %u,\n",
			       submit.result_status_ta_offset);
			printf("      \"result_status_3d_offset\": %u,\n",
			       submit.result_status_3d_offset);
			printf("      \"result_status_stride\": %u,\n",
			       submit.result_status_stride);
			printf("      \"result_status_value_seed\": %u,\n",
			       submit.result_status_value_seed);
			printf("      \"result_control_object_id\": %u,\n",
			       submit.result_control_object_id);
			printf("      \"result_status_object_id\": %u,\n",
			       submit.result_status_object_id);
			printf("      \"result_control_gpu_va32\": %u,\n",
			       submit.result_control_gpu_va32);
			printf("      \"result_status_gpu_va32\": %u,\n",
			       submit.result_status_gpu_va32);
			printf("      \"result_report_offset\": %u,\n",
			       submit.result_report_offset);
			printf("      \"result_manifest_offset\": %u,\n",
			       submit.result_manifest_offset);
			printf("      \"result_descriptor_offset\": %u,\n",
			       submit.result_descriptor_offset);
			printf("      \"result_plan_offset\": %u,\n",
			       submit.result_plan_offset);
			printf("      \"result_config_count\": %u,\n",
			       submit.result_config_count);
			printf("      \"manifest_offset\": %u,\n",
			       submit.manifest_offset);
			printf("      \"manifest_stage_name\": ");
			print_json_string(report_stage_name(submit.manifest_stage));
			printf(",\n      \"manifest_stage_count\": %u,\n",
			       submit.manifest_stage_count);
			printf("      \"manifest_context_name\": ");
			print_json_string(context_name(submit.manifest_context));
			printf(",\n      \"manifest_cmd_type_name\": ");
			print_json_string(cmd_type_name(submit.manifest_cmd_type));
			printf(",\n      \"manifest_owner_state_name\": ");
			print_json_string(owner_state_name(
				submit.manifest_owner_state));
			printf(",\n      \"manifest_ctrl_flags\": %u,\n",
			       submit.manifest_ctrl_flags);
			printf("      \"manifest_queue_packet_count\": %u,\n",
			       submit.manifest_queue_packet_count);
			printf("      \"manifest_queue_epoch\": %u,\n",
			       submit.manifest_queue_epoch);
			printf("      \"manifest_ccb_offset\": %u,\n",
			       submit.manifest_ccb_offset);
			printf("      \"manifest_ccb_write_offset\": %u,\n",
			       submit.manifest_ccb_write_offset);
			printf("      \"manifest_ccb_read_offset\": %u,\n",
			       submit.manifest_ccb_read_offset);
			printf("      \"manifest_dependency_active_count\": %u,\n",
			       submit.manifest_dependency_active_count);
			printf("      \"manifest_global_issue_ticket\": %u,\n",
			       submit.manifest_global_issue_ticket);
			printf("      \"manifest_global_running_ticket\": %u,\n",
			       submit.manifest_global_running_ticket);
			printf("      \"manifest_global_complete_ticket\": %u,\n",
			       submit.manifest_global_complete_ticket);
			printf("      \"manifest_pds_gpu_va32\": %u,\n",
			       submit.manifest_pds_gpu_va32);
			printf("      \"manifest_use_gpu_va32\": %u,\n",
			       submit.manifest_use_gpu_va32);
			printf("      \"manifest_target_gpu_va32\": %u,\n",
			       submit.manifest_target_gpu_va32);
			printf("      \"manifest_status_gpu_va32\": %u,\n",
			       submit.manifest_status_gpu_va32);
			printf("      \"manifest_sync_complete_seq\": %u,\n",
			       submit.manifest_sync_complete_seq);
			printf("      \"manifest_sync_pending_seq\": %u,\n",
			       submit.manifest_sync_pending_seq);
			printf("      \"manifest_owner_issue_cookie\": %u,\n",
			       submit.manifest_owner_issue_cookie);
			printf("      \"manifest_owner_complete_cookie\": %u,\n",
			       submit.manifest_owner_complete_cookie);
			printf("      \"manifest_config_count\": %u,\n",
			       submit.manifest_config_count);
			printf("      \"descriptor_offset\": %u,\n",
			       submit.descriptor_offset);
			printf("      \"descriptor_stage_name\": ");
			print_json_string(
				report_stage_name(submit.descriptor_stage));
			printf(",\n      \"descriptor_stage_count\": %u,\n",
			       submit.descriptor_stage_count);
			printf("      \"descriptor_context_name\": ");
			print_json_string(
				context_name(submit.descriptor_context));
			printf(",\n      \"descriptor_cmd_type_name\": ");
			print_json_string(
				cmd_type_name(submit.descriptor_cmd_type));
			printf(",\n      \"descriptor_owner_state_name\": ");
			print_json_string(
				owner_state_name(submit.descriptor_owner_state));
			printf(",\n      \"descriptor_issued_packets\": %u,\n",
			       submit.descriptor_issued_packets);
			printf("      \"descriptor_completed_packets\": %u,\n",
			       submit.descriptor_completed_packets);
			printf("      \"descriptor_dependency_active_count\": %u,\n",
			       submit.descriptor_dependency_active_count);
			printf("      \"descriptor_queue_packet_count\": %u,\n",
			       submit.descriptor_queue_packet_count);
			printf("      \"descriptor_queue_epoch\": %u,\n",
			       submit.descriptor_queue_epoch);
			printf("      \"descriptor_ctrl_flags\": %u,\n",
			       submit.descriptor_ctrl_flags);
			printf("      \"descriptor_queue_base_offset\": %u,\n",
			       submit.descriptor_queue_base_offset);
			printf("      \"descriptor_queue_size\": %u,\n",
			       submit.descriptor_queue_size);
			printf("      \"descriptor_queue_packet_size\": %u,\n",
			       submit.descriptor_queue_packet_size);
			printf("      \"descriptor_ccb_offset\": %u,\n",
			       submit.descriptor_ccb_offset);
			printf("      \"descriptor_sync_offset\": %u,\n",
			       submit.descriptor_sync_offset);
			printf("      \"descriptor_owner_offset\": %u,\n",
			       submit.descriptor_owner_offset);
			printf("      \"descriptor_report_offset\": %u,\n",
			       submit.descriptor_report_offset);
			printf("      \"descriptor_manifest_offset\": %u,\n",
			       submit.descriptor_manifest_offset);
			printf("      \"descriptor_global_issue_ticket\": %u,\n",
			       submit.descriptor_global_issue_ticket);
			printf("      \"descriptor_global_running_ticket\": %u,\n",
			       submit.descriptor_global_running_ticket);
			printf("      \"descriptor_global_complete_ticket\": %u,\n",
			       submit.descriptor_global_complete_ticket);
			printf("      \"descriptor_report_flags\": %u,\n",
			       submit.descriptor_report_flags);
			printf("      \"descriptor_status_ta_offset\": %u,\n",
			       submit.descriptor_status_ta_offset);
			printf("      \"descriptor_status_3d_offset\": %u,\n",
			       submit.descriptor_status_3d_offset);
			printf("      \"descriptor_status_stride\": %u,\n",
			       submit.descriptor_status_stride);
			printf("      \"descriptor_status_value_seed\": %u,\n",
			       submit.descriptor_status_value_seed);
			printf("      \"descriptor_control_gpu_va32\": %u,\n",
			       submit.descriptor_control_gpu_va32);
			printf("      \"descriptor_pds_gpu_va32\": %u,\n",
			       submit.descriptor_pds_gpu_va32);
			printf("      \"descriptor_use_gpu_va32\": %u,\n",
			       submit.descriptor_use_gpu_va32);
			printf("      \"descriptor_target_gpu_va32\": %u,\n",
			       submit.descriptor_target_gpu_va32);
			printf("      \"descriptor_status_gpu_va32\": %u,\n",
			       submit.descriptor_status_gpu_va32);
			printf("      \"descriptor_config_count\": %u,\n",
			       submit.descriptor_config_count);
			printf("      \"plan_offset\": %u,\n",
			       submit.plan_offset);
			printf("      \"plan_stage_name\": ");
			print_json_string(report_stage_name(submit.plan_stage));
			printf(",\n      \"plan_stage_count\": %u,\n",
			       submit.plan_stage_count);
			printf("      \"plan_source_name\": ");
			print_json_string(plan_source_name(submit.plan_source));
			printf(",\n      \"plan_context_name\": ");
			print_json_string(context_name(submit.plan_context));
			printf(",\n      \"plan_cmd_type_name\": ");
			print_json_string(cmd_type_name(submit.plan_cmd_type));
			printf(",\n      \"plan_owner_state_name\": ");
			print_json_string(
				owner_state_name(submit.plan_owner_state));
			printf(",\n      \"plan_issued_packets\": %u,\n",
			       submit.plan_issued_packets);
			printf("      \"plan_completed_packets\": %u,\n",
			       submit.plan_completed_packets);
			printf("      \"plan_dependency_active_count\": %u,\n",
			       submit.plan_dependency_active_count);
			printf("      \"plan_refilled_from_complete\": %u,\n",
			       submit.plan_refilled_from_complete);
			printf("      \"plan_queue_packet_count\": %u,\n",
			       submit.plan_queue_packet_count);
			printf("      \"plan_queue_epoch\": %u,\n",
			       submit.plan_queue_epoch);
			printf("      \"plan_queue_base_offset\": %u,\n",
			       submit.plan_queue_base_offset);
			printf("      \"plan_queue_size\": %u,\n",
			       submit.plan_queue_size);
			printf("      \"plan_queue_packet_size\": %u,\n",
			       submit.plan_queue_packet_size);
			printf("      \"plan_ccb_offset\": %u,\n",
			       submit.plan_ccb_offset);
			printf("      \"plan_ccb_write_offset\": %u,\n",
			       submit.plan_ccb_write_offset);
			printf("      \"plan_ccb_read_offset\": %u,\n",
			       submit.plan_ccb_read_offset);
			printf("      \"plan_sync_offset\": %u,\n",
			       submit.plan_sync_offset);
			printf("      \"plan_sync_complete_seq\": %u,\n",
			       submit.plan_sync_complete_seq);
			printf("      \"plan_sync_pending_seq\": %u,\n",
			       submit.plan_sync_pending_seq);
			printf("      \"plan_sync_flags\": %u,\n",
			       submit.plan_sync_flags);
			printf("      \"plan_owner_offset\": %u,\n",
			       submit.plan_owner_offset);
			printf("      \"plan_owner_issue_cookie\": %u,\n",
			       submit.plan_owner_issue_cookie);
			printf("      \"plan_owner_complete_cookie\": %u,\n",
			       submit.plan_owner_complete_cookie);
			printf("      \"plan_global_issue_ticket\": %u,\n",
			       submit.plan_global_issue_ticket);
			printf("      \"plan_global_running_ticket\": %u,\n",
			       submit.plan_global_running_ticket);
			printf("      \"plan_global_complete_ticket\": %u,\n",
			       submit.plan_global_complete_ticket);
			printf("      \"plan_report_offset\": %u,\n",
			       submit.plan_report_offset);
			printf("      \"plan_manifest_offset\": %u,\n",
			       submit.plan_manifest_offset);
			printf("      \"plan_descriptor_offset\": %u,\n",
			       submit.plan_descriptor_offset);
			printf("      \"plan_ctrl_flags\": %u,\n",
			       submit.plan_ctrl_flags);
			printf("      \"plan_ccb_service_address\": %u,\n",
			       submit.plan_ccb_service_address);
			printf("      \"plan_ccb_cache_control\": %u,\n",
			       submit.plan_ccb_cache_control);
			printf("      \"plan_ccb_data0\": %u,\n",
			       submit.plan_ccb_data0);
			printf("      \"plan_ccb_data1\": %u,\n",
			       submit.plan_ccb_data1);
			printf("      \"plan_control_gpu_va32\": %u,\n",
			       submit.plan_control_gpu_va32);
			printf("      \"plan_pds_gpu_va32\": %u,\n",
			       submit.plan_pds_gpu_va32);
			printf("      \"plan_use_gpu_va32\": %u,\n",
			       submit.plan_use_gpu_va32);
			printf("      \"plan_target_gpu_va32\": %u,\n",
			       submit.plan_target_gpu_va32);
			printf("      \"plan_status_gpu_va32\": %u,\n",
			       submit.plan_status_gpu_va32);
			printf("      \"plan_config_count\": %u,\n",
			       submit.plan_config_count);
			printf("      \"issue_count\": %u,\n", submit.issue_count);
			printf("      \"validate_count\": %u,\n",
			       submit.validate_count);
			printf("      \"transition_count\": %u,\n",
			       submit.transition_count);
			printf("      \"queue_packet_count\": %u,\n",
			       submit.queue_packet_count);
			printf("      \"sync_pending_seq\": %u,\n",
			       submit.sync_pending_seq);
			printf("      \"owner_issue_cookie\": %u,\n",
			       submit.owner_issue_cookie);
			printf("      \"global_issue_ticket\": %u,\n",
			       submit.global_issue_ticket);
			printf("      \"ccb_pds_gpu_va32\": %u,\n",
			       submit.ccb_pds_gpu_va32);
			printf("      \"ccb_use_gpu_va32\": %u,\n",
			       submit.ccb_use_gpu_va32);
			printf("      \"ccb_target_gpu_va32\": %u,\n",
			       submit.ccb_target_gpu_va32);
			printf("      \"ccb_status_gpu_va32\": %u,\n",
			       submit.ccb_status_gpu_va32);
			printf("      \"control_object_id\": %u,\n",
			       submit.control_object_id);
			printf("      \"status_object_id\": %u,\n",
			       submit.status_object_id);
			printf("      \"control_gpu_va\": %" PRIu64 ",\n",
			       (uint64_t)submit.control_gpu_va);
			printf("      \"status_gpu_va\": %" PRIu64 "\n",
			       (uint64_t)submit.status_gpu_va);
		} else {
			printf("\n");
		}
		printf("    }%s\n",
		       (i + 1 < coord->live_submit_count) ? "," : "");
	}
	printf("  ]\n");
	printf("}\n");
}

int main(int argc, char **argv)
{
	const char *path = "/dev/dri/card0";
	const char *mode = "cached";
	struct drm_psb_sgx_lab_coord_query coord;
	struct drm_psb_sgx_lab_gtt_state_query gtt;
	struct drm_psb_sgx_lab_sgx_state_query sgx;
	int capture = 0;
	int json_mode = 0;
	int fd;
	int ret;
	int argi = 1;

	if (argc > 1 && strncmp(argv[1], "/dev/", 5) == 0) {
		path = argv[1];
		argi++;
	}

	if (argc - argi > 1) {
		usage(argv[0]);
		return 2;
	}

	if (argc - argi == 1)
		mode = argv[argi++];

	if (strcmp(mode, "capture") == 0) {
		capture = 1;
	} else if (strcmp(mode, "json") == 0) {
		json_mode = 1;
	} else if (strcmp(mode, "capture-json") == 0) {
		capture = 1;
		json_mode = 1;
	} else if (strcmp(mode, "cached") != 0) {
		fprintf(stderr, "invalid mode: %s\n", mode);
		usage(argv[0]);
		return 2;
	}

	fd = open(path, O_RDWR);
	if (!json_mode) {
		printf("drm_path=%s\n", path);
		printf("dump_mode=%s\n", mode);
		printf("drm_open_result=%d\n", fd);
	}
	if (fd < 0) {
		if (!json_mode)
			printf("drm_open_errno=%d\n", errno);
		return 1;
	}

	errno = 0;
	ret = query_coord(fd, &coord);
	if (ret != 0) {
		if (!json_mode)
			printf("coord_ioctl_ret=%d errno=%d\n", ret, errno);
		close(fd);
		return 1;
	}

	errno = 0;
	ret = query_gtt_state(fd, &gtt);
	if (ret != 0) {
		if (!json_mode)
			printf("gtt_state_ioctl_ret=%d errno=%d\n", ret, errno);
		close(fd);
		return 1;
	}

	errno = 0;
	ret = query_sgx_state(fd, capture, &sgx);
	if (ret != 0) {
		if (!json_mode)
			printf("sgx_state_ioctl_ret=%d errno=%d\n", ret, errno);
		close(fd);
		return 1;
	}

	if (json_mode)
		print_dump_json(fd, path, mode, &coord, &gtt, &sgx);
	else {
		printf("coord_ioctl_ret=0 errno=0\n");
		printf("gtt_state_ioctl_ret=0 errno=0\n");
		printf("sgx_state_ioctl_ret=0 errno=0\n");
		print_dump_text(fd, &coord, &gtt, &sgx);
	}

	close(fd);
	return 0;
}
