#!/bin/bash
# Build an out-of-tree kernel module for the Vaio (5.4.0-150-generic i686) in the
# i386 container. Usage: build.sh <module-src-dir-on-host>
# Produces <module-src-dir>/*.ko built against the mirrored Vaio headers.
set -e
MODSRC="${1:?usage: build.sh <module-src-dir>}"
KBUILD=/tmp/claude-1000/-Projects-Poulsbo/476a621f-66ff-423a-96f0-a1dca3ae4adf/scratchpad/kbuild
KDIR=/usr/src/linux-headers-5.4.0-150-generic

sudo docker run --rm \
  -v "$KBUILD/usr/src:/usr/src:ro" \
  -v "$MODSRC:/work" \
  poulsbo-kbuild:18.04 \
  bash -c "make -C $KDIR M=/work ARCH=x86 modules && \
           echo '--- vermagic ---' && modinfo /work/*.ko | grep -E 'vermagic|filename' "
