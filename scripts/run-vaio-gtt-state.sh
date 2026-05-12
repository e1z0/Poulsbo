#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"

ssh "${target_user}@${target_host}" 'bash -s' <<'EOF'
set -euo pipefail
attr=$(find /sys -name poulsbo_gtt_state -print -quit)
loaded=$(modinfo -n gma500_gfx)

printf 'ATTR=%s\n' "$attr"
printf 'LOADED=%s\n' "$loaded"
printf 'SRCVERSION=%s\n' "$(modinfo -F srcversion "$loaded")"
printf 'STATE\n'
cat "$attr"
EOF
