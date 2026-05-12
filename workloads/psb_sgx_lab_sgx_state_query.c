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

static void usage(const char *argv0)
{
	fprintf(stderr, "usage: %s [/dev/dri/card0] [capture|cached]\n", argv0);
}

int main(int argc, char **argv)
{
	const char *path = "/dev/dri/card0";
	const char *mode = "cached";
	struct drm_psb_sgx_lab_sgx_state_query query;
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

	memset(&query, 0, sizeof(query));
	query.size = sizeof(query);

	if (strcmp(mode, "capture") == 0) {
		query.flags |= DRM_PSB_SGX_LAB_QUERY_SGX_STATE_CAPTURE;
	} else if (strcmp(mode, "cached") != 0) {
		fprintf(stderr, "invalid mode: %s\n", mode);
		usage(argv[0]);
		return 2;
	}

	fd = open(path, O_RDWR);
	printf("drm_path=%s\n", path);
	printf("sgx_state_query_mode=%s\n", mode);
	printf("sgx_state_query_capture=%u\n",
	       (query.flags & DRM_PSB_SGX_LAB_QUERY_SGX_STATE_CAPTURE) ? 1 : 0);
	printf("drm_open_result=%d\n", fd);
	if (fd < 0) {
		printf("drm_open_errno=%d\n", errno);
		return 1;
	}

	errno = 0;
	ret = ioctl(fd, DRM_IOCTL_PSB_SGX_LAB_QUERY_SGX_STATE, &query);
	printf("sgx_state_ioctl_ret=%d errno=%d\n", ret, errno);
	if (ret == 0) {
		printf("sgx_state_reason=%s\n", query.reason[0] ?
		       (const char *)query.reason : "none");
		printf("sgx_state_sequence=%u\n", query.sequence);
		printf("sgx_state_captured_jiffies=%" PRIu64 "\n",
		       (uint64_t)query.captured_jiffies);
		printf("sgx_state_capture_failed=%u\n", query.capture_failed);
		printf("sgx_state_irq_count=%u\n", query.irq_count);
		printf("sgx_state_fault_count=%u\n", query.fault_count);
		printf("sgx_state_vdc_irq_mask=0x%x\n", query.vdc_irq_mask);
		printf("sgx_state_gatt_start=0x%x\n", query.gatt_start);
		printf("sgx_state_mmu_gatt_start=0x%x\n", query.mmu_gatt_start);
		printf("sgx_state_gatt_free_offset=0x%x\n",
		       query.gatt_free_offset);
		printf("sgx_state_mmu_driver_bif_ctrl=0x%x\n",
		       query.mmu_driver_bif_ctrl);
		printf("sgx_state_mmu_needs_tlbflush=%u\n",
		       query.mmu_needs_tlbflush);
		printf("sgx_state_mmu_default_pd_context=%u\n",
		       query.mmu_default_pd_context);
		printf("sgx_state_core_id=0x%x\n", query.core_id);
		printf("sgx_state_core_revision=0x%x\n", query.core_revision);
		printf("sgx_state_clkgatectl=0x%x\n", query.clkgatectl);
		printf("sgx_state_soft_reset=0x%x\n", query.sgx_soft_reset);
		printf("sgx_state_event_status=0x%x\n", query.event_status);
		printf("sgx_state_event_status2=0x%x\n", query.event_status2);
		printf("sgx_state_event_host_enable=0x%x\n",
		       query.event_host_enable);
		printf("sgx_state_event_host_enable2=0x%x\n",
		       query.event_host_enable2);
		printf("sgx_state_event_kicker=0x%x\n", query.event_kicker);
		printf("sgx_state_event_kick=0x%x\n", query.event_kick);
		printf("sgx_state_pds_exec_base=0x%x\n", query.pds_exec_base);
		printf("sgx_state_use_code_base0=0x%x\n",
		       query.use_code_base0);
		printf("sgx_state_use_code_base1=0x%x\n",
		       query.use_code_base1);
		printf("sgx_state_bif_ctrl=0x%x\n", query.bif_ctrl);
		printf("sgx_state_bif_int_stat=0x%x\n", query.bif_int_stat);
		printf("sgx_state_bif_fault=0x%x\n", query.bif_fault);
		printf("sgx_state_bif_dir_list_base0=0x%x\n",
		       query.bif_dir_list_base0);
		printf("sgx_state_bif_dir_list_base1=0x%x\n",
		       query.bif_dir_list_base1);
		printf("sgx_state_bif_bank0=0x%x\n", query.bif_bank0);
		printf("sgx_state_bif_bank1=0x%x\n", query.bif_bank1);
		printf("sgx_state_bif_twod_req_base=0x%x\n",
		       query.bif_twod_req_base);
		printf("sgx_state_bif_3d_req_base=0x%x\n",
		       query.bif_3d_req_base);
		printf("sgx_state_twod_socif=0x%x\n", query.twod_socif);
		printf("sgx_state_twod_blit_status=0x%x\n",
		       query.twod_blit_status);
	}

	close(fd);
	return ret == 0 ? 0 : 1;
}
