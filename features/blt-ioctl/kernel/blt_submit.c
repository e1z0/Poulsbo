// SPDX-License-Identifier: GPL-2.0-only
/*
 * blt_submit.c - DRM_PSB_BLT_SUBMIT: userspace 2D blit submission for
 * the gma500 (Poulsbo/SGX535) 2D blitter.
 *
 * Modeled on Patrik Jakobsson's GMA_GEM_BLT_SUBMIT (drm-gma500 branch
 * blitter_hacking), hardened with:
 *   - a strict opcode allow-list parser with exact per-block lengths
 *     (payload dwords are never re-interpreted as block headers),
 *   - out-of-band relocations with an exact-match invariant between
 *     reloc entries and parser-discovered address slots (userspace can
 *     never place a raw GPU address in the stream),
 *   - per-surface bounds checks against the GEM object size,
 *   - rejection of stolen-memory objects (the fbcon scanout can never
 *     be a blit target),
 *   - a single unwind path for all pins/refs (lesson from patjak
 *     commit d62d7c8d767),
 *   - sleeping serialization on dev_priv->mutex_2d; the FIFO burst
 *     itself (psb_2d_submit_user, accel_2d.c) runs under lock_2d for
 *     mutual exclusion with the live fbcon acceleration, and never
 *     resets the engine while holding that spinlock,
 *   - a power reference (gma_power_begin) held across every SGX
 *     register access, pin and unpin, so the graphics island cannot
 *     be runtime-suspended mid-submit and we never touch MMIO of a
 *     powered-down island.
 *
 * Locking order: mutex_2d -> gtt_mutex (inside psb_gtt_pin/unpin) ->
 * lock_2d (inside psb_2d_submit_user and psb_blt_reset_engine).
 * gtt_mutex and lock_2d are both leaves today, so this order is
 * acyclic.  gma_power_begin only takes its own leaf spinlock.
 *
 * All validation and patching happens on a kernel copy of the stream
 * obtained by a single copy_from_user; user memory is never re-read
 * after validation (no TOCTOU window).
 */

#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include <drm/drm_device.h>
#include <drm/drm_file.h>
#include <drm/drm_gem.h>

#include "psb_drv.h"
#include "psb_reg.h"
#include "gtt.h"
#include "power.h"
#include "gma_blt_drm.h"

/* Not defined in this tree's psb_reg.h */
#ifndef PSB_2D_BH_MASK
#define PSB_2D_BH_MASK			(0xF0000000)
#endif

/* Only defined locally in accel_2d.c */
#ifndef PSB_2D_ROP3_PATINVERT
#define PSB_2D_ROP3_PATINVERT		(0x5A5A)
#endif

/* 12-bit coordinate/size fields */
#define PSB_BLT_COORD_MASK		0xFFF
#define PSB_BLT_COORD_LIMIT		4096

/*
 * Decoded, validated blit operation.  Filled in by psb_blt_validate()
 * from the kernel copy of the command stream.
 */
struct psb_blt_op {
	bool is_copy;		/* BLIT_BH had PSB_2D_USE_PAT set */
	u32 dst_slot;		/* dword index of the dst address slot */
	u32 src_slot;		/* dword index of the src address slot
				 * (copy streams only) */
	u32 dst_stride;		/* bytes */
	u32 dst_cpp;
	u32 src_stride;		/* bytes (copy streams only) */
	u32 src_cpp;
	u32 dst_x, dst_y;
	u32 src_x, src_y;
	u32 size_x, size_y;
};

/*
 * Surface format allow-list.  SRC and DST use identical encodings in
 * the hdr & 0x00078000 field.  Everything else (palettized, alpha-only,
 * 4444/1555 ARGB, YUV, lookup, reserved) is rejected because we never
 * program palette/lookup state.  Returns bytes-per-pixel, 0 = reject.
 */
static u32 psb_blt_format_cpp(u32 format)
{
	switch (format) {
	case PSB_2D_DST_332RGB:
		return 1;
	case PSB_2D_DST_555RGB:
	case PSB_2D_DST_565RGB:
		return 2;
	case PSB_2D_DST_0888ARGB:
	case PSB_2D_DST_8888ARGB:
		return 4;
	default:
		return 0;
	}
}

/*
 * Highest byte offset (exclusive-ish: offset of the last byte touched,
 * plus nothing) a blit touches inside a surface, relative to the start
 * of the GEM object.  All math in u64: 12-bit coords times 15-bit
 * strides cannot overflow 32 bits, but do not rely on it.
 */
static u64 psb_blt_last_byte(u32 delta, u32 x, u32 y, u32 w, u32 h,
			     u32 stride, u32 cpp)
{
	return (u64)delta + (u64)(y + h - 1) * stride + (u64)(x + w) * cpp;
}

/* Window with no FIFO forward progress after which we call it a hang */
#define PSB_BLT_HANG_WINDOW	(HZ)
/* Overall cap on one drain wait, even with a busy-but-healthy engine */
#define PSB_BLT_WAIT_CAP	(4 * HZ)

/*
 * psb_blt_wait_idle_progress - sleeping engine drain with hang detection
 * @dev_priv: our DRM device
 *
 * gma_blt_wait_idle() busy-polls with a single shared deadline, so a
 * healthy engine kept merely busy by concurrent fbcon traffic (e.g. a
 * console scroll storm) is indistinguishable from a hang.  Here a hang
 * is declared only on lack of forward progress: any change in the FIFO
 * free-dword count or blit status restarts the hang window.  Process
 * context, only mutex_2d (plus a power reference) held, so we sleep
 * between samples instead of burning the single Atom core.
 *
 * Returns 0 once the engine is idle, -EIO if the engine state is
 * static for a full hang window (a real hang: caller should reset),
 * or -EBUSY if the engine is still making forward progress when the
 * overall cap expires (healthy but saturated: do NOT reset it).
 */
static int psb_blt_wait_idle_progress(struct drm_psb_private *dev_priv)
{
	unsigned long hang = jiffies + PSB_BLT_HANG_WINDOW;
	unsigned long cap = jiffies + PSB_BLT_WAIT_CAP;
	u32 socif = PSB_RSGX32(PSB_CR_2D_SOCIF);
	u32 status = PSB_RSGX32(PSB_CR_2D_BLIT_STATUS);

	for (;;) {
		u32 new_socif, new_status;

		if (socif == _PSB_C2_SOCIF_EMPTY &&
		    !(status & _PSB_C2B_STATUS_BUSY))
			return 0;
		if (time_after_eq(jiffies, hang))
			return -EIO;
		if (time_after_eq(jiffies, cap))
			return -EBUSY;

		usleep_range(100, 500);

		new_socif = PSB_RSGX32(PSB_CR_2D_SOCIF);
		new_status = PSB_RSGX32(PSB_CR_2D_BLIT_STATUS);
		if (new_socif != socif || new_status != status)
			hang = jiffies + PSB_BLT_HANG_WINDOW;
		socif = new_socif;
		status = new_status;
	}
}

/*
 * psb_blt_reset_engine - reset a hung 2D engine, serialized with fbcon
 * @dev_priv: our DRM device
 *
 * psb_spank() performs the full SGX soft-reset register sequence;
 * running it while fbcon is mid-burst into the slave port would leave
 * that stream half-consumed across the reset (payload dwords later
 * parsed as headers), so like every other engine access it must run
 * under lock_2d.  psb_spank() is atomic-safe (mdelay, see accel_2d.c).
 * The engine state is re-checked under the lock first so a stale
 * verdict cannot reset an engine that has since drained.  Callers
 * print their diagnostics *before* calling this: printk itself
 * generates fbcon 2D traffic and must not run under lock_2d.
 */
static void psb_blt_reset_engine(struct drm_psb_private *dev_priv)
{
	unsigned long flags;

	spin_lock_irqsave(&dev_priv->lock_2d, flags);
	if (PSB_RSGX32(PSB_CR_2D_SOCIF) != _PSB_C2_SOCIF_EMPTY ||
	    (PSB_RSGX32(PSB_CR_2D_BLIT_STATUS) & _PSB_C2B_STATUS_BUSY))
		psb_spank(dev_priv);
	spin_unlock_irqrestore(&dev_priv->lock_2d, flags);
}

/*
 * psb_blt_validate - strict allow-list parse of a user command stream
 * @cs: kernel copy of the stream
 * @n: stream length in dwords (already range-checked by the caller)
 * @op: decoded operation out-param
 *
 * Single left-to-right pass; every block advances the cursor by its
 * exact length, so payload dwords (fill colour, xy, wh, address slots)
 * are never re-interpreted as block headers.  Anything not on the
 * allow-list is rejected.  Returns 0 or -EINVAL.
 */
static int psb_blt_validate(const u32 *cs, u32 n, struct psb_blt_op *op)
{
	u32 n_fence = 0, n_dst = 0, n_src = 0, n_srcoff = 0;
	u32 n_blit = 0, n_flush = 0;
	bool seen_blit = false;
	bool last_was_flush = false;
	u32 i = 0;

	memset(op, 0, sizeof(*op));

	while (i < n) {
		u32 hdr = cs[i];

		last_was_flush = false;

		switch (hdr & PSB_2D_BH_MASK) {
		case PSB_2D_FENCE_BH:
			/* Exactly one, must be dword 0, low 28 bits zero */
			if (i != 0 || hdr != PSB_2D_FENCE_BH)
				return -EINVAL;
			n_fence++;
			i += 1;
			break;

		case PSB_2D_DST_SURF_BH:
			if (n_dst || seen_blit)
				return -EINVAL;
			if (i + 2 > n)
				return -EINVAL;
			if (hdr & ~(PSB_2D_BH_MASK | PSB_2D_DST_FORMAT_MASK |
				    PSB_2D_DST_STRIDE_MASK))
				return -EINVAL;
			op->dst_cpp =
			    psb_blt_format_cpp(hdr & PSB_2D_DST_FORMAT_MASK);
			if (!op->dst_cpp)
				return -EINVAL;
			op->dst_stride = hdr & PSB_2D_DST_STRIDE_MASK;
			if (!op->dst_stride)
				return -EINVAL;
			/* Address slot must be 0 on input */
			if (cs[i + 1] != 0)
				return -EINVAL;
			op->dst_slot = i + 1;
			n_dst++;
			i += 2;
			break;

		case PSB_2D_SRC_SURF_BH:
			if (n_src || seen_blit)
				return -EINVAL;
			if (i + 2 > n)
				return -EINVAL;
			if (hdr & ~(PSB_2D_BH_MASK | PSB_2D_SRC_FORMAT_MASK |
				    PSB_2D_SRC_STRIDE_MASK))
				return -EINVAL;
			op->src_cpp =
			    psb_blt_format_cpp(hdr & PSB_2D_SRC_FORMAT_MASK);
			if (!op->src_cpp)
				return -EINVAL;
			op->src_stride = hdr & PSB_2D_SRC_STRIDE_MASK;
			if (!op->src_stride)
				return -EINVAL;
			/* Address slot must be 0 on input */
			if (cs[i + 1] != 0)
				return -EINVAL;
			op->src_slot = i + 1;
			n_src++;
			i += 2;
			break;

		case PSB_2D_SRC_OFF_BH:
			if (n_srcoff || seen_blit)
				return -EINVAL;
			/* x in bits 12..23, y in bits 0..11, rest zero */
			if (hdr & ~(PSB_2D_BH_MASK | 0x00FFF000 | 0x00000FFF))
				return -EINVAL;
			op->src_x = (hdr >> PSB_2D_SRCOFF_XSTART_SHIFT) &
				    PSB_BLT_COORD_MASK;
			op->src_y = hdr & PSB_BLT_COORD_MASK;
			n_srcoff++;
			i += 1;
			break;

		case PSB_2D_BLIT_BH: {
			u32 len, rop, xy, wh;

			if (n_blit)
				return -EINVAL;
			/*
			 * Only USE_PAT and the ROP3 code may be set.  This
			 * single mask forces ROT_NONE, DSTCK_DISABLE,
			 * SRCCK_DISABLE, no alpha, no clip and
			 * COPYORDER_TL2BR (bits 23-24 zero; v1 forbids
			 * reverse copy orders).
			 */
			if (hdr & ~(PSB_2D_BH_MASK | PSB_2D_USE_PAT | 0xFFFF))
				return -EINVAL;
			op->is_copy = (hdr & PSB_2D_USE_PAT) != 0;
			rop = hdr & 0xFFFF;
			if (op->is_copy) {
				if (rop != PSB_2D_ROP3_SRCCOPY)
					return -EINVAL;
				len = 3;	/* hdr, xy, wh */
			} else {
				if (rop != PSB_2D_ROP3_PATCOPY &&
				    rop != PSB_2D_ROP3_PATINVERT &&
				    rop != PSB_2D_ROP3_BLACKNESS &&
				    rop != PSB_2D_ROP3_WHITENESS)
					return -EINVAL;
				len = 4;	/* hdr, colour, xy, wh */
			}
			if (i + len > n)
				return -EINVAL;
			xy = cs[i + len - 2];
			wh = cs[i + len - 1];
			/* bits 24-31 of xy/wh must be zero */
			if ((xy & 0xFF000000) || (wh & 0xFF000000))
				return -EINVAL;
			op->dst_x = (xy >> PSB_2D_DST_XSTART_SHIFT) &
				    PSB_BLT_COORD_MASK;
			op->dst_y = xy & PSB_BLT_COORD_MASK;
			op->size_x = (wh >> PSB_2D_DST_XSIZE_SHIFT) &
				     PSB_BLT_COORD_MASK;
			op->size_y = wh & PSB_BLT_COORD_MASK;
			n_blit++;
			seen_blit = true;
			i += len;
			break;
		}

		case PSB_2D_FLUSH_BH:
			/* Exactly one, low 28 bits zero */
			if (n_flush || hdr != PSB_2D_FLUSH_BH)
				return -EINVAL;
			n_flush++;
			last_was_flush = true;
			i += 1;
			break;

		default:
			/*
			 * CLIP, PAT, CTRL, MASK_OFF, RESERVED, PAT_SURF,
			 * SRC_PAL, PAT_PAL, MASK_SURF: no palette/mask/
			 * pattern/clip programming in v1.
			 */
			return -EINVAL;
		}
	}

	/* Structural template */
	if (n_fence != 1 || n_dst != 1 || n_blit != 1 || n_flush != 1)
		return -EINVAL;
	if (!last_was_flush)
		return -EINVAL;
	if (op->is_copy) {
		if (n_src != 1 || n_srcoff != 1)
			return -EINVAL;
	} else {
		if (n_src != 0 || n_srcoff != 0)
			return -EINVAL;
	}

	/* Geometry: no zero extents, no 12-bit field wrap */
	if (!op->size_x || !op->size_y)
		return -EINVAL;
	if (op->dst_x + op->size_x > PSB_BLT_COORD_LIMIT ||
	    op->dst_y + op->size_y > PSB_BLT_COORD_LIMIT)
		return -EINVAL;
	if (op->is_copy) {
		if (op->src_x + op->size_x > PSB_BLT_COORD_LIMIT ||
		    op->src_y + op->size_y > PSB_BLT_COORD_LIMIT)
			return -EINVAL;
		/*
		 * 8-pixel copy erratum, mirrors the psbfb_copyarea guard
		 * in accel_2d.c.  Fills are unaffected, matching fbcon.
		 */
		if (op->size_x == 8 || op->size_y == 8)
			return -EINVAL;
	}

	return 0;
}

/**
 *	psb_blt_submit_ioctl	-	DRM_PSB_BLT_SUBMIT handler
 *	@dev: our DRM device
 *	@data: struct drm_psb_blt_submit from userspace
 *	@file_priv: submitting DRM file
 *
 *	Validate a user 2D command stream against the opcode allow-list,
 *	patch the address slots from the reloc table (bounds-checked
 *	against each GEM object), pin the surfaces, submit through the
 *	slave-port FIFO under the same lock_2d serialization fbcon uses
 *	and synchronously wait for engine idle before unpinning.  A
 *	power reference is held across the whole sequence.
 */
int psb_blt_submit_ioctl(struct drm_device *dev, void *data,
			 struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_psb_blt_submit *args = data;
	u32 cs[DRM_PSB_BLT_MAX_DWORDS];
	struct drm_psb_blt_reloc rel[DRM_PSB_BLT_MAX_RELOCS];
	struct drm_gem_object *objs[DRM_PSB_BLT_MAX_RELOCS] = { NULL, NULL };
	bool pinned[DRM_PSB_BLT_MAX_RELOCS] = { false, false };
	struct psb_blt_op op;
	u32 nr_slots, patched;
	int ret = 0;
	int ret2;
	int i;

	if (!dev_priv)
		return -ENODEV;
	if (!args || args->size < sizeof(*args))
		return -EINVAL;
	if (args->flags & ~DRM_PSB_BLT_SUBMIT_FLAGS_MASK)
		return -EINVAL;
	if (args->cmd_dwords < DRM_PSB_BLT_MIN_DWORDS ||
	    args->cmd_dwords > DRM_PSB_BLT_MAX_DWORDS)
		return -EINVAL;
	if (args->nr_relocs > DRM_PSB_BLT_MAX_RELOCS)
		return -EINVAL;
	/*
	 * User pointers must round-trip through the native pointer width:
	 * on the i686 target u64_to_user_ptr() would silently truncate,
	 * aliasing a garbage pointer onto a valid low address.
	 */
	if ((u64)(unsigned long)args->cmd_ptr != args->cmd_ptr ||
	    (u64)(unsigned long)args->relocs_ptr != args->relocs_ptr)
		return -EINVAL;

	/*
	 * Single copy of stream and relocs; everything below operates on
	 * the kernel copies only.
	 */
	if (copy_from_user(cs, u64_to_user_ptr(args->cmd_ptr),
			   (size_t)args->cmd_dwords * sizeof(u32)))
		return -EFAULT;
	if (args->nr_relocs &&
	    copy_from_user(rel, u64_to_user_ptr(args->relocs_ptr),
			   (size_t)args->nr_relocs * sizeof(rel[0])))
		return -EFAULT;

	for (i = 0; i < args->nr_relocs; i++) {
		if (rel[i].pad != 0)
			return -EINVAL;
		if (rel[i].delta & 3)
			return -EINVAL;
		if (rel[i].offset_dw >= args->cmd_dwords)
			return -EINVAL;
	}
	/* offset_dw values must be pairwise distinct */
	if (args->nr_relocs == 2 && rel[0].offset_dw == rel[1].offset_dw)
		return -EINVAL;

	ret = psb_blt_validate(cs, args->cmd_dwords, &op);
	if (ret)
		return ret;

	/*
	 * Exact-match invariant: the set of reloc offsets must equal the
	 * set of parser-discovered address slots.  With pairwise-distinct
	 * offsets and equal cardinality, membership of every reloc in the
	 * slot set implies full coverage of the slots.
	 */
	nr_slots = op.is_copy ? 2 : 1;
	if (args->nr_relocs != nr_slots)
		return -EINVAL;
	for (i = 0; i < args->nr_relocs; i++) {
		if (rel[i].offset_dw != op.dst_slot &&
		    !(op.is_copy && rel[i].offset_dw == op.src_slot))
			return -EINVAL;
	}

	/*
	 * Locking order (frozen): mutex_2d -> gtt_mutex (inside
	 * psb_gtt_pin/unpin) -> lock_2d (inside psb_2d_submit_user and
	 * psb_blt_reset_engine).
	 */
	mutex_lock(&dev_priv->mutex_2d);

	/*
	 * Pin the graphics island up (force resume: this is an explicit
	 * user submit) for the whole pin -> submit -> wait -> unpin
	 * sequence.  Without this, GTT PTE writes are dropped and SGX
	 * reads return junk on a runtime-suspended island, and nothing
	 * would stop a concurrent suspend mid-burst.
	 */
	if (!gma_power_begin(dev, true)) {
		mutex_unlock(&dev_priv->mutex_2d);
		return -EIO;
	}

	patched = 0;
	for (i = 0; i < args->nr_relocs; i++) {
		struct gtt_range *gt;
		bool is_dst = (rel[i].offset_dw == op.dst_slot);
		u64 last_byte;

		objs[i] = drm_gem_object_lookup(file_priv, rel[i].handle);
		if (!objs[i]) {
			ret = -ENOENT;
			goto unwind;
		}
		gt = to_gtt_range(objs[i]);

		/*
		 * Never allow stolen memory as a blit surface: the fbcon
		 * scanout is stolen-backed, so it can never be a target,
		 * even via a leaked handle.
		 */
		if (gt->stolen) {
			ret = -EINVAL;
			goto unwind;
		}

		if (is_dst)
			last_byte = psb_blt_last_byte(rel[i].delta,
						      op.dst_x, op.dst_y,
						      op.size_x, op.size_y,
						      op.dst_stride,
						      op.dst_cpp);
		else
			last_byte = psb_blt_last_byte(rel[i].delta,
						      op.src_x, op.src_y,
						      op.size_x, op.size_y,
						      op.src_stride,
						      op.src_cpp);
		if (last_byte > (u64)objs[i]->size) {
			ret = -EINVAL;
			goto unwind;
		}

		ret = psb_gtt_pin(gt);
		if (ret)
			goto unwind;
		pinned[i] = true;

		/*
		 * The engine address space is 28 bits, 4-byte aligned
		 * (PSB_2D_DST_ADDR_MASK).  gt->offset is a GTT byte
		 * offset, valid directly because BIF_TWOD_REQ_BASE ==
		 * gatt_start (re-asserted by psb_spank after any reset).
		 */
		if ((((u64)gt->offset + rel[i].delta) & 3) ||
		    ((u64)gt->offset + last_byte) > PSB_2D_DST_ADDR_MASK) {
			ret = -EINVAL;
			goto unwind;
		}

		cs[rel[i].offset_dw] = gt->offset + rel[i].delta;
		patched++;
	}

	/* Every parser-discovered address slot must have been patched */
	if (patched != nr_slots) {
		ret = -EINVAL;
		goto unwind;
	}

	/*
	 * Pre-drain the engine with a sleeping, progress-aware wait (only
	 * mutex_2d and a power reference held) so the bounded in-spinlock
	 * FIFO wait inside psb_2d_submit_user is effectively instant for
	 * our single <= 96-dword burst.  -EBUSY means the engine is still
	 * making forward progress under fbcon load: back off without
	 * resetting a healthy engine.  Only a static FIFO (-EIO) is a
	 * hang worth a reset.
	 */
	ret = psb_blt_wait_idle_progress(dev_priv);
	if (ret) {
		if (ret == -EIO) {
			dev_err(dev->dev,
				"blt_submit: engine hung on pre-drain, resetting\n");
			psb_blt_reset_engine(dev_priv);
		}
		goto unwind;
	}

	ret = psb_2d_submit_user(dev_priv, cs, args->cmd_dwords);

	/*
	 * v1 submits are always synchronous, whether or not
	 * DRM_PSB_BLT_SUBMIT_WAIT is set: psb_gtt_unpin in this tree does
	 * NOT quiesce the blitter, so the surfaces must stay pinned until
	 * the engine has drained.
	 */
	ret2 = psb_blt_wait_idle_progress(dev_priv);
	if (ret2 == -EIO) {
		dev_err(dev->dev, "blt_submit: idle timeout, resetting\n");
		psb_blt_reset_engine(dev_priv);
	}
	if (!ret)
		ret = ret2;

unwind:
	/*
	 * Unpin without psb_gtt_unpin's internal blitter idle-wait, whose
	 * silent -EBUSY bail would permanently leak the pin whenever
	 * fbcon keeps the engine busy past its 1s window.  Skipping the
	 * wait is safe: only OUR burst can reference these pages, and at
	 * this point either it never reached the FIFO (early errors), or
	 * we drained/reset the engine above, or (post-submit -EBUSY) the
	 * strictly-ordered FIFO made forward progress for the entire
	 * multi-second cap, which long since consumed and executed our
	 * <= 96 dwords ahead of any later fbcon traffic.
	 */
	for (i = (int)args->nr_relocs - 1; i >= 0; i--) {
		if (pinned[i])
			psb_gtt_unpin_idle(to_gtt_range(objs[i]));
		if (objs[i])
			drm_gem_object_put_unlocked(objs[i]);
	}
	gma_power_end(dev);
	mutex_unlock(&dev_priv->mutex_2d);
	return ret;
}
