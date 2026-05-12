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
remote_shell_trace="${REMOTE_SHELL_TRACE:-0}"
shared_control_submits="${SHARED_CONTROL_SUBMITS:-1}"
enumerate_matches="${ENUMERATE_MATCHES:-0}"

if [[ "${shared_control_submits}" != "1" && "${shared_control_submits}" != "2" ]]; then
    echo "SHARED_CONTROL_SUBMITS must be 1 or 2" >&2
    exit 1
fi

if [[ "${enumerate_matches}" != "0" && "${enumerate_matches}" != "1" ]]; then
    echo "ENUMERATE_MATCHES must be 0 or 1" >&2
    exit 1
fi

for bin in \
    "$local_probe_bin" \
    "$local_range_bin" \
    "$local_object_op_bin" \
    "$local_submit_op_bin" \
    "$local_query_bin"; do
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

remote_shell_trace="${remote_shell_trace}"
if [[ "\${remote_shell_trace}" == "1" ]]; then
    set -x
fi

hold_seconds="${hold_seconds}"
shared_control_submits="${shared_control_submits}"
enumerate_matches="${enumerate_matches}"

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

"${target_submit_op_bin}" "${target_node}" cleanup > "\${tmpdir}/submit_cleanup_before.txt" 2>&1 || true
"${target_object_op_bin}" "${target_node}" cleanup > "\${tmpdir}/object_cleanup_before.txt" 2>&1 || true

for probe_index in \$(seq 1 "${probe_count}"); do
    "${target_probe_bin}" "${target_node}" "${hold_seconds}" > "\${tmpdir}/drm_probe_\${probe_index}.log" 2>&1 &
    probe_pids+=("\$!")
done

for _ in \$(seq 1 \$((hold_seconds + 5))); do
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
done

"${target_submit_op_bin}" "${target_node}" create pf > "\${tmpdir}/submit_create.txt"
submit_id=\$(awk -F= '/^submit_resolved_id=/ { print \$2 }' "\${tmpdir}/submit_create.txt" | tail -n 1)
if [[ -z "\${submit_id}" || "\${submit_id}" == "0" ]]; then
    echo "Failed to create submit" >&2
    cat "\${tmpdir}/submit_create.txt" >&2
    exit 1
fi
submit_ids=("\${submit_id}")

"${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" control "\${object_control}" > "\${tmpdir}/attach_control.txt"
"${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" pds "\${object_pds}" > "\${tmpdir}/attach_pds.txt"
"${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" use "\${object_use}" > "\${tmpdir}/attach_use.txt"
"${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" target "\${object_target}" > "\${tmpdir}/attach_target.txt"
"${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" status "\${object_status}" > "\${tmpdir}/attach_status.txt"

second_submit_id=""
if [[ "\${shared_control_submits}" == "2" ]]; then
    "${target_submit_op_bin}" "${target_node}" create pf > "\${tmpdir}/submit_create_2.txt"
    second_submit_id=\$(awk -F= '/^submit_resolved_id=/ { print \$2 }' "\${tmpdir}/submit_create_2.txt" | tail -n 1)
    if [[ -z "\${second_submit_id}" || "\${second_submit_id}" == "0" ]]; then
        echo "Failed to create second submit" >&2
        cat "\${tmpdir}/submit_create_2.txt" >&2
        exit 1
    fi
    submit_ids+=("\${second_submit_id}")
    "${target_submit_op_bin}" "${target_node}" attach "\${second_submit_id}" control "\${object_control}" > "\${tmpdir}/attach_control_2.txt"
fi

"${target_range_bin}" "${target_node}" id "\${range_ids[0]}" gem > "\${tmpdir}/control_range.txt"
control_gpu_va=\$(awk -F= '/^range_gpu_va=/{print \$2; exit}' "\${tmpdir}/control_range.txt")
control_len_hex=\$(awk -F= '/^range_len=/{print \$2; exit}' "\${tmpdir}/control_range.txt")
if [[ -z "\${control_gpu_va}" || -z "\${control_len_hex}" ]]; then
    echo "Failed to resolve control GPU VA" >&2
    exit 1
fi
control_query_offset="0x0"
if [[ \$((control_len_hex)) -gt 0x1000 ]]; then
    control_query_offset="0x1000"
fi
control_query_gpu_va=\$(printf '0x%x' \$((control_gpu_va + control_query_offset)))
control_query_gpu_va_expected=\$(printf '0x%08x' \$((control_query_gpu_va)))
control_query_offset_expected=\$(printf '0x%08x' \$((control_query_offset)))

"${target_query_bin}" "${target_node}" gpuva "\${control_query_gpu_va}" > "\${tmpdir}/submit_query_gpuva_live.txt"
grep -q "^submit_resolved_id=\${submit_id}\$" "\${tmpdir}/submit_query_gpuva_live.txt"
grep -q "^submit_matching_submit_count=\${shared_control_submits}\$" "\${tmpdir}/submit_query_gpuva_live.txt"
grep -q '^submit_resolved_slot_name=control$' "\${tmpdir}/submit_query_gpuva_live.txt"
grep -q "^submit_resolved_object_id=\${object_control}\$" "\${tmpdir}/submit_query_gpuva_live.txt"
grep -q "^submit_resolved_range_id=\${range_ids[0]}\$" "\${tmpdir}/submit_query_gpuva_live.txt"
grep -q '^submit_resolved_range_type_name=gem$' "\${tmpdir}/submit_query_gpuva_live.txt"
grep -q '^submit_resolved_object_role_name=control$' "\${tmpdir}/submit_query_gpuva_live.txt"
grep -q '^submit_resolved_object_class_name=packet$' "\${tmpdir}/submit_query_gpuva_live.txt"
grep -q '^submit_resolved_object_gpu_use_name=control_ccb$' "\${tmpdir}/submit_query_gpuva_live.txt"
grep -q '^submit_resolved_object_expected_context_name=pf$' "\${tmpdir}/submit_query_gpuva_live.txt"
grep -q "^submit_query_gpu_va=\${control_query_gpu_va_expected}\$" "\${tmpdir}/submit_query_gpuva_live.txt"
grep -q "^submit_query_gpu_va_offset=\${control_query_offset_expected}\$" "\${tmpdir}/submit_query_gpuva_live.txt"

if [[ "\${shared_control_submits}" == "2" ]]; then
    "${target_query_bin}" "${target_node}" gpuva-match "\${control_query_gpu_va}" 0 > "\${tmpdir}/submit_query_gpuva_match0_live.txt"
    grep -q "^submit_query_match_index=0\$" "\${tmpdir}/submit_query_gpuva_match0_live.txt"
    grep -q "^submit_resolved_id=\${submit_id}\$" "\${tmpdir}/submit_query_gpuva_match0_live.txt"
    grep -q "^submit_matching_submit_count=2\$" "\${tmpdir}/submit_query_gpuva_match0_live.txt"
    grep -q "^submit_resolved_match_index=0\$" "\${tmpdir}/submit_query_gpuva_match0_live.txt"

    "${target_query_bin}" "${target_node}" gpuva-match "\${control_query_gpu_va}" 1 > "\${tmpdir}/submit_query_gpuva_match1_live.txt"
    grep -q "^submit_query_match_index=1\$" "\${tmpdir}/submit_query_gpuva_match1_live.txt"
    grep -q "^submit_resolved_id=\${second_submit_id}\$" "\${tmpdir}/submit_query_gpuva_match1_live.txt"
    grep -q "^submit_matching_submit_count=2\$" "\${tmpdir}/submit_query_gpuva_match1_live.txt"
    grep -q "^submit_resolved_match_index=1\$" "\${tmpdir}/submit_query_gpuva_match1_live.txt"
    grep -q '^submit_resolved_slot_name=control$' "\${tmpdir}/submit_query_gpuva_match1_live.txt"
    grep -q "^submit_resolved_object_id=\${object_control}\$" "\${tmpdir}/submit_query_gpuva_match1_live.txt"

    if [[ "\${enumerate_matches}" == "1" ]]; then
        "${target_query_bin}" "${target_node}" gpuva-match "\${control_query_gpu_va}" 0 > "\${tmpdir}/submit_query_gpuva_list_live_0.txt"
        "${target_query_bin}" "${target_node}" gpuva-match "\${control_query_gpu_va}" 1 > "\${tmpdir}/submit_query_gpuva_list_live_1.txt"
        grep -q "^submit_resolved_id=\${submit_id}\$" "\${tmpdir}/submit_query_gpuva_list_live_0.txt"
        grep -q "^submit_resolved_id=\${second_submit_id}\$" "\${tmpdir}/submit_query_gpuva_list_live_1.txt"
        grep -q '^submit_matching_submit_count=2$' "\${tmpdir}/submit_query_gpuva_list_live_0.txt"
        grep -q '^submit_matching_submit_count=2$' "\${tmpdir}/submit_query_gpuva_list_live_1.txt"
    fi
fi

for pid in "\${probe_pids[@]}"; do
    wait "\${pid}"
done
probe_pids=()

"${target_query_bin}" "${target_node}" gpuva "\${control_query_gpu_va}" > "\${tmpdir}/submit_query_gpuva_post_probe_exit.txt"
grep -q "^submit_resolved_id=\${submit_id}\$" "\${tmpdir}/submit_query_gpuva_post_probe_exit.txt"
grep -q "^submit_matching_submit_count=\${shared_control_submits}\$" "\${tmpdir}/submit_query_gpuva_post_probe_exit.txt"
grep -q '^submit_resolved_slot_name=control$' "\${tmpdir}/submit_query_gpuva_post_probe_exit.txt"
grep -q "^submit_resolved_object_id=\${object_control}\$" "\${tmpdir}/submit_query_gpuva_post_probe_exit.txt"
grep -q "^submit_resolved_range_id=\${range_ids[0]}\$" "\${tmpdir}/submit_query_gpuva_post_probe_exit.txt"
grep -q '^submit_resolved_range_type_name=gem$' "\${tmpdir}/submit_query_gpuva_post_probe_exit.txt"
grep -q '^submit_resolved_object_role_name=control$' "\${tmpdir}/submit_query_gpuva_post_probe_exit.txt"
grep -q '^submit_resolved_object_class_name=packet$' "\${tmpdir}/submit_query_gpuva_post_probe_exit.txt"
grep -q '^submit_resolved_object_gpu_use_name=control_ccb$' "\${tmpdir}/submit_query_gpuva_post_probe_exit.txt"
grep -q '^submit_resolved_object_expected_context_name=pf$' "\${tmpdir}/submit_query_gpuva_post_probe_exit.txt"

if [[ "\${shared_control_submits}" == "2" ]]; then
    "${target_query_bin}" "${target_node}" gpuva-match "\${control_query_gpu_va}" 1 > "\${tmpdir}/submit_query_gpuva_match1_post_probe_exit.txt"
    grep -q "^submit_query_match_index=1\$" "\${tmpdir}/submit_query_gpuva_match1_post_probe_exit.txt"
    grep -q "^submit_resolved_id=\${second_submit_id}\$" "\${tmpdir}/submit_query_gpuva_match1_post_probe_exit.txt"
    grep -q "^submit_matching_submit_count=2\$" "\${tmpdir}/submit_query_gpuva_match1_post_probe_exit.txt"
    grep -q "^submit_resolved_match_index=1\$" "\${tmpdir}/submit_query_gpuva_match1_post_probe_exit.txt"

    if [[ "\${enumerate_matches}" == "1" ]]; then
        "${target_query_bin}" "${target_node}" gpuva-match "\${control_query_gpu_va}" 0 > "\${tmpdir}/submit_query_gpuva_list_post_probe_exit_0.txt"
        "${target_query_bin}" "${target_node}" gpuva-match "\${control_query_gpu_va}" 1 > "\${tmpdir}/submit_query_gpuva_list_post_probe_exit_1.txt"
        grep -q "^submit_resolved_id=\${submit_id}\$" "\${tmpdir}/submit_query_gpuva_list_post_probe_exit_0.txt"
        grep -q "^submit_resolved_id=\${second_submit_id}\$" "\${tmpdir}/submit_query_gpuva_list_post_probe_exit_1.txt"
    fi
fi

"${target_submit_op_bin}" "${target_node}" drop "\${submit_id}" > "\${tmpdir}/submit_drop.txt"

if [[ "\${shared_control_submits}" == "2" ]]; then
    "${target_query_bin}" "${target_node}" gpuva "\${control_query_gpu_va}" > "\${tmpdir}/submit_query_gpuva_after_primary_drop.txt"
    grep -q "^submit_resolved_id=\${second_submit_id}\$" "\${tmpdir}/submit_query_gpuva_after_primary_drop.txt"
    grep -q '^submit_resolved_slot_name=control$' "\${tmpdir}/submit_query_gpuva_after_primary_drop.txt"
    grep -q "^submit_resolved_object_id=\${object_control}\$" "\${tmpdir}/submit_query_gpuva_after_primary_drop.txt"
    grep -q "^submit_matching_submit_count=1\$" "\${tmpdir}/submit_query_gpuva_after_primary_drop.txt"
    grep -q "^submit_resolved_range_id=\${range_ids[0]}\$" "\${tmpdir}/submit_query_gpuva_after_primary_drop.txt"
    grep -q '^submit_resolved_range_type_name=gem$' "\${tmpdir}/submit_query_gpuva_after_primary_drop.txt"
    grep -q '^submit_resolved_object_role_name=control$' "\${tmpdir}/submit_query_gpuva_after_primary_drop.txt"
    grep -q '^submit_resolved_object_class_name=packet$' "\${tmpdir}/submit_query_gpuva_after_primary_drop.txt"
    grep -q '^submit_resolved_object_gpu_use_name=control_ccb$' "\${tmpdir}/submit_query_gpuva_after_primary_drop.txt"
    grep -q '^submit_resolved_object_expected_context_name=pf$' "\${tmpdir}/submit_query_gpuva_after_primary_drop.txt"

    "${target_query_bin}" "${target_node}" gpuva-match "\${control_query_gpu_va}" 0 > "\${tmpdir}/submit_query_gpuva_match0_after_primary_drop.txt"
    grep -q "^submit_query_match_index=0\$" "\${tmpdir}/submit_query_gpuva_match0_after_primary_drop.txt"
    grep -q "^submit_resolved_id=\${second_submit_id}\$" "\${tmpdir}/submit_query_gpuva_match0_after_primary_drop.txt"
    grep -q "^submit_matching_submit_count=1\$" "\${tmpdir}/submit_query_gpuva_match0_after_primary_drop.txt"
    grep -q "^submit_resolved_match_index=0\$" "\${tmpdir}/submit_query_gpuva_match0_after_primary_drop.txt"

    set +e
    "${target_query_bin}" "${target_node}" gpuva-match "\${control_query_gpu_va}" 1 > "\${tmpdir}/submit_query_gpuva_match1_after_primary_drop.txt" 2>&1
    submit_query_match1_after_primary_drop_rc=\$?
    set -e
    if [[ "\${submit_query_match1_after_primary_drop_rc}" -eq 0 ]]; then
        echo "Submit GPU-VA match-index query unexpectedly succeeded after primary drop" >&2
        exit 1
    fi
    grep -q '^submit_ioctl_ret=-1 errno=2$' "\${tmpdir}/submit_query_gpuva_match1_after_primary_drop.txt"

    if [[ "\${enumerate_matches}" == "1" ]]; then
        "${target_query_bin}" "${target_node}" gpuva-match "\${control_query_gpu_va}" 0 > "\${tmpdir}/submit_query_gpuva_list_after_primary_drop_0.txt"
        grep -q "^submit_resolved_id=\${second_submit_id}\$" "\${tmpdir}/submit_query_gpuva_list_after_primary_drop_0.txt"
        grep -q '^submit_matching_submit_count=1$' "\${tmpdir}/submit_query_gpuva_list_after_primary_drop_0.txt"
    fi

    "${target_submit_op_bin}" "${target_node}" drop "\${second_submit_id}" > "\${tmpdir}/submit_drop_second.txt"
fi

set +e
"${target_query_bin}" "${target_node}" gpuva "\${control_query_gpu_va}" > "\${tmpdir}/submit_query_gpuva_after_drop.txt" 2>&1
submit_query_after_drop_rc=\$?
set -e

if [[ "\${submit_query_after_drop_rc}" -eq 0 ]]; then
    echo "Submit GPU-VA query unexpectedly succeeded after submit drop" >&2
    exit 1
fi
grep -q '^submit_ioctl_ret=-1 errno=2$' "\${tmpdir}/submit_query_gpuva_after_drop.txt"

"${target_object_op_bin}" "${target_node}" cleanup > "\${tmpdir}/object_cleanup.txt"
"${target_submit_op_bin}" "${target_node}" cleanup > "\${tmpdir}/submit_cleanup_after.txt"

echo "RANGE_IDS=\${range_ids[*]}"
echo "OBJECT_IDS=\${object_ids[*]}"
echo "SUBMIT_ID=\${submit_id}"
if [[ -n "\${second_submit_id}" ]]; then
    echo "SECOND_SUBMIT_ID=\${second_submit_id}"
fi
echo "CONTROL_QUERY_GPU_VA=\${control_query_gpu_va}"
echo "CONTROL_QUERY_OFFSET=\${control_query_offset}"
echo "SUBMIT_QUERY_GPUVA_LIVE"
cat "\${tmpdir}/submit_query_gpuva_live.txt"
if [[ "\${shared_control_submits}" == "2" ]]; then
    echo "SUBMIT_QUERY_GPUVA_MATCH0_LIVE"
    cat "\${tmpdir}/submit_query_gpuva_match0_live.txt"
    echo "SUBMIT_QUERY_GPUVA_MATCH1_LIVE"
    cat "\${tmpdir}/submit_query_gpuva_match1_live.txt"
    if [[ "\${enumerate_matches}" == "1" ]]; then
        echo "SUBMIT_QUERY_GPUVA_LIST_LIVE_0"
        cat "\${tmpdir}/submit_query_gpuva_list_live_0.txt"
        echo "SUBMIT_QUERY_GPUVA_LIST_LIVE_1"
        cat "\${tmpdir}/submit_query_gpuva_list_live_1.txt"
    fi
fi
echo "SUBMIT_QUERY_GPUVA_POST_PROBE_EXIT"
cat "\${tmpdir}/submit_query_gpuva_post_probe_exit.txt"
if [[ "\${shared_control_submits}" == "2" ]]; then
    echo "SUBMIT_QUERY_GPUVA_MATCH1_POST_PROBE_EXIT"
    cat "\${tmpdir}/submit_query_gpuva_match1_post_probe_exit.txt"
    if [[ "\${enumerate_matches}" == "1" ]]; then
        echo "SUBMIT_QUERY_GPUVA_LIST_POST_PROBE_EXIT_0"
        cat "\${tmpdir}/submit_query_gpuva_list_post_probe_exit_0.txt"
        echo "SUBMIT_QUERY_GPUVA_LIST_POST_PROBE_EXIT_1"
        cat "\${tmpdir}/submit_query_gpuva_list_post_probe_exit_1.txt"
    fi
fi
echo "SUBMIT_DROP"
cat "\${tmpdir}/submit_drop.txt"
if [[ "\${shared_control_submits}" == "2" ]]; then
    echo "SUBMIT_QUERY_GPUVA_AFTER_PRIMARY_DROP"
    cat "\${tmpdir}/submit_query_gpuva_after_primary_drop.txt"
    echo "SUBMIT_QUERY_GPUVA_MATCH0_AFTER_PRIMARY_DROP"
    cat "\${tmpdir}/submit_query_gpuva_match0_after_primary_drop.txt"
    echo "SUBMIT_QUERY_GPUVA_MATCH1_AFTER_PRIMARY_DROP_RC=\${submit_query_match1_after_primary_drop_rc}"
    cat "\${tmpdir}/submit_query_gpuva_match1_after_primary_drop.txt"
    if [[ "\${enumerate_matches}" == "1" ]]; then
        echo "SUBMIT_QUERY_GPUVA_LIST_AFTER_PRIMARY_DROP_0"
        cat "\${tmpdir}/submit_query_gpuva_list_after_primary_drop_0.txt"
    fi
    echo "SUBMIT_DROP_SECOND"
    cat "\${tmpdir}/submit_drop_second.txt"
fi
echo "SUBMIT_QUERY_GPUVA_AFTER_DROP_RC=\${submit_query_after_drop_rc}"
cat "\${tmpdir}/submit_query_gpuva_after_drop.txt"
echo "OBJECT_CLEANUP"
cat "\${tmpdir}/object_cleanup.txt"
echo "SUBMIT_CLEANUP_AFTER"
cat "\${tmpdir}/submit_cleanup_after.txt"
EOF
