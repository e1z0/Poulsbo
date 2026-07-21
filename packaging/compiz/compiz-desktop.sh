#!/bin/sh
# compiz-desktop.sh - launch the GL-composited MATE desktop with WOBBLY windows
# on GMA500/SGX535.  This is the exact recipe confirmed working on the Vaio
# (2026-07-21): "works without any flickering, no artefacts, wobbly animations".
#
# Prereqs (deployed once): the poulsbo-sgx-glx stack, then:
#   - emgd_dri.so   : format-match patch (glx-enable.sh) -- un-hangs compiz
#   - gma500blt_drv.so : the TFP-fix DDX (texture_from_pixmap returns the redirect
#                        pixmap's own PVR2D export) -- composited windows show content
#   - libcomposite.so : damage-promotion patch (compiz-enable.sh) -- full repaint,
#                       no flicker / black trails
#   - libopengl.so  : MUST stay ORIGINAL/unpatched (blend patches break it)
#
# Plugin set is deliberate: NO 'decor' (its shadow renders opaque black = a black
# rectangle) and NO framebuffer-object/bench (they panic-reboot the SGX).
set -e
export DISPLAY="${DISPLAY:-:0}"
export MESA_LOADER_DRIVER_OVERRIDE=emgd

WOBBLY=1; [ "$1" = "nowobbly" ] && WOBBLY=0
PLUGINS='"core","composite","opengl","move","resize","place"'
[ "$WOBBLY" = 1 ] && PLUGINS="$PLUGINS,\"wobbly\""

# compizconfig must use the gsettings backend for `compiz ccp` to load plugins/opts
mkdir -p "$HOME/.config/compiz-1/compizconfig"
cat > "$HOME/.config/compiz-1/compizconfig/config" <<CFG
[general]
backend = gsettings
profile = Default
integration = false
CFG
CORE=org.compiz.core:/org/compiz/profiles/Default/plugins/core/
gsettings set "$CORE" active-plugins "[$PLUGINS]" 2>/dev/null || true

chvt 7 2>/dev/null || true
# replace the running WM (marco) with compiz reading the gsettings config
compiz --replace ccp >/tmp/compiz.log 2>&1 &
sleep 8
# draw the MATE desktop (wallpaper + icons) as a real window compiz can texture
caja --force-desktop --no-default-window >/dev/null 2>&1 &

echo "compiz-desktop: launched (wobbly=$WOBBLY). Log: /tmp/compiz.log"
echo "  Alt+left-drag a window to wobble it. Revert:  marco --replace &"
