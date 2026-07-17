/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * psb_blt_test.c - userspace test client for the gma500 (Poulsbo/SGX535)
 * DRM_PSB_BLT_SUBMIT ioctl.
 *
 * Style matches /Projects/Poulsbo/workloads/psb_sgx_lab_*.c: plain
 * open(2) + ioctl(2), no libdrm runtime dependency (headers only).
 *
 * What it does:
 *   1. Opens /dev/dri/card0 (path overridable via argv[1]).
 *   2. Creates its OWN dumb buffer (256x64, 32bpp), maps it, fills it
 *      with a 0x11111111 sentinel.  The fbcon scanout is never touched.
 *   3. Submits an 8-dword solid-fill stream (colour 0x00C0FFEE into
 *      rect (32,16)-(95,47)) with one reloc pointing at the dumb BO.
 *   4. Verifies by CPU readback: inside rect == fill colour, outside
 *      rect == sentinel (proves both the fill and no-overdraw/stride).
 *   5. Runs negative tests, each expecting a specific errno, and
 *      re-verifies the buffer was NOT modified by rejected submits.
 *
 * Exit status: 0 if every check passed, 1 otherwise.
 */

#define _GNU_SOURCE	/* O_CLOEXEC, usleep with -std=c11 */

#include <drm/drm.h>
#include <drm/drm_mode.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "gma_blt_drm.h"

/*
 * PSB_2D_* constants, values verified against
 * build/devnull-vaio-gma500-src/psb_reg.h (self-contained here so the
 * test builds without -I into the kernel tree).
 */
#define PSB_2D_FENCE_BH		0x70000000u
#define PSB_2D_BLIT_BH		0x80000000u
#define PSB_2D_DST_SURF_BH	0xA0000000u
#define PSB_2D_PAT_SURF_BH	0xB0000000u	/* deliberately NOT allowed by v1 */
#define PSB_2D_FLUSH_BH		0xF0000000u

#define PSB_2D_DST_8888ARGB	0x00060000u
#define PSB_2D_USE_FILL		0x00000000u
#define PSB_2D_ROP3_PATCOPY	0x0000F0F0u

/* test geometry */
#define BUF_W		256u
#define BUF_H		64u
#define BUF_BPP		32u

#define FILL_X		32u
#define FILL_Y		16u
#define FILL_W		64u
#define FILL_H		32u

#define SENTINEL	0x11111111u
#define FILL_COLOUR	0x00C0FFEEu

static unsigned int g_pass;
static unsigned int g_fail;

static void report(const char *name, int ok, const char *detail)
{
	if (ok) {
		g_pass++;
		printf("PASS: %s\n", name);
	} else {
		g_fail++;
		printf("FAIL: %s (%s)\n", name, detail);
	}
}

/* Build the proven 8-dword solid-fill stream.  cs[2] is the address slot. */
static void build_fill_stream(uint32_t *cs, uint32_t pitch_bytes)
{
	cs[0] = PSB_2D_FENCE_BH;
	cs[1] = PSB_2D_DST_SURF_BH | PSB_2D_DST_8888ARGB | pitch_bytes;
	cs[2] = 0;					/* address slot */
	cs[3] = PSB_2D_BLIT_BH | PSB_2D_USE_FILL | PSB_2D_ROP3_PATCOPY;
	cs[4] = FILL_COLOUR;
	cs[5] = (FILL_X << 12) | FILL_Y;		/* xy */
	cs[6] = (FILL_W << 12) | FILL_H;		/* wh */
	cs[7] = PSB_2D_FLUSH_BH;
}

static int submit(int fd, const uint32_t *cs, uint32_t cmd_dwords,
		  const struct drm_psb_blt_reloc *relocs, uint32_t nr_relocs,
		  uint32_t flags)
{
	struct drm_psb_blt_submit args;
	int ret;

	memset(&args, 0, sizeof(args));
	args.size = sizeof(args);
	args.flags = flags;
	args.cmd_ptr = (uint64_t)(uintptr_t)cs;
	args.relocs_ptr = (uint64_t)(uintptr_t)relocs;
	args.cmd_dwords = cmd_dwords;
	args.nr_relocs = nr_relocs;

	do {
		ret = ioctl(fd, DRM_IOCTL_PSB_BLT_SUBMIT, &args);
	} while (ret == -1 && (errno == EINTR || errno == EAGAIN));

	return ret;
}

/*
 * Verify the whole buffer: every pixel inside the fill rect must be
 * `inside`, every pixel outside must be `outside`.  Returns number of
 * mismatching pixels (0 == good) and reports the first mismatch.
 */
static unsigned long check_pixels(const volatile uint32_t *px,
				  uint32_t pitch_bytes,
				  uint32_t inside, uint32_t outside)
{
	uint32_t stride_dw = pitch_bytes / 4;
	unsigned long bad = 0;
	uint32_t x, y;
	int first = 1;

	for (y = 0; y < BUF_H; y++) {
		for (x = 0; x < BUF_W; x++) {
			int in_rect = (x >= FILL_X && x < FILL_X + FILL_W &&
				       y >= FILL_Y && y < FILL_Y + FILL_H);
			uint32_t want = in_rect ? inside : outside;
			uint32_t got = px[y * stride_dw + x];

			if (got != want) {
				bad++;
				if (first) {
					printf("  first mismatch at (%u,%u): "
					       "got 0x%08x want 0x%08x\n",
					       x, y, got, want);
					first = 0;
				}
			}
		}
	}
	return bad;
}

/*
 * Run one negative test: submit must fail with `want_errno`, and the
 * buffer content (post-fill state) must be unchanged.
 */
static void negative_test(const char *name, int fd,
			  const uint32_t *cs, uint32_t cmd_dwords,
			  const struct drm_psb_blt_reloc *relocs,
			  uint32_t nr_relocs, uint32_t flags,
			  int want_errno,
			  const volatile uint32_t *px, uint32_t pitch_bytes)
{
	char detail[128];
	int ret, err;

	ret = submit(fd, cs, cmd_dwords, relocs, nr_relocs, flags);
	err = errno;

	if (ret == 0) {
		report(name, 0, "ioctl unexpectedly succeeded");
	} else if (err != want_errno) {
		snprintf(detail, sizeof(detail),
			 "expected errno %d (%s), got %d (%s)",
			 want_errno, strerror(want_errno), err, strerror(err));
		report(name, 0, detail);
	} else {
		report(name, 1, NULL);
	}

	/* rejected submit must not have touched the buffer */
	if (check_pixels(px, pitch_bytes, FILL_COLOUR, SENTINEL) == 0) {
		report("  buffer unchanged after rejected submit", 1, NULL);
	} else {
		report("  buffer unchanged after rejected submit", 0,
		       "buffer content modified by a rejected submit");
	}
}

int main(int argc, char **argv)
{
	const char *path = (argc > 1) ? argv[1] : "/dev/dri/card0";
	struct drm_mode_create_dumb create;
	struct drm_mode_map_dumb map;
	struct drm_mode_destroy_dumb destroy;
	struct drm_psb_blt_reloc reloc;
	uint32_t cs[DRM_PSB_BLT_MAX_DWORDS + 1];
	volatile uint32_t *px;
	void *ptr;
	unsigned long bad;
	char detail[128];
	int fd, ret;

	fd = open(path, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		fprintf(stderr, "open(%s): %s\n", path, strerror(errno));
		return 1;
	}
	printf("opened %s\n", path);

	/* --- 1. create + map our OWN dumb buffer ------------------------ */
	memset(&create, 0, sizeof(create));
	create.width = BUF_W;
	create.height = BUF_H;
	create.bpp = BUF_BPP;
	ret = ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
	if (ret) {
		fprintf(stderr, "MODE_CREATE_DUMB: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("dumb buffer: handle=%u pitch=%u size=%llu\n",
	       create.handle, create.pitch,
	       (unsigned long long)create.size);
	if (create.pitch % 4 != 0 || create.pitch < BUF_W * 4) {
		fprintf(stderr, "unusable pitch %u\n", create.pitch);
		close(fd);
		return 1;
	}

	memset(&map, 0, sizeof(map));
	map.handle = create.handle;
	ret = ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);
	if (ret) {
		fprintf(stderr, "MODE_MAP_DUMB: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	ptr = mmap(NULL, create.size, PROT_READ | PROT_WRITE, MAP_SHARED,
		   fd, (off_t)map.offset);
	if (ptr == MAP_FAILED) {
		fprintf(stderr, "mmap: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	px = ptr;

	/* sentinel pattern: every byte 0x11 -> every u32 0x11111111 */
	memset(ptr, 0x11, create.size);

	/* --- 2. positive test: solid fill + readback -------------------- */
	build_fill_stream(cs, create.pitch);
	memset(&reloc, 0, sizeof(reloc));
	reloc.offset_dw = 2;
	reloc.handle = create.handle;
	reloc.delta = 0;
	reloc.pad = 0;

	ret = submit(fd, cs, 8, &reloc, 1, DRM_PSB_BLT_SUBMIT_WAIT);
	if (ret) {
		snprintf(detail, sizeof(detail), "ioctl failed: %s",
			 strerror(errno));
		report("fill submit", 0, detail);
	} else {
		report("fill submit", 1, NULL);
	}

	bad = check_pixels(px, create.pitch, FILL_COLOUR, SENTINEL);
	if (bad != 0) {
		/*
		 * Known open risk: CPU/GPU coherency on cacheable dumb
		 * pages.  Give the readback one more chance after a
		 * short delay before declaring failure.
		 */
		printf("  %lu bad pixels; retrying readback after 10ms "
		       "(coherency check)\n", bad);
		usleep(10000);
		bad = check_pixels(px, create.pitch, FILL_COLOUR, SENTINEL);
	}
	if (bad == 0) {
		report("fill readback (rect filled, border untouched)",
		       1, NULL);
	} else {
		snprintf(detail, sizeof(detail),
			 "%lu mismatching pixels of %u", bad, BUF_W * BUF_H);
		report("fill readback (rect filled, border untouched)",
		       0, detail);
	}

	/* --- 3. negative tests ------------------------------------------ */
	/* Buffer is now in post-fill state; every negative test verifies
	 * it stays that way. */

	/* 3a. bad GEM handle -> ENOENT */
	{
		struct drm_psb_blt_reloc r = reloc;

		r.handle = 0xdeadbeef;
		build_fill_stream(cs, create.pitch);
		negative_test("reject bad GEM handle (ENOENT)", fd,
			      cs, 8, &r, 1, DRM_PSB_BLT_SUBMIT_WAIT,
			      ENOENT, px, create.pitch);
	}

	/* 3b. stream longer than DRM_PSB_BLT_MAX_DWORDS -> EINVAL */
	{
		memset(cs, 0, sizeof(cs));
		build_fill_stream(cs, create.pitch);
		/* cs[] has MAX+1 dwords of storage, so the pointer is
		 * fully readable even if the kernel copied first */
		negative_test("reject oversize stream (EINVAL)", fd,
			      cs, DRM_PSB_BLT_MAX_DWORDS + 1, &reloc, 1,
			      DRM_PSB_BLT_SUBMIT_WAIT,
			      EINVAL, px, create.pitch);
	}

	/* 3c. disallowed opcode (PAT_SURF_BH) in place of BLIT -> EINVAL */
	{
		build_fill_stream(cs, create.pitch);
		cs[3] = PSB_2D_PAT_SURF_BH;	/* not in the v1 allow-list */
		negative_test("reject disallowed opcode PAT_SURF_BH (EINVAL)",
			      fd, cs, 8, &reloc, 1, DRM_PSB_BLT_SUBMIT_WAIT,
			      EINVAL, px, create.pitch);
	}

	/* 3d. reloc delta beyond BO size -> EINVAL */
	{
		struct drm_psb_blt_reloc r = reloc;

		r.delta = (uint32_t)create.size;	/* == size: out of bounds */
		build_fill_stream(cs, create.pitch);
		negative_test("reject reloc beyond BO size (EINVAL)", fd,
			      cs, 8, &r, 1, DRM_PSB_BLT_SUBMIT_WAIT,
			      EINVAL, px, create.pitch);
	}

	/* 3e. reserved flags bit set -> EINVAL */
	{
		build_fill_stream(cs, create.pitch);
		negative_test("reject reserved flags bits (EINVAL)", fd,
			      cs, 8, &reloc, 1,
			      DRM_PSB_BLT_SUBMIT_WAIT | 0x2,
			      EINVAL, px, create.pitch);
	}

	/* --- 4. cleanup -------------------------------------------------- */
	munmap(ptr, create.size);
	memset(&destroy, 0, sizeof(destroy));
	destroy.handle = create.handle;
	ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
	close(fd);

	printf("\n%u passed, %u failed\n", g_pass, g_fail);
	return g_fail ? 1 : 0;
}
