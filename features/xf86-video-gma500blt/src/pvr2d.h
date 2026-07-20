/*
 * pvr2d.h - PVR2D v1 ABI (SGX DDK 1.5.15.3226) used across the gma500blt DDX.
 *
 * The DDX is a PVR2D client: it allocates SGX-mapped buffers (PVR2DMemAlloc)
 * and blits between them with PVR2DBlt -- the SGX microkernel's own 2D path,
 * which coexists with the GL/DRI2 stack (unlike the direct-MMIO 2D-BLT engine,
 * which the shared BIF wedges once SGX is up; see the 2d-sgx-bif-conflict
 * notes).  Shared here so the pixmap allocator, EXA Solid/Copy and DRI2 present
 * all speak the same ABI.  Matches features/emgd-wsegl/wsegl_win.c.
 */
#ifndef GMA500BLT_PVR2D_H
#define GMA500BLT_PVR2D_H

typedef void *PVR2DCONTEXTHANDLE;
typedef void *PVR2DHANDLE;
typedef long  PVR2DERROR;
#define PVR2D_OK 0
#define PVR2D_MAX_DEVICE_NAME 20
#define PVR2D_MEM_WRITECOMBINE 0x00000004UL

typedef struct { unsigned long ulDevID; char szDeviceName[PVR2D_MAX_DEVICE_NAME]; } PVR2DDEVICEINFO;

typedef struct {
	void         *pBase;		/* CPU virtual address                 */
	unsigned long ui32MemSize;
	unsigned long ui32DevAddr;	/* SGX device virtual address          */
	unsigned long ulFlags;
	void         *hPrivateData;
	void         *hPrivateMapData;
} PVR2DMEMINFO;

/* PVR2DBLTINFO (SGX DDK 1.x).  A solid fill sets pSrcMemInfo=NULL with a
 * pattern-copy ROP (0xF0) + Colour; a copy sets SRCCOPY (0xCC) + pSrcMemInfo. */
#define PVR2D_ARGB8888   4
#define PVR2DROPcopy     0xCC		/* ROP3 SRCCOPY   */
#define PVR2DROPclear    0x00
#define PVR2DPATROPcopy  0xF0		/* ROP3 PATCOPY (solid fill) */
typedef struct {
	unsigned long  CopyCode;
	unsigned long  Colour;
	unsigned long  ColourKey;
	unsigned char  GlobalAlphaValue;
	unsigned char  AlphaBlendingFunc;
	unsigned long  BlitFlags;
	PVR2DMEMINFO  *pDstMemInfo;
	unsigned long  DstOffset;
	long           DstStride;
	unsigned long  DstFormat;
	long           DstSurfWidth, DstSurfHeight;
	long           DstX, DstY, DSizeX, DSizeY;
	PVR2DMEMINFO  *pSrcMemInfo;
	unsigned long  SrcOffset;
	long           SrcStride;
	unsigned long  SrcFormat;
	long           SrcSurfWidth, SrcSurfHeight;
	long           SrcX, SrcY, SizeX, SizeY;
	PVR2DMEMINFO  *pMaskMemInfo;
	unsigned long  MaskOffset;
	long           MaskStride;
	long           MaskSurfWidth, MaskSurfHeight;
	long           MaskX, MaskY;
} PVR2DBLTINFO;

extern int        PVR2DEnumerateDevices(PVR2DDEVICEINFO *);
extern PVR2DERROR PVR2DCreateDeviceContext(unsigned long, PVR2DCONTEXTHANDLE *, unsigned long);
extern PVR2DERROR PVR2DMemAlloc(PVR2DCONTEXTHANDLE, unsigned long, unsigned long, unsigned long, PVR2DMEMINFO **);
extern PVR2DERROR PVR2DMemFree(PVR2DCONTEXTHANDLE, PVR2DMEMINFO *);
extern PVR2DERROR PVR2DMemExport(PVR2DCONTEXTHANDLE, unsigned long, PVR2DMEMINFO *, PVR2DHANDLE *);
extern PVR2DERROR PVR2DBlt(PVR2DCONTEXTHANDLE, PVR2DBLTINFO *);
extern PVR2DERROR PVR2DQueryBlitsComplete(PVR2DCONTEXTHANDLE, PVR2DMEMINFO *, unsigned int);

/* Lazily-created shared PVR2D device context for this screen (gma500blt_dri2.c).
 * Returns NULL if PVR services are not up (srvinit). */
PVR2DCONTEXTHANDLE gma500blt_pvr2d_ctx(ScrnInfoPtr scrn);

#endif /* GMA500BLT_PVR2D_H */
