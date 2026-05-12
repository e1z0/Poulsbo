#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_dir="${TARGET_DIR:-/root/poulsbo-workloads}"

target_probe_bin="${TARGET_PROBE_BIN:-${target_dir}/drm_probe}"
target_query_bin="${TARGET_QUERY_BIN:-${target_dir}/psb_sgx_lab_query}"
target_submit_op_bin="${TARGET_SUBMIT_OP_BIN:-${target_dir}/psb_sgx_lab_submit_op}"
target_object_op_bin="${TARGET_OBJECT_OP_BIN:-${target_dir}/psb_sgx_lab_object_op}"
target_object_query_bin="${TARGET_OBJECT_QUERY_BIN:-${target_dir}/psb_sgx_lab_object_query}"
target_range_query_bin="${TARGET_RANGE_QUERY_BIN:-${target_dir}/psb_sgx_lab_range_query}"
target_gtt_state_bin="${TARGET_GTT_STATE_BIN:-${target_dir}/psb_sgx_lab_gtt_state_query}"
remote_shell_trace="${REMOTE_SHELL_TRACE:-0}"
remote_lock_file="${REMOTE_LOCK_FILE:-/run/poulsbo-sgx-lab.lock}"
remote_lock_wait_secs="${REMOTE_LOCK_WAIT_SECS:-600}"
remote_keep_tmpdir_on_failure="${REMOTE_KEEP_TMPDIR_ON_FAILURE:-0}"
autoload_gma500_if_missing="${AUTOLOAD_GMA500_IF_MISSING:-1}"
gma500_module_name="${GMA500_MODULE_NAME:-gma500_gfx}"
drm_node_wait_secs="${DRM_NODE_WAIT_SECS:-20}"

target_probe_stage="${TARGET_PROBE_STAGE:-/root/drm_probe.$$.$RANDOM.new}"
target_query_stage="${TARGET_QUERY_STAGE:-/root/psb_sgx_lab_query.$$.$RANDOM.new}"
target_submit_op_stage="${TARGET_SUBMIT_OP_STAGE:-/root/psb_sgx_lab_submit_op.$$.$RANDOM.new}"
target_object_op_stage="${TARGET_OBJECT_OP_STAGE:-/root/psb_sgx_lab_object_op.$$.$RANDOM.new}"
target_object_query_stage="${TARGET_OBJECT_QUERY_STAGE:-/root/psb_sgx_lab_object_query.$$.$RANDOM.new}"
target_range_query_stage="${TARGET_RANGE_QUERY_STAGE:-/root/psb_sgx_lab_range_query.$$.$RANDOM.new}"
target_gtt_state_stage="${TARGET_GTT_STATE_STAGE:-/root/psb_sgx_lab_gtt_state_query.$$.$RANDOM.new}"

local_probe_bin="${LOCAL_PROBE_BIN:-$(pwd)/workloads/drm_probe}"
local_query_bin="${LOCAL_QUERY_BIN:-$(pwd)/workloads/psb_sgx_lab_query}"
local_submit_op_bin="${LOCAL_SUBMIT_OP_BIN:-$(pwd)/workloads/psb_sgx_lab_submit_op}"
local_object_op_bin="${LOCAL_OBJECT_OP_BIN:-$(pwd)/workloads/psb_sgx_lab_object_op}"
local_object_query_bin="${LOCAL_OBJECT_QUERY_BIN:-$(pwd)/workloads/psb_sgx_lab_object_query}"
local_range_query_bin="${LOCAL_RANGE_QUERY_BIN:-$(pwd)/workloads/psb_sgx_lab_range_query}"
local_gtt_state_bin="${LOCAL_GTT_STATE_BIN:-$(pwd)/workloads/psb_sgx_lab_gtt_state_query}"

target_node="${TARGET_NODE:-/dev/dri/card0}"
hold_seconds="${HOLD_SECONDS:-15}"
probe_count="${PROBE_COUNT:-5}"
range_scan_max="${RANGE_SCAN_MAX:-128}"
report_offset="${REPORT_OFFSET:-0x120}"
manifest_offset="${MANIFEST_OFFSET:-0x180}"
descriptor_offset="${DESCRIPTOR_OFFSET:-0x200}"
plan_offset="${PLAN_OFFSET:-0x300}"
stop_after_stage="${STOP_AFTER_STAGE:-}"

is_valid_stop_stage() {
    case "$1" in
        ""|sync|owner|report|manifest|descriptor|descriptor_op|descriptor_query|plan|plan_op|plan_query|plan_query_fetch|plan_query_basic|plan_query_offset|plan_query_config|plan_query_snapshot|prepare|prepare_op|prepare_query|arm|emit|validate|launch|issue|running|complete)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

if ! is_valid_stop_stage "${stop_after_stage}"; then
    echo "STOP_AFTER_STAGE must be one of: sync owner report manifest descriptor descriptor_op descriptor_query plan plan_op plan_query plan_query_fetch plan_query_basic plan_query_offset plan_query_config plan_query_snapshot prepare prepare_op prepare_query arm emit validate launch issue running complete" >&2
    exit 1
fi

if [[ "${probe_count}" -lt 5 ]]; then
    echo "PROBE_COUNT must be at least 5" >&2
    exit 1
fi

if [[ "${remote_shell_trace}" != "0" && "${remote_shell_trace}" != "1" ]]; then
    echo "REMOTE_SHELL_TRACE must be 0 or 1" >&2
    exit 1
fi

if [[ "${remote_lock_wait_secs}" -lt 1 ]]; then
    echo "REMOTE_LOCK_WAIT_SECS must be at least 1" >&2
    exit 1
fi

if [[ "${remote_keep_tmpdir_on_failure}" != "0" && "${remote_keep_tmpdir_on_failure}" != "1" ]]; then
    echo "REMOTE_KEEP_TMPDIR_ON_FAILURE must be 0 or 1" >&2
    exit 1
fi

for bin in "$local_probe_bin" "$local_query_bin" "$local_submit_op_bin" \
    "$local_object_op_bin" "$local_object_query_bin" "$local_range_query_bin" \
    "$local_gtt_state_bin"; do
    if [[ ! -f "$bin" ]]; then
        echo "Local binary not found: $bin" >&2
        exit 1
    fi
done

scp "$local_probe_bin" "${target_user}@${target_host}:${target_probe_stage}"
scp "$local_query_bin" "${target_user}@${target_host}:${target_query_stage}"
scp "$local_submit_op_bin" "${target_user}@${target_host}:${target_submit_op_stage}"
scp "$local_object_op_bin" "${target_user}@${target_host}:${target_object_op_stage}"
scp "$local_object_query_bin" "${target_user}@${target_host}:${target_object_query_stage}"
scp "$local_range_query_bin" "${target_user}@${target_host}:${target_range_query_stage}"
scp "$local_gtt_state_bin" "${target_user}@${target_host}:${target_gtt_state_stage}"

ssh "${target_user}@${target_host}" "bash -s" <<EOF
set -euo pipefail
remote_shell_trace="${remote_shell_trace}"
remote_lock_file="${remote_lock_file}"
remote_lock_wait_secs="${remote_lock_wait_secs}"
remote_keep_tmpdir_on_failure="${remote_keep_tmpdir_on_failure}"
autoload_gma500_if_missing="${autoload_gma500_if_missing}"
gma500_module_name="${gma500_module_name}"
drm_node_wait_secs="${drm_node_wait_secs}"
if [[ "\${remote_shell_trace}" == "1" ]]; then
    set -x
fi

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
install -m 0755 -T "${target_query_stage}" "${target_query_bin}"
install -m 0755 -T "${target_submit_op_stage}" "${target_submit_op_bin}"
install -m 0755 -T "${target_object_op_stage}" "${target_object_op_bin}"
install -m 0755 -T "${target_object_query_stage}" "${target_object_query_bin}"
install -m 0755 -T "${target_range_query_stage}" "${target_range_query_bin}"
install -m 0755 -T "${target_gtt_state_stage}" "${target_gtt_state_bin}"
rm -f "${target_probe_stage}" "${target_query_stage}" \
    "${target_submit_op_stage}" "${target_object_op_stage}" \
    "${target_object_query_stage}" "${target_range_query_stage}" \
    "${target_gtt_state_stage}"

tmpdir=\$(mktemp -d)
probe_pids=()
submit_id=""
submit_dropped=0
object_ids=()
range_ids=()
failed_step=""
failed_rc=0

cleanup() {
    local pid object_id
    if [[ -n "\${submit_id}" && "\${submit_dropped}" != "1" ]]; then
        "${target_submit_op_bin}" "${target_node}" drop "\${submit_id}" >/dev/null 2>&1 || true
    fi
    if [[ "\${#object_ids[@]}" -gt 0 ]]; then
        "${target_object_op_bin}" "${target_node}" cleanup >/dev/null 2>&1 || true
    fi
    for pid in "\${probe_pids[@]:-}"; do
        if [[ -n "\${pid}" ]] && kill -0 "\${pid}" 2>/dev/null; then
            kill "\${pid}" 2>/dev/null || true
            wait "\${pid}" 2>/dev/null || true
        fi
    done
    if [[ -n "\${failed_step}" && "\${remote_keep_tmpdir_on_failure}" == "1" ]]; then
        echo "REMOTE_TMPDIR_PRESERVED=\${tmpdir}" >&2
    else
        rm -rf "\${tmpdir}"
    fi
}
trap cleanup EXIT

record_failure() {
    local step_name="\$1"
    local step_rc="\$2"
    local step_file="\${3:-}"

    failed_step="\${step_name}"
    failed_rc="\${step_rc}"
    echo "FAILED_STEP=\${step_name}" >&2
    echo "FAILED_RC=\${step_rc}" >&2
    if [[ -n "\${step_file}" && -f "\${step_file}" ]]; then
        echo "FAILED_LOG=\${step_file}" >&2
        cat "\${step_file}" >&2 || true
    fi
}

run_capture() {
    local step_name="\$1"
    local outfile="\$2"
    shift 2
    local step_rc

    set +e
    "\$@" > "\${outfile}" 2>&1
    step_rc=\$?
    set -e
    if [[ "\${step_rc}" -ne 0 ]]; then
        record_failure "\${step_name}" "\${step_rc}" "\${outfile}"
        exit "\${step_rc}"
    fi
}

require_line() {
    local step_name="\$1"
    local infile="\$2"
    local pattern="\$3"

    if ! grep -q -- "\${pattern}" "\${infile}"; then
        record_failure "\${step_name}" 1 "\${infile}"
        exit 1
    fi
}

ensure_target_drm_node() {
    local waited=0

    if [[ -e "${target_node}" ]]; then
        return 0
    fi

    echo "TARGET_DRM_NODE_MISSING=${target_node}" >&2

    if [[ "\${autoload_gma500_if_missing}" != "1" ]]; then
        echo "Auto-loading \${gma500_module_name} is disabled" >&2
        return 1
    fi

    echo "AUTOLOAD_GMA500_MODULE=\${gma500_module_name}" >&2
    if ! modprobe "\${gma500_module_name}" >&2; then
        echo "Failed to load \${gma500_module_name}" >&2
        return 1
    fi

    while [[ "\${waited}" -lt "\${drm_node_wait_secs}" ]]; do
        if [[ -e "${target_node}" ]]; then
            echo "TARGET_DRM_NODE_READY=${target_node}" >&2
            return 0
        fi
        sleep 1
        waited=\$((waited + 1))
    done

    echo "Timed out waiting for ${target_node} after loading \${gma500_module_name}" >&2
    return 1
}

mark_stage_complete() {
    local stage_name="\$1"
    shift

    echo "STAGE_OK=\${stage_name}"
    if [[ "${stop_after_stage}" == "\${stage_name}" ]]; then
        echo "STOP_AFTER_STAGE_REACHED=\${stage_name}"
        exit 0
    fi

    for legacy_stage in "\$@"; do
        if [[ "${stop_after_stage}" == "\${legacy_stage}" ]]; then
            echo "STOP_AFTER_STAGE_REACHED=\${stage_name}"
            echo "STOP_AFTER_STAGE_ALIAS=\${legacy_stage}"
            exit 0
        fi
    done
}

extract_value_from_file() {
    local infile="\$1"
    local key="\$2"

    awk -F= -v want="\${key}" '\$1 == want { print substr(\$0, index(\$0, "=") + 1); exit }' "\${infile}"
}

assert_manifest_snapshot_file() {
    local infile="\$1"
    local expected_stage="\$2"
    local expected_stage_count="\$3"
    local expected_owner_state="\$4"
    local expected_queue_count="\$5"
    local expected_dependency_active="\$6"
    local expected_pds_gpu_va="\$7"
    local expected_use_gpu_va="\$8"
    local expected_target_gpu_va="\$9"
    local expected_status_gpu_va="\${10}"

    grep -q "^submit_manifest_offset=${manifest_offset}\$" "\${infile}"
    grep -q '^submit_manifest_config_count=1$' "\${infile}"
    grep -q "^submit_manifest_stage_name=\${expected_stage}\$" "\${infile}"
    grep -q "^submit_manifest_stage_count=\${expected_stage_count}\$" "\${infile}"
    grep -q "^submit_manifest_owner_state_name=\${expected_owner_state}\$" "\${infile}"
    grep -q "^submit_manifest_queue_packet_count=\${expected_queue_count}\$" "\${infile}"
    grep -q "^submit_manifest_dependency_active_count=\${expected_dependency_active}\$" "\${infile}"
    grep -q "^submit_manifest_pds_gpu_va32=\${expected_pds_gpu_va}\$" "\${infile}"
    grep -q "^submit_manifest_use_gpu_va32=\${expected_use_gpu_va}\$" "\${infile}"
    grep -q "^submit_manifest_target_gpu_va32=\${expected_target_gpu_va}\$" "\${infile}"
    grep -q "^submit_manifest_status_gpu_va32=\${expected_status_gpu_va}\$" "\${infile}"
}

assert_descriptor_snapshot_file() {
    local infile="\$1"
    local expected_stage="\$2"
    local expected_stage_count="\$3"
    local expected_owner_state="\$4"
    local expected_issued_packets="\$5"
    local expected_completed_packets="\$6"
    local expected_queue_count="\$7"
    local expected_dependency_active="\$8"
    local expected_control_gpu_va="\$9"
    local expected_pds_gpu_va="\${10}"
    local expected_use_gpu_va="\${11}"
    local expected_target_gpu_va="\${12}"
    local expected_status_gpu_va="\${13}"

    grep -q "^submit_descriptor_offset=${descriptor_offset}\$" "\${infile}"
    grep -q '^submit_descriptor_config_count=1$' "\${infile}"
    grep -q "^submit_descriptor_stage_name=\${expected_stage}\$" "\${infile}"
    grep -q "^submit_descriptor_stage_count=\${expected_stage_count}\$" "\${infile}"
    grep -q "^submit_descriptor_owner_state_name=\${expected_owner_state}\$" "\${infile}"
    grep -q "^submit_descriptor_issued_packets=\${expected_issued_packets}\$" "\${infile}"
    grep -q "^submit_descriptor_completed_packets=\${expected_completed_packets}\$" "\${infile}"
    grep -q "^submit_descriptor_queue_packet_count=\${expected_queue_count}\$" "\${infile}"
    grep -q "^submit_descriptor_dependency_active_count=\${expected_dependency_active}\$" "\${infile}"
    grep -q "^submit_descriptor_report_offset=${report_offset}\$" "\${infile}"
    grep -q "^submit_descriptor_manifest_offset=${manifest_offset}\$" "\${infile}"
    grep -q "^submit_descriptor_control_gpu_va32=\${expected_control_gpu_va}\$" "\${infile}"
    grep -q "^submit_descriptor_pds_gpu_va32=\${expected_pds_gpu_va}\$" "\${infile}"
    grep -q "^submit_descriptor_use_gpu_va32=\${expected_use_gpu_va}\$" "\${infile}"
    grep -q "^submit_descriptor_target_gpu_va32=\${expected_target_gpu_va}\$" "\${infile}"
    grep -q "^submit_descriptor_status_gpu_va32=\${expected_status_gpu_va}\$" "\${infile}"
}

assert_plan_snapshot_file() {
    local infile="\$1"
    local expected_stage="\$2"
    local expected_stage_count="\$3"
    local expected_source="\$4"
    local expected_owner_state="\$5"
    local expected_issued_packets="\$6"
    local expected_completed_packets="\$7"
    local expected_dependency_active="\$8"
    local expected_queue_count="\$9"
    local expected_queue_epoch="\${10}"
    local expected_control_gpu_va="\${11}"
    local expected_pds_gpu_va="\${12}"
    local expected_use_gpu_va="\${13}"
    local expected_target_gpu_va="\${14}"
    local expected_status_gpu_va="\${15}"

    grep -q "^submit_plan_offset=${plan_offset}\$" "\${infile}"
    grep -q '^submit_plan_config_count=1$' "\${infile}"
    grep -q "^submit_plan_stage_name=\${expected_stage}\$" "\${infile}"
    grep -q "^submit_plan_stage_count=\${expected_stage_count}\$" "\${infile}"
    grep -q "^submit_plan_source_name=\${expected_source}\$" "\${infile}"
    grep -q "^submit_plan_owner_state_name=\${expected_owner_state}\$" "\${infile}"
    grep -q "^submit_plan_issued_packets=\${expected_issued_packets}\$" "\${infile}"
    grep -q "^submit_plan_completed_packets=\${expected_completed_packets}\$" "\${infile}"
    grep -q "^submit_plan_dependency_active_count=\${expected_dependency_active}\$" "\${infile}"
    grep -q "^submit_plan_queue_packet_count=\${expected_queue_count}\$" "\${infile}"
    grep -q "^submit_plan_queue_epoch=\${expected_queue_epoch}\$" "\${infile}"
    grep -q "^submit_plan_report_offset=${report_offset}\$" "\${infile}"
    grep -q "^submit_plan_manifest_offset=${manifest_offset}\$" "\${infile}"
    grep -q "^submit_plan_descriptor_offset=${descriptor_offset}\$" "\${infile}"
    grep -q "^submit_plan_control_gpu_va32=\${expected_control_gpu_va}\$" "\${infile}"
    grep -q "^submit_plan_pds_gpu_va32=\${expected_pds_gpu_va}\$" "\${infile}"
    grep -q "^submit_plan_use_gpu_va32=\${expected_use_gpu_va}\$" "\${infile}"
    grep -q "^submit_plan_target_gpu_va32=\${expected_target_gpu_va}\$" "\${infile}"
    grep -q "^submit_plan_status_gpu_va32=\${expected_status_gpu_va}\$" "\${infile}"
}

"${target_submit_op_bin}" "${target_node}" cleanup >/dev/null 2>&1 || true
"${target_object_op_bin}" "${target_node}" cleanup >/dev/null 2>&1 || true

ensure_target_drm_node

run_capture gtt_before "\${tmpdir}/gtt_before.txt" \
    "${target_gtt_state_bin}" "${target_node}"

for idx in \$(seq 1 "${probe_count}"); do
    "${target_probe_bin}" "${target_node}" "${hold_seconds}" > "\${tmpdir}/drm_probe_\${idx}.log" 2>&1 &
    probe_pids+=("\$!")
done

for _ in \$(seq 1 $((hold_seconds + 5))); do
    ready_count=\$(grep -h '^drm_hold_seconds=' "\${tmpdir}"/drm_probe_*.log 2>/dev/null | wc -l || true)
    if [[ "\${ready_count}" -ge "${probe_count}" ]]; then
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
        range_ids+=("\${range_id_candidate}")
        printf '%s\n' "\${range_out}" > "\${tmpdir}/range_\${range_id_candidate}_before_adopt.txt"
        if [[ "\${#range_ids[@]}" -ge "${probe_count}" ]]; then
            break
        fi
    fi
done

if [[ "\${#range_ids[@]}" -lt "${probe_count}" ]]; then
    echo "Expected ${probe_count} fresh SGX-adoptable ranges, found \${#range_ids[@]}" >&2
    exit 1
fi

for range_id in "\${range_ids[@]}"; do
    run_capture "object_adopt_\${range_id}" "\${tmpdir}/object_adopt_\${range_id}.txt" \
        "${target_object_op_bin}" "${target_node}" adopt "\${range_id}"
    object_id=\$(awk -F= '/^object_resolved_id=/{ print \$2; exit }' "\${tmpdir}/object_adopt_\${range_id}.txt")
    if [[ -z "\${object_id}" || "\${object_id}" == "0" ]]; then
        echo "Failed to resolve adopted object ID for range \${range_id}" >&2
        cat "\${tmpdir}/object_adopt_\${range_id}.txt" >&2
        exit 1
    fi
    object_ids+=("\${object_id}")
done

object_control="\${object_ids[0]}"
object_pds="\${object_ids[1]}"
object_use="\${object_ids[2]}"
object_target="\${object_ids[3]}"
object_status="\${object_ids[4]}"
range_control="\${range_ids[0]}"
range_status="\${range_ids[4]}"

run_capture object_role_control "\${tmpdir}/object_role_control.txt" \
    "${target_object_op_bin}" "${target_node}" role "\${object_control}" control
run_capture object_role_pds "\${tmpdir}/object_role_pds.txt" \
    "${target_object_op_bin}" "${target_node}" role "\${object_pds}" pds
run_capture object_role_use "\${tmpdir}/object_role_use.txt" \
    "${target_object_op_bin}" "${target_node}" role "\${object_use}" use
run_capture object_role_target "\${tmpdir}/object_role_target.txt" \
    "${target_object_op_bin}" "${target_node}" role "\${object_target}" target
run_capture object_role_status "\${tmpdir}/object_role_status.txt" \
    "${target_object_op_bin}" "${target_node}" role "\${object_status}" status

run_capture object_class_control "\${tmpdir}/object_class_control.txt" \
    "${target_object_op_bin}" "${target_node}" class "\${object_control}" packet
run_capture object_class_pds "\${tmpdir}/object_class_pds.txt" \
    "${target_object_op_bin}" "${target_node}" class "\${object_pds}" code
run_capture object_class_use "\${tmpdir}/object_class_use.txt" \
    "${target_object_op_bin}" "${target_node}" class "\${object_use}" code
run_capture object_class_target "\${tmpdir}/object_class_target.txt" \
    "${target_object_op_bin}" "${target_node}" class "\${object_target}" surface
run_capture object_class_status "\${tmpdir}/object_class_status.txt" \
    "${target_object_op_bin}" "${target_node}" class "\${object_status}" state

run_capture object_gpu_use_control "\${tmpdir}/object_gpu_use_control.txt" \
    "${target_object_op_bin}" "${target_node}" gpu_use "\${object_control}" control_ccb
run_capture object_gpu_use_pds "\${tmpdir}/object_gpu_use_pds.txt" \
    "${target_object_op_bin}" "${target_node}" gpu_use "\${object_pds}" pds_code
run_capture object_gpu_use_use "\${tmpdir}/object_gpu_use_use.txt" \
    "${target_object_op_bin}" "${target_node}" gpu_use "\${object_use}" use_code
run_capture object_gpu_use_target "\${tmpdir}/object_gpu_use_target.txt" \
    "${target_object_op_bin}" "${target_node}" gpu_use "\${object_target}" render_target
run_capture object_gpu_use_status "\${tmpdir}/object_gpu_use_status.txt" \
    "${target_object_op_bin}" "${target_node}" gpu_use "\${object_status}" status_block

for object_id in "\${object_ids[@]}"; do
    run_capture "object_context_\${object_id}" "\${tmpdir}/object_context_\${object_id}.txt" \
        "${target_object_op_bin}" "${target_node}" context "\${object_id}" pf
    run_capture "object_pf_bind_\${object_id}" "\${tmpdir}/object_pf_bind_\${object_id}.txt" \
        "${target_object_op_bin}" "${target_node}" pf_bind "\${object_id}"
done

run_capture object_control_after_setup "\${tmpdir}/object_control_after_setup.txt" \
    "${target_object_query_bin}" "${target_node}" id "\${object_control}"
run_capture object_status_after_setup "\${tmpdir}/object_status_after_setup.txt" \
    "${target_object_query_bin}" "${target_node}" id "\${object_status}"
run_capture range_control_after_setup "\${tmpdir}/range_control_after_setup.txt" \
    "${target_range_query_bin}" "${target_node}" id "\${range_control}" gem
run_capture gtt_after_object_setup "\${tmpdir}/gtt_after_object_setup.txt" \
    "${target_gtt_state_bin}" "${target_node}"

require_line object_control_after_setup "\${tmpdir}/object_control_after_setup.txt" '^object_role_name=control$'
require_line object_control_after_setup "\${tmpdir}/object_control_after_setup.txt" '^object_class_name=packet$'
require_line object_control_after_setup "\${tmpdir}/object_control_after_setup.txt" '^object_gpu_use_name=control_ccb$'
require_line object_control_after_setup "\${tmpdir}/object_control_after_setup.txt" '^object_expected_context_name=pf$'
require_line object_control_after_setup "\${tmpdir}/object_control_after_setup.txt" '^object_pf_bind_count=1$'
require_line object_status_after_setup "\${tmpdir}/object_status_after_setup.txt" '^object_role_name=status$'
require_line object_status_after_setup "\${tmpdir}/object_status_after_setup.txt" '^object_class_name=state$'
require_line object_status_after_setup "\${tmpdir}/object_status_after_setup.txt" '^object_gpu_use_name=status_block$'
require_line object_status_after_setup "\${tmpdir}/object_status_after_setup.txt" '^object_expected_context_name=pf$'
require_line object_status_after_setup "\${tmpdir}/object_status_after_setup.txt" '^object_pf_bind_count=1$'
require_line range_control_after_setup "\${tmpdir}/range_control_after_setup.txt" "^range_sgx_object_id=\${object_control}\$"
require_line range_control_after_setup "\${tmpdir}/range_control_after_setup.txt" '^range_pf_bind_count=1$'
require_line gtt_after_object_setup "\${tmpdir}/gtt_after_object_setup.txt" '^gtt_state_range_sgx_object_count=5$'
require_line gtt_after_object_setup "\${tmpdir}/gtt_after_object_setup.txt" '^gtt_state_range_pf_bound_count=5$'

run_capture submit_op_create_pf "\${tmpdir}/submit_op_create_pf.txt" \
    "${target_submit_op_bin}" "${target_node}" create pf
submit_id=\$(awk -F= '/^submit_resolved_id=/{ print \$2; exit }' "\${tmpdir}/submit_op_create_pf.txt")

if [[ -z "\${submit_id}" || "\${submit_id}" == "0" ]]; then
    echo "Failed to identify SGX submit ID from ioctl create" >&2
    cat "\${tmpdir}/submit_op_create_pf.txt" >&2
    exit 1
fi

run_capture submit_op_focus_id "\${tmpdir}/submit_op_focus_id.txt" \
    "${target_submit_op_bin}" "${target_node}" focus "\${submit_id}"
run_capture submit_op_attach_control_id "\${tmpdir}/submit_op_attach_control_id.txt" \
    "${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" control "\${object_control}"
run_capture submit_op_attach_pds_id "\${tmpdir}/submit_op_attach_pds_id.txt" \
    "${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" pds "\${object_pds}"
run_capture submit_op_attach_use_id "\${tmpdir}/submit_op_attach_use_id.txt" \
    "${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" use "\${object_use}"
run_capture submit_op_attach_target_id "\${tmpdir}/submit_op_attach_target_id.txt" \
    "${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" target "\${object_target}"
run_capture submit_op_attach_status_focus "\${tmpdir}/submit_op_attach_status_focus.txt" \
    "${target_submit_op_bin}" "${target_node}" attach focus status "\${object_status}"

run_capture query_by_id_after_attach "\${tmpdir}/query_by_id_after_attach.txt" \
    "${target_query_bin}" "${target_node}" "\${submit_id}"
run_capture query_focus_after_attach "\${tmpdir}/query_focus_after_attach.txt" \
    "${target_query_bin}" "${target_node}" focus
require_line query_by_id_after_attach "\${tmpdir}/query_by_id_after_attach.txt" "^submit_control_object_id=\${object_control}\$"
require_line query_by_id_after_attach "\${tmpdir}/query_by_id_after_attach.txt" "^submit_pds_object_id=\${object_pds}\$"
require_line query_by_id_after_attach "\${tmpdir}/query_by_id_after_attach.txt" "^submit_use_object_id=\${object_use}\$"
require_line query_by_id_after_attach "\${tmpdir}/query_by_id_after_attach.txt" "^submit_target_object_id=\${object_target}\$"
require_line query_by_id_after_attach "\${tmpdir}/query_by_id_after_attach.txt" "^submit_status_object_id=\${object_status}\$"

run_capture submit_op_detach_status_focus "\${tmpdir}/submit_op_detach_status_focus.txt" \
    "${target_submit_op_bin}" "${target_node}" detach focus status
run_capture query_by_id_after_detach_status "\${tmpdir}/query_by_id_after_detach_status.txt" \
    "${target_query_bin}" "${target_node}" "\${submit_id}"
require_line query_by_id_after_detach_status "\${tmpdir}/query_by_id_after_detach_status.txt" '^submit_status_object_id=0$'

run_capture submit_op_reattach_status_id "\${tmpdir}/submit_op_reattach_status_id.txt" \
    "${target_submit_op_bin}" "${target_node}" attach "\${submit_id}" status "\${object_status}"
run_capture query_by_id_after_reattach_status "\${tmpdir}/query_by_id_after_reattach_status.txt" \
    "${target_query_bin}" "${target_node}" "\${submit_id}"
require_line query_by_id_after_reattach_status "\${tmpdir}/query_by_id_after_reattach_status.txt" "^submit_status_object_id=\${object_status}\$"

run_capture submit_op_kick_focus "\${tmpdir}/submit_op_kick_focus.txt" \
    "${target_submit_op_bin}" "${target_node}" kick focus ta 0x20 0x1 1 1 0
run_capture query_by_id_after_kick "\${tmpdir}/query_by_id_after_kick.txt" \
    "${target_query_bin}" "${target_node}" "\${submit_id}"
require_line query_by_id_after_kick "\${tmpdir}/query_by_id_after_kick.txt" '^submit_cmd_type_name=ta$'
require_line query_by_id_after_kick "\${tmpdir}/query_by_id_after_kick.txt" '^submit_ccb_offset=0x20$'
require_line query_by_id_after_kick "\${tmpdir}/query_by_id_after_kick.txt" '^submit_ctrl_flags=0x1$'
require_line query_by_id_after_kick "\${tmpdir}/query_by_id_after_kick.txt" '^submit_num_ta_status_vals=1$'
require_line query_by_id_after_kick "\${tmpdir}/query_by_id_after_kick.txt" '^submit_num_3d_status_vals=1$'

run_capture submit_op_ccb_focus "\${tmpdir}/submit_op_ccb_focus.txt" \
    "${target_submit_op_bin}" "${target_node}" ccb focus 0x20 0x0 0x1000 0x0 0xc0de0001 0xc0de0002
run_capture query_by_id_after_ccb "\${tmpdir}/query_by_id_after_ccb.txt" \
    "${target_query_bin}" "${target_node}" "\${submit_id}"
require_line query_by_id_after_ccb "\${tmpdir}/query_by_id_after_ccb.txt" '^submit_ccb_write_offset=0x20$'
require_line query_by_id_after_ccb "\${tmpdir}/query_by_id_after_ccb.txt" '^submit_ccb_read_offset=0x0$'
require_line query_by_id_after_ccb "\${tmpdir}/query_by_id_after_ccb.txt" '^submit_ccb_service_address=0x1000$'
require_line query_by_id_after_ccb "\${tmpdir}/query_by_id_after_ccb.txt" '^submit_ccb_data0=0xc0de0001$'
require_line query_by_id_after_ccb "\${tmpdir}/query_by_id_after_ccb.txt" '^submit_ccb_data1=0xc0de0002$'

run_capture submit_op_queue_focus "\${tmpdir}/submit_op_queue_focus.txt" \
    "${target_submit_op_bin}" "${target_node}" queue focus 0x20 0x40 0x20 1
run_capture query_by_id_after_queue "\${tmpdir}/query_by_id_after_queue.txt" \
    "${target_query_bin}" "${target_node}" "\${submit_id}"
require_line query_by_id_after_queue "\${tmpdir}/query_by_id_after_queue.txt" '^submit_queue_base_offset=0x20$'
require_line query_by_id_after_queue "\${tmpdir}/query_by_id_after_queue.txt" '^submit_queue_size=0x40$'
require_line query_by_id_after_queue "\${tmpdir}/query_by_id_after_queue.txt" '^submit_queue_packet_size=0x20$'
require_line query_by_id_after_queue "\${tmpdir}/query_by_id_after_queue.txt" '^submit_queue_packet_count=1$'

run_capture submit_op_status_focus "\${tmpdir}/submit_op_status_focus.txt" \
    "${target_submit_op_bin}" "${target_node}" status focus 0x80 0xc0 0x4 0x5a500000
run_capture query_by_id_after_status "\${tmpdir}/query_by_id_after_status.txt" \
    "${target_query_bin}" "${target_node}" "\${submit_id}"
require_line query_by_id_after_status "\${tmpdir}/query_by_id_after_status.txt" '^submit_status_ta_offset=0x80$'
require_line query_by_id_after_status "\${tmpdir}/query_by_id_after_status.txt" '^submit_status_3d_offset=0xc0$'
require_line query_by_id_after_status "\${tmpdir}/query_by_id_after_status.txt" '^submit_status_stride=0x4$'
require_line query_by_id_after_status "\${tmpdir}/query_by_id_after_status.txt" '^submit_status_value_seed=0x5a500000$'

run_capture submit_op_sync_focus "\${tmpdir}/submit_op_sync_focus.txt" \
    "${target_submit_op_bin}" "${target_node}" sync focus 0x20 0x40 0x41 0x1
run_capture query_by_id_after_sync "\${tmpdir}/query_by_id_after_sync.txt" \
    "${target_query_bin}" "${target_node}" "\${submit_id}"
require_line query_by_id_after_sync "\${tmpdir}/query_by_id_after_sync.txt" '^submit_sync_offset=0x20$'
require_line query_by_id_after_sync "\${tmpdir}/query_by_id_after_sync.txt" '^submit_sync_complete_seq=0x40$'
require_line query_by_id_after_sync "\${tmpdir}/query_by_id_after_sync.txt" '^submit_sync_pending_seq=0x41$'
require_line query_by_id_after_sync "\${tmpdir}/query_by_id_after_sync.txt" '^submit_sync_flags=0x1$'
mark_stage_complete sync

"${target_submit_op_bin}" "${target_node}" owner focus 0x40 queued 0x101 0x100 > "\${tmpdir}/submit_op_owner_focus.txt"
"${target_query_bin}" "${target_node}" "\${submit_id}" > "\${tmpdir}/query_by_id_after_owner.txt"
grep -q '^submit_owner_offset=0x40$' "\${tmpdir}/query_by_id_after_owner.txt"
grep -q '^submit_owner_state_name=queued$' "\${tmpdir}/query_by_id_after_owner.txt"
grep -q '^submit_owner_issue_cookie=0x101$' "\${tmpdir}/query_by_id_after_owner.txt"
grep -q '^submit_owner_complete_cookie=0x100$' "\${tmpdir}/query_by_id_after_owner.txt"
mark_stage_complete owner

"${target_submit_op_bin}" "${target_node}" report focus "${report_offset}" > "\${tmpdir}/submit_op_report_focus.txt"
"${target_query_bin}" "${target_node}" "\${submit_id}" > "\${tmpdir}/query_by_id_after_report.txt"
grep -q "^submit_report_offset=${report_offset}\$" "\${tmpdir}/query_by_id_after_report.txt"
grep -q '^submit_report_config_count=1$' "\${tmpdir}/query_by_id_after_report.txt"
grep -q '^submit_report_stage_name=none$' "\${tmpdir}/query_by_id_after_report.txt"
grep -q '^submit_report_owner_state_name=queued$' "\${tmpdir}/query_by_id_after_report.txt"
grep -q '^submit_report_queue_packet_count=1$' "\${tmpdir}/query_by_id_after_report.txt"
grep -q '^submit_report_completed_packets=0$' "\${tmpdir}/query_by_id_after_report.txt"
grep -q '^submit_report_dependency_active_count=0$' "\${tmpdir}/query_by_id_after_report.txt"
mark_stage_complete report

"${target_submit_op_bin}" "${target_node}" manifest focus "${manifest_offset}" > "\${tmpdir}/submit_op_manifest_focus.txt"
"${target_query_bin}" "${target_node}" "\${submit_id}" > "\${tmpdir}/query_by_id_after_manifest.txt"
grep -q "^submit_manifest_offset=${manifest_offset}\$" "\${tmpdir}/query_by_id_after_manifest.txt"
grep -q '^submit_manifest_config_count=1$' "\${tmpdir}/query_by_id_after_manifest.txt"
control_gpu_va=\$(extract_value_from_file "\${tmpdir}/query_by_id_after_manifest.txt" "submit_control_gpu_va")
pds_gpu_va=\$(extract_value_from_file "\${tmpdir}/query_by_id_after_manifest.txt" "submit_pds_gpu_va")
use_gpu_va=\$(extract_value_from_file "\${tmpdir}/query_by_id_after_manifest.txt" "submit_use_gpu_va")
target_gpu_va=\$(extract_value_from_file "\${tmpdir}/query_by_id_after_manifest.txt" "submit_target_gpu_va")
status_gpu_va=\$(extract_value_from_file "\${tmpdir}/query_by_id_after_manifest.txt" "submit_status_gpu_va")
assert_manifest_snapshot_file "\${tmpdir}/query_by_id_after_manifest.txt" none 0 queued 1 0 \
    "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
mark_stage_complete manifest

run_capture submit_op_descriptor_focus "\${tmpdir}/submit_op_descriptor_focus.txt" \
    "${target_submit_op_bin}" "${target_node}" descriptor focus "${descriptor_offset}"
mark_stage_complete descriptor_op

run_capture query_by_id_after_descriptor "\${tmpdir}/query_by_id_after_descriptor.txt" \
    "${target_query_bin}" "${target_node}" "\${submit_id}"
require_line query_by_id_after_descriptor "\${tmpdir}/query_by_id_after_descriptor.txt" "^submit_descriptor_offset=${descriptor_offset}\$"
require_line query_by_id_after_descriptor "\${tmpdir}/query_by_id_after_descriptor.txt" '^submit_descriptor_config_count=1$'
assert_descriptor_snapshot_file "\${tmpdir}/query_by_id_after_descriptor.txt" none 0 queued 0 0 1 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
mark_stage_complete descriptor_query descriptor

run_capture submit_op_plan_focus "\${tmpdir}/submit_op_plan_focus.txt" \
    "${target_submit_op_bin}" "${target_node}" plan focus "${plan_offset}"
mark_stage_complete plan_op

run_capture query_by_id_after_plan "\${tmpdir}/query_by_id_after_plan.txt" \
    "${target_query_bin}" "${target_node}" "\${submit_id}"
mark_stage_complete plan_query_fetch
require_line query_by_id_after_plan "\${tmpdir}/query_by_id_after_plan.txt" "^submit_plan_offset=${plan_offset}\$"
mark_stage_complete plan_query_offset
require_line query_by_id_after_plan "\${tmpdir}/query_by_id_after_plan.txt" '^submit_plan_config_count=1$'
mark_stage_complete plan_query_config plan_query_basic
assert_plan_snapshot_file "\${tmpdir}/query_by_id_after_plan.txt" none 0 live queued 0 0 0 1 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
mark_stage_complete plan_query_snapshot plan_query plan

run_capture submit_op_prepare_focus "\${tmpdir}/submit_op_prepare_focus.txt" \
    "${target_submit_op_bin}" "${target_node}" prepare focus
mark_stage_complete prepare_op

run_capture query_by_id_after_prepare "\${tmpdir}/query_by_id_after_prepare.txt" \
    "${target_query_bin}" "${target_node}" "\${submit_id}"
require_line query_by_id_after_prepare "\${tmpdir}/query_by_id_after_prepare.txt" '^submit_prepare_count=1$'
require_line query_by_id_after_prepare "\${tmpdir}/query_by_id_after_prepare.txt" '^submit_arm_valid=0$'
require_line query_by_id_after_prepare "\${tmpdir}/query_by_id_after_prepare.txt" '^submit_report_stage_name=prepared$'
require_line query_by_id_after_prepare "\${tmpdir}/query_by_id_after_prepare.txt" '^submit_report_stage_count=1$'
require_line query_by_id_after_prepare "\${tmpdir}/query_by_id_after_prepare.txt" '^submit_report_owner_state_name=queued$'
require_line query_by_id_after_prepare "\${tmpdir}/query_by_id_after_prepare.txt" '^submit_report_queue_packet_count=1$'
require_line query_by_id_after_prepare "\${tmpdir}/query_by_id_after_prepare.txt" '^submit_report_completed_packets=0$'
assert_manifest_snapshot_file "\${tmpdir}/query_by_id_after_prepare.txt" prepared 1 queued 1 0 \
    "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_descriptor_snapshot_file "\${tmpdir}/query_by_id_after_prepare.txt" prepared 1 queued 0 0 1 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_plan_snapshot_file "\${tmpdir}/query_by_id_after_prepare.txt" prepared 1 live queued 0 0 0 1 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
ccb_pds_gpu_va32=\$(extract_value_from_file "\${tmpdir}/query_by_id_after_prepare.txt" "submit_ccb_pds_gpu_va32")
ccb_use_gpu_va32=\$(extract_value_from_file "\${tmpdir}/query_by_id_after_prepare.txt" "submit_ccb_use_gpu_va32")
ccb_target_gpu_va32=\$(extract_value_from_file "\${tmpdir}/query_by_id_after_prepare.txt" "submit_ccb_target_gpu_va32")
ccb_status_gpu_va32=\$(extract_value_from_file "\${tmpdir}/query_by_id_after_prepare.txt" "submit_ccb_status_gpu_va32")
if [[ "\${ccb_pds_gpu_va32}" != "\${pds_gpu_va}" || \
      "\${ccb_use_gpu_va32}" != "\${use_gpu_va}" || \
      "\${ccb_target_gpu_va32}" != "\${target_gpu_va}" || \
      "\${ccb_status_gpu_va32}" != "\${status_gpu_va}" ]]; then
    echo "Expanded CCB record GPU-VA readback did not match attached object VAs" >&2
    cat "\${tmpdir}/query_by_id_after_prepare.txt" >&2
    exit 1
fi
mark_stage_complete prepare_query prepare

"${target_submit_op_bin}" "${target_node}" arm focus > "\${tmpdir}/submit_op_arm_focus.txt"
"${target_query_bin}" "${target_node}" "\${submit_id}" > "\${tmpdir}/query_by_id_after_arm.txt"
grep -q '^submit_arm_valid=1$' "\${tmpdir}/query_by_id_after_arm.txt"
grep -q '^submit_arm_count=1$' "\${tmpdir}/query_by_id_after_arm.txt"
grep -q '^submit_arm_prepare_count=1$' "\${tmpdir}/query_by_id_after_arm.txt"
grep -q '^submit_arm_queue_packet_count=1$' "\${tmpdir}/query_by_id_after_arm.txt"
grep -q '^submit_arm_owner_state_name=queued$' "\${tmpdir}/query_by_id_after_arm.txt"
grep -q '^submit_emit_valid=0$' "\${tmpdir}/query_by_id_after_arm.txt"
grep -q '^submit_report_stage_name=armed$' "\${tmpdir}/query_by_id_after_arm.txt"
grep -q '^submit_report_stage_count=1$' "\${tmpdir}/query_by_id_after_arm.txt"
grep -q '^submit_report_owner_state_name=queued$' "\${tmpdir}/query_by_id_after_arm.txt"
grep -q '^submit_report_queue_packet_count=1$' "\${tmpdir}/query_by_id_after_arm.txt"
grep -q '^submit_report_flags=0x1$' "\${tmpdir}/query_by_id_after_arm.txt"
assert_manifest_snapshot_file "\${tmpdir}/query_by_id_after_arm.txt" armed 1 queued 1 0 \
    "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_descriptor_snapshot_file "\${tmpdir}/query_by_id_after_arm.txt" armed 1 queued 0 0 1 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_plan_snapshot_file "\${tmpdir}/query_by_id_after_arm.txt" armed 1 live queued 0 0 0 1 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
mark_stage_complete arm

"${target_submit_op_bin}" "${target_node}" emit focus > "\${tmpdir}/submit_op_emit_focus.txt"
"${target_query_bin}" "${target_node}" "\${submit_id}" > "\${tmpdir}/query_by_id_after_emit.txt"
grep -q '^submit_emit_valid=1$' "\${tmpdir}/query_by_id_after_emit.txt"
grep -q '^submit_emit_count=1$' "\${tmpdir}/query_by_id_after_emit.txt"
grep -q '^submit_emit_prepare_count=1$' "\${tmpdir}/query_by_id_after_emit.txt"
grep -q '^submit_emit_arm_count=1$' "\${tmpdir}/query_by_id_after_emit.txt"
grep -q '^submit_emit_cmd_type_name=ta$' "\${tmpdir}/query_by_id_after_emit.txt"
grep -q '^submit_emit_owner_state_name=queued$' "\${tmpdir}/query_by_id_after_emit.txt"
grep -q '^submit_emit_queue_packet_count=1$' "\${tmpdir}/query_by_id_after_emit.txt"
grep -q '^submit_emit_dependency_active_count=0$' "\${tmpdir}/query_by_id_after_emit.txt"
grep -q "^submit_emit_control_object_id=\${object_control}\$" "\${tmpdir}/query_by_id_after_emit.txt"
grep -q "^submit_emit_status_object_id=\${object_status}\$" "\${tmpdir}/query_by_id_after_emit.txt"
grep -q '^submit_launch_valid=0$' "\${tmpdir}/query_by_id_after_emit.txt"
grep -q '^submit_report_stage_name=emitted$' "\${tmpdir}/query_by_id_after_emit.txt"
grep -q '^submit_report_stage_count=1$' "\${tmpdir}/query_by_id_after_emit.txt"
grep -q '^submit_report_owner_state_name=queued$' "\${tmpdir}/query_by_id_after_emit.txt"
grep -q '^submit_report_queue_packet_count=1$' "\${tmpdir}/query_by_id_after_emit.txt"
grep -q '^submit_report_dependency_active_count=0$' "\${tmpdir}/query_by_id_after_emit.txt"
grep -q '^submit_report_flags=0x3$' "\${tmpdir}/query_by_id_after_emit.txt"
assert_manifest_snapshot_file "\${tmpdir}/query_by_id_after_emit.txt" emitted 1 queued 1 0 \
    "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_descriptor_snapshot_file "\${tmpdir}/query_by_id_after_emit.txt" emitted 1 queued 0 0 1 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_plan_snapshot_file "\${tmpdir}/query_by_id_after_emit.txt" emitted 1 live queued 0 0 0 1 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
mark_stage_complete emit

"${target_query_bin}" "${target_node}" coord > "\${tmpdir}/coord_base.txt"
"${target_query_bin}" "${target_node}" "\${submit_id}" > "\${tmpdir}/query_by_id_base.txt"
"${target_query_bin}" "${target_node}" focus > "\${tmpdir}/query_focus_base.txt"
coord_issue_ticket_base=\$(extract_value_from_file "\${tmpdir}/coord_base.txt" "coord_issue_ticket")
coord_running_ticket_base=\$(extract_value_from_file "\${tmpdir}/coord_base.txt" "coord_running_ticket")
coord_complete_ticket_base=\$(extract_value_from_file "\${tmpdir}/coord_base.txt" "coord_complete_ticket")
if [[ -z "\${coord_issue_ticket_base}" || ! "\${coord_issue_ticket_base}" =~ ^[0-9]+$ ]]; then
    echo "Failed to resolve coord issue-ticket baseline" >&2
    cat "\${tmpdir}/coord_base.txt" >&2
    exit 1
fi
if [[ -z "\${coord_running_ticket_base}" || ! "\${coord_running_ticket_base}" =~ ^[0-9]+$ ]]; then
    echo "Failed to resolve coord running-ticket baseline" >&2
    cat "\${tmpdir}/coord_base.txt" >&2
    exit 1
fi
if [[ -z "\${coord_complete_ticket_base}" || ! "\${coord_complete_ticket_base}" =~ ^[0-9]+$ ]]; then
    echo "Failed to resolve coord complete-ticket baseline" >&2
    cat "\${tmpdir}/coord_base.txt" >&2
    exit 1
fi
expected_issue_ticket=\$((coord_issue_ticket_base + 1))
expected_running_ticket=\$((coord_running_ticket_base + 1))
expected_complete_ticket=\$((coord_complete_ticket_base + 1))

"${target_submit_op_bin}" "${target_node}" validate focus > "\${tmpdir}/submit_op_validate_focus.txt"
"${target_query_bin}" "${target_node}" "\${submit_id}" > "\${tmpdir}/query_by_id_after_validate.txt"
grep -q '^submit_validate_count=1$' "\${tmpdir}/query_by_id_after_validate.txt"
grep -q '^submit_last_validate_status=0$' "\${tmpdir}/query_by_id_after_validate.txt"
grep -q '^submit_arm_valid=1$' "\${tmpdir}/query_by_id_after_validate.txt"
grep -q '^submit_emit_valid=1$' "\${tmpdir}/query_by_id_after_validate.txt"
grep -q '^submit_launch_valid=0$' "\${tmpdir}/query_by_id_after_validate.txt"
grep -q '^submit_report_stage_name=emitted$' "\${tmpdir}/query_by_id_after_validate.txt"
assert_manifest_snapshot_file "\${tmpdir}/query_by_id_after_validate.txt" emitted 1 queued 1 0 \
    "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_descriptor_snapshot_file "\${tmpdir}/query_by_id_after_validate.txt" emitted 1 queued 0 0 1 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_plan_snapshot_file "\${tmpdir}/query_by_id_after_validate.txt" emitted 1 live queued 0 0 0 1 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
mark_stage_complete validate

set +e
"${target_submit_op_bin}" "${target_node}" launch focus queued 1 > "\${tmpdir}/submit_op_launch_focus.txt" 2>&1
launch_rc=\$?
set -e
if [[ "\${launch_rc}" -ne 0 ]]; then
    echo "submit launch failed" >&2
    cat "\${tmpdir}/submit_op_launch_focus.txt" >&2 || true
    exit "\${launch_rc}"
fi
"${target_query_bin}" "${target_node}" "\${submit_id}" > "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_launch_valid=1$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_launch_consumed=0$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_launch_count=1$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_launch_prepare_count=1$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_launch_arm_count=1$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_launch_emit_count=1$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_launch_cmd_type_name=ta$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_launch_owner_state_name=queued$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_launch_issued_packets=1$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_launch_queue_packet_count=2$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_launch_sync_pending_seq=0x42$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_launch_owner_issue_cookie=0x102$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q "^submit_launch_global_issue_ticket=\${expected_issue_ticket}\$" "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_launch_dependency_active_count=0$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q "^submit_launch_control_object_id=\${object_control}\$" "\${tmpdir}/query_by_id_after_launch.txt"
grep -q "^submit_launch_status_object_id=\${object_status}\$" "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_report_stage_name=launched$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_report_stage_count=1$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_report_owner_state_name=queued$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_report_queue_packet_count=1$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_report_dependency_active_count=0$' "\${tmpdir}/query_by_id_after_launch.txt"
grep -q '^submit_report_flags=0x7$' "\${tmpdir}/query_by_id_after_launch.txt"
assert_manifest_snapshot_file "\${tmpdir}/query_by_id_after_launch.txt" launched 1 queued 1 0 \
    "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_descriptor_snapshot_file "\${tmpdir}/query_by_id_after_launch.txt" launched 1 queued 1 0 1 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_plan_snapshot_file "\${tmpdir}/query_by_id_after_launch.txt" launched 1 launch queued 1 0 0 2 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
mark_stage_complete launch

set +e
"${target_submit_op_bin}" "${target_node}" issue "\${submit_id}" queued 1 > "\${tmpdir}/submit_op_issue_id.txt" 2>&1
issue_rc=\$?
set -e
if [[ "\${issue_rc}" -ne 0 ]]; then
    echo "submit issue failed" >&2
    cat "\${tmpdir}/submit_op_issue_id.txt" >&2 || true
    exit "\${issue_rc}"
fi
"${target_query_bin}" "${target_node}" coord > "\${tmpdir}/coord_after_issue.txt"
"${target_query_bin}" "${target_node}" "\${submit_id}" > "\${tmpdir}/query_by_id_after_issue.txt"
"${target_query_bin}" "${target_node}" focus > "\${tmpdir}/query_focus_after_issue.txt"
grep -q "^coord_issue_ticket=\${expected_issue_ticket}\$" "\${tmpdir}/coord_after_issue.txt"
grep -q '^submit_arm_valid=0$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_emit_valid=0$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_launch_valid=1$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_launch_consumed=1$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_launch_count=1$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_dispatch_valid=1$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_dispatch_count=1$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_dispatch_launch_count=1$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_dispatch_owner_state_name=queued$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_dispatch_issued_packets=1$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_dispatch_completed_packets=0$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_dispatch_queue_packet_count=2$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_dispatch_sync_complete_seq=0x40$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_dispatch_sync_pending_seq=0x42$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_dispatch_owner_issue_cookie=0x102$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q "^submit_dispatch_global_issue_ticket=\${expected_issue_ticket}\$" "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_issue_count=1$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_queue_packet_count=2$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_sync_pending_seq=0x42$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_owner_issue_cookie=0x102$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q "^coord_issue_submit_id=\${submit_id}\$" "\${tmpdir}/coord_after_issue.txt"
grep -q '^submit_report_stage_name=dispatched$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_report_stage_count=1$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_report_owner_state_name=queued$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_report_queue_packet_count=2$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_report_completed_packets=0$' "\${tmpdir}/query_by_id_after_issue.txt"
grep -q "^submit_report_global_issue_ticket=\${expected_issue_ticket}\$" "\${tmpdir}/query_by_id_after_issue.txt"
grep -q '^submit_report_flags=0x1c$' "\${tmpdir}/query_by_id_after_issue.txt"
assert_manifest_snapshot_file "\${tmpdir}/query_by_id_after_issue.txt" dispatched 1 queued 2 0 \
    "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_descriptor_snapshot_file "\${tmpdir}/query_by_id_after_issue.txt" dispatched 1 queued 1 0 2 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_plan_snapshot_file "\${tmpdir}/query_by_id_after_issue.txt" dispatched 1 dispatch queued 1 0 0 2 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
mark_stage_complete issue

"${target_submit_op_bin}" "${target_node}" transition focus running 1 > "\${tmpdir}/submit_op_transition_running_focus.txt"
"${target_query_bin}" "${target_node}" coord > "\${tmpdir}/coord_after_transition_running.txt"
"${target_query_bin}" "${target_node}" "\${submit_id}" > "\${tmpdir}/query_by_id_after_transition_running.txt"
"${target_query_bin}" "${target_node}" focus > "\${tmpdir}/query_focus_after_transition_running.txt"
grep -q "^coord_running_ticket=\${expected_running_ticket}\$" "\${tmpdir}/coord_after_transition_running.txt"
grep -q '^submit_transition_count=1$' "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q '^submit_owner_state_name=running$' "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q '^submit_dispatch_valid=1$' "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q '^submit_dispatch_owner_state_name=running$' "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q '^submit_dispatch_completed_packets=1$' "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q '^submit_dispatch_queue_packet_count=1$' "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q '^submit_dispatch_sync_complete_seq=0x41$' "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q '^submit_dispatch_sync_pending_seq=0x42$' "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q "^submit_dispatch_global_issue_ticket=\${expected_issue_ticket}\$" "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q "^submit_dispatch_global_running_ticket=\${expected_running_ticket}\$" "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q "^coord_running_submit_id=\${submit_id}\$" "\${tmpdir}/coord_after_transition_running.txt"
grep -q '^submit_report_stage_name=dispatched$' "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q '^submit_report_stage_count=1$' "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q '^submit_report_owner_state_name=running$' "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q '^submit_report_queue_packet_count=1$' "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q '^submit_report_completed_packets=1$' "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q "^submit_report_global_running_ticket=\${expected_running_ticket}\$" "\${tmpdir}/query_by_id_after_transition_running.txt"
grep -q '^submit_report_flags=0x10$' "\${tmpdir}/query_by_id_after_transition_running.txt"
assert_manifest_snapshot_file "\${tmpdir}/query_by_id_after_transition_running.txt" dispatched 1 running 1 0 \
    "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_descriptor_snapshot_file "\${tmpdir}/query_by_id_after_transition_running.txt" dispatched 1 running 1 1 1 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_plan_snapshot_file "\${tmpdir}/query_by_id_after_transition_running.txt" dispatched 1 dispatch running 1 1 0 1 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
mark_stage_complete running

"${target_submit_op_bin}" "${target_node}" transition "\${submit_id}" complete 1 > "\${tmpdir}/submit_op_transition_complete_id.txt"
"${target_query_bin}" "${target_node}" coord > "\${tmpdir}/coord_after_transition_complete.txt"
"${target_query_bin}" "${target_node}" "\${submit_id}" > "\${tmpdir}/query_by_id_after_transition_complete.txt"
"${target_query_bin}" "${target_node}" focus > "\${tmpdir}/query_focus_after_transition_complete.txt"
grep -q "^coord_complete_ticket=\${expected_complete_ticket}\$" "\${tmpdir}/coord_after_transition_complete.txt"
grep -q '^submit_transition_count=2$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_owner_state_name=complete$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_dispatch_valid=1$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_dispatch_owner_state_name=complete$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_dispatch_completed_packets=2$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_dispatch_queue_packet_count=0$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_dispatch_sync_complete_seq=0x42$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_dispatch_sync_pending_seq=0x42$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_dispatch_owner_complete_cookie=0x102$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q "^submit_dispatch_global_issue_ticket=\${expected_issue_ticket}\$" "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q "^submit_dispatch_global_running_ticket=\${expected_running_ticket}\$" "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q "^submit_dispatch_global_complete_ticket=\${expected_complete_ticket}\$" "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_report_stage_name=retired$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_report_stage_count=1$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_report_owner_state_name=complete$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_report_queue_packet_count=0$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_report_completed_packets=2$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q "^submit_report_global_complete_ticket=\${expected_complete_ticket}\$" "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_report_flags=0x30$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
assert_manifest_snapshot_file "\${tmpdir}/query_by_id_after_transition_complete.txt" retired 1 complete 0 0 \
    "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_descriptor_snapshot_file "\${tmpdir}/query_by_id_after_transition_complete.txt" retired 1 complete 1 2 0 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
assert_plan_snapshot_file "\${tmpdir}/query_by_id_after_transition_complete.txt" retired 1 retired complete 1 2 0 0 0 \
    "\${control_gpu_va}" "\${pds_gpu_va}" "\${use_gpu_va}" "\${target_gpu_va}" "\${status_gpu_va}"
grep -q '^submit_retired_valid=1$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_retired_count=1$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_retired_dispatch_count=1$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_retired_owner_state_name=complete$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_retired_issued_packets=1$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_retired_completed_packets=2$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_retired_queue_packet_count=0$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_retired_sync_complete_seq=0x42$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q '^submit_retired_sync_pending_seq=0x42$' "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q "^submit_retired_global_issue_ticket=\${expected_issue_ticket}\$" "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q "^submit_retired_global_running_ticket=\${expected_running_ticket}\$" "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q "^submit_retired_global_complete_ticket=\${expected_complete_ticket}\$" "\${tmpdir}/query_by_id_after_transition_complete.txt"
grep -q "^coord_complete_submit_id=\${submit_id}\$" "\${tmpdir}/coord_after_transition_complete.txt"
issue_ticket=\$(extract_value_from_file "\${tmpdir}/query_by_id_after_transition_complete.txt" "submit_global_issue_ticket")
running_ticket=\$(extract_value_from_file "\${tmpdir}/query_by_id_after_transition_complete.txt" "submit_global_running_ticket")
complete_ticket=\$(extract_value_from_file "\${tmpdir}/query_by_id_after_transition_complete.txt" "submit_global_complete_ticket")
if [[ -z "\${issue_ticket}" || "\${issue_ticket}" != "\${expected_issue_ticket}" ]]; then
    echo "Submit global issue ticket did not match expected baseline-relative value" >&2
    cat "\${tmpdir}/query_by_id_after_transition_complete.txt" >&2
    exit 1
fi
if [[ -z "\${running_ticket}" || "\${running_ticket}" != "\${expected_running_ticket}" ]]; then
    echo "Submit global running ticket did not match expected baseline-relative value" >&2
    cat "\${tmpdir}/query_by_id_after_transition_complete.txt" >&2
    exit 1
fi
if [[ -z "\${complete_ticket}" || "\${complete_ticket}" != "\${expected_complete_ticket}" ]]; then
    echo "Submit global complete ticket did not match expected baseline-relative value" >&2
    cat "\${tmpdir}/query_by_id_after_transition_complete.txt" >&2
    exit 1
fi
mark_stage_complete complete

"${target_submit_op_bin}" "${target_node}" drop focus > "\${tmpdir}/submit_op_drop_focus.txt"
submit_dropped=1
"${target_query_bin}" "${target_node}" coord > "\${tmpdir}/coord_after_drop.txt"
set +e
"${target_query_bin}" "${target_node}" focus > "\${tmpdir}/query_focus_after_drop.txt" 2>&1
query_focus_after_drop_rc=\$?
"${target_query_bin}" "${target_node}" "\${submit_id}" > "\${tmpdir}/query_by_id_after_drop.txt" 2>&1
query_by_id_after_drop_rc=\$?
set -e
if [[ "\${query_focus_after_drop_rc}" -eq 0 || "\${query_by_id_after_drop_rc}" -eq 0 ]]; then
    echo "Submit query unexpectedly succeeded after ioctl drop" >&2
    cat "\${tmpdir}/query_focus_after_drop.txt" >&2 || true
    cat "\${tmpdir}/query_by_id_after_drop.txt" >&2 || true
    exit 1
fi
grep -q '^submit_ioctl_ret=-1 errno=2$' "\${tmpdir}/query_focus_after_drop.txt"
grep -q '^submit_ioctl_ret=-1 errno=2$' "\${tmpdir}/query_by_id_after_drop.txt"

"${target_object_op_bin}" "${target_node}" cleanup > "\${tmpdir}/object_cleanup.txt"
"${target_gtt_state_bin}" "${target_node}" > "\${tmpdir}/gtt_after_object_cleanup.txt"
"${target_object_query_bin}" "${target_node}" id "\${object_control}" > "\${tmpdir}/object_control_after_cleanup.txt" 2>&1 || true
grep -q '^object_result_count=5$' "\${tmpdir}/object_cleanup.txt"
grep -q '^gtt_state_range_sgx_object_count=0$' "\${tmpdir}/gtt_after_object_cleanup.txt"
grep -q '^gtt_state_range_pf_bound_count=0$' "\${tmpdir}/gtt_after_object_cleanup.txt"
grep -q '^object_ioctl_ret=-1 errno=2$' "\${tmpdir}/object_control_after_cleanup.txt"
object_ids=()

for pid in "\${probe_pids[@]}"; do
    wait "\${pid}"
done
probe_pids=()

echo "REMOTE_PROBE_BIN=${target_probe_bin}"
echo "REMOTE_QUERY_BIN=${target_query_bin}"
echo "REMOTE_SUBMIT_OP_BIN=${target_submit_op_bin}"
echo "REMOTE_OBJECT_OP_BIN=${target_object_op_bin}"
echo "REMOTE_OBJECT_QUERY_BIN=${target_object_query_bin}"
echo "REMOTE_RANGE_QUERY_BIN=${target_range_query_bin}"
echo "REMOTE_GTT_STATE_BIN=${target_gtt_state_bin}"
echo "SUBMIT_ID=\${submit_id}"
echo "RANGE_IDS=\${range_ids[*]}"
echo "OBJECT_IDS=\${object_control} \${object_pds} \${object_use} \${object_target} \${object_status}"
for range_id in "\${range_ids[@]}"; do
    echo "RANGE_\${range_id}_BEFORE_ADOPT"
    cat "\${tmpdir}/range_\${range_id}_before_adopt.txt"
    echo "OBJECT_ADOPT_RANGE_\${range_id}"
    cat "\${tmpdir}/object_adopt_\${range_id}.txt"
done
echo "GTT_BEFORE"
cat "\${tmpdir}/gtt_before.txt"
echo "OBJECT_CONTROL_AFTER_SETUP"
cat "\${tmpdir}/object_control_after_setup.txt"
echo "OBJECT_STATUS_AFTER_SETUP"
cat "\${tmpdir}/object_status_after_setup.txt"
echo "RANGE_CONTROL_AFTER_SETUP"
cat "\${tmpdir}/range_control_after_setup.txt"
echo "GTT_AFTER_OBJECT_SETUP"
cat "\${tmpdir}/gtt_after_object_setup.txt"
echo "IOCTL_SUBMIT_OP_CREATE_PF"
cat "\${tmpdir}/submit_op_create_pf.txt"
echo "IOCTL_SUBMIT_OP_FOCUS_ID"
cat "\${tmpdir}/submit_op_focus_id.txt"
echo "IOCTL_SUBMIT_OP_ATTACH_CONTROL_ID"
cat "\${tmpdir}/submit_op_attach_control_id.txt"
echo "IOCTL_SUBMIT_OP_ATTACH_PDS_ID"
cat "\${tmpdir}/submit_op_attach_pds_id.txt"
echo "IOCTL_SUBMIT_OP_ATTACH_USE_ID"
cat "\${tmpdir}/submit_op_attach_use_id.txt"
echo "IOCTL_SUBMIT_OP_ATTACH_TARGET_ID"
cat "\${tmpdir}/submit_op_attach_target_id.txt"
echo "IOCTL_SUBMIT_OP_ATTACH_STATUS_FOCUS"
cat "\${tmpdir}/submit_op_attach_status_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_ATTACH"
cat "\${tmpdir}/query_by_id_after_attach.txt"
echo "IOCTL_QUERY_FOCUS_AFTER_ATTACH"
cat "\${tmpdir}/query_focus_after_attach.txt"
echo "IOCTL_SUBMIT_OP_DETACH_STATUS_FOCUS"
cat "\${tmpdir}/submit_op_detach_status_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_DETACH_STATUS"
cat "\${tmpdir}/query_by_id_after_detach_status.txt"
echo "IOCTL_SUBMIT_OP_REATTACH_STATUS_ID"
cat "\${tmpdir}/submit_op_reattach_status_id.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_REATTACH_STATUS"
cat "\${tmpdir}/query_by_id_after_reattach_status.txt"
echo "IOCTL_SUBMIT_OP_KICK_FOCUS"
cat "\${tmpdir}/submit_op_kick_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_KICK"
cat "\${tmpdir}/query_by_id_after_kick.txt"
echo "IOCTL_SUBMIT_OP_CCB_FOCUS"
cat "\${tmpdir}/submit_op_ccb_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_CCB"
cat "\${tmpdir}/query_by_id_after_ccb.txt"
echo "IOCTL_SUBMIT_OP_QUEUE_FOCUS"
cat "\${tmpdir}/submit_op_queue_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_QUEUE"
cat "\${tmpdir}/query_by_id_after_queue.txt"
echo "IOCTL_SUBMIT_OP_STATUS_FOCUS"
cat "\${tmpdir}/submit_op_status_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_STATUS"
cat "\${tmpdir}/query_by_id_after_status.txt"
echo "IOCTL_SUBMIT_OP_SYNC_FOCUS"
cat "\${tmpdir}/submit_op_sync_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_SYNC"
cat "\${tmpdir}/query_by_id_after_sync.txt"
echo "IOCTL_SUBMIT_OP_OWNER_FOCUS"
cat "\${tmpdir}/submit_op_owner_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_OWNER"
cat "\${tmpdir}/query_by_id_after_owner.txt"
echo "IOCTL_SUBMIT_OP_REPORT_FOCUS"
cat "\${tmpdir}/submit_op_report_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_REPORT"
cat "\${tmpdir}/query_by_id_after_report.txt"
echo "IOCTL_SUBMIT_OP_MANIFEST_FOCUS"
cat "\${tmpdir}/submit_op_manifest_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_MANIFEST"
cat "\${tmpdir}/query_by_id_after_manifest.txt"
echo "IOCTL_SUBMIT_OP_DESCRIPTOR_FOCUS"
cat "\${tmpdir}/submit_op_descriptor_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_DESCRIPTOR"
cat "\${tmpdir}/query_by_id_after_descriptor.txt"
echo "IOCTL_SUBMIT_OP_PREPARE_FOCUS"
cat "\${tmpdir}/submit_op_prepare_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_PREPARE"
cat "\${tmpdir}/query_by_id_after_prepare.txt"
echo "IOCTL_SUBMIT_OP_ARM_FOCUS"
cat "\${tmpdir}/submit_op_arm_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_ARM"
cat "\${tmpdir}/query_by_id_after_arm.txt"
echo "IOCTL_SUBMIT_OP_EMIT_FOCUS"
cat "\${tmpdir}/submit_op_emit_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_EMIT"
cat "\${tmpdir}/query_by_id_after_emit.txt"
echo "COORD_BASE"
cat "\${tmpdir}/coord_base.txt"
echo "IOCTL_QUERY_BY_ID_BASE"
cat "\${tmpdir}/query_by_id_base.txt"
echo "IOCTL_QUERY_FOCUS_BASE"
cat "\${tmpdir}/query_focus_base.txt"
echo "IOCTL_SUBMIT_OP_VALIDATE_FOCUS"
cat "\${tmpdir}/submit_op_validate_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_VALIDATE"
cat "\${tmpdir}/query_by_id_after_validate.txt"
echo "IOCTL_SUBMIT_OP_LAUNCH_FOCUS"
cat "\${tmpdir}/submit_op_launch_focus.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_LAUNCH"
cat "\${tmpdir}/query_by_id_after_launch.txt"
echo "IOCTL_SUBMIT_OP_ISSUE_ID"
cat "\${tmpdir}/submit_op_issue_id.txt"
echo "COORD_AFTER_ISSUE"
cat "\${tmpdir}/coord_after_issue.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_ISSUE"
cat "\${tmpdir}/query_by_id_after_issue.txt"
echo "IOCTL_QUERY_FOCUS_AFTER_ISSUE"
cat "\${tmpdir}/query_focus_after_issue.txt"
echo "IOCTL_SUBMIT_OP_TRANSITION_RUNNING_FOCUS"
cat "\${tmpdir}/submit_op_transition_running_focus.txt"
echo "COORD_AFTER_TRANSITION_RUNNING"
cat "\${tmpdir}/coord_after_transition_running.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_TRANSITION_RUNNING"
cat "\${tmpdir}/query_by_id_after_transition_running.txt"
echo "IOCTL_QUERY_FOCUS_AFTER_TRANSITION_RUNNING"
cat "\${tmpdir}/query_focus_after_transition_running.txt"
echo "IOCTL_SUBMIT_OP_TRANSITION_COMPLETE_ID"
cat "\${tmpdir}/submit_op_transition_complete_id.txt"
echo "COORD_AFTER_TRANSITION_COMPLETE"
cat "\${tmpdir}/coord_after_transition_complete.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_TRANSITION_COMPLETE"
cat "\${tmpdir}/query_by_id_after_transition_complete.txt"
echo "IOCTL_QUERY_FOCUS_AFTER_TRANSITION_COMPLETE"
cat "\${tmpdir}/query_focus_after_transition_complete.txt"
echo "IOCTL_SUBMIT_OP_DROP_FOCUS"
cat "\${tmpdir}/submit_op_drop_focus.txt"
echo "COORD_AFTER_DROP"
cat "\${tmpdir}/coord_after_drop.txt"
echo "QUERY_FOCUS_AFTER_DROP_RC=\${query_focus_after_drop_rc}"
echo "IOCTL_QUERY_FOCUS_AFTER_DROP"
cat "\${tmpdir}/query_focus_after_drop.txt"
echo "QUERY_BY_ID_AFTER_DROP_RC=\${query_by_id_after_drop_rc}"
echo "IOCTL_QUERY_BY_ID_AFTER_DROP"
cat "\${tmpdir}/query_by_id_after_drop.txt"
echo "OBJECT_CLEANUP"
cat "\${tmpdir}/object_cleanup.txt"
echo "GTT_AFTER_OBJECT_CLEANUP"
cat "\${tmpdir}/gtt_after_object_cleanup.txt"
echo "OBJECT_CONTROL_AFTER_CLEANUP"
cat "\${tmpdir}/object_control_after_cleanup.txt"
echo "DRM_PROBE_HOLD_LOGS"
for idx in \$(seq 1 "${probe_count}"); do
    echo "DRM_PROBE_\${idx}"
    cat "\${tmpdir}/drm_probe_\${idx}.log"
done
EOF
