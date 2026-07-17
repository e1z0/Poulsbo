# Makefile for gma500dc.ko
#
# Build the SAME way as the pvrsrvkm port: i386/i686 kernel-headers build,
# typically inside the i386 Docker on the Vaio host, then scp gma500dc.ko to
# the target.
#
# Usage:
#   make                 # build against /lib/modules/$(uname -r)/build
#   make KDIR=/path/to/linux-5.4-i686/build
#   make PVRSRV_SRC=/path/to/pvrsrvkm-port/src
#   make clean
#
# KBUILD_EXTRA_SYMBOLS points at the pvrsrvkm port Module.symvers so the
# module resolves PVRGetDisplayClassJTable at link/modpost time (no undefined
# symbol warning, correct CRC).

KDIR        ?= /lib/modules/$(shell uname -r)/build
PVRSRV_SRC  ?= /tmp/claude-1000/-Projects-Poulsbo/476a621f-66ff-423a-96f0-a1dca3ae4adf/scratchpad/pvrsrvkm-port/src

# gma500_gfx build dir: supplies the flip-helper exports (gma500_flip_alloc,
# gma500_flip_free, gma500_flip_to, gma500_get_drm) and their Module.symvers.
GMA500_SRC  ?= /tmp/claude-1000/-Projects-Poulsbo/476a621f-66ff-423a-96f0-a1dca3ae4adf/scratchpad/gma500-3d-build

# ARCH/CROSS_COMPILE default to a native i686 toolchain (as used in the i386
# Docker). Override if cross-building from x86_64.
ARCH        ?= i386

# This module now depends on BOTH pvrsrvkm AND gma500_gfx, so both
# Module.symvers must be visible to modpost (space-separated list).
export KBUILD_EXTRA_SYMBOLS := $(PVRSRV_SRC)/Module.symvers $(GMA500_SRC)/Module.symvers

PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) PVRSRV_SRC=$(PVRSRV_SRC) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f Module.symvers modules.order

.PHONY: all clean
