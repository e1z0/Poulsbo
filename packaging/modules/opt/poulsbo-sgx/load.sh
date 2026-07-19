#!/bin/bash
# load.sh - bring up the Poulsbo SGX driver stack in the required order.
#
# The stock in-kernel gma500_gfx is blacklisted (see
# /etc/modprobe.d/poulsbo-sgx.conf) so it never grabs the device first.  We then
# insmod our three modules BY PATH, in order:
#     pvrsrvkm  (SGX render core; must be first so gma500 can resolve its
#                exported bridge + interrupt-forward symbols)
#     gma500_gfx (display / KMS + the PVR-DRM bridge + SGX IRQ forwarder)
#     gma500dc  (display-class device that backs the zero-copy flip chain)
# Bringing the SGX microkernel to INIT_COMPLETE (for GL) is done separately by
# poulsbo-sgx-srvinit.service, which runs later and cannot hang early boot.
# 2D X11 acceleration (the gma500blt EXA DDX) does not need srvinit at all.
#
# Tunable module parameters live in /etc/default/poulsbo-sgx.
set -u
DIR=/opt/poulsbo-sgx
MARK=$DIR/.loading

# Boot-loop guard: if a previous load did not finish, skip (a half-loaded
# stack can hang the box; better to come up unaccelerated and let the user
# investigate than to wedge every boot).
if [ -f "$MARK" ]; then
    logger -t poulsbo-sgx "previous load did not complete; skipping this boot"
    exit 0
fi
touch "$MARK"; sync

# Defaults (overridable via /etc/default/poulsbo-sgx).
GMA_NO_SGX=1
PVR_IRQ_FORWARD=1
FLIP_VSYNC=0
FLIP_ENABLE=1
[ -r /etc/default/poulsbo-sgx ] && . /etc/default/poulsbo-sgx

# Dependencies the modules need present.
for m in drm drm_kms_helper i2c_algo_bit video; do modprobe "$m" 2>/dev/null; done

insmod "$DIR/pvrsrvkm.ko";                                              prc=$?
insmod "$DIR/gma500_gfx.ko" gma_no_sgx="$GMA_NO_SGX" \
       pvr_irq_forward="$PVR_IRQ_FORWARD" flip_vsync="$FLIP_VSYNC";     grc=$?
insmod "$DIR/gma500dc.ko" flip_enable="$FLIP_ENABLE";                   drc=$?

# proc_dri_compat: recreates the legacy /proc/dri/0/name interface (removed from
# modern kernels) that the closed emgd_dri.so requires to initialise DRI2/GLX.
# Proc-only, so it is safe to insmod live and cannot wedge boot. Needed only for
# HW OpenGL/GLX under X; 2D EXA does not use it.
[ -f "$DIR/proc_dri_compat.ko" ] && insmod "$DIR/proc_dri_compat.ko";   pdc=$?

# NOTE: srvinit (SGX microkernel INIT_COMPLETE, needed only for GL) is run
# later by poulsbo-sgx-srvinit.service so it can never wedge early boot.

rm -f "$MARK"; sync
logger -t poulsbo-sgx "pvrsrvkm=$prc gma500_gfx=$grc gma500dc=$drc proc_dri=${pdc:-skip}"
exit 0
