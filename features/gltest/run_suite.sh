#!/bin/bash
# run_suite.sh - full driver burn-in on the target.  Verifies the accelerated
# stack is loaded, brings the SGX up, runs the whole glburn GLES2 battery under
# an X server, captures any SGX/MMU faults from the kernel log around the run,
# and reports overall pass/fail + box health.
#
#   sudo ./run_suite.sh [seconds-per-test]   # default 5
set -u
SEC="${1:-5}"
EMGD=/opt/emgd/lib
DIR="$(cd "$(dirname "$0")" && pwd)"
fail=0

say(){ echo "=== $* ==="; }

say "1. driver stack loaded?"
for m in pvrsrvkm gma500_gfx gma500dc; do
    if grep -q "^$m " /proc/modules; then echo "  $m: loaded";
    else echo "  $m: MISSING"; fail=1; fi
done
ls /dev/dri/card0 >/dev/null 2>&1 && echo "  /dev/dri/card0: present" || { echo "  card0 MISSING"; fail=1; }

say "2. module parameters"
for pp in gma500_gfx/pvr_irq_forward gma500_gfx/flip_vsync gma500dc/flip_enable; do
    v=$(cat /sys/module/${pp%/*}/parameters/${pp#*/} 2>/dev/null)
    echo "  ${pp}=$v"
done

say "3. bring the SGX up (srvinit)"
LD_LIBRARY_PATH=$EMGD "$DIR/srvinit" 2>&1 | tail -2 || true
[ -x "$DIR/srvinit" ] || echo "  (srvinit helper not in $DIR; assuming already initialised)"

say "4. build glburn"
( cd "$DIR" && ./build.sh ) || { echo "  build FAILED"; exit 2; }

say "5. clear kernel log fault markers + snapshot"
FAULTS_BEFORE=$(dmesg | grep -icE "SGX.*fault|MMU.*fault|BIF_REQUESTER_FAULT|HWRecovery|BUG:|oops")

say "6. GLES2 burn battery (${SEC}s/test) under Xvfb :9"
(Xvfb :9 -screen 0 800x600x24 >/dev/null 2>&1 &) ; XVFB=$!
sleep 3
DISPLAY=:9 LD_LIBRARY_PATH=$EMGD "$DIR/glburn" all "$SEC"
GLRC=$?
pkill -f "Xvfb :9" 2>/dev/null

say "7. faults during the run?"
FAULTS_AFTER=$(dmesg | grep -icE "SGX.*fault|MMU.*fault|BIF_REQUESTER_FAULT|HWRecovery|BUG:|oops")
NEWF=$((FAULTS_AFTER-FAULTS_BEFORE))
echo "  new SGX/kernel fault lines: $NEWF"
[ "$NEWF" -gt 0 ] && { echo "  --- new faults ---"; dmesg | grep -iE "SGX.*fault|MMU.*fault|BIF_REQUESTER_FAULT|HWRecovery|BUG:|oops" | tail -"$NEWF"; fail=1; }

say "8. box health"
uptime
grep -qE "^pvrsrvkm|^gma500" /proc/modules && echo "  modules still live" || { echo "  modules GONE"; fail=1; }

echo
[ "$GLRC" -eq 0 ] && [ "$fail" -eq 0 ] && [ "$NEWF" -eq 0 ] \
    && echo "SUITE RESULT: PASS" || { echo "SUITE RESULT: FAIL (glburn=$GLRC checks=$fail faults=$NEWF)"; exit 1; }
