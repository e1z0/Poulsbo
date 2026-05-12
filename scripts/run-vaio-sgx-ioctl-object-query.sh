#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_dir="${TARGET_DIR:-/root/poulsbo-workloads}"
target_probe_bin="${TARGET_PROBE_BIN:-${target_dir}/drm_probe}"
target_range_bin="${TARGET_RANGE_BIN:-${target_dir}/psb_sgx_lab_range_query}"
target_object_op_bin="${TARGET_OBJECT_OP_BIN:-${target_dir}/psb_sgx_lab_object_op}"
target_object_query_bin="${TARGET_OBJECT_QUERY_BIN:-${target_dir}/psb_sgx_lab_object_query}"
target_probe_stage="${TARGET_PROBE_STAGE:-/root/drm_probe.$$.$RANDOM.new}"
target_range_stage="${TARGET_RANGE_STAGE:-/root/psb_sgx_lab_range_query.$$.$RANDOM.new}"
target_object_op_stage="${TARGET_OBJECT_OP_STAGE:-/root/psb_sgx_lab_object_op.$$.$RANDOM.new}"
target_object_query_stage="${TARGET_OBJECT_QUERY_STAGE:-/root/psb_sgx_lab_object_query.$$.$RANDOM.new}"
local_probe_bin="${LOCAL_PROBE_BIN:-$(pwd)/workloads/drm_probe}"
local_range_bin="${LOCAL_RANGE_BIN:-$(pwd)/workloads/psb_sgx_lab_range_query}"
local_object_op_bin="${LOCAL_OBJECT_OP_BIN:-$(pwd)/workloads/psb_sgx_lab_object_op}"
local_object_query_bin="${LOCAL_OBJECT_QUERY_BIN:-$(pwd)/workloads/psb_sgx_lab_object_query}"
target_node="${TARGET_NODE:-/dev/dri/card0}"
sysfs_dir="${SYSFS_DIR:-/sys/devices/pci0000:00/0000:00:02.0}"
hold_seconds="${HOLD_SECONDS:-20}"
range_scan_max="${RANGE_SCAN_MAX:-128}"

for bin in "$local_probe_bin" "$local_range_bin" "$local_object_op_bin" "$local_object_query_bin"; do
    if [[ ! -f "$bin" ]]; then
        echo "Local binary not found: $bin" >&2
        exit 1
    fi
done

scp "$local_probe_bin" "${target_user}@${target_host}:${target_probe_stage}"
scp "$local_range_bin" "${target_user}@${target_host}:${target_range_stage}"
scp "$local_object_op_bin" "${target_user}@${target_host}:${target_object_op_stage}"
scp "$local_object_query_bin" "${target_user}@${target_host}:${target_object_query_stage}"

ssh "${target_user}@${target_host}" "bash -s" <<EOF
set -euo pipefail

mkdir -p "${target_dir}"
install -m 0755 -T "${target_probe_stage}" "${target_probe_bin}"
install -m 0755 -T "${target_range_stage}" "${target_range_bin}"
install -m 0755 -T "${target_object_op_stage}" "${target_object_op_bin}"
install -m 0755 -T "${target_object_query_stage}" "${target_object_query_bin}"
rm -f "${target_probe_stage}" "${target_range_stage}" "${target_object_op_stage}" "${target_object_query_stage}"

objects_node="${sysfs_dir}/poulsbo_sgx_objects"
tmpdir=\$(mktemp -d)
probe_pids=()

cleanup() {
    local pid
    for pid in "\${probe_pids[@]:-}"; do
        if [[ -n "\${pid}" ]] && kill -0 "\${pid}" 2>/dev/null; then
            kill "\${pid}" 2>/dev/null || true
            wait "\${pid}" 2>/dev/null || true
        fi
    done
    rm -rf "\${tmpdir}"
}
trap cleanup EXIT

if [[ ! -e "\${objects_node}" ]]; then
    echo "Missing sysfs node: \${objects_node}" >&2
    exit 1
fi

"${target_object_op_bin}" "${target_node}" cleanup >/dev/null 2>&1 || true

for probe_index in 1 2; do
    "${target_probe_bin}" "${target_node}" "${hold_seconds}" > "\${tmpdir}/drm_probe_\${probe_index}.log" 2>&1 &
    probe_pids+=("\$!")
done

for _ in \$(seq 1 $((hold_seconds + 5))); do
    ready=1
    for probe_index in 1 2; do
        if ! grep -q '^drm_hold_seconds=' "\${tmpdir}/drm_probe_\${probe_index}.log" 2>/dev/null; then
            ready=0
            break
        fi
    done
    if [[ "\${ready}" == "1" ]]; then
        break
    fi
    sleep 1
done

range_ids=()
for range_index in \$(seq 0 "${range_scan_max}"); do
    range_out="\$("${target_range_bin}" "${target_node}" index "\${range_index}" gem 2>/dev/null || true)"
    [[ -n "\${range_out}" ]] || break
    range_id=\$(printf '%s\n' "\${range_out}" | awk -F= '/^range_resolved_id=/ { print \$2 }')
    range_mmapping=\$(printf '%s\n' "\${range_out}" | awk -F= '/^range_mmapping=/ { print \$2 }')
    range_sgx_object_id=\$(printf '%s\n' "\${range_out}" | awk -F= '/^range_sgx_object_id=/ { print \$2 }')
    if [[ "\${range_mmapping}" == "1" && "\${range_sgx_object_id}" == "0" ]]; then
        range_ids+=("\${range_id}")
        if [[ "\${#range_ids[@]}" -ge 2 ]]; then
            break
        fi
    fi
done

if [[ "\${#range_ids[@]}" -lt 2 ]]; then
    echo "Failed to find two live mmapping GEM ranges" >&2
    exit 1
fi

range_id_a="\${range_ids[0]}"
range_id_b="\${range_ids[1]}"
echo "RANGE_ID_A=\${range_id_a}"
echo "RANGE_ID_B=\${range_id_b}"

object_a_out="\$("${target_object_op_bin}" "${target_node}" adopt "\${range_id_a}")"
object_a_id=\$(printf '%s\n' "\${object_a_out}" | awk -F= '/^object_resolved_id=/ { print \$2 }')
echo "OBJECT_A_CREATE"
printf '%s\n' "\${object_a_out}"

object_b_out="\$("${target_object_op_bin}" "${target_node}" adopt "\${range_id_b}")"
object_b_id=\$(printf '%s\n' "\${object_b_out}" | awk -F= '/^object_resolved_id=/ { print \$2 }')
echo "OBJECT_B_CREATE"
printf '%s\n' "\${object_b_out}"

echo "OBJECT_A_ROLE"
"${target_object_op_bin}" "${target_node}" role "\${object_a_id}" control
echo "OBJECT_A_CLASS"
"${target_object_op_bin}" "${target_node}" class "\${object_a_id}" packet
echo "OBJECT_A_GPU_USE"
"${target_object_op_bin}" "${target_node}" gpu_use "\${object_a_id}" control_ccb
echo "OBJECT_A_CONTEXT"
"${target_object_op_bin}" "${target_node}" context "\${object_a_id}" pf
echo "OBJECT_A_PF_BIND"
"${target_object_op_bin}" "${target_node}" pf_bind "\${object_a_id}"

echo "OBJECTS_SYSFS_DURING"
cat "\${objects_node}"

echo "OBJECT_QUERY_ID_A_DURING"
"${target_object_query_bin}" "${target_node}" id "\${object_a_id}"
echo "OBJECT_QUERY_ID_B_DURING"
"${target_object_query_bin}" "${target_node}" id "\${object_b_id}"
echo "OBJECT_QUERY_INDEX_0_DURING"
"${target_object_query_bin}" "${target_node}" index 0
echo "OBJECT_QUERY_INDEX_1_DURING"
"${target_object_query_bin}" "${target_node}" index 1

for pid in "\${probe_pids[@]}"; do
    wait "\${pid}"
done
probe_pids=()

echo "OBJECT_QUERY_ID_A_AFTER_PROBES"
"${target_object_query_bin}" "${target_node}" id "\${object_a_id}"
echo "OBJECT_QUERY_ID_B_AFTER_PROBES"
"${target_object_query_bin}" "${target_node}" id "\${object_b_id}"

echo "OBJECT_CLEANUP"
"${target_object_op_bin}" "${target_node}" cleanup
echo "OBJECTS_SYSFS_AFTER_CLEANUP"
cat "\${objects_node}"

echo "OBJECT_QUERY_ID_A_AFTER_CLEANUP"
"${target_object_query_bin}" "${target_node}" id "\${object_a_id}" || true
echo "OBJECT_QUERY_ID_B_AFTER_CLEANUP"
"${target_object_query_bin}" "${target_node}" id "\${object_b_id}" || true
EOF
