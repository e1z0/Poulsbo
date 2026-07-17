#!/bin/bash
# sgx3d_batch_test_all.sh  - test every prepared batch (dir with a .ko) in order.
set -u
SP=/tmp/claude-1000/-Projects-Poulsbo/476a621f-66ff-423a-96f0-a1dca3ae4adf/scratchpad
OUT="$SP/batches"; BD=/Projects/Poulsbo/scripts
echo "=== testing all prepared batches ==="
for d in "$OUT"/*/; do
  t=$(basename "$d"); [ -f "$d/gma500_gfx.ko" ] || continue
  "$BD/sgx3d_batch_test.sh" "$t" || echo "(batch $t aborted; continuing)"
done
echo; echo "=== SUMMARY ==="
for d in "$OUT"/*/; do t=$(basename "$d"); [ -f "$d/result.log" ] && grep -h "^RESULT" "$d/result.log"; done
