#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
crashkernel="${CRASHKERNEL:-128M}"
panic_timeout_secs="${PANIC_TIMEOUT_SECS:-10}"
panic_on_oops="${PANIC_ON_OOPS:-1}"
sysrq_value="${SYSRQ_VALUE:-1}"
dump_dir="${DUMP_DIR:-/var/crash}"
state_dir="${STATE_DIR:-/var/lib/poulsbo-panic}"
grub_dropin="${GRUB_DROPIN:-/etc/default/grub.d/50-poulsbo-panic-kdump.cfg}"
sysctl_file="${SYSCTL_FILE:-/etc/sysctl.d/99-poulsbo-panic.conf}"
kdump_defaults="${KDUMP_DEFAULTS:-/etc/default/kdump-tools}"
collector_script="${COLLECTOR_SCRIPT:-/usr/local/sbin/poulsbo-panic-collect.sh}"
collector_service="${COLLECTOR_SERVICE:-/etc/systemd/system/poulsbo-panic-collect.service}"

usage() {
    cat <<'EOF'
Usage:
  configure-vaio-panic-capture.sh

Behavior:
  Installs and configures target-side panic reboot plus kdump capture on the
  VAIO. The script is idempotent and does not reboot by itself.

Environment:
  TARGET_HOST          Remote target. Default: 10.1.11.218
  TARGET_USER          Remote user. Default: root
  CRASHKERNEL          Reserved crashkernel size. Default: 128M
  PANIC_TIMEOUT_SECS   Seconds before automatic reboot after panic. Default: 10
  PANIC_ON_OOPS        Panic on kernel oops. Default: 1
  SYSRQ_VALUE          Persistent kernel.sysrq value. Default: 1
  DUMP_DIR             Crash dump directory. Default: /var/crash
  STATE_DIR            Collector state directory. Default:
                       /var/lib/poulsbo-panic
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

ssh "${target_user}@${target_host}" bash -s -- \
    "${crashkernel}" \
    "${panic_timeout_secs}" \
    "${panic_on_oops}" \
    "${sysrq_value}" \
    "${dump_dir}" \
    "${state_dir}" \
    "${grub_dropin}" \
    "${sysctl_file}" \
    "${kdump_defaults}" \
    "${collector_script}" \
    "${collector_service}" <<'EOF_REMOTE'
set -euo pipefail

crashkernel="$1"
panic_timeout_secs="$2"
panic_on_oops="$3"
sysrq_value="$4"
dump_dir="$5"
state_dir="$6"
grub_dropin="$7"
sysctl_file="$8"
kdump_defaults="$9"
collector_script="${10}"
collector_service="${11}"

packages_installed=0
reboot_required=0
grub_changed=0
sysctl_changed=0
kdump_defaults_changed=0
collector_changed=0

set_assignment() {
    local file="$1"
    local key="$2"
    local value="$3"
    local tmp="${file}.tmp.$$"

    if [[ -f "$file" ]] && grep -q "^${key}=" "$file"; then
        awk -v key="$key" -v value="$value" '
            BEGIN { done = 0 }
            $0 ~ ("^" key "=") {
                print key "=" value
                done = 1
                next
            }
            { print }
            END {
                if (!done) {
                    print key "=" value
                }
            }
        ' "$file" >"$tmp"
        mv "$tmp" "$file"
    else
        if [[ -f "$file" ]]; then
            cp "$file" "$tmp"
        else
            : >"$tmp"
        fi
        printf '%s=%s\n' "$key" "$value" >>"$tmp"
        mv "$tmp" "$file"
    fi
}

csv_from_find() {
    local dir="$1"

    if [[ ! -d "$dir" ]]; then
        echo none
        return
    fi

    find "$dir" -mindepth 1 -maxdepth 1 -printf '%f\n' 2>/dev/null \
        | paste -sd, - \
        | sed 's/^$/none/'
}

tmpdir="$(mktemp -d)"
cleanup() {
    rm -rf "$tmpdir"
}
trap cleanup EXIT

missing_packages=0
for pkg in kdump-tools kexec-tools makedumpfile; do
    if ! dpkg -s "$pkg" >/dev/null 2>&1; then
        missing_packages=1
        break
    fi
done

if (( missing_packages )); then
    export DEBIAN_FRONTEND=noninteractive
    apt-get update
    apt-get install -y --no-install-recommends kdump-tools kexec-tools makedumpfile
    packages_installed=1
fi

install -d -m 0755 "$(dirname "$grub_dropin")" "$state_dir" "$dump_dir"

cat >"${tmpdir}/panic.sysctl" <<EOF_SYSCTL
kernel.panic=${panic_timeout_secs}
kernel.panic_on_oops=${panic_on_oops}
kernel.sysrq=${sysrq_value}
EOF_SYSCTL
if [[ ! -f "$sysctl_file" ]] || ! cmp -s "${tmpdir}/panic.sysctl" "$sysctl_file"; then
    install -m 0644 "${tmpdir}/panic.sysctl" "$sysctl_file"
    sysctl_changed=1
fi

cat >"${tmpdir}/panic.grub" <<EOF_GRUB
GRUB_CMDLINE_LINUX_DEFAULT="\${GRUB_CMDLINE_LINUX_DEFAULT:+\${GRUB_CMDLINE_LINUX_DEFAULT} }crashkernel=${crashkernel} panic=${panic_timeout_secs} panic_on_oops=${panic_on_oops}"
EOF_GRUB
if [[ ! -f "$grub_dropin" ]] || ! cmp -s "${tmpdir}/panic.grub" "$grub_dropin"; then
    install -m 0644 "${tmpdir}/panic.grub" "$grub_dropin"
    grub_changed=1
fi

if [[ ! -f "$kdump_defaults" ]]; then
    cat >"$kdump_defaults" <<'EOF_KDUMP'
USE_KDUMP=1
KDUMP_COREDIR="/var/crash"
KDUMP_SYSCTL="kernel.panic=10 kernel.panic_on_oops=1"
MAKEDUMP_ARGS="-c -d 31"
EOF_KDUMP
    kdump_defaults_changed=1
fi

before_hash="$(sha256sum "$kdump_defaults" | awk '{print $1}')"
set_assignment "$kdump_defaults" "USE_KDUMP" "1"
set_assignment "$kdump_defaults" "KDUMP_COREDIR" "\"${dump_dir}\""
set_assignment "$kdump_defaults" "KDUMP_SYSCTL" "\"kernel.panic=${panic_timeout_secs} kernel.panic_on_oops=${panic_on_oops}\""
set_assignment "$kdump_defaults" "MAKEDUMP_ARGS" "\"-c -d 31\""
after_hash="$(sha256sum "$kdump_defaults" | awk '{print $1}')"
if [[ "$before_hash" != "$after_hash" ]]; then
    kdump_defaults_changed=1
fi

cat >"${tmpdir}/collector.sh" <<'EOF_COLLECTOR'
#!/usr/bin/env bash
set -euo pipefail

state_dir='/var/lib/poulsbo-panic'
dump_dir='/var/crash'
latest_env="${state_dir}/latest.env"
latest_prevboot_log="${state_dir}/latest-kernel-prevboot.log"
latest_pstore_dir="${state_dir}/latest-pstore"
tmp_env="$(mktemp)"

cleanup() {
    rm -f "$tmp_env"
}
trap cleanup EXIT

install -d -m 0755 "$state_dir"
rm -rf "$latest_pstore_dir"
install -d -m 0755 "$latest_pstore_dir"

prevboot_available=0
if journalctl -k -b -1 --no-pager >"$latest_prevboot_log" 2>/dev/null; then
    prevboot_available=1
else
    : >"$latest_prevboot_log"
fi

if [[ -d /sys/fs/pstore ]]; then
    cp -a /sys/fs/pstore/. "$latest_pstore_dir"/ 2>/dev/null || true
fi

latest_dump="$(find "$dump_dir" -type f \( -name vmcore -o -name 'dump.*' \) -printf '%T@ %p\n' 2>/dev/null | sort -nr | head -n 1 | cut -d' ' -f2-)"
latest_dump_dir=""
latest_dump_size=0
latest_dump_kind="none"
latest_dump_dmesg="$(find "$dump_dir" -type f \( -name dmesg.txt -o -name vmcore-dmesg.txt -o -name 'dmesg.*' \) -printf '%T@ %p\n' 2>/dev/null | sort -nr | head -n 1 | cut -d' ' -f2-)"
if [[ -n "$latest_dump" ]]; then
    latest_dump_dir="$(dirname "$latest_dump")"
    latest_dump_size="$(stat -c %s "$latest_dump" 2>/dev/null || echo 0)"
    if [[ "${latest_dump##*/}" == "vmcore" ]]; then
        latest_dump_kind="vmcore"
    else
        latest_dump_kind="dumpfile"
    fi
fi

pstore_entries="$(find "$latest_pstore_dir" -mindepth 1 -maxdepth 1 -printf '%f\n' 2>/dev/null | paste -sd, -)"
[[ -n "$pstore_entries" ]] || pstore_entries="none"

journal_panic_matches=0
if [[ -s "$latest_prevboot_log" ]]; then
    journal_panic_matches="$(grep -Eic 'Kernel panic|Oops:|BUG:' "$latest_prevboot_log" || true)"
fi

{
    echo "collected_at=$(date -Iseconds)"
    echo "current_boot_id=$(cat /proc/sys/kernel/random/boot_id 2>/dev/null || echo unknown)"
    echo "prevboot_log_present=${prevboot_available}"
    echo "journal_panic_matches=${journal_panic_matches}"
    if [[ -n "$latest_dump" ]]; then
        echo "latest_dump_present=1"
        echo "latest_dump_kind=${latest_dump_kind}"
        echo "latest_dump_path=${latest_dump}"
        echo "latest_dump_dir=${latest_dump_dir}"
        echo "latest_dump_size=${latest_dump_size}"
        echo "latest_vmcore_present=1"
        echo "latest_vmcore_path=${latest_dump}"
        echo "latest_vmcore_dir=${latest_dump_dir}"
        echo "latest_vmcore_size=${latest_dump_size}"
    else
        echo "latest_dump_present=0"
        echo "latest_dump_kind=none"
        echo "latest_dump_path="
        echo "latest_dump_dir="
        echo "latest_dump_size=0"
        echo "latest_vmcore_present=0"
        echo "latest_vmcore_path="
        echo "latest_vmcore_dir="
        echo "latest_vmcore_size=0"
    fi
    if [[ -n "$latest_dump_dmesg" ]]; then
        echo "latest_dump_dmesg_present=1"
        echo "latest_dump_dmesg_path=${latest_dump_dmesg}"
        echo "latest_vmcore_dmesg_present=1"
        echo "latest_vmcore_dmesg_path=${latest_dump_dmesg}"
    else
        echo "latest_dump_dmesg_present=0"
        echo "latest_dump_dmesg_path="
        echo "latest_vmcore_dmesg_present=0"
        echo "latest_vmcore_dmesg_path="
    fi
    echo "latest_pstore_entries=${pstore_entries}"
} >"$tmp_env"

mv "$tmp_env" "$latest_env"
EOF_COLLECTOR
if [[ ! -f "$collector_script" ]] || ! cmp -s "${tmpdir}/collector.sh" "$collector_script"; then
    install -m 0755 "${tmpdir}/collector.sh" "$collector_script"
    collector_changed=1
fi

cat >"${tmpdir}/collector.service" <<'EOF_SERVICE'
[Unit]
Description=Collect Poulsbo panic artifacts after boot
After=local-fs.target systemd-journald.service
Wants=systemd-journald.service

[Service]
Type=oneshot
ExecStart=/usr/local/sbin/poulsbo-panic-collect.sh
RemainAfterExit=no

[Install]
WantedBy=multi-user.target
EOF_SERVICE
if [[ ! -f "$collector_service" ]] || ! cmp -s "${tmpdir}/collector.service" "$collector_service"; then
    install -m 0644 "${tmpdir}/collector.service" "$collector_service"
    collector_changed=1
fi

sysctl --system >/dev/null
systemctl daemon-reload
systemctl enable poulsbo-panic-collect.service >/dev/null
systemctl restart poulsbo-panic-collect.service

if (( grub_changed )); then
    update-grub
    reboot_required=1
fi

if (( packages_installed || kdump_defaults_changed || grub_changed )); then
    update-initramfs -u
    reboot_required=1
fi

systemctl enable kdump-tools >/dev/null 2>&1 || true
systemctl restart kdump-tools >/dev/null 2>&1 || true

cmdline="$(cat /proc/cmdline)"
crashkernel_cmdline_value=""
for arg in $cmdline; do
    if [[ "$arg" == crashkernel=* ]]; then
        crashkernel_cmdline_value="${arg#crashkernel=}"
        break
    fi
done

echo "packages_installed=${packages_installed}"
echo "sysctl_changed=${sysctl_changed}"
echo "grub_changed=${grub_changed}"
echo "kdump_defaults_changed=${kdump_defaults_changed}"
echo "collector_changed=${collector_changed}"
echo "reboot_required=${reboot_required}"
echo "configured_crashkernel=${crashkernel}"
echo "configured_panic_timeout_secs=${panic_timeout_secs}"
echo "configured_panic_on_oops=${panic_on_oops}"
echo "configured_sysrq=${sysrq_value}"
echo "current_cmdline=${cmdline}"
echo "current_crashkernel_cmdline_value=${crashkernel_cmdline_value:-absent}"
echo "current_kernel_panic=$(cat /proc/sys/kernel/panic)"
echo "current_kernel_panic_on_oops=$(cat /proc/sys/kernel/panic_on_oops)"
echo "current_kernel_sysrq=$(cat /proc/sys/kernel/sysrq)"
echo "kdump_enabled=$(systemctl is-enabled kdump-tools 2>/dev/null || true)"
echo "kdump_active=$(systemctl is-active kdump-tools 2>/dev/null || true)"
echo "collector_enabled=$(systemctl is-enabled poulsbo-panic-collect.service 2>/dev/null || true)"
echo "collector_active=$(systemctl is-active poulsbo-panic-collect.service 2>/dev/null || true)"
echo "kdump_status_begin"
kdump-config status 2>/dev/null || true
echo "kdump_status_end"
echo "panic_latest_begin"
sed -n '1,200p' "${state_dir}/latest.env" 2>/dev/null || true
echo "panic_latest_end"
EOF_REMOTE
