# Build psb_blt_test natively on the i686 target (no -m32 needed).
# Mirrors the /Projects/Poulsbo/workloads Makefile conventions:
# pkg-config libdrm supplies header include paths only; the binary
# links against nothing beyond libc (raw ioctl(2)).
CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c11
# 64-bit off_t on the i686 target: DRM mmap fake offsets can exceed
# 2^31 once cumulative BO allocations grow; a 32-bit off_t would
# silently truncate them (glibc routes mmap through mmap2 with this).
CFLAGS += -D_FILE_OFFSET_BITS=64
PKG_CONFIG ?= pkg-config

DRM_CFLAGS := $(shell $(PKG_CONFIG) --cflags libdrm 2>/dev/null)

all: psb_blt_test

psb_blt_test: psb_blt_test.c gma_blt_drm.h
	$(CC) $(CFLAGS) $(DRM_CFLAGS) -o $@ $<

clean:
	rm -f psb_blt_test

.PHONY: all clean
