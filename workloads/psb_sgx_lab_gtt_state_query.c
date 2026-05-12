#include <drm/drm.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "psb_drm_lab.h"

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

int main(int argc, char **argv)
{
	const char *path = argc > 1 ? argv[1] : "/dev/dri/card0";
	struct drm_psb_sgx_lab_gtt_state_query query;
	int fd;
	int ret;

	memset(&query, 0, sizeof(query));
	query.size = sizeof(query);

	fd = open(path, O_RDWR);
	printf("drm_path=%s\n", path);
	printf("drm_open_result=%d\n", fd);
	if (fd < 0) {
		printf("drm_open_errno=%d\n", errno);
		return 1;
	}

	errno = 0;
	ret = ioctl(fd, DRM_IOCTL_PSB_SGX_LAB_QUERY_GTT_STATE, &query);
	printf("gtt_state_ioctl_ret=%d errno=%d\n", ret, errno);
	if (ret == 0) {
		printf("gtt_state_gtt_start=0x%x\n", query.gtt_start);
		printf("gtt_state_gatt_start=0x%x\n", query.gatt_start);
		printf("gtt_state_mmu_gatt_start=0x%x\n",
		       query.mmu_gatt_start);
		printf("gtt_state_gatt_free_offset=0x%x\n",
		       query.gatt_free_offset);
		printf("gtt_state_stolen_size=0x%08" PRIx64 "\n",
		       (uint64_t)query.stolen_size);
		printf("gtt_state_vram_stolen_size=0x%08" PRIx64 "\n",
		       (uint64_t)query.vram_stolen_size);
		printf("gtt_state_mmu_last_op=%u\n", query.mmu_last_op);
		printf("gtt_state_mmu_last_op_name=%s\n",
		       mmu_op_name(query.mmu_last_op));
		printf("gtt_state_mmu_last_address=0x%08" PRIx64 "\n",
		       (uint64_t)query.mmu_last_address);
		printf("gtt_state_mmu_last_num_pages=%u\n",
		       query.mmu_last_num_pages);
		printf("gtt_state_mmu_last_type=%d\n", query.mmu_last_type);
		printf("gtt_state_mmu_last_hw_context=%d\n",
		       query.mmu_last_hw_context);
		printf("gtt_state_mmu_last_jiffies=%" PRIu64 "\n",
		       (uint64_t)query.mmu_last_jiffies);
		printf("gtt_state_mmu_flush_count=%u\n",
		       query.mmu_flush_count);
		printf("gtt_state_mmu_set_pd_context_count=%u\n",
		       query.mmu_set_pd_context_count);
		printf("gtt_state_mmu_insert_pfn_sequence_count=%u\n",
		       query.mmu_insert_pfn_sequence_count);
		printf("gtt_state_mmu_insert_pages_count=%u\n",
		       query.mmu_insert_pages_count);
		printf("gtt_state_mmu_remove_pfn_sequence_count=%u\n",
		       query.mmu_remove_pfn_sequence_count);
		printf("gtt_state_mmu_remove_pages_count=%u\n",
		       query.mmu_remove_pages_count);
		printf("gtt_state_mmu_default_pd_context=%d\n",
		       query.mmu_default_pd_context);
		printf("gtt_state_mmu_pf_pd_context=%d\n",
		       query.mmu_pf_pd_context);
		printf("gtt_state_range_total_count=%u\n",
		       query.range_total_count);
		printf("gtt_state_range_gem_count=%u\n",
		       query.range_gem_count);
		printf("gtt_state_range_fb_count=%u\n",
		       query.range_fb_count);
		printf("gtt_state_range_cursor_count=%u\n",
		       query.range_cursor_count);
		printf("gtt_state_range_other_count=%u\n",
		       query.range_other_count);
		printf("gtt_state_range_in_gart_count=%u\n",
		       query.range_in_gart_count);
		printf("gtt_state_range_mmapping_count=%u\n",
		       query.range_mmapping_count);
		printf("gtt_state_range_sgx_object_count=%u\n",
		       query.range_sgx_object_count);
		printf("gtt_state_range_pf_bound_count=%u\n",
		       query.range_pf_bound_count);
	}

	close(fd);
	return ret == 0 ? 0 : 1;
}
