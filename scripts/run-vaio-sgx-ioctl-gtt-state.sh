#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_dir="${TARGET_DIR:-/root/poulsbo-workloads}"
target_probe_bin="${TARGET_PROBE_BIN:-${target_dir}/drm_probe}"
target_query_bin="${TARGET_QUERY_BIN:-${target_dir}/psb_sgx_lab_gtt_state_query}"
target_probe_stage="${TARGET_PROBE_STAGE:-/root/drm_probe.$$.$RANDOM.new}"
target_query_stage="${TARGET_QUERY_STAGE:-/root/psb_sgx_lab_gtt_state_query.$$.$RANDOM.new}"
local_probe_bin="${LOCAL_PROBE_BIN:-$(pwd)/workloads/drm_probe}"
local_query_bin="${LOCAL_QUERY_BIN:-$(pwd)/workloads/psb_sgx_lab_gtt_state_query}"
target_node="${TARGET_NODE:-/dev/dri/card0}"
sysfs_dir="${SYSFS_DIR:-/sys/devices/pci0000:00/0000:00:02.0}"
hold_seconds="${HOLD_SECONDS:-15}"
probe_count="${PROBE_COUNT:-3}"

if [[ "${probe_count}" -lt 1 ]]; then
    echo "PROBE_COUNT must be at least 1" >&2
    exit 1
fi

for bin in "$local_probe_bin" "$local_query_bin"; do
    if [[ ! -f "$bin" ]]; then
        echo "Local binary not found: $bin" >&2
        exit 1
    fi
done

scp "$local_probe_bin" "${target_user}@${target_host}:${target_probe_stage}"
scp "$local_query_bin" "${target_user}@${target_host}:${target_query_stage}"

ssh "${target_user}@${target_host}" "bash -s" <<EOF
set -euo pipefail

mkdir -p "${target_dir}"
install -m 0755 -T "${target_probe_stage}" "${target_probe_bin}"
install -m 0755 -T "${target_query_stage}" "${target_query_bin}"
rm -f "${target_probe_stage}" "${target_query_stage}"

gtt_attr=\$(find "${sysfs_dir}" -name poulsbo_gtt_state -print -quit)
loaded=\$(modinfo -n gma500_gfx)
hold_seconds="${hold_seconds}"
tmpdir=\$(mktemp -d)
probe_pids=()

cleanup() {
    local pid
    for pid in "\${probe_pids[@]:-}"; do
        kill "\${pid}" 2>/dev/null || true
        wait "\${pid}" 2>/dev/null || true
    done
    rm -rf "\${tmpdir}"
}
trap cleanup EXIT

printf 'ATTR=%s\n' "\${gtt_attr}"
printf 'REMOTE_QUERY_BIN=%s\n' "${target_query_bin}"
printf 'LOADED=%s\n' "\${loaded}"
printf 'SRCVERSION=%s\n' "\$(modinfo -F srcversion "\${loaded}")"

printf 'IOCTL_GTT_STATE_BEFORE\n'
"${target_query_bin}" "${target_node}" > "\${tmpdir}/ioctl_before.txt"
cat "\${tmpdir}/ioctl_before.txt"
printf 'SYSFS_GTT_STATE_BEFORE\n'
cat "\${gtt_attr}" > "\${tmpdir}/sysfs_before.txt"
cat "\${tmpdir}/sysfs_before.txt"

for idx in \$(seq 1 "${probe_count}"); do
    "${target_probe_bin}" "${target_node}" "${hold_seconds}" > "\${tmpdir}/drm_probe_\${idx}.log" 2>&1 &
    probe_pids+=("\$!")
done

for _ in \$(seq 1 \$((hold_seconds + 5))); do
    ready_count=\$(grep -h '^drm_hold_seconds=' "\${tmpdir}"/drm_probe_*.log 2>/dev/null | wc -l || true)
    if [[ "\${ready_count}" -ge "${probe_count}" ]]; then
        break
    fi
    sleep 1
done

printf 'IOCTL_GTT_STATE_DURING\n'
"${target_query_bin}" "${target_node}" > "\${tmpdir}/ioctl_during.txt"
cat "\${tmpdir}/ioctl_during.txt"
printf 'SYSFS_GTT_STATE_DURING\n'
cat "\${gtt_attr}" > "\${tmpdir}/sysfs_during.txt"
cat "\${tmpdir}/sysfs_during.txt"

for pid in "\${probe_pids[@]}"; do
    wait "\${pid}"
done
probe_pids=()

printf 'IOCTL_GTT_STATE_AFTER\n'
"${target_query_bin}" "${target_node}" > "\${tmpdir}/ioctl_after.txt"
cat "\${tmpdir}/ioctl_after.txt"
printf 'SYSFS_GTT_STATE_AFTER\n'
cat "\${gtt_attr}" > "\${tmpdir}/sysfs_after.txt"
cat "\${tmpdir}/sysfs_after.txt"
EOF
