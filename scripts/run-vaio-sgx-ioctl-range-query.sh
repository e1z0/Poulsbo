#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_dir="${TARGET_DIR:-/root/poulsbo-workloads}"
target_probe_bin="${TARGET_PROBE_BIN:-${target_dir}/drm_probe}"
target_range_bin="${TARGET_RANGE_BIN:-${target_dir}/psb_sgx_lab_range_query}"
target_object_bin="${TARGET_OBJECT_BIN:-${target_dir}/psb_sgx_lab_object_op}"
target_submit_bin="${TARGET_SUBMIT_BIN:-${target_dir}/psb_sgx_lab_submit_op}"
target_probe_stage="${TARGET_PROBE_STAGE:-/root/drm_probe.$$.$RANDOM.new}"
target_range_stage="${TARGET_RANGE_STAGE:-/root/psb_sgx_lab_range_query.$$.$RANDOM.new}"
target_object_stage="${TARGET_OBJECT_STAGE:-/root/psb_sgx_lab_object_op.$$.$RANDOM.new}"
target_submit_stage="${TARGET_SUBMIT_STAGE:-/root/psb_sgx_lab_submit_op.$$.$RANDOM.new}"
local_probe_bin="${LOCAL_PROBE_BIN:-$(pwd)/workloads/drm_probe}"
local_range_bin="${LOCAL_RANGE_BIN:-$(pwd)/workloads/psb_sgx_lab_range_query}"
local_object_bin="${LOCAL_OBJECT_BIN:-$(pwd)/workloads/psb_sgx_lab_object_op}"
local_submit_bin="${LOCAL_SUBMIT_BIN:-$(pwd)/workloads/psb_sgx_lab_submit_op}"
target_node="${TARGET_NODE:-/dev/dri/card0}"
sysfs_dir="${SYSFS_DIR:-/sys/devices/pci0000:00/0000:00:02.0}"
hold_seconds="${HOLD_SECONDS:-15}"
probe_count="${PROBE_COUNT:-3}"
shared_control_submits="${SHARED_CONTROL_SUBMITS:-1}"

if [[ "${probe_count}" -lt 1 ]]; then
    echo "PROBE_COUNT must be at least 1" >&2
    exit 1
fi

if [[ "${shared_control_submits}" != "1" && "${shared_control_submits}" != "2" ]]; then
    echo "SHARED_CONTROL_SUBMITS must be 1 or 2" >&2
    exit 1
fi

for bin in "$local_probe_bin" "$local_range_bin" "$local_object_bin" "$local_submit_bin"; do
    if [[ ! -f "$bin" ]]; then
        echo "Local binary not found: $bin" >&2
        exit 1
    fi
done

scp "$local_probe_bin" "${target_user}@${target_host}:${target_probe_stage}"
scp "$local_range_bin" "${target_user}@${target_host}:${target_range_stage}"
scp "$local_object_bin" "${target_user}@${target_host}:${target_object_stage}"
scp "$local_submit_bin" "${target_user}@${target_host}:${target_submit_stage}"

ssh "${target_user}@${target_host}" "bash -s" <<EOF
set -euo pipefail

mkdir -p "${target_dir}"
install -m 0755 -T "${target_probe_stage}" "${target_probe_bin}"
install -m 0755 -T "${target_range_stage}" "${target_range_bin}"
install -m 0755 -T "${target_object_stage}" "${target_object_bin}"
install -m 0755 -T "${target_submit_stage}" "${target_submit_bin}"
rm -f "${target_probe_stage}" "${target_range_stage}" "${target_object_stage}" "${target_submit_stage}"

gtt_node="${sysfs_dir}/poulsbo_gtt_state"
hold_seconds="${hold_seconds}"
shared_control_submits="${shared_control_submits}"
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

if [[ ! -e "\${gtt_node}" ]]; then
    echo "Missing sysfs node: \${gtt_node}" >&2
    exit 1
fi

cat "\${gtt_node}" > "\${tmpdir}/before_gtt.txt"

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

cat "\${gtt_node}" > "\${tmpdir}/during_gtt.txt"

echo "BEFORE_GTT"
cat "\${tmpdir}/before_gtt.txt"

echo "DURING_GTT"
cat "\${tmpdir}/during_gtt.txt"

query_count=0
first_range_id=""
fresh_range_id=""
fresh_range_is_new=0
baseline_range_ids=""
query_gpu_va=""
query_gpu_va_offset=""
adopted_object_id=""
attached_submit_id=""
second_submit_id=""
adopt_role="control"
adopt_class="packet"
adopt_gpu_use="control_ccb"
adopt_context="pf"

"${target_submit_bin}" "${target_node}" cleanup > "\${tmpdir}/submit_cleanup_before.log" 2>&1
echo "SUBMIT_CLEANUP_BEFORE"
cat "\${tmpdir}/submit_cleanup_before.log"
"${target_object_bin}" "${target_node}" cleanup > "\${tmpdir}/object_cleanup_before.log" 2>&1
echo "OBJECT_CLEANUP_BEFORE"
cat "\${tmpdir}/object_cleanup_before.log"

while :; do
    if ! "${target_range_bin}" "${target_node}" index "\${query_count}" gem > "\${tmpdir}/before_range_\${query_count}.log" 2>&1; then
        break
    fi
    baseline_range_id=\$(awk -F= '/^range_resolved_id=/{print \$2; exit}' "\${tmpdir}/before_range_\${query_count}.log")
    if [[ -n "\${baseline_range_id}" ]]; then
        baseline_range_ids+=" \${baseline_range_id}"
    fi
    query_count=\$((query_count + 1))
done

echo "BASELINE_RANGE_IDS=\${baseline_range_ids# }"
query_count=0

while :; do
    if ! "${target_range_bin}" "${target_node}" index "\${query_count}" gem > "\${tmpdir}/range_\${query_count}.log" 2>&1; then
        break
    fi
    echo "RANGE_QUERY_INDEX_\${query_count}"
    cat "\${tmpdir}/range_\${query_count}.log"
    if [[ -z "\${first_range_id}" ]]; then
        first_range_id=\$(awk -F= '/^range_resolved_id=/{print \$2; exit}' "\${tmpdir}/range_\${query_count}.log")
    fi
    current_range_id=\$(awk -F= '/^range_resolved_id=/{print \$2; exit}' "\${tmpdir}/range_\${query_count}.log")
    if [[ -z "\${fresh_range_id}" && -n "\${current_range_id}" ]]; then
        is_baseline=0
        for baseline_range_id in \${baseline_range_ids}; do
            if [[ "\${baseline_range_id}" == "\${current_range_id}" ]]; then
                is_baseline=1
                break
            fi
        done
        if [[ "\${is_baseline}" == "0" ]]; then
            fresh_range_id="\${current_range_id}"
            fresh_range_is_new=1
        fi
    fi
    query_count=\$((query_count + 1))
done

echo "RANGE_QUERY_COUNT=\${query_count}"

if [[ -z "\${first_range_id}" || "\${first_range_id}" == "0" ]]; then
    echo "Failed to resolve first GEM range ID" >&2
    exit 1
fi

if [[ -z "\${fresh_range_id}" || "\${fresh_range_id}" == "0" ]]; then
    fresh_range_id="\${first_range_id}"
fi

echo "FRESH_RANGE_ID=\${fresh_range_id}"
echo "FRESH_RANGE_IS_NEW=\${fresh_range_is_new}"

"${target_range_bin}" "${target_node}" id "\${fresh_range_id}" gem > "\${tmpdir}/range_by_id.log" 2>&1
echo "RANGE_QUERY_BY_ID"
cat "\${tmpdir}/range_by_id.log"

range_by_id_resolved_id=\$(awk -F= '/^range_resolved_id=/{print \$2; exit}' "\${tmpdir}/range_by_id.log")
if [[ "\${range_by_id_resolved_id}" != "\${fresh_range_id}" ]]; then
    echo "Range-by-id lookup resolved unexpected range id: \${range_by_id_resolved_id}" >&2
    exit 1
fi

query_gpu_va=\$(awk -F= '/^range_gpu_va=/{print \$2; exit}' "\${tmpdir}/range_by_id.log")
if [[ -z "\${query_gpu_va}" ]]; then
    echo "Failed to resolve range GPU VA from range_by_id.log" >&2
    exit 1
fi

range_len_hex=\$(awk -F= '/^range_len=/{print \$2; exit}' "\${tmpdir}/range_by_id.log")
range_len_dec=\$((range_len_hex))
query_gpu_va_offset="0x0"
if [[ "\${range_len_dec}" -gt 0x1000 ]]; then
    query_gpu_va_offset="0x1000"
fi
query_gpu_va_value=\$(printf '0x%x' \$((query_gpu_va + query_gpu_va_offset)))

"${target_range_bin}" "${target_node}" gpuva "\${query_gpu_va_value}" gem > "\${tmpdir}/range_by_gpuva_live.log" 2>&1
echo "RANGE_QUERY_BY_GPUVA_LIVE"
cat "\${tmpdir}/range_by_gpuva_live.log"

range_live_resolved_id=\$(awk -F= '/^range_resolved_id=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_live.log")
range_live_offset=\$(awk -F= '/^range_query_gpu_va_offset=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_live.log")
range_live_object_id=\$(awk -F= '/^range_sgx_object_id=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_live.log")
if [[ "\${range_live_resolved_id}" != "\${fresh_range_id}" ]]; then
    echo "GPU-VA live lookup resolved unexpected range id: \${range_live_resolved_id}" >&2
    exit 1
fi
if [[ \$((range_live_offset)) -ne \$((query_gpu_va_offset)) ]]; then
    echo "GPU-VA live lookup resolved unexpected offset: \${range_live_offset}" >&2
    exit 1
fi
if [[ "\${range_live_object_id}" != "0" ]]; then
    echo "GPU-VA live lookup unexpectedly reported an SGX object before adopt: \${range_live_object_id}" >&2
    exit 1
fi

"${target_object_bin}" "${target_node}" adopt "\${fresh_range_id}" > "\${tmpdir}/object_adopt.log" 2>&1
echo "OBJECT_ADOPT"
cat "\${tmpdir}/object_adopt.log"

adopted_object_id=\$(awk -F= '/^object_resolved_id=/{print \$2; exit}' "\${tmpdir}/object_adopt.log")
if [[ -z "\${adopted_object_id}" || "\${adopted_object_id}" == "0" ]]; then
    echo "Failed to adopt first GEM range into SGX lab object" >&2
    exit 1
fi

"${target_object_bin}" "${target_node}" role "\${adopted_object_id}" "\${adopt_role}" > "\${tmpdir}/object_role.log" 2>&1
echo "OBJECT_ROLE"
cat "\${tmpdir}/object_role.log"
"${target_object_bin}" "${target_node}" class "\${adopted_object_id}" "\${adopt_class}" > "\${tmpdir}/object_class.log" 2>&1
echo "OBJECT_CLASS"
cat "\${tmpdir}/object_class.log"
"${target_object_bin}" "${target_node}" gpu_use "\${adopted_object_id}" "\${adopt_gpu_use}" > "\${tmpdir}/object_gpu_use.log" 2>&1
echo "OBJECT_GPU_USE"
cat "\${tmpdir}/object_gpu_use.log"
"${target_object_bin}" "${target_node}" context "\${adopted_object_id}" "\${adopt_context}" > "\${tmpdir}/object_context.log" 2>&1
echo "OBJECT_CONTEXT"
cat "\${tmpdir}/object_context.log"

"${target_range_bin}" "${target_node}" gpuva "\${query_gpu_va_value}" gem > "\${tmpdir}/range_by_gpuva_adopted.log" 2>&1
echo "RANGE_QUERY_BY_GPUVA_ADOPTED"
cat "\${tmpdir}/range_by_gpuva_adopted.log"

range_adopted_object_id=\$(awk -F= '/^range_sgx_object_id=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_adopted.log")
range_adopted_role=\$(awk -F= '/^range_sgx_object_role_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_adopted.log")
range_adopted_class=\$(awk -F= '/^range_sgx_object_class_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_adopted.log")
range_adopted_gpu_use=\$(awk -F= '/^range_sgx_object_gpu_use_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_adopted.log")
range_adopted_context=\$(awk -F= '/^range_sgx_object_expected_context_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_adopted.log")
range_adopted_submit_ref_count=\$(awk -F= '/^range_sgx_submit_ref_count=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_adopted.log")
range_adopted_first_submit_id=\$(awk -F= '/^range_sgx_first_submit_id=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_adopted.log")
range_adopted_first_submit_slot_name=\$(awk -F= '/^range_sgx_first_submit_slot_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_adopted.log")
if [[ "\${range_adopted_object_id}" != "\${adopted_object_id}" ]]; then
    echo "GPU-VA adopted lookup reported unexpected SGX object id: \${range_adopted_object_id}" >&2
    exit 1
fi
if [[ "\${range_adopted_role}" != "\${adopt_role}" || "\${range_adopted_class}" != "\${adopt_class}" || "\${range_adopted_gpu_use}" != "\${adopt_gpu_use}" || "\${range_adopted_context}" != "\${adopt_context}" ]]; then
    echo "GPU-VA adopted lookup reported unexpected SGX object metadata" >&2
    exit 1
fi
if [[ "\${range_adopted_submit_ref_count}" != "0" || "\${range_adopted_first_submit_id}" != "0" || "\${range_adopted_first_submit_slot_name}" != "none" ]]; then
    echo "GPU-VA adopted lookup unexpectedly reported live submit references" >&2
    exit 1
fi
range_adopted_resolved_submit_found=\$(awk -F= '/^range_sgx_resolved_submit_found=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_adopted.log")
if [[ "\${range_adopted_resolved_submit_found}" != "0" ]]; then
    echo "GPU-VA adopted lookup unexpectedly reported a resolved selected submit" >&2
    exit 1
fi

"${target_submit_bin}" "${target_node}" create default > "\${tmpdir}/submit_create.log" 2>&1
echo "SUBMIT_CREATE"
cat "\${tmpdir}/submit_create.log"

attached_submit_id=\$(awk -F= '/^submit_resolved_id=/{print \$2; exit}' "\${tmpdir}/submit_create.log")
if [[ -z "\${attached_submit_id}" || "\${attached_submit_id}" == "0" ]]; then
    echo "Failed to create SGX lab submit for GPU-VA attachment check" >&2
    exit 1
fi

"${target_submit_bin}" "${target_node}" attach "\${attached_submit_id}" control "\${adopted_object_id}" > "\${tmpdir}/submit_attach.log" 2>&1
echo "SUBMIT_ATTACH"
cat "\${tmpdir}/submit_attach.log"

if [[ "\${shared_control_submits}" == "2" ]]; then
    "${target_submit_bin}" "${target_node}" create default > "\${tmpdir}/submit_create_2.log" 2>&1
    echo "SUBMIT_CREATE_2"
    cat "\${tmpdir}/submit_create_2.log"

    second_submit_id=\$(awk -F= '/^submit_resolved_id=/{print \$2; exit}' "\${tmpdir}/submit_create_2.log")
    if [[ -z "\${second_submit_id}" || "\${second_submit_id}" == "0" ]]; then
        echo "Failed to create second SGX lab submit for GPU-VA attachment check" >&2
        exit 1
    fi

    "${target_submit_bin}" "${target_node}" attach "\${second_submit_id}" control "\${adopted_object_id}" > "\${tmpdir}/submit_attach_2.log" 2>&1
    echo "SUBMIT_ATTACH_2"
    cat "\${tmpdir}/submit_attach_2.log"
fi

"${target_range_bin}" "${target_node}" gpuva "\${query_gpu_va_value}" gem > "\${tmpdir}/range_by_gpuva_attached.log" 2>&1
echo "RANGE_QUERY_BY_GPUVA_ATTACHED"
cat "\${tmpdir}/range_by_gpuva_attached.log"

range_attached_object_id=\$(awk -F= '/^range_sgx_object_id=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_attached.log")
range_attached_role=\$(awk -F= '/^range_sgx_object_role_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_attached.log")
range_attached_class=\$(awk -F= '/^range_sgx_object_class_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_attached.log")
range_attached_gpu_use=\$(awk -F= '/^range_sgx_object_gpu_use_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_attached.log")
range_attached_context=\$(awk -F= '/^range_sgx_object_expected_context_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_attached.log")
range_attached_submit_ref_count=\$(awk -F= '/^range_sgx_submit_ref_count=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_attached.log")
range_attached_first_submit_id=\$(awk -F= '/^range_sgx_first_submit_id=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_attached.log")
range_attached_first_submit_slot_name=\$(awk -F= '/^range_sgx_first_submit_slot_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_attached.log")
range_attached_resolved_submit_found=\$(awk -F= '/^range_sgx_resolved_submit_found=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_attached.log")
if [[ "\${range_attached_object_id}" != "\${adopted_object_id}" ]]; then
    echo "GPU-VA attached lookup reported unexpected SGX object id: \${range_attached_object_id}" >&2
    exit 1
fi
if [[ "\${range_attached_role}" != "\${adopt_role}" || "\${range_attached_class}" != "\${adopt_class}" || "\${range_attached_gpu_use}" != "\${adopt_gpu_use}" || "\${range_attached_context}" != "\${adopt_context}" ]]; then
    echo "GPU-VA attached lookup reported unexpected SGX object metadata" >&2
    exit 1
fi
if [[ "\${range_attached_submit_ref_count}" != "\${shared_control_submits}" || "\${range_attached_first_submit_id}" != "\${attached_submit_id}" || "\${range_attached_first_submit_slot_name}" != "control" ]]; then
    echo "GPU-VA attached lookup reported unexpected SGX submit reference state" >&2
    exit 1
fi
if [[ "\${range_attached_resolved_submit_found}" != "0" ]]; then
    echo "GPU-VA attached lookup unexpectedly resolved a selected submit without gpuva-submit mode" >&2
    exit 1
fi

if [[ "\${shared_control_submits}" == "2" ]]; then
    "${target_range_bin}" "${target_node}" gpuva-submit "\${query_gpu_va_value}" 0 gem > "\${tmpdir}/range_by_gpuva_submit0_attached.log" 2>&1
    echo "RANGE_QUERY_BY_GPUVA_SUBMIT0_ATTACHED"
    cat "\${tmpdir}/range_by_gpuva_submit0_attached.log"
    grep -q '^range_sgx_submit_ref_count=2$' "\${tmpdir}/range_by_gpuva_submit0_attached.log"
    grep -q '^range_sgx_resolved_submit_found=1$' "\${tmpdir}/range_by_gpuva_submit0_attached.log"
    grep -q "^range_sgx_resolved_submit_id=\${attached_submit_id}\$" "\${tmpdir}/range_by_gpuva_submit0_attached.log"
    grep -q '^range_sgx_resolved_submit_slot_name=control$' "\${tmpdir}/range_by_gpuva_submit0_attached.log"
    grep -q '^range_sgx_resolved_submit_match_index=0$' "\${tmpdir}/range_by_gpuva_submit0_attached.log"

    "${target_range_bin}" "${target_node}" gpuva-submit "\${query_gpu_va_value}" 1 gem > "\${tmpdir}/range_by_gpuva_submit1_attached.log" 2>&1
    echo "RANGE_QUERY_BY_GPUVA_SUBMIT1_ATTACHED"
    cat "\${tmpdir}/range_by_gpuva_submit1_attached.log"
    grep -q '^range_sgx_submit_ref_count=2$' "\${tmpdir}/range_by_gpuva_submit1_attached.log"
    grep -q '^range_sgx_resolved_submit_found=1$' "\${tmpdir}/range_by_gpuva_submit1_attached.log"
    grep -q "^range_sgx_resolved_submit_id=\${second_submit_id}\$" "\${tmpdir}/range_by_gpuva_submit1_attached.log"
    grep -q '^range_sgx_resolved_submit_slot_name=control$' "\${tmpdir}/range_by_gpuva_submit1_attached.log"
    grep -q '^range_sgx_resolved_submit_match_index=1$' "\${tmpdir}/range_by_gpuva_submit1_attached.log"
fi

wait
probe_pids=()

"${target_range_bin}" "${target_node}" gpuva "\${query_gpu_va_value}" gem > "\${tmpdir}/range_by_gpuva_post_probe_exit.log" 2>&1
echo "RANGE_QUERY_BY_GPUVA_POST_PROBE_EXIT"
cat "\${tmpdir}/range_by_gpuva_post_probe_exit.log"

range_post_exit_object_id=\$(awk -F= '/^range_sgx_object_id=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_post_probe_exit.log")
range_post_exit_role=\$(awk -F= '/^range_sgx_object_role_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_post_probe_exit.log")
range_post_exit_class=\$(awk -F= '/^range_sgx_object_class_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_post_probe_exit.log")
range_post_exit_gpu_use=\$(awk -F= '/^range_sgx_object_gpu_use_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_post_probe_exit.log")
range_post_exit_context=\$(awk -F= '/^range_sgx_object_expected_context_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_post_probe_exit.log")
range_post_exit_submit_ref_count=\$(awk -F= '/^range_sgx_submit_ref_count=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_post_probe_exit.log")
range_post_exit_first_submit_id=\$(awk -F= '/^range_sgx_first_submit_id=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_post_probe_exit.log")
range_post_exit_first_submit_slot_name=\$(awk -F= '/^range_sgx_first_submit_slot_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_post_probe_exit.log")
range_post_exit_resolved_submit_found=\$(awk -F= '/^range_sgx_resolved_submit_found=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_post_probe_exit.log")
if [[ "\${range_post_exit_object_id}" != "\${adopted_object_id}" ]]; then
    echo "GPU-VA post-probe-exit lookup lost adopted SGX object ownership: \${range_post_exit_object_id}" >&2
    exit 1
fi
if [[ "\${range_post_exit_role}" != "\${adopt_role}" || "\${range_post_exit_class}" != "\${adopt_class}" || "\${range_post_exit_gpu_use}" != "\${adopt_gpu_use}" || "\${range_post_exit_context}" != "\${adopt_context}" ]]; then
    echo "GPU-VA post-probe-exit lookup lost adopted SGX object metadata" >&2
    exit 1
fi
if [[ "\${range_post_exit_submit_ref_count}" != "\${shared_control_submits}" || "\${range_post_exit_first_submit_id}" != "\${attached_submit_id}" || "\${range_post_exit_first_submit_slot_name}" != "control" ]]; then
    echo "GPU-VA post-probe-exit lookup lost SGX submit reference state" >&2
    exit 1
fi
if [[ "\${range_post_exit_resolved_submit_found}" != "0" ]]; then
    echo "GPU-VA post-probe-exit lookup unexpectedly resolved a selected submit without gpuva-submit mode" >&2
    exit 1
fi

if [[ "\${shared_control_submits}" == "2" ]]; then
    "${target_range_bin}" "${target_node}" gpuva-submit "\${query_gpu_va_value}" 1 gem > "\${tmpdir}/range_by_gpuva_submit1_post_probe_exit.log" 2>&1
    echo "RANGE_QUERY_BY_GPUVA_SUBMIT1_POST_PROBE_EXIT"
    cat "\${tmpdir}/range_by_gpuva_submit1_post_probe_exit.log"
    grep -q '^range_sgx_submit_ref_count=2$' "\${tmpdir}/range_by_gpuva_submit1_post_probe_exit.log"
    grep -q '^range_sgx_resolved_submit_found=1$' "\${tmpdir}/range_by_gpuva_submit1_post_probe_exit.log"
    grep -q "^range_sgx_resolved_submit_id=\${second_submit_id}\$" "\${tmpdir}/range_by_gpuva_submit1_post_probe_exit.log"
    grep -q '^range_sgx_resolved_submit_match_index=1$' "\${tmpdir}/range_by_gpuva_submit1_post_probe_exit.log"
fi

"${target_submit_bin}" "${target_node}" drop "\${attached_submit_id}" > "\${tmpdir}/submit_drop.log" 2>&1
echo "SUBMIT_DROP"
cat "\${tmpdir}/submit_drop.log"

"${target_range_bin}" "${target_node}" gpuva "\${query_gpu_va_value}" gem > "\${tmpdir}/range_by_gpuva_after_submit_drop.log" 2>&1
echo "RANGE_QUERY_BY_GPUVA_AFTER_SUBMIT_DROP"
cat "\${tmpdir}/range_by_gpuva_after_submit_drop.log"

range_after_submit_drop_object_id=\$(awk -F= '/^range_sgx_object_id=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_submit_drop.log")
range_after_submit_drop_role=\$(awk -F= '/^range_sgx_object_role_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_submit_drop.log")
range_after_submit_drop_class=\$(awk -F= '/^range_sgx_object_class_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_submit_drop.log")
range_after_submit_drop_gpu_use=\$(awk -F= '/^range_sgx_object_gpu_use_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_submit_drop.log")
range_after_submit_drop_context=\$(awk -F= '/^range_sgx_object_expected_context_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_submit_drop.log")
range_after_submit_drop_submit_ref_count=\$(awk -F= '/^range_sgx_submit_ref_count=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_submit_drop.log")
range_after_submit_drop_first_submit_id=\$(awk -F= '/^range_sgx_first_submit_id=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_submit_drop.log")
range_after_submit_drop_first_submit_slot_name=\$(awk -F= '/^range_sgx_first_submit_slot_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_submit_drop.log")
range_after_submit_drop_resolved_submit_found=\$(awk -F= '/^range_sgx_resolved_submit_found=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_submit_drop.log")
if [[ "\${range_after_submit_drop_object_id}" != "\${adopted_object_id}" ]]; then
    echo "GPU-VA lookup after submit drop lost adopted SGX object ownership: \${range_after_submit_drop_object_id}" >&2
    exit 1
fi
if [[ "\${range_after_submit_drop_role}" != "\${adopt_role}" || "\${range_after_submit_drop_class}" != "\${adopt_class}" || "\${range_after_submit_drop_gpu_use}" != "\${adopt_gpu_use}" || "\${range_after_submit_drop_context}" != "\${adopt_context}" ]]; then
    echo "GPU-VA lookup after submit drop lost adopted SGX object metadata" >&2
    exit 1
fi
if [[ "\${shared_control_submits}" == "1" ]]; then
    if [[ "\${range_after_submit_drop_submit_ref_count}" != "0" || "\${range_after_submit_drop_first_submit_id}" != "0" || "\${range_after_submit_drop_first_submit_slot_name}" != "none" || "\${range_after_submit_drop_resolved_submit_found}" != "0" ]]; then
        echo "GPU-VA lookup after submit drop still reported live SGX submit references" >&2
        exit 1
    fi
else
    if [[ "\${range_after_submit_drop_submit_ref_count}" != "1" || "\${range_after_submit_drop_first_submit_id}" != "\${second_submit_id}" || "\${range_after_submit_drop_first_submit_slot_name}" != "control" || "\${range_after_submit_drop_resolved_submit_found}" != "0" ]]; then
        echo "GPU-VA lookup after primary submit drop reported unexpected remaining SGX submit references" >&2
        exit 1
    fi

    "${target_range_bin}" "${target_node}" gpuva-submit "\${query_gpu_va_value}" 0 gem > "\${tmpdir}/range_by_gpuva_submit0_after_primary_drop.log" 2>&1
    echo "RANGE_QUERY_BY_GPUVA_SUBMIT0_AFTER_PRIMARY_DROP"
    cat "\${tmpdir}/range_by_gpuva_submit0_after_primary_drop.log"
    grep -q '^range_sgx_submit_ref_count=1$' "\${tmpdir}/range_by_gpuva_submit0_after_primary_drop.log"
    grep -q '^range_sgx_resolved_submit_found=1$' "\${tmpdir}/range_by_gpuva_submit0_after_primary_drop.log"
    grep -q "^range_sgx_resolved_submit_id=\${second_submit_id}\$" "\${tmpdir}/range_by_gpuva_submit0_after_primary_drop.log"
    grep -q '^range_sgx_resolved_submit_match_index=0$' "\${tmpdir}/range_by_gpuva_submit0_after_primary_drop.log"

    "${target_range_bin}" "${target_node}" gpuva-submit "\${query_gpu_va_value}" 1 gem > "\${tmpdir}/range_by_gpuva_submit1_after_primary_drop.log" 2>&1
    echo "RANGE_QUERY_BY_GPUVA_SUBMIT1_AFTER_PRIMARY_DROP"
    cat "\${tmpdir}/range_by_gpuva_submit1_after_primary_drop.log"
    grep -q '^range_sgx_submit_ref_count=1$' "\${tmpdir}/range_by_gpuva_submit1_after_primary_drop.log"
    grep -q '^range_sgx_resolved_submit_found=0$' "\${tmpdir}/range_by_gpuva_submit1_after_primary_drop.log"
    grep -q '^range_sgx_resolved_submit_id=0$' "\${tmpdir}/range_by_gpuva_submit1_after_primary_drop.log"

    "${target_submit_bin}" "${target_node}" drop "\${second_submit_id}" > "\${tmpdir}/submit_drop_2.log" 2>&1
    echo "SUBMIT_DROP_2"
    cat "\${tmpdir}/submit_drop_2.log"

    "${target_range_bin}" "${target_node}" gpuva "\${query_gpu_va_value}" gem > "\${tmpdir}/range_by_gpuva_after_submit_drop_2.log" 2>&1
    echo "RANGE_QUERY_BY_GPUVA_AFTER_SUBMIT_DROP_2"
    cat "\${tmpdir}/range_by_gpuva_after_submit_drop_2.log"

    grep -q '^range_sgx_submit_ref_count=0$' "\${tmpdir}/range_by_gpuva_after_submit_drop_2.log"
    grep -q '^range_sgx_first_submit_id=0$' "\${tmpdir}/range_by_gpuva_after_submit_drop_2.log"
    grep -q '^range_sgx_first_submit_slot_name=none$' "\${tmpdir}/range_by_gpuva_after_submit_drop_2.log"
    grep -q '^range_sgx_resolved_submit_found=0$' "\${tmpdir}/range_by_gpuva_after_submit_drop_2.log"
fi

"${target_object_bin}" "${target_node}" drop "\${adopted_object_id}" > "\${tmpdir}/object_drop.log" 2>&1
echo "OBJECT_DROP"
cat "\${tmpdir}/object_drop.log"

set +e
"${target_range_bin}" "${target_node}" gpuva "\${query_gpu_va_value}" gem > "\${tmpdir}/range_by_gpuva_after_drop.log" 2>&1
after_drop_rc=\$?
set -e
echo "RANGE_QUERY_BY_GPUVA_AFTER_DROP_RC=\${after_drop_rc}"
echo "RANGE_QUERY_BY_GPUVA_AFTER_DROP"
cat "\${tmpdir}/range_by_gpuva_after_drop.log"

if [[ "\${fresh_range_is_new}" == "1" ]]; then
    if [[ "\${after_drop_rc}" == "0" ]]; then
        echo "GPU-VA lookup should have failed after dropping the fresh adopted range" >&2
        exit 1
    fi
else
    if [[ "\${after_drop_rc}" != "0" ]]; then
        echo "GPU-VA lookup should have fallen back to the persistent baseline range after drop" >&2
        exit 1
    fi
    range_after_drop_object_id=\$(awk -F= '/^range_sgx_object_id=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_drop.log")
    range_after_drop_role=\$(awk -F= '/^range_sgx_object_role_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_drop.log")
    range_after_drop_class=\$(awk -F= '/^range_sgx_object_class_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_drop.log")
    range_after_drop_gpu_use=\$(awk -F= '/^range_sgx_object_gpu_use_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_drop.log")
    range_after_drop_context=\$(awk -F= '/^range_sgx_object_expected_context_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_drop.log")
    range_after_drop_submit_ref_count=\$(awk -F= '/^range_sgx_submit_ref_count=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_drop.log")
    range_after_drop_first_submit_id=\$(awk -F= '/^range_sgx_first_submit_id=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_drop.log")
    range_after_drop_first_submit_slot_name=\$(awk -F= '/^range_sgx_first_submit_slot_name=/{print \$2; exit}' "\${tmpdir}/range_by_gpuva_after_drop.log")
    if [[ "\${range_after_drop_object_id}" != "0" ]]; then
        echo "GPU-VA lookup after drop still reported SGX ownership: \${range_after_drop_object_id}" >&2
        exit 1
    fi
    if [[ "\${range_after_drop_role}" != "generic" || "\${range_after_drop_class}" != "generic" || "\${range_after_drop_gpu_use}" != "generic" || "\${range_after_drop_context}" != "auto" ]]; then
        echo "GPU-VA lookup after drop still reported SGX object metadata" >&2
        exit 1
    fi
    if [[ "\${range_after_drop_submit_ref_count}" != "0" || "\${range_after_drop_first_submit_id}" != "0" || "\${range_after_drop_first_submit_slot_name}" != "none" ]]; then
        echo "GPU-VA lookup after drop still reported SGX submit references" >&2
        exit 1
    fi
fi
EOF
