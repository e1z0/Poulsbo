#!/bin/sh
# compiz-desktop.sh - launch the GL-composited MATE desktop on GMA500/SGX535.
#
# Prereqs (run once): glx-enable.sh (emgd GLX/DRI2 + format-match un-hang +
# texture_from_pixmap fix) and compiz-enable.sh (libcomposite full-damage patch
# that stops idle windows going black).  Both are in packaging/.
#
# Usage:  compiz-desktop.sh          # clean composited desktop (recommended)
#         compiz-desktop.sh wobbly   # + wobbly windows (physics work; the
#                                     # deformed mesh renders with wrong colours
#                                     # DURING the wobble on this closed GL --
#                                     # settled windows are correct)
#
# State of play on this hardware (SGX535 + closed emgd GL):
#  * WORKS: GL compositing, correct colours, smooth window movement, no black.
#  * wobbly: the spring deformation is correct, but emgd renders the subdivided
#    textured mesh with wrong colours (white/magenta) mid-wobble. Fixable only
#    via compiz options (the ccp config plugin destabilises this box) or by
#    reverse-engineering emgd's GL geometry path. Off by default for that reason.
#  * Do NOT enable: framebuffer-object / bench / continuous full-repaint -- they
#    panic-reboot the SGX. The decor plugin's shadow renders opaque black; left
#    out here (windows are borderless; Alt+drag to move).
set -e
export DISPLAY="${DISPLAY:-:0}"
export MESA_LOADER_DRIVER_OVERRIDE=emgd

PLUGINS="core composite opengl move resize place"
[ "$1" = "wobbly" ] && PLUGINS="$PLUGINS wobbly"

# bring the display to X's VT (X runs on vt7 under the autologin getty)
chvt 7 2>/dev/null || true

# replace the running WM (marco) with compiz + the chosen plugins
compiz --replace $PLUGINS >/tmp/compiz.log 2>&1 &
sleep 6
# draw the MATE desktop (wallpaper + icons) as a real window compiz can texture
caja --force-desktop --no-default-window >/dev/null 2>&1 &

echo "compiz-desktop: launched ($PLUGINS). Log: /tmp/compiz.log"
echo "  revert to the stable non-composited desktop with:  marco --replace &"
