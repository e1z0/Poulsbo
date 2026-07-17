#!/bin/bash
# sgx3d_iterate.sh - fully automated SGX 3D bring-up iteration harness.
# Builds the gma500+sgx_3d module, deploys to the Vaio, reboots, triggers the
# 3D microkernel init, captures dmesg, and (relying on the Vaio watchdog)
# auto-recovers from any hang and pulls the kdump crash log. No human needed.
#
# Usage: sgx3d_iterate.sh [--no-build]
set -u
HOST=root@10.1.11.218
BDF=0000:00:02.0
SP=/tmp/claude-1000/-Projects-Poulsbo/476a621f-66ff-423a-96f0-a1dca3ae4adf/scratchpad
B="$SP/gma500-3d-build"
SSH="ssh -o BatchMode=yes -o ConnectTimeout=10"

log(){ echo "[$(date +%H:%M:%S)] $*"; }

wait_up(){  # wait until SSH answers (watchdog auto-reboots hung box)
  local n=0
  while [ $n -lt 40 ]; do
    if timeout 12 $SSH "$HOST" true 2>/dev/null; then return 0; fi
    sleep 8; n=$((n+1))
  done
  return 1
}

# 1. build
if [ "${1:-}" != "--no-build" ]; then
  log "building module..."
  find "$B" -name '*.o' -o -name '*.ko' -o -name '.*.cmd' -o -name '*.mod*' -o -name 'modules.order' -o -name 'Module.symvers' | xargs rm -f 2>/dev/null
  if ! "$SP/kbuild/build.sh" "$B" >/tmp/sgx3d_build.log 2>&1; then
    log "BUILD FAILED:"; grep -iE 'error:' /tmp/sgx3d_build.log | head; exit 1
  fi
  grep -iE 'warning:|error:' /tmp/sgx3d_build.log | head
  log "build ok: $(md5sum "$B/gma500_gfx.ko" | awk '{print $1}')"
fi

# 2. ensure box is up, then deploy + reboot
log "ensuring box up..."; wait_up || { log "box down and not recovering (needs physical power cycle)"; exit 2; }
log "deploying module..."
scp -o BatchMode=yes -o ConnectTimeout=12 "$B/gma500_gfx.ko" "$HOST":/root/stage/gma500_gfx.ko >/dev/null 2>&1
timeout 25 $SSH "$HOST" 'cp /root/stage/gma500_gfx.ko /opt/gma500-blt/gma500_gfx.ko; ( sleep 2; systemctl reboot ) >/dev/null 2>&1 &' 2>/dev/null
log "rebooting to load new module..."
sleep 20
wait_up || { log "box did not come back after deploy-reboot"; exit 2; }
log "box back up"

# 3. trigger 3D init (fbcon unbound; NEVER rebind -> avoids 2D wedge). Capture.
log "triggering 3D init..."
OUT=$(timeout 50 $SSH "$HOST" 'bash -s' <<'EOF' 2>&1
BDF=0000:00:02.0
echo 1 > /sys/module/gma500_gfx/parameters/sgx3d_enable
echo 0 > /sys/class/vtconsole/vtcon1/bind 2>/dev/null; sleep 1
dmesg -C
echo init > /sys/bus/pci/devices/$BDF/poulsbo_sgx_3d 2>&1; echo "trigger_rc=$?"
sleep 1
echo "===DMESG==="
dmesg | grep -iE "sgx3d|SGXReset|INIT_COMPLETE|BIF|fault|uploaded|EVENT_KICK|timeout|complete|requester|fault_addr" | tail -50
echo "===STATUS==="
cat /sys/bus/pci/devices/$BDF/poulsbo_sgx_3d 2>&1
echo "===ALIVE==="
uptime
EOF
)
RC=$?
echo "$OUT"

# 4. if the trigger hung the box, wait for watchdog auto-recovery + pull kdump
if [ $RC -ne 0 ] || ! echo "$OUT" | grep -q "===ALIVE==="; then
  log "box hung during trigger (rc=$RC) - waiting for watchdog auto-reboot..."
  if wait_up; then
    log "AUTO-RECOVERED by watchdog. Pulling kdump crash log..."
    timeout 30 $SSH "$HOST" 'D=$(ls -1dt /var/crash/*/ 2>/dev/null | head -1); echo "dump: $D"; cat "$D"/dmesg.* 2>/dev/null | grep -iE "sgx3d|Oops|BUG|Call Trace|EIP:|panic|fault|NULL|psb_|note_page" | tail -30' 2>/dev/null
  else
    log "box did NOT auto-recover - needs physical power cycle (SoC fully frozen)"
    exit 2
  fi
fi
log "iteration complete."
