#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_dir="${TARGET_DIR:-/root/poulsbo-workloads}"
target_node="${TARGET_NODE:-/dev/dri/card0}"
hold_seconds="${HOLD_SECONDS:-20}"
probe_count="${PROBE_COUNT:-5}"
range_scan_max="${RANGE_SCAN_MAX:-128}"

target_probe_bin="${TARGET_PROBE_BIN:-${target_dir}/drm_probe}"
target_range_bin="${TARGET_RANGE_BIN:-${target_dir}/psb_sgx_lab_range_query}"
target_object_op_bin="${TARGET_OBJECT_OP_BIN:-${target_dir}/psb_sgx_lab_object_op}"
target_object_query_bin="${TARGET_OBJECT_QUERY_BIN:-${target_dir}/psb_sgx_lab_object_query}"
target_submit_op_bin="${TARGET_SUBMIT_OP_BIN:-${target_dir}/psb_sgx_lab_submit_op}"
target_query_bin="${TARGET_QUERY_BIN:-${target_dir}/psb_sgx_lab_query}"
target_sgx_state_bin="${TARGET_SGX_STATE_BIN:-${target_dir}/psb_sgx_lab_sgx_state_query}"
target_gtt_state_bin="${TARGET_GTT_STATE_BIN:-${target_dir}/psb_sgx_lab_gtt_state_query}"
target_dump_bin="${TARGET_DUMP_BIN:-${target_dir}/psb_sgx_lab_dump}"

target_probe_stage="${TARGET_PROBE_STAGE:-/root/drm_probe.$$.$RANDOM.new}"
target_range_stage="${TARGET_RANGE_STAGE:-/root/psb_sgx_lab_range_query.$$.$RANDOM.new}"
target_object_op_stage="${TARGET_OBJECT_OP_STAGE:-/root/psb_sgx_lab_object_op.$$.$RANDOM.new}"
target_object_query_stage="${TARGET_OBJECT_QUERY_STAGE:-/root/psb_sgx_lab_object_query.$$.$RANDOM.new}"
target_submit_op_stage="${TARGET_SUBMIT_OP_STAGE:-/root/psb_sgx_lab_submit_op.$$.$RANDOM.new}"
target_query_stage="${TARGET_QUERY_STAGE:-/root/psb_sgx_lab_query.$$.$RANDOM.new}"
target_sgx_state_stage="${TARGET_SGX_STATE_STAGE:-/root/psb_sgx_lab_sgx_state_query.$$.$RANDOM.new}"
target_gtt_state_stage="${TARGET_GTT_STATE_STAGE:-/root/psb_sgx_lab_gtt_state_query.$$.$RANDOM.new}"
target_dump_stage="${TARGET_DUMP_STAGE:-/root/psb_sgx_lab_dump.$$.$RANDOM.new}"

local_probe_bin="${LOCAL_PROBE_BIN:-$(pwd)/workloads/drm_probe}"
local_range_bin="${LOCAL_RANGE_BIN:-$(pwd)/workloads/psb_sgx_lab_range_query}"
local_object_op_bin="${LOCAL_OBJECT_OP_BIN:-$(pwd)/workloads/psb_sgx_lab_object_op}"
local_object_query_bin="${LOCAL_OBJECT_QUERY_BIN:-$(pwd)/workloads/psb_sgx_lab_object_query}"
local_submit_op_bin="${LOCAL_SUBMIT_OP_BIN:-$(pwd)/workloads/psb_sgx_lab_submit_op}"
local_query_bin="${LOCAL_QUERY_BIN:-$(pwd)/workloads/psb_sgx_lab_query}"
local_sgx_state_bin="${LOCAL_SGX_STATE_BIN:-$(pwd)/workloads/psb_sgx_lab_sgx_state_query}"
local_gtt_state_bin="${LOCAL_GTT_STATE_BIN:-$(pwd)/workloads/psb_sgx_lab_gtt_state_query}"
local_dump_bin="${LOCAL_DUMP_BIN:-$(pwd)/workloads/psb_sgx_lab_dump}"

for bin in "$local_probe_bin" "$local_range_bin" "$local_object_op_bin" \
    "$local_object_query_bin" "$local_submit_op_bin" "$local_query_bin" \
    "$local_sgx_state_bin" "$local_gtt_state_bin" "$local_dump_bin"; do
    if [[ ! -f "$bin" ]]; then
        echo "Local binary not found: $bin" >&2
        exit 1
    fi
done

scp "$local_probe_bin" "${target_user}@${target_host}:${target_probe_stage}"
scp "$local_range_bin" "${target_user}@${target_host}:${target_range_stage}"
scp "$local_object_op_bin" "${target_user}@${target_host}:${target_object_op_stage}"
scp "$local_object_query_bin" "${target_user}@${target_host}:${target_object_query_stage}"
scp "$local_submit_op_bin" "${target_user}@${target_host}:${target_submit_op_stage}"
scp "$local_query_bin" "${target_user}@${target_host}:${target_query_stage}"
scp "$local_sgx_state_bin" "${target_user}@${target_host}:${target_sgx_state_stage}"
scp "$local_gtt_state_bin" "${target_user}@${target_host}:${target_gtt_state_stage}"
scp "$local_dump_bin" "${target_user}@${target_host}:${target_dump_stage}"

ssh "${target_user}@${target_host}" "bash -s" <<EOF
set -euo pipefail

mkdir -p "${target_dir}"
install -m 0755 -T "${target_probe_stage}" "${target_probe_bin}"
install -m 0755 -T "${target_range_stage}" "${target_range_bin}"
install -m 0755 -T "${target_object_op_stage}" "${target_object_op_bin}"
install -m 0755 -T "${target_object_query_stage}" "${target_object_query_bin}"
install -m 0755 -T "${target_submit_op_stage}" "${target_submit_op_bin}"
install -m 0755 -T "${target_query_stage}" "${target_query_bin}"
install -m 0755 -T "${target_sgx_state_stage}" "${target_sgx_state_bin}"
install -m 0755 -T "${target_gtt_state_stage}" "${target_gtt_state_bin}"
install -m 0755 -T "${target_dump_stage}" "${target_dump_bin}"
rm -f "${target_probe_stage}" "${target_range_stage}" "${target_object_op_stage}" \
    "${target_object_query_stage}" "${target_submit_op_stage}" \
    "${target_query_stage}" "${target_sgx_state_stage}" "${target_gtt_state_stage}" \
    "${target_dump_stage}"

tmpdir=\$(mktemp -d)
probe_pids=()

cleanup() {
    local pid
    "${target_submit_op_bin}" "${target_node}" cleanup >/dev/null 2>&1 || true
    "${target_object_op_bin}" "${target_node}" cleanup >/dev/null 2>&1 || true
    for pid in "\${probe_pids[@]:-}"; do
        if [[ -n "\${pid}" ]] && kill -0 "\${pid}" 2>/dev/null; then
            kill "\${pid}" 2>/dev/null || true
            wait "\${pid}" 2>/dev/null || true
        fi
    done
    rm -rf "\${tmpdir}"
}
trap cleanup EXIT

"${target_submit_op_bin}" "${target_node}" cleanup >/dev/null 2>&1 || true
"${target_object_op_bin}" "${target_node}" cleanup >/dev/null 2>&1 || true

for probe_index in \$(seq 1 "${probe_count}"); do
    "${target_probe_bin}" "${target_node}" "${hold_seconds}" > "\${tmpdir}/drm_probe_\${probe_index}.log" 2>&1 &
    probe_pids+=("\$!")
done

for _ in \$(seq 1 $((hold_seconds + 5))); do
    ready_count=\$(grep -h '^drm_hold_seconds=' "\${tmpdir}"/drm_probe_*.log 2>/dev/null | wc -l || true)
    if [[ "\${ready_count}" -ge "${probe_count}" ]]; then
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
        if [[ "\${#range_ids[@]}" -ge "${probe_count}" ]]; then
            break
        fi
    fi
done

if [[ "\${#range_ids[@]}" -lt "${probe_count}" ]]; then
    echo "Failed to find ${probe_count} live mmapping GEM ranges" >&2
    exit 1
fi

object_ids=()
for range_id in "\${range_ids[@]}"; do
    object_out="\$("${target_object_op_bin}" "${target_node}" adopt "\${range_id}")"
    object_id=\$(printf '%s\n' "\${object_out}" | awk -F= '/^object_resolved_id=/ { print \$2 }' | tail -n 1)
    if [[ -z "\${object_id}" || "\${object_id}" == "0" ]]; then
        echo "Failed to adopt range \${range_id}" >&2
        printf '%s\n' "\${object_out}" >&2
        exit 1
    fi
    object_ids+=("\${object_id}")
done

object_control="\${object_ids[0]}"
object_pds="\${object_ids[1]}"
object_use="\${object_ids[2]}"
object_target="\${object_ids[3]}"
object_status="\${object_ids[4]}"

"${target_object_op_bin}" "${target_node}" role "\${object_control}" control > "\${tmpdir}/object_role_control.txt"
"${target_object_op_bin}" "${target_node}" role "\${object_pds}" pds > "\${tmpdir}/object_role_pds.txt"
"${target_object_op_bin}" "${target_node}" role "\${object_use}" use > "\${tmpdir}/object_role_use.txt"
"${target_object_op_bin}" "${target_node}" role "\${object_target}" target > "\${tmpdir}/object_role_target.txt"
"${target_object_op_bin}" "${target_node}" role "\${object_status}" status > "\${tmpdir}/object_role_status.txt"
"${target_object_op_bin}" "${target_node}" class "\${object_control}" packet > "\${tmpdir}/object_class_control.txt"
"${target_object_op_bin}" "${target_node}" class "\${object_pds}" code > "\${tmpdir}/object_class_pds.txt"
"${target_object_op_bin}" "${target_node}" class "\${object_use}" code > "\${tmpdir}/object_class_use.txt"
"${target_object_op_bin}" "${target_node}" class "\${object_target}" surface > "\${tmpdir}/object_class_target.txt"
"${target_object_op_bin}" "${target_node}" class "\${object_status}" state > "\${tmpdir}/object_class_status.txt"
"${target_object_op_bin}" "${target_node}" gpu_use "\${object_control}" control_ccb > "\${tmpdir}/object_gpu_use_control.txt"
"${target_object_op_bin}" "${target_node}" gpu_use "\${object_pds}" pds_code > "\${tmpdir}/object_gpu_use_pds.txt"
"${target_object_op_bin}" "${target_node}" gpu_use "\${object_use}" use_code > "\${tmpdir}/object_gpu_use_use.txt"
"${target_object_op_bin}" "${target_node}" gpu_use "\${object_target}" render_target > "\${tmpdir}/object_gpu_use_target.txt"
"${target_object_op_bin}" "${target_node}" gpu_use "\${object_status}" status_block > "\${tmpdir}/object_gpu_use_status.txt"
for object_id in "\${object_ids[@]}"; do
    "${target_object_op_bin}" "${target_node}" context "\${object_id}" pf > "\${tmpdir}/object_context_\${object_id}.txt"
    "${target_object_op_bin}" "${target_node}" pf_bind "\${object_id}" > "\${tmpdir}/object_pf_bind_\${object_id}.txt"
done

configure_submit() {
    local submit_id="\$1"
    local prefix="\$2"

    "${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" control "\${object_control}" > "\${tmpdir}/\${prefix}_attach_control.txt"
    "${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" pds "\${object_pds}" > "\${tmpdir}/\${prefix}_attach_pds.txt"
    "${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" use "\${object_use}" > "\${tmpdir}/\${prefix}_attach_use.txt"
    "${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" target "\${object_target}" > "\${tmpdir}/\${prefix}_attach_target.txt"
    "${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" status "\${object_status}" > "\${tmpdir}/\${prefix}_attach_status.txt"
    "${target_submit_op_bin}" "${target_node}" kick "\${submit_id}" ta 0x20 0x1 1 1 0 > "\${tmpdir}/\${prefix}_kick.txt"
    "${target_submit_op_bin}" "${target_node}" ccb "\${submit_id}" 0x20 0x0 0x1000 0x0 0xc0de0001 0xc0de0002 > "\${tmpdir}/\${prefix}_ccb.txt"
    "${target_submit_op_bin}" "${target_node}" queue "\${submit_id}" 0x20 0x40 0x20 1 > "\${tmpdir}/\${prefix}_queue.txt"
    "${target_submit_op_bin}" "${target_node}" status "\${submit_id}" 0x80 0xc0 0x4 0x5a500000 > "\${tmpdir}/\${prefix}_status.txt"
    "${target_submit_op_bin}" "${target_node}" sync "\${submit_id}" 0x20 0x40 0x41 0x1 > "\${tmpdir}/\${prefix}_sync.txt"
    "${target_submit_op_bin}" "${target_node}" owner "\${submit_id}" 0x40 queued 0x101 0x100 > "\${tmpdir}/\${prefix}_owner.txt"
    "${target_submit_op_bin}" "${target_node}" validate "\${submit_id}" > "\${tmpdir}/\${prefix}_validate.txt"
}

"${target_submit_op_bin}" "${target_node}" create pf > "\${tmpdir}/submit_a_create.txt"
submit_a_id=\$(awk -F= '/^submit_resolved_id=/ { print \$2 }' "\${tmpdir}/submit_a_create.txt" | tail -n 1)
if [[ -z "\${submit_a_id}" ]]; then
    echo "Failed to create submit A" >&2
    exit 1
fi
configure_submit "\${submit_a_id}" submit_a
"${target_submit_op_bin}" "${target_node}" issue "\${submit_a_id}" queued 1 > "\${tmpdir}/submit_a_issue.txt"

"${target_submit_op_bin}" "${target_node}" create pf > "\${tmpdir}/submit_b_create.txt"
submit_b_id=\$(awk -F= '/^submit_resolved_id=/ { print \$2 }' "\${tmpdir}/submit_b_create.txt" | tail -n 1)
if [[ -z "\${submit_b_id}" ]]; then
    echo "Failed to create submit B" >&2
    exit 1
fi
configure_submit "\${submit_b_id}" submit_b
"${target_submit_op_bin}" "${target_node}" focus "\${submit_b_id}" > "\${tmpdir}/submit_b_focus.txt"

"${target_query_bin}" "${target_node}" coord > "\${tmpdir}/coord_during.txt"
"${target_gtt_state_bin}" "${target_node}" > "\${tmpdir}/gtt_during.txt"
"${target_sgx_state_bin}" "${target_node}" capture > "\${tmpdir}/sgx_during.txt"
"${target_dump_bin}" "${target_node}" capture > "\${tmpdir}/dump_during.txt"
"${target_dump_bin}" "${target_node}" capture-json > "\${tmpdir}/dump_json_during.txt"
"${target_query_bin}" "${target_node}" focus > "\${tmpdir}/focus_during.txt"

grep -q '^coord_live_submit_count=2$' "\${tmpdir}/coord_during.txt"
grep -q '^coord_live_object_count=5$' "\${tmpdir}/coord_during.txt"
grep -q '^gtt_state_range_sgx_object_count=5$' "\${tmpdir}/gtt_during.txt"
grep -q '^sgx_state_ioctl_ret=0 errno=0$' "\${tmpdir}/sgx_during.txt"
grep -q '^coord_live_submit_count=2$' "\${tmpdir}/dump_during.txt"
grep -q '^coord_live_object_count=5$' "\${tmpdir}/dump_during.txt"
grep -q '^gtt_state_range_sgx_object_count=5$' "\${tmpdir}/dump_during.txt"
grep -q '^sgx_state_fault_count=0$' "\${tmpdir}/dump_during.txt"
grep -q '^object\[0\]_role_name=control$' "\${tmpdir}/dump_during.txt"
grep -q '^submit\[0\]_issue_count=1$' "\${tmpdir}/dump_during.txt"
grep -q '"live_submit_count": 2' "\${tmpdir}/dump_json_during.txt"
grep -q '"live_object_count": 5' "\${tmpdir}/dump_json_during.txt"
grep -q '"range_sgx_object_count": 5' "\${tmpdir}/dump_json_during.txt"
grep -q '"fault_count": 0' "\${tmpdir}/dump_json_during.txt"
grep -q '"role_name": "control"' "\${tmpdir}/dump_json_during.txt"
grep -q '"issue_count": 1' "\${tmpdir}/dump_json_during.txt"
grep -q "^submit_resolved_id=\${submit_b_id}\$" "\${tmpdir}/focus_during.txt"

for object_index in \$(seq 0 4); do
    "${target_object_query_bin}" "${target_node}" index "\${object_index}" > "\${tmpdir}/object_\${object_index}.txt"
done
for submit_index in 0 1; do
    "${target_query_bin}" "${target_node}" index "\${submit_index}" > "\${tmpdir}/submit_\${submit_index}.txt"
done

grep -q "^object_resolved_id=\${object_control}\$" "\${tmpdir}/object_0.txt"
grep -q "^object_resolved_id=\${object_status}\$" "\${tmpdir}/object_4.txt"
grep -q "^submit_resolved_id=\${submit_a_id}\$" "\${tmpdir}/submit_0.txt"
grep -q '^submit_issue_count=1$' "\${tmpdir}/submit_0.txt"
grep -q "^submit_resolved_id=\${submit_b_id}\$" "\${tmpdir}/submit_1.txt"
grep -q '^submit_issue_count=0$' "\${tmpdir}/submit_1.txt"

for pid in "\${probe_pids[@]}"; do
    wait "\${pid}"
done
probe_pids=()

"${target_query_bin}" "${target_node}" coord > "\${tmpdir}/coord_after_probes.txt"
"${target_object_query_bin}" "${target_node}" index 0 > "\${tmpdir}/object_0_after_probes.txt"
"${target_query_bin}" "${target_node}" index 0 > "\${tmpdir}/submit_0_after_probes.txt"
grep -q '^coord_live_submit_count=2$' "\${tmpdir}/coord_after_probes.txt"
grep -q '^coord_live_object_count=5$' "\${tmpdir}/coord_after_probes.txt"
grep -q "^object_resolved_id=\${object_control}\$" "\${tmpdir}/object_0_after_probes.txt"
grep -q "^submit_resolved_id=\${submit_a_id}\$" "\${tmpdir}/submit_0_after_probes.txt"

"${target_submit_op_bin}" "${target_node}" cleanup > "\${tmpdir}/submit_cleanup.txt"
"${target_object_op_bin}" "${target_node}" cleanup > "\${tmpdir}/object_cleanup.txt"
"${target_query_bin}" "${target_node}" coord > "\${tmpdir}/coord_after_cleanup.txt"
"${target_gtt_state_bin}" "${target_node}" > "\${tmpdir}/gtt_after_cleanup.txt"
"${target_dump_bin}" "${target_node}" cached > "\${tmpdir}/dump_after_cleanup.txt"
"${target_dump_bin}" "${target_node}" json > "\${tmpdir}/dump_json_after_cleanup.txt"

grep -q '^coord_live_submit_count=0$' "\${tmpdir}/coord_after_cleanup.txt"
grep -q '^coord_live_object_count=0$' "\${tmpdir}/coord_after_cleanup.txt"
grep -q '^gtt_state_range_sgx_object_count=0$' "\${tmpdir}/gtt_after_cleanup.txt"
grep -q '^coord_live_submit_count=0$' "\${tmpdir}/dump_after_cleanup.txt"
grep -q '^coord_live_object_count=0$' "\${tmpdir}/dump_after_cleanup.txt"
grep -q '^gtt_state_range_sgx_object_count=0$' "\${tmpdir}/dump_after_cleanup.txt"
grep -q '"live_submit_count": 0' "\${tmpdir}/dump_json_after_cleanup.txt"
grep -q '"live_object_count": 0' "\${tmpdir}/dump_json_after_cleanup.txt"
grep -q '"range_sgx_object_count": 0' "\${tmpdir}/dump_json_after_cleanup.txt"

echo "REMOTE_PROBE_BIN=${target_probe_bin}"
echo "REMOTE_RANGE_BIN=${target_range_bin}"
echo "REMOTE_OBJECT_OP_BIN=${target_object_op_bin}"
echo "REMOTE_OBJECT_QUERY_BIN=${target_object_query_bin}"
echo "REMOTE_SUBMIT_OP_BIN=${target_submit_op_bin}"
echo "REMOTE_QUERY_BIN=${target_query_bin}"
echo "REMOTE_SGX_STATE_BIN=${target_sgx_state_bin}"
echo "REMOTE_GTT_STATE_BIN=${target_gtt_state_bin}"
echo "REMOTE_DUMP_BIN=${target_dump_bin}"
echo "RANGE_IDS=\${range_ids[*]}"
echo "OBJECT_IDS=\${object_ids[*]}"
echo "SUBMIT_A_ID=\${submit_a_id}"
echo "SUBMIT_B_ID=\${submit_b_id}"
echo "COORD_DURING"
cat "\${tmpdir}/coord_during.txt"
echo "GTT_DURING"
cat "\${tmpdir}/gtt_during.txt"
echo "SGX_DURING"
cat "\${tmpdir}/sgx_during.txt"
echo "DUMP_DURING"
cat "\${tmpdir}/dump_during.txt"
echo "DUMP_JSON_DURING"
cat "\${tmpdir}/dump_json_during.txt"
echo "FOCUS_DURING"
cat "\${tmpdir}/focus_during.txt"
for object_index in \$(seq 0 4); do
    echo "OBJECT_INDEX_\${object_index}_DURING"
    cat "\${tmpdir}/object_\${object_index}.txt"
done
for submit_index in 0 1; do
    echo "SUBMIT_INDEX_\${submit_index}_DURING"
    cat "\${tmpdir}/submit_\${submit_index}.txt"
done
echo "COORD_AFTER_PROBES"
cat "\${tmpdir}/coord_after_probes.txt"
echo "OBJECT_INDEX_0_AFTER_PROBES"
cat "\${tmpdir}/object_0_after_probes.txt"
echo "SUBMIT_INDEX_0_AFTER_PROBES"
cat "\${tmpdir}/submit_0_after_probes.txt"
echo "SUBMIT_CLEANUP"
cat "\${tmpdir}/submit_cleanup.txt"
echo "OBJECT_CLEANUP"
cat "\${tmpdir}/object_cleanup.txt"
echo "COORD_AFTER_CLEANUP"
cat "\${tmpdir}/coord_after_cleanup.txt"
echo "GTT_AFTER_CLEANUP"
cat "\${tmpdir}/gtt_after_cleanup.txt"
echo "DUMP_AFTER_CLEANUP"
cat "\${tmpdir}/dump_after_cleanup.txt"
echo "DUMP_JSON_AFTER_CLEANUP"
cat "\${tmpdir}/dump_json_after_cleanup.txt"
EOF
