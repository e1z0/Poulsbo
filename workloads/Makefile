CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c11
PKG_CONFIG ?= pkg-config

PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags x11 egl glesv2 2>/dev/null)
PKG_LIBS := $(shell $(PKG_CONFIG) --libs x11 egl glesv2 2>/dev/null)
PROBE_CFLAGS := $(shell $(PKG_CONFIG) --cflags x11 egl 2>/dev/null)
PROBE_LIBS := $(shell $(PKG_CONFIG) --libs x11 egl 2>/dev/null)
DRI2_CFLAGS := $(shell $(PKG_CONFIG) --cflags xcb xcb-dri2 libdrm 2>/dev/null)
DRI2_LIBS := $(shell $(PKG_CONFIG) --libs xcb xcb-dri2 libdrm 2>/dev/null)
DRM_CFLAGS := $(shell $(PKG_CONFIG) --cflags libdrm 2>/dev/null)
DRM_LIBS := $(shell $(PKG_CONFIG) --libs libdrm 2>/dev/null)
LAB_DRM_CFLAGS := -I../third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500 $(DRM_CFLAGS)

all: gles2_clear_x11 gles2_triangle_x11 x11_egl_probe x11_dri2_probe drm_probe psb_sgx_lab_query psb_sgx_lab_range_query psb_sgx_lab_sgx_state_query psb_sgx_lab_gtt_state_query psb_sgx_lab_object_query psb_sgx_lab_submit_op psb_sgx_lab_object_op psb_sgx_lab_dump

gles2_clear_x11: gles2_clear_x11.c
	$(CC) $(CFLAGS) $(PKG_CFLAGS) -o $@ $< $(PKG_LIBS)

gles2_triangle_x11: gles2_triangle_x11.c
	$(CC) $(CFLAGS) $(PKG_CFLAGS) -o $@ $< $(PKG_LIBS)

x11_egl_probe: x11_egl_probe.c
	$(CC) $(CFLAGS) $(PROBE_CFLAGS) -o $@ $< $(PROBE_LIBS) -ldl

x11_dri2_probe: x11_dri2_probe.c
	$(CC) $(CFLAGS) $(DRI2_CFLAGS) -o $@ $< $(DRI2_LIBS)

drm_probe: drm_probe.c
	$(CC) $(CFLAGS) $(DRM_CFLAGS) -o $@ $< $(DRM_LIBS)

psb_sgx_lab_query: psb_sgx_lab_query.c
	$(CC) $(CFLAGS) $(LAB_DRM_CFLAGS) -o $@ $< $(DRM_LIBS)

psb_sgx_lab_range_query: psb_sgx_lab_range_query.c
	$(CC) $(CFLAGS) $(LAB_DRM_CFLAGS) -o $@ $< $(DRM_LIBS)

psb_sgx_lab_sgx_state_query: psb_sgx_lab_sgx_state_query.c
	$(CC) $(CFLAGS) $(LAB_DRM_CFLAGS) -o $@ $< $(DRM_LIBS)

psb_sgx_lab_gtt_state_query: psb_sgx_lab_gtt_state_query.c
	$(CC) $(CFLAGS) $(LAB_DRM_CFLAGS) -o $@ $< $(DRM_LIBS)

psb_sgx_lab_object_query: psb_sgx_lab_object_query.c
	$(CC) $(CFLAGS) $(LAB_DRM_CFLAGS) -o $@ $< $(DRM_LIBS)

psb_sgx_lab_submit_op: psb_sgx_lab_submit_op.c
	$(CC) $(CFLAGS) $(LAB_DRM_CFLAGS) -o $@ $< $(DRM_LIBS)

psb_sgx_lab_object_op: psb_sgx_lab_object_op.c
	$(CC) $(CFLAGS) $(LAB_DRM_CFLAGS) -o $@ $< $(DRM_LIBS)

psb_sgx_lab_dump: psb_sgx_lab_dump.c
	$(CC) $(CFLAGS) $(LAB_DRM_CFLAGS) -o $@ $< $(DRM_LIBS)

clean:
	rm -f gles2_clear_x11 gles2_triangle_x11 x11_egl_probe x11_dri2_probe drm_probe psb_sgx_lab_query psb_sgx_lab_range_query psb_sgx_lab_sgx_state_query psb_sgx_lab_gtt_state_query psb_sgx_lab_object_query psb_sgx_lab_submit_op psb_sgx_lab_object_op psb_sgx_lab_dump

.PHONY: all clean
