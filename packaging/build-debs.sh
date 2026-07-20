#!/bin/bash
# build-debs.sh - assemble and build the Poulsbo SGX .deb packages.
#
# Produces three packages under packaging/out/:
#   poulsbo-sgx-modules_<ver>_i386.deb    kernel modules + boot loader service
#   poulsbo-sgx-userspace_<ver>_i386.deb  EMGD EGL/GLES runtime + WSEGL + EXA DDX
#   poulsbo-sgx_<ver>_all.deb             metapackage pulling in both
#
# It stages prebuilt binary artifacts (which are NOT committed to git) into the
# package trees, then runs dpkg-deb.  Point ARTIFACTS at a directory holding:
#
#   pvrsrvkm.ko        built from features/pvrsrvkm-port  (i386 Docker, 5.4 hdrs)
#   gma500_gfx.ko      built from the gma500 source w/ the bridge+flip+params
#   gma500dc.ko        built from features/gma500dc
#   srvinit            the SGX services init helper (i386 ELF)
#   emgd-lib.tgz       tar of /opt/emgd/lib from a provisioned target (EMGD 1.18)
#   gma500blt_drv.so   built from features/xf86-video-gma500blt (Xorg ABI 24)
#
# See ../PACKAGING.md for how to produce each artifact.  Requires dpkg-deb.
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
VERSION="${VERSION:-1.0.0}"
ARCH="${ARCH:-i386}"
KERNEL="${KERNEL:-5.4.0-150-generic}"
ARTIFACTS="${ARTIFACTS:-$HERE/artifacts}"
OUT="$HERE/out"

echo ">> artifacts from: $ARTIFACTS"
for f in pvrsrvkm.ko gma500_gfx.ko gma500dc.ko srvinit emgd-lib.tgz gma500blt_drv.so; do
    [ -e "$ARTIFACTS/$f" ] || { echo "MISSING artifact: $ARTIFACTS/$f" >&2; exit 1; }
done

# Fresh build trees (copy the tracked skeletons, then drop in binaries).
rm -rf "$HERE/build"; mkdir -p "$HERE/build" "$OUT"
cp -a "$HERE/modules"   "$HERE/build/modules"
cp -a "$HERE/userspace" "$HERE/build/userspace"
cp -a "$HERE/meta"      "$HERE/build/meta"

# --- modules package: drop in the three .ko + srvinit -------------------------
M="$HERE/build/modules"
install -m 0644 "$ARTIFACTS/pvrsrvkm.ko"   "$M/opt/poulsbo-sgx/pvrsrvkm.ko"
install -m 0644 "$ARTIFACTS/gma500_gfx.ko" "$M/opt/poulsbo-sgx/gma500_gfx.ko"
install -m 0644 "$ARTIFACTS/gma500dc.ko"   "$M/opt/poulsbo-sgx/gma500dc.ko"
# proc_dri_compat: /proc/dri/0/name shim the closed emgd_dri.so needs for GLX.
install -m 0644 "$ARTIFACTS/proc_dri_compat.ko" "$M/opt/poulsbo-sgx/proc_dri_compat.ko"
install -m 0755 "$ARTIFACTS/srvinit"       "$M/opt/poulsbo-sgx/srvinit"
chmod 0755 "$M/opt/poulsbo-sgx/load.sh" "$M/opt/poulsbo-sgx/srvinit.sh" \
          "$M/DEBIAN/postinst" "$M/DEBIAN/prerm" "$M/DEBIAN/postrm"

# --- userspace package: EMGD libs + DDX --------------------------------------
U="$HERE/build/userspace"
mkdir -p "$U/opt/emgd" "$U/usr/lib/xorg/modules/drivers"
tar xzf "$ARTIFACTS/emgd-lib.tgz" -C "$U/opt/emgd"          # -> opt/emgd/lib/*
# WSEGL: HW-accelerated OpenGL/GLX under X needs the ORIGINAL DDK window WSEGL
# (it creates the window render surface via WSEGL_CreateWindowDrawable); the
# custom offscreen/flip stub cannot and makes glXMakeCurrent NULL-deref. Install
# the recovered DDK build (features/emgd-wsegl/orig-ddk/) as the canonical
# libemgdPVR2D_DRIWSEGL.so.1.5.15.3226 and drop any .real backup so ldconfig
# does not repoint the SONAME symlink elsewhere.
rm -f "$U/opt/emgd/lib/libemgdPVR2D_DRIWSEGL.so.real"
install -m 0644 "$HERE/../features/emgd-wsegl/orig-ddk/libemgdPVR2D_DRIWSEGL.so.1.5.15.3226" \
        "$U/opt/emgd/lib/libemgdPVR2D_DRIWSEGL.so.1.5.15.3226"
ln -sf libemgdPVR2D_DRIWSEGL.so.1.5.15.3226 \
       "$U/opt/emgd/lib/libemgdPVR2D_DRIWSEGL.so"
ln -sf libemgdPVR2D_DRIWSEGL.so.1.5.15.3226 \
       "$U/opt/emgd/lib/libemgdPVR2D_DRIWSEGL.so.1"
install -m 0644 "$ARTIFACTS/gma500blt_drv.so" \
        "$U/usr/lib/xorg/modules/drivers/gma500blt_drv.so"
# GLX payload staged under /opt/poulsbo-sgx-glx; glx-enable.sh (run by postinst)
# installs the glapi-patched emgd DRI driver on the Mesa path, sets the loader
# override, patches libGLX_mesa, and installs the AIGLX/DRI2 X config.
install -D -m 0755 "$HERE/glx-extra/glx-enable.sh" \
        "$U/opt/poulsbo-sgx-glx/glx-enable.sh"
install -D -m 0644 "$HERE/glx-extra/emgd_dri.so" \
        "$U/opt/poulsbo-sgx-glx/emgd_dri.so"
install -D -m 0644 "$HERE/glx-extra/xorg.conf.glx" \
        "$U/opt/poulsbo-sgx-glx/xorg.conf.glx"
# Desktop auto-start: a systemd service runs startx on vt7 (after the SGX
# modules + srvinit) into the marco session xinitrc -- marco (non-compositing)
# + HW 2D (PVR2DBlt) + HW OpenGL.  Deliberately NO compiz: GL compositing on
# this stack renders black and has hard-hung the box.
install -D -m 0644 "$HERE/desktop/poulsbo-desktop.service" \
        "$U/lib/systemd/system/poulsbo-desktop.service"
install -D -m 0755 "$HERE/desktop/xinitrc" "$U/root/.xinitrc"
chmod 0755 "$U/DEBIAN/postinst" "$U/DEBIAN/postrm"

# --- stamp versions/arch into every control file -----------------------------
for p in modules userspace; do
    sed -i "s/^Version: .*/Version: $VERSION/;s/^Architecture: .*/Architecture: $ARCH/" \
        "$HERE/build/$p/DEBIAN/control"
done
sed -i "s/^Version: .*/Version: $VERSION/" "$HERE/build/meta/DEBIAN/control"
sed -i "s/(= 1\.0\.0)/(= $VERSION)/g"      "$HERE/build/meta/DEBIAN/control"

# --- record Installed-Size + build each package ------------------------------
build_one() {
    local tree="$1" name="$2"
    local kb; kb=$(du -sk "$tree" | cut -f1)
    if grep -q '^Installed-Size:' "$tree/DEBIAN/control"; then
        sed -i "s/^Installed-Size: .*/Installed-Size: $kb/" "$tree/DEBIAN/control"
    else
        echo "Installed-Size: $kb" >> "$tree/DEBIAN/control"
    fi
    dpkg-deb --root-owner-group --build "$tree" "$OUT/$name" >/dev/null
    echo ">> built $OUT/$name"
}
build_one "$HERE/build/modules"   "poulsbo-sgx-modules_${VERSION}_${ARCH}.deb"
build_one "$HERE/build/userspace" "poulsbo-sgx-userspace_${VERSION}_${ARCH}.deb"
build_one "$HERE/build/meta"      "poulsbo-sgx_${VERSION}_all.deb"

echo
echo "Done. Packages in $OUT :"
ls -l "$OUT"/*.deb
