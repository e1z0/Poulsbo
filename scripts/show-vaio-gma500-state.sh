#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_kernel="${TARGET_KERNEL:-5.4.0-150-generic}"
state_dir="/var/lib/poulsbo-rollback"
pending_file="${state_dir}/pending.env"
backup_file="${state_dir}/gma500_gfx.previous.ko"
service_name="poulsbo-module-rollback.service"
module_path="/lib/modules/${target_kernel}/updates/poulsbo/gma500_gfx.ko"
panic_state_dir="/var/lib/poulsbo-panic"
panic_latest_env="${panic_state_dir}/latest.env"
panic_collector_service="poulsbo-panic-collect.service"

ssh "${target_user}@${target_host}" bash -s -- \
    "${module_path}" \
    "${pending_file}" \
    "${backup_file}" \
    "${service_name}" \
    "${panic_state_dir}" \
    "${panic_latest_env}" \
    "${panic_collector_service}" <<'EOF_REMOTE'
set -euo pipefail

module_path="$1"
pending_file="$2"
backup_file="$3"
service_name="$4"
panic_state_dir="$5"
panic_latest_env="$6"
panic_collector_service="$7"

join_csv() {
    local first=1
    local item
    for item in "$@"; do
        if (( first )); then
            printf '%s' "$item"
            first=0
        else
            printf ',%s' "$item"
        fi
    done
}

collect_glob_names() {
    local pattern="$1"
    local matches=()
    local path

    shopt -s nullglob
    for path in $pattern; do
        matches+=("${path##*/}")
    done
    shopt -u nullglob

    if (( ${#matches[@]} == 0 )); then
        echo none
        return
    fi

    join_csv "${matches[@]}"
}

collect_filtered_glob_names() {
    local pattern="$1"
    local regex="$2"
    local matches=()
    local path
    local name

    shopt -s nullglob
    for path in $pattern; do
        name="${path##*/}"
        if [[ "${name}" =~ ${regex} ]]; then
            matches+=("${name}")
        fi
    done
    shopt -u nullglob

    if (( ${#matches[@]} == 0 )); then
        echo none
        return
    fi

    join_csv "${matches[@]}"
}

extract_cmdline_arg() {
    local key="$1"
    local arg

    for arg in $(cat /proc/cmdline); do
        if [[ "$arg" == "${key}="* ]]; then
            printf '%s\n' "${arg#${key}=}"
            return 0
        fi
    done
    printf '%s\n' "absent"
}

read_shell_assignment() {
    local key="$1"
    local file="$2"
    local line

    line="$(grep -E "^${key}=" "${file}" | tail -n 1 || true)"
    line="${line#*=}"

    if [[ "${line}" == '"'*'"' ]]; then
        line="${line#\"}"
        line="${line%\"}"
    fi

    printf '%s\n' "${line}"
}

echo "kernel=$(uname -r)"
echo 'driver_module_name=gma500_gfx'
echo "driver_modinfo_path=$(modinfo -n gma500_gfx 2>/dev/null || echo missing)"
echo "driver_srcversion=$(modinfo -F srcversion gma500_gfx 2>/dev/null || echo missing)"
echo "override_module_path=${module_path}"
if [[ -f "${module_path}" ]]; then
    echo 'override_module_present=1'
    echo "override_module_size=$(stat -c %s "${module_path}")"
else
    echo 'override_module_present=0'
fi

if [[ -d /sys/module/gma500_gfx ]]; then
    echo 'driver_module_loaded=1'
    echo "driver_module_refcnt=$(cat /sys/module/gma500_gfx/refcnt 2>/dev/null || echo unknown)"
    echo "driver_module_coresize=$(cat /sys/module/gma500_gfx/coresize 2>/dev/null || echo unknown)"
else
    echo 'driver_module_loaded=0'
fi

if [[ -d /sys/class/drm ]]; then
    echo 'drm_sys_class_present=1'
    drm_cards="$(collect_filtered_glob_names '/sys/class/drm/card*' '^card[0-9]+$')"
    drm_connectors="$(collect_filtered_glob_names '/sys/class/drm/card*' '^card[0-9]+-')"
    echo "drm_card_entries=${drm_cards}"
    echo "drm_connector_entries=${drm_connectors}"
else
    echo 'drm_sys_class_present=0'
    echo 'drm_card_entries=none'
    echo 'drm_connector_entries=none'
fi

if [[ -d /dev/dri ]]; then
    echo 'dri_dev_dir_present=1'
    echo "dri_dev_nodes=$(collect_glob_names '/dev/dri/*')"
else
    echo 'dri_dev_dir_present=0'
    echo 'dri_dev_nodes=none'
fi

if [[ -e /dev/dri/card0 ]]; then
    echo 'drm_card0_present=1'
else
    echo 'drm_card0_present=0'
fi

if systemctl list-unit-files lightdm.service >/dev/null 2>&1; then
    lightdm_enabled="$(systemctl is-enabled lightdm.service 2>/dev/null || true)"
    lightdm_active="$(systemctl is-active lightdm.service 2>/dev/null || true)"
    [[ -n "${lightdm_enabled}" ]] || lightdm_enabled='unknown'
    [[ -n "${lightdm_active}" ]] || lightdm_active='unknown'
    echo 'lightdm_unit_present=1'
    echo "lightdm_enabled=${lightdm_enabled}"
    echo "lightdm_active=${lightdm_active}"
else
    echo 'lightdm_unit_present=0'
    echo 'lightdm_enabled=absent'
    echo 'lightdm_active=absent'
fi

echo "system_default_target=$(systemctl get-default 2>/dev/null || echo unknown)"
echo "proc_cmdline=$(cat /proc/cmdline 2>/dev/null || echo unknown)"
echo "crashkernel_cmdline_value=$(extract_cmdline_arg crashkernel)"
echo "kernel_panic=$(cat /proc/sys/kernel/panic 2>/dev/null || echo unknown)"
echo "kernel_panic_on_oops=$(cat /proc/sys/kernel/panic_on_oops 2>/dev/null || echo unknown)"
echo "kernel_sysrq=$(cat /proc/sys/kernel/sysrq 2>/dev/null || echo unknown)"

xorg_count="$(pgrep -cx Xorg 2>/dev/null || true)"
[[ -n "${xorg_count}" ]] || xorg_count=0
echo "xorg_process_count=${xorg_count}"

if dpkg -s kdump-tools >/dev/null 2>&1; then
    echo 'kdump_tools_installed=1'
else
    echo 'kdump_tools_installed=0'
fi

if systemctl list-unit-files kdump-tools.service >/dev/null 2>&1; then
    kdump_enabled="$(systemctl is-enabled kdump-tools.service 2>/dev/null || true)"
    kdump_active="$(systemctl is-active kdump-tools.service 2>/dev/null || true)"
    [[ -n "${kdump_enabled}" ]] || kdump_enabled='unknown'
    [[ -n "${kdump_active}" ]] || kdump_active='unknown'
    echo "kdump_service_enabled=${kdump_enabled}"
    echo "kdump_service_active=${kdump_active}"
else
    echo 'kdump_service_enabled=absent'
    echo 'kdump_service_active=absent'
fi

if [[ -f /etc/default/kdump-tools ]]; then
    echo 'kdump_default_present=1'
    echo "kdump_use_kdump=$(read_shell_assignment USE_KDUMP /etc/default/kdump-tools)"
    echo "kdump_coredir=$(read_shell_assignment KDUMP_COREDIR /etc/default/kdump-tools)"
    echo "kdump_makedump_args=$(read_shell_assignment MAKEDUMP_ARGS /etc/default/kdump-tools)"
else
    echo 'kdump_default_present=0'
    echo 'kdump_use_kdump='
    echo 'kdump_coredir='
    echo 'kdump_makedump_args='
fi

if systemctl list-unit-files "${panic_collector_service}" >/dev/null 2>&1; then
    panic_enabled="$(systemctl is-enabled "${panic_collector_service}" 2>/dev/null || true)"
    panic_active="$(systemctl is-active "${panic_collector_service}" 2>/dev/null || true)"
    [[ -n "${panic_enabled}" ]] || panic_enabled='unknown'
    [[ -n "${panic_active}" ]] || panic_active='unknown'
    echo "panic_collector_service_enabled=${panic_enabled}"
    echo "panic_collector_service_active=${panic_active}"
else
    echo 'panic_collector_service_enabled=absent'
    echo 'panic_collector_service_active=absent'
fi

if [[ -d "${panic_state_dir}" ]]; then
    echo 'panic_state_dir_present=1'
else
    echo 'panic_state_dir_present=0'
fi

if [[ -f "${panic_latest_env}" ]]; then
    echo 'panic_latest_present=1'
    # shellcheck disable=SC1090
    . "${panic_latest_env}"
    echo "panic_latest_dump_present=${latest_dump_present:-${latest_vmcore_present:-0}}"
    echo "panic_latest_dump_kind=${latest_dump_kind:-$([[ -n "${latest_vmcore_path:-}" ]] && printf '%s' vmcore || printf '%s' none)}"
    echo "panic_latest_dump_path=${latest_dump_path:-${latest_vmcore_path:-}}"
    echo "panic_latest_dump_dir=${latest_dump_dir:-${latest_vmcore_dir:-}}"
    echo "panic_latest_dump_size=${latest_dump_size:-${latest_vmcore_size:-0}}"
    echo "panic_latest_dump_dmesg_present=${latest_dump_dmesg_present:-${latest_vmcore_dmesg_present:-0}}"
    echo "panic_latest_dump_dmesg_path=${latest_dump_dmesg_path:-${latest_vmcore_dmesg_path:-}}"
    echo "panic_latest_vmcore_present=${latest_dump_present:-${latest_vmcore_present:-0}}"
    echo "panic_latest_vmcore_path=${latest_dump_path:-${latest_vmcore_path:-}}"
    echo "panic_latest_vmcore_size=${latest_dump_size:-${latest_vmcore_size:-0}}"
    echo "panic_latest_vmcore_dmesg_present=${latest_dump_dmesg_present:-${latest_vmcore_dmesg_present:-0}}"
    echo "panic_latest_vmcore_dmesg_path=${latest_dump_dmesg_path:-${latest_vmcore_dmesg_path:-}}"
    echo "panic_latest_prevboot_log_present=${prevboot_log_present:-0}"
    echo "panic_latest_journal_panic_matches=${journal_panic_matches:-0}"
    echo "panic_latest_pstore_entries=${latest_pstore_entries:-none}"
else
    echo 'panic_latest_present=0'
    echo 'panic_latest_dump_present=0'
    echo 'panic_latest_dump_kind=none'
    echo 'panic_latest_dump_path='
    echo 'panic_latest_dump_dir='
    echo 'panic_latest_dump_size=0'
    echo 'panic_latest_dump_dmesg_present=0'
    echo 'panic_latest_dump_dmesg_path='
    echo 'panic_latest_vmcore_present=0'
    echo 'panic_latest_vmcore_path='
    echo 'panic_latest_vmcore_size=0'
    echo 'panic_latest_vmcore_dmesg_present=0'
    echo 'panic_latest_vmcore_dmesg_path='
    echo 'panic_latest_prevboot_log_present=0'
    echo 'panic_latest_journal_panic_matches=0'
    echo 'panic_latest_pstore_entries=none'
fi

if systemctl list-unit-files "${service_name}" >/dev/null 2>&1; then
    rollback_enabled="$(systemctl is-enabled "${service_name}" 2>/dev/null || true)"
    rollback_active="$(systemctl is-active "${service_name}" 2>/dev/null || true)"
    [[ -n "${rollback_enabled}" ]] || rollback_enabled='unknown'
    [[ -n "${rollback_active}" ]] || rollback_active='unknown'
    echo "rollback_service_enabled=${rollback_enabled}"
    echo "rollback_service_active=${rollback_active}"
else
    echo 'rollback_service_enabled=absent'
    echo 'rollback_service_active=absent'
fi

if [[ -f "${pending_file}" ]]; then
    echo 'rollback_pending_present=1'
    # shellcheck disable=SC1090
    . "${pending_file}"
    echo "rollback_pending_target_kernel=${TARGET_KERNEL:-}"
    echo "rollback_pending_module_path=${MODULE_PATH:-}"
    echo "rollback_pending_backup_module_path=${BACKUP_MODULE_PATH:-}"
    echo "rollback_pending_grace_secs=${ROLLBACK_GRACE_SECS:-}"
else
    echo 'rollback_pending_present=0'
fi

if [[ -f "${backup_file}" ]]; then
    echo 'rollback_backup_present=1'
    echo "rollback_backup_size=$(stat -c %s "${backup_file}")"
else
    echo 'rollback_backup_present=0'
fi
EOF_REMOTE
