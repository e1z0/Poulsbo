#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_kernel="${TARGET_KERNEL:-5.4.0-150-generic}"
target_module="/lib/modules/${target_kernel}/updates/poulsbo/gma500_gfx.ko"

ssh "${target_user}@${target_host}" "
set -euo pipefail
rm -f '${target_module}'
depmod -a '${target_kernel}'
modinfo -n gma500_gfx
"
