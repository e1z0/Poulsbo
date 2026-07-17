#!/bin/bash
# sgx3d_batch_test.sh <tag>
# Deploys batches/<tag>/gma500_gfx.ko to the Vaio, reboots, triggers 3D init,
# captures the full diagnostic to batches/<tag>/result.log, auto-recovers on hang.
# Prints PASS if INIT_COMPLETE reached, else FAIL with the key DIAG lines.
set -u
HOST=root@10.1.11.218; BDF=0000:00:02.0
SP=/tmp/claude-1000/-Projects-Poulsbo/476a621f-66ff-423a-96f0-a1dca3ae4adf/scratchpad
OUT="$SP/batches"; SSH="ssh -o BatchMode=yes -o ConnectTimeout=10"
TAG="${1:?usage: sgx3d_batch_test.sh <tag>}"
KO="$OUT/$TAG/gma500_gfx.ko"; RES="$OUT/$TAG/result.log"
log(){ echo "[$(date +%H:%M:%S)] $*"; }
wait_up(){ local n=0; while [ $n -lt 40 ]; do timeout 12 $SSH "$HOST" true 2>/dev/null && return 0; sleep 8; n=$((n+1)); done; return 1; }
[ -f "$KO" ] || { log "no prebuilt .ko for batch '$TAG' (run sgx3d_batch_build.sh first)"; exit 1; }

log "=== batch '$TAG' : $(cat "$OUT/$TAG/desc.txt" 2>/dev/null) ==="
log "ensuring box up..."; wait_up || { log "box down, needs power cycle"; exit 2; }
WANT_MD5=$(cat "$OUT/$TAG/md5" 2>/dev/null)
log "deploying (want md5 $WANT_MD5)..."
scp -o BatchMode=yes -o ConnectTimeout=12 "$KO" "$HOST":/root/stage/gma500_gfx.ko >/dev/null 2>&1 || { log "scp failed"; exit 2; }
# Record the current boot id so we can PROVE the box actually rebooted (the old
# flow assumed reboot happened; it silently didn't -> stale module ran).
OLDBOOT=$(timeout 12 $SSH "$HOST" 'cat /proc/sys/kernel/random/boot_id' 2>/dev/null)
# Install, verify md5 landed, then reboot detached (setsid so the SSH close does
# not kill it; -i/--force fallbacks in case systemd is unhappy).
GOTMD5=$(timeout 25 $SSH "$HOST" "cp /root/stage/gma500_gfx.ko /opt/gma500-blt/gma500_gfx.ko && md5sum /opt/gma500-blt/gma500_gfx.ko | awk '{print \$1}'" 2>/dev/null)
if [ -n "$WANT_MD5" ] && [ "$GOTMD5" != "$WANT_MD5" ]; then
  log "WARNING: installed md5 $GOTMD5 != batch md5 $WANT_MD5 (copy failed?)"
fi
timeout 12 $SSH "$HOST" 'setsid bash -c "sleep 2; systemctl reboot -i || systemctl reboot || reboot -f" </dev/null >/dev/null 2>&1 &' 2>/dev/null
log "reboot requested (old boot_id ${OLDBOOT:0:8}); waiting for a FRESH boot..."
sleep 20
# wait until the box answers AND boot_id changed (i.e. it really rebooted)
n=0; NEWBOOT=""
while [ $n -lt 45 ]; do
  if timeout 12 $SSH "$HOST" true 2>/dev/null; then
    NEWBOOT=$(timeout 12 $SSH "$HOST" 'cat /proc/sys/kernel/random/boot_id' 2>/dev/null)
    [ -n "$NEWBOOT" ] && [ "$NEWBOOT" != "$OLDBOOT" ] && break
  fi
  sleep 8; n=$((n+1))
done
if [ -z "$NEWBOOT" ]; then log "box did not come back after reboot (needs power cycle)"; exit 2; fi
if [ "$NEWBOOT" = "$OLDBOOT" ]; then log "ERROR: box never rebooted (boot_id unchanged) - stale module would run; aborting batch"; exit 3; fi
# Confirm the freshly-booted kernel actually loaded OUR module.
RUNMD5=$(timeout 12 $SSH "$HOST" 'md5sum /opt/gma500-blt/gma500_gfx.ko 2>/dev/null | awk "{print \$1}"' 2>/dev/null)
log "box rebooted (new boot_id ${NEWBOOT:0:8}); loaded module md5 $RUNMD5"
[ -n "$WANT_MD5" ] && [ "$RUNMD5" != "$WANT_MD5" ] && log "WARNING: running md5 != batch md5!"
log "box up (fresh); triggering 3D init..."
OUT_T=$(timeout 55 $SSH "$HOST" 'bash -s' <<'EOF' 2>&1
BDF=0000:00:02.0
echo 1 > /sys/module/gma500_gfx/parameters/sgx3d_enable
echo 0 > /sys/class/vtconsole/vtcon1/bind 2>/dev/null; sleep 1
dmesg -C
echo init > /sys/bus/pci/devices/$BDF/poulsbo_sgx_3d 2>&1; echo "trigger_rc=$?"
sleep 1
echo "===DMESG==="
dmesg | grep -iE "sgx3d|SGXReset|INIT_COMPLETE|BIF|fault|uploaded|EVENT_KICK|timeout|complete|DIAG|MEM_REQ|PDS_PC|DPM|requester" | tail -60
echo "===STATUS==="; cat /sys/bus/pci/devices/$BDF/poulsbo_sgx_3d 2>&1
echo "===ALIVE==="; uptime
EOF
)
RC=$?
{ echo "# batch $TAG  $(date)"; echo "# desc: $(cat "$OUT/$TAG/desc.txt" 2>/dev/null)"; echo "$OUT_T"; } > "$RES"
echo "$OUT_T"
if [ $RC -ne 0 ] || ! echo "$OUT_T" | grep -q "===ALIVE==="; then
  log "box hung - waiting for watchdog..."
  if wait_up; then log "auto-recovered"; else log "NO recovery - power cycle needed"; echo "RESULT $TAG: HUNG-NORECOVER" | tee -a "$RES"; exit 2; fi
  echo "RESULT $TAG: HUNG (box froze during trigger)" | tee -a "$RES"
elif echo "$OUT_T" | grep -q "INIT_COMPLETE (status"; then
  echo "RESULT $TAG: *** PASS - INIT_COMPLETE ***" | tee -a "$RES"
else
  echo "RESULT $TAG: FAIL - $(echo "$OUT_T" | grep -oE 'PDS_PC_BASE=[^ ]*|MEM_REQ_STAT=[^ ]*|last status=[^ )]*' | tr '\n' ' ')" | tee -a "$RES"
fi
log "batch '$TAG' done -> $RES"
