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

static const char *submit_op_name(uint32_t op)
{
	switch (op) {
	case DRM_PSB_SGX_LAB_SUBMIT_OP_VALIDATE:
		return "validate";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_PREPARE:
		return "prepare";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_ARM:
		return "arm";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_EMIT:
		return "emit";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_LAUNCH:
		return "launch";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE:
		return "issue";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION:
		return "transition";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_CREATE:
		return "create";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DROP:
		return "drop";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_FOCUS:
		return "focus";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_FOCUS_CLEAR:
		return "focus-clear";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_ATTACH:
		return "attach";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DETACH:
		return "detach";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_KICK:
		return "kick";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_CCB:
		return "ccb";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_QUEUE:
		return "queue";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_STATUS:
		return "status";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_SYNC:
		return "sync";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_OWNER:
		return "owner";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_REPORT:
		return "report";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_LIVE:
		return "live";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_RESULT:
		return "result";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_MANIFEST:
		return "manifest";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DESCRIPTOR:
		return "descriptor";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_PLAN:
		return "plan";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE_PLAN:
		return "issue-plan";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DEPEND_SLOT:
		return "depend-slot";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DEPTICKET_SLOT:
		return "depticket-slot";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DEPSTATE_SLOT:
		return "depstate-slot";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DEPCLEAR_SLOT:
		return "depclear-slot";
	case DRM_PSB_SGX_LAB_SUBMIT_OP_CLEANUP:
		return "cleanup";
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

static int parse_submit_selector(const char *arg,
				 struct drm_psb_sgx_lab_submit_op *op)
{
	if (strcmp(arg, "focus") == 0) {
		op->flags |= DRM_PSB_SGX_LAB_SUBMIT_OP_USE_FOCUS;
		return 0;
	}

	return parse_u32(arg, &op->submit_id);
}

static int parse_owner_state(const char *arg, uint32_t *out)
{
	if (strcmp(arg, "queued") == 0) {
		*out = DRM_PSB_SGX_LAB_OWNER_QUEUED;
		return 0;
	}
	if (strcmp(arg, "running") == 0) {
		*out = DRM_PSB_SGX_LAB_OWNER_RUNNING;
		return 0;
	}
	if (strcmp(arg, "complete") == 0) {
		*out = DRM_PSB_SGX_LAB_OWNER_COMPLETE;
		return 0;
	}
	if (strcmp(arg, "none") == 0) {
		*out = DRM_PSB_SGX_LAB_OWNER_NONE;
		return 0;
	}

	return parse_u32(arg, out);
}

static int parse_context(const char *arg, uint32_t *out)
{
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

static int parse_slot(const char *arg, uint32_t *out)
{
	if (strcmp(arg, "control") == 0) {
		*out = DRM_PSB_SGX_LAB_SUBMIT_SLOT_CONTROL;
		return 0;
	}
	if (strcmp(arg, "pds") == 0) {
		*out = DRM_PSB_SGX_LAB_SUBMIT_SLOT_PDS;
		return 0;
	}
	if (strcmp(arg, "use") == 0) {
		*out = DRM_PSB_SGX_LAB_SUBMIT_SLOT_USE;
		return 0;
	}
	if (strcmp(arg, "target") == 0) {
		*out = DRM_PSB_SGX_LAB_SUBMIT_SLOT_TARGET;
		return 0;
	}
	if (strcmp(arg, "status") == 0) {
		*out = DRM_PSB_SGX_LAB_SUBMIT_SLOT_STATUS;
		return 0;
	}
	return parse_u32(arg, out);
}

static int parse_cmd_type(const char *arg, uint32_t *out)
{
	if (strcmp(arg, "ta") == 0) {
		*out = DRM_PSB_SGX_LAB_CMD_TA;
		return 0;
	}
	if (strcmp(arg, "transfer") == 0) {
		*out = DRM_PSB_SGX_LAB_CMD_TRANSFER;
		return 0;
	}
	if (strcmp(arg, "2d") == 0) {
		*out = DRM_PSB_SGX_LAB_CMD_2D;
		return 0;
	}
	if (strcmp(arg, "power") == 0) {
		*out = DRM_PSB_SGX_LAB_CMD_POWER;
		return 0;
	}
	if (strcmp(arg, "cleanup") == 0) {
		*out = DRM_PSB_SGX_LAB_CMD_CLEANUP;
		return 0;
	}
	if (strcmp(arg, "getmiscinfo") == 0) {
		*out = DRM_PSB_SGX_LAB_CMD_GETMISCINFO;
		return 0;
	}
	if (strcmp(arg, "process_queues") == 0) {
		*out = DRM_PSB_SGX_LAB_CMD_PROCESS_QUEUES;
		return 0;
	}
	return parse_u32(arg, out);
}

static int submit_op_uses_selector(uint32_t op)
{
	switch (op) {
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DROP:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_VALIDATE:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_PREPARE:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_ARM:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_EMIT:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_LAUNCH:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_ATTACH:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DETACH:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_KICK:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_CCB:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_QUEUE:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_STATUS:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_SYNC:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_OWNER:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_REPORT:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_LIVE:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_RESULT:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_MANIFEST:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DESCRIPTOR:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_PLAN:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE_PLAN:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DEPEND_SLOT:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DEPTICKET_SLOT:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DEPSTATE_SLOT:
	case DRM_PSB_SGX_LAB_SUBMIT_OP_DEPCLEAR_SLOT:
		return 1;
	default:
		return 0;
	}
}

int main(int argc, char **argv)
{
	const char *path = "/dev/dri/card0";
	const char *op_arg;
	const char *selector_arg;
	struct drm_psb_sgx_lab_submit_op op;
	int argi = 1;
	int fd;
	int ret;

	if (argc > 1 && argv[1][0] == '/') {
		path = argv[1];
		argi++;
	}
	if (argc - argi < 1) {
		fprintf(stderr,
			"usage: %s [/dev/dri/card0] <create|drop|focus|focus-clear|cleanup|validate|prepare|arm|emit|launch|issue|transition|attach|detach|kick|ccb|queue|status|sync|owner|report|result|manifest|descriptor|plan|issue-plan|depend-slot|depticket-slot|depstate-slot|depclear-slot> ...\n",
			argv[0]);
		return 2;
	}

	op_arg = argv[argi++];

	memset(&op, 0, sizeof(op));
	op.size = sizeof(op);

	if (strcmp(op_arg, "create") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_CREATE;
		if (argc - argi != 1)
			return 2;
		if (parse_context(argv[argi++], &op.context) != 0) {
			fprintf(stderr, "invalid context: %s\n", argv[argi - 1]);
			return 2;
		}
		selector_arg = "(none)";
	} else if (strcmp(op_arg, "focus-clear") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_FOCUS_CLEAR;
		if (argc != argi)
			return 2;
		selector_arg = "(none)";
	} else if (strcmp(op_arg, "cleanup") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_CLEANUP;
		if (argc != argi)
			return 2;
		selector_arg = "(none)";
	} else {
		if (argc - argi < 1)
			return 2;
		selector_arg = argv[argi++];
	}

	if (strcmp(op_arg, "focus") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_FOCUS;
		if (argc != argi)
			return 2;
		if (parse_submit_selector(selector_arg, &op) != 0 ||
		    (op.flags & DRM_PSB_SGX_LAB_SUBMIT_OP_USE_FOCUS)) {
			fprintf(stderr, "invalid focus selector: %s\n",
				selector_arg);
			return 2;
		}
	} else if (strcmp(op_arg, "drop") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_DROP;
		if (argc != argi)
			return 2;
		if (parse_submit_selector(selector_arg, &op) != 0) {
			fprintf(stderr, "invalid submit selector: %s\n",
				selector_arg);
			return 2;
		}
	} else if (strcmp(op_arg, "prepare") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_PREPARE;
		if (argc != argi)
			return 2;
	} else if (strcmp(op_arg, "arm") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_ARM;
		if (argc != argi)
			return 2;
	} else if (strcmp(op_arg, "emit") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_EMIT;
		if (argc != argi)
			return 2;
	} else if (strcmp(op_arg, "launch") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_LAUNCH;
		if (argc - argi != 2)
			return 2;
		if (parse_owner_state(argv[argi++], &op.owner_state) != 0) {
			fprintf(stderr, "invalid owner state: %s\n",
				argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.packet_count) != 0) {
			fprintf(stderr, "invalid issued packet count: %s\n",
				argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "attach") == 0 ||
		   strcmp(op_arg, "detach") == 0) {
		op.op = strcmp(op_arg, "attach") == 0 ?
			DRM_PSB_SGX_LAB_SUBMIT_OP_ATTACH :
			DRM_PSB_SGX_LAB_SUBMIT_OP_DETACH;
		if (strcmp(op_arg, "attach") == 0) {
			if (argc - argi != 2)
				return 2;
		} else {
			if (argc - argi != 1)
				return 2;
		}
		if (parse_slot(argv[argi++], &op.slot) != 0) {
			fprintf(stderr, "invalid slot: %s\n", argv[argi - 1]);
			return 2;
		}
		if (op.op == DRM_PSB_SGX_LAB_SUBMIT_OP_ATTACH &&
		    parse_u32(argv[argi++], &op.object_id) != 0) {
			fprintf(stderr, "invalid object id: %s\n",
				argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "kick") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_KICK;
		if (argc - argi != 6)
			return 2;
		if (parse_cmd_type(argv[argi++], &op.cmd_type) != 0) {
			fprintf(stderr, "invalid cmd type: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.ccb_offset) != 0) {
			fprintf(stderr, "invalid ccb offset: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.ctrl_flags) != 0) {
			fprintf(stderr, "invalid ctrl flags: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.num_ta_status_vals) != 0) {
			fprintf(stderr, "invalid TA status count: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.num_3d_status_vals) != 0) {
			fprintf(stderr, "invalid 3D status count: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.ta_dependency) != 0) {
			fprintf(stderr, "invalid TA dependency: %s\n", argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "ccb") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_CCB;
		if (argc - argi != 6)
			return 2;
		if (parse_u32(argv[argi++], &op.ccb_write_offset) != 0) {
			fprintf(stderr, "invalid CCB write offset: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.ccb_read_offset) != 0) {
			fprintf(stderr, "invalid CCB read offset: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.ccb_service_address) != 0) {
			fprintf(stderr, "invalid CCB service address: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.ccb_cache_control) != 0) {
			fprintf(stderr, "invalid CCB cache control: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.ccb_data0) != 0) {
			fprintf(stderr, "invalid CCB data0: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.ccb_data1) != 0) {
			fprintf(stderr, "invalid CCB data1: %s\n", argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "queue") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_QUEUE;
		if (argc - argi != 4)
			return 2;
		if (parse_u32(argv[argi++], &op.queue_base_offset) != 0) {
			fprintf(stderr, "invalid queue base offset: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.queue_size) != 0) {
			fprintf(stderr, "invalid queue size: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.queue_packet_size) != 0) {
			fprintf(stderr, "invalid queue packet size: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.queue_packet_count) != 0) {
			fprintf(stderr, "invalid queue packet count: %s\n", argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "status") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_STATUS;
		if (argc - argi != 4)
			return 2;
		if (parse_u32(argv[argi++], &op.status_ta_offset) != 0) {
			fprintf(stderr, "invalid TA status offset: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.status_3d_offset) != 0) {
			fprintf(stderr, "invalid 3D status offset: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.status_stride) != 0) {
			fprintf(stderr, "invalid status stride: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.status_value_seed) != 0) {
			fprintf(stderr, "invalid status seed: %s\n", argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "sync") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_SYNC;
		if (argc - argi != 4)
			return 2;
		if (parse_u32(argv[argi++], &op.sync_offset) != 0) {
			fprintf(stderr, "invalid sync offset: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.sync_complete_seq) != 0) {
			fprintf(stderr, "invalid sync complete seq: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.sync_pending_seq) != 0) {
			fprintf(stderr, "invalid sync pending seq: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.sync_flags) != 0) {
			fprintf(stderr, "invalid sync flags: %s\n", argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "owner") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_OWNER;
		if (argc - argi != 4)
			return 2;
		if (parse_u32(argv[argi++], &op.owner_offset) != 0) {
			fprintf(stderr, "invalid owner offset: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_owner_state(argv[argi++], &op.owner_state) != 0) {
			fprintf(stderr, "invalid owner state: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.owner_issue_cookie) != 0) {
			fprintf(stderr, "invalid owner issue cookie: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.owner_complete_cookie) != 0) {
			fprintf(stderr, "invalid owner complete cookie: %s\n", argv[argi - 1]);
			return 2;
		}
		} else if (strcmp(op_arg, "report") == 0) {
			op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_REPORT;
			if (argc - argi != 1)
				return 2;
			if (parse_u32(argv[argi++], &op.report_offset) != 0) {
				fprintf(stderr, "invalid report offset: %s\n", argv[argi - 1]);
				return 2;
			}
		} else if (strcmp(op_arg, "live") == 0) {
			op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_LIVE;
			if (argc - argi != 1)
				return 2;
			if (parse_u32(argv[argi++], &op.live_offset) != 0) {
				fprintf(stderr, "invalid live offset: %s\n", argv[argi - 1]);
				return 2;
			}
		} else if (strcmp(op_arg, "result") == 0) {
			op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_RESULT;
			if (argc - argi != 1)
				return 2;
			if (parse_u32(argv[argi++], &op.result_offset) != 0) {
				fprintf(stderr, "invalid result offset: %s\n", argv[argi - 1]);
				return 2;
			}
		} else if (strcmp(op_arg, "manifest") == 0) {
			op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_MANIFEST;
		if (argc - argi != 1)
			return 2;
		if (parse_u32(argv[argi++], &op.manifest_offset) != 0) {
			fprintf(stderr, "invalid manifest offset: %s\n",
				argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "descriptor") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_DESCRIPTOR;
		if (argc - argi != 1)
			return 2;
		if (parse_u32(argv[argi++], &op.descriptor_offset) != 0) {
			fprintf(stderr, "invalid descriptor offset: %s\n",
				argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "plan") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_PLAN;
		if (argc - argi != 1)
			return 2;
		if (parse_u32(argv[argi++], &op.plan_offset) != 0) {
			fprintf(stderr, "invalid plan offset: %s\n",
				argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "issue-plan") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE_PLAN;
		if (argc - argi != 3)
			return 2;
		if (parse_u32(argv[argi++], &op.issue_plan_offset) != 0) {
			fprintf(stderr, "invalid issue-plan offset: %s\n",
				argv[argi - 1]);
			return 2;
		}
		if (parse_owner_state(argv[argi++], &op.owner_state) != 0) {
			fprintf(stderr, "invalid owner state: %s\n",
				argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.packet_count) != 0) {
			fprintf(stderr, "invalid packet count: %s\n",
				argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "depend-slot") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_DEPEND_SLOT;
		if (argc - argi != 2)
			return 2;
		if (parse_u32(argv[argi++], &op.dependency_slot) != 0) {
			fprintf(stderr, "invalid dependency slot: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.dependency_submit_id) != 0) {
			fprintf(stderr, "invalid dependency submit id: %s\n", argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "depticket-slot") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_DEPTICKET_SLOT;
		if (argc - argi != 2)
			return 2;
		if (parse_u32(argv[argi++], &op.dependency_slot) != 0) {
			fprintf(stderr, "invalid dependency slot: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.dependency_ticket) != 0) {
			fprintf(stderr, "invalid dependency ticket: %s\n", argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "depstate-slot") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_DEPSTATE_SLOT;
		if (argc - argi != 2)
			return 2;
		if (parse_u32(argv[argi++], &op.dependency_slot) != 0) {
			fprintf(stderr, "invalid dependency slot: %s\n", argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.dependency_offset) != 0) {
			fprintf(stderr, "invalid dependency offset: %s\n", argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "depclear-slot") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_DEPCLEAR_SLOT;
		if (argc - argi != 1)
			return 2;
		if (parse_u32(argv[argi++], &op.dependency_slot) != 0) {
			fprintf(stderr, "invalid dependency slot: %s\n", argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "validate") == 0) {
		op.op = DRM_PSB_SGX_LAB_SUBMIT_OP_VALIDATE;
		if (argc != argi)
			return 2;
	} else if (strcmp(op_arg, "issue") == 0 ||
		   strcmp(op_arg, "transition") == 0) {
		op.op = strcmp(op_arg, "issue") == 0 ?
			DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE :
			DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION;
		if (argc - argi != 2)
			return 2;
		if (parse_owner_state(argv[argi++], &op.owner_state) != 0) {
			fprintf(stderr, "invalid owner state: %s\n",
				argv[argi - 1]);
			return 2;
		}
		if (parse_u32(argv[argi++], &op.packet_count) != 0) {
			fprintf(stderr, "invalid packet count: %s\n",
				argv[argi - 1]);
			return 2;
		}
	} else if (strcmp(op_arg, "create") == 0 ||
		   strcmp(op_arg, "focus-clear") == 0 ||
		   strcmp(op_arg, "cleanup") == 0) {
		/* already parsed above */
	} else {
		fprintf(stderr, "invalid op: %s\n", op_arg);
		return 2;
	}

	if (submit_op_uses_selector(op.op) &&
	    parse_submit_selector(selector_arg, &op) != 0) {
		fprintf(stderr, "invalid submit selector: %s\n", selector_arg);
		return 2;
	}

	fd = open(path, O_RDWR);
	printf("drm_path=%s\n", path);
	printf("drm_open_result=%d\n", fd);
	if (fd < 0) {
		printf("drm_open_errno=%d\n", errno);
		return 1;
	}

	errno = 0;
	/*
	 * Use raw ioctl() instead of libdrm's drmIoctl(). The libdrm helper
	 * retries on EAGAIN, but this lab ioctl intentionally uses EAGAIN to
	 * report dependency-gated submit state to userspace.
	 */
	ret = ioctl(fd, DRM_IOCTL_PSB_SGX_LAB_SUBMIT_OP, &op);
	printf("submit_op_name=%s\n", submit_op_name(op.op));
	printf("submit_selector=%s\n", selector_arg);
	printf("submit_flags=0x%x\n", op.flags);
	printf("submit_op_ret=%d errno=%d\n", ret, errno);
	printf("submit_resolved_id=%u\n", op.resolved_submit_id);
	printf("submit_result_count=%u\n", op.result_count);
	printf("submit_context=%u\n", op.context);
	printf("submit_context_name=%s\n", context_name(op.context));
	printf("submit_slot=%u\n", op.slot);
	printf("submit_slot_name=%s\n", slot_name(op.slot));
	printf("submit_object_id=%u\n", op.object_id);
	printf("submit_cmd_type=%u\n", op.cmd_type);
	printf("submit_cmd_type_name=%s\n", cmd_type_name(op.cmd_type));
	printf("submit_ccb_offset=0x%x\n", op.ccb_offset);
	printf("submit_ctrl_flags=0x%x\n", op.ctrl_flags);
	printf("submit_num_ta_status_vals=%u\n", op.num_ta_status_vals);
	printf("submit_num_3d_status_vals=%u\n", op.num_3d_status_vals);
	printf("submit_ta_dependency=%u\n", op.ta_dependency);
	printf("submit_ccb_write_offset=0x%x\n", op.ccb_write_offset);
	printf("submit_ccb_read_offset=0x%x\n", op.ccb_read_offset);
	printf("submit_ccb_service_address=0x%x\n", op.ccb_service_address);
	printf("submit_ccb_cache_control=0x%x\n", op.ccb_cache_control);
	printf("submit_ccb_data0=0x%x\n", op.ccb_data0);
	printf("submit_ccb_data1=0x%x\n", op.ccb_data1);
	printf("submit_queue_base_offset=0x%x\n", op.queue_base_offset);
	printf("submit_queue_size=0x%x\n", op.queue_size);
	printf("submit_queue_packet_size=0x%x\n", op.queue_packet_size);
	printf("submit_queue_packet_count=%u\n", op.queue_packet_count);
	printf("submit_status_ta_offset=0x%x\n", op.status_ta_offset);
	printf("submit_status_3d_offset=0x%x\n", op.status_3d_offset);
	printf("submit_status_stride=0x%x\n", op.status_stride);
	printf("submit_status_value_seed=0x%x\n", op.status_value_seed);
	printf("submit_sync_offset=0x%x\n", op.sync_offset);
	printf("submit_sync_complete_seq=0x%x\n", op.sync_complete_seq);
	printf("submit_sync_pending_seq=0x%x\n", op.sync_pending_seq);
	printf("submit_sync_flags=0x%x\n", op.sync_flags);
	printf("submit_owner_offset=0x%x\n", op.owner_offset);
	printf("submit_owner_issue_cookie=0x%x\n", op.owner_issue_cookie);
	printf("submit_owner_complete_cookie=0x%x\n", op.owner_complete_cookie);
	printf("submit_dependency_slot=%u\n", op.dependency_slot);
	printf("submit_dependency_submit_id=%u\n", op.dependency_submit_id);
	printf("submit_dependency_ticket=%u\n", op.dependency_ticket);
	printf("submit_dependency_offset=0x%x\n", op.dependency_offset);
	printf("submit_report_offset=0x%x\n", op.report_offset);
	printf("submit_live_offset=0x%x\n", op.live_offset);
	printf("submit_manifest_offset=0x%x\n", op.manifest_offset);
	printf("submit_descriptor_offset=0x%x\n", op.descriptor_offset);
	printf("submit_plan_offset=0x%x\n", op.plan_offset);
	printf("submit_issue_plan_offset=0x%x\n", op.issue_plan_offset);
	printf("submit_owner_state=%u\n", op.owner_state);
	printf("submit_owner_state_name=%s\n",
	       owner_state_name(op.owner_state));
	printf("submit_packet_count=%u\n", op.packet_count);

	close(fd);
	return ret ? 1 : 0;
}
