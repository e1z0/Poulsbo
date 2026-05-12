#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_dir="${TARGET_DIR:-/root/poulsbo-workloads}"
target_probe_bin="${TARGET_PROBE_BIN:-${target_dir}/drm_probe}"
target_range_bin="${TARGET_RANGE_BIN:-${target_dir}/psb_sgx_lab_range_query}"
target_object_op_bin="${TARGET_OBJECT_OP_BIN:-${target_dir}/psb_sgx_lab_object_op}"
target_submit_op_bin="${TARGET_SUBMIT_OP_BIN:-${target_dir}/psb_sgx_lab_submit_op}"
target_query_bin="${TARGET_QUERY_BIN:-${target_dir}/psb_sgx_lab_query}"
target_probe_stage="${TARGET_PROBE_STAGE:-/root/drm_probe.$$.$RANDOM.new}"
target_range_stage="${TARGET_RANGE_STAGE:-/root/psb_sgx_lab_range_query.$$.$RANDOM.new}"
target_object_op_stage="${TARGET_OBJECT_OP_STAGE:-/root/psb_sgx_lab_object_op.$$.$RANDOM.new}"
target_submit_op_stage="${TARGET_SUBMIT_OP_STAGE:-/root/psb_sgx_lab_submit_op.$$.$RANDOM.new}"
target_query_stage="${TARGET_QUERY_STAGE:-/root/psb_sgx_lab_query.$$.$RANDOM.new}"
local_probe_bin="${LOCAL_PROBE_BIN:-$(pwd)/workloads/drm_probe}"
local_range_bin="${LOCAL_RANGE_BIN:-$(pwd)/workloads/psb_sgx_lab_range_query}"
local_object_op_bin="${LOCAL_OBJECT_OP_BIN:-$(pwd)/workloads/psb_sgx_lab_object_op}"
local_submit_op_bin="${LOCAL_SUBMIT_OP_BIN:-$(pwd)/workloads/psb_sgx_lab_submit_op}"
local_query_bin="${LOCAL_QUERY_BIN:-$(pwd)/workloads/psb_sgx_lab_query}"
target_node="${TARGET_NODE:-/dev/dri/card0}"
hold_seconds="${HOLD_SECONDS:-20}"
probe_count="${PROBE_COUNT:-5}"
range_scan_max="${RANGE_SCAN_MAX:-128}"

for bin in "$local_probe_bin" "$local_range_bin" "$local_object_op_bin" \
    "$local_submit_op_bin" "$local_query_bin"; do
    if [[ ! -f "$bin" ]]; then
        echo "Local binary not found: $bin" >&2
        exit 1
    fi
done

scp "$local_probe_bin" "${target_user}@${target_host}:${target_probe_stage}"
scp "$local_range_bin" "${target_user}@${target_host}:${target_range_stage}"
scp "$local_object_op_bin" "${target_user}@${target_host}:${target_object_op_stage}"
scp "$local_submit_op_bin" "${target_user}@${target_host}:${target_submit_op_stage}"
scp "$local_query_bin" "${target_user}@${target_host}:${target_query_stage}"

ssh "${target_user}@${target_host}" "bash -s" <<EOF
set -euo pipefail

mkdir -p "${target_dir}"
install -m 0755 -T "${target_probe_stage}" "${target_probe_bin}"
install -m 0755 -T "${target_range_stage}" "${target_range_bin}"
install -m 0755 -T "${target_object_op_stage}" "${target_object_op_bin}"
install -m 0755 -T "${target_submit_op_stage}" "${target_submit_op_bin}"
install -m 0755 -T "${target_query_stage}" "${target_query_bin}"
rm -f "${target_probe_stage}" "${target_range_stage}" \
    "${target_object_op_stage}" "${target_submit_op_stage}" \
    "${target_query_stage}"

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
    local ta_dependency="\${3:-0}"

    "${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" control "\${object_control}" > "\${tmpdir}/\${prefix}_attach_control.txt"
    "${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" pds "\${object_pds}" > "\${tmpdir}/\${prefix}_attach_pds.txt"
    "${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" use "\${object_use}" > "\${tmpdir}/\${prefix}_attach_use.txt"
    "${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" target "\${object_target}" > "\${tmpdir}/\${prefix}_attach_target.txt"
    "${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" status "\${object_status}" > "\${tmpdir}/\${prefix}_attach_status.txt"
    "${target_submit_op_bin}" "${target_node}" kick "\${submit_id}" ta 0x20 0x1 1 1 "\${ta_dependency}" > "\${tmpdir}/\${prefix}_kick.txt"
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
    cat "\${tmpdir}/submit_a_create.txt" >&2
    exit 1
fi
configure_submit "\${submit_a_id}" submit_a
"${target_submit_op_bin}" "${target_node}" issue "\${submit_a_id}" queued 1 > "\${tmpdir}/submit_a_issue.txt"
"${target_query_bin}" "${target_node}" "\${submit_a_id}" > "\${tmpdir}/query_submit_a_after_issue.txt"

"${target_submit_op_bin}" "${target_node}" create pf > "\${tmpdir}/submit_b_create.txt"
submit_b_id=\$(awk -F= '/^submit_resolved_id=/ { print \$2 }' "\${tmpdir}/submit_b_create.txt" | tail -n 1)
if [[ -z "\${submit_b_id}" ]]; then
    echo "Failed to create submit B" >&2
    cat "\${tmpdir}/submit_b_create.txt" >&2
    exit 1
fi
configure_submit "\${submit_b_id}" submit_b
"${target_submit_op_bin}" "${target_node}" focus "\${submit_b_id}" > "\${tmpdir}/submit_b_focus.txt"
"${target_query_bin}" "${target_node}" "\${submit_a_id}" > "\${tmpdir}/query_submit_a_with_b_present.txt"
"${target_query_bin}" "${target_node}" "\${submit_b_id}" > "\${tmpdir}/query_submit_b_base.txt"
"${target_query_bin}" "${target_node}" focus > "\${tmpdir}/query_focus_base.txt"

submit_a_index=\$(awk -F= '/^submit_resolved_index=/ { print \$2 }' "\${tmpdir}/query_submit_a_with_b_present.txt")
submit_b_index=\$(awk -F= '/^submit_resolved_index=/ { print \$2 }' "\${tmpdir}/query_submit_b_base.txt")
submit_a_total=\$(awk -F= '/^submit_total_count=/ { print \$2 }' "\${tmpdir}/query_submit_a_with_b_present.txt")
submit_b_total=\$(awk -F= '/^submit_total_count=/ { print \$2 }' "\${tmpdir}/query_submit_b_base.txt")

if [[ -z "\${submit_a_index}" || -z "\${submit_b_index}" ]]; then
    echo "Failed to resolve submit indices" >&2
    exit 1
fi
if [[ "\${submit_a_total}" != "2" || "\${submit_b_total}" != "2" ]]; then
    echo "Unexpected submit totals: A=\${submit_a_total} B=\${submit_b_total}" >&2
    exit 1
fi
if [[ "\${submit_a_index}" == "\${submit_b_index}" ]]; then
    echo "Submit indices unexpectedly collided" >&2
    exit 1
fi

"${target_query_bin}" "${target_node}" index "\${submit_a_index}" > "\${tmpdir}/query_submit_index_a.txt"
"${target_query_bin}" "${target_node}" index "\${submit_b_index}" > "\${tmpdir}/query_submit_index_b.txt"

grep -q "^submit_resolved_id=\${submit_a_id}\$" "\${tmpdir}/query_submit_index_a.txt"
grep -q "^submit_resolved_index=\${submit_a_index}\$" "\${tmpdir}/query_submit_index_a.txt"
grep -q '^submit_issue_count=1$' "\${tmpdir}/query_submit_index_a.txt"
grep -q "^submit_resolved_id=\${submit_b_id}\$" "\${tmpdir}/query_submit_index_b.txt"
grep -q "^submit_resolved_index=\${submit_b_index}\$" "\${tmpdir}/query_submit_index_b.txt"
grep -q '^submit_issue_count=0$' "\${tmpdir}/query_submit_index_b.txt"
grep -q "^submit_resolved_id=\${submit_b_id}\$" "\${tmpdir}/query_focus_base.txt"
grep -q "^submit_resolved_index=\${submit_b_index}\$" "\${tmpdir}/query_focus_base.txt"

for pid in "\${probe_pids[@]}"; do
    wait "\${pid}"
done
probe_pids=()

"${target_query_bin}" "${target_node}" "\${submit_a_id}" > "\${tmpdir}/query_submit_a_after_probes.txt"
"${target_query_bin}" "${target_node}" "\${submit_b_id}" > "\${tmpdir}/query_submit_b_after_probes.txt"
"${target_query_bin}" "${target_node}" index "\${submit_a_index}" > "\${tmpdir}/query_submit_index_a_after_probes.txt"
"${target_query_bin}" "${target_node}" index "\${submit_b_index}" > "\${tmpdir}/query_submit_index_b_after_probes.txt"

"${target_submit_op_bin}" "${target_node}" cleanup > "\${tmpdir}/submit_cleanup.txt"
"${target_object_op_bin}" "${target_node}" cleanup > "\${tmpdir}/object_cleanup.txt"

set +e
"${target_query_bin}" "${target_node}" "\${submit_a_id}" > "\${tmpdir}/query_submit_a_after_cleanup.txt" 2>&1
query_submit_a_after_cleanup_rc=\$?
"${target_query_bin}" "${target_node}" "\${submit_b_id}" > "\${tmpdir}/query_submit_b_after_cleanup.txt" 2>&1
query_submit_b_after_cleanup_rc=\$?
"${target_query_bin}" "${target_node}" index 0 > "\${tmpdir}/query_submit_index_0_after_cleanup.txt" 2>&1
query_submit_index_0_after_cleanup_rc=\$?
"${target_query_bin}" "${target_node}" focus > "\${tmpdir}/query_focus_after_cleanup.txt" 2>&1
query_focus_after_cleanup_rc=\$?
set -e

if [[ "\${query_submit_a_after_cleanup_rc}" -eq 0 || \
      "\${query_submit_b_after_cleanup_rc}" -eq 0 || \
      "\${query_submit_index_0_after_cleanup_rc}" -eq 0 || \
      "\${query_focus_after_cleanup_rc}" -eq 0 ]]; then
    echo "Submit query unexpectedly succeeded after typed cleanup" >&2
    exit 1
fi

echo "REMOTE_PROBE_BIN=${target_probe_bin}"
echo "REMOTE_RANGE_BIN=${target_range_bin}"
echo "REMOTE_OBJECT_OP_BIN=${target_object_op_bin}"
echo "REMOTE_SUBMIT_OP_BIN=${target_submit_op_bin}"
echo "REMOTE_QUERY_BIN=${target_query_bin}"
echo "RANGE_IDS=\${range_ids[*]}"
echo "OBJECT_IDS=\${object_ids[*]}"
echo "SUBMIT_A_ID=\${submit_a_id}"
echo "SUBMIT_A_INDEX=\${submit_a_index}"
echo "SUBMIT_B_ID=\${submit_b_id}"
echo "SUBMIT_B_INDEX=\${submit_b_index}"
echo "SUBMIT_A_QUERY_AFTER_ISSUE"
cat "\${tmpdir}/query_submit_a_after_issue.txt"
echo "SUBMIT_A_QUERY_WITH_B_PRESENT"
cat "\${tmpdir}/query_submit_a_with_b_present.txt"
echo "SUBMIT_B_QUERY_BASE"
cat "\${tmpdir}/query_submit_b_base.txt"
echo "FOCUS_QUERY_BASE"
cat "\${tmpdir}/query_focus_base.txt"
echo "SUBMIT_QUERY_INDEX_A"
cat "\${tmpdir}/query_submit_index_a.txt"
echo "SUBMIT_QUERY_INDEX_B"
cat "\${tmpdir}/query_submit_index_b.txt"
echo "SUBMIT_QUERY_A_AFTER_PROBES"
cat "\${tmpdir}/query_submit_a_after_probes.txt"
echo "SUBMIT_QUERY_B_AFTER_PROBES"
cat "\${tmpdir}/query_submit_b_after_probes.txt"
echo "SUBMIT_QUERY_INDEX_A_AFTER_PROBES"
cat "\${tmpdir}/query_submit_index_a_after_probes.txt"
echo "SUBMIT_QUERY_INDEX_B_AFTER_PROBES"
cat "\${tmpdir}/query_submit_index_b_after_probes.txt"
echo "SUBMIT_CLEANUP"
cat "\${tmpdir}/submit_cleanup.txt"
echo "OBJECT_CLEANUP"
cat "\${tmpdir}/object_cleanup.txt"
echo "SUBMIT_QUERY_A_AFTER_CLEANUP_RC=\${query_submit_a_after_cleanup_rc}"
cat "\${tmpdir}/query_submit_a_after_cleanup.txt"
echo "SUBMIT_QUERY_B_AFTER_CLEANUP_RC=\${query_submit_b_after_cleanup_rc}"
cat "\${tmpdir}/query_submit_b_after_cleanup.txt"
echo "SUBMIT_QUERY_INDEX_0_AFTER_CLEANUP_RC=\${query_submit_index_0_after_cleanup_rc}"
cat "\${tmpdir}/query_submit_index_0_after_cleanup.txt"
echo "FOCUS_QUERY_AFTER_CLEANUP_RC=\${query_focus_after_cleanup_rc}"
cat "\${tmpdir}/query_focus_after_cleanup.txt"
EOF
