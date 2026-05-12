#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_node="${TARGET_NODE:-/dev/dri/card0}"
target_dir="${TARGET_DIR:-/root/poulsbo-workloads}"
target_submit_bin="${TARGET_SUBMIT_BIN:-${target_dir}/psb_sgx_lab_submit_op}"
target_object_bin="${TARGET_OBJECT_BIN:-${target_dir}/psb_sgx_lab_object_op}"
target_query_bin="${TARGET_QUERY_BIN:-${target_dir}/psb_sgx_lab_query}"
target_range_bin="${TARGET_RANGE_BIN:-${target_dir}/psb_sgx_lab_range_query}"
target_gtt_state_bin="${TARGET_GTT_STATE_BIN:-${target_dir}/psb_sgx_lab_gtt_state_query}"
remote_lock_file="${REMOTE_LOCK_FILE:-/run/poulsbo-sgx-lab.lock}"
remote_lock_wait_secs="${REMOTE_LOCK_WAIT_SECS:-600}"
max_attempts="${MAX_ATTEMPTS:-20}"
sleep_secs="${SLEEP_SECS:-1}"
baseline_mmapping_gem_max="${BASELINE_MMAPPING_GEM_MAX:-2}"

if [[ "${max_attempts}" -lt 1 ]]; then
    echo "MAX_ATTEMPTS must be at least 1" >&2
    exit 1
fi

if [[ "${sleep_secs}" -lt 0 ]]; then
    echo "SLEEP_SECS must be zero or greater" >&2
    exit 1
fi

if [[ "${baseline_mmapping_gem_max}" -lt 0 ]]; then
    echo "BASELINE_MMAPPING_GEM_MAX must be zero or greater" >&2
    exit 1
fi

if [[ "${remote_lock_wait_secs}" -lt 1 ]]; then
    echo "REMOTE_LOCK_WAIT_SECS must be at least 1" >&2
    exit 1
fi

ssh "${target_user}@${target_host}" "bash -s" <<EOF
set -euo pipefail

target_node="${target_node}"
target_submit_bin="${target_submit_bin}"
target_object_bin="${target_object_bin}"
target_query_bin="${target_query_bin}"
target_range_bin="${target_range_bin}"
target_gtt_state_bin="${target_gtt_state_bin}"
remote_lock_file="${remote_lock_file}"
remote_lock_wait_secs="${remote_lock_wait_secs}"
max_attempts="${max_attempts}"
sleep_secs="${sleep_secs}"
baseline_mmapping_gem_max="${baseline_mmapping_gem_max}"

if ! command -v flock >/dev/null 2>&1; then
    echo "Target is missing flock(1)" >&2
    exit 1
fi

exec 9>"\${remote_lock_file}"
if ! flock -w "\${remote_lock_wait_secs}" 9; then
    echo "Timed out waiting for remote SGX lab lock: \${remote_lock_file}" >&2
    exit 1
fi

for required in "\${target_submit_bin}" "\${target_object_bin}" "\${target_query_bin}" "\${target_range_bin}" "\${target_gtt_state_bin}"; do
    if [[ ! -x "\${required}" ]]; then
        echo "required target executable missing: \${required}" >&2
        exit 1
    fi
done

extract_value() {
    local key="\$1"
    awk -F= -v want="\${key}" '\$1 == want { print \$2; exit }'
}

count_live_mmapping_gem_ranges() {
    local range_index
    local range_out
    local range_ioctl_ret
    local range_type_name
    local range_mmapping
    local range_sgx_object_id
    local live_count=0

    for range_index in \$(seq 0 1024); do
        range_out="\$("\${target_range_bin}" "\${target_node}" index "\${range_index}" gem 2>/dev/null || true)"
        [[ -n "\${range_out}" ]] || break
        range_ioctl_ret=\$(printf '%s\n' "\${range_out}" | awk '/^range_ioctl_ret=/ { sub(/^range_ioctl_ret=/, ""); print \$1; exit }')
        if [[ "\${range_ioctl_ret:-}" != "0" ]]; then
            break
        fi
        range_type_name=\$(printf '%s\n' "\${range_out}" | awk -F= '/^range_type_name=/ { print \$2 }')
        range_mmapping=\$(printf '%s\n' "\${range_out}" | awk -F= '/^range_mmapping=/ { print \$2 }')
        range_sgx_object_id=\$(printf '%s\n' "\${range_out}" | awk -F= '/^range_sgx_object_id=/ { print \$2 }')
        if [[ "\${range_type_name}" == "gem" && "\${range_mmapping}" == "1" && "\${range_sgx_object_id}" == "0" ]]; then
            live_count=\$((live_count + 1))
        fi
    done

    echo "\${live_count}"
}

attempt=1
while [[ "\${attempt}" -le "\${max_attempts}" ]]; do
    submit_cleanup_out="\$("\${target_submit_bin}" "\${target_node}" cleanup 2>/dev/null || true)"
    object_cleanup_out="\$("\${target_object_bin}" "\${target_node}" cleanup 2>/dev/null || true)"
    coord_out="\$("\${target_query_bin}" "\${target_node}" coord 2>/dev/null || true)"
    gtt_out="\$("\${target_gtt_state_bin}" "\${target_node}" 2>/dev/null || true)"

    live_submit_count=\$(printf '%s\n' "\${coord_out}" | extract_value coord_live_submit_count)
    live_object_count=\$(printf '%s\n' "\${coord_out}" | extract_value coord_live_object_count)
    focused_submit_valid=\$(printf '%s\n' "\${coord_out}" | extract_value coord_focus_valid)
    range_sgx_object_count=\$(printf '%s\n' "\${gtt_out}" | extract_value gtt_state_range_sgx_object_count)
    range_pf_bound_count=\$(printf '%s\n' "\${gtt_out}" | extract_value gtt_state_range_pf_bound_count)
    range_query_live_gem_count=\$(count_live_mmapping_gem_ranges)

    live_submit_count="\${live_submit_count:-0}"
    live_object_count="\${live_object_count:-0}"
    focused_submit_valid="\${focused_submit_valid:-0}"
    range_sgx_object_count="\${range_sgx_object_count:-0}"
    range_pf_bound_count="\${range_pf_bound_count:-0}"
    range_query_live_gem_count="\${range_query_live_gem_count:-0}"

    echo "attempt=\${attempt}"
    echo "submit_result_count=\$(printf '%s\n' "\${submit_cleanup_out}" | extract_value submit_result_count)"
    echo "object_result_count=\$(printf '%s\n' "\${object_cleanup_out}" | extract_value object_result_count)"
    echo "coord_live_submit_count=\${live_submit_count}"
    echo "coord_live_object_count=\${live_object_count}"
    echo "coord_focus_valid=\${focused_submit_valid}"
    echo "gtt_state_range_sgx_object_count=\${range_sgx_object_count}"
    echo "gtt_state_range_pf_bound_count=\${range_pf_bound_count}"
    echo "range_query_live_gem_count=\${range_query_live_gem_count}"
    echo "baseline_mmapping_gem_max=\${baseline_mmapping_gem_max}"

    if [[ "\${live_submit_count}" == "0" && \
          "\${live_object_count}" == "0" && \
          "\${focused_submit_valid}" == "0" && \
          "\${range_sgx_object_count}" == "0" && \
          "\${range_pf_bound_count}" == "0" && \
          "\${range_query_live_gem_count}" -le "\${baseline_mmapping_gem_max}" ]]; then
        exit 0
    fi

    if [[ "\${attempt}" -lt "\${max_attempts}" && "\${sleep_secs}" -gt 0 ]]; then
        sleep "\${sleep_secs}"
    fi
    attempt=\$((attempt + 1))
done

echo "SGX lab reset did not converge to zero state" >&2
printf '%s\n' "\${coord_out}" >&2
printf '%s\n' "\${gtt_out}" >&2
exit 1
EOF
