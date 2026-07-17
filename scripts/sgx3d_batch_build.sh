#!/bin/bash
# sgx3d_batch_build.sh <tag> ["description"]
# Builds the current build-tree module and stashes it as batches/<tag>/gma500_gfx.ko
# so it can be tested later (build needs Docker only, NOT the Vaio).
set -u
SP=/tmp/claude-1000/-Projects-Poulsbo/476a621f-66ff-423a-96f0-a1dca3ae4adf/scratchpad
B="$SP/gma500-3d-build"; OUT="$SP/batches"
TAG="${1:?usage: sgx3d_batch_build.sh <tag> [desc]}"; DESC="${2:-}"
log(){ echo "[$(date +%H:%M:%S)] $*"; }
mkdir -p "$OUT/$TAG"
log "building batch '$TAG'..."
find "$B" -name '*.o' -o -name '*.ko' -o -name '.*.cmd' -o -name '*.mod*' -o -name 'modules.order' -o -name 'Module.symvers' | xargs rm -f 2>/dev/null
if ! "$SP/kbuild/build.sh" "$B" >"$OUT/$TAG/build.log" 2>&1; then
  log "BUILD FAILED:"; grep -iE 'error:' "$OUT/$TAG/build.log" | head; exit 1
fi
cp "$B/gma500_gfx.ko" "$OUT/$TAG/gma500_gfx.ko"
echo "$DESC" > "$OUT/$TAG/desc.txt"
md5sum "$B/gma500_gfx.ko" | awk '{print $1}' > "$OUT/$TAG/md5"
grep -iE 'warning:' "$OUT/$TAG/build.log" | head -3
log "batch '$TAG' built: $(cat "$OUT/$TAG/md5")  [$DESC]"
