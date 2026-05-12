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

static void usage(const char *argv0)
{
	fprintf(stderr,
		"usage: %s [/dev/dri/card0] <index|id> <value>\n",
		argv0);
}

int main(int argc, char **argv)
{
	const char *path = "/dev/dri/card0";
	const char *mode;
	const char *value_arg;
	struct drm_psb_sgx_lab_object_query query;
	int fd;
	int ret;
	int argi = 1;

	if (argc > 1 && strncmp(argv[1], "/dev/", 5) == 0) {
		path = argv[1];
		argi++;
	}

	if (argc - argi != 2) {
		usage(argv[0]);
		return 2;
	}

	mode = argv[argi++];
	value_arg = argv[argi++];

	memset(&query, 0, sizeof(query));
	query.size = sizeof(query);

	if (strcmp(mode, "index") == 0) {
		if (parse_u32(value_arg, &query.query_index) != 0) {
			fprintf(stderr, "invalid query index: %s\n", value_arg);
			return 2;
		}
	} else if (strcmp(mode, "id") == 0) {
		if (parse_u32(value_arg, &query.object_id) != 0) {
			fprintf(stderr, "invalid object id: %s\n", value_arg);
			return 2;
		}
		query.flags |= DRM_PSB_SGX_LAB_QUERY_OBJECT_USE_ID;
	} else {
		fprintf(stderr, "invalid mode: %s\n", mode);
		usage(argv[0]);
		return 2;
	}

	fd = open(path, O_RDWR);
	printf("drm_path=%s\n", path);
	printf("object_query_mode=%s\n", mode);
	printf("object_query_value=%s\n", value_arg);
	printf("drm_open_result=%d\n", fd);
	if (fd < 0) {
		printf("drm_open_errno=%d\n", errno);
		return 1;
	}

	errno = 0;
	ret = ioctl(fd, DRM_IOCTL_PSB_SGX_LAB_QUERY_OBJECT, &query);
	printf("object_ioctl_ret=%d errno=%d\n", ret, errno);
	if (ret == 0) {
		printf("object_resolved_id=%u\n", query.resolved_object_id);
		printf("object_resolved_index=%u\n", query.resolved_index);
		printf("object_total_count=%u\n", query.total_object_count);
		printf("object_range_id=%u\n", query.range_id);
		printf("object_range_type=%u\n", query.range_type);
		printf("object_range_type_name=%s\n",
		       range_type_name(query.range_type));
		printf("object_role=%u\n", query.role);
		printf("object_role_name=%s\n", role_name(query.role));
		printf("object_class=%u\n", query.object_class);
		printf("object_class_name=%s\n", class_name(query.object_class));
		printf("object_gpu_use=%u\n", query.gpu_use);
		printf("object_gpu_use_name=%s\n", gpu_use_name(query.gpu_use));
		printf("object_expected_context=%u\n", query.expected_context);
		printf("object_expected_context_name=%s\n",
		       context_name(query.expected_context));
		printf("object_size_bytes=0x%08" PRIx64 "\n",
		       (uint64_t)query.size_bytes);
		printf("object_gpu_va=0x%08" PRIx64 "\n",
		       (uint64_t)query.gpu_va);
		printf("object_offset=0x%x\n", query.offset);
		printf("object_stolen=%u\n", query.stolen);
		printf("object_in_gart=%d\n", query.in_gart);
		printf("object_mmapping=%u\n", query.mmapping);
		printf("object_npage=%d\n", query.npage);
		printf("object_pf_bind_count=%u\n", query.pf_bind_count);
		printf("object_pf_bind_total=%u\n", query.pf_bind_total);
		printf("object_pf_unbind_total=%u\n", query.pf_unbind_total);
		printf("object_created_jiffies=%" PRIu64 "\n",
		       (uint64_t)query.created_jiffies);
		printf("object_last_pf_bind_jiffies=%" PRIu64 "\n",
		       (uint64_t)query.last_pf_bind_jiffies);
		printf("object_last_pf_unbind_jiffies=%" PRIu64 "\n",
		       (uint64_t)query.last_pf_unbind_jiffies);
		printf("object_last_role_jiffies=%" PRIu64 "\n",
		       (uint64_t)query.last_role_jiffies);
		printf("object_last_class_jiffies=%" PRIu64 "\n",
		       (uint64_t)query.last_class_jiffies);
		printf("object_last_gpu_use_jiffies=%" PRIu64 "\n",
		       (uint64_t)query.last_gpu_use_jiffies);
		printf("object_last_context_jiffies=%" PRIu64 "\n",
		       (uint64_t)query.last_context_jiffies);
	}

	close(fd);
	return ret == 0 ? 0 : 1;
}
