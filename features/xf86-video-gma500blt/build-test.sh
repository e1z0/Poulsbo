#!/bin/sh
# build-test.sh - run ON THE VAIO. Build the gma500blt DDX against the Xorg 1.20
# SDK, install it, and start a test X server on it, capturing the log.
set -e
cd /root/gma500blt-ddx
echo "[1] build gma500blt_drv.so"
make clean >/dev/null 2>&1 || true
make 2>&1 | tail -20
ls -l gma500blt_drv.so || { echo "BUILD FAILED"; exit 1; }

echo "[2] install to xorg modules dir"
install -m0644 gma500blt_drv.so /usr/lib/xorg/modules/drivers/gma500blt_drv.so

echo "[3] xorg.conf for gma500blt"
cat > /etc/X11/xorg.conf.gma500blt <<EOF
Section "Device"
    Identifier "gma500blt"
    Driver     "gma500blt"
    BusID      "PCI:0:2:0"
EndSection
Section "ServerFlags"
    Option "AutoAddGPU" "false"
EndSection
EOF

echo "[4] start test X :8 with the gma500blt driver"
rm -f /var/log/Xorg.8.log
(Xorg :8 -config xorg.conf.gma500blt -logfile /var/log/Xorg.8.log vt8 -noreset >/tmp/x8.out 2>&1 &)
sleep 8
echo "[5] result:"
if pgrep -f "Xorg :8" >/dev/null; then
    echo "  Xorg :8 is RUNNING on gma500blt"
    DISPLAY=:8 xdpyinfo 2>/dev/null | grep -iE "dimensions|depth of root" | head
    echo "  --- EXA accel active? (log) ---"
    grep -iE "gma500blt|EXA|Solid|Copy|acceleration|drmmode|CRTC" /var/log/Xorg.8.log | head -15
    # quick 2D accel exercise
    DISPLAY=:8 timeout 10 x11perf -rect500 2>/dev/null | tail -3 || echo "  (x11perf not installed)"
else
    echo "  Xorg FAILED to start. Log tail:"
    grep -iE "\(EE\)|\(WW\)|fatal|gma500blt|drmmode|no screens|abort" /var/log/Xorg.8.log | tail -20
fi
echo "[6] stop test X"
pkill -f "Xorg :8" 2>/dev/null || true
echo "done. revert: rm /etc/X11/xorg.conf.gma500blt"
