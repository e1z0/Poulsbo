#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
state_dir="/var/lib/poulsbo-rollback"
pending_file="${state_dir}/pending.env"

ssh "${target_user}@${target_host}" "
set -euo pipefail
rm -f '${pending_file}'
systemctl stop poulsbo-module-rollback.service >/dev/null 2>&1 || true
echo 'rollback_guard=cleared'
echo 'rollback_pending_file=${pending_file}'
"
