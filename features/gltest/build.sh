#!/bin/sh
# build.sh - compile the glburn GLES2 test suite ON THE TARGET (i686), against
# the EMGD GLES2/EGL headers + libs.  Run from this directory.
#
#   ./build.sh                 # -> ./glburn
#   LD_LIBRARY_PATH=/opt/emgd/lib ./glburn all 5
#
# Headers: uses ./emgd-headers if present (copied from the EMGD SDK), else the
# system GLES2/EGL headers.  Links against the EMGD libEGL/libGLESv2 in
# /opt/emgd/lib.
set -e
INC=""
[ -d emgd-headers ] && INC="-Iemgd-headers"
cc -O2 -o glburn glburn.c $INC -L/opt/emgd/lib -Wl,-rpath-link,/opt/emgd/lib \
   -lEGL -lGLESv2 -lm
echo "built ./glburn  (run: LD_LIBRARY_PATH=/opt/emgd/lib ./glburn all 5)"
