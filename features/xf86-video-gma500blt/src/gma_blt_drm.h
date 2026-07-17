/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * gma_blt_drm.h - UAPI for the gma500 (Poulsbo/SGX535) 2D blitter
 * submit ioctl (DRM_PSB_BLT_SUBMIT).
 *
 * Modeled on Patrik Jakobsson's GMA_GEM_BLT_SUBMIT (drm-gma500 branch
 * blitter_hacking), reworked to use out-of-band relocations and a
 * strict kernel-side opcode allow-list.
 *
 * This header MUST stay byte-identical in both locations:
 *   build/devnull-vaio-gma500-src/gma_blt_drm.h                        (kernel)
 *   third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/gma_blt_drm.h (userspace -I)
 *
 * Command stream contract (v1):
 *
 *   The stream is a sequence of little-endian 32-bit dwords in the
 *   native SGX535 2D slave-port format (see psb_reg.h for the PSB_2D_*
 *   defines).  Only the following blocks are accepted, with EXACT
 *   lengths; anything else is rejected with -EINVAL:
 *
 *     PSB_2D_FENCE_BH    (0x70000000)  1 dword, must be dword 0
 *     PSB_2D_DST_SURF_BH (0xA0000000)  2 dwords: header | dst format |
 *                                      byte stride; payload dword is an
 *                                      ADDRESS SLOT (see below)
 *     PSB_2D_SRC_SURF_BH (0x90000000)  2 dwords, copy streams only;
 *                                      payload dword is an ADDRESS SLOT
 *     PSB_2D_SRC_OFF_BH  (0x30000000)  1 dword, copy streams only
 *     PSB_2D_BLIT_BH     (0x80000000)  4 dwords for fill (header,
 *                                      fill colour, xy, wh) or
 *                                      3 dwords for copy (header, xy, wh)
 *     PSB_2D_FLUSH_BH    (0xF0000000)  1 dword, must be the last dword
 *
 *   ADDRESS SLOTS: userspace must write 0 in every surface address
 *   payload dword and describe each one with exactly one reloc entry.
 *   The kernel validates the stream, bounds-checks each reloc against
 *   the GEM object's size, then patches the slot with the object's
 *   GTT byte offset + delta.  A stream is rejected unless the set of
 *   reloc offsets matches the set of address slots exactly.
 *
 *   Userspace never supplies a GPU address; stolen-memory objects
 *   (e.g. the fbcon scanout) are rejected as blit targets.
 *
 *   v1 submits are synchronous: the ioctl always waits (bounded, 1s)
 *   for 2D engine idle before returning, whether or not
 *   DRM_PSB_BLT_SUBMIT_WAIT is set.  Set the flag anyway if you rely
 *   on completion-on-return; it is guaranteed to keep that meaning in
 *   any future version.
 */
#ifndef _GMA_BLT_DRM_H_
#define _GMA_BLT_DRM_H_

#include <drm/drm.h>

/*
 * DRM_COMMAND_BASE offsets 0x00..0x07 are used by the SGX lab ioctls
 * (psb_drm_lab.h); 0x08 is the first free slot.
 */
#define DRM_PSB_BLT_SUBMIT		0x08

/* flags for drm_psb_blt_submit.flags */
#define DRM_PSB_BLT_SUBMIT_WAIT		0x00000001
#define DRM_PSB_BLT_SUBMIT_FLAGS_MASK	(DRM_PSB_BLT_SUBMIT_WAIT)

/* stream and reloc limits */
#define DRM_PSB_BLT_MIN_DWORDS		8	/* smallest valid (fill) stream */
#define DRM_PSB_BLT_MAX_DWORDS		96	/* one 2D slave-port burst */
#define DRM_PSB_BLT_MAX_RELOCS		2	/* 1 = fill, 2 = copy */

/*
 * One relocation entry.  offset_dw indexes the command stream (in
 * dwords) and must land exactly on the address payload dword of a
 * DST_SURF_BH or SRC_SURF_BH block.
 */
struct drm_psb_blt_reloc {
	__u32 offset_dw;	/* dword index of the address slot to patch */
	__u32 handle;		/* GEM handle of the target buffer object */
	__u32 delta;		/* byte offset inside the target object;
				 * must be a multiple of 4 */
	__u32 pad;		/* must be 0 */
};

/*
 * Ioctl argument.  32 bytes; identical layout on i386 and x86_64
 * (all __u64 members sit on natural 8-byte offsets).
 */
struct drm_psb_blt_submit {
	__u32 size;		/* sizeof(struct drm_psb_blt_submit),
				 * set by caller (tree convention) */
	__u32 flags;		/* DRM_PSB_BLT_SUBMIT_* */
	__u64 cmd_ptr;		/* user pointer to __u32 command stream */
	__u64 relocs_ptr;	/* user pointer to
				 * struct drm_psb_blt_reloc[nr_relocs] */
	__u32 cmd_dwords;	/* stream length in dwords,
				 * DRM_PSB_BLT_MIN_DWORDS..DRM_PSB_BLT_MAX_DWORDS */
	__u32 nr_relocs;	/* number of reloc entries,
				 * <= DRM_PSB_BLT_MAX_RELOCS */
};

#define DRM_IOCTL_PSB_BLT_SUBMIT \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_BLT_SUBMIT, \
		 struct drm_psb_blt_submit)

#endif /* _GMA_BLT_DRM_H_ */
