#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
state_dir="${STATE_DIR:-/var/lib/poulsbo-panic}"
dump_dir="${DUMP_DIR:-/var/crash}"
dest_dir="${DEST_DIR:-}"

usage() {
    cat <<'EOF'
Usage:
  fetch-vaio-panic-artifacts.sh

Behavior:
  Fetches the target-side panic collector state and the newest crash dump
  directory, when present, into one local destination directory.

Environment:
  TARGET_HOST   Remote target. Default: 10.1.11.218
  TARGET_USER   Remote user. Default: root
  STATE_DIR     Remote collector state directory. Default:
                /var/lib/poulsbo-panic
  DUMP_DIR      Remote crash dump root. Default: /var/crash
  DEST_DIR      Local destination directory. Required.
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if [[ -z "$dest_dir" ]]; then
    echo "DEST_DIR is required" >&2
    exit 1
fi

mkdir -p "$dest_dir"

remote_latest_dump_dir="$(
    ssh "${target_user}@${target_host}" bash -s -- "${state_dir}" "${dump_dir}" <<'EOF_REMOTE'
set -euo pipefail

state_dir="$1"
dump_dir="$2"
latest_env="${state_dir}/latest.env"
latest_dump_dir=""

if [[ -f "$latest_env" ]]; then
    latest_dump_dir="$(awk -F= '$1 == "latest_dump_dir" { print substr($0, index($0, "=") + 1) }' "$latest_env" | tail -n 1)"
    if [[ -z "$latest_dump_dir" ]]; then
        latest_dump_dir="$(awk -F= '$1 == "latest_vmcore_dir" { print substr($0, index($0, "=") + 1) }' "$latest_env" | tail -n 1)"
    fi
fi

if [[ -z "$latest_dump_dir" ]]; then
    latest_dump_dir="$(find "$dump_dir" -type f \( -name vmcore -o -name 'dump.*' \) -printf '%T@ %h\n' 2>/dev/null | sort -nr | head -n 1 | cut -d' ' -f2-)"
fi

printf '%s\n' "${latest_dump_dir}"
EOF_REMOTE
)"

echo "dest_dir=${dest_dir}"
echo "remote_state_dir=${state_dir}"
echo "remote_dump_dir=${dump_dir}"
echo "remote_latest_dump_dir=${remote_latest_dump_dir:-}"
echo "remote_latest_vmcore_dir=${remote_latest_dump_dir:-}"

mkdir -p "${dest_dir}/state"
scp -r \
    "${target_user}@${target_host}:${state_dir}/latest.env" \
    "${target_user}@${target_host}:${state_dir}/latest-kernel-prevboot.log" \
    "${target_user}@${target_host}:${state_dir}/latest-pstore" \
    "${dest_dir}/state/" >/dev/null 2>&1 || true

if [[ -n "${remote_latest_dump_dir}" ]]; then
    mkdir -p "${dest_dir}/crashdump"
    scp -r "${target_user}@${target_host}:${remote_latest_dump_dir}" "${dest_dir}/crashdump/" >/dev/null 2>&1 || true
    ln -sfn crashdump "${dest_dir}/vmcore"
fi

if [[ -f "${dest_dir}/state/latest.env" ]]; then
    cat "${dest_dir}/state/latest.env"
fi
