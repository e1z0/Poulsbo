#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_kernel="${TARGET_KERNEL:-5.4.0-150-generic}"
rollback_grace_secs="${ROLLBACK_GRACE_SECS:-180}"
module_path="/lib/modules/${target_kernel}/updates/poulsbo/gma500_gfx.ko"
state_dir="/var/lib/poulsbo-rollback"
pending_file="${state_dir}/pending.env"
backup_module_path="${state_dir}/gma500_gfx.previous.ko"
rollback_script="/usr/local/sbin/poulsbo-module-rollback.sh"
rollback_service="/etc/systemd/system/poulsbo-module-rollback.service"

ssh "${target_user}@${target_host}" "
set -euo pipefail
install -d -m 0755 '${state_dir}'
cat > '${rollback_script}' <<'EOF_SCRIPT'
#!/usr/bin/env bash
set -euo pipefail

state_dir='/var/lib/poulsbo-rollback'
pending_file=\"\${state_dir}/pending.env\"

if [[ ! -f \"\$pending_file\" ]]; then
    exit 0
fi

# shellcheck source=/dev/null
. \"\$pending_file\"

grace_secs=\"\${ROLLBACK_GRACE_SECS:-180}\"
sleep \"\$grace_secs\"

if [[ ! -f \"\$pending_file\" ]]; then
    exit 0
fi

if [[ -n \"\${BACKUP_MODULE_PATH:-}\" && -f \"\${BACKUP_MODULE_PATH}\" ]]; then
    logger -t poulsbo-rollback \"confirmation missing; restoring \${BACKUP_MODULE_PATH} to \${MODULE_PATH} and rebooting\"
    install -D -m 0644 \"\${BACKUP_MODULE_PATH}\" \"\${MODULE_PATH}\"
else
    logger -t poulsbo-rollback \"confirmation missing; removing \${MODULE_PATH} and rebooting\"
    rm -f \"\$MODULE_PATH\"
fi
depmod -a \"\$TARGET_KERNEL\" || true
rm -f \"\$pending_file\"
systemctl --no-block reboot || reboot
EOF_SCRIPT
chmod 0755 '${rollback_script}'
# Keep the unit bound directly to multi-user.target. The validated boot-time
# path does not add After=/Wants= on that same target.
cat > '${rollback_service}' <<'EOF_SERVICE'
[Unit]
Description=Rollback unconfirmed Poulsbo override module

[Service]
Type=simple
ExecStart=/usr/local/sbin/poulsbo-module-rollback.sh
Restart=no

[Install]
WantedBy=multi-user.target
EOF_SERVICE
cat > '${pending_file}' <<'EOF_PENDING'
TARGET_KERNEL='${target_kernel}'
MODULE_PATH='${module_path}'
BACKUP_MODULE_PATH='${backup_module_path}'
ROLLBACK_GRACE_SECS='${rollback_grace_secs}'
EOF_PENDING
systemctl daemon-reload
systemctl enable poulsbo-module-rollback.service >/dev/null
systemctl stop poulsbo-module-rollback.service >/dev/null 2>&1 || true
echo 'rollback_guard=armed'
echo 'rollback_pending_file=${pending_file}'
echo 'rollback_backup_module_path=${backup_module_path}'
echo 'rollback_grace_secs=${rollback_grace_secs}'
"
