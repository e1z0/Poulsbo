#!/bin/sh
# compiz-desktop.sh - launch the GL-composited MATE desktop with WOBBLY windows
# and window TITLEBARS on GMA500/SGX535.  Recipe confirmed working on the Vaio
# (2026-07-21): wobbly drags, gray Greybird titlebars, no black artefacts.
#
# Prereqs (deployed once): the poulsbo-sgx-glx stack, then:
#   - emgd_dri.so   : format-match patch (glx-enable.sh) -- un-hangs compiz
#   - gma500blt_drv.so : the TFP-fix DDX (texture_from_pixmap returns the redirect
#                        pixmap's own PVR2D export) -- composited windows show content
#   - libcomposite.so : damage-promotion patch (compiz-enable.sh) -- full repaint,
#                       no flicker / black trails
#   - libopengl.so  : MUST stay ORIGINAL/unpatched (blend patches break it)
#
# *** COMPOSITING RUNS ON SOFTWARE GL (llvmpipe), ON PURPOSE. ***
# On real SGX hardware GL, compiz's SGX 3D compositing fights the DDX's HW on-screen
# 2D over the shared BIF (see memory: 2d-sgx-bif-conflict) -> black artefacts.  Forcing
# LIBGL_ALWAYS_SOFTWARE=1 means no SGX 3D, so no BIF contention -> clean.  Apps still
# get HW 2D accel from the DDX; only the wobble compositing is CPU.  A fully-hardware
# composited desktop needs the DDX 2D/3D conflict fixed first (disable the scanout
# on-screen-2D wrap so SGX GL owns the screen).  To try hardware anyway: pass 'hw'.
#
# Plugin set is deliberate: NO framebuffer-object/bench (they panic-reboot the SGX).
# 'decor' (titlebars) works ONLY in a clean state (good DDX + a single compiz); the
# earlier "No default decoration found" throw was a symptom of a corrupted DDX /
# dueling compiz instances, not a real limitation.  The decor plugin's `command` does
# NOT reliably auto-spawn the decorator, so we launch gtk-window-decorator explicitly.
set -e
export DISPLAY="${DISPLAY:-:0}"

WOBBLY=1;  [ "$1" = "nowobbly" ] && WOBBLY=0
DECOR=1;   [ "$1" = "nodecor"  ] && DECOR=0
HWGL=0;    [ "$1" = "hw"       ] && HWGL=1

# Software GL compositing = the artefact-free default; 'hw' opts into SGX 3D (artefacts).
if [ "$HWGL" = 1 ]; then
	export MESA_LOADER_DRIVER_OVERRIDE=emgd
else
	unset MESA_LOADER_DRIVER_OVERRIDE
	export LIBGL_ALWAYS_SOFTWARE=1
fi

PLUGINS='"core","composite","opengl"'
[ "$DECOR"  = 1 ] && PLUGINS="$PLUGINS,\"decor\""
PLUGINS="$PLUGINS,\"move\",\"resize\",\"place\""
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

# --- tuning (all confirmed on the Vaio) ---
OG=org.compiz.opengl:/org/compiz/profiles/Default/plugins/opengl/
CM=org.compiz.composite:/org/compiz/profiles/Default/plugins/composite/
WB=org.compiz.wobbly:/org/compiz/profiles/Default/plugins/wobbly/
# THE slow-motion fix: compiz times animations by 1/refresh-rate per frame; the
# desktop composites at ~30fps, so the stock refresh-rate=60 plays every animation
# at HALF speed (slow-motion). Pin it to the real rate.
gsettings set "$CM" detect-refresh-rate false 2>/dev/null || true
gsettings set "$CM" refresh-rate 30 2>/dev/null || true
gsettings set "$OG" sync-to-vblank false 2>/dev/null || true   # no vsync stalls
gsettings set "$OG" texture-filter 0 2>/dev/null || true       # nearest = cheaper
gsettings set "$OG" lighting false 2>/dev/null || true
gsettings set "$OG" texture-compression false 2>/dev/null || true
gsettings set "$WB" friction 10.0 2>/dev/null || true          # snappy wobble
gsettings set "$WB" spring-k 10.0 2>/dev/null || true

# --- decor: titlebars via gtk-window-decorator, NO shadow (opaque-black artefact) ---
DC=org.compiz.decor:/org/compiz/profiles/Default/plugins/decor/
gsettings set "$DC" command 'gtk-window-decorator --replace' 2>/dev/null || true
gsettings set "$DC" active-shadow-opacity 0.0 2>/dev/null || true
gsettings set "$DC" inactive-shadow-opacity 0.0 2>/dev/null || true

chvt 7 2>/dev/null || true
# replace the running WM (marco) with compiz reading the gsettings config
pkill -f gtk-window-decorator 2>/dev/null || true
compiz --replace ccp >/tmp/compiz.log 2>&1 &
sleep 8
# the decor plugin's own `command` is unreliable; spawn the decorator ourselves
if [ "$DECOR" = 1 ]; then
	setsid gtk-window-decorator --replace >/tmp/gwd.log 2>&1 &
fi
# draw the MATE desktop (wallpaper + icons) as a real window compiz can texture
caja --force-desktop --no-default-window >/dev/null 2>&1 &

echo "compiz-desktop: launched (wobbly=$WOBBLY decor=$DECOR hwgl=$HWGL). Log: /tmp/compiz.log"
echo "  Alt+left-drag a window to wobble it. Revert:  marco --replace &"
