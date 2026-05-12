#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_dir="${TARGET_DIR:-/root/poulsbo-workloads}"
target_bin="${TARGET_BIN:-${target_dir}/x11_dri2_probe}"
target_stage="${TARGET_STAGE:-/root/x11_dri2_probe.new}"
display="${DISPLAY_ID:-:0}"
xauthority="${XAUTHORITY_PATH:-/var/run/lightdm/root/:0}"
local_bin="${LOCAL_BIN:-$(pwd)/workloads/x11_dri2_probe}"

if [[ ! -f "$local_bin" ]]; then
    echo "Local probe not found: $local_bin" >&2
    exit 1
fi

scp "$local_bin" "${target_user}@${target_host}:${target_stage}"
ssh "${target_user}@${target_host}" "bash -s" <<EOF
set -euo pipefail
mkdir -p "${target_dir}"
install -m 0755 "${target_stage}" "${target_bin}"
rm -f "${target_stage}"

printf 'REMOTE_BIN=%s\n' "${target_bin}"
file "${target_bin}"
printf 'LDD\n'
ldd "${target_bin}" || true
printf 'XRANDR_SUMMARY\n'
XAUTHORITY="${xauthority}" DISPLAY="${display}" xrandr --current 2>/dev/null | sed -n '1,12p' || true
printf 'PROBE\n'
XAUTHORITY="${xauthority}" DISPLAY="${display}" "${target_bin}"
EOF
