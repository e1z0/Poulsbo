#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_dir="${TARGET_DIR:-/root/poulsbo-workloads}"
target_bin="${TARGET_BIN:-${target_dir}/drm_probe}"
target_op_bin="${TARGET_OP_BIN:-${target_dir}/psb_sgx_lab_submit_op}"
target_obj_bin="${TARGET_OBJ_BIN:-${target_dir}/psb_sgx_lab_object_op}"
target_query_bin="${TARGET_QUERY_BIN:-${target_dir}/psb_sgx_lab_query}"
target_range_bin="${TARGET_RANGE_BIN:-${target_dir}/psb_sgx_lab_range_query}"
target_gtt_state_bin="${TARGET_GTT_STATE_BIN:-${target_dir}/psb_sgx_lab_gtt_state_query}"
target_dump_bin="${TARGET_DUMP_BIN:-${target_dir}/psb_sgx_lab_dump}"
target_stage="${TARGET_STAGE:-/root/drm_probe.$$.$RANDOM.new}"
target_op_stage="${TARGET_OP_STAGE:-/root/psb_sgx_lab_submit_op.$$.$RANDOM.new}"
target_obj_stage="${TARGET_OBJ_STAGE:-/root/psb_sgx_lab_object_op.$$.$RANDOM.new}"
target_query_stage="${TARGET_QUERY_STAGE:-/root/psb_sgx_lab_query.$$.$RANDOM.new}"
target_range_stage="${TARGET_RANGE_STAGE:-/root/psb_sgx_lab_range_query.$$.$RANDOM.new}"
target_gtt_state_stage="${TARGET_GTT_STATE_STAGE:-/root/psb_sgx_lab_gtt_state_query.$$.$RANDOM.new}"
target_dump_stage="${TARGET_DUMP_STAGE:-/root/psb_sgx_lab_dump.$$.$RANDOM.new}"
local_bin="${LOCAL_BIN:-$(pwd)/workloads/drm_probe}"
local_op_bin="${LOCAL_OP_BIN:-$(pwd)/workloads/psb_sgx_lab_submit_op}"
local_obj_bin="${LOCAL_OBJ_BIN:-$(pwd)/workloads/psb_sgx_lab_object_op}"
local_query_bin="${LOCAL_QUERY_BIN:-$(pwd)/workloads/psb_sgx_lab_query}"
local_range_bin="${LOCAL_RANGE_BIN:-$(pwd)/workloads/psb_sgx_lab_range_query}"
local_gtt_state_bin="${LOCAL_GTT_STATE_BIN:-$(pwd)/workloads/psb_sgx_lab_gtt_state_query}"
local_dump_bin="${LOCAL_DUMP_BIN:-$(pwd)/workloads/psb_sgx_lab_dump}"
target_node="${TARGET_NODE:-/dev/dri/card0}"
hold_seconds="${HOLD_SECONDS:-30}"
probe_count="${PROBE_COUNT:-10}"
sysfs_dir="${SYSFS_DIR:-/sys/devices/pci0000:00/0000:00:02.0}"
remote_shell_trace="${REMOTE_SHELL_TRACE:-0}"
remote_lock_file="${REMOTE_LOCK_FILE:-/run/poulsbo-sgx-lab.lock}"
remote_lock_wait_secs="${REMOTE_LOCK_WAIT_SECS:-600}"
allow_direct_legacy_multi_submit="${ALLOW_DIRECT_LEGACY_MULTI_SUBMIT:-0}"
use_ioctl_object_setup="${USE_IOCTL_OBJECT_SETUP:-0}"
use_ioctl_submit_setup="${USE_IOCTL_SUBMIT_SETUP:-0}"
use_ioctl_verification="${USE_IOCTL_VERIFICATION:-0}"
use_ioctl_dependency_setup="${USE_IOCTL_DEPENDENCY_SETUP:-0}"
use_ioctl_range_discovery="${USE_IOCTL_RANGE_DISCOVERY:-0}"
submit_a_queue_mode="${SUBMIT_A_QUEUE_MODE:-wrap}"
submit_a_refill_cycles="${SUBMIT_A_REFILL_CYCLES:-2}"
submit_b_queue_mode="${SUBMIT_B_QUEUE_MODE:-nowrap}"
submit_b_refill_cycles="${SUBMIT_B_REFILL_CYCLES:-1}"
submit_b_dependency_complete_ticket="${SUBMIT_B_DEPENDENCY_COMPLETE_TICKET:-auto}"
submit_b_dependency2_complete_ticket="${SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET:-shared}"
submit_b_dependency3_complete_ticket="${SUBMIT_B_DEPENDENCY3_COMPLETE_TICKET:-shared}"
submit_b_dependency4_complete_ticket="${SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET:-shared}"
submit_b_dependency_offset="${SUBMIT_B_DEPENDENCY_OFFSET:-0x60}"
submit_b_dependency2_offset="${SUBMIT_B_DEPENDENCY2_OFFSET:-0x70}"
submit_b_dependency3_offset="${SUBMIT_B_DEPENDENCY3_OFFSET:-0xe0}"
submit_b_dependency4_offset="${SUBMIT_B_DEPENDENCY4_OFFSET:-0x100}"
drop_source_before_run="${DROP_SOURCE_BEFORE_RUN:-0}"
drop_source_after_release="${DROP_SOURCE_AFTER_RELEASE:-0}"
run_unrelated_after_satisfied_drop="${RUN_UNRELATED_AFTER_SATISFIED_DROP:-0}"
run_secondary_dependency_source="${RUN_SECONDARY_DEPENDENCY_SOURCE:-0}"
run_tertiary_dependency_source="${RUN_TERTIARY_DEPENDENCY_SOURCE:-0}"
run_quaternary_dependency_source="${RUN_QUATERNARY_DEPENDENCY_SOURCE:-0}"
clear_dependency4_after_d="${CLEAR_DEPENDENCY4_AFTER_D:-0}"
run_submit_e_after_clear="${RUN_SUBMIT_E_AFTER_CLEAR:-1}"
use_indexed_dependencies="${USE_INDEXED_DEPENDENCIES:-0}"
submit_c_queue_mode="${SUBMIT_C_QUEUE_MODE:-nowrap}"
submit_c_refill_cycles="${SUBMIT_C_REFILL_CYCLES:-0}"
submit_d_queue_mode="${SUBMIT_D_QUEUE_MODE:-nowrap}"
submit_d_refill_cycles="${SUBMIT_D_REFILL_CYCLES:-0}"
submit_e_queue_mode="${SUBMIT_E_QUEUE_MODE:-nowrap}"
submit_e_refill_cycles="${SUBMIT_E_REFILL_CYCLES:-0}"
stale_submit_scan_max="${STALE_SUBMIT_SCAN_MAX:-256}"
stale_object_scan_max="${STALE_OBJECT_SCAN_MAX:-512}"
range_query_scan_max="${RANGE_QUERY_SCAN_MAX:-512}"
range_scan_max="${RANGE_SCAN_MAX:-512}"

required_probe_count=10
if [[ "${run_quaternary_dependency_source}" == "1" ]]; then
    required_probe_count=25
elif [[ "${run_tertiary_dependency_source}" == "1" ]]; then
    required_probe_count=20
elif [[ "${run_unrelated_after_satisfied_drop}" == "1" || "${run_secondary_dependency_source}" == "1" ]]; then
    required_probe_count=15
fi

if [[ "${probe_count}" -lt "${required_probe_count}" ]]; then
    echo "PROBE_COUNT must be at least ${required_probe_count} for the requested submit sets" >&2
    exit 1
fi

if [[ "${range_scan_max}" -lt 1 ]]; then
    echo "RANGE_SCAN_MAX must be at least 1" >&2
    exit 1
fi

if [[ "${remote_lock_wait_secs}" -lt 1 ]]; then
    echo "REMOTE_LOCK_WAIT_SECS must be at least 1" >&2
    exit 1
fi

for mode in "${submit_a_queue_mode}" "${submit_b_queue_mode}" "${submit_c_queue_mode}" "${submit_d_queue_mode}" "${submit_e_queue_mode}"; do
    case "${mode}" in
        wrap|nowrap)
            ;;
        *)
            echo "Submit queue modes must be 'wrap' or 'nowrap'" >&2
            exit 1
            ;;
    esac
done

submit_a_refill_cycles_dec=$((submit_a_refill_cycles))
submit_b_refill_cycles_dec=$((submit_b_refill_cycles))
submit_c_refill_cycles_dec=$((submit_c_refill_cycles))
submit_d_refill_cycles_dec=$((submit_d_refill_cycles))
submit_e_refill_cycles_dec=$((submit_e_refill_cycles))
if [[ "${submit_a_refill_cycles_dec}" -lt 0 || "${submit_b_refill_cycles_dec}" -lt 0 || "${submit_c_refill_cycles_dec}" -lt 0 || "${submit_d_refill_cycles_dec}" -lt 0 || "${submit_e_refill_cycles_dec}" -lt 0 ]]; then
    echo "Submit refill cycles must be zero or greater" >&2
    exit 1
fi

if [[ "${submit_b_dependency_complete_ticket}" == "auto" ]]; then
    submit_b_dependency_complete_ticket_dec=$((submit_a_refill_cycles_dec + 1))
    submit_b_dependency_complete_ticket_relative=0
elif [[ "${submit_b_dependency_complete_ticket}" =~ ^\+[0-9]+$ ]]; then
    submit_b_dependency_complete_ticket_dec=0
    submit_b_dependency_complete_ticket_relative=$(( ${submit_b_dependency_complete_ticket#+} ))
elif [[ "${submit_b_dependency_complete_ticket}" =~ ^[0-9]+$ ]]; then
    submit_b_dependency_complete_ticket_dec=$((submit_b_dependency_complete_ticket))
    submit_b_dependency_complete_ticket_relative=0
else
    echo "SUBMIT_B_DEPENDENCY_COMPLETE_TICKET must be 'auto', +<n>, or a non-negative integer" >&2
    exit 1
fi

if [[ "${submit_b_dependency2_complete_ticket}" == "shared" ]]; then
    submit_b_dependency2_complete_ticket_dec=0
    submit_b_dependency2_complete_ticket_relative=0
elif [[ "${submit_b_dependency2_complete_ticket}" =~ ^\+[0-9]+$ ]]; then
    submit_b_dependency2_complete_ticket_dec=0
    submit_b_dependency2_complete_ticket_relative=$(( ${submit_b_dependency2_complete_ticket#+} ))
elif [[ "${submit_b_dependency2_complete_ticket}" =~ ^[0-9]+$ ]]; then
    submit_b_dependency2_complete_ticket_dec=$((submit_b_dependency2_complete_ticket))
    submit_b_dependency2_complete_ticket_relative=0
else
    echo "SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET must be 'shared', +<n>, or a non-negative integer" >&2
    exit 1
fi

if [[ "${submit_b_dependency3_complete_ticket}" == "shared" ]]; then
    submit_b_dependency3_complete_ticket_dec=0
    submit_b_dependency3_complete_ticket_relative=0
elif [[ "${submit_b_dependency3_complete_ticket}" =~ ^\+[0-9]+$ ]]; then
    submit_b_dependency3_complete_ticket_dec=0
    submit_b_dependency3_complete_ticket_relative=$(( ${submit_b_dependency3_complete_ticket#+} ))
elif [[ "${submit_b_dependency3_complete_ticket}" =~ ^[0-9]+$ ]]; then
    submit_b_dependency3_complete_ticket_dec=$((submit_b_dependency3_complete_ticket))
    submit_b_dependency3_complete_ticket_relative=0
else
    echo "SUBMIT_B_DEPENDENCY3_COMPLETE_TICKET must be 'shared', +<n>, or a non-negative integer" >&2
    exit 1
fi

if [[ "${submit_b_dependency4_complete_ticket}" == "shared" ]]; then
    submit_b_dependency4_complete_ticket_dec=0
    submit_b_dependency4_complete_ticket_relative=0
elif [[ "${submit_b_dependency4_complete_ticket}" =~ ^\+[0-9]+$ ]]; then
    submit_b_dependency4_complete_ticket_dec=0
    submit_b_dependency4_complete_ticket_relative=$(( ${submit_b_dependency4_complete_ticket#+} ))
elif [[ "${submit_b_dependency4_complete_ticket}" =~ ^[0-9]+$ ]]; then
    submit_b_dependency4_complete_ticket_dec=$((submit_b_dependency4_complete_ticket))
    submit_b_dependency4_complete_ticket_relative=0
else
    echo "SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET must be 'shared', +<n>, or a non-negative integer" >&2
    exit 1
fi

if [[ "${drop_source_before_run}" != "0" && "${drop_source_before_run}" != "1" ]]; then
    echo "DROP_SOURCE_BEFORE_RUN must be 0 or 1" >&2
    exit 1
fi

if [[ "${drop_source_after_release}" != "0" && "${drop_source_after_release}" != "1" ]]; then
    echo "DROP_SOURCE_AFTER_RELEASE must be 0 or 1" >&2
    exit 1
fi

if [[ "${drop_source_before_run}" == "1" && "${drop_source_after_release}" == "1" ]]; then
    echo "DROP_SOURCE_BEFORE_RUN and DROP_SOURCE_AFTER_RELEASE cannot both be 1" >&2
    exit 1
fi

if [[ "${run_unrelated_after_satisfied_drop}" != "0" && "${run_unrelated_after_satisfied_drop}" != "1" ]]; then
    echo "RUN_UNRELATED_AFTER_SATISFIED_DROP must be 0 or 1" >&2
    exit 1
fi

if [[ "${run_secondary_dependency_source}" != "0" && "${run_secondary_dependency_source}" != "1" ]]; then
    echo "RUN_SECONDARY_DEPENDENCY_SOURCE must be 0 or 1" >&2
    exit 1
fi

if [[ "${run_tertiary_dependency_source}" != "0" && "${run_tertiary_dependency_source}" != "1" ]]; then
    echo "RUN_TERTIARY_DEPENDENCY_SOURCE must be 0 or 1" >&2
    exit 1
fi

if [[ "${run_quaternary_dependency_source}" != "0" && "${run_quaternary_dependency_source}" != "1" ]]; then
    echo "RUN_QUATERNARY_DEPENDENCY_SOURCE must be 0 or 1" >&2
    exit 1
fi

if [[ "${clear_dependency4_after_d}" != "0" && "${clear_dependency4_after_d}" != "1" ]]; then
    echo "CLEAR_DEPENDENCY4_AFTER_D must be 0 or 1" >&2
    exit 1
fi

if [[ "${run_submit_e_after_clear}" != "0" && "${run_submit_e_after_clear}" != "1" ]]; then
    echo "RUN_SUBMIT_E_AFTER_CLEAR must be 0 or 1" >&2
    exit 1
fi

if [[ "${use_indexed_dependencies}" != "0" && "${use_indexed_dependencies}" != "1" ]]; then
    echo "USE_INDEXED_DEPENDENCIES must be 0 or 1" >&2
    exit 1
fi

if [[ "${use_ioctl_dependency_setup}" != "0" && "${use_ioctl_dependency_setup}" != "1" ]]; then
    echo "USE_IOCTL_DEPENDENCY_SETUP must be 0 or 1" >&2
    exit 1
fi

if [[ "${use_ioctl_object_setup}" != "0" && "${use_ioctl_object_setup}" != "1" ]]; then
    echo "USE_IOCTL_OBJECT_SETUP must be 0 or 1" >&2
    exit 1
fi

if [[ "${use_ioctl_range_discovery}" != "0" && "${use_ioctl_range_discovery}" != "1" ]]; then
    echo "USE_IOCTL_RANGE_DISCOVERY must be 0 or 1" >&2
    exit 1
fi

if [[ "${remote_shell_trace}" != "0" && "${remote_shell_trace}" != "1" ]]; then
    echo "REMOTE_SHELL_TRACE must be 0 or 1" >&2
    exit 1
fi

if [[ "${use_ioctl_submit_setup}" != "0" && "${use_ioctl_submit_setup}" != "1" ]]; then
    echo "USE_IOCTL_SUBMIT_SETUP must be 0 or 1" >&2
    exit 1
fi

if [[ "${use_ioctl_verification}" != "0" && "${use_ioctl_verification}" != "1" ]]; then
    echo "USE_IOCTL_VERIFICATION must be 0 or 1" >&2
    exit 1
fi

if [[ "${use_ioctl_verification}" == "1" ]]; then
    if [[ "${use_ioctl_object_setup}" != "1" || "${use_ioctl_submit_setup}" != "1" || "${use_ioctl_dependency_setup}" != "1" || "${use_ioctl_range_discovery}" != "1" ]]; then
        echo "USE_IOCTL_VERIFICATION=1 requires USE_IOCTL_OBJECT_SETUP=1, USE_IOCTL_SUBMIT_SETUP=1, USE_IOCTL_DEPENDENCY_SETUP=1, and USE_IOCTL_RANGE_DISCOVERY=1" >&2
        exit 1
    fi
fi

if [[ "${use_ioctl_verification}" != "1" && \
      "${allow_direct_legacy_multi_submit}" != "1" ]]; then
    cat >&2 <<'EOF'
Direct run-vaio-sgx-multi-submit.sh execution is blocked for the legacy/sysfs path.

Use one of:
  ./scripts/run-vaio-sgx-multi-submit-default.sh
  ./scripts/run-vaio-sgx-multi-submit-typed.sh

If you intentionally need the raw legacy implementation, opt in explicitly:
  ALLOW_DIRECT_LEGACY_MULTI_SUBMIT=1 ./scripts/run-vaio-sgx-multi-submit.sh
EOF
    exit 1
fi

if [[ "${run_unrelated_after_satisfied_drop}" == "1" && "${drop_source_after_release}" != "1" ]]; then
    echo "RUN_UNRELATED_AFTER_SATISFIED_DROP requires DROP_SOURCE_AFTER_RELEASE=1" >&2
    exit 1
fi

if [[ "${run_unrelated_after_satisfied_drop}" == "1" && "${run_secondary_dependency_source}" == "1" ]]; then
    echo "RUN_UNRELATED_AFTER_SATISFIED_DROP and RUN_SECONDARY_DEPENDENCY_SOURCE cannot both be 1" >&2
    exit 1
fi

if [[ "${run_unrelated_after_satisfied_drop}" == "1" && "${run_tertiary_dependency_source}" == "1" ]]; then
    echo "RUN_UNRELATED_AFTER_SATISFIED_DROP and RUN_TERTIARY_DEPENDENCY_SOURCE cannot both be 1" >&2
    exit 1
fi

if [[ "${run_unrelated_after_satisfied_drop}" == "1" && "${run_quaternary_dependency_source}" == "1" ]]; then
    echo "RUN_UNRELATED_AFTER_SATISFIED_DROP and RUN_QUATERNARY_DEPENDENCY_SOURCE cannot both be 1" >&2
    exit 1
fi

if [[ "${run_secondary_dependency_source}" == "1" ]]; then
    if [[ "${drop_source_before_run}" != "0" || "${drop_source_after_release}" != "0" ]]; then
        echo "RUN_SECONDARY_DEPENDENCY_SOURCE does not combine with source-drop modes" >&2
        exit 1
    fi
    if [[ "${submit_b_dependency_complete_ticket_dec}" -le 0 ]]; then
        echo "RUN_SECONDARY_DEPENDENCY_SOURCE requires SUBMIT_B_DEPENDENCY_COMPLETE_TICKET > 0" >&2
        exit 1
    fi
fi

if [[ "${run_tertiary_dependency_source}" == "1" ]]; then
    if [[ "${run_secondary_dependency_source}" != "1" ]]; then
        echo "RUN_TERTIARY_DEPENDENCY_SOURCE requires RUN_SECONDARY_DEPENDENCY_SOURCE=1" >&2
        exit 1
    fi
    if [[ "${drop_source_before_run}" != "0" || "${drop_source_after_release}" != "0" ]]; then
        echo "RUN_TERTIARY_DEPENDENCY_SOURCE does not combine with source-drop modes" >&2
        exit 1
    fi
fi

if [[ "${run_quaternary_dependency_source}" == "1" ]]; then
    if [[ "${run_tertiary_dependency_source}" != "1" ]]; then
        echo "RUN_QUATERNARY_DEPENDENCY_SOURCE requires RUN_TERTIARY_DEPENDENCY_SOURCE=1" >&2
        exit 1
    fi
    if [[ "${use_indexed_dependencies}" != "1" ]]; then
        echo "RUN_QUATERNARY_DEPENDENCY_SOURCE requires USE_INDEXED_DEPENDENCIES=1" >&2
        exit 1
    fi
    if [[ "${drop_source_before_run}" != "0" || "${drop_source_after_release}" != "0" ]]; then
        echo "RUN_QUATERNARY_DEPENDENCY_SOURCE does not combine with source-drop modes" >&2
        exit 1
    fi
fi

if [[ "${clear_dependency4_after_d}" == "1" ]]; then
    if [[ "${run_quaternary_dependency_source}" != "1" ]]; then
        echo "CLEAR_DEPENDENCY4_AFTER_D requires RUN_QUATERNARY_DEPENDENCY_SOURCE=1" >&2
        exit 1
    fi
    if [[ "${use_indexed_dependencies}" != "1" ]]; then
        echo "CLEAR_DEPENDENCY4_AFTER_D requires USE_INDEXED_DEPENDENCIES=1" >&2
        exit 1
    fi
fi

if [[ ! -f "${local_bin}" ]]; then
    echo "Local probe not found: ${local_bin}" >&2
    exit 1
fi

if [[ "${use_ioctl_object_setup}" == "1" && ! -f "${local_obj_bin}" ]]; then
    echo "Local object-op binary not found: ${local_obj_bin}" >&2
    exit 1
fi

if [[ "${use_ioctl_range_discovery}" == "1" && ! -f "${local_range_bin}" ]]; then
    echo "Local range-query binary not found: ${local_range_bin}" >&2
    exit 1
fi

if [[ ("${use_ioctl_dependency_setup}" == "1" || "${use_ioctl_submit_setup}" == "1") && ! -f "${local_op_bin}" ]]; then
    echo "Local submit-op binary not found: ${local_op_bin}" >&2
    exit 1
fi

if [[ "${use_ioctl_verification}" == "1" ]]; then
    for helper_bin in "${local_query_bin}" "${local_gtt_state_bin}" "${local_dump_bin}"; do
        if [[ ! -f "${helper_bin}" ]]; then
            echo "Local typed verification helper not found: ${helper_bin}" >&2
            exit 1
        fi
    done
fi

scp "${local_bin}" "${target_user}@${target_host}:${target_stage}"
if [[ "${use_ioctl_object_setup}" == "1" ]]; then
    scp "${local_obj_bin}" "${target_user}@${target_host}:${target_obj_stage}"
fi
if [[ "${use_ioctl_verification}" == "1" ]]; then
    scp "${local_query_bin}" "${target_user}@${target_host}:${target_query_stage}"
    scp "${local_gtt_state_bin}" "${target_user}@${target_host}:${target_gtt_state_stage}"
    scp "${local_dump_bin}" "${target_user}@${target_host}:${target_dump_stage}"
fi
if [[ "${use_ioctl_range_discovery}" == "1" ]]; then
    scp "${local_range_bin}" "${target_user}@${target_host}:${target_range_stage}"
fi
if [[ "${use_ioctl_dependency_setup}" == "1" || "${use_ioctl_submit_setup}" == "1" ]]; then
    scp "${local_op_bin}" "${target_user}@${target_host}:${target_op_stage}"
fi
ssh "${target_user}@${target_host}" "bash -s" <<EOF
set -euo pipefail
remote_shell_trace="${remote_shell_trace}"
remote_lock_file="${remote_lock_file}"
remote_lock_wait_secs="${remote_lock_wait_secs}"
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
rm -f "${target_bin}"
install -m 0755 -T "${target_stage}" "${target_bin}"
rm -f "${target_stage}"
use_ioctl_object_setup="${use_ioctl_object_setup}"
use_ioctl_submit_setup="${use_ioctl_submit_setup}"
use_ioctl_dependency_setup="${use_ioctl_dependency_setup}"
use_ioctl_range_discovery="${use_ioctl_range_discovery}"
use_ioctl_verification="${use_ioctl_verification}"
target_obj_bin="${target_obj_bin}"
target_op_bin="${target_op_bin}"
target_query_bin="${target_query_bin}"
target_range_bin="${target_range_bin}"
target_gtt_state_bin="${target_gtt_state_bin}"
target_dump_bin="${target_dump_bin}"
if [[ "\${use_ioctl_object_setup}" == "1" ]]; then
    install -m 0755 -T "${target_obj_stage}" "\${target_obj_bin}"
    rm -f "${target_obj_stage}"
fi
if [[ "\${use_ioctl_verification}" == "1" ]]; then
    install -m 0755 -T "${target_query_stage}" "\${target_query_bin}"
    install -m 0755 -T "${target_gtt_state_stage}" "\${target_gtt_state_bin}"
    install -m 0755 -T "${target_dump_stage}" "\${target_dump_bin}"
    rm -f "${target_query_stage}" "${target_gtt_state_stage}" "${target_dump_stage}"
fi
if [[ "\${use_ioctl_range_discovery}" == "1" ]]; then
    install -m 0755 -T "${target_range_stage}" "\${target_range_bin}"
    rm -f "${target_range_stage}"
fi
if [[ "\${use_ioctl_dependency_setup}" == "1" || "\${use_ioctl_submit_setup}" == "1" ]]; then
    install -m 0755 -T "${target_op_stage}" "\${target_op_bin}"
    rm -f "${target_op_stage}"
fi

object_node="${sysfs_dir}/poulsbo_sgx_object"
objects_node="${sysfs_dir}/poulsbo_sgx_objects"
submit_node="${sysfs_dir}/poulsbo_sgx_submit"
submits_node="${sysfs_dir}/poulsbo_sgx_submits"
submit_state_node="${sysfs_dir}/poulsbo_sgx_submit_state"
coord_node="${sysfs_dir}/poulsbo_sgx_coord_state"
gtt_node="${sysfs_dir}/poulsbo_gtt_state"
hold_seconds="${hold_seconds}"
submit_b_dependency_offset="${submit_b_dependency_offset}"
submit_b_dependency_complete_ticket_dec="${submit_b_dependency_complete_ticket_dec}"
submit_b_dependency_complete_ticket_relative="${submit_b_dependency_complete_ticket_relative}"
submit_b_dependency2_offset="${submit_b_dependency2_offset}"
submit_b_dependency3_offset="${submit_b_dependency3_offset}"
submit_b_dependency4_offset="${submit_b_dependency4_offset}"
submit_b_dependency2_complete_ticket_dec="${submit_b_dependency2_complete_ticket_dec}"
submit_b_dependency2_complete_ticket_relative="${submit_b_dependency2_complete_ticket_relative}"
submit_b_dependency3_complete_ticket_dec="${submit_b_dependency3_complete_ticket_dec}"
submit_b_dependency3_complete_ticket_relative="${submit_b_dependency3_complete_ticket_relative}"
submit_b_dependency4_complete_ticket_dec="${submit_b_dependency4_complete_ticket_dec}"
submit_b_dependency4_complete_ticket_relative="${submit_b_dependency4_complete_ticket_relative}"
use_indexed_dependencies="${use_indexed_dependencies}"
stale_submit_scan_max="${stale_submit_scan_max}"
stale_object_scan_max="${stale_object_scan_max}"
range_query_scan_max="${range_query_scan_max}"
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

if [[ "\${use_ioctl_verification}" != "1" ]]; then
    for node in "\${object_node}" "\${objects_node}" "\${submit_node}" "\${submits_node}" "\${submit_state_node}" "\${coord_node}" "\${gtt_node}"; do
        if [[ ! -e "\${node}" ]]; then
            echo "Missing sysfs node: \${node}" >&2
            exit 1
        fi
    done
fi

submit_query_capture() {
    local outfile="\$1"
    shift
    "\${target_query_bin}" "${target_node}" "\$@" > "\${tmpdir}/\${outfile}"
}

submit_coord_capture() {
    local outfile="\$1"

    submit_query_capture "\${outfile}" coord
}

gtt_state_capture() {
    local outfile="\$1"

    "\${target_gtt_state_bin}" "${target_node}" > "\${tmpdir}/\${outfile}"
}

typed_dump_capture() {
    local outfile="\$1"
    local mode="\${2:-cached}"

    "\${target_dump_bin}" "${target_node}" "\${mode}" > "\${tmpdir}/\${outfile}"
}

capture_objects_snapshot() {
    local outfile="\$1"

    if [[ "\${use_ioctl_verification}" == "1" ]]; then
        typed_dump_capture "\${outfile}" cached
    else
        cat "\${objects_node}" > "\${tmpdir}/\${outfile}"
    fi
}

capture_submits_snapshot() {
    local outfile="\$1"

    if [[ "\${use_ioctl_verification}" == "1" ]]; then
        typed_dump_capture "\${outfile}" cached
    else
        cat "\${submits_node}" > "\${tmpdir}/\${outfile}"
    fi
}

capture_gtt_snapshot() {
    local outfile="\$1"

    if [[ "\${use_ioctl_verification}" == "1" ]]; then
        gtt_state_capture "\${outfile}"
    else
        cat "\${gtt_node}" > "\${tmpdir}/\${outfile}"
    fi
}

submit_focus_dump() {
    local submit_id="\$1"
    local label="\$2"
    local outfile="\$3"

    submit_focus "\${submit_id}"
    if [[ "\${use_ioctl_verification}" == "1" ]]; then
        submit_query_capture "\${outfile}" focus
    else
        cat "\${submit_state_node}" > "\${tmpdir}/\${outfile}"
    fi
    echo "\${label}"
    cat "\${tmpdir}/\${outfile}"
}

submit_coord_dump() {
    local label="\$1"
    local outfile="\$2"

    if [[ "\${use_ioctl_verification}" == "1" ]]; then
        submit_coord_capture "\${outfile}"
    else
        cat "\${coord_node}" > "\${tmpdir}/\${outfile}"
    fi
    echo "\${label}"
    cat "\${tmpdir}/\${outfile}"
}

capture_coord_snapshot_file() {
    local outfile="\$1"

    if [[ "\${use_ioctl_verification}" == "1" ]]; then
        submit_coord_capture "\${outfile}"
    else
        cat "\${coord_node}" > "\${tmpdir}/\${outfile}"
    fi
}

extract_value_from_file() {
    local infile="\$1"
    local key="\$2"

    awk -F= -v want="\${key}" '\$1 == want { print substr(\$0, index(\$0, "=") + 1); exit }' \
        "\${tmpdir}/\${infile}"
}

require_snapshot_value() {
    local infile="\$1"
    local key="\$2"
    local expected="\$3"
    local actual

    actual="\$(extract_value_from_file "\${infile}" "\${key}")"
    if [[ "\${actual}" != "\${expected}" ]]; then
        echo "Snapshot assertion failed: \${infile} \${key} expected '\${expected}' got '\${actual}'" >&2
        cat "\${tmpdir}/\${infile}" >&2
        exit 1
    fi
}

require_snapshot_int_ge() {
    local infile="\$1"
    local key="\$2"
    local minimum="\$3"
    local actual

    actual="\$(extract_value_from_file "\${infile}" "\${key}")"
    if [[ -z "\${actual}" ]]; then
        echo "Snapshot assertion failed: \${infile} missing \${key}" >&2
        cat "\${tmpdir}/\${infile}" >&2
        exit 1
    fi
    if (( actual < minimum )); then
        echo "Snapshot assertion failed: \${infile} \${key} expected >= \${minimum} got \${actual}" >&2
        cat "\${tmpdir}/\${infile}" >&2
        exit 1
    fi
}

assert_dispatch_snapshot_file() {
    local infile="\$1"
    local expected_count="\$2"
    local expected_state="\$3"
    local expected_issued="\$4"
    local expected_completed="\$5"
    local expected_queue="\$6"
    local expected_dependency_active="\$7"

    require_snapshot_value "\${infile}" submit_dispatch_valid 1
    require_snapshot_value "\${infile}" submit_dispatch_count "\${expected_count}"
    require_snapshot_value "\${infile}" submit_dispatch_launch_count "\${expected_count}"
    require_snapshot_value "\${infile}" submit_dispatch_owner_state_name "\${expected_state}"
    require_snapshot_value "\${infile}" submit_dispatch_issued_packets "\${expected_issued}"
    require_snapshot_value "\${infile}" submit_dispatch_completed_packets "\${expected_completed}"
    require_snapshot_value "\${infile}" submit_dispatch_queue_packet_count "\${expected_queue}"
    require_snapshot_value "\${infile}" submit_dispatch_dependency_active_count "\${expected_dependency_active}"
}

assert_retired_snapshot_file() {
    local infile="\$1"
    local expected_count="\$2"
    local expected_dispatch_count="\$3"
    local expected_state="\$4"
    local expected_issued="\$5"
    local expected_completed="\$6"
    local expected_queue="\$7"
    local expected_dependency_active="\$8"

    require_snapshot_value "\${infile}" submit_retired_valid 1
    require_snapshot_value "\${infile}" submit_retired_count "\${expected_count}"
    require_snapshot_value "\${infile}" submit_retired_dispatch_count "\${expected_dispatch_count}"
    require_snapshot_value "\${infile}" submit_retired_owner_state_name "\${expected_state}"
    require_snapshot_value "\${infile}" submit_retired_issued_packets "\${expected_issued}"
    require_snapshot_value "\${infile}" submit_retired_completed_packets "\${expected_completed}"
    require_snapshot_value "\${infile}" submit_retired_queue_packet_count "\${expected_queue}"
    require_snapshot_value "\${infile}" submit_retired_dependency_active_count "\${expected_dependency_active}"
}

assert_retired_persisted_snapshot_file() {
    local infile="\$1"
    local expected_count="\$2"
    local expected_dispatch_count="\$3"
    local expected_state="\$4"
    local expected_issued="\$5"
    local minimum_completed="\$6"
    local expected_queue="\$7"
    local expected_dependency_active="\$8"

    require_snapshot_value "\${infile}" submit_retired_valid 1
    require_snapshot_value "\${infile}" submit_retired_count "\${expected_count}"
    require_snapshot_value "\${infile}" submit_retired_dispatch_count "\${expected_dispatch_count}"
    require_snapshot_value "\${infile}" submit_retired_owner_state_name "\${expected_state}"
    require_snapshot_value "\${infile}" submit_retired_issued_packets "\${expected_issued}"
    require_snapshot_int_ge "\${infile}" submit_retired_completed_packets "\${minimum_completed}"
    require_snapshot_value "\${infile}" submit_retired_queue_packet_count "\${expected_queue}"
    require_snapshot_value "\${infile}" submit_retired_dependency_active_count "\${expected_dependency_active}"
}

assert_report_snapshot_file() {
    local infile="\$1"
    local expected_stage="\$2"
    local expected_stage_count="\$3"
    local expected_state="\$4"
    local expected_queue="\$5"
    local expected_completed="\$6"
    local expected_dependency_active="\$7"

    require_snapshot_value "\${infile}" submit_report_stage_name "\${expected_stage}"
    require_snapshot_value "\${infile}" submit_report_stage_count "\${expected_stage_count}"
    require_snapshot_value "\${infile}" submit_report_owner_state_name "\${expected_state}"
    require_snapshot_value "\${infile}" submit_report_queue_packet_count "\${expected_queue}"
    require_snapshot_value "\${infile}" submit_report_completed_packets "\${expected_completed}"
    require_snapshot_value "\${infile}" submit_report_dependency_active_count "\${expected_dependency_active}"
    require_snapshot_value "\${infile}" submit_report_config_count 1
}

assert_manifest_snapshot_file() {
    local infile="\$1"
    local expected_stage="\$2"
    local expected_stage_count="\$3"
    local expected_state="\$4"
    local expected_queue="\$5"
    local expected_dependency_active="\$6"

    require_snapshot_value "\${infile}" submit_manifest_stage_name "\${expected_stage}"
    require_snapshot_value "\${infile}" submit_manifest_stage_count "\${expected_stage_count}"
    require_snapshot_value "\${infile}" submit_manifest_owner_state_name "\${expected_state}"
    require_snapshot_value "\${infile}" submit_manifest_queue_packet_count "\${expected_queue}"
    require_snapshot_value "\${infile}" submit_manifest_dependency_active_count "\${expected_dependency_active}"
    require_snapshot_value "\${infile}" submit_manifest_config_count 1
}

assert_descriptor_snapshot_file() {
    local infile="\$1"
    local expected_stage="\$2"
    local expected_stage_count="\$3"
    local expected_state="\$4"
    local expected_issued="\$5"
    local expected_completed="\$6"
    local expected_queue="\$7"
    local expected_dependency_active="\$8"

    require_snapshot_value "\${infile}" submit_descriptor_stage_name "\${expected_stage}"
    require_snapshot_value "\${infile}" submit_descriptor_stage_count "\${expected_stage_count}"
    require_snapshot_value "\${infile}" submit_descriptor_owner_state_name "\${expected_state}"
    require_snapshot_value "\${infile}" submit_descriptor_issued_packets "\${expected_issued}"
    require_snapshot_value "\${infile}" submit_descriptor_completed_packets "\${expected_completed}"
    require_snapshot_value "\${infile}" submit_descriptor_queue_packet_count "\${expected_queue}"
    require_snapshot_value "\${infile}" submit_descriptor_dependency_active_count "\${expected_dependency_active}"
    require_snapshot_value "\${infile}" submit_descriptor_config_count 1
}

assert_plan_snapshot_file() {
    local infile="\$1"
    local expected_stage="\$2"
    local expected_stage_count="\$3"
    local expected_source="\$4"
    local expected_state="\$5"
    local expected_issued="\$6"
    local expected_completed="\$7"
    local expected_queue="\$8"
    local expected_queue_epoch="\$9"
    local expected_dependency_active="\${10}"

    require_snapshot_value "\${infile}" submit_plan_stage_name "\${expected_stage}"
    require_snapshot_value "\${infile}" submit_plan_stage_count "\${expected_stage_count}"
    require_snapshot_value "\${infile}" submit_plan_source_name "\${expected_source}"
    require_snapshot_value "\${infile}" submit_plan_owner_state_name "\${expected_state}"
    require_snapshot_value "\${infile}" submit_plan_issued_packets "\${expected_issued}"
    require_snapshot_value "\${infile}" submit_plan_completed_packets "\${expected_completed}"
    require_snapshot_value "\${infile}" submit_plan_queue_packet_count "\${expected_queue}"
    require_snapshot_value "\${infile}" submit_plan_queue_epoch "\${expected_queue_epoch}"
    require_snapshot_value "\${infile}" submit_plan_dependency_active_count "\${expected_dependency_active}"
    require_snapshot_value "\${infile}" submit_plan_config_count 1
}

assert_issue_plan_snapshot_file() {
    local infile="\$1"
    local expected_stage="\$2"
    local expected_stage_count="\$3"
    local expected_valid="\$4"
    local expected_state="\$5"
    local expected_issued="\$6"
    local expected_refilled="\$7"
    local expected_queue="\$8"
    local expected_queue_epoch="\$9"
    local expected_dependency_active="\${10}"

    require_snapshot_value "\${infile}" submit_issue_plan_stage_name "\${expected_stage}"
    require_snapshot_value "\${infile}" submit_issue_plan_stage_count "\${expected_stage_count}"
    require_snapshot_value "\${infile}" submit_issue_plan_valid "\${expected_valid}"
    require_snapshot_value "\${infile}" submit_issue_plan_owner_state_name "\${expected_state}"
    require_snapshot_value "\${infile}" submit_issue_plan_issued_packets "\${expected_issued}"
    require_snapshot_value "\${infile}" submit_issue_plan_refilled_from_complete "\${expected_refilled}"
    require_snapshot_value "\${infile}" submit_issue_plan_queue_packet_count "\${expected_queue}"
    require_snapshot_value "\${infile}" submit_issue_plan_queue_epoch "\${expected_queue_epoch}"
    require_snapshot_value "\${infile}" submit_issue_plan_dependency_active_count "\${expected_dependency_active}"
    require_snapshot_value "\${infile}" submit_issue_plan_config_count 1
}

assert_result_snapshot_file() {
    local infile="\$1"
    local expected_stage="\$2"
    local expected_stage_count="\$3"
    local expected_source="\$4"
    local expected_state="\$5"
    local expected_issued="\$6"
    local expected_completed="\$7"
    local expected_queue="\$8"
    local expected_queue_epoch="\$9"
    local expected_dependency_active="\${10}"

    require_snapshot_value "\${infile}" submit_result_stage_name "\${expected_stage}"
    require_snapshot_value "\${infile}" submit_result_stage_count "\${expected_stage_count}"
    require_snapshot_value "\${infile}" submit_result_source_name "\${expected_source}"
    require_snapshot_value "\${infile}" submit_result_owner_state_name "\${expected_state}"
    require_snapshot_value "\${infile}" submit_result_issued_packets "\${expected_issued}"
    require_snapshot_value "\${infile}" submit_result_completed_packets "\${expected_completed}"
    require_snapshot_value "\${infile}" submit_result_queue_packet_count "\${expected_queue}"
    require_snapshot_value "\${infile}" submit_result_queue_epoch "\${expected_queue_epoch}"
    require_snapshot_value "\${infile}" submit_result_dependency_active_count "\${expected_dependency_active}"
    require_snapshot_value "\${infile}" submit_result_config_count 1
}

assert_live_snapshot_file() {
    local infile="\$1"
    local expected_stage="\$2"
    local expected_stage_count="\$3"
    local expected_state="\$4"
    local expected_issued="\$5"
    local expected_completed="\$6"
    local expected_queue="\$7"
    local expected_queue_epoch="\$8"
    local expected_dependency_active="\$9"

    require_snapshot_value "\${infile}" submit_live_stage_name "\${expected_stage}"
    require_snapshot_value "\${infile}" submit_live_stage_count "\${expected_stage_count}"
    require_snapshot_value "\${infile}" submit_live_owner_state_name "\${expected_state}"
    require_snapshot_value "\${infile}" submit_live_issued_packets "\${expected_issued}"
    require_snapshot_value "\${infile}" submit_live_completed_packets "\${expected_completed}"
    require_snapshot_value "\${infile}" submit_live_queue_packet_count "\${expected_queue}"
    require_snapshot_value "\${infile}" submit_live_queue_epoch "\${expected_queue_epoch}"
    require_snapshot_value "\${infile}" submit_live_dependency_active_count "\${expected_dependency_active}"
    require_snapshot_value "\${infile}" submit_live_config_count 1
}

resolve_dependency_ticket_value() {
    local absolute_value="\$1"
    local relative_delta="\$2"
    local baseline_complete="\$3"

    if [[ "\${relative_delta}" -gt 0 ]]; then
        printf '%s\n' "\$((baseline_complete + relative_delta))"
    else
        printf '%s\n' "\${absolute_value}"
    fi
}

object_ioctl_capture() {
    local outfile="\$1"
    shift
    "\${target_obj_bin}" "${target_node}" "\$@" > "\${tmpdir}/\${outfile}"
}

object_adopt() {
    local range_id="\$1"
    local label="\$2"
    local object_id

    object_ioctl_capture "\${label}.txt" adopt "\${range_id}"
    object_id=\$(awk -F= '/^object_resolved_id=/ { print \$2; exit }' "\${tmpdir}/\${label}.txt")
    if [[ -z "\${object_id}" || "\${object_id}" == "0" ]]; then
        return 1
    fi
    printf '%s\n' "\${object_id}"
}

object_role() {
    local object_id="\$1"
    local role="\$2"

    object_ioctl_capture "object_role_\${object_id}.txt" role "\${object_id}" "\${role}"
}

object_class() {
    local object_id="\$1"
    local object_class="\$2"

    object_ioctl_capture "object_class_\${object_id}.txt" class "\${object_id}" "\${object_class}"
}

object_gpu_use() {
    local object_id="\$1"
    local gpu_use="\$2"

    object_ioctl_capture "object_gpu_use_\${object_id}.txt" gpu_use "\${object_id}" "\${gpu_use}"
}

object_context() {
    local object_id="\$1"
    local context_name="\$2"

    object_ioctl_capture "object_context_\${object_id}.txt" context "\${object_id}" "\${context_name}"
}

object_pf_bind() {
    local object_id="\$1"

    object_ioctl_capture "object_pf_bind_\${object_id}.txt" pf_bind "\${object_id}"
}

object_drop() {
    local object_id="\$1"

    if [[ "\${use_ioctl_object_setup}" == "1" ]]; then
        object_ioctl_capture "object_drop_\${object_id}.txt" drop "\${object_id}"
    else
        echo "drop \${object_id}" > "\${object_node}"
    fi
}

range_query_capture() {
    local outfile="\$1"
    shift
    "\${target_range_bin}" "${target_node}" "\$@" > "\${tmpdir}/\${outfile}"
}

range_query_list_gem_ids() {
    local prefix="\$1"
    local idx=0
    local range_id

    while [[ "\${idx}" -lt "\${range_query_scan_max}" ]]; do
        if ! range_query_capture "\${prefix}_range_query_\${idx}.txt" index "\${idx}" gem 2>/dev/null; then
            break
        fi
        range_id=\$(awk -F= '/^range_resolved_id=/ { print \$2; exit }' "\${tmpdir}/\${prefix}_range_query_\${idx}.txt")
        if [[ -z "\${range_id}" || "\${range_id}" == "0" ]]; then
            echo "Invalid range_resolved_id for query index \${idx}" >&2
            cat "\${tmpdir}/\${prefix}_range_query_\${idx}.txt" >&2
            exit 1
        fi
        printf '%s\n' "\${range_id}"
        idx=\$((idx + 1))
    done
}

submit_ioctl_capture() {
    local outfile="\$1"
    shift
    "\${target_op_bin}" "${target_node}" "\$@" > "\${tmpdir}/\${outfile}"
}

submit_create() {
    local context="\$1"
    local label="\$2"

    submit_ioctl_capture "\${label}.txt" create "\${context}"
    awk -F= '/^submit_resolved_id=/ { print \$2; exit }' "\${tmpdir}/\${label}.txt"
}

submit_focus() {
    local submit_id="\$1"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "focus_\${submit_id}.txt" focus "\${submit_id}"
    else
        echo "focus \${submit_id}" > "\${submit_node}"
    fi
}

submit_validate() {
    local submit_id="\$1"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "validate_\${submit_id}.txt" validate "\${submit_id}"
    else
        echo "validate \${submit_id}" > "\${submit_node}"
    fi
}

submit_prepare() {
    local submit_id="\$1"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "prepare_\${submit_id}.txt" prepare "\${submit_id}"
    fi
}

submit_arm() {
    local submit_id="\$1"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "arm_\${submit_id}.txt" arm "\${submit_id}"
    fi
}

submit_emit() {
    local submit_id="\$1"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "emit_\${submit_id}.txt" emit "\${submit_id}"
    fi
}

submit_launch() {
    local submit_id="\$1"
    local state="\$2"
    local packet_count="\$3"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "launch_\${submit_id}_\${state}_\${packet_count}.txt" launch "\${submit_id}" "\${state}" "\${packet_count}"
    else
        echo "launch is only supported on the typed submit-op path" >&2
        exit 1
    fi
}

submit_issue() {
    local submit_id="\$1"
    local state="\$2"
    local packet_count="\$3"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "issue_\${submit_id}_\${state}_\${packet_count}.txt" issue "\${submit_id}" "\${state}" "\${packet_count}"
    else
        echo "issue \${submit_id} \${state} \${packet_count}" > "\${submit_node}"
    fi
}

submit_transition() {
    local submit_id="\$1"
    local state="\$2"
    local packet_count="\$3"
    local outfile="\$4"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "\${outfile}" transition "\${submit_id}" "\${state}" "\${packet_count}"
    else
        echo "transition \${submit_id} \${state} \${packet_count}" > "\${submit_node}"
    fi
}

submit_attach() {
    local submit_id="\$1"
    local slot_name="\$2"
    local object_id="\$3"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "attach_\${submit_id}_\${slot_name}.txt" attach "\${submit_id}" "\${slot_name}" "\${object_id}"
    else
        echo "attach \${submit_id} \${slot_name} \${object_id}" > "\${submit_node}"
    fi
}

submit_kick() {
    local submit_id="\$1"
    local ccb_offset="\$2"
    local ta_dependency="\$3"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "kick_\${submit_id}.txt" kick "\${submit_id}" ta "\${ccb_offset}" 0x1 1 1 "\${ta_dependency}"
    else
        echo "kick \${submit_id} ta \${ccb_offset} 0x1 1 1 \${ta_dependency}" > "\${submit_node}"
    fi
}

submit_ccb() {
    local submit_id="\$1"
    local write_offset="\$2"
    local read_offset="\$3"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "ccb_\${submit_id}.txt" ccb "\${submit_id}" "\${write_offset}" "\${read_offset}" 0x1000 0x0 0xc0de0001 0xc0de0002
    else
        echo "ccb \${submit_id} \${write_offset} \${read_offset} 0x1000 0x0 0xc0de0001 0xc0de0002" > "\${submit_node}"
    fi
}

submit_queue() {
    local submit_id="\$1"
    local base_offset="\$2"
    local queue_size="\$3"
    local packet_size="\$4"
    local packet_count="\$5"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "queue_\${submit_id}.txt" queue "\${submit_id}" "\${base_offset}" "\${queue_size}" "\${packet_size}" "\${packet_count}"
    else
        echo "queue \${submit_id} \${base_offset} \${queue_size} \${packet_size} \${packet_count}" > "\${submit_node}"
    fi
}

submit_status() {
    local submit_id="\$1"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "status_\${submit_id}.txt" status "\${submit_id}" 0x80 0xc0 0x4 0x5a500000
    else
        echo "status \${submit_id} 0x80 0xc0 0x4 0x5a500000" > "\${submit_node}"
    fi
}

submit_sync() {
    local submit_id="\$1"
    local sync_offset="\$2"
    local complete_seq="\$3"
    local pending_seq="\$4"
    local sync_flags="\$5"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "sync_\${submit_id}.txt" sync "\${submit_id}" "\${sync_offset}" "\${complete_seq}" "\${pending_seq}" "\${sync_flags}"
    else
        echo "sync \${submit_id} \${sync_offset} \${complete_seq} \${pending_seq} \${sync_flags}" > "\${submit_node}"
    fi
}

submit_owner() {
    local submit_id="\$1"
    local owner_offset="\$2"
    local owner_state="\$3"
    local issue_cookie="\$4"
    local complete_cookie="\$5"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "owner_\${submit_id}.txt" owner "\${submit_id}" "\${owner_offset}" "\${owner_state}" "\${issue_cookie}" "\${complete_cookie}"
    else
        echo "owner \${submit_id} \${owner_offset} \${owner_state} \${issue_cookie} \${complete_cookie}" > "\${submit_node}"
    fi
}

submit_report() {
    local submit_id="\$1"
    local report_offset="\$2"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "report_\${submit_id}.txt" report "\${submit_id}" "\${report_offset}"
    else
        echo "report \${submit_id} \${report_offset}" > "\${submit_node}"
    fi
}

submit_result() {
    local submit_id="\$1"
    local result_offset="\$2"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "result_\${submit_id}.txt" result "\${submit_id}" "\${result_offset}"
    else
        echo "result \${submit_id} \${result_offset}" > "\${submit_node}"
    fi
}

submit_manifest() {
    local submit_id="\$1"
    local manifest_offset="\$2"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "manifest_\${submit_id}.txt" manifest "\${submit_id}" "\${manifest_offset}"
    fi
}

submit_descriptor() {
    local submit_id="\$1"
    local descriptor_offset="\$2"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "descriptor_\${submit_id}.txt" descriptor "\${submit_id}" "\${descriptor_offset}"
    fi
}

submit_plan() {
    local submit_id="\$1"
    local plan_offset="\$2"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "plan_\${submit_id}.txt" plan "\${submit_id}" "\${plan_offset}"
    fi
}

submit_issue_plan() {
    local submit_id="\$1"
    local issue_plan_offset="\$2"
    local owner_state="\$3"
    local issued_packets="\$4"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "issue_plan_\${submit_id}.txt" issue-plan "\${submit_id}" "\${issue_plan_offset}" "\${owner_state}" "\${issued_packets}"
    fi
}

submit_live() {
    local submit_id="\$1"
    local live_offset="\$2"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "live_\${submit_id}.txt" live "\${submit_id}" "\${live_offset}"
    fi
}

submit_drop() {
    local submit_id="\$1"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_ioctl_capture "drop_\${submit_id}.txt" drop "\${submit_id}"
    else
        echo "drop \${submit_id}" > "\${submit_node}"
    fi
}

expect_running_busy() {
    local submit_id="\$1"
    local label="\$2"
    local outfile="\$3"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        if submit_transition "\${submit_id}" running 0 "\${outfile}"; then
            echo "Expected running-owner contention failure for submit \${submit_id}" >&2
            exit 1
        fi
    elif echo "transition \${submit_id} running 0" > "\${submit_node}" 2>"\${tmpdir}/\${outfile}"; then
        echo "Expected running-owner contention failure for submit \${submit_id}" >&2
        exit 1
    fi
    echo "\${label}"
    cat "\${tmpdir}/\${outfile}"
}

expect_dependency_wait() {
    local submit_id="\$1"
    local label="\$2"
    local outfile="\$3"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        if submit_transition "\${submit_id}" running 0 "\${outfile}"; then
            echo "Expected dependency-gated running failure for submit \${submit_id}" >&2
            exit 1
        fi
        if ! grep -Eq "errno=11|Resource temporarily unavailable" "\${tmpdir}/\${outfile}"; then
            echo "Unexpected dependency failure text for submit \${submit_id}" >&2
            cat "\${tmpdir}/\${outfile}" >&2
            exit 1
        fi
    else
        if echo "transition \${submit_id} running 0" > "\${submit_node}" 2>"\${tmpdir}/\${outfile}"; then
            echo "Expected dependency-gated running failure for submit \${submit_id}" >&2
            exit 1
        fi
        if ! grep -q "Resource temporarily unavailable" "\${tmpdir}/\${outfile}"; then
            echo "Unexpected dependency failure text for submit \${submit_id}" >&2
            cat "\${tmpdir}/\${outfile}" >&2
            exit 1
        fi
    fi
    echo "\${label}"
    cat "\${tmpdir}/\${outfile}"
}

configure_dependency_slot() {
    local submit_id="\$1"
    local slot="\$2"
    local dependency_submit_id="\$3"

    if [[ "\${use_ioctl_dependency_setup}" == "1" ]]; then
        "\${target_op_bin}" "${target_node}" depend-slot "\${submit_id}" "\${slot}" "\${dependency_submit_id}" >/dev/null
        return
    fi

    if [[ "\${use_indexed_dependencies}" == "1" ]]; then
        echo "depend_slot \${submit_id} \${slot} \${dependency_submit_id}" > "\${submit_node}"
        return
    fi

    case "\${slot}" in
        1)
            echo "depend \${submit_id} \${dependency_submit_id}" > "\${submit_node}"
            ;;
        2)
            echo "depend2 \${submit_id} \${dependency_submit_id}" > "\${submit_node}"
            ;;
        3)
            echo "depend3 \${submit_id} \${dependency_submit_id}" > "\${submit_node}"
            ;;
        4)
            echo "Dependency slot 4 requires USE_INDEXED_DEPENDENCIES=1" >&2
            exit 1
            ;;
        *)
            echo "Unsupported dependency slot: \${slot}" >&2
            exit 1
            ;;
    esac
}

configure_dependency_ticket_slot() {
    local submit_id="\$1"
    local slot="\$2"
    local required_ticket="\$3"

    if [[ "\${use_ioctl_dependency_setup}" == "1" ]]; then
        "\${target_op_bin}" "${target_node}" depticket-slot "\${submit_id}" "\${slot}" "\${required_ticket}" >/dev/null
        return
    fi

    if [[ "\${use_indexed_dependencies}" == "1" ]]; then
        echo "depticket_slot \${submit_id} \${slot} \${required_ticket}" > "\${submit_node}"
        return
    fi

    case "\${slot}" in
        1)
            ;;
        2)
            echo "depticket2 \${submit_id} \${required_ticket}" > "\${submit_node}"
            ;;
        3)
            echo "depticket3 \${submit_id} \${required_ticket}" > "\${submit_node}"
            ;;
        4)
            echo "Dependency ticket slot 4 requires USE_INDEXED_DEPENDENCIES=1" >&2
            exit 1
            ;;
        *)
            echo "Unsupported dependency-ticket slot: \${slot}" >&2
            exit 1
            ;;
    esac
}

configure_dependency_state_slot() {
    local submit_id="\$1"
    local slot="\$2"
    local offset="\$3"

    if [[ "\${use_ioctl_dependency_setup}" == "1" ]]; then
        "\${target_op_bin}" "${target_node}" depstate-slot "\${submit_id}" "\${slot}" "\${offset}" >/dev/null
        return
    fi

    if [[ "\${use_indexed_dependencies}" == "1" ]]; then
        echo "depstate_slot \${submit_id} \${slot} \${offset}" > "\${submit_node}"
        return
    fi

    case "\${slot}" in
        1)
            echo "depstate \${submit_id} \${offset}" > "\${submit_node}"
            ;;
        2)
            echo "depstate2 \${submit_id} \${offset}" > "\${submit_node}"
            ;;
        3)
            echo "depstate3 \${submit_id} \${offset}" > "\${submit_node}"
            ;;
        4)
            echo "Dependency state slot 4 requires USE_INDEXED_DEPENDENCIES=1" >&2
            exit 1
            ;;
        *)
            echo "Unsupported dependency-state slot: \${slot}" >&2
            exit 1
            ;;
    esac
}

refresh_dependency_state_slot() {
    local submit_id="\$1"
    local slot="\$2"
    local offset="\$3"
    local label="\$4"
    local outfile="\$5"

    configure_dependency_state_slot "\${submit_id}" "\${slot}" "\${offset}"
    submit_validate "\${submit_id}"
    submit_focus_dump "\${submit_id}" "\${label}" "\${outfile}"
}

clear_dependency_slot() {
    local submit_id="\$1"
    local slot="\$2"

    if [[ "\${use_ioctl_dependency_setup}" == "1" ]]; then
        "\${target_op_bin}" "${target_node}" depclear-slot "\${submit_id}" "\${slot}" >/dev/null
        return
    fi

    if [[ "\${use_indexed_dependencies}" != "1" ]]; then
        echo "Dependency slot clear requires USE_INDEXED_DEPENDENCIES=1" >&2
        exit 1
    fi

    echo "depclear_slot \${submit_id} \${slot}" > "\${submit_node}"
}

refresh_dependency_state() {
    local submit_id="\$1"
    local offset="\$2"
    local label="\$3"
    local outfile="\$4"

    refresh_dependency_state_slot "\${submit_id}" 1 "\${offset}" "\${label}" "\${outfile}"
}

refresh_dependency_state2() {
    local submit_id="\$1"
    local offset="\$2"
    local label="\$3"
    local outfile="\$4"

    refresh_dependency_state_slot "\${submit_id}" 2 "\${offset}" "\${label}" "\${outfile}"
}

validate_submit_state() {
    local submit_id="\$1"
    local label="\$2"
    local outfile="\$3"

    submit_validate "\${submit_id}"
    submit_focus_dump "\${submit_id}" "\${label}" "\${outfile}"
}

setup_object() {
    local object_id="\$1"
    local role="\$2"
    local object_class="\$3"
    local gpu_use="\$4"

    if [[ "\${use_ioctl_object_setup}" == "1" ]]; then
        object_role "\${object_id}" "\${role}"
        object_class "\${object_id}" "\${object_class}"
        object_gpu_use "\${object_id}" "\${gpu_use}"
        object_context "\${object_id}" pf
        object_pf_bind "\${object_id}"
    else
        echo "role \${object_id} \${role}" > "\${object_node}"
        echo "class \${object_id} \${object_class}" > "\${object_node}"
        echo "gpu_use \${object_id} \${gpu_use}" > "\${object_node}"
        echo "context \${object_id} pf" > "\${object_node}"
        echo "pf_bind \${object_id}" > "\${object_node}"
    fi
}

configure_submit() {
    local submit_id="\$1"
    local mode="\$2"
    local object_control="\$3"
    local object_pds="\$4"
    local object_use="\$5"
    local object_target="\$6"
    local object_status="\$7"
    local ta_dependency="\${8:-0}"
    local kick_ccb_offset
    local ccb_write_offset
    local ccb_read_offset
    local queue_base_offset="0x20"
    local queue_size
    local queue_packet_size="0x20"
    local queue_packet_count="1"
    local sync_offset="0x20"
    local sync_complete_seq="0x40"
    local sync_pending_seq="0x41"
    local sync_flags="0x1"
    local owner_offset="0x40"
    local owner_issue_cookie="0x101"
    local owner_complete_cookie="0x100"
    local report_offset="0x120"
    local result_offset="0x180"
    local manifest_offset="0x180"
    local descriptor_offset="0x200"
    local plan_offset="0x300"
    local live_offset="0x3c0"
    local issue_plan_offset="0x4a0"

    case "\${mode}" in
        wrap)
            kick_ccb_offset="0x80"
            ccb_write_offset="0x0"
            ccb_read_offset="0x60"
            queue_size="0x80"
            ;;
        nowrap)
            kick_ccb_offset="0x20"
            ccb_write_offset="0x20"
            ccb_read_offset="0x0"
            queue_size="0x40"
            ;;
        *)
            echo "Unsupported mode in configure_submit: \${mode}" >&2
            exit 1
            ;;
    esac

    submit_attach "\${submit_id}" control "\${object_control}"
    submit_attach "\${submit_id}" pds "\${object_pds}"
    submit_attach "\${submit_id}" use "\${object_use}"
    submit_attach "\${submit_id}" target "\${object_target}"
    submit_attach "\${submit_id}" status "\${object_status}"
    submit_kick "\${submit_id}" "\${kick_ccb_offset}" "\${ta_dependency}"
    submit_ccb "\${submit_id}" "\${ccb_write_offset}" "\${ccb_read_offset}"
    submit_queue "\${submit_id}" "\${queue_base_offset}" "\${queue_size}" "\${queue_packet_size}" "\${queue_packet_count}"
    submit_status "\${submit_id}"
    submit_sync "\${submit_id}" "\${sync_offset}" "\${sync_complete_seq}" "\${sync_pending_seq}" "\${sync_flags}"
    submit_owner "\${submit_id}" "\${owner_offset}" queued "\${owner_issue_cookie}" "\${owner_complete_cookie}"
    submit_report "\${submit_id}" "\${report_offset}"
    submit_live "\${submit_id}" "\${live_offset}"
    submit_manifest "\${submit_id}" "\${manifest_offset}"
    submit_descriptor "\${submit_id}" "\${descriptor_offset}"
    submit_plan "\${submit_id}" "\${plan_offset}"
    submit_issue_plan "\${submit_id}" "\${issue_plan_offset}" queued 1
    submit_result "\${submit_id}" "\${result_offset}"
    submit_prepare "\${submit_id}"
    submit_validate "\${submit_id}"
}

drive_submit_initial_cycle() {
    local submit_id="\$1"
    local prefix="\$2"
    local contend_submit_id="\${3:-}"
    local contend_prefix="\${4:-}"
    local dependency_active_count="\${5:-0}"
    local running_completed="1"
    local complete_completed="1"

    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_focus_dump "\${submit_id}" "\${prefix}_AFTER_PREPARE" "\${prefix}_after_prepare.txt"
        submit_validate "\${submit_id}"
        submit_focus_dump "\${submit_id}" "\${prefix}_AFTER_PREPARE_VALIDATE" "\${prefix}_after_prepare_validate.txt"
        if [[ "\${use_ioctl_verification}" == "1" ]]; then
            assert_report_snapshot_file "\${prefix}_after_prepare.txt" prepared 1 queued 1 0 "\${dependency_active_count}"
            assert_report_snapshot_file "\${prefix}_after_prepare_validate.txt" prepared 1 queued 1 0 "\${dependency_active_count}"
            assert_manifest_snapshot_file "\${prefix}_after_prepare.txt" prepared 1 queued 1 "\${dependency_active_count}"
            assert_manifest_snapshot_file "\${prefix}_after_prepare_validate.txt" prepared 1 queued 1 "\${dependency_active_count}"
            assert_descriptor_snapshot_file "\${prefix}_after_prepare.txt" prepared 1 queued 0 0 1 "\${dependency_active_count}"
            assert_descriptor_snapshot_file "\${prefix}_after_prepare_validate.txt" prepared 1 queued 0 0 1 "\${dependency_active_count}"
            assert_live_snapshot_file "\${prefix}_after_prepare.txt" prepared 1 queued 0 0 1 0 "\${dependency_active_count}"
            assert_live_snapshot_file "\${prefix}_after_prepare_validate.txt" prepared 1 queued 0 0 1 0 "\${dependency_active_count}"
            assert_plan_snapshot_file "\${prefix}_after_prepare.txt" prepared 1 live queued 0 0 1 0 "\${dependency_active_count}"
            assert_plan_snapshot_file "\${prefix}_after_prepare_validate.txt" prepared 1 live queued 0 0 1 0 "\${dependency_active_count}"
            assert_issue_plan_snapshot_file "\${prefix}_after_prepare.txt" prepared 1 0 none 0 0 0 0 0
            assert_issue_plan_snapshot_file "\${prefix}_after_prepare_validate.txt" prepared 1 0 none 0 0 0 0 0
            assert_result_snapshot_file "\${prefix}_after_prepare.txt" prepared 1 live queued 0 0 1 0 "\${dependency_active_count}"
            assert_result_snapshot_file "\${prefix}_after_prepare_validate.txt" prepared 1 live queued 0 0 1 0 "\${dependency_active_count}"
        fi
    fi
    submit_arm "\${submit_id}"
    if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
        submit_focus_dump "\${submit_id}" "\${prefix}_AFTER_ARM" "\${prefix}_after_arm.txt"
        submit_validate "\${submit_id}"
        submit_focus_dump "\${submit_id}" "\${prefix}_AFTER_ARM_VALIDATE" "\${prefix}_after_arm_validate.txt"
        if [[ "\${use_ioctl_verification}" == "1" ]]; then
            assert_report_snapshot_file "\${prefix}_after_arm.txt" armed 1 queued 1 0 "\${dependency_active_count}"
            assert_report_snapshot_file "\${prefix}_after_arm_validate.txt" armed 1 queued 1 0 "\${dependency_active_count}"
            assert_manifest_snapshot_file "\${prefix}_after_arm.txt" armed 1 queued 1 "\${dependency_active_count}"
            assert_manifest_snapshot_file "\${prefix}_after_arm_validate.txt" armed 1 queued 1 "\${dependency_active_count}"
            assert_descriptor_snapshot_file "\${prefix}_after_arm.txt" armed 1 queued 0 0 1 "\${dependency_active_count}"
            assert_descriptor_snapshot_file "\${prefix}_after_arm_validate.txt" armed 1 queued 0 0 1 "\${dependency_active_count}"
            assert_live_snapshot_file "\${prefix}_after_arm.txt" armed 1 queued 0 0 1 0 "\${dependency_active_count}"
            assert_live_snapshot_file "\${prefix}_after_arm_validate.txt" armed 1 queued 0 0 1 0 "\${dependency_active_count}"
            assert_plan_snapshot_file "\${prefix}_after_arm.txt" armed 1 live queued 0 0 1 0 "\${dependency_active_count}"
            assert_plan_snapshot_file "\${prefix}_after_arm_validate.txt" armed 1 live queued 0 0 1 0 "\${dependency_active_count}"
            assert_issue_plan_snapshot_file "\${prefix}_after_arm.txt" armed 1 0 none 0 0 0 0 0
            assert_issue_plan_snapshot_file "\${prefix}_after_arm_validate.txt" armed 1 0 none 0 0 0 0 0
            assert_result_snapshot_file "\${prefix}_after_arm.txt" armed 1 live queued 0 0 1 0 "\${dependency_active_count}"
            assert_result_snapshot_file "\${prefix}_after_arm_validate.txt" armed 1 live queued 0 0 1 0 "\${dependency_active_count}"
        fi
        submit_emit "\${submit_id}"
        submit_focus_dump "\${submit_id}" "\${prefix}_AFTER_EMIT" "\${prefix}_after_emit.txt"
        submit_validate "\${submit_id}"
        submit_focus_dump "\${submit_id}" "\${prefix}_AFTER_EMIT_VALIDATE" "\${prefix}_after_emit_validate.txt"
        if [[ "\${use_ioctl_verification}" == "1" ]]; then
            assert_report_snapshot_file "\${prefix}_after_emit.txt" emitted 1 queued 1 0 "\${dependency_active_count}"
            assert_report_snapshot_file "\${prefix}_after_emit_validate.txt" emitted 1 queued 1 0 "\${dependency_active_count}"
            assert_manifest_snapshot_file "\${prefix}_after_emit.txt" emitted 1 queued 1 "\${dependency_active_count}"
            assert_manifest_snapshot_file "\${prefix}_after_emit_validate.txt" emitted 1 queued 1 "\${dependency_active_count}"
            assert_descriptor_snapshot_file "\${prefix}_after_emit.txt" emitted 1 queued 0 0 1 "\${dependency_active_count}"
            assert_descriptor_snapshot_file "\${prefix}_after_emit_validate.txt" emitted 1 queued 0 0 1 "\${dependency_active_count}"
            assert_live_snapshot_file "\${prefix}_after_emit.txt" emitted 1 queued 0 0 1 0 "\${dependency_active_count}"
            assert_live_snapshot_file "\${prefix}_after_emit_validate.txt" emitted 1 queued 0 0 1 0 "\${dependency_active_count}"
            assert_plan_snapshot_file "\${prefix}_after_emit.txt" emitted 1 live queued 0 0 1 0 "\${dependency_active_count}"
            assert_plan_snapshot_file "\${prefix}_after_emit_validate.txt" emitted 1 live queued 0 0 1 0 "\${dependency_active_count}"
            assert_issue_plan_snapshot_file "\${prefix}_after_emit.txt" emitted 1 0 none 0 0 0 0 0
            assert_issue_plan_snapshot_file "\${prefix}_after_emit_validate.txt" emitted 1 0 none 0 0 0 0 0
            assert_result_snapshot_file "\${prefix}_after_emit.txt" emitted 1 live queued 0 0 1 0 "\${dependency_active_count}"
            assert_result_snapshot_file "\${prefix}_after_emit_validate.txt" emitted 1 live queued 0 0 1 0 "\${dependency_active_count}"
        fi
        submit_launch "\${submit_id}" queued 1
        submit_focus_dump "\${submit_id}" "\${prefix}_AFTER_LAUNCH" "\${prefix}_after_launch.txt"
        submit_validate "\${submit_id}"
        submit_focus_dump "\${submit_id}" "\${prefix}_AFTER_LAUNCH_VALIDATE" "\${prefix}_after_launch_validate.txt"
        if [[ "\${use_ioctl_verification}" == "1" ]]; then
            assert_report_snapshot_file "\${prefix}_after_launch.txt" launched 1 queued 1 0 "\${dependency_active_count}"
            assert_report_snapshot_file "\${prefix}_after_launch_validate.txt" launched 1 queued 1 0 "\${dependency_active_count}"
            assert_manifest_snapshot_file "\${prefix}_after_launch.txt" launched 1 queued 1 "\${dependency_active_count}"
            assert_manifest_snapshot_file "\${prefix}_after_launch_validate.txt" launched 1 queued 1 "\${dependency_active_count}"
            assert_descriptor_snapshot_file "\${prefix}_after_launch.txt" launched 1 queued 1 0 1 "\${dependency_active_count}"
            assert_descriptor_snapshot_file "\${prefix}_after_launch_validate.txt" launched 1 queued 1 0 1 "\${dependency_active_count}"
            assert_live_snapshot_file "\${prefix}_after_launch.txt" launched 1 queued 1 0 1 0 "\${dependency_active_count}"
            assert_live_snapshot_file "\${prefix}_after_launch_validate.txt" launched 1 queued 1 0 1 0 "\${dependency_active_count}"
            assert_plan_snapshot_file "\${prefix}_after_launch.txt" launched 1 launch queued 1 0 2 0 "\${dependency_active_count}"
            assert_plan_snapshot_file "\${prefix}_after_launch_validate.txt" launched 1 launch queued 1 0 2 0 "\${dependency_active_count}"
            assert_issue_plan_snapshot_file "\${prefix}_after_launch.txt" launched 1 1 queued 1 0 2 0 "\${dependency_active_count}"
            assert_issue_plan_snapshot_file "\${prefix}_after_launch_validate.txt" launched 1 1 queued 1 0 2 0 "\${dependency_active_count}"
            assert_result_snapshot_file "\${prefix}_after_launch.txt" launched 1 live queued 1 0 1 0 "\${dependency_active_count}"
            assert_result_snapshot_file "\${prefix}_after_launch_validate.txt" launched 1 live queued 1 0 1 0 "\${dependency_active_count}"
        fi
    fi
    submit_issue "\${submit_id}" queued 1
    submit_focus_dump "\${submit_id}" "\${prefix}_AFTER_ISSUE" "\${prefix}_after_issue.txt"
    if [[ "\${use_ioctl_verification}" == "1" ]]; then
        assert_report_snapshot_file "\${prefix}_after_issue.txt" dispatched 1 queued 2 0 "\${dependency_active_count}"
        assert_dispatch_snapshot_file "\${prefix}_after_issue.txt" 1 queued 1 0 2 "\${dependency_active_count}"
        assert_manifest_snapshot_file "\${prefix}_after_issue.txt" dispatched 1 queued 2 "\${dependency_active_count}"
        assert_descriptor_snapshot_file "\${prefix}_after_issue.txt" dispatched 1 queued 1 0 2 "\${dependency_active_count}"
        assert_live_snapshot_file "\${prefix}_after_issue.txt" dispatched 1 queued 1 0 2 0 "\${dependency_active_count}"
        assert_plan_snapshot_file "\${prefix}_after_issue.txt" dispatched 1 dispatch queued 1 0 2 0 "\${dependency_active_count}"
        assert_issue_plan_snapshot_file "\${prefix}_after_issue.txt" dispatched 1 0 none 0 0 0 0 0
        assert_result_snapshot_file "\${prefix}_after_issue.txt" dispatched 1 dispatch queued 1 0 2 0 "\${dependency_active_count}"
    fi
    submit_validate "\${submit_id}"
    submit_focus_dump "\${submit_id}" "\${prefix}_AFTER_ISSUE_VALIDATE" "\${prefix}_after_issue_validate.txt"
    if [[ "\${use_ioctl_verification}" == "1" ]]; then
        assert_report_snapshot_file "\${prefix}_after_issue_validate.txt" dispatched 1 queued 2 0 "\${dependency_active_count}"
        assert_dispatch_snapshot_file "\${prefix}_after_issue_validate.txt" 1 queued 1 0 2 "\${dependency_active_count}"
        assert_manifest_snapshot_file "\${prefix}_after_issue_validate.txt" dispatched 1 queued 2 "\${dependency_active_count}"
        assert_descriptor_snapshot_file "\${prefix}_after_issue_validate.txt" dispatched 1 queued 1 0 2 "\${dependency_active_count}"
        assert_live_snapshot_file "\${prefix}_after_issue_validate.txt" dispatched 1 queued 1 0 2 0 "\${dependency_active_count}"
        assert_plan_snapshot_file "\${prefix}_after_issue_validate.txt" dispatched 1 dispatch queued 1 0 2 0 "\${dependency_active_count}"
        assert_issue_plan_snapshot_file "\${prefix}_after_issue_validate.txt" dispatched 1 0 none 0 0 0 0 0
        assert_result_snapshot_file "\${prefix}_after_issue_validate.txt" dispatched 1 dispatch queued 1 0 2 0 "\${dependency_active_count}"
    fi
    submit_transition "\${submit_id}" running "\${running_completed}" "transition_\${submit_id}_running_\${running_completed}.txt"
    submit_focus_dump "\${submit_id}" "\${prefix}_AFTER_RUNNING" "\${prefix}_after_running.txt"
    if [[ "\${use_ioctl_verification}" == "1" ]]; then
        assert_report_snapshot_file "\${prefix}_after_running.txt" dispatched 1 running 1 1 "\${dependency_active_count}"
        assert_dispatch_snapshot_file "\${prefix}_after_running.txt" 1 running 1 1 1 "\${dependency_active_count}"
        assert_manifest_snapshot_file "\${prefix}_after_running.txt" dispatched 1 running 1 "\${dependency_active_count}"
        assert_descriptor_snapshot_file "\${prefix}_after_running.txt" dispatched 1 running 1 1 1 "\${dependency_active_count}"
        assert_live_snapshot_file "\${prefix}_after_running.txt" dispatched 1 running 1 1 1 0 "\${dependency_active_count}"
        assert_plan_snapshot_file "\${prefix}_after_running.txt" dispatched 1 dispatch running 1 1 1 0 "\${dependency_active_count}"
        assert_issue_plan_snapshot_file "\${prefix}_after_running.txt" dispatched 1 0 none 0 0 0 0 0
        assert_result_snapshot_file "\${prefix}_after_running.txt" dispatched 1 dispatch running 1 1 1 0 "\${dependency_active_count}"
    fi
    submit_validate "\${submit_id}"
    submit_focus_dump "\${submit_id}" "\${prefix}_AFTER_RUNNING_VALIDATE" "\${prefix}_after_running_validate.txt"
    if [[ "\${use_ioctl_verification}" == "1" ]]; then
        assert_report_snapshot_file "\${prefix}_after_running_validate.txt" dispatched 1 running 1 1 "\${dependency_active_count}"
        assert_dispatch_snapshot_file "\${prefix}_after_running_validate.txt" 1 running 1 1 1 "\${dependency_active_count}"
        assert_manifest_snapshot_file "\${prefix}_after_running_validate.txt" dispatched 1 running 1 "\${dependency_active_count}"
        assert_descriptor_snapshot_file "\${prefix}_after_running_validate.txt" dispatched 1 running 1 1 1 "\${dependency_active_count}"
        assert_live_snapshot_file "\${prefix}_after_running_validate.txt" dispatched 1 running 1 1 1 0 "\${dependency_active_count}"
        assert_plan_snapshot_file "\${prefix}_after_running_validate.txt" dispatched 1 dispatch running 1 1 1 0 "\${dependency_active_count}"
        assert_issue_plan_snapshot_file "\${prefix}_after_running_validate.txt" dispatched 1 0 none 0 0 0 0 0
        assert_result_snapshot_file "\${prefix}_after_running_validate.txt" dispatched 1 dispatch running 1 1 1 0 "\${dependency_active_count}"
    fi
    if [[ -n "\${contend_submit_id}" ]]; then
        expect_running_busy "\${contend_submit_id}" "\${contend_prefix}_RUNNING_BUSY_WHILE_\${prefix}_ACTIVE" "\${contend_prefix}_running_busy_while_\${prefix}_active.txt"
        submit_focus_dump "\${contend_submit_id}" "\${contend_prefix}_STILL_IDLE_WHILE_\${prefix}_ACTIVE" "\${contend_prefix}_still_idle_while_\${prefix}_active.txt"
        submit_coord_dump "COORD_STATE_WHILE_\${prefix}_RUNNING" "coord_state_while_\${prefix}_running.txt"
    fi
    submit_transition "\${submit_id}" complete "\${complete_completed}" "transition_\${submit_id}_complete_\${complete_completed}.txt"
    submit_focus_dump "\${submit_id}" "\${prefix}_AFTER_COMPLETE" "\${prefix}_after_complete.txt"
    if [[ "\${use_ioctl_verification}" == "1" ]]; then
        assert_report_snapshot_file "\${prefix}_after_complete.txt" retired 1 complete 0 2 "\${dependency_active_count}"
        assert_dispatch_snapshot_file "\${prefix}_after_complete.txt" 1 complete 1 2 0 "\${dependency_active_count}"
        assert_retired_snapshot_file "\${prefix}_after_complete.txt" 1 1 complete 1 2 0 "\${dependency_active_count}"
        assert_manifest_snapshot_file "\${prefix}_after_complete.txt" retired 1 complete 0 "\${dependency_active_count}"
        assert_descriptor_snapshot_file "\${prefix}_after_complete.txt" retired 1 complete 1 2 0 "\${dependency_active_count}"
        assert_live_snapshot_file "\${prefix}_after_complete.txt" retired 1 complete 1 2 0 0 "\${dependency_active_count}"
        assert_plan_snapshot_file "\${prefix}_after_complete.txt" retired 1 retired complete 1 2 0 0 "\${dependency_active_count}"
        assert_issue_plan_snapshot_file "\${prefix}_after_complete.txt" retired 1 0 none 0 0 0 0 0
        assert_result_snapshot_file "\${prefix}_after_complete.txt" retired 1 retired complete 1 2 0 0 "\${dependency_active_count}"
    fi
    submit_validate "\${submit_id}"
    submit_focus_dump "\${submit_id}" "\${prefix}_AFTER_COMPLETE_VALIDATE" "\${prefix}_after_complete_validate.txt"
    if [[ "\${use_ioctl_verification}" == "1" ]]; then
        assert_report_snapshot_file "\${prefix}_after_complete_validate.txt" retired 1 complete 0 2 "\${dependency_active_count}"
        assert_dispatch_snapshot_file "\${prefix}_after_complete_validate.txt" 1 complete 1 2 0 "\${dependency_active_count}"
        assert_retired_snapshot_file "\${prefix}_after_complete_validate.txt" 1 1 complete 1 2 0 "\${dependency_active_count}"
        assert_manifest_snapshot_file "\${prefix}_after_complete_validate.txt" retired 1 complete 0 "\${dependency_active_count}"
        assert_descriptor_snapshot_file "\${prefix}_after_complete_validate.txt" retired 1 complete 1 2 0 "\${dependency_active_count}"
        assert_live_snapshot_file "\${prefix}_after_complete_validate.txt" retired 1 complete 1 2 0 0 "\${dependency_active_count}"
        assert_plan_snapshot_file "\${prefix}_after_complete_validate.txt" retired 1 retired complete 1 2 0 0 "\${dependency_active_count}"
        assert_issue_plan_snapshot_file "\${prefix}_after_complete_validate.txt" retired 1 0 none 0 0 0 0 0
        assert_result_snapshot_file "\${prefix}_after_complete_validate.txt" retired 1 retired complete 1 2 0 0 "\${dependency_active_count}"
    fi
}

drive_submit_refill_cycles() {
    local submit_id="\$1"
    local refill_cycles="\$2"
    local prefix="\$3"
    local dependency_active_count="\${4:-0}"
    local cycle
    local expected_count

    if [[ "\${refill_cycles}" -le 0 ]]; then
        return
    fi

    for cycle in \$(seq 1 "\${refill_cycles}"); do
        submit_arm "\${submit_id}"
        if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
            submit_focus_dump "\${submit_id}" "\${prefix}_REFILL_CYCLE_\${cycle}_ARM" "\${prefix}_refill_cycle_\${cycle}_arm.txt"
            submit_validate "\${submit_id}"
            submit_focus_dump "\${submit_id}" "\${prefix}_REFILL_CYCLE_\${cycle}_ARM_VALIDATE" "\${prefix}_refill_cycle_\${cycle}_arm_validate.txt"
            if [[ "\${use_ioctl_verification}" == "1" ]]; then
                assert_report_snapshot_file "\${prefix}_refill_cycle_\${cycle}_arm.txt" armed "\$((cycle + 1))" complete 0 0 "\${dependency_active_count}"
                assert_report_snapshot_file "\${prefix}_refill_cycle_\${cycle}_arm_validate.txt" armed "\$((cycle + 1))" complete 0 0 "\${dependency_active_count}"
                assert_manifest_snapshot_file "\${prefix}_refill_cycle_\${cycle}_arm.txt" armed "\$((cycle + 1))" complete 0 "\${dependency_active_count}"
                assert_manifest_snapshot_file "\${prefix}_refill_cycle_\${cycle}_arm_validate.txt" armed "\$((cycle + 1))" complete 0 "\${dependency_active_count}"
                assert_descriptor_snapshot_file "\${prefix}_refill_cycle_\${cycle}_arm.txt" armed "\$((cycle + 1))" complete 0 0 0 "\${dependency_active_count}"
                assert_descriptor_snapshot_file "\${prefix}_refill_cycle_\${cycle}_arm_validate.txt" armed "\$((cycle + 1))" complete 0 0 0 "\${dependency_active_count}"
                assert_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_arm.txt" armed "\$((cycle + 1))" live complete 0 0 0 "\$((cycle - 1))" "\${dependency_active_count}"
                assert_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_arm_validate.txt" armed "\$((cycle + 1))" live complete 0 0 0 "\$((cycle - 1))" "\${dependency_active_count}"
                assert_issue_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_arm.txt" armed "\$((cycle + 1))" 0 none 0 0 0 0 0
                assert_issue_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_arm_validate.txt" armed "\$((cycle + 1))" 0 none 0 0 0 0 0
            fi
            submit_emit "\${submit_id}"
            submit_focus_dump "\${submit_id}" "\${prefix}_REFILL_CYCLE_\${cycle}_EMIT" "\${prefix}_refill_cycle_\${cycle}_emit.txt"
            submit_validate "\${submit_id}"
            submit_focus_dump "\${submit_id}" "\${prefix}_REFILL_CYCLE_\${cycle}_EMIT_VALIDATE" "\${prefix}_refill_cycle_\${cycle}_emit_validate.txt"
            if [[ "\${use_ioctl_verification}" == "1" ]]; then
                assert_report_snapshot_file "\${prefix}_refill_cycle_\${cycle}_emit.txt" emitted "\$((cycle + 1))" complete 0 0 "\${dependency_active_count}"
                assert_report_snapshot_file "\${prefix}_refill_cycle_\${cycle}_emit_validate.txt" emitted "\$((cycle + 1))" complete 0 0 "\${dependency_active_count}"
                assert_manifest_snapshot_file "\${prefix}_refill_cycle_\${cycle}_emit.txt" emitted "\$((cycle + 1))" complete 0 "\${dependency_active_count}"
                assert_manifest_snapshot_file "\${prefix}_refill_cycle_\${cycle}_emit_validate.txt" emitted "\$((cycle + 1))" complete 0 "\${dependency_active_count}"
                assert_descriptor_snapshot_file "\${prefix}_refill_cycle_\${cycle}_emit.txt" emitted "\$((cycle + 1))" complete 0 0 0 "\${dependency_active_count}"
                assert_descriptor_snapshot_file "\${prefix}_refill_cycle_\${cycle}_emit_validate.txt" emitted "\$((cycle + 1))" complete 0 0 0 "\${dependency_active_count}"
                assert_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_emit.txt" emitted "\$((cycle + 1))" live complete 0 0 0 "\$((cycle - 1))" "\${dependency_active_count}"
                assert_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_emit_validate.txt" emitted "\$((cycle + 1))" live complete 0 0 0 "\$((cycle - 1))" "\${dependency_active_count}"
                assert_issue_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_emit.txt" emitted "\$((cycle + 1))" 0 none 0 0 0 0 0
                assert_issue_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_emit_validate.txt" emitted "\$((cycle + 1))" 0 none 0 0 0 0 0
            fi
            submit_launch "\${submit_id}" queued 1
            submit_focus_dump "\${submit_id}" "\${prefix}_REFILL_CYCLE_\${cycle}_LAUNCH" "\${prefix}_refill_cycle_\${cycle}_launch.txt"
            submit_validate "\${submit_id}"
            submit_focus_dump "\${submit_id}" "\${prefix}_REFILL_CYCLE_\${cycle}_LAUNCH_VALIDATE" "\${prefix}_refill_cycle_\${cycle}_launch_validate.txt"
            if [[ "\${use_ioctl_verification}" == "1" ]]; then
                assert_report_snapshot_file "\${prefix}_refill_cycle_\${cycle}_launch.txt" launched "\$((cycle + 1))" complete 0 0 "\${dependency_active_count}"
                assert_report_snapshot_file "\${prefix}_refill_cycle_\${cycle}_launch_validate.txt" launched "\$((cycle + 1))" complete 0 0 "\${dependency_active_count}"
                assert_manifest_snapshot_file "\${prefix}_refill_cycle_\${cycle}_launch.txt" launched "\$((cycle + 1))" complete 0 "\${dependency_active_count}"
                assert_manifest_snapshot_file "\${prefix}_refill_cycle_\${cycle}_launch_validate.txt" launched "\$((cycle + 1))" complete 0 "\${dependency_active_count}"
                assert_descriptor_snapshot_file "\${prefix}_refill_cycle_\${cycle}_launch.txt" launched "\$((cycle + 1))" complete 1 0 0 "\${dependency_active_count}"
                assert_descriptor_snapshot_file "\${prefix}_refill_cycle_\${cycle}_launch_validate.txt" launched "\$((cycle + 1))" complete 1 0 0 "\${dependency_active_count}"
                assert_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_launch.txt" launched "\$((cycle + 1))" launch queued 1 0 1 "\${cycle}" "\${dependency_active_count}"
                assert_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_launch_validate.txt" launched "\$((cycle + 1))" launch queued 1 0 1 "\${cycle}" "\${dependency_active_count}"
                assert_issue_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_launch.txt" launched "\$((cycle + 1))" 1 queued 1 1 1 "\${cycle}" "\${dependency_active_count}"
                assert_issue_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_launch_validate.txt" launched "\$((cycle + 1))" 1 queued 1 1 1 "\${cycle}" "\${dependency_active_count}"
            fi
        fi
        submit_issue "\${submit_id}" queued 1
        submit_focus_dump "\${submit_id}" "\${prefix}_REFILL_CYCLE_\${cycle}_ISSUE" "\${prefix}_refill_cycle_\${cycle}_issue.txt"
        expected_count="\$((cycle + 1))"
        if [[ "\${use_ioctl_verification}" == "1" ]]; then
            assert_report_snapshot_file "\${prefix}_refill_cycle_\${cycle}_issue.txt" dispatched "\${expected_count}" queued 1 0 "\${dependency_active_count}"
            assert_dispatch_snapshot_file "\${prefix}_refill_cycle_\${cycle}_issue.txt" "\${expected_count}" queued 1 0 1 "\${dependency_active_count}"
            assert_retired_persisted_snapshot_file "\${prefix}_refill_cycle_\${cycle}_issue.txt" "\$((expected_count - 1))" "\$((expected_count - 1))" complete 1 1 0 "\${dependency_active_count}"
            assert_manifest_snapshot_file "\${prefix}_refill_cycle_\${cycle}_issue.txt" dispatched "\${expected_count}" queued 1 "\${dependency_active_count}"
            assert_descriptor_snapshot_file "\${prefix}_refill_cycle_\${cycle}_issue.txt" dispatched "\${expected_count}" queued 1 0 1 "\${dependency_active_count}"
            assert_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_issue.txt" dispatched "\${expected_count}" dispatch queued 1 0 1 "\${cycle}" "\${dependency_active_count}"
            assert_issue_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_issue.txt" dispatched "\${expected_count}" 0 none 0 0 0 0 0
        fi
        submit_validate "\${submit_id}"
        submit_focus_dump "\${submit_id}" "\${prefix}_REFILL_CYCLE_\${cycle}_ISSUE_VALIDATE" "\${prefix}_refill_cycle_\${cycle}_issue_validate.txt"
        if [[ "\${use_ioctl_verification}" == "1" ]]; then
            assert_report_snapshot_file "\${prefix}_refill_cycle_\${cycle}_issue_validate.txt" dispatched "\${expected_count}" queued 1 0 "\${dependency_active_count}"
            assert_dispatch_snapshot_file "\${prefix}_refill_cycle_\${cycle}_issue_validate.txt" "\${expected_count}" queued 1 0 1 "\${dependency_active_count}"
            assert_retired_persisted_snapshot_file "\${prefix}_refill_cycle_\${cycle}_issue_validate.txt" "\$((expected_count - 1))" "\$((expected_count - 1))" complete 1 1 0 "\${dependency_active_count}"
            assert_manifest_snapshot_file "\${prefix}_refill_cycle_\${cycle}_issue_validate.txt" dispatched "\${expected_count}" queued 1 "\${dependency_active_count}"
            assert_descriptor_snapshot_file "\${prefix}_refill_cycle_\${cycle}_issue_validate.txt" dispatched "\${expected_count}" queued 1 0 1 "\${dependency_active_count}"
            assert_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_issue_validate.txt" dispatched "\${expected_count}" dispatch queued 1 0 1 "\${cycle}" "\${dependency_active_count}"
            assert_issue_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_issue_validate.txt" dispatched "\${expected_count}" 0 none 0 0 0 0 0
        fi
        submit_transition "\${submit_id}" running 0 "transition_\${submit_id}_refill_\${cycle}_running.txt"
        submit_focus_dump "\${submit_id}" "\${prefix}_REFILL_CYCLE_\${cycle}_RUNNING" "\${prefix}_refill_cycle_\${cycle}_running.txt"
        if [[ "\${use_ioctl_verification}" == "1" ]]; then
            assert_report_snapshot_file "\${prefix}_refill_cycle_\${cycle}_running.txt" dispatched "\${expected_count}" running 1 0 "\${dependency_active_count}"
            assert_dispatch_snapshot_file "\${prefix}_refill_cycle_\${cycle}_running.txt" "\${expected_count}" running 1 0 1 "\${dependency_active_count}"
            assert_retired_persisted_snapshot_file "\${prefix}_refill_cycle_\${cycle}_running.txt" "\$((expected_count - 1))" "\$((expected_count - 1))" complete 1 1 0 "\${dependency_active_count}"
            assert_manifest_snapshot_file "\${prefix}_refill_cycle_\${cycle}_running.txt" dispatched "\${expected_count}" running 1 "\${dependency_active_count}"
            assert_descriptor_snapshot_file "\${prefix}_refill_cycle_\${cycle}_running.txt" dispatched "\${expected_count}" running 1 0 1 "\${dependency_active_count}"
            assert_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_running.txt" dispatched "\${expected_count}" dispatch running 1 0 1 "\${cycle}" "\${dependency_active_count}"
            assert_issue_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_running.txt" dispatched "\${expected_count}" 0 none 0 0 0 0 0
        fi
        submit_validate "\${submit_id}"
        submit_focus_dump "\${submit_id}" "\${prefix}_REFILL_CYCLE_\${cycle}_RUNNING_VALIDATE" "\${prefix}_refill_cycle_\${cycle}_running_validate.txt"
        if [[ "\${use_ioctl_verification}" == "1" ]]; then
            assert_report_snapshot_file "\${prefix}_refill_cycle_\${cycle}_running_validate.txt" dispatched "\${expected_count}" running 1 0 "\${dependency_active_count}"
            assert_dispatch_snapshot_file "\${prefix}_refill_cycle_\${cycle}_running_validate.txt" "\${expected_count}" running 1 0 1 "\${dependency_active_count}"
            assert_retired_persisted_snapshot_file "\${prefix}_refill_cycle_\${cycle}_running_validate.txt" "\$((expected_count - 1))" "\$((expected_count - 1))" complete 1 1 0 "\${dependency_active_count}"
            assert_manifest_snapshot_file "\${prefix}_refill_cycle_\${cycle}_running_validate.txt" dispatched "\${expected_count}" running 1 "\${dependency_active_count}"
            assert_descriptor_snapshot_file "\${prefix}_refill_cycle_\${cycle}_running_validate.txt" dispatched "\${expected_count}" running 1 0 1 "\${dependency_active_count}"
            assert_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_running_validate.txt" dispatched "\${expected_count}" dispatch running 1 0 1 "\${cycle}" "\${dependency_active_count}"
            assert_issue_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_running_validate.txt" dispatched "\${expected_count}" 0 none 0 0 0 0 0
        fi
        submit_transition "\${submit_id}" complete 1 "transition_\${submit_id}_refill_\${cycle}_complete.txt"
        submit_focus_dump "\${submit_id}" "\${prefix}_REFILL_CYCLE_\${cycle}_COMPLETE" "\${prefix}_refill_cycle_\${cycle}_complete.txt"
        if [[ "\${use_ioctl_verification}" == "1" ]]; then
            assert_report_snapshot_file "\${prefix}_refill_cycle_\${cycle}_complete.txt" retired "\${expected_count}" complete 0 1 "\${dependency_active_count}"
            assert_dispatch_snapshot_file "\${prefix}_refill_cycle_\${cycle}_complete.txt" "\${expected_count}" complete 1 1 0 "\${dependency_active_count}"
            assert_retired_persisted_snapshot_file "\${prefix}_refill_cycle_\${cycle}_complete.txt" "\${expected_count}" "\${expected_count}" complete 1 1 0 "\${dependency_active_count}"
            assert_manifest_snapshot_file "\${prefix}_refill_cycle_\${cycle}_complete.txt" retired "\${expected_count}" complete 0 "\${dependency_active_count}"
            assert_descriptor_snapshot_file "\${prefix}_refill_cycle_\${cycle}_complete.txt" retired "\${expected_count}" complete 1 1 0 "\${dependency_active_count}"
            assert_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_complete.txt" retired "\${expected_count}" retired complete 1 1 0 "\${cycle}" "\${dependency_active_count}"
            assert_issue_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_complete.txt" retired "\${expected_count}" 0 none 0 0 0 0 0
        fi
        submit_validate "\${submit_id}"
        submit_focus_dump "\${submit_id}" "\${prefix}_REFILL_CYCLE_\${cycle}_COMPLETE_VALIDATE" "\${prefix}_refill_cycle_\${cycle}_complete_validate.txt"
        if [[ "\${use_ioctl_verification}" == "1" ]]; then
            assert_report_snapshot_file "\${prefix}_refill_cycle_\${cycle}_complete_validate.txt" retired "\${expected_count}" complete 0 1 "\${dependency_active_count}"
            assert_dispatch_snapshot_file "\${prefix}_refill_cycle_\${cycle}_complete_validate.txt" "\${expected_count}" complete 1 1 0 "\${dependency_active_count}"
            assert_retired_persisted_snapshot_file "\${prefix}_refill_cycle_\${cycle}_complete_validate.txt" "\${expected_count}" "\${expected_count}" complete 1 1 0 "\${dependency_active_count}"
            assert_manifest_snapshot_file "\${prefix}_refill_cycle_\${cycle}_complete_validate.txt" retired "\${expected_count}" complete 0 "\${dependency_active_count}"
            assert_descriptor_snapshot_file "\${prefix}_refill_cycle_\${cycle}_complete_validate.txt" retired "\${expected_count}" complete 1 1 0 "\${dependency_active_count}"
            assert_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_complete_validate.txt" retired "\${expected_count}" retired complete 1 1 0 "\${cycle}" "\${dependency_active_count}"
            assert_issue_plan_snapshot_file "\${prefix}_refill_cycle_\${cycle}_complete_validate.txt" retired "\${expected_count}" 0 none 0 0 0 0 0
        fi
    done
}

drive_submit() {
    local submit_id="\$1"
    local mode="\$2"
    local refill_cycles="\$3"
    local prefix="\$4"
    local contend_submit_id="\${5:-}"
    local contend_prefix="\${6:-}"
    local dependency_active_count="\${7:-0}"

    drive_submit_initial_cycle "\${submit_id}" "\${prefix}" "\${contend_submit_id}" "\${contend_prefix}" "\${dependency_active_count}"
    drive_submit_refill_cycles "\${submit_id}" "\${refill_cycles}" "\${prefix}" "\${dependency_active_count}"
}

if "${target_op_bin}" "${target_node}" cleanup >/dev/null 2>&1; then
    echo "STALE_SUBMIT_CLEANUP=ioctl"
else
    if [[ "\${use_ioctl_verification}" == "1" ]]; then
        echo "Typed submit cleanup unavailable under USE_IOCTL_VERIFICATION=1" >&2
        exit 1
    fi
    echo "STALE_SUBMIT_CLEANUP=sysfs"
    while read -r stale_submit_id; do
        [[ -n "\${stale_submit_id}" ]] || continue
        echo "drop \${stale_submit_id}" > "\${submit_node}" 2>/dev/null || true
    done < <(awk '
/^submit id=/ {
    for (i = 1; i <= NF; i++) {
        if (\$i ~ /^id=/) {
            split(\$i, a, "=")
            print a[2]
        }
    }
}
' "\${submits_node}")

    for stale_submit_id in \$(seq 1 "\${stale_submit_scan_max}"); do
        echo "drop \${stale_submit_id}" > "\${submit_node}" 2>/dev/null || true
    done
fi

if "${target_obj_bin}" "${target_node}" cleanup >/dev/null 2>&1; then
    echo "STALE_OBJECT_CLEANUP=ioctl"
else
    if [[ "\${use_ioctl_verification}" == "1" ]]; then
        echo "Typed object cleanup unavailable under USE_IOCTL_VERIFICATION=1" >&2
        exit 1
    fi
    echo "STALE_OBJECT_CLEANUP=sysfs"
    while read -r stale_object_id; do
        [[ -n "\${stale_object_id}" ]] || continue
        echo "drop \${stale_object_id}" > "\${object_node}" 2>/dev/null || true
    done < <(awk '
/^object id=/ {
    for (i = 1; i <= NF; i++) {
        if (\$i ~ /^id=/) {
            split(\$i, a, "=")
            print a[2]
        }
    }
}
' "\${objects_node}")

    for stale_object_id in \$(seq 1 "\${stale_object_scan_max}"); do
        echo "drop \${stale_object_id}" > "\${object_node}" 2>/dev/null || true
    done
fi

capture_gtt_snapshot "before_gtt.txt"
capture_objects_snapshot "before_objects.txt"
capture_submits_snapshot "before_submits.txt"
before_range_ids=()
while read -r before_range_id; do
    [[ -n "\${before_range_id}" ]] || continue
    before_range_ids+=("\${before_range_id}")
done < <(range_query_list_gem_ids before)
max_before_range_id=\$(awk '
/^range id=/ {
    for (i = 1; i <= NF; i++) {
        if (\$i ~ /^id=/) {
            split(\$i, a, "=")
            if (a[2] + 0 > max_id)
                max_id = a[2] + 0
        }
    }
}
END {
    print max_id + 0
}
' "\${tmpdir}/before_gtt.txt")

for idx in \$(seq 1 "${probe_count}"); do
    "${target_bin}" "${target_node}" "${hold_seconds}" > "\${tmpdir}/drm_probe_\${idx}.log" 2>&1 &
    probe_pids+=("\$!")
done

for _ in \$(seq 1 \$((hold_seconds + 5))); do
    ready_count=\$(grep -h '^drm_hold_seconds=' "\${tmpdir}"/drm_probe_*.log 2>/dev/null | wc -l || true)
    if [[ "\${ready_count}" -ge "${probe_count}" ]]; then
        break
    fi
    sleep 1
done

capture_gtt_snapshot "during_gtt.txt"
range_ids=()
object_ids=()
candidate_range_ids=""
range_query_during_available=0
while read -r candidate_range_id; do
    [[ -n "\${candidate_range_id}" ]] || continue
    range_query_during_available=1
    local_seen=0
    for before_range_id in "\${before_range_ids[@]:-}"; do
        if [[ "\${before_range_id}" == "\${candidate_range_id}" ]]; then
            local_seen=1
            break
        fi
    done
    if [[ "\${local_seen}" == "0" ]]; then
        candidate_range_ids+="\${candidate_range_id}"$'\n'
    fi
done < <(range_query_list_gem_ids during)

if [[ "\${range_query_during_available}" != "1" ]]; then
candidate_range_ids=\$(awk -v min_id="\${max_before_range_id}" '
/^range id=/ {
    id = -1
    for (i = 1; i <= NF; i++) {
        if (\$i ~ /^id=/) {
            split(\$i, a, "=")
            id = a[2] + 0
        }
    }
    if (id > min_id)
        print id
}
' "\${tmpdir}/during_gtt.txt")
fi

for candidate_range_id in \${candidate_range_ids}; do
    if [[ "\${use_ioctl_object_setup}" == "1" ]]; then
        if adopted_object_id="\$(object_adopt "\${candidate_range_id}" "object_adopt_range_\${candidate_range_id}" 2>/dev/null)"; then
            range_ids+=("\${candidate_range_id}")
            object_ids+=("\${adopted_object_id}")
            if [[ "\${#range_ids[@]}" -ge "${probe_count}" ]]; then
                break
            fi
        fi
    else
        if echo "adopt \${candidate_range_id}" > "\${object_node}" 2>/dev/null; then
            range_ids+=("\${candidate_range_id}")
            if [[ "\${#range_ids[@]}" -ge "${probe_count}" ]]; then
                break
            fi
        fi
    fi
done

if [[ "\${range_query_during_available}" != "1" && "\${#range_ids[@]}" -lt "${probe_count}" ]]; then
    for candidate_range_id in \$(seq \$((max_before_range_id + 1)) \$((max_before_range_id + ${range_scan_max}))); do
        already_tried=0
        for existing_range_id in "\${range_ids[@]:-}"; do
            if [[ "\${existing_range_id}" == "\${candidate_range_id}" ]]; then
                already_tried=1
                break
            fi
        done
        if [[ "\${already_tried}" == "1" ]]; then
            continue
        fi
        if [[ "\${use_ioctl_object_setup}" == "1" ]]; then
            if adopted_object_id="\$(object_adopt "\${candidate_range_id}" "object_adopt_range_\${candidate_range_id}" 2>/dev/null)"; then
                range_ids+=("\${candidate_range_id}")
                object_ids+=("\${adopted_object_id}")
                if [[ "\${#range_ids[@]}" -ge "${probe_count}" ]]; then
                    break
                fi
            fi
        else
            if echo "adopt \${candidate_range_id}" > "\${object_node}" 2>/dev/null; then
                range_ids+=("\${candidate_range_id}")
                if [[ "\${#range_ids[@]}" -ge "${probe_count}" ]]; then
                    break
                fi
            fi
        fi
    done
fi

if [[ "\${#range_ids[@]}" -lt "${probe_count}" ]]; then
    echo "Expected ${probe_count} fresh SGX-adoptable GEM ranges, found \${#range_ids[@]}" >&2
    echo "MAX_BEFORE_RANGE_ID=\${max_before_range_id}" >&2
    echo "RANGE_SCAN_MAX=${range_scan_max}" >&2
    cat "\${tmpdir}/during_gtt.txt" >&2
    exit 1
fi

if [[ "\${use_ioctl_object_setup}" == "1" ]]; then
    capture_objects_snapshot "after_adopt_objects.txt"
else
    cat "\${objects_node}" > "\${tmpdir}/after_adopt_objects.txt"
    first_adopted_object_id=\$(awk '
/^object id=/ {
    for (i = 1; i <= NF; i++) {
        if (\$i ~ /^id=/) {
            split(\$i, a, "=")
            print a[2]
            exit
        }
    }
}
' "\${tmpdir}/after_adopt_objects.txt")

    if [[ -z "\${first_adopted_object_id}" ]]; then
        echo "Failed to identify the first adopted object ID" >&2
        cat "\${tmpdir}/after_adopt_objects.txt" >&2
        exit 1
    fi

    for idx in \$(seq 0 \$(( ${probe_count} - 1 ))); do
        object_ids+=("\$((first_adopted_object_id + idx))")
    done
fi

if [[ "\${#object_ids[@]}" -lt "${probe_count}" ]]; then
    echo "Failed to map adopted object IDs" >&2
    cat "\${tmpdir}/after_adopt_objects.txt" >&2 || true
    exit 1
fi

object_a_control="\${object_ids[0]}"
object_a_pds="\${object_ids[1]}"
object_a_use="\${object_ids[2]}"
object_a_target="\${object_ids[3]}"
object_a_status="\${object_ids[4]}"
object_b_control="\${object_ids[5]}"
object_b_pds="\${object_ids[6]}"
object_b_use="\${object_ids[7]}"
object_b_target="\${object_ids[8]}"
object_b_status="\${object_ids[9]}"
object_c_control=""
object_c_pds=""
object_c_use=""
object_c_target=""
object_c_status=""
object_d_control=""
object_d_pds=""
object_d_use=""
object_d_target=""
object_d_status=""
object_e_control=""
object_e_pds=""
object_e_use=""
object_e_target=""
object_e_status=""

if [[ "${run_unrelated_after_satisfied_drop}" == "1" || "${run_secondary_dependency_source}" == "1" ]]; then
    object_c_control="\${object_ids[10]}"
    object_c_pds="\${object_ids[11]}"
    object_c_use="\${object_ids[12]}"
    object_c_target="\${object_ids[13]}"
    object_c_status="\${object_ids[14]}"
fi

if [[ "${run_tertiary_dependency_source}" == "1" ]]; then
    object_d_control="\${object_ids[15]}"
    object_d_pds="\${object_ids[16]}"
    object_d_use="\${object_ids[17]}"
    object_d_target="\${object_ids[18]}"
    object_d_status="\${object_ids[19]}"
fi

if [[ "${run_quaternary_dependency_source}" == "1" ]]; then
    object_e_control="\${object_ids[20]}"
    object_e_pds="\${object_ids[21]}"
    object_e_use="\${object_ids[22]}"
    object_e_target="\${object_ids[23]}"
    object_e_status="\${object_ids[24]}"
fi

for object_id in "\${object_a_control}" "\${object_a_pds}" "\${object_a_use}" "\${object_a_target}" "\${object_a_status}" "\${object_b_control}" "\${object_b_pds}" "\${object_b_use}" "\${object_b_target}" "\${object_b_status}" "\${object_c_control}" "\${object_c_pds}" "\${object_c_use}" "\${object_c_target}" "\${object_c_status}" "\${object_d_control}" "\${object_d_pds}" "\${object_d_use}" "\${object_d_target}" "\${object_d_status}" "\${object_e_control}" "\${object_e_pds}" "\${object_e_use}" "\${object_e_target}" "\${object_e_status}"; do
    [[ -n "\${object_id}" ]] || continue
    if [[ -z "\${object_id}" || "\${object_id}" == "0" ]]; then
        echo "Failed to map adopted object IDs" >&2
        cat "\${tmpdir}/after_adopt_objects.txt" >&2
        exit 1
    fi
done

setup_object "\${object_a_control}" control packet control_ccb
setup_object "\${object_a_pds}" pds code pds_code
setup_object "\${object_a_use}" use code use_code
setup_object "\${object_a_target}" target surface render_target
setup_object "\${object_a_status}" status state status_block
setup_object "\${object_b_control}" control packet control_ccb
setup_object "\${object_b_pds}" pds code pds_code
setup_object "\${object_b_use}" use code use_code
setup_object "\${object_b_target}" target surface render_target
setup_object "\${object_b_status}" status state status_block
if [[ "${run_unrelated_after_satisfied_drop}" == "1" || "${run_secondary_dependency_source}" == "1" ]]; then
    setup_object "\${object_c_control}" control packet control_ccb
    setup_object "\${object_c_pds}" pds code pds_code
    setup_object "\${object_c_use}" use code use_code
    setup_object "\${object_c_target}" target surface render_target
    setup_object "\${object_c_status}" status state status_block
fi
if [[ "${run_tertiary_dependency_source}" == "1" ]]; then
    setup_object "\${object_d_control}" control packet control_ccb
    setup_object "\${object_d_pds}" pds code pds_code
    setup_object "\${object_d_use}" use code use_code
    setup_object "\${object_d_target}" target surface render_target
    setup_object "\${object_d_status}" status state status_block
fi
if [[ "${run_quaternary_dependency_source}" == "1" ]]; then
    setup_object "\${object_e_control}" control packet control_ccb
    setup_object "\${object_e_pds}" pds code pds_code
    setup_object "\${object_e_use}" use code use_code
    setup_object "\${object_e_target}" target surface render_target
    setup_object "\${object_e_status}" status state status_block
fi

capture_objects_snapshot "after_role_bind_objects.txt"
capture_gtt_snapshot "after_role_bind_gtt.txt"
capture_coord_snapshot_file "coord_state_before_submit_setup.txt"
coord_complete_ticket_baseline="\$(extract_value_from_file "coord_state_before_submit_setup.txt" "coord_complete_ticket")"
if [[ -z "\${coord_complete_ticket_baseline}" ]]; then
    coord_complete_ticket_baseline="\$(extract_value_from_file "coord_state_before_submit_setup.txt" "complete_ticket_counter")"
fi
if [[ -z "\${coord_complete_ticket_baseline}" || ! "\${coord_complete_ticket_baseline}" =~ ^[0-9]+$ ]]; then
    echo "Failed to resolve coord_complete_ticket baseline before submit setup" >&2
    cat "\${tmpdir}/coord_state_before_submit_setup.txt" >&2
    exit 1
fi
submit_b_dependency_complete_ticket_dec="\$(resolve_dependency_ticket_value "\${submit_b_dependency_complete_ticket_dec}" "\${submit_b_dependency_complete_ticket_relative}" "\${coord_complete_ticket_baseline}")"
submit_b_dependency2_complete_ticket_dec="\$(resolve_dependency_ticket_value "\${submit_b_dependency2_complete_ticket_dec}" "\${submit_b_dependency2_complete_ticket_relative}" "\${coord_complete_ticket_baseline}")"
submit_b_dependency3_complete_ticket_dec="\$(resolve_dependency_ticket_value "\${submit_b_dependency3_complete_ticket_dec}" "\${submit_b_dependency3_complete_ticket_relative}" "\${coord_complete_ticket_baseline}")"
submit_b_dependency4_complete_ticket_dec="\$(resolve_dependency_ticket_value "\${submit_b_dependency4_complete_ticket_dec}" "\${submit_b_dependency4_complete_ticket_relative}" "\${coord_complete_ticket_baseline}")"

if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
    submit_a_id="\$(submit_create pf submit_a_create)"
    capture_submits_snapshot "after_submit_a_create_submits.txt"
else
    echo "create pf" > "\${submit_node}"
    cat "\${submits_node}" > "\${tmpdir}/after_submit_a_create_submits.txt"
    submit_a_id=\$(awk '
/^submit id=/ {
    submit_id = ""
    for (i = 1; i <= NF; i++) {
        if (\$i ~ /^id=/) {
            split(\$i, a, "=")
            submit_id = a[2]
        }
    }
    if (submit_id != "")
        last = submit_id
}
END {
    if (last != "")
        print last
}
' "\${tmpdir}/after_submit_a_create_submits.txt")
fi

if [[ -z "\${submit_a_id}" ]]; then
    echo "Failed to identify submit A ID" >&2
    cat "\${tmpdir}/after_submit_a_create_submits.txt" "\${tmpdir}/submit_a_create.txt" 2>/dev/null >&2 || true
    exit 1
fi

submit_b_id="\$(submit_create pf submit_b_create)"
submit_focus "\${submit_b_id}"
if [[ "\${use_ioctl_verification}" == "1" ]]; then
    submit_query_capture "after_submit_b_focus.txt" focus
else
    cat "\${submit_state_node}" > "\${tmpdir}/after_submit_b_focus.txt"
fi
submit_c_id=""
submit_d_id=""
submit_e_id=""
if [[ "${run_unrelated_after_satisfied_drop}" == "1" || "${run_secondary_dependency_source}" == "1" ]]; then
    submit_c_id="\$(submit_create pf submit_c_create)"
fi
if [[ "${run_tertiary_dependency_source}" == "1" ]]; then
    submit_d_id="\$(submit_create pf submit_d_create)"
fi
if [[ "${run_quaternary_dependency_source}" == "1" ]]; then
    submit_e_id="\$(submit_create pf submit_e_create)"
fi

configure_submit "\${submit_a_id}" "${submit_a_queue_mode}" "\${object_a_control}" "\${object_a_pds}" "\${object_a_use}" "\${object_a_target}" "\${object_a_status}" 0
configure_submit "\${submit_b_id}" "${submit_b_queue_mode}" "\${object_b_control}" "\${object_b_pds}" "\${object_b_use}" "\${object_b_target}" "\${object_b_status}" "${submit_b_dependency_complete_ticket_dec}"
submit_b_expected_dependency_active_count=0
if [[ "${run_unrelated_after_satisfied_drop}" == "1" || "${run_secondary_dependency_source}" == "1" ]]; then
    configure_submit "\${submit_c_id}" "${submit_c_queue_mode}" "\${object_c_control}" "\${object_c_pds}" "\${object_c_use}" "\${object_c_target}" "\${object_c_status}" 0
fi
if [[ "${run_tertiary_dependency_source}" == "1" ]]; then
    configure_submit "\${submit_d_id}" "${submit_d_queue_mode}" "\${object_d_control}" "\${object_d_pds}" "\${object_d_use}" "\${object_d_target}" "\${object_d_status}" 0
fi
if [[ "${run_quaternary_dependency_source}" == "1" ]]; then
    configure_submit "\${submit_e_id}" "${submit_e_queue_mode}" "\${object_e_control}" "\${object_e_pds}" "\${object_e_use}" "\${object_e_target}" "\${object_e_status}" 0
fi
if [[ "${submit_b_dependency_complete_ticket_dec}" -gt 0 ]]; then
    configure_dependency_slot "\${submit_b_id}" 1 "\${submit_a_id}"
    submit_b_expected_dependency_active_count=\$((submit_b_expected_dependency_active_count + 1))
    if [[ "\${use_indexed_dependencies}" == "1" ]]; then
        configure_dependency_ticket_slot "\${submit_b_id}" 1 "\${submit_b_dependency_complete_ticket_dec}"
        validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_DEPTICKET1_SETUP" "submit_b_after_depticket1_setup.txt"
        refresh_dependency_state_slot "\${submit_b_id}" 1 "\${submit_b_dependency_offset}" "SUBMIT_B_AFTER_DEPSTATE1_SETUP" "submit_b_after_depstate1_setup.txt"
    else
        refresh_dependency_state "\${submit_b_id}" "\${submit_b_dependency_offset}" "SUBMIT_B_AFTER_DEPSTATE_SETUP" "submit_b_after_depstate_setup.txt"
    fi
    if [[ "${run_secondary_dependency_source}" == "1" ]]; then
        configure_dependency_slot "\${submit_b_id}" 2 "\${submit_c_id}"
        submit_b_expected_dependency_active_count=\$((submit_b_expected_dependency_active_count + 1))
        if [[ "\${submit_b_dependency2_complete_ticket_dec}" -gt 0 ]]; then
            configure_dependency_ticket_slot "\${submit_b_id}" 2 "\${submit_b_dependency2_complete_ticket_dec}"
            validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_DEPTICKET2_SETUP" "submit_b_after_depticket2_setup.txt"
        fi
        refresh_dependency_state_slot "\${submit_b_id}" 2 "\${submit_b_dependency2_offset}" "SUBMIT_B_AFTER_DEPSTATE2_SETUP" "submit_b_after_depstate2_setup.txt"
    fi
    if [[ "${run_tertiary_dependency_source}" == "1" ]]; then
        configure_dependency_slot "\${submit_b_id}" 3 "\${submit_d_id}"
        submit_b_expected_dependency_active_count=\$((submit_b_expected_dependency_active_count + 1))
        if [[ "\${submit_b_dependency3_complete_ticket_dec}" -gt 0 ]]; then
            configure_dependency_ticket_slot "\${submit_b_id}" 3 "\${submit_b_dependency3_complete_ticket_dec}"
            validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_DEPTICKET3_SETUP" "submit_b_after_depticket3_setup.txt"
        fi
        refresh_dependency_state_slot "\${submit_b_id}" 3 "\${submit_b_dependency3_offset}" "SUBMIT_B_AFTER_DEPSTATE3_SETUP" "submit_b_after_depstate3_setup.txt"
    fi
    if [[ "${run_quaternary_dependency_source}" == "1" ]]; then
        configure_dependency_slot "\${submit_b_id}" 4 "\${submit_e_id}"
        submit_b_expected_dependency_active_count=\$((submit_b_expected_dependency_active_count + 1))
        if [[ "\${submit_b_dependency4_complete_ticket_dec}" -gt 0 ]]; then
            configure_dependency_ticket_slot "\${submit_b_id}" 4 "\${submit_b_dependency4_complete_ticket_dec}"
            validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_DEPTICKET4_SETUP" "submit_b_after_depticket4_setup.txt"
        fi
        refresh_dependency_state_slot "\${submit_b_id}" 4 "\${submit_b_dependency4_offset}" "SUBMIT_B_AFTER_DEPSTATE4_SETUP" "submit_b_after_depstate4_setup.txt"
    fi
fi

submit_focus_dump "\${submit_a_id}" "SUBMIT_A_AFTER_SETUP" "submit_a_after_setup.txt"
submit_focus_dump "\${submit_b_id}" "SUBMIT_B_AFTER_SETUP" "submit_b_after_setup.txt"
if [[ "${run_unrelated_after_satisfied_drop}" == "1" || "${run_secondary_dependency_source}" == "1" ]]; then
    submit_focus_dump "\${submit_c_id}" "SUBMIT_C_AFTER_SETUP" "submit_c_after_setup.txt"
fi
if [[ "${run_tertiary_dependency_source}" == "1" ]]; then
    submit_focus_dump "\${submit_d_id}" "SUBMIT_D_AFTER_SETUP" "submit_d_after_setup.txt"
fi
if [[ "${run_quaternary_dependency_source}" == "1" ]]; then
    submit_focus_dump "\${submit_e_id}" "SUBMIT_E_AFTER_SETUP" "submit_e_after_setup.txt"
fi
submit_coord_dump "COORD_STATE_AFTER_SETUP" "coord_state_after_setup.txt"
if [[ "\${use_ioctl_verification}" == "1" ]]; then
    expected_submit_count=2
    if [[ "${run_unrelated_after_satisfied_drop}" == "1" || "${run_secondary_dependency_source}" == "1" ]]; then
        expected_submit_count=\$((expected_submit_count + 1))
    fi
    if [[ "${run_tertiary_dependency_source}" == "1" ]]; then
        expected_submit_count=\$((expected_submit_count + 1))
    fi
    if [[ "${run_quaternary_dependency_source}" == "1" ]]; then
        expected_submit_count=\$((expected_submit_count + 1))
    fi
    grep -q '^gtt_state_range_sgx_object_count='"${probe_count}"'\$' "\${tmpdir}/after_role_bind_gtt.txt"
    grep -q '^gtt_state_range_pf_bound_count='"${probe_count}"'\$' "\${tmpdir}/after_role_bind_gtt.txt"
    grep -q '^coord_live_submit_count='"\${expected_submit_count}"'\$' "\${tmpdir}/coord_state_after_setup.txt"
fi
if [[ "${submit_b_dependency_complete_ticket_dec}" -gt 0 ]]; then
    expect_dependency_wait "\${submit_b_id}" "SUBMIT_B_DEPENDENCY_WAIT_BEFORE_A" "submit_b_dependency_wait_before_a.txt"
    validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_AUTO_DEPSTATE_BEFORE_RELEASE" "submit_b_after_auto_depstate_before_release.txt"
    submit_focus_dump "\${submit_b_id}" "SUBMIT_B_STILL_IDLE_BEFORE_DEPENDENCY_RELEASE" "submit_b_still_idle_before_dependency_release.txt"
    submit_coord_dump "COORD_STATE_AFTER_DEPENDENCY_WAIT" "coord_state_after_dependency_wait.txt"
fi

if [[ "${drop_source_before_run}" == "1" ]]; then
    submit_drop "\${submit_a_id}"
    validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_SOURCE_DROP" "submit_b_after_source_drop.txt"
    expect_dependency_wait "\${submit_b_id}" "SUBMIT_B_DEPENDENCY_WAIT_AFTER_SOURCE_DROP" "submit_b_dependency_wait_after_source_drop.txt"
    validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_SOURCE_DROP_WAIT" "submit_b_after_source_drop_wait.txt"
    submit_coord_dump "COORD_STATE_AFTER_SOURCE_DROP" "coord_state_after_source_drop.txt"
else
    drive_submit "\${submit_a_id}" "${submit_a_queue_mode}" "${submit_a_refill_cycles}" "SUBMIT_A" "\${submit_b_id}" "SUBMIT_B" 0
    if [[ "${submit_b_dependency_complete_ticket_dec}" -gt 0 ]]; then
        validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_A" "submit_b_after_auto_depstate_after_a.txt"
    fi
    if [[ "${run_secondary_dependency_source}" == "1" ]]; then
        expect_dependency_wait "\${submit_b_id}" "SUBMIT_B_DEPENDENCY_WAIT_AFTER_A" "submit_b_dependency_wait_after_a.txt"
        validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_DEPENDENCY_WAIT_AFTER_A" "submit_b_after_dependency_wait_after_a.txt"
        submit_focus_dump "\${submit_b_id}" "SUBMIT_B_STILL_IDLE_AFTER_A" "submit_b_still_idle_after_a.txt"
        submit_coord_dump "COORD_STATE_AFTER_SUBMIT_A" "coord_state_after_submit_a.txt"
        drive_submit_initial_cycle "\${submit_c_id}" "SUBMIT_C" "" "" 0
        if [[ "\${submit_b_dependency2_complete_ticket_dec}" -gt 0 && "${submit_c_refill_cycles_dec}" -gt 0 ]]; then
            validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_C_FIRST_COMPLETE" "submit_b_after_auto_depstate_after_c_first_complete.txt"
            expect_dependency_wait "\${submit_b_id}" "SUBMIT_B_DEPENDENCY_WAIT_AFTER_C_FIRST_COMPLETE" "submit_b_dependency_wait_after_c_first_complete.txt"
            validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_DEPENDENCY_WAIT_AFTER_C_FIRST_COMPLETE" "submit_b_after_dependency_wait_after_c_first_complete.txt"
        fi
        drive_submit_refill_cycles "\${submit_c_id}" "${submit_c_refill_cycles}" "SUBMIT_C" 0
        validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_C" "submit_b_after_auto_depstate_after_c.txt"
        submit_coord_dump "COORD_STATE_AFTER_SUBMIT_C" "coord_state_after_submit_c.txt"
        if [[ "${run_tertiary_dependency_source}" == "1" ]]; then
            expect_dependency_wait "\${submit_b_id}" "SUBMIT_B_DEPENDENCY_WAIT_AFTER_C" "submit_b_dependency_wait_after_c.txt"
            validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_DEPENDENCY_WAIT_AFTER_C" "submit_b_after_dependency_wait_after_c.txt"
            submit_focus_dump "\${submit_b_id}" "SUBMIT_B_STILL_IDLE_AFTER_C" "submit_b_still_idle_after_c.txt"
            drive_submit "\${submit_d_id}" "${submit_d_queue_mode}" "${submit_d_refill_cycles}" "SUBMIT_D" "" "" 0
            validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_D" "submit_b_after_auto_depstate_after_d.txt"
            submit_coord_dump "COORD_STATE_AFTER_SUBMIT_D" "coord_state_after_submit_d.txt"
            if [[ "${run_quaternary_dependency_source}" == "1" ]]; then
                expect_dependency_wait "\${submit_b_id}" "SUBMIT_B_DEPENDENCY_WAIT_AFTER_D" "submit_b_dependency_wait_after_d.txt"
                validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_DEPENDENCY_WAIT_AFTER_D" "submit_b_after_dependency_wait_after_d.txt"
                submit_focus_dump "\${submit_b_id}" "SUBMIT_B_STILL_IDLE_AFTER_D" "submit_b_still_idle_after_d.txt"
                if [[ "${clear_dependency4_after_d}" == "1" ]]; then
                    clear_dependency_slot "\${submit_b_id}" 4
                    submit_b_expected_dependency_active_count=\$((submit_b_expected_dependency_active_count - 1))
                    validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_DEPCLEAR4" "submit_b_after_depclear4.txt"
                    submit_focus_dump "\${submit_b_id}" "SUBMIT_B_STILL_IDLE_AFTER_DEPCLEAR4" "submit_b_still_idle_after_depclear4.txt"
                    submit_coord_dump "COORD_STATE_AFTER_DEPCLEAR4" "coord_state_after_depclear4.txt"
                else
                    drive_submit "\${submit_e_id}" "${submit_e_queue_mode}" "${submit_e_refill_cycles}" "SUBMIT_E" "" "" 0
                    validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_E" "submit_b_after_auto_depstate_after_e.txt"
                    submit_coord_dump "COORD_STATE_AFTER_SUBMIT_E" "coord_state_after_submit_e.txt"
                fi
            fi
        fi
    elif [[ "${drop_source_after_release}" == "1" ]]; then
        submit_drop "\${submit_a_id}"
        validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_SATISFIED_SOURCE_DROP" "submit_b_after_satisfied_source_drop.txt"
        submit_focus_dump "\${submit_b_id}" "SUBMIT_B_STILL_IDLE_AFTER_SATISFIED_SOURCE_DROP" "submit_b_still_idle_after_satisfied_source_drop.txt"
        submit_coord_dump "COORD_STATE_AFTER_SATISFIED_SOURCE_DROP" "coord_state_after_satisfied_source_drop.txt"
        if [[ "${run_unrelated_after_satisfied_drop}" == "1" ]]; then
            drive_submit "\${submit_c_id}" "${submit_c_queue_mode}" "${submit_c_refill_cycles}" "SUBMIT_C" "" "" 0
            validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_UNRELATED_SUBMIT_C" "submit_b_after_unrelated_submit_c.txt"
            submit_coord_dump "COORD_STATE_AFTER_SUBMIT_C" "coord_state_after_submit_c.txt"
        fi
    else
        submit_focus_dump "\${submit_b_id}" "SUBMIT_B_STILL_IDLE_AFTER_A" "submit_b_still_idle_after_a.txt"
        submit_coord_dump "COORD_STATE_AFTER_SUBMIT_A" "coord_state_after_submit_a.txt"
    fi

    drive_submit "\${submit_b_id}" "${submit_b_queue_mode}" "${submit_b_refill_cycles}" "SUBMIT_B" "" "" "\${submit_b_expected_dependency_active_count}"
    if [[ "${drop_source_after_release}" != "1" ]]; then
        submit_focus_dump "\${submit_a_id}" "SUBMIT_A_STILL_FINAL_AFTER_B" "submit_a_still_final_after_b.txt"
    fi
    submit_coord_dump "COORD_STATE_AFTER_SUBMIT_B" "coord_state_after_submit_b.txt"
    if [[ "${clear_dependency4_after_d}" == "1" && "${run_submit_e_after_clear}" == "1" ]]; then
        drive_submit "\${submit_e_id}" "${submit_e_queue_mode}" "${submit_e_refill_cycles}" "SUBMIT_E" "" "" 0
        validate_submit_state "\${submit_b_id}" "SUBMIT_B_AFTER_SUBMIT_E_POST_CLEAR" "submit_b_after_submit_e_post_clear.txt"
        submit_coord_dump "COORD_STATE_AFTER_SUBMIT_E_POST_CLEAR" "coord_state_after_submit_e_post_clear.txt"
    fi
fi

for pid in "\${probe_pids[@]}"; do
    wait "\${pid}"
done
probe_pids=()

capture_objects_snapshot "after_probe_exit_objects.txt"
capture_gtt_snapshot "after_probe_exit_gtt.txt"
if [[ "${drop_source_before_run}" != "1" && "${drop_source_after_release}" != "1" ]]; then
        submit_validate "\${submit_a_id}"
    submit_focus_dump "\${submit_a_id}" "SUBMIT_A_AFTER_PROBE_EXIT_VALIDATE" "submit_a_after_probe_exit_validate.txt"
fi
submit_validate "\${submit_b_id}"
submit_focus_dump "\${submit_b_id}" "SUBMIT_B_AFTER_PROBE_EXIT_VALIDATE" "submit_b_after_probe_exit_validate.txt"
if [[ "${run_unrelated_after_satisfied_drop}" == "1" || "${run_secondary_dependency_source}" == "1" ]]; then
    submit_validate "\${submit_c_id}"
    submit_focus_dump "\${submit_c_id}" "SUBMIT_C_AFTER_PROBE_EXIT_VALIDATE" "submit_c_after_probe_exit_validate.txt"
fi
if [[ "${run_tertiary_dependency_source}" == "1" ]]; then
    submit_validate "\${submit_d_id}"
    submit_focus_dump "\${submit_d_id}" "SUBMIT_D_AFTER_PROBE_EXIT_VALIDATE" "submit_d_after_probe_exit_validate.txt"
fi
if [[ "${run_quaternary_dependency_source}" == "1" ]]; then
    submit_validate "\${submit_e_id}"
    submit_focus_dump "\${submit_e_id}" "SUBMIT_E_AFTER_PROBE_EXIT_VALIDATE" "submit_e_after_probe_exit_validate.txt"
fi

if [[ "${drop_source_before_run}" != "1" && "${drop_source_after_release}" != "1" ]]; then
    submit_drop "\${submit_a_id}"
fi
submit_drop "\${submit_b_id}"
if [[ "${run_unrelated_after_satisfied_drop}" == "1" || "${run_secondary_dependency_source}" == "1" ]]; then
    submit_drop "\${submit_c_id}"
fi
if [[ "${run_tertiary_dependency_source}" == "1" ]]; then
    submit_drop "\${submit_d_id}"
fi
if [[ "${run_quaternary_dependency_source}" == "1" ]]; then
    submit_drop "\${submit_e_id}"
fi
for object_id in "\${object_a_control}" "\${object_a_pds}" "\${object_a_use}" "\${object_a_target}" "\${object_a_status}" "\${object_b_control}" "\${object_b_pds}" "\${object_b_use}" "\${object_b_target}" "\${object_b_status}" "\${object_c_control}" "\${object_c_pds}" "\${object_c_use}" "\${object_c_target}" "\${object_c_status}" "\${object_d_control}" "\${object_d_pds}" "\${object_d_use}" "\${object_d_target}" "\${object_d_status}" "\${object_e_control}" "\${object_e_pds}" "\${object_e_use}" "\${object_e_target}" "\${object_e_status}"; do
    [[ -n "\${object_id}" ]] || continue
    object_drop "\${object_id}"
done
capture_submits_snapshot "after_drop_submits.txt"
capture_objects_snapshot "after_drop_objects.txt"
capture_gtt_snapshot "after_drop_gtt.txt"
if [[ "\${use_ioctl_verification}" == "1" ]]; then
    submit_coord_capture "after_drop_coord.txt"
else
    cat "\${coord_node}" > "\${tmpdir}/after_drop_coord.txt"
fi

if [[ "\${use_ioctl_verification}" == "1" ]]; then
    grep -q '^coord_live_submit_count=0\$' "\${tmpdir}/after_drop_coord.txt"
    grep -q '^gtt_state_range_sgx_object_count=0\$' "\${tmpdir}/after_drop_gtt.txt"
    grep -q '^gtt_state_range_pf_bound_count=0\$' "\${tmpdir}/after_drop_gtt.txt"
fi

echo "REMOTE_BIN=${target_bin}"
if [[ "\${use_ioctl_submit_setup}" == "1" ]]; then
echo "SUBMIT_SETUP=ioctl"
else
echo "SUBMIT_SETUP=sysfs"
fi
if [[ "\${use_ioctl_object_setup}" == "1" ]]; then
echo "OBJECT_SETUP=ioctl"
else
echo "OBJECT_SETUP=sysfs"
fi
if [[ "\${use_ioctl_range_discovery}" == "1" ]]; then
echo "RANGE_DISCOVERY=ioctl"
else
echo "RANGE_DISCOVERY=sysfs"
fi
if [[ "\${use_ioctl_dependency_setup}" == "1" ]]; then
echo "DEPENDENCY_SETUP=ioctl"
else
echo "DEPENDENCY_SETUP=sysfs"
fi
if [[ "\${use_ioctl_verification}" == "1" ]]; then
echo "VERIFICATION_MODE=ioctl"
else
echo "VERIFICATION_MODE=sysfs"
fi
if [[ "\${use_indexed_dependencies}" == "1" ]]; then
echo "DEPENDENCY_INTERFACE=indexed"
else
echo "DEPENDENCY_INTERFACE=named"
fi
echo "SUBMIT_STATE_NODE=\${submit_state_node}"
echo "SUBMIT_A_ID=\${submit_a_id}"
echo "SUBMIT_A_QUEUE_MODE=${submit_a_queue_mode}"
echo "SUBMIT_A_REFILL_CYCLES=${submit_a_refill_cycles}"
echo "SUBMIT_B_ID=\${submit_b_id}"
echo "SUBMIT_B_QUEUE_MODE=${submit_b_queue_mode}"
echo "SUBMIT_B_REFILL_CYCLES=${submit_b_refill_cycles}"
if [[ "${run_unrelated_after_satisfied_drop}" == "1" || "${run_secondary_dependency_source}" == "1" ]]; then
echo "SUBMIT_C_ID=\${submit_c_id}"
echo "SUBMIT_C_QUEUE_MODE=${submit_c_queue_mode}"
echo "SUBMIT_C_REFILL_CYCLES=${submit_c_refill_cycles}"
fi
if [[ "${run_tertiary_dependency_source}" == "1" ]]; then
echo "SUBMIT_D_ID=\${submit_d_id}"
echo "SUBMIT_D_QUEUE_MODE=${submit_d_queue_mode}"
echo "SUBMIT_D_REFILL_CYCLES=${submit_d_refill_cycles}"
fi
if [[ "${run_quaternary_dependency_source}" == "1" ]]; then
echo "SUBMIT_E_ID=\${submit_e_id}"
echo "SUBMIT_E_QUEUE_MODE=${submit_e_queue_mode}"
echo "SUBMIT_E_REFILL_CYCLES=${submit_e_refill_cycles}"
fi
echo "CLEAR_DEPENDENCY4_AFTER_D=${clear_dependency4_after_d}"
echo "RUN_SUBMIT_E_AFTER_CLEAR=${run_submit_e_after_clear}"
echo "COORD_COMPLETE_TICKET_BASELINE=\${coord_complete_ticket_baseline}"
echo "SUBMIT_B_DEPENDENCY_COMPLETE_TICKET=${submit_b_dependency_complete_ticket_dec}"
echo "SUBMIT_B_DEPENDENCY_COMPLETE_TICKET_RELATIVE=${submit_b_dependency_complete_ticket_relative}"
if [[ "${submit_b_dependency_complete_ticket_dec}" -gt 0 ]]; then
echo "SUBMIT_B_DEPSTATE_OFFSET=\${submit_b_dependency_offset}"
echo "SUBMIT_B_DEPENDENCY_SOURCE_SUBMIT_ID=\${submit_a_id}"
if [[ "${run_secondary_dependency_source}" == "1" ]]; then
echo "SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET=\${submit_b_dependency2_complete_ticket_dec}"
echo "SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET_RELATIVE=${submit_b_dependency2_complete_ticket_relative}"
echo "SUBMIT_B_DEPSTATE2_OFFSET=\${submit_b_dependency2_offset}"
echo "SUBMIT_B_DEPENDENCY2_SOURCE_SUBMIT_ID=\${submit_c_id}"
fi
if [[ "${run_tertiary_dependency_source}" == "1" ]]; then
echo "SUBMIT_B_DEPENDENCY3_COMPLETE_TICKET=\${submit_b_dependency3_complete_ticket_dec}"
echo "SUBMIT_B_DEPENDENCY3_COMPLETE_TICKET_RELATIVE=${submit_b_dependency3_complete_ticket_relative}"
echo "SUBMIT_B_DEPSTATE3_OFFSET=\${submit_b_dependency3_offset}"
echo "SUBMIT_B_DEPENDENCY3_SOURCE_SUBMIT_ID=\${submit_d_id}"
fi
if [[ "${run_quaternary_dependency_source}" == "1" ]]; then
echo "SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET=\${submit_b_dependency4_complete_ticket_dec}"
echo "SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET_RELATIVE=${submit_b_dependency4_complete_ticket_relative}"
echo "SUBMIT_B_DEPSTATE4_OFFSET=\${submit_b_dependency4_offset}"
echo "SUBMIT_B_DEPENDENCY4_SOURCE_SUBMIT_ID=\${submit_e_id}"
fi
fi
echo "RANGE_IDS=\${range_ids[*]}"
echo "OBJECT_IDS_A=\${object_a_control} \${object_a_pds} \${object_a_use} \${object_a_target} \${object_a_status}"
echo "OBJECT_IDS_B=\${object_b_control} \${object_b_pds} \${object_b_use} \${object_b_target} \${object_b_status}"
if [[ "${run_unrelated_after_satisfied_drop}" == "1" || "${run_secondary_dependency_source}" == "1" ]]; then
echo "OBJECT_IDS_C=\${object_c_control} \${object_c_pds} \${object_c_use} \${object_c_target} \${object_c_status}"
fi
if [[ "${run_tertiary_dependency_source}" == "1" ]]; then
echo "OBJECT_IDS_D=\${object_d_control} \${object_d_pds} \${object_d_use} \${object_d_target} \${object_d_status}"
fi
if [[ "${run_quaternary_dependency_source}" == "1" ]]; then
echo "OBJECT_IDS_E=\${object_e_control} \${object_e_pds} \${object_e_use} \${object_e_target} \${object_e_status}"
fi
echo "GTT_STATE_BEFORE"
cat "\${tmpdir}/before_gtt.txt"
echo "OBJECTS_BEFORE"
cat "\${tmpdir}/before_objects.txt"
echo "SUBMITS_BEFORE"
cat "\${tmpdir}/before_submits.txt"
echo "GTT_STATE_DURING"
cat "\${tmpdir}/during_gtt.txt"
echo "OBJECTS_AFTER_ADOPT"
cat "\${tmpdir}/after_adopt_objects.txt"
echo "OBJECTS_AFTER_ROLE_BIND"
cat "\${tmpdir}/after_role_bind_objects.txt"
echo "GTT_STATE_AFTER_ROLE_BIND"
cat "\${tmpdir}/after_role_bind_gtt.txt"
echo "SUBMITS_AFTER_SUBMIT_A_CREATE"
cat "\${tmpdir}/after_submit_a_create_submits.txt"
echo "SUBMIT_B_AFTER_FOCUS"
cat "\${tmpdir}/after_submit_b_focus.txt"
echo "COORD_STATE_AFTER_SETUP"
cat "\${tmpdir}/coord_state_after_setup.txt"
if [[ -f "\${tmpdir}/submit_b_dependency_wait_before_a.txt" ]]; then
echo "SUBMIT_B_DEPENDENCY_WAIT_BEFORE_A"
cat "\${tmpdir}/submit_b_dependency_wait_before_a.txt"
echo "SUBMIT_B_AFTER_AUTO_DEPSTATE_BEFORE_RELEASE"
cat "\${tmpdir}/submit_b_after_auto_depstate_before_release.txt"
if [[ -f "\${tmpdir}/submit_b_after_depticket1_setup.txt" ]]; then
echo "SUBMIT_B_AFTER_DEPTICKET1_SETUP"
cat "\${tmpdir}/submit_b_after_depticket1_setup.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_depstate1_setup.txt" ]]; then
echo "SUBMIT_B_AFTER_DEPSTATE1_SETUP"
cat "\${tmpdir}/submit_b_after_depstate1_setup.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_depstate_setup.txt" ]]; then
echo "SUBMIT_B_AFTER_DEPSTATE_SETUP"
cat "\${tmpdir}/submit_b_after_depstate_setup.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_depticket2_setup.txt" ]]; then
echo "SUBMIT_B_AFTER_DEPTICKET2_SETUP"
cat "\${tmpdir}/submit_b_after_depticket2_setup.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_depstate2_setup.txt" ]]; then
echo "SUBMIT_B_AFTER_DEPSTATE2_SETUP"
cat "\${tmpdir}/submit_b_after_depstate2_setup.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_depticket3_setup.txt" ]]; then
echo "SUBMIT_B_AFTER_DEPTICKET3_SETUP"
cat "\${tmpdir}/submit_b_after_depticket3_setup.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_depstate3_setup.txt" ]]; then
echo "SUBMIT_B_AFTER_DEPSTATE3_SETUP"
cat "\${tmpdir}/submit_b_after_depstate3_setup.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_depticket4_setup.txt" ]]; then
echo "SUBMIT_B_AFTER_DEPTICKET4_SETUP"
cat "\${tmpdir}/submit_b_after_depticket4_setup.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_depstate4_setup.txt" ]]; then
echo "SUBMIT_B_AFTER_DEPSTATE4_SETUP"
cat "\${tmpdir}/submit_b_after_depstate4_setup.txt"
fi
echo "SUBMIT_B_STILL_IDLE_BEFORE_DEPENDENCY_RELEASE"
cat "\${tmpdir}/submit_b_still_idle_before_dependency_release.txt"
echo "COORD_STATE_AFTER_DEPENDENCY_WAIT"
cat "\${tmpdir}/coord_state_after_dependency_wait.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_auto_depstate_after_a.txt" ]]; then
echo "SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_A"
cat "\${tmpdir}/submit_b_after_auto_depstate_after_a.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_source_drop.txt" ]]; then
echo "SUBMIT_B_AFTER_SOURCE_DROP"
cat "\${tmpdir}/submit_b_after_source_drop.txt"
echo "SUBMIT_B_DEPENDENCY_WAIT_AFTER_SOURCE_DROP"
cat "\${tmpdir}/submit_b_dependency_wait_after_source_drop.txt"
echo "SUBMIT_B_AFTER_SOURCE_DROP_WAIT"
cat "\${tmpdir}/submit_b_after_source_drop_wait.txt"
echo "COORD_STATE_AFTER_SOURCE_DROP"
cat "\${tmpdir}/coord_state_after_source_drop.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_satisfied_source_drop.txt" ]]; then
echo "SUBMIT_B_AFTER_SATISFIED_SOURCE_DROP"
cat "\${tmpdir}/submit_b_after_satisfied_source_drop.txt"
echo "SUBMIT_B_STILL_IDLE_AFTER_SATISFIED_SOURCE_DROP"
cat "\${tmpdir}/submit_b_still_idle_after_satisfied_source_drop.txt"
echo "COORD_STATE_AFTER_SATISFIED_SOURCE_DROP"
cat "\${tmpdir}/coord_state_after_satisfied_source_drop.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_unrelated_submit_c.txt" ]]; then
echo "SUBMIT_B_AFTER_UNRELATED_SUBMIT_C"
cat "\${tmpdir}/submit_b_after_unrelated_submit_c.txt"
echo "COORD_STATE_AFTER_SUBMIT_C"
cat "\${tmpdir}/coord_state_after_submit_c.txt"
fi
if [[ -f "\${tmpdir}/submit_b_dependency_wait_after_a.txt" ]]; then
echo "SUBMIT_B_DEPENDENCY_WAIT_AFTER_A"
cat "\${tmpdir}/submit_b_dependency_wait_after_a.txt"
echo "SUBMIT_B_AFTER_DEPENDENCY_WAIT_AFTER_A"
cat "\${tmpdir}/submit_b_after_dependency_wait_after_a.txt"
if [[ -f "\${tmpdir}/submit_b_after_auto_depstate_after_c_first_complete.txt" ]]; then
echo "SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_C_FIRST_COMPLETE"
cat "\${tmpdir}/submit_b_after_auto_depstate_after_c_first_complete.txt"
echo "SUBMIT_B_DEPENDENCY_WAIT_AFTER_C_FIRST_COMPLETE"
cat "\${tmpdir}/submit_b_dependency_wait_after_c_first_complete.txt"
echo "SUBMIT_B_AFTER_DEPENDENCY_WAIT_AFTER_C_FIRST_COMPLETE"
cat "\${tmpdir}/submit_b_after_dependency_wait_after_c_first_complete.txt"
fi
echo "SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_C"
cat "\${tmpdir}/submit_b_after_auto_depstate_after_c.txt"
echo "COORD_STATE_AFTER_SUBMIT_C"
cat "\${tmpdir}/coord_state_after_submit_c.txt"
if [[ -f "\${tmpdir}/submit_b_dependency_wait_after_c.txt" ]]; then
echo "SUBMIT_B_DEPENDENCY_WAIT_AFTER_C"
cat "\${tmpdir}/submit_b_dependency_wait_after_c.txt"
echo "SUBMIT_B_AFTER_DEPENDENCY_WAIT_AFTER_C"
cat "\${tmpdir}/submit_b_after_dependency_wait_after_c.txt"
echo "SUBMIT_B_STILL_IDLE_AFTER_C"
cat "\${tmpdir}/submit_b_still_idle_after_c.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_auto_depstate_after_d.txt" ]]; then
echo "SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_D"
cat "\${tmpdir}/submit_b_after_auto_depstate_after_d.txt"
echo "COORD_STATE_AFTER_SUBMIT_D"
cat "\${tmpdir}/coord_state_after_submit_d.txt"
fi
if [[ -f "\${tmpdir}/submit_b_dependency_wait_after_d.txt" ]]; then
echo "SUBMIT_B_DEPENDENCY_WAIT_AFTER_D"
cat "\${tmpdir}/submit_b_dependency_wait_after_d.txt"
echo "SUBMIT_B_AFTER_DEPENDENCY_WAIT_AFTER_D"
cat "\${tmpdir}/submit_b_after_dependency_wait_after_d.txt"
echo "SUBMIT_B_STILL_IDLE_AFTER_D"
cat "\${tmpdir}/submit_b_still_idle_after_d.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_depclear4.txt" ]]; then
echo "SUBMIT_B_AFTER_DEPCLEAR4"
cat "\${tmpdir}/submit_b_after_depclear4.txt"
echo "SUBMIT_B_STILL_IDLE_AFTER_DEPCLEAR4"
cat "\${tmpdir}/submit_b_still_idle_after_depclear4.txt"
echo "COORD_STATE_AFTER_DEPCLEAR4"
cat "\${tmpdir}/coord_state_after_depclear4.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_auto_depstate_after_e.txt" ]]; then
echo "SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_E"
cat "\${tmpdir}/submit_b_after_auto_depstate_after_e.txt"
echo "COORD_STATE_AFTER_SUBMIT_E"
cat "\${tmpdir}/coord_state_after_submit_e.txt"
fi
if [[ -f "\${tmpdir}/submit_b_after_submit_e_post_clear.txt" ]]; then
echo "SUBMIT_B_AFTER_SUBMIT_E_POST_CLEAR"
cat "\${tmpdir}/submit_b_after_submit_e_post_clear.txt"
echo "COORD_STATE_AFTER_SUBMIT_E_POST_CLEAR"
cat "\${tmpdir}/coord_state_after_submit_e_post_clear.txt"
fi
fi
if [[ -f "\${tmpdir}/coord_state_while_SUBMIT_A_running.txt" ]]; then
echo "COORD_STATE_WHILE_SUBMIT_A_RUNNING"
cat "\${tmpdir}/coord_state_while_SUBMIT_A_running.txt"
echo "SUBMIT_B_RUNNING_BUSY_WHILE_SUBMIT_A_ACTIVE"
cat "\${tmpdir}/SUBMIT_B_running_busy_while_SUBMIT_A_active.txt"
echo "SUBMIT_B_STILL_IDLE_WHILE_SUBMIT_A_ACTIVE"
cat "\${tmpdir}/SUBMIT_B_still_idle_while_SUBMIT_A_active.txt"
fi
if [[ -f "\${tmpdir}/coord_state_after_submit_a.txt" ]]; then
echo "COORD_STATE_AFTER_SUBMIT_A"
cat "\${tmpdir}/coord_state_after_submit_a.txt"
fi
if [[ -f "\${tmpdir}/coord_state_after_submit_b.txt" ]]; then
echo "COORD_STATE_AFTER_SUBMIT_B"
cat "\${tmpdir}/coord_state_after_submit_b.txt"
fi
echo "OBJECTS_AFTER_PROBE_EXIT"
cat "\${tmpdir}/after_probe_exit_objects.txt"
echo "GTT_STATE_AFTER_PROBE_EXIT"
cat "\${tmpdir}/after_probe_exit_gtt.txt"
if [[ -f "\${tmpdir}/submit_a_after_probe_exit_validate.txt" ]]; then
echo "SUBMIT_A_AFTER_PROBE_EXIT_VALIDATE"
cat "\${tmpdir}/submit_a_after_probe_exit_validate.txt"
fi
echo "SUBMIT_B_AFTER_PROBE_EXIT_VALIDATE"
cat "\${tmpdir}/submit_b_after_probe_exit_validate.txt"
if [[ -f "\${tmpdir}/submit_c_after_probe_exit_validate.txt" ]]; then
echo "SUBMIT_C_AFTER_PROBE_EXIT_VALIDATE"
cat "\${tmpdir}/submit_c_after_probe_exit_validate.txt"
fi
if [[ -f "\${tmpdir}/submit_d_after_probe_exit_validate.txt" ]]; then
echo "SUBMIT_D_AFTER_PROBE_EXIT_VALIDATE"
cat "\${tmpdir}/submit_d_after_probe_exit_validate.txt"
fi
if [[ -f "\${tmpdir}/submit_e_after_probe_exit_validate.txt" ]]; then
echo "SUBMIT_E_AFTER_PROBE_EXIT_VALIDATE"
cat "\${tmpdir}/submit_e_after_probe_exit_validate.txt"
fi
echo "SUBMITS_AFTER_DROP"
cat "\${tmpdir}/after_drop_submits.txt"
echo "OBJECTS_AFTER_DROP"
cat "\${tmpdir}/after_drop_objects.txt"
echo "GTT_STATE_AFTER_DROP"
cat "\${tmpdir}/after_drop_gtt.txt"
echo "COORD_STATE_AFTER_DROP"
cat "\${tmpdir}/after_drop_coord.txt"
for idx in \$(seq 1 "${probe_count}"); do
    echo "DRM_PROBE_LOG_\${idx}"
    cat "\${tmpdir}/drm_probe_\${idx}.log"
done
echo "DMESG_TAIL"
dmesg | egrep 'lab object|lab submit|lab pf_|gma500' | tail -n 200 || true
EOF
