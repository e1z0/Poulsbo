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

static const char *context_name(uint32_t context)
{
	switch (context) {
	case DRM_PSB_SGX_LAB_CONTEXT_DEFAULT:
		return "default";
	case DRM_PSB_SGX_LAB_CONTEXT_PF:
		return "pf";
	case DRM_PSB_SGX_LAB_CONTEXT_AUTO:
	default:
		return "auto";
	}
}

static const char *submit_slot_name(uint32_t slot)
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

static int parse_u32(const char *arg, uint32_t *out)
{
	char *end = NULL;
	unsigned long value;

	errno = 0;
	value = strtoul(arg, &end, 0);
	if (errno || !end || *end != '\0' || value > UINT32_MAX)
		return -1;
	*out = (uint32_t)value;
	return 0;
}

static int parse_u64(const char *arg, uint64_t *out)
{
	char *end = NULL;
	unsigned long long value;

	errno = 0;
	value = strtoull(arg, &end, 0);
	if (errno || !end || *end != '\0')
		return -1;
	*out = (uint64_t)value;
	return 0;
}

static void usage(const char *argv0)
{
	fprintf(stderr,
		"usage: %s [/dev/dri/card0] <index|id|gpuva|gpuva-submit> <value> [submit_match_index|gem] [gem]\n",
		argv0);
}

int main(int argc, char **argv)
{
	const char *path = "/dev/dri/card0";
	const char *mode;
	const char *value_arg;
	const char *submit_match_arg = NULL;
	const char *filter_arg = NULL;
	struct drm_psb_sgx_lab_range_query query;
	int fd;
	int ret;
	int argi = 1;

	if (argc > 1 && strncmp(argv[1], "/dev/", 5) == 0) {
		path = argv[1];
		argi++;
	}

	if (argc - argi < 2 || argc - argi > 4) {
		usage(argv[0]);
		return 2;
	}

	mode = argv[argi++];
	value_arg = argv[argi++];
	if (strcmp(mode, "gpuva-submit") == 0) {
		if (argc - argi < 1 || argc - argi > 2) {
			usage(argv[0]);
			return 2;
		}
		submit_match_arg = argv[argi++];
		if (argc - argi == 1)
			filter_arg = argv[argi++];
	} else if (argc - argi == 1) {
		filter_arg = argv[argi++];
	}

	memset(&query, 0, sizeof(query));
	query.size = sizeof(query);

	if (strcmp(mode, "index") == 0) {
		if (parse_u32(value_arg, &query.query_index) != 0) {
			fprintf(stderr, "invalid query index: %s\n", value_arg);
			return 2;
		}
	} else if (strcmp(mode, "id") == 0) {
		if (parse_u32(value_arg, &query.range_id) != 0) {
			fprintf(stderr, "invalid range id: %s\n", value_arg);
			return 2;
		}
		query.flags |= DRM_PSB_SGX_LAB_QUERY_RANGE_USE_ID;
	} else if (strcmp(mode, "gpuva") == 0) {
		if (parse_u64(value_arg, &query.query_gpu_va) != 0) {
			fprintf(stderr, "invalid gpu va: %s\n", value_arg);
			return 2;
		}
		query.flags |= DRM_PSB_SGX_LAB_QUERY_RANGE_USE_GPU_VA;
	} else if (strcmp(mode, "gpuva-submit") == 0) {
		if (parse_u64(value_arg, &query.query_gpu_va) != 0) {
			fprintf(stderr, "invalid gpu va: %s\n", value_arg);
			return 2;
		}
		if (parse_u32(submit_match_arg,
			      &query.query_submit_match_index) != 0) {
			fprintf(stderr, "invalid submit match index: %s\n",
				submit_match_arg);
			return 2;
		}
		query.flags |= DRM_PSB_SGX_LAB_QUERY_RANGE_USE_GPU_VA;
		query.flags |= DRM_PSB_SGX_LAB_QUERY_RANGE_USE_SUBMIT_MATCH_INDEX;
	} else {
		fprintf(stderr, "invalid mode: %s\n", mode);
		usage(argv[0]);
		return 2;
	}

	if (filter_arg) {
		if (strcmp(filter_arg, "gem") == 0) {
			query.flags |= DRM_PSB_SGX_LAB_QUERY_RANGE_FILTER_GEM;
		} else {
			fprintf(stderr, "invalid filter: %s\n", filter_arg);
			usage(argv[0]);
			return 2;
		}
	}

	fd = open(path, O_RDWR);
	printf("drm_path=%s\n", path);
	printf("range_query_mode=%s\n", mode);
	printf("range_query_value=%s\n", value_arg);
	if (submit_match_arg)
		printf("range_query_submit_match_index=%s\n",
		       submit_match_arg);
	printf("range_query_filter_gem=%u\n",
	       (query.flags & DRM_PSB_SGX_LAB_QUERY_RANGE_FILTER_GEM) ? 1 : 0);
	printf("drm_open_result=%d\n", fd);
	if (fd < 0) {
		printf("drm_open_errno=%d\n", errno);
		return 1;
	}

	errno = 0;
	/*
	 * Use raw ioctl() so future lab-only range-query error semantics are not
	 * hidden by libdrm retry behavior.
	 */
	ret = ioctl(fd, DRM_IOCTL_PSB_SGX_LAB_QUERY_RANGE, &query);
	printf("range_ioctl_ret=%d errno=%d\n", ret, errno);
	if (ret == 0) {
		printf("range_resolved_id=%u\n", query.resolved_range_id);
		printf("range_resolved_index=%u\n", query.resolved_index);
		printf("range_type=%u\n", query.range_type);
		printf("range_type_name=%s\n", range_type_name(query.range_type));
		printf("range_total_count=%u\n", query.total_range_count);
		printf("range_matching_count=%u\n", query.matching_range_count);
		printf("range_start=0x%08" PRIx64 "\n", (uint64_t)query.start);
		printf("range_end=0x%08" PRIx64 "\n", (uint64_t)query.end);
		printf("range_len=0x%08" PRIx64 "\n", (uint64_t)query.len);
		printf("range_query_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)query.query_gpu_va);
		printf("range_query_gpu_va_offset=0x%08" PRIx64 "\n",
		       (uint64_t)query.query_gpu_va_offset);
		printf("range_gpu_va=0x%08" PRIx64 "\n", (uint64_t)query.gpu_va);
		printf("range_offset=0x%x\n", query.offset);
		printf("range_stolen=%u\n", query.stolen);
		printf("range_in_gart=%d\n", query.in_gart);
		printf("range_mmapping=%u\n", query.mmapping);
		printf("range_npage=%d\n", query.npage);
		printf("range_roll=%d\n", query.roll);
		printf("range_sgx_object_id=%u\n", query.sgx_object_id);
		printf("range_sgx_object_role=%u\n", query.sgx_object_role);
		printf("range_sgx_object_role_name=%s\n",
		       role_name(query.sgx_object_role));
		printf("range_sgx_object_class=%u\n", query.sgx_object_class);
		printf("range_sgx_object_class_name=%s\n",
		       class_name(query.sgx_object_class));
		printf("range_sgx_object_gpu_use=%u\n",
		       query.sgx_object_gpu_use);
		printf("range_sgx_object_gpu_use_name=%s\n",
		       gpu_use_name(query.sgx_object_gpu_use));
		printf("range_sgx_object_expected_context=%u\n",
		       query.sgx_object_expected_context);
		printf("range_sgx_object_expected_context_name=%s\n",
		       context_name(query.sgx_object_expected_context));
		printf("range_sgx_submit_ref_count=%u\n",
		       query.sgx_submit_ref_count);
		printf("range_sgx_first_submit_id=%u\n",
		       query.sgx_first_submit_id);
		printf("range_sgx_first_submit_slot=%u\n",
		       query.sgx_first_submit_slot);
		printf("range_sgx_first_submit_slot_name=%s\n",
		       submit_slot_name(query.sgx_first_submit_slot));
		printf("range_sgx_resolved_submit_found=%u\n",
		       query.sgx_resolved_submit_found);
		printf("range_sgx_resolved_submit_id=%u\n",
		       query.sgx_resolved_submit_id);
		printf("range_sgx_resolved_submit_slot=%u\n",
		       query.sgx_resolved_submit_slot);
		printf("range_sgx_resolved_submit_slot_name=%s\n",
		       submit_slot_name(query.sgx_resolved_submit_slot));
		printf("range_sgx_resolved_submit_match_index=%u\n",
		       query.sgx_resolved_submit_match_index);
		printf("range_pin_count_total=%u\n", query.pin_count_total);
		printf("range_unpin_count_total=%u\n", query.unpin_count_total);
		printf("range_pf_bind_count=%u\n", query.pf_bind_count);
		printf("range_pf_bind_total=%u\n", query.pf_bind_total);
		printf("range_pf_unbind_total=%u\n", query.pf_unbind_total);
	}

	close(fd);
	return ret == 0 ? 0 : 1;
}
