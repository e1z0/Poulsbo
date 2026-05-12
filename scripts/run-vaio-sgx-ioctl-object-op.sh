#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_dir="${TARGET_DIR:-/root/poulsbo-workloads}"

target_probe_bin="${TARGET_PROBE_BIN:-${target_dir}/drm_probe}"
target_op_bin="${TARGET_OP_BIN:-${target_dir}/psb_sgx_lab_object_op}"
target_object_query_bin="${TARGET_OBJECT_QUERY_BIN:-${target_dir}/psb_sgx_lab_object_query}"
target_range_query_bin="${TARGET_RANGE_QUERY_BIN:-${target_dir}/psb_sgx_lab_range_query}"
target_gtt_state_bin="${TARGET_GTT_STATE_BIN:-${target_dir}/psb_sgx_lab_gtt_state_query}"
remote_lock_file="${REMOTE_LOCK_FILE:-/run/poulsbo-sgx-lab.lock}"
remote_lock_wait_secs="${REMOTE_LOCK_WAIT_SECS:-600}"

target_probe_stage="${TARGET_PROBE_STAGE:-/root/drm_probe.$$.$RANDOM.new}"
target_op_stage="${TARGET_OP_STAGE:-/root/psb_sgx_lab_object_op.$$.$RANDOM.new}"
target_object_query_stage="${TARGET_OBJECT_QUERY_STAGE:-/root/psb_sgx_lab_object_query.$$.$RANDOM.new}"
target_range_query_stage="${TARGET_RANGE_QUERY_STAGE:-/root/psb_sgx_lab_range_query.$$.$RANDOM.new}"
target_gtt_state_stage="${TARGET_GTT_STATE_STAGE:-/root/psb_sgx_lab_gtt_state_query.$$.$RANDOM.new}"

local_probe_bin="${LOCAL_PROBE_BIN:-$(pwd)/workloads/drm_probe}"
local_op_bin="${LOCAL_OP_BIN:-$(pwd)/workloads/psb_sgx_lab_object_op}"
local_object_query_bin="${LOCAL_OBJECT_QUERY_BIN:-$(pwd)/workloads/psb_sgx_lab_object_query}"
local_range_query_bin="${LOCAL_RANGE_QUERY_BIN:-$(pwd)/workloads/psb_sgx_lab_range_query}"
local_gtt_state_bin="${LOCAL_GTT_STATE_BIN:-$(pwd)/workloads/psb_sgx_lab_gtt_state_query}"

target_node="${TARGET_NODE:-/dev/dri/card0}"
hold_seconds="${HOLD_SECONDS:-15}"
range_scan_max="${RANGE_SCAN_MAX:-128}"

if [[ "${remote_lock_wait_secs}" -lt 1 ]]; then
    echo "REMOTE_LOCK_WAIT_SECS must be at least 1" >&2
    exit 1
fi

for bin in "$local_probe_bin" "$local_op_bin" "$local_object_query_bin" \
    "$local_range_query_bin" "$local_gtt_state_bin"; do
    if [[ ! -f "$bin" ]]; then
        echo "Local binary not found: $bin" >&2
        exit 1
    fi
done

scp "$local_probe_bin" "${target_user}@${target_host}:${target_probe_stage}"
scp "$local_op_bin" "${target_user}@${target_host}:${target_op_stage}"
scp "$local_object_query_bin" "${target_user}@${target_host}:${target_object_query_stage}"
scp "$local_range_query_bin" "${target_user}@${target_host}:${target_range_query_stage}"
scp "$local_gtt_state_bin" "${target_user}@${target_host}:${target_gtt_state_stage}"

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
install -m 0755 -T "${target_op_stage}" "${target_op_bin}"
install -m 0755 -T "${target_object_query_stage}" "${target_object_query_bin}"
install -m 0755 -T "${target_range_query_stage}" "${target_range_query_bin}"
install -m 0755 -T "${target_gtt_state_stage}" "${target_gtt_state_bin}"
rm -f "${target_probe_stage}" "${target_op_stage}" \
    "${target_object_query_stage}" "${target_range_query_stage}" \
    "${target_gtt_state_stage}"

tmpdir=\$(mktemp -d)
probe_pid=""
range_id=""
object_id=""
saved_object_id=""

cleanup() {
    if [[ -n "\${object_id}" ]]; then
        "${target_op_bin}" "${target_node}" drop "\${object_id}" >/dev/null 2>&1 || true
    fi
    if [[ -n "\${probe_pid}" ]] && kill -0 "\${probe_pid}" 2>/dev/null; then
        kill "\${probe_pid}" 2>/dev/null || true
        wait "\${probe_pid}" 2>/dev/null || true
    fi
    rm -rf "\${tmpdir}"
}
trap cleanup EXIT

"${target_op_bin}" "${target_node}" cleanup >/dev/null 2>&1 || true

"${target_gtt_state_bin}" "${target_node}" > "\${tmpdir}/gtt_before.txt"
"${target_probe_bin}" "${target_node}" "${hold_seconds}" > "\${tmpdir}/drm_probe_hold.log" 2>&1 &
probe_pid=\$!

for _ in \$(seq 1 $((hold_seconds + 5))); do
    if grep -q '^drm_hold_seconds=' "\${tmpdir}/drm_probe_hold.log" 2>/dev/null; then
        break
    fi
    if ! kill -0 "\${probe_pid}" 2>/dev/null; then
        break
    fi
    sleep 1
done

for range_index in \$(seq 0 "${range_scan_max}"); do
    range_out="\$("${target_range_query_bin}" "${target_node}" index "\${range_index}" gem 2>/dev/null || true)"
    [[ -n "\${range_out}" ]] || break
    range_id_candidate=\$(printf '%s\n' "\${range_out}" | awk -F= '/^range_resolved_id=/{ print \$2 }')
    range_mmapping=\$(printf '%s\n' "\${range_out}" | awk -F= '/^range_mmapping=/{ print \$2 }')
    range_stolen=\$(printf '%s\n' "\${range_out}" | awk -F= '/^range_stolen=/{ print \$2 }')
    range_sgx_object_id=\$(printf '%s\n' "\${range_out}" | awk -F= '/^range_sgx_object_id=/{ print \$2 }')
    if [[ "\${range_mmapping}" == "1" && "\${range_stolen}" == "0" && "\${range_sgx_object_id}" == "0" ]]; then
        range_id="\${range_id_candidate}"
        printf '%s\n' "\${range_out}" > "\${tmpdir}/range_before_adopt.txt"
        break
    fi
done

if [[ -z "\${range_id}" ]]; then
    echo "Failed to identify a live GEM range to adopt" >&2
    cat "\${tmpdir}/drm_probe_hold.log" >&2
    exit 1
fi

"${target_op_bin}" "${target_node}" adopt "\${range_id}" > "\${tmpdir}/object_op_adopt.txt"
object_id=\$(awk -F= '/^object_resolved_id=/{ print \$2; exit }' "\${tmpdir}/object_op_adopt.txt")
if [[ -z "\${object_id}" || "\${object_id}" == "0" ]]; then
    echo "Failed to resolve adopted object ID" >&2
    cat "\${tmpdir}/object_op_adopt.txt" >&2
    exit 1
fi
saved_object_id="\${object_id}"

"${target_object_query_bin}" "${target_node}" id "\${object_id}" > "\${tmpdir}/object_after_adopt.txt"
"${target_range_query_bin}" "${target_node}" id "\${range_id}" gem > "\${tmpdir}/range_after_adopt.txt"
"${target_gtt_state_bin}" "${target_node}" > "\${tmpdir}/gtt_after_adopt.txt"

grep -q '^object_role_name=generic$' "\${tmpdir}/object_after_adopt.txt"
grep -q '^object_class_name=generic$' "\${tmpdir}/object_after_adopt.txt"
grep -q '^object_gpu_use_name=generic$' "\${tmpdir}/object_after_adopt.txt"
grep -q '^object_expected_context_name=auto$' "\${tmpdir}/object_after_adopt.txt"
grep -q '^object_pf_bind_count=0$' "\${tmpdir}/object_after_adopt.txt"
grep -q "^range_sgx_object_id=\${object_id}\$" "\${tmpdir}/range_after_adopt.txt"
grep -q '^range_pf_bind_count=0$' "\${tmpdir}/range_after_adopt.txt"
grep -q '^gtt_state_range_sgx_object_count=1$' "\${tmpdir}/gtt_after_adopt.txt"
grep -q '^gtt_state_range_pf_bound_count=0$' "\${tmpdir}/gtt_after_adopt.txt"

"${target_op_bin}" "${target_node}" role "\${object_id}" control > "\${tmpdir}/object_op_role.txt"
"${target_op_bin}" "${target_node}" class "\${object_id}" packet > "\${tmpdir}/object_op_class.txt"
"${target_op_bin}" "${target_node}" gpu_use "\${object_id}" control_ccb > "\${tmpdir}/object_op_gpu_use.txt"
"${target_op_bin}" "${target_node}" context "\${object_id}" pf > "\${tmpdir}/object_op_context.txt"
"${target_object_query_bin}" "${target_node}" id "\${object_id}" > "\${tmpdir}/object_after_meta.txt"

grep -q '^object_role_name=control$' "\${tmpdir}/object_after_meta.txt"
grep -q '^object_class_name=packet$' "\${tmpdir}/object_after_meta.txt"
grep -q '^object_gpu_use_name=control_ccb$' "\${tmpdir}/object_after_meta.txt"
grep -q '^object_expected_context_name=pf$' "\${tmpdir}/object_after_meta.txt"

"${target_op_bin}" "${target_node}" pf_bind "\${object_id}" > "\${tmpdir}/object_op_pf_bind.txt"
"${target_object_query_bin}" "${target_node}" id "\${object_id}" > "\${tmpdir}/object_after_bind.txt"
"${target_range_query_bin}" "${target_node}" id "\${range_id}" gem > "\${tmpdir}/range_after_bind.txt"
"${target_gtt_state_bin}" "${target_node}" > "\${tmpdir}/gtt_after_bind.txt"

grep -q '^object_pf_bind_count=1$' "\${tmpdir}/object_after_bind.txt"
grep -q "^range_sgx_object_id=\${object_id}\$" "\${tmpdir}/range_after_bind.txt"
grep -q '^range_pf_bind_count=1$' "\${tmpdir}/range_after_bind.txt"
grep -q '^gtt_state_range_sgx_object_count=1$' "\${tmpdir}/gtt_after_bind.txt"
grep -q '^gtt_state_range_pf_bound_count=1$' "\${tmpdir}/gtt_after_bind.txt"

"${target_op_bin}" "${target_node}" pf_unbind "\${object_id}" > "\${tmpdir}/object_op_pf_unbind.txt"
"${target_object_query_bin}" "${target_node}" id "\${object_id}" > "\${tmpdir}/object_after_unbind.txt"
"${target_range_query_bin}" "${target_node}" id "\${range_id}" gem > "\${tmpdir}/range_after_unbind.txt"
"${target_gtt_state_bin}" "${target_node}" > "\${tmpdir}/gtt_after_unbind.txt"

grep -q '^object_pf_bind_count=0$' "\${tmpdir}/object_after_unbind.txt"
grep -q "^range_sgx_object_id=\${object_id}\$" "\${tmpdir}/range_after_unbind.txt"
grep -q '^range_pf_bind_count=0$' "\${tmpdir}/range_after_unbind.txt"
grep -q '^gtt_state_range_sgx_object_count=1$' "\${tmpdir}/gtt_after_unbind.txt"
grep -q '^gtt_state_range_pf_bound_count=0$' "\${tmpdir}/gtt_after_unbind.txt"

wait "\${probe_pid}"
probe_pid=""

"${target_object_query_bin}" "${target_node}" id "\${object_id}" > "\${tmpdir}/object_after_probe_exit.txt"
grep -q "^object_resolved_id=\${object_id}\$" "\${tmpdir}/object_after_probe_exit.txt"

"${target_op_bin}" "${target_node}" drop "\${object_id}" > "\${tmpdir}/object_op_drop.txt"
"${target_object_query_bin}" "${target_node}" id "\${object_id}" > "\${tmpdir}/object_after_drop.txt" 2>&1 || true
"${target_range_query_bin}" "${target_node}" id "\${range_id}" gem > "\${tmpdir}/range_after_drop.txt"
"${target_gtt_state_bin}" "${target_node}" > "\${tmpdir}/gtt_after_drop.txt"
object_id=""

grep -q '^object_ioctl_ret=-1 errno=2$' "\${tmpdir}/object_after_drop.txt"
grep -q '^range_sgx_object_id=0$' "\${tmpdir}/range_after_drop.txt"
grep -q '^range_pf_bind_count=0$' "\${tmpdir}/range_after_drop.txt"
grep -q '^gtt_state_range_sgx_object_count=0$' "\${tmpdir}/gtt_after_drop.txt"
grep -q '^gtt_state_range_pf_bound_count=0$' "\${tmpdir}/gtt_after_drop.txt"

echo "REMOTE_PROBE_BIN=${target_probe_bin}"
echo "REMOTE_OP_BIN=${target_op_bin}"
echo "REMOTE_OBJECT_QUERY_BIN=${target_object_query_bin}"
echo "REMOTE_RANGE_QUERY_BIN=${target_range_query_bin}"
echo "REMOTE_GTT_STATE_BIN=${target_gtt_state_bin}"
echo "HOLD_SECONDS=${hold_seconds}"
echo "RANGE_ID=\${range_id}"
echo "OBJECT_ID=\${saved_object_id}"
echo "RANGE_BEFORE_ADOPT"
cat "\${tmpdir}/range_before_adopt.txt"
echo "GTT_BEFORE"
cat "\${tmpdir}/gtt_before.txt"
echo "IOCTL_OBJECT_OP_ADOPT"
cat "\${tmpdir}/object_op_adopt.txt"
echo "OBJECT_AFTER_ADOPT"
cat "\${tmpdir}/object_after_adopt.txt"
echo "RANGE_AFTER_ADOPT"
cat "\${tmpdir}/range_after_adopt.txt"
echo "GTT_AFTER_ADOPT"
cat "\${tmpdir}/gtt_after_adopt.txt"
echo "IOCTL_OBJECT_OP_ROLE"
cat "\${tmpdir}/object_op_role.txt"
echo "IOCTL_OBJECT_OP_CLASS"
cat "\${tmpdir}/object_op_class.txt"
echo "IOCTL_OBJECT_OP_GPU_USE"
cat "\${tmpdir}/object_op_gpu_use.txt"
echo "IOCTL_OBJECT_OP_CONTEXT"
cat "\${tmpdir}/object_op_context.txt"
echo "OBJECT_AFTER_META"
cat "\${tmpdir}/object_after_meta.txt"
echo "IOCTL_OBJECT_OP_PF_BIND"
cat "\${tmpdir}/object_op_pf_bind.txt"
echo "OBJECT_AFTER_BIND"
cat "\${tmpdir}/object_after_bind.txt"
echo "RANGE_AFTER_BIND"
cat "\${tmpdir}/range_after_bind.txt"
echo "GTT_AFTER_BIND"
cat "\${tmpdir}/gtt_after_bind.txt"
echo "IOCTL_OBJECT_OP_PF_UNBIND"
cat "\${tmpdir}/object_op_pf_unbind.txt"
echo "OBJECT_AFTER_UNBIND"
cat "\${tmpdir}/object_after_unbind.txt"
echo "RANGE_AFTER_UNBIND"
cat "\${tmpdir}/range_after_unbind.txt"
echo "GTT_AFTER_UNBIND"
cat "\${tmpdir}/gtt_after_unbind.txt"
echo "OBJECT_AFTER_PROBE_EXIT"
cat "\${tmpdir}/object_after_probe_exit.txt"
echo "IOCTL_OBJECT_OP_DROP"
cat "\${tmpdir}/object_op_drop.txt"
echo "OBJECT_AFTER_DROP"
cat "\${tmpdir}/object_after_drop.txt"
echo "RANGE_AFTER_DROP"
cat "\${tmpdir}/range_after_drop.txt"
echo "GTT_AFTER_DROP"
cat "\${tmpdir}/gtt_after_drop.txt"
echo "DRM_PROBE_HOLD_LOG"
cat "\${tmpdir}/drm_probe_hold.log"
EOF
