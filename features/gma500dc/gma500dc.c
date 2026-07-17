/*
 * gma500dc.c - Minimal PowerVR SGX display-class kernel module for GMA500.
 *
 * Purpose (MINIMAL, deliberately not a full display driver):
 *   Register ONE 3rd-party display-class device with the pvrsrvkm services
 *   kernel module so that userspace PVR2D can:
 *       - enumerate a display device,
 *       - PVR2DCreateDeviceContext() succeeds,
 *       - PVR2DMemAlloc() (SGX device memory) succeeds.
 *   This unblocks a userspace WSEGL "null window" path.
 *
 *   We do NOT implement real display flipping. We register working
 *   Open/Close/Info/EnumFormats/EnumDims/GetDCSystemBuffer/GetDCBufferAddr
 *   callbacks backed by one contiguous-enough kernel system buffer, plus
 *   stubbed swap-chain callbacks. PVR2DCreateDeviceContext only needs a
 *   registered DC device with a valid system buffer; SGX memory allocation
 *   goes through the services bridge, not through this display class.
 *
 * Structure/signature reference (EXACT jtable + callback shapes):
 *   services4/3rdparty/emgd_displayclass/emgd_dc.c  (do NOT copy wholesale;
 *   that file is 2841 lines coupled to the closed EMGD kernel HAL).
 *
 * Symbol we consume from pvrsrvkm (EXPORT_SYMBOL'd):
 *   extern IMG_BOOL PVRGetDisplayClassJTable(PVRSRV_DC_DISP2SRV_KMJTABLE *);
 *
 * Registration sequence (mirrors emgddc_init_devinfo()):
 *   1. PVRGetDisplayClassJTable(&pvr_jtable)          -> DISP2SRV table
 *   2. fill PVRSRV_DC_SRV2DISP_KMJTABLE (our callbacks)
 *   3. pvr_jtable.pfnPVRSRVRegisterDCDevice(&dc_jtable, &device_id)
 *   teardown:
 *      pvr_jtable.pfnPVRSRVRemoveDCDevice(device_id)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/io.h>		/* page_to_phys() */
#include <linux/version.h>

/* PVR services kernel headers (same include dirs as the pvrsrvkm port).
 * Order matters: img_defs.h pulls in img_types.h; servicesext.h defines the
 * DISPLAY_* / PVRSRV_* types and enums; kerneldisplay.h defines the two
 * jump-table structs and the PFN_* callback typedefs. */
#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"

/* Exported by pvrsrvkm: services4/srvkm/env/linux/module.c
 *   EXPORT_SYMBOL(PVRGetDisplayClassJTable) */
extern IMG_BOOL PVRGetDisplayClassJTable(PVRSRV_DC_DISP2SRV_KMJTABLE *jtable);

/* -------------------------------------------------------------------------
 * Zero-copy flip helpers EXPORT_SYMBOL'd by gma500_gfx (gma500_flip.c).
 * This module now depends on BOTH pvrsrvkm AND gma500_gfx; the Makefile lists
 * both Module.symvers in KBUILD_EXTRA_SYMBOLS so these resolve at modpost.
 *
 * struct gma_flipbuf MUST stay byte-for-byte identical to the definition in
 * gma500_flip.h (both modules allocate/read it).  struct gtt_range/page are
 * only referenced by pointer here, so forward declarations suffice.
 * ------------------------------------------------------------------------- */
struct gtt_range;
struct gma_flipbuf {
	struct gtt_range *gt;
	u32               gtt_offset;
	struct page     **pages;
	int               npage;
	void             *cpu;
};

extern struct drm_device *gma500_get_drm(void);
extern int  gma500_flip_alloc(u32 width, u32 height, u32 *stride_out,
			      struct gma_flipbuf *out);
extern void gma500_flip_free(struct gma_flipbuf *fb);
extern int  gma500_flip_to(u32 gtt_offset);

/*
 * flip_enable - when non-zero (default), a PVR2D present page-flips the display
 * to the just-rendered swap-chain buffer (the zero-copy present).  Set to 0 to
 * disable the actual scanout flip: GL still renders into the swap-chain buffers
 * and every flip command is still completed (no stall), but the display is left
 * on its current buffer.  A diagnostic / safety escape hatch to isolate the
 * scanout flip from the rest of the GL pipeline.
 *   /sys/module/gma500dc/parameters/flip_enable
 */
static int flip_enable = 1;
module_param(flip_enable, int, 0644);
MODULE_PARM_DESC(flip_enable,
	"Page-flip the display on PVR2D present (1=on/zero-copy flip [default], "
	"0=off/leave scanout in place)");

/* Gate every scanout flip through the flip_enable parameter. */
static int gma500dc_do_flip(u32 gtt_offset)
{
	if (!flip_enable)
		return 0;
	return gma500_flip_to(gtt_offset);
}

#define GMA500DC_NAME        "gma500dc"
#define GMA500DC_DISPLAY_NAME "GMA500 SGX Display (minimal)"

/* Framebuffer geometry. 1600x768x4 = 4915200 bytes. A plain kernel buffer is
 * fine here: PVR2D only needs *a* device + *a* system buffer with valid page
 * addresses to enumerate and create a context. */
#define GMA500DC_WIDTH   1600U
#define GMA500DC_HEIGHT   768U
#define GMA500DC_BPP        4U
#define GMA500DC_STRIDE  (GMA500DC_WIDTH * GMA500DC_BPP)
#define GMA500DC_FB_SIZE (GMA500DC_STRIDE * GMA500DC_HEIGHT)

struct gma500dc_swapchain_tag;

/* Per-buffer record. Opaque to PVR services (passed back as a handle from
 * GetDCSystemBuffer / GetDCBuffers and consumed by GetDCBufferAddr). Used both
 * for the enumeration "system buffer" (vmalloc'd, is_flip==0) and for real
 * swap-chain buffers (GTT-pinned gma_flipbuf, is_flip==1). */
typedef struct gma500dc_buffer_tag {
	IMG_CPU_VIRTADDR   virt_addr;   /* kernel virtual address               */
	IMG_UINT32         size;        /* byte size of the buffer              */
	IMG_UINT32         num_pages;   /* number of PAGE_SIZE pages            */
	IMG_SYS_PHYADDR   *phys_pages;  /* per-page system phys addr array      */
	PVRSRV_SYNC_DATA  *sync_data;   /* set in OpenDCDevice / CreateDCSwapChain */

	/* Flip / scanout fields (swap-chain buffers only; is_flip==0 otherwise) */
	IMG_BOOL           is_flip;     /* backed by a GTT-pinned gma_flipbuf    */
	IMG_UINT32         gtt_offset;  /* scanout base address for the flip     */
	struct gma_flipbuf fb;          /* owning gma500 flipbuf (gt==NULL if none) */
	struct gma500dc_swapchain_tag *swap_chain; /* owner (swap buffers only)  */
} gma500dc_buffer_t;

/* One flip-able swap chain (created by CreateDCSwapChain). */
typedef struct gma500dc_swapchain_tag {
	struct gma500dc_devinfo_tag *devinfo;
	IMG_UINT32          buffer_count;
	gma500dc_buffer_t  *buffers;    /* array[buffer_count]                   */
	IMG_UINT32          flags;
	IMG_BOOL            valid;
	struct gma500dc_swapchain_tag *next;
} gma500dc_swapchain_t;

/* Minimal device record. One global instance == one DC device. */
typedef struct gma500dc_devinfo_tag {
	IMG_UINT32                  device_id;      /* assigned by services      */
	PVRSRV_DC_DISP2SRV_KMJTABLE pvr_jtable;     /* services -> us table copy */
	PVRSRV_DC_SRV2DISP_KMJTABLE dc_jtable;      /* our callbacks             */
	DISPLAY_INFO                display_info;
	DISPLAY_FORMAT              display_format; /* single format             */
	DISPLAY_DIMS                display_dim;    /* single dimension          */
	gma500dc_buffer_t           system_buffer;
	gma500dc_swapchain_t       *swap_chains;    /* linked list               */
	IMG_UINT32                  swap_chain_id_counter;
	IMG_BOOL                    cmd_proc_registered;
	IMG_BOOL                    registered;
} gma500dc_devinfo_t;

static gma500dc_devinfo_t *g_devinfo;

/* Number of DC command types we register a processor for (just DC_FLIP). */
#define GMA500DC_COMMAND_COUNT  (DC_FLIP_COMMAND + 1)

/* Clamp for swap-chain buffer counts (double / triple buffering). */
#define GMA500DC_MIN_SWAP_BUFFERS  2U
#define GMA500DC_MAX_SWAP_BUFFERS  3U

/* -------------------------------------------------------------------------
 * DC callbacks (PVRSRV_DC_SRV2DISP_KMJTABLE). Signatures mirror the PFN_*
 * typedefs in kerneldisplay.h / servicesext.h EXACTLY.
 * ------------------------------------------------------------------------- */

static PVRSRV_ERROR OpenDCDevice(IMG_UINT32 device_id,
	IMG_HANDLE *device_h,
	PVRSRV_SYNC_DATA *system_buffer_sync_data)
{
	if (!g_devinfo || device_id != g_devinfo->device_id) {
		printk(KERN_ERR "[gma500dc] OpenDCDevice: unknown device id %u\n",
			device_id);
		return PVRSRV_ERROR_INVALID_DEVICE;
	}
	/* Services hands us the system buffer's sync data on open. */
	g_devinfo->system_buffer.sync_data = system_buffer_sync_data;
	*device_h = (IMG_HANDLE) g_devinfo;
	return PVRSRV_OK;
}

static PVRSRV_ERROR CloseDCDevice(IMG_HANDLE device_h)
{
	if (!device_h || device_h != (IMG_HANDLE) g_devinfo)
		return PVRSRV_ERROR_INVALID_PARAMS;
	return PVRSRV_OK;
}

/* Called twice: first with format==NULL to learn the count, then with a
 * caller-allocated array to fill. We report exactly one format. */
static PVRSRV_ERROR EnumDCFormats(IMG_HANDLE device_h,
	IMG_UINT32 *num_formats,
	DISPLAY_FORMAT *format)
{
	gma500dc_devinfo_t *dev = (gma500dc_devinfo_t *) device_h;

	if (!device_h || !num_formats || dev != g_devinfo)
		return PVRSRV_ERROR_INVALID_PARAMS;

	*num_formats = 1;
	if (format)
		format[0] = dev->display_format;
	return PVRSRV_OK;
}

/* Called twice, like EnumDCFormats. We report exactly one dimension. */
static PVRSRV_ERROR EnumDCDims(IMG_HANDLE device_h,
	DISPLAY_FORMAT *format,
	IMG_UINT32 *num_dims,
	DISPLAY_DIMS *dims)
{
	gma500dc_devinfo_t *dev = (gma500dc_devinfo_t *) device_h;

	if (!device_h || !format || !num_dims || dev != g_devinfo)
		return PVRSRV_ERROR_INVALID_PARAMS;

	*num_dims = 1;
	if (dims)
		dims[0] = dev->display_dim;
	return PVRSRV_OK;
}

static PVRSRV_ERROR GetDCSystemBuffer(IMG_HANDLE device_h, IMG_HANDLE *buffer_h)
{
	gma500dc_devinfo_t *dev = (gma500dc_devinfo_t *) device_h;

	if (!device_h || !buffer_h || dev != g_devinfo)
		return PVRSRV_ERROR_INVALID_PARAMS;

	*buffer_h = (IMG_HANDLE) &dev->system_buffer;
	return PVRSRV_OK;
}

static PVRSRV_ERROR GetDCInfo(IMG_HANDLE device_h, DISPLAY_INFO *dc_info)
{
	gma500dc_devinfo_t *dev = (gma500dc_devinfo_t *) device_h;

	if (!device_h || !dc_info || dev != g_devinfo)
		return PVRSRV_ERROR_INVALID_PARAMS;

	*dc_info = dev->display_info;
	return PVRSRV_OK;
}

/*
 * GetDCBufferAddr: hand services the page-address array + kernel VA of a
 * buffer so it can map it. Signature matches PFN_GET_BUFFER_ADDR in
 * servicesext.h EXACTLY (8 params, IMG_SIZE_T* size, trailing IMG_UINT32*).
 *
 * ON-TARGET RISK: the exact semantics services expects from *phys_addr:
 *   - We return a per-page array of IMG_SYS_PHYADDR and set *is_contiguous
 *     = IMG_FALSE, mirroring EMGD's non-contiguous model (emgd_dc.c returns
 *     the gmm page-list array). This is the shape services' deviceclass.c
 *     consumes when the buffer is not flagged contiguous.
 *   - Our buffer is vmalloc'd => genuinely non-contiguous in phys, so the
 *     per-page array is correct. If a future revision instead wants a single
 *     base phys addr with is_contiguous=IMG_TRUE, switch the buffer to
 *     dma_alloc_coherent / __get_free_pages and return &phys_pages[0] with
 *     is_contiguous=IMG_TRUE. VERIFY ON HARDWARE which path services takes.
 */
static PVRSRV_ERROR GetDCBufferAddr(IMG_HANDLE device_h,
	IMG_HANDLE buffer_h,
	IMG_SYS_PHYADDR **phys_addr,
	IMG_SIZE_T *fb_size,
	IMG_VOID **virt_addr,
	IMG_HANDLE *os_map_info_h,
	IMG_BOOL *is_contiguous,
	IMG_UINT32 *pui32_flags)
{
	gma500dc_devinfo_t *dev = (gma500dc_devinfo_t *) device_h;
	gma500dc_buffer_t  *buf = (gma500dc_buffer_t *) buffer_h;

	if (!device_h || dev != g_devinfo)
		return PVRSRV_ERROR_INVALID_PARAMS;
	/* Accept the enumeration system buffer OR any swap-chain buffer (both
	 * carry a valid per-page phys array). */
	if (!buf || !buf->phys_pages)
		return PVRSRV_ERROR_INVALID_PARAMS;
	if (!phys_addr)
		return PVRSRV_ERROR_INVALID_PARAMS;

	*phys_addr = buf->phys_pages;          /* array of per-page sys phys */

	if (fb_size)
		*fb_size = (IMG_SIZE_T) buf->size;
	if (virt_addr)
		*virt_addr = buf->virt_addr;
	if (os_map_info_h)                     /* ignored by services */
		*os_map_info_h = (IMG_HANDLE) NULL;
	if (is_contiguous)
		*is_contiguous = IMG_FALSE;        /* page-backed => non-contiguous */
	if (pui32_flags)                       /* ON-TARGET: purpose unclear; 0 */
		*pui32_flags = 0;

	return PVRSRV_OK;
}

/* -------------------------------------------------------------------------
 * Swap-chain helpers.
 * ------------------------------------------------------------------------- */

static int is_valid_swap_chain(gma500dc_devinfo_t *dev,
	gma500dc_swapchain_t *sc)
{
	gma500dc_swapchain_t *p;

	if (dev != g_devinfo || !sc)
		return 0;
	for (p = dev->swap_chains; p; p = p->next)
		if (p == sc)
			return sc->valid;
	return 0;
}

/* Free every GTT-pinned buffer in a swap chain plus its bookkeeping arrays. */
static void free_swap_chain(gma500dc_swapchain_t *sc)
{
	IMG_UINT32 i;

	if (!sc)
		return;
	if (sc->buffers) {
		for (i = 0; i < sc->buffer_count; i++) {
			gma500dc_buffer_t *b = &sc->buffers[i];

			if (b->phys_pages) {
				kfree(b->phys_pages);
				b->phys_pages = NULL;
			}
			if (b->fb.gt)
				gma500_flip_free(&b->fb);
		}
		kfree(sc->buffers);
		sc->buffers = NULL;
	}
	kfree(sc);
}

/*
 * CreateDCSwapChain - allocate a flip-able swap chain of GTT-pinned,
 * SGX-renderable scanout buffers.
 *
 * Mirrors emgd_dc.c CreateDCSwapChain() for the swap-chain/buffer wiring, the
 * per-buffer sync-data assignment (sync_data[i]) and the per-page phys-addr
 * fill, but backs each buffer with a gma500 gma_flipbuf (GTT-pinned scanout)
 * instead of the closed EMGD gmm allocator.  Every buffer is scanout-capable,
 * so a present is a page flip (see gma500dc_process_flip / SwapToDCBuffer).
 */
static PVRSRV_ERROR CreateDCSwapChain(IMG_HANDLE device_h,
	IMG_UINT32 flags,
	DISPLAY_SURF_ATTRIBUTES *dst_surf_attrib,
	DISPLAY_SURF_ATTRIBUTES *src_surf_attrib,
	IMG_UINT32 buffer_count,
	PVRSRV_SYNC_DATA **sync_data,
	IMG_UINT32 oem_flags,
	IMG_HANDLE *swap_chain_h,
	IMG_UINT32 *swap_chain_id)
{
	gma500dc_devinfo_t   *dev = (gma500dc_devinfo_t *) device_h;
	gma500dc_swapchain_t *sc;
	IMG_UINT32 width, height;
	u32 stride = 0;             /* matches gma500_flip_alloc()'s u32* out */
	IMG_UINT32 i;

	(void)oem_flags;

	if (!device_h || !dst_surf_attrib || !src_surf_attrib || !sync_data ||
	    !swap_chain_h || !swap_chain_id || dev != g_devinfo)
		return PVRSRV_ERROR_INVALID_PARAMS;

	/* Flip-able chains need at least double buffering. */
	if (buffer_count < GMA500DC_MIN_SWAP_BUFFERS)
		return PVRSRV_ERROR_TOO_FEW_BUFFERS;
	if (buffer_count > GMA500DC_MAX_SWAP_BUFFERS)
		buffer_count = GMA500DC_MAX_SWAP_BUFFERS;

	/* Source and destination surface geometry must agree. */
	if (dst_surf_attrib->pixelformat != src_surf_attrib->pixelformat ||
	    dst_surf_attrib->sDims.ui32Width != src_surf_attrib->sDims.ui32Width ||
	    dst_surf_attrib->sDims.ui32Height != src_surf_attrib->sDims.ui32Height)
		return PVRSRV_ERROR_INVALID_PARAMS;

	width  = dst_surf_attrib->sDims.ui32Width;
	height = dst_surf_attrib->sDims.ui32Height;
	if (!width || !height)
		return PVRSRV_ERROR_INVALID_PARAMS;

	sc = kzalloc(sizeof(*sc), GFP_KERNEL);
	if (!sc)
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	sc->devinfo      = dev;
	sc->buffer_count = buffer_count;
	sc->flags        = flags;
	sc->valid        = IMG_TRUE;

	sc->buffers = kcalloc(buffer_count, sizeof(*sc->buffers), GFP_KERNEL);
	if (!sc->buffers) {
		kfree(sc);
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	for (i = 0; i < buffer_count; i++) {
		gma500dc_buffer_t *b = &sc->buffers[i];
		int ret, p;

		ret = gma500_flip_alloc(width, height, &stride, &b->fb);
		if (ret) {
			printk(KERN_ERR "[gma500dc] flip_alloc buf %u failed (%d)\n",
			       i, ret);
			free_swap_chain(sc);
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}

		b->is_flip    = IMG_TRUE;
		b->gtt_offset = b->fb.gtt_offset;
		b->num_pages  = (IMG_UINT32) b->fb.npage;
		b->size       = (IMG_UINT32) b->fb.npage * PAGE_SIZE;
		b->virt_addr  = b->fb.cpu;      /* may be NULL (vmap optional) */
		b->sync_data  = sync_data[i];
		b->swap_chain = sc;

		/* Per-page system phys addresses, handed to pvrsrvkm via
		 * GetDCBufferAddr so the SGX MMU maps these same pages into the
		 * GL context => the render target IS the scanout buffer. */
		b->phys_pages = kmalloc_array(b->num_pages,
			sizeof(IMG_SYS_PHYADDR), GFP_KERNEL);
		if (!b->phys_pages) {
			free_swap_chain(sc);
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}
		for (p = 0; p < b->fb.npage; p++)
			b->phys_pages[p].uiAddr =
				(IMG_UINTPTR_T) page_to_phys(b->fb.pages[p]);
	}

	/* Report the stride we actually allocated (ALIGN(width*4,64)) back to
	 * services, mirroring emgd_dc.c which rewrites the surf-attrib stride. */
	dst_surf_attrib->sDims.ui32ByteStride = stride;
	src_surf_attrib->sDims.ui32ByteStride = stride;

	/* Link the chain. */
	sc->next = dev->swap_chains;
	dev->swap_chains = sc;

	*swap_chain_id = ++dev->swap_chain_id_counter;
	*swap_chain_h  = (IMG_HANDLE) sc;

	printk(KERN_INFO "[gma500dc] swap chain %u: %u buffers %ux%u stride=%u\n",
	       *swap_chain_id, buffer_count, width, height, stride);
	return PVRSRV_OK;
}

static PVRSRV_ERROR DestroyDCSwapChain(IMG_HANDLE device_h,
	IMG_HANDLE swap_chain_h)
{
	gma500dc_devinfo_t   *dev = (gma500dc_devinfo_t *) device_h;
	gma500dc_swapchain_t *sc  = (gma500dc_swapchain_t *) swap_chain_h;
	gma500dc_swapchain_t **pp;

	if (!is_valid_swap_chain(dev, sc))
		return PVRSRV_ERROR_INVALID_PARAMS;

	/* Flip scanout back to the system buffer (buffer 0) before tearing the
	 * GTT-pinned buffers down. */
	if (sc->buffer_count && sc->buffers[0].is_flip)
		gma500dc_do_flip(sc->buffers[0].gtt_offset);

	/* Unlink. */
	for (pp = &dev->swap_chains; *pp; pp = &(*pp)->next) {
		if (*pp == sc) {
			*pp = sc->next;
			break;
		}
	}

	free_swap_chain(sc);
	return PVRSRV_OK;
}

static PVRSRV_ERROR SetDCDstRect(IMG_HANDLE device_h,
	IMG_HANDLE swap_chain_h, IMG_RECT *rect)
{
	(void)device_h; (void)swap_chain_h; (void)rect;
	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR SetDCSrcRect(IMG_HANDLE device_h,
	IMG_HANDLE swap_chain_h, IMG_RECT *rect)
{
	(void)device_h; (void)swap_chain_h; (void)rect;
	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR SetDCDstColourKey(IMG_HANDLE device_h,
	IMG_HANDLE swap_chain_h, IMG_UINT32 color)
{
	(void)device_h; (void)swap_chain_h; (void)color;
	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR SetDCSrcColourKey(IMG_HANDLE device_h,
	IMG_HANDLE swap_chain_h, IMG_UINT32 color)
{
	(void)device_h; (void)swap_chain_h; (void)color;
	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/* Return the per-buffer handles for a swap chain (services then calls
 * GetDCBufferAddr on each). Mirrors emgd_dc.c GetDCBuffers(). */
static PVRSRV_ERROR GetDCBuffers(IMG_HANDLE device_h,
	IMG_HANDLE swap_chain_h,
	IMG_UINT32 *buffer_count,
	IMG_HANDLE *buffer_h)
{
	gma500dc_devinfo_t   *dev = (gma500dc_devinfo_t *) device_h;
	gma500dc_swapchain_t *sc  = (gma500dc_swapchain_t *) swap_chain_h;
	IMG_UINT32 i;

	if (!buffer_count || !buffer_h || !is_valid_swap_chain(dev, sc))
		return PVRSRV_ERROR_INVALID_PARAMS;

	*buffer_count = sc->buffer_count;
	for (i = 0; i < sc->buffer_count; i++)
		buffer_h[i] = (IMG_HANDLE) &sc->buffers[i];

	return PVRSRV_OK;
}

/*
 * SwapToDCBuffer - flip scanout to a specific buffer.
 *
 * NOTE: with the command-processor path wired (gma500dc_process_flip),
 * services drives flips via DC_FLIP_COMMAND and does NOT call this in the
 * steady state (same as emgd_dc.c, where it is stubbed).  We still implement a
 * real flip here for completeness / direct callers.
 */
static PVRSRV_ERROR SwapToDCBuffer(IMG_HANDLE device_h,
	IMG_HANDLE buffer_h,
	IMG_UINT32 swap_interval,
	IMG_HANDLE private_tag,
	IMG_UINT32 clip_rect_count,
	IMG_RECT *clip_rect)
{
	gma500dc_devinfo_t *dev = (gma500dc_devinfo_t *) device_h;
	gma500dc_buffer_t  *buf = (gma500dc_buffer_t *) buffer_h;

	(void)swap_interval; (void)private_tag;
	(void)clip_rect_count; (void)clip_rect;

	if (dev != g_devinfo || !buf)
		return PVRSRV_ERROR_INVALID_PARAMS;
	if (!buf->is_flip)
		return PVRSRV_ERROR_INVALID_PARAMS;

	if (gma500dc_do_flip(buf->gtt_offset) != 0)
		return PVRSRV_ERROR_BAD_MAPPING;

	return PVRSRV_OK;
}

/* Flip back to the swap chain's buffer 0 (its "system"/front buffer). */
static PVRSRV_ERROR SwapToDCSystem(IMG_HANDLE device_h,
	IMG_HANDLE swap_chain_h)
{
	gma500dc_devinfo_t   *dev = (gma500dc_devinfo_t *) device_h;
	gma500dc_swapchain_t *sc  = (gma500dc_swapchain_t *) swap_chain_h;

	if (!is_valid_swap_chain(dev, sc))
		return PVRSRV_ERROR_INVALID_PARAMS;
	if (!sc->buffer_count || !sc->buffers[0].is_flip)
		return PVRSRV_ERROR_INVALID_PARAMS;

	if (gma500dc_do_flip(sc->buffers[0].gtt_offset) != 0)
		return PVRSRV_ERROR_BAD_MAPPING;

	return PVRSRV_OK;
}

/*
 * gma500dc_process_flip - DC_FLIP_COMMAND processor.
 *
 * Registered via pfnPVRSRVRegisterCmdProcList (see gma500dc_init).  This is the
 * real present path: PVR services calls it (through the SGX command queue, once
 * the GL render into the back buffer has completed) with a
 * DISPLAYCLASS_FLIP_COMMAND naming the buffer to show.  We page-flip scanout to
 * that buffer, then signal completion so services releases the render and lets
 * the next frame proceed.  Mirrors emgd_dc.c emgddc_process_flip() for the
 * command unpack + pfnPVRSRVCmdComplete handshake, minus the vblank-deferred
 * queue (we flip immediately, i.e. swap-interval 0 semantics).
 */
static IMG_BOOL gma500dc_process_flip(IMG_HANDLE cmd_cookie_h,
	IMG_UINT32 data_size,
	IMG_VOID *data)
{
	DISPLAYCLASS_FLIP_COMMAND *flip_cmd;
	gma500dc_devinfo_t   *dev;
	gma500dc_buffer_t    *buf;
	gma500dc_swapchain_t *sc;

	if (!cmd_cookie_h || !data)
		return IMG_FALSE;
	if (data_size < sizeof(DISPLAYCLASS_FLIP_COMMAND))
		return IMG_FALSE;

	flip_cmd = (DISPLAYCLASS_FLIP_COMMAND *) data;
	dev = (gma500dc_devinfo_t *) flip_cmd->hExtDevice;
	buf = (gma500dc_buffer_t *)  flip_cmd->hExtBuffer;
	sc  = (gma500dc_swapchain_t *) flip_cmd->hExtSwapChain;

	/* Perform the flip when we have a valid GTT-pinned target. */
	if (dev == g_devinfo && is_valid_swap_chain(dev, sc) &&
	    buf && buf->is_flip)
		gma500dc_do_flip(buf->gtt_offset);

	/* Always complete the command so services never stalls (matches the
	 * emgd behaviour even on the error/ignore paths). */
	g_devinfo->pvr_jtable.pfnPVRSRVCmdComplete(cmd_cookie_h, IMG_TRUE);
	return IMG_TRUE;
}

/* -------------------------------------------------------------------------
 * System buffer allocation.
 * ------------------------------------------------------------------------- */

static int gma500dc_alloc_system_buffer(gma500dc_buffer_t *buf)
{
	IMG_UINT32 i;
	unsigned long addr;

	buf->size      = GMA500DC_FB_SIZE;
	buf->num_pages = (GMA500DC_FB_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;

	/* vmalloc: large (4.9MB) buffer, avoids high-order contiguous alloc
	 * failures. Pages are non-contiguous in phys, which matches the
	 * is_contiguous=IMG_FALSE path in GetDCBufferAddr. */
	buf->virt_addr = vmalloc_user(buf->num_pages * PAGE_SIZE);
	if (!buf->virt_addr)
		return -ENOMEM;

	buf->phys_pages = kmalloc_array(buf->num_pages,
		sizeof(IMG_SYS_PHYADDR), GFP_KERNEL);
	if (!buf->phys_pages) {
		vfree(buf->virt_addr);
		buf->virt_addr = NULL;
		return -ENOMEM;
	}

	/* Resolve each vmalloc page to its system physical address. */
	addr = (unsigned long) buf->virt_addr;
	for (i = 0; i < buf->num_pages; i++) {
		struct page *pg = vmalloc_to_page((void *)(addr + i * PAGE_SIZE));

		if (!pg) {
			kfree(buf->phys_pages);
			buf->phys_pages = NULL;
			vfree(buf->virt_addr);
			buf->virt_addr = NULL;
			return -EFAULT;
		}
		/* IMG_SYS_PHYADDR.uiAddr is IMG_UINTPTR_T (32-bit on i686). */
		buf->phys_pages[i].uiAddr = (IMG_UINTPTR_T) page_to_phys(pg);
	}

	return 0;
}

static void gma500dc_free_system_buffer(gma500dc_buffer_t *buf)
{
	if (buf->phys_pages) {
		kfree(buf->phys_pages);
		buf->phys_pages = NULL;
	}
	if (buf->virt_addr) {
		vfree(buf->virt_addr);
		buf->virt_addr = NULL;
	}
}

/* -------------------------------------------------------------------------
 * Module init / exit.
 * ------------------------------------------------------------------------- */

static int __init gma500dc_init(void)
{
	gma500dc_devinfo_t *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;
	g_devinfo = dev;

	/* Allocate the one system buffer. */
	ret = gma500dc_alloc_system_buffer(&dev->system_buffer);
	if (ret) {
		printk(KERN_ERR "[gma500dc] system buffer alloc failed (%d)\n", ret);
		goto err_free_dev;
	}

	/* Static display description. */
	dev->display_format.pixelformat = PVRSRV_PIXEL_FORMAT_ARGB8888;
	dev->display_dim.ui32ByteStride = GMA500DC_STRIDE;
	dev->display_dim.ui32Width      = GMA500DC_WIDTH;
	dev->display_dim.ui32Height     = GMA500DC_HEIGHT;

	memset(&dev->display_info, 0, sizeof(dev->display_info));
	/* DISPLAY_INFO fields per servicesext.h DISPLAY_INFO_TAG. We now support
	 * real flip-able swap chains (double/triple buffered), so advertise it. */
	dev->display_info.ui32MaxSwapChains       = 4;
	dev->display_info.ui32MaxSwapChainBuffers = GMA500DC_MAX_SWAP_BUFFERS;
	dev->display_info.ui32MinSwapInterval     = 0;
	dev->display_info.ui32MaxSwapInterval     = 1;
	dev->display_info.ui32PhysicalWidthmm     = 0;
	dev->display_info.ui32PhysicalHeightmm    = 0;
	strncpy(dev->display_info.szDisplayName, GMA500DC_DISPLAY_NAME,
		MAX_DISPLAY_NAME_SIZE - 1);

	/* 1. Obtain the services DISP2SRV jump table. */
	if (!PVRGetDisplayClassJTable(&dev->pvr_jtable)) {
		printk(KERN_ERR "[gma500dc] PVRGetDisplayClassJTable failed "
			"(is pvrsrvkm loaded?)\n");
		ret = -EAGAIN;
		goto err_free_buf;
	}

	/* 2. Fill our SRV2DISP callback table. Casts mirror emgd_dc.c and are
	 * no-ops here since each function already matches its PFN_* typedef. */
	dev->dc_jtable.ui32TableSize =
		(IMG_UINT32) sizeof(PVRSRV_DC_SRV2DISP_KMJTABLE);
	dev->dc_jtable.pfnOpenDCDevice      = (PFN_OPEN_DC_DEVICE) OpenDCDevice;
	dev->dc_jtable.pfnCloseDCDevice     = (PFN_CLOSE_DC_DEVICE) CloseDCDevice;
	dev->dc_jtable.pfnEnumDCFormats     = (PFN_ENUM_DC_FORMATS) EnumDCFormats;
	dev->dc_jtable.pfnEnumDCDims        = (PFN_ENUM_DC_DIMS) EnumDCDims;
	dev->dc_jtable.pfnGetDCSystemBuffer =
		(PFN_GET_DC_SYSTEMBUFFER) GetDCSystemBuffer;
	dev->dc_jtable.pfnGetDCInfo         = (PFN_GET_DC_INFO) GetDCInfo;
	dev->dc_jtable.pfnGetBufferAddr     = (PFN_GET_BUFFER_ADDR) GetDCBufferAddr;
	dev->dc_jtable.pfnCreateDCSwapChain =
		(PFN_CREATE_DC_SWAPCHAIN) CreateDCSwapChain;
	dev->dc_jtable.pfnDestroyDCSwapChain =
		(PFN_DESTROY_DC_SWAPCHAIN) DestroyDCSwapChain;
	dev->dc_jtable.pfnSetDCDstRect      = (PFN_SET_DC_DSTRECT) SetDCDstRect;
	dev->dc_jtable.pfnSetDCSrcRect      = (PFN_SET_DC_SRCRECT) SetDCSrcRect;
	dev->dc_jtable.pfnSetDCDstColourKey = (PFN_SET_DC_DSTCK) SetDCDstColourKey;
	dev->dc_jtable.pfnSetDCSrcColourKey = (PFN_SET_DC_SRCCK) SetDCSrcColourKey;
	dev->dc_jtable.pfnGetDCBuffers      = (PFN_GET_DC_BUFFERS) GetDCBuffers;
	dev->dc_jtable.pfnSwapToDCBuffer    = (PFN_SWAP_TO_DC_BUFFER) SwapToDCBuffer;
	dev->dc_jtable.pfnSwapToDCSystem    = (PFN_SWAP_TO_DC_SYSTEM) SwapToDCSystem;
	/* pfnSetDCState / pfnQuerySwapCommandID left NULL: optional, services
	 * null-checks them. */

	/* 3. Register with services. */
	if (dev->pvr_jtable.pfnPVRSRVRegisterDCDevice(&dev->dc_jtable,
			&dev->device_id) != PVRSRV_OK) {
		printk(KERN_ERR "[gma500dc] pfnPVRSRVRegisterDCDevice failed\n");
		ret = -EIO;
		goto err_free_buf;
	}

	dev->registered = IMG_TRUE;

	/* 4. Register the DC_FLIP_COMMAND processor so PVR services drives page
	 * flips through gma500dc_process_flip() (the real present path).  Sync
	 * counts mirror emgd_dc.c: [0]=0 src, [1]=2 dst. */
	{
		PFN_CMD_PROC cmd_proc_list[GMA500DC_COMMAND_COUNT];
		IMG_UINT32   sync_count_list[GMA500DC_COMMAND_COUNT][2];

		cmd_proc_list[DC_FLIP_COMMAND]      = gma500dc_process_flip;
		sync_count_list[DC_FLIP_COMMAND][0] = 0;
		sync_count_list[DC_FLIP_COMMAND][1] = 2;

		if (dev->pvr_jtable.pfnPVRSRVRegisterCmdProcList(dev->device_id,
				&cmd_proc_list[0], sync_count_list,
				GMA500DC_COMMAND_COUNT) != PVRSRV_OK) {
			printk(KERN_ERR "[gma500dc] RegisterCmdProcList failed\n");
			ret = -EIO;
			goto err_unregister_dc;
		}
		dev->cmd_proc_registered = IMG_TRUE;
	}

	printk(KERN_INFO "[gma500dc] registered DC device: id=%u, "
		"fb=%ux%u stride=%u size=%u (%u pages), flip=on\n",
		dev->device_id, GMA500DC_WIDTH, GMA500DC_HEIGHT,
		GMA500DC_STRIDE, dev->system_buffer.size,
		dev->system_buffer.num_pages);
	return 0;

err_unregister_dc:
	dev->pvr_jtable.pfnPVRSRVRemoveDCDevice(dev->device_id);
	dev->registered = IMG_FALSE;
err_free_buf:
	gma500dc_free_system_buffer(&dev->system_buffer);
err_free_dev:
	kfree(dev);
	g_devinfo = NULL;
	return ret;
}

static void __exit gma500dc_exit(void)
{
	gma500dc_devinfo_t *dev = g_devinfo;

	if (!dev)
		return;

	/* Tear down any swap chains that outlived their clients. */
	while (dev->swap_chains) {
		gma500dc_swapchain_t *sc = dev->swap_chains;

		dev->swap_chains = sc->next;
		free_swap_chain(sc);
	}

	if (dev->cmd_proc_registered) {
		PVRSRV_ERROR e =
			dev->pvr_jtable.pfnPVRSRVRemoveCmdProcList(dev->device_id,
				GMA500DC_COMMAND_COUNT);
		if (e != PVRSRV_OK)
			printk(KERN_ERR "[gma500dc] RemoveCmdProcList(%u) "
				"returned %d\n", dev->device_id, e);
	}

	if (dev->registered) {
		PVRSRV_ERROR e =
			dev->pvr_jtable.pfnPVRSRVRemoveDCDevice(dev->device_id);
		if (e != PVRSRV_OK)
			printk(KERN_ERR "[gma500dc] pfnPVRSRVRemoveDCDevice(%u) "
				"returned %d\n", dev->device_id, e);
	}

	gma500dc_free_system_buffer(&dev->system_buffer);
	kfree(dev);
	g_devinfo = NULL;
	printk(KERN_INFO "[gma500dc] unregistered\n");
}

module_init(gma500dc_init);
module_exit(gma500dc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Poulsbo bring-up");
MODULE_DESCRIPTION("Minimal PowerVR SGX display-class device for GMA500 (PVR2D enumerate/context/memalloc)");
MODULE_VERSION("0.1");
