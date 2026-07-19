#!/bin/sh
# Deploy the current DDX + run the one-shot MakeCurrent diagnostic on the Vaio.
# Usage: ./run_debug.sh   (run from a host that can ssh root@10.1.11.218)
set -e
H=root@10.1.11.218
scp makecurrent.gdb $H:/root/makecurrent.gdb
ssh $H '
  pkill -9 Xorg glx_probe gdb 2>/dev/null; sleep 1
  echo 0 > /proc/sys/kernel/randomize_va_space
  # (rebuild+install DDX first if changed: cd /root/gma500blt-ddx && make && cp gma500blt_drv.so /usr/lib/xorg/modules/drivers/)
  cd /root; LD_LIBRARY_PATH=/opt/emgd/lib ./srvinit >/dev/null 2>&1
  timeout 60 X :5 -config /etc/X11/xorg.conf.glx -logfile /var/log/Xorg.5.log -noreset vt7 >/tmp/x5.out 2>&1 &
  sleep 7
  DISPLAY=:5 LD_LIBRARY_PATH=/opt/emgd/lib MESA_LOADER_DRIVER_OVERRIDE=emgd \
    gdb -batch -x /root/makecurrent.gdb --args /root/glx_probe 2>&1 |
    grep -viE "drmOpen|node name|open result|drmGetBusid|drmOpenMinor|Searching|card[0-9]"
  pkill -9 Xorg 2>/dev/null
'
