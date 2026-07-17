#!/bin/bash
# build-modules.sh - build the three Poulsbo SGX kernel modules from the
# consolidated sources in ../features, using the i386 Ubuntu-18.04 kbuild
# container, and drop the .ko (+ srvinit if present) where build-debs.sh
# expects them.
#
# PREREQUISITES (one-time):
#   1. A Docker image with an i386 GCC toolchain + the target kernel headers:
#          docker build -t poulsbo-kbuild:18.04 ../docker/kbuild
#   2. A directory tree holding the 5.4.0-150-generic (i686) kernel headers /
#      build symlink at $KDIR (the container mounts it read-only).  On the
#      target you can copy /usr/src/linux-headers-$(uname -r) and its
#      linux-headers-$(uname -r)-generic peer; point KBUILD_SRC at the parent.
#
# The build order matters: pvrsrvkm first (it exports the bridge +
# gma_pvr_sgx_isr symbols), then gma500_gfx against pvrsrvkm's Module.symvers,
# then gma500dc against the two combined symvers.
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
FEAT="$HERE/../features"
IMAGE="${IMAGE:-poulsbo-kbuild:18.04}"
KERNEL="${KERNEL:-5.4.0-150-generic}"
KDIR="${KDIR:-/usr/src/linux-headers-$KERNEL}"
KBUILD_SRC="${KBUILD_SRC:?set KBUILD_SRC to the dir that contains usr/src/linux-headers-$KERNEL}"
OUT="${OUT:-$HERE/artifacts}"
mkdir -p "$OUT"

dock() { # dock <module-src-dir> <extra 'make' args...>
    local src="$1"; shift
    sudo docker run --rm \
        -v "$KBUILD_SRC/usr/src:/usr/src:ro" \
        -v "$src:/work" \
        "$IMAGE" \
        bash -c "make -C $KDIR M=/work ARCH=x86 $* modules"
}

echo ">> [1/3] pvrsrvkm"
dock "$FEAT/pvrsrvkm-port/src"
cp "$FEAT/pvrsrvkm-port/src/pvrsrvkm.ko" "$OUT/"

echo ">> [2/3] gma500_gfx (against pvrsrvkm symbols)"
cp "$FEAT/pvrsrvkm-port/src/Module.symvers" "$FEAT/gma500-sgx/pvrsrvkm.symvers"
dock "$FEAT/gma500-sgx" "KBUILD_EXTRA_SYMBOLS=/work/pvrsrvkm.symvers"
cp "$FEAT/gma500-sgx/gma500_gfx.ko" "$OUT/"

echo ">> [3/3] gma500dc (against pvrsrvkm + gma500 symbols)"
cat "$FEAT/pvrsrvkm-port/src/Module.symvers" "$FEAT/gma500-sgx/Module.symvers" \
    > "$FEAT/gma500dc/combined.symvers"
# gma500dc's Kbuild pulls the pvrsrvkm include tree; see features/gma500dc.
dock "$FEAT/gma500dc" "KBUILD_EXTRA_SYMBOLS=/work/combined.symvers"
cp "$FEAT/gma500dc/gma500dc.ko" "$OUT/"

echo
echo ">> modules built into $OUT :"
ls -l "$OUT"/*.ko
echo
echo "Next: put srvinit, emgd-lib.tgz and gma500blt_drv.so alongside these in"
echo "$OUT, then run:  ARTIFACTS=$OUT ./build-debs.sh"
