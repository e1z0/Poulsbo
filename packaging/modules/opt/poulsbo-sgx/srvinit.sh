#!/bin/bash
# srvinit.sh - bring the SGX535 microkernel to INIT_COMPLETE so OpenGL ES apps
# work.  Run by poulsbo-sgx-srvinit.service AFTER the modules are loaded and
# late enough that it can never wedge early boot.  Entirely optional: 2D X11
# acceleration (the gma500blt EXA DDX) does not need it.
set -u
[ -r /etc/default/poulsbo-sgx ] && . /etc/default/poulsbo-sgx

if [ "${RUN_SRVINIT:-1}" != "1" ]; then
    logger -t poulsbo-sgx "srvinit disabled via /etc/default/poulsbo-sgx"; exit 0
fi
if ! grep -q '^pvrsrvkm' /proc/modules 2>/dev/null; then
    logger -t poulsbo-sgx "pvrsrvkm not loaded; skipping srvinit"; exit 0
fi
if [ ! -x /opt/poulsbo-sgx/srvinit ] || [ ! -d /opt/emgd/lib ]; then
    logger -t poulsbo-sgx "srvinit or /opt/emgd/lib missing; skipping"; exit 0
fi

LD_LIBRARY_PATH=/opt/emgd/lib timeout 30 /opt/poulsbo-sgx/srvinit >/dev/null 2>&1
logger -t poulsbo-sgx "srvinit rc=$?"
exit 0
