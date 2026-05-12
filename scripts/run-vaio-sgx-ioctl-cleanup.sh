#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_dir="${TARGET_DIR:-/root/poulsbo-workloads}"
target_probe_bin="${TARGET_PROBE_BIN:-${target_dir}/drm_probe}"
target_range_bin="${TARGET_RANGE_BIN:-${target_dir}/psb_sgx_lab_range_query}"
target_submit_bin="${TARGET_SUBMIT_BIN:-${target_dir}/psb_sgx_lab_submit_op}"
target_object_bin="${TARGET_OBJECT_BIN:-${target_dir}/psb_sgx_lab_object_op}"
target_query_bin="${TARGET_QUERY_BIN:-${target_dir}/psb_sgx_lab_query}"
target_object_query_bin="${TARGET_OBJECT_QUERY_BIN:-${target_dir}/psb_sgx_lab_object_query}"
target_dump_bin="${TARGET_DUMP_BIN:-${target_dir}/psb_sgx_lab_dump}"
remote_lock_file="${REMOTE_LOCK_FILE:-/run/poulsbo-sgx-lab.lock}"
remote_lock_wait_secs="${REMOTE_LOCK_WAIT_SECS:-600}"

target_probe_stage="${TARGET_PROBE_STAGE:-/root/drm_probe.$$.$RANDOM.new}"
target_range_stage="${TARGET_RANGE_STAGE:-/root/psb_sgx_lab_range_query.$$.$RANDOM.new}"
target_submit_stage="${TARGET_SUBMIT_STAGE:-/root/psb_sgx_lab_submit_op.$$.$RANDOM.new}"
target_object_stage="${TARGET_OBJECT_STAGE:-/root/psb_sgx_lab_object_op.$$.$RANDOM.new}"
target_query_stage="${TARGET_QUERY_STAGE:-/root/psb_sgx_lab_query.$$.$RANDOM.new}"
target_object_query_stage="${TARGET_OBJECT_QUERY_STAGE:-/root/psb_sgx_lab_object_query.$$.$RANDOM.new}"
target_dump_stage="${TARGET_DUMP_STAGE:-/root/psb_sgx_lab_dump.$$.$RANDOM.new}"

local_probe_bin="${LOCAL_PROBE_BIN:-$(pwd)/workloads/drm_probe}"
local_range_bin="${LOCAL_RANGE_BIN:-$(pwd)/workloads/psb_sgx_lab_range_query}"
local_submit_bin="${LOCAL_SUBMIT_BIN:-$(pwd)/workloads/psb_sgx_lab_submit_op}"
local_object_bin="${LOCAL_OBJECT_BIN:-$(pwd)/workloads/psb_sgx_lab_object_op}"
local_query_bin="${LOCAL_QUERY_BIN:-$(pwd)/workloads/psb_sgx_lab_query}"
local_object_query_bin="${LOCAL_OBJECT_QUERY_BIN:-$(pwd)/workloads/psb_sgx_lab_object_query}"
local_dump_bin="${LOCAL_DUMP_BIN:-$(pwd)/workloads/psb_sgx_lab_dump}"

target_node="${TARGET_NODE:-/dev/dri/card0}"
hold_seconds="${HOLD_SECONDS:-20}"
range_scan_max="${RANGE_SCAN_MAX:-128}"

if [[ "${remote_lock_wait_secs}" -lt 1 ]]; then
    echo "REMOTE_LOCK_WAIT_SECS must be at least 1" >&2
    exit 1
fi

for bin in "$local_probe_bin" "$local_range_bin" "$local_submit_bin" \
    "$local_object_bin" "$local_query_bin" "$local_object_query_bin" \
    "$local_dump_bin"; do
    if [[ ! -f "$bin" ]]; then
        echo "Local binary not found: $bin" >&2
        exit 1
    fi
done

scp "$local_probe_bin" "${target_user}@${target_host}:${target_probe_stage}"
scp "$local_range_bin" "${target_user}@${target_host}:${target_range_stage}"
scp "$local_submit_bin" "${target_user}@${target_host}:${target_submit_stage}"
scp "$local_object_bin" "${target_user}@${target_host}:${target_object_stage}"
scp "$local_query_bin" "${target_user}@${target_host}:${target_query_stage}"
scp "$local_object_query_bin" "${target_user}@${target_host}:${target_object_query_stage}"
scp "$local_dump_bin" "${target_user}@${target_host}:${target_dump_stage}"

ssh "${target_user}@${target_host}" "bash -s" <<EOF
set -euo pipefail
remote_lock_file="${remote_lock_file}"
remote_lock_wait_secs="${remote_lock_wait_secs}"

if ! command -v flock >/dev/null 2>&1; then
    echo "Target is missing flock(1)" >&2
    exit 1
fi

exec 9>"\${remote_lock_file}"
if ! flock -w "\${remote_lock_wait_secs}" 9; then
    echo "Timed out waiting for remote SGX lab lock: \${remote_lock_file}" >&2
    exit 1
fi

mkdir -p "${target_dir}"
install -m 0755 -T "${target_probe_stage}" "${target_probe_bin}"
install -m 0755 -T "${target_range_stage}" "${target_range_bin}"
install -m 0755 -T "${target_submit_stage}" "${target_submit_bin}"
install -m 0755 -T "${target_object_stage}" "${target_object_bin}"
install -m 0755 -T "${target_query_stage}" "${target_query_bin}"
install -m 0755 -T "${target_object_query_stage}" "${target_object_query_bin}"
install -m 0755 -T "${target_dump_stage}" "${target_dump_bin}"
rm -f "${target_probe_stage}" "${target_range_stage}" "${target_submit_stage}" \
    "${target_object_stage}" "${target_query_stage}" \
    "${target_object_query_stage}" "${target_dump_stage}"

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

"${target_submit_bin}" "${target_node}" cleanup >/dev/null 2>&1 || true
"${target_object_bin}" "${target_node}" cleanup >/dev/null 2>&1 || true

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

object_a_out="\$("${target_object_bin}" "${target_node}" adopt "\${range_id_a}")"
object_a_id=\$(printf '%s\n' "\${object_a_out}" | awk -F= '/^object_resolved_id=/ { print \$2 }')
echo "OBJECT_A_CREATE"
printf '%s\n' "\${object_a_out}"

object_b_out="\$("${target_object_bin}" "${target_node}" adopt "\${range_id_b}")"
object_b_id=\$(printf '%s\n' "\${object_b_out}" | awk -F= '/^object_resolved_id=/ { print \$2 }')
echo "OBJECT_B_CREATE"
printf '%s\n' "\${object_b_out}"

if [[ -z "\${object_a_id}" || "\${object_a_id}" == "0" || \
      -z "\${object_b_id}" || "\${object_b_id}" == "0" ]]; then
    echo "Failed to resolve object IDs" >&2
    exit 1
fi

"${target_object_bin}" "${target_node}" context "\${object_a_id}" pf > "\${tmpdir}/object_a_context.txt"
"${target_object_bin}" "${target_node}" gpu_use "\${object_a_id}" control_ccb > "\${tmpdir}/object_a_gpu_use.txt"
"${target_object_bin}" "${target_node}" pf_bind "\${object_a_id}" > "\${tmpdir}/object_a_pf_bind.txt"
echo "OBJECT_A_CONTEXT"
cat "\${tmpdir}/object_a_context.txt"
echo "OBJECT_A_GPU_USE"
cat "\${tmpdir}/object_a_gpu_use.txt"
echo "OBJECT_A_PF_BIND"
cat "\${tmpdir}/object_a_pf_bind.txt"

submit_a_out="\$("${target_submit_bin}" "${target_node}" create pf)"
submit_a_id=\$(printf '%s\n' "\${submit_a_out}" | awk -F= '/^submit_resolved_id=/ { print \$2 }')
echo "SUBMIT_A_CREATE"
printf '%s\n' "\${submit_a_out}"

submit_b_out="\$("${target_submit_bin}" "${target_node}" create pf)"
submit_b_id=\$(printf '%s\n' "\${submit_b_out}" | awk -F= '/^submit_resolved_id=/ { print \$2 }')
echo "SUBMIT_B_CREATE"
printf '%s\n' "\${submit_b_out}"

if [[ -z "\${submit_a_id}" || "\${submit_a_id}" == "0" || \
      -z "\${submit_b_id}" || "\${submit_b_id}" == "0" ]]; then
    echo "Failed to resolve submit IDs" >&2
    exit 1
fi

focus_out="\$("${target_submit_bin}" "${target_node}" focus "\${submit_a_id}")"
echo "SUBMIT_FOCUS"
printf '%s\n' "\${focus_out}"

"${target_query_bin}" "${target_node}" coord > "\${tmpdir}/coord_before_cleanup.txt"
"${target_query_bin}" "${target_node}" index 0 > "\${tmpdir}/submit_index_0_before_cleanup.txt"
"${target_query_bin}" "${target_node}" index 1 > "\${tmpdir}/submit_index_1_before_cleanup.txt"
"${target_object_query_bin}" "${target_node}" id "\${object_a_id}" > "\${tmpdir}/object_a_before_cleanup.txt"
"${target_object_query_bin}" "${target_node}" id "\${object_b_id}" > "\${tmpdir}/object_b_before_cleanup.txt"
"${target_dump_bin}" "${target_node}" cached > "\${tmpdir}/dump_before_cleanup.txt"

grep -q '^coord_live_submit_count=2$' "\${tmpdir}/coord_before_cleanup.txt"
grep -q '^coord_live_object_count=2$' "\${tmpdir}/coord_before_cleanup.txt"
grep -q '^coord_focus_valid=1$' "\${tmpdir}/coord_before_cleanup.txt"
grep -q "^coord_focused_submit_id=\${submit_a_id}\$" "\${tmpdir}/coord_before_cleanup.txt"
grep -q "^submit_resolved_id=\${submit_a_id}\$" "\${tmpdir}/submit_index_0_before_cleanup.txt"
grep -q "^submit_resolved_id=\${submit_b_id}\$" "\${tmpdir}/submit_index_1_before_cleanup.txt"
grep -q "^object_resolved_id=\${object_a_id}\$" "\${tmpdir}/object_a_before_cleanup.txt"
grep -q "^object_resolved_id=\${object_b_id}\$" "\${tmpdir}/object_b_before_cleanup.txt"
grep -q '^coord_live_submit_count=2$' "\${tmpdir}/dump_before_cleanup.txt"
grep -q '^coord_live_object_count=2$' "\${tmpdir}/dump_before_cleanup.txt"
grep -q '^gtt_state_range_sgx_object_count=2$' "\${tmpdir}/dump_before_cleanup.txt"
grep -q '^gtt_state_range_pf_bound_count=1$' "\${tmpdir}/dump_before_cleanup.txt"

echo "COORD_BEFORE_CLEANUP"
cat "\${tmpdir}/coord_before_cleanup.txt"
echo "SUBMIT_INDEX_0_BEFORE_CLEANUP"
cat "\${tmpdir}/submit_index_0_before_cleanup.txt"
echo "SUBMIT_INDEX_1_BEFORE_CLEANUP"
cat "\${tmpdir}/submit_index_1_before_cleanup.txt"
echo "OBJECT_A_BEFORE_CLEANUP"
cat "\${tmpdir}/object_a_before_cleanup.txt"
echo "OBJECT_B_BEFORE_CLEANUP"
cat "\${tmpdir}/object_b_before_cleanup.txt"
echo "DUMP_BEFORE_CLEANUP"
cat "\${tmpdir}/dump_before_cleanup.txt"

submit_cleanup_out="\$("${target_submit_bin}" "${target_node}" cleanup)"
echo "SUBMIT_CLEANUP"
printf '%s\n' "\${submit_cleanup_out}"

"${target_query_bin}" "${target_node}" coord > "\${tmpdir}/coord_after_submit_cleanup.txt"
"${target_query_bin}" "${target_node}" focus > "\${tmpdir}/submit_focus_after_submit_cleanup.txt" 2>&1 || true
"${target_query_bin}" "${target_node}" index 0 > "\${tmpdir}/submit_index_0_after_submit_cleanup.txt" 2>&1 || true

grep -q '^submit_result_count=2$' <<< "\${submit_cleanup_out}"
grep -q '^coord_focus_valid=0$' "\${tmpdir}/coord_after_submit_cleanup.txt"
grep -q '^coord_focused_submit_id=0$' "\${tmpdir}/coord_after_submit_cleanup.txt"
grep -q '^coord_live_submit_count=0$' "\${tmpdir}/coord_after_submit_cleanup.txt"
grep -q '^submit_ioctl_ret=-1 errno=2$' "\${tmpdir}/submit_focus_after_submit_cleanup.txt"
grep -q '^submit_ioctl_ret=-1 errno=2$' "\${tmpdir}/submit_index_0_after_submit_cleanup.txt"

echo "COORD_AFTER_SUBMIT_CLEANUP"
cat "\${tmpdir}/coord_after_submit_cleanup.txt"
echo "SUBMIT_FOCUS_AFTER_SUBMIT_CLEANUP"
cat "\${tmpdir}/submit_focus_after_submit_cleanup.txt"
echo "SUBMIT_INDEX_0_AFTER_SUBMIT_CLEANUP"
cat "\${tmpdir}/submit_index_0_after_submit_cleanup.txt"

object_cleanup_out="\$("${target_object_bin}" "${target_node}" cleanup)"
echo "OBJECT_CLEANUP"
printf '%s\n' "\${object_cleanup_out}"

"${target_object_query_bin}" "${target_node}" id "\${object_a_id}" > "\${tmpdir}/object_a_after_object_cleanup.txt" 2>&1 || true
"${target_object_query_bin}" "${target_node}" id "\${object_b_id}" > "\${tmpdir}/object_b_after_object_cleanup.txt" 2>&1 || true
"${target_dump_bin}" "${target_node}" cached > "\${tmpdir}/dump_after_object_cleanup.txt"

grep -q '^object_result_count=2$' <<< "\${object_cleanup_out}"
grep -q '^object_ioctl_ret=-1 errno=2$' "\${tmpdir}/object_a_after_object_cleanup.txt"
grep -q '^object_ioctl_ret=-1 errno=2$' "\${tmpdir}/object_b_after_object_cleanup.txt"
grep -q '^coord_live_submit_count=0$' "\${tmpdir}/dump_after_object_cleanup.txt"
grep -q '^coord_live_object_count=0$' "\${tmpdir}/dump_after_object_cleanup.txt"
grep -q '^gtt_state_range_sgx_object_count=0$' "\${tmpdir}/dump_after_object_cleanup.txt"
grep -q '^gtt_state_range_pf_bound_count=0$' "\${tmpdir}/dump_after_object_cleanup.txt"

echo "OBJECT_A_AFTER_OBJECT_CLEANUP"
cat "\${tmpdir}/object_a_after_object_cleanup.txt"
echo "OBJECT_B_AFTER_OBJECT_CLEANUP"
cat "\${tmpdir}/object_b_after_object_cleanup.txt"
echo "DUMP_AFTER_OBJECT_CLEANUP"
cat "\${tmpdir}/dump_after_object_cleanup.txt"

echo "RANGE_A_AFTER_CLEANUP"
"${target_range_bin}" "${target_node}" id "\${range_id_a}" gem > "\${tmpdir}/range_a_after_cleanup.txt"
cat "\${tmpdir}/range_a_after_cleanup.txt"
echo "RANGE_B_AFTER_CLEANUP"
"${target_range_bin}" "${target_node}" id "\${range_id_b}" gem > "\${tmpdir}/range_b_after_cleanup.txt"
cat "\${tmpdir}/range_b_after_cleanup.txt"

grep -q '^range_sgx_object_id=0$' "\${tmpdir}/range_a_after_cleanup.txt"
grep -q '^range_pf_bind_count=0$' "\${tmpdir}/range_a_after_cleanup.txt"
grep -q '^range_sgx_object_id=0$' "\${tmpdir}/range_b_after_cleanup.txt"
grep -q '^range_pf_bind_count=0$' "\${tmpdir}/range_b_after_cleanup.txt"
range_a_bind_total=\$(awk -F= '/^range_pf_bind_total=/{ print \$2; exit }' \
    "\${tmpdir}/range_a_after_cleanup.txt")
range_a_unbind_total=\$(awk -F= '/^range_pf_unbind_total=/{ print \$2; exit }' \
    "\${tmpdir}/range_a_after_cleanup.txt")
if [[ -z "\${range_a_bind_total}" || "\${range_a_bind_total}" -lt 1 ]]; then
    echo "Range A bind total did not remain nonzero after cleanup" >&2
    cat "\${tmpdir}/range_a_after_cleanup.txt" >&2
    exit 1
fi
if [[ -z "\${range_a_unbind_total}" || "\${range_a_unbind_total}" -lt 1 ]]; then
    echo "Range A unbind total did not remain nonzero after cleanup" >&2
    cat "\${tmpdir}/range_a_after_cleanup.txt" >&2
    exit 1
fi

for pid in "\${probe_pids[@]}"; do
    wait "\${pid}"
done
probe_pids=()
EOF
