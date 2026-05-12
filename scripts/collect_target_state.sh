#!/usr/bin/env bash
set -euo pipefail

out_root="${1:-target-captures}"
timestamp="$(date -u +%Y%m%dT%H%M%SZ)"
host="$(hostname 2>/dev/null || echo unknown-host)"
out_dir="${out_root%/}/${timestamp}-${host}"

mkdir -p "$out_dir"

capture() {
    local name="$1"
    shift
    {
        echo "# $*"
        echo
        "$@"
    } >"${out_dir}/${name}.txt" 2>&1 || true
}

capture_if_exists() {
    local name="$1"
    shift
    if command -v "$1" >/dev/null 2>&1; then
        capture "$name" "$@"
    fi
}

capture "uname" uname -a
capture_if_exists "lsb_release" lsb_release -a
capture_if_exists "lspci_nn" lspci -nn
capture_if_exists "lspci_nnk" lspci -nnk
capture_if_exists "lsmod" lsmod
capture_if_exists "modinfo_gma500" modinfo gma500_gfx
capture_if_exists "modinfo_psb" modinfo psb_gfx
capture_if_exists "dmesg" dmesg
capture_if_exists "journal_kernel" journalctl -k -b
capture_if_exists "xrandr_verbose" xrandr --verbose
capture_if_exists "glxinfo_B" glxinfo -B
capture_if_exists "eglinfo" eglinfo
capture_if_exists "drm_info" drm_info

if [[ -d /sys/class/drm ]]; then
    find /sys/class/drm -maxdepth 3 -type f | sort >"${out_dir}/sys-class-drm-files.txt" || true
fi

if [[ -d /sys/kernel/debug/dri ]]; then
    find /sys/kernel/debug/dri -maxdepth 3 -type f | sort >"${out_dir}/debugfs-dri-files.txt" || true
fi

{
    echo "capture_dir=${out_dir}"
    echo "utc_timestamp=${timestamp}"
} >"${out_dir}/meta.txt"

echo "Wrote target capture to ${out_dir}"
