#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_kernel="${TARGET_KERNEL:-5.4.0-150-generic}"
local_module="${LOCAL_MODULE:-$(pwd)/build/devnull-vaio-gma500-src/gma500_gfx.ko}"
target_updates_dir="/lib/modules/${target_kernel}/updates/poulsbo"
target_stage="/root/gma500_gfx.instrumented.ko"
rollback_state_dir="/var/lib/poulsbo-rollback"
rollback_backup_path="${rollback_state_dir}/gma500_gfx.previous.ko"

if [[ ! -f "$local_module" ]]; then
    echo "Local module not found: $local_module" >&2
    exit 1
fi

scp "$local_module" "${target_user}@${target_host}:${target_stage}"
ssh "${target_user}@${target_host}" "
set -euo pipefail
mkdir -p '${target_updates_dir}'
mkdir -p '${rollback_state_dir}'
if [[ -f '${target_updates_dir}/gma500_gfx.ko' ]]; then
    install -m 0644 '${target_updates_dir}/gma500_gfx.ko' '${rollback_backup_path}'
    echo 'rollback_backup=updated'
else
    rm -f '${rollback_backup_path}'
    echo 'rollback_backup=none'
fi
install -m 0644 '${target_stage}' '${target_updates_dir}/gma500_gfx.ko'
depmod -a '${target_kernel}'
modinfo -n gma500_gfx
"
