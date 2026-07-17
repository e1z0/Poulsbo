/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * gma500_flip.h - EXPORT_SYMBOL'd zero-copy page-flip scanout API.
 *
 * Shared between the in-tree gma500 driver (gma500_flip.c) and the out-of-tree
 * display-class module (gma500dc.ko).  Uses only forward declarations so it can
 * be included from either build without dragging in gma500-private headers.
 */
#ifndef _GMA500_FLIP_H_
#define _GMA500_FLIP_H_

#include <linux/types.h>

struct drm_device;
struct gtt_range;
struct page;

/*
 * Descriptor for one GTT-pinned, scanout-capable, SGX-renderable buffer.
 * Layout MUST stay identical in both modules that use it.
 */
struct gma_flipbuf {
	struct gtt_range *gt;		/* owning gma500 GTT range */
	u32 gtt_offset;			/* GTT byte offset == scanout base for flip */
	struct page **pages;		/* backing pages (for SGX MMU import) */
	int npage;			/* number of backing pages */
	void *cpu;			/* optional kernel VA (vmap) or NULL */
};

/* Return the bound gma500 DRM device, or NULL if the driver has not probed. */
struct drm_device *gma500_get_drm(void);

/* Allocate + GTT-pin a scanout/render buffer.  0 on success, -errno on error. */
int gma500_flip_alloc(u32 width, u32 height, u32 *stride_out,
		      struct gma_flipbuf *out);

/* Unpin + free a buffer from gma500_flip_alloc(). */
void gma500_flip_free(struct gma_flipbuf *fb);

/* Page-flip pipe 0 scanout to @gtt_offset.  0 on success, -errno on error. */
int gma500_flip_to(u32 gtt_offset);

#endif /* _GMA500_FLIP_H_ */
