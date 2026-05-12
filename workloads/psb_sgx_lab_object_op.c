#include <drm/drm.h>
#include <xf86drm.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "psb_drm_lab.h"

static const char *object_op_name(uint32_t op)
{
	switch (op) {
	case DRM_PSB_SGX_LAB_OBJECT_OP_ADOPT:
		return "adopt";
	case DRM_PSB_SGX_LAB_OBJECT_OP_DROP:
		return "drop";
	case DRM_PSB_SGX_LAB_OBJECT_OP_PF_BIND:
		return "pf_bind";
	case DRM_PSB_SGX_LAB_OBJECT_OP_PF_UNBIND:
		return "pf_unbind";
	case DRM_PSB_SGX_LAB_OBJECT_OP_GPU_USE:
		return "gpu_use";
	case DRM_PSB_SGX_LAB_OBJECT_OP_CONTEXT:
		return "context";
	case DRM_PSB_SGX_LAB_OBJECT_OP_ROLE:
		return "role";
	case DRM_PSB_SGX_LAB_OBJECT_OP_CLASS:
		return "class";
	case DRM_PSB_SGX_LAB_OBJECT_OP_CLEANUP:
		return "cleanup";
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

static int parse_context(const char *arg, uint32_t *out)
{
	if (strcmp(arg, "auto") == 0) {
		*out = DRM_PSB_SGX_LAB_CONTEXT_AUTO;
		return 0;
	}
	if (strcmp(arg, "default") == 0) {
		*out = DRM_PSB_SGX_LAB_CONTEXT_DEFAULT;
		return 0;
	}
	if (strcmp(arg, "pf") == 0) {
		*out = DRM_PSB_SGX_LAB_CONTEXT_PF;
		return 0;
	}
	return parse_u32(arg, out);
}

static int parse_gpu_use(const char *arg, uint32_t *out)
{
	if (strcmp(arg, "generic") == 0) {
		*out = DRM_PSB_SGX_LAB_GPU_USE_GENERIC;
		return 0;
	}
	if (strcmp(arg, "control_ccb") == 0) {
		*out = DRM_PSB_SGX_LAB_GPU_USE_CONTROL_CCB;
		return 0;
	}
	if (strcmp(arg, "pds_code") == 0) {
		*out = DRM_PSB_SGX_LAB_GPU_USE_PDS_CODE;
		return 0;
	}
	if (strcmp(arg, "use_code") == 0) {
		*out = DRM_PSB_SGX_LAB_GPU_USE_USE_CODE;
		return 0;
	}
	if (strcmp(arg, "render_target") == 0) {
		*out = DRM_PSB_SGX_LAB_GPU_USE_RENDER_TARGET;
		return 0;
	}
	if (strcmp(arg, "status_block") == 0) {
		*out = DRM_PSB_SGX_LAB_GPU_USE_STATUS_BLOCK;
		return 0;
	}
	return parse_u32(arg, out);
}

static int parse_role(const char *arg, uint32_t *out)
{
	if (strcmp(arg, "generic") == 0) {
		*out = DRM_PSB_SGX_LAB_ROLE_GENERIC;
		return 0;
	}
	if (strcmp(arg, "pds") == 0) {
		*out = DRM_PSB_SGX_LAB_ROLE_PDS;
		return 0;
	}
	if (strcmp(arg, "use") == 0) {
		*out = DRM_PSB_SGX_LAB_ROLE_USE;
		return 0;
	}
	if (strcmp(arg, "control") == 0) {
		*out = DRM_PSB_SGX_LAB_ROLE_CONTROL;
		return 0;
	}
	if (strcmp(arg, "target") == 0) {
		*out = DRM_PSB_SGX_LAB_ROLE_TARGET;
		return 0;
	}
	if (strcmp(arg, "status") == 0) {
		*out = DRM_PSB_SGX_LAB_ROLE_STATUS;
		return 0;
	}
	return parse_u32(arg, out);
}

static int parse_class(const char *arg, uint32_t *out)
{
	if (strcmp(arg, "generic") == 0) {
		*out = DRM_PSB_SGX_LAB_CLASS_GENERIC;
		return 0;
	}
	if (strcmp(arg, "packet") == 0) {
		*out = DRM_PSB_SGX_LAB_CLASS_PACKET;
		return 0;
	}
	if (strcmp(arg, "code") == 0) {
		*out = DRM_PSB_SGX_LAB_CLASS_CODE;
		return 0;
	}
	if (strcmp(arg, "state") == 0) {
		*out = DRM_PSB_SGX_LAB_CLASS_STATE;
		return 0;
	}
	if (strcmp(arg, "surface") == 0) {
		*out = DRM_PSB_SGX_LAB_CLASS_SURFACE;
		return 0;
	}
	return parse_u32(arg, out);
}

int main(int argc, char **argv)
{
	const char *path = "/dev/dri/card0";
	const char *op_arg;
	struct drm_psb_sgx_lab_object_op op;
	int argi = 1;
	int fd;
	int ret;

	if (argc > 1 && argv[1][0] == '/') {
		path = argv[1];
		argi++;
	}
	if (argc - argi < 1) {
		fprintf(stderr,
			"usage: %s [/dev/dri/card0] <adopt|drop|pf_bind|pf_unbind|gpu_use|context|role|class|cleanup> ...\n",
			argv[0]);
		return 2;
	}

	memset(&op, 0, sizeof(op));
	op.size = sizeof(op);
	op_arg = argv[argi++];

	if (strcmp(op_arg, "adopt") == 0) {
		op.op = DRM_PSB_SGX_LAB_OBJECT_OP_ADOPT;
		if (argc - argi != 1 || parse_u32(argv[argi++], &op.range_id) != 0) {
			fprintf(stderr, "invalid range id\n");
			return 2;
		}
	} else if (strcmp(op_arg, "drop") == 0) {
		op.op = DRM_PSB_SGX_LAB_OBJECT_OP_DROP;
		if (argc - argi != 1 || parse_u32(argv[argi++], &op.object_id) != 0) {
			fprintf(stderr, "invalid object id\n");
			return 2;
		}
	} else if (strcmp(op_arg, "pf_bind") == 0) {
		op.op = DRM_PSB_SGX_LAB_OBJECT_OP_PF_BIND;
		if (argc - argi != 1 || parse_u32(argv[argi++], &op.object_id) != 0) {
			fprintf(stderr, "invalid object id\n");
			return 2;
		}
	} else if (strcmp(op_arg, "pf_unbind") == 0) {
		op.op = DRM_PSB_SGX_LAB_OBJECT_OP_PF_UNBIND;
		if (argc - argi != 1 || parse_u32(argv[argi++], &op.object_id) != 0) {
			fprintf(stderr, "invalid object id\n");
			return 2;
		}
	} else if (strcmp(op_arg, "gpu_use") == 0) {
		op.op = DRM_PSB_SGX_LAB_OBJECT_OP_GPU_USE;
		if (argc - argi != 2 || parse_u32(argv[argi++], &op.object_id) != 0 ||
		    parse_gpu_use(argv[argi++], &op.gpu_use) != 0) {
			fprintf(stderr, "invalid gpu_use arguments\n");
			return 2;
		}
	} else if (strcmp(op_arg, "context") == 0) {
		op.op = DRM_PSB_SGX_LAB_OBJECT_OP_CONTEXT;
		if (argc - argi != 2 || parse_u32(argv[argi++], &op.object_id) != 0 ||
		    parse_context(argv[argi++], &op.context) != 0) {
			fprintf(stderr, "invalid context arguments\n");
			return 2;
		}
	} else if (strcmp(op_arg, "role") == 0) {
		op.op = DRM_PSB_SGX_LAB_OBJECT_OP_ROLE;
		if (argc - argi != 2 || parse_u32(argv[argi++], &op.object_id) != 0 ||
		    parse_role(argv[argi++], &op.role) != 0) {
			fprintf(stderr, "invalid role arguments\n");
			return 2;
		}
	} else if (strcmp(op_arg, "class") == 0) {
		op.op = DRM_PSB_SGX_LAB_OBJECT_OP_CLASS;
		if (argc - argi != 2 || parse_u32(argv[argi++], &op.object_id) != 0 ||
		    parse_class(argv[argi++], &op.object_class) != 0) {
			fprintf(stderr, "invalid class arguments\n");
			return 2;
		}
	} else if (strcmp(op_arg, "cleanup") == 0) {
		op.op = DRM_PSB_SGX_LAB_OBJECT_OP_CLEANUP;
		if (argc != argi) {
			fprintf(stderr, "cleanup takes no arguments\n");
			return 2;
		}
	} else {
		fprintf(stderr, "invalid op: %s\n", op_arg);
		return 2;
	}

	fd = open(path, O_RDWR);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	errno = 0;
	/*
	 * Use raw ioctl() so custom lab-object error returns are preserved
	 * exactly instead of going through libdrm retry behavior.
	 */
	ret = ioctl(fd, DRM_IOCTL_PSB_SGX_LAB_OBJECT_OP, &op);

	printf("drm_path=%s\n", path);
	printf("drm_open_result=%d\n", fd);
	printf("object_op_name=%s\n", object_op_name(op.op));
	printf("object_op_ret=%d errno=%d\n", ret, errno);
	printf("object_input_id=%u\n", op.object_id);
	printf("object_resolved_id=%u\n", op.resolved_object_id);
	printf("object_result_count=%u\n", op.result_count);
	printf("object_range_id=%u\n", op.range_id);
	printf("object_role=%u\n", op.role);
	printf("object_role_name=%s\n", role_name(op.role));
	printf("object_class=%u\n", op.object_class);
	printf("object_class_name=%s\n", class_name(op.object_class));
	printf("object_gpu_use=%u\n", op.gpu_use);
	printf("object_gpu_use_name=%s\n", gpu_use_name(op.gpu_use));
	printf("object_context=%u\n", op.context);
	printf("object_context_name=%s\n", context_name(op.context));

	close(fd);
	return ret ? 1 : 0;
}
