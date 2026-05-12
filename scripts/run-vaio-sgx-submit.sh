#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
submit_driver_mode="${SGX_SUBMIT_DRIVER_MODE:-typed}"

case "${submit_driver_mode}" in
    typed)
        exec "${script_dir}/run-vaio-sgx-submit-typed.sh"
        ;;
    legacy)
        ;;
    *)
        echo "SGX_SUBMIT_DRIVER_MODE must be 'typed' or 'legacy'" >&2
        exit 1
        ;;
esac

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_dir="${TARGET_DIR:-/root/poulsbo-workloads}"
target_bin="${TARGET_BIN:-${target_dir}/drm_probe}"
target_range_bin="${TARGET_RANGE_BIN:-${target_dir}/psb_sgx_lab_range_query}"
target_stage="${TARGET_STAGE:-/root/drm_probe.$$.$RANDOM.new}"
target_range_stage="${TARGET_RANGE_STAGE:-/root/psb_sgx_lab_range_query.$$.$RANDOM.new}"
local_bin="${LOCAL_BIN:-$(pwd)/workloads/drm_probe}"
local_range_bin="${LOCAL_RANGE_BIN:-$(pwd)/workloads/psb_sgx_lab_range_query}"
target_node="${TARGET_NODE:-/dev/dri/card0}"
hold_seconds="${HOLD_SECONDS:-20}"
probe_count="${PROBE_COUNT:-5}"
sysfs_dir="${SYSFS_DIR:-/sys/devices/pci0000:00/0000:00:02.0}"
range_scan_max="${RANGE_SCAN_MAX:-512}"
queue_mode="${QUEUE_MODE:-wrap}"
kick_cmd_type="${KICK_CMD_TYPE:-ta}"
kick_ctrl_flags="${KICK_CTRL_FLAGS:-0x1}"
kick_num_ta_status="${KICK_NUM_TA_STATUS:-1}"
kick_num_3d_status="${KICK_NUM_3D_STATUS:-1}"
kick_ta_dependency="${KICK_TA_DEPENDENCY:-0}"
ccb_service="${CCB_SERVICE:-0x1000}"
ccb_cache="${CCB_CACHE:-0x0}"
ccb_data0="${CCB_DATA0:-0xc0de0001}"
ccb_data1="${CCB_DATA1:-0xc0de0002}"
status_ta_offset="${STATUS_TA_OFFSET:-0x80}"
status_3d_offset="${STATUS_3D_OFFSET:-0xc0}"
status_stride="${STATUS_STRIDE:-0x4}"
status_seed="${STATUS_SEED:-0x5a500000}"
issue_enable="${ISSUE_ENABLE:-1}"
transition_enable="${TRANSITION_ENABLE:-1}"
issue_state="${ISSUE_STATE:-}"
issue_issued_packets="${ISSUE_ISSUED_PACKETS:-1}"
transition_running_state="${TRANSITION_RUNNING_STATE:-running}"
transition_complete_state="${TRANSITION_COMPLETE_STATE:-complete}"
refill_enable="${REFILL_ENABLE:-1}"
refill_issue_state="${REFILL_ISSUE_STATE:-queued}"
refill_issued_packets="${REFILL_ISSUED_PACKETS:-1}"
refill_running_state="${REFILL_RUNNING_STATE:-running}"
refill_running_completed_packets="${REFILL_RUNNING_COMPLETED_PACKETS:-0}"
refill_complete_state="${REFILL_COMPLETE_STATE:-complete}"
refill_complete_completed_packets="${REFILL_COMPLETE_COMPLETED_PACKETS:-${refill_issued_packets}}"
refill_cycles="${REFILL_CYCLES:-1}"

case "${queue_mode}" in
    wrap)
        kick_ccb_offset="${KICK_CCB_OFFSET:-0x80}"
        ccb_read_offset="${CCB_READ_OFFSET:-0x60}"
        queue_base_offset="${QUEUE_BASE_OFFSET:-0x20}"
        queue_size="${QUEUE_SIZE:-0x80}"
        queue_packet_size="${QUEUE_PACKET_SIZE:-0x20}"
        sync_offset="${SYNC_OFFSET:-0x20}"
        sync_complete_seq="${SYNC_COMPLETE_SEQ:-0x40}"
        owner_offset="${OWNER_OFFSET:-0x40}"
        owner_complete_cookie_default="0x100"
        if [[ "${issue_enable}" == "1" ]]; then
            ccb_write_offset="${CCB_WRITE_OFFSET:-0x0}"
            queue_packet_count="${QUEUE_PACKET_COUNT:-1}"
            sync_pending_seq="${SYNC_PENDING_SEQ:-0x41}"
            sync_flags="${SYNC_FLAGS:-0x1}"
            owner_issue_cookie_default="0x101"
            transition_running_completed_packets_default="1"
        else
            ccb_write_offset="${CCB_WRITE_OFFSET:-0x20}"
            queue_packet_count="${QUEUE_PACKET_COUNT:-2}"
            sync_pending_seq="${SYNC_PENDING_SEQ:-0x42}"
            sync_flags="${SYNC_FLAGS:-0x3}"
            owner_issue_cookie_default="0x102"
            transition_running_completed_packets_default="1"
        fi
        ;;
    nowrap)
        kick_ccb_offset="${KICK_CCB_OFFSET:-0x20}"
        ccb_write_offset="${CCB_WRITE_OFFSET:-0x20}"
        ccb_read_offset="${CCB_READ_OFFSET:-0x0}"
        queue_base_offset="${QUEUE_BASE_OFFSET:-0x20}"
        queue_size="${QUEUE_SIZE:-0x40}"
        queue_packet_size="${QUEUE_PACKET_SIZE:-0x20}"
        queue_packet_count="${QUEUE_PACKET_COUNT:-1}"
        sync_offset="${SYNC_OFFSET:-0x20}"
        sync_complete_seq="${SYNC_COMPLETE_SEQ:-0x40}"
        sync_pending_seq="${SYNC_PENDING_SEQ:-0x41}"
        sync_flags="${SYNC_FLAGS:-0x1}"
        owner_offset="${OWNER_OFFSET:-0x40}"
        owner_issue_cookie_default="0x101"
        owner_complete_cookie_default="0x100"
        if [[ "${issue_enable}" == "1" ]]; then
            transition_running_completed_packets_default="1"
        else
            transition_running_completed_packets_default="0"
        fi
        ;;
    *)
        echo "QUEUE_MODE must be 'wrap' or 'nowrap'" >&2
        exit 1
        ;;
esac

if [[ "${issue_enable}" == "1" || "${transition_enable}" == "1" ]]; then
    owner_state_default="queued"
else
    owner_state_default="running"
fi
owner_state="${OWNER_STATE:-${owner_state_default}}"
owner_issue_cookie="${OWNER_ISSUE_COOKIE:-${owner_issue_cookie_default}}"
owner_complete_cookie="${OWNER_COMPLETE_COOKIE:-${owner_complete_cookie_default}}"
if [[ -z "${issue_state}" ]]; then
    issue_state="${owner_state}"
fi
queue_packet_count_dec=$((queue_packet_count))
issue_issued_packets_dec=$((issue_issued_packets))
if [[ "${issue_enable}" == "1" && "${issue_issued_packets_dec}" -lt 1 ]]; then
    echo "ISSUE_ISSUED_PACKETS must be at least 1 when ISSUE_ENABLE=1" >&2
    exit 1
fi
refill_issued_packets_dec=$((refill_issued_packets))
refill_running_completed_packets_dec=$((refill_running_completed_packets))
refill_complete_completed_packets_dec=$((refill_complete_completed_packets))
refill_cycles_dec=$((refill_cycles))
if [[ "${refill_enable}" == "1" ]]; then
    if [[ "${refill_issued_packets_dec}" -lt 1 ]]; then
        echo "REFILL_ISSUED_PACKETS must be at least 1 when REFILL_ENABLE=1" >&2
        exit 1
    fi
    if [[ "${refill_cycles_dec}" -lt 1 ]]; then
        echo "REFILL_CYCLES must be at least 1 when REFILL_ENABLE=1" >&2
        exit 1
    fi
    if [[ $((refill_running_completed_packets_dec + refill_complete_completed_packets_dec)) -ne "${refill_issued_packets_dec}" ]]; then
        echo "REFILL_RUNNING_COMPLETED_PACKETS + REFILL_COMPLETE_COMPLETED_PACKETS must equal REFILL_ISSUED_PACKETS" >&2
        exit 1
    fi
fi
transition_base_packet_count_dec=$((queue_packet_count_dec))
if [[ "${issue_enable}" == "1" ]]; then
    transition_base_packet_count_dec=$((transition_base_packet_count_dec + issue_issued_packets_dec))
fi
transition_running_completed_packets="${TRANSITION_RUNNING_COMPLETED_PACKETS:-${transition_running_completed_packets_default}}"
transition_running_completed_packets_dec=$((transition_running_completed_packets))
transition_complete_completed_packets_default=$((transition_base_packet_count_dec - transition_running_completed_packets_dec))
transition_complete_completed_packets="${TRANSITION_COMPLETE_COMPLETED_PACKETS:-${transition_complete_completed_packets_default}}"

if [[ "${probe_count}" -lt 5 ]]; then
    echo "PROBE_COUNT must be at least 5 for control+pds+use+target+status coverage" >&2
    exit 1
fi

if [[ "${range_scan_max}" -lt 1 ]]; then
    echo "RANGE_SCAN_MAX must be at least 1" >&2
    exit 1
fi

if [[ ! -f "$local_bin" ]]; then
    echo "Local probe not found: $local_bin" >&2
    exit 1
fi

if [[ ! -f "$local_range_bin" ]]; then
    echo "Local range query helper not found: $local_range_bin" >&2
    exit 1
fi

scp "$local_bin" "${target_user}@${target_host}:${target_stage}"
scp "$local_range_bin" "${target_user}@${target_host}:${target_range_stage}"
ssh "${target_user}@${target_host}" "bash -s" <<EOF
set -euo pipefail

mkdir -p "${target_dir}"
rm -f "${target_bin}"
install -m 0755 -T "${target_stage}" "${target_bin}"
install -m 0755 -T "${target_range_stage}" "${target_range_bin}"
rm -f "${target_stage}"
rm -f "${target_range_stage}"

object_node="${sysfs_dir}/poulsbo_sgx_object"
objects_node="${sysfs_dir}/poulsbo_sgx_objects"
submit_node="${sysfs_dir}/poulsbo_sgx_submit"
submits_node="${sysfs_dir}/poulsbo_sgx_submits"
gtt_node="${sysfs_dir}/poulsbo_gtt_state"
tmpdir=\$(mktemp -d)
probe_pids=()
submit_id=""

cleanup() {
    local pid
    for pid in "\${probe_pids[@]:-}"; do
        kill "\${pid}" 2>/dev/null || true
        wait "\${pid}" 2>/dev/null || true
    done
    rm -rf "\${tmpdir}"
}
trap cleanup EXIT

for node in "\${object_node}" "\${objects_node}" "\${submit_node}" "\${submits_node}" "\${gtt_node}"; do
    if [[ ! -e "\${node}" ]]; then
        echo "Missing sysfs node: \${node}" >&2
        exit 1
    fi
done

range_query_list_gem_ids() {
    local range_index
    local range_out
    local range_ioctl_ret
    local range_id
    local range_type_name
    local range_mmapping
    local range_sgx_object_id

    for range_index in \$(seq 0 "${range_scan_max}"); do
        range_out="\$("${target_range_bin}" "${target_node}" index "\${range_index}" gem 2>/dev/null || true)"
        [[ -n "\${range_out}" ]] || break
        range_ioctl_ret=\$(printf '%s\n' "\${range_out}" | awk '/^range_ioctl_ret=/ { sub(/^range_ioctl_ret=/, ""); print \$1; exit }')
        if [[ "\${range_ioctl_ret:-}" != "0" ]]; then
            break
        fi
        range_id=\$(printf '%s\n' "\${range_out}" | awk -F= '/^range_resolved_id=/ { print \$2 }')
        range_type_name=\$(printf '%s\n' "\${range_out}" | awk -F= '/^range_type_name=/ { print \$2 }')
        range_mmapping=\$(printf '%s\n' "\${range_out}" | awk -F= '/^range_mmapping=/ { print \$2 }')
        range_sgx_object_id=\$(printf '%s\n' "\${range_out}" | awk -F= '/^range_sgx_object_id=/ { print \$2 }')
        if [[ "\${range_type_name}" == "gem" && "\${range_mmapping}" == "1" && "\${range_sgx_object_id}" == "0" ]]; then
            echo "\${range_id}"
        fi
    done
}

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

cat "\${gtt_node}" > "\${tmpdir}/before_gtt.txt"
cat "\${objects_node}" > "\${tmpdir}/before_objects.txt"
cat "\${submits_node}" > "\${tmpdir}/before_submits.txt"
before_range_ids=()
while read -r before_range_id; do
    [[ -n "\${before_range_id}" ]] || continue
    before_range_ids+=("\${before_range_id}")
done < <(range_query_list_gem_ids)
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

for _ in \$(seq 1 $((hold_seconds + 5))); do
    ready_count=\$(grep -h '^drm_hold_seconds=' "\${tmpdir}"/drm_probe_*.log 2>/dev/null | wc -l || true)
    if [[ "\${ready_count}" -ge "${probe_count}" ]]; then
        break
    fi
    sleep 1
done

cat "\${gtt_node}" > "\${tmpdir}/during_gtt.txt"
range_ids=()
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
done < <(range_query_list_gem_ids)

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
    if echo "adopt \${candidate_range_id}" > "\${object_node}" 2>/dev/null; then
        range_ids+=("\${candidate_range_id}")
        if [[ "\${#range_ids[@]}" -ge "${probe_count}" ]]; then
            break
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
        if echo "adopt \${candidate_range_id}" > "\${object_node}" 2>/dev/null; then
            range_ids+=("\${candidate_range_id}")
            if [[ "\${#range_ids[@]}" -ge "${probe_count}" ]]; then
                break
            fi
        fi
    done
fi

if [[ "\${#range_ids[@]}" -lt "${probe_count}" ]]; then
    echo "Expected ${probe_count} fresh SGX-adoptable GEM ranges, found \${#range_ids[@]}" >&2
    echo "MAX_BEFORE_RANGE_ID=\${max_before_range_id}" >&2
    echo "RANGE_SCAN_MAX=${range_scan_max}" >&2
    echo "DURING_GTT" >&2
    cat "\${tmpdir}/during_gtt.txt" >&2
    exit 1
fi

cat "\${objects_node}" > "\${tmpdir}/after_adopt_objects.txt"

declare -A range_to_object
while read -r object_id range_id; do
    range_to_object["\${range_id}"]="\${object_id}"
done < <(awk '
/^object id=/ {
    object_id = range_id = ""
    for (i = 1; i <= NF; i++) {
        if (\$i ~ /^id=/) {
            split(\$i, a, "=")
            object_id = a[2]
        } else if (\$i ~ /^range_id=/) {
            split(\$i, a, "=")
            range_id = a[2]
        }
    }
    if (object_id != "" && range_id != "")
        print object_id, range_id
}
' "\${tmpdir}/after_adopt_objects.txt")

object_control="\${range_to_object[\${range_ids[0]}]}"
object_pds="\${range_to_object[\${range_ids[1]}]}"
object_use="\${range_to_object[\${range_ids[2]}]}"
object_target="\${range_to_object[\${range_ids[3]}]}"
object_status="\${range_to_object[\${range_ids[4]}]}"

for object_id in "\${object_control}" "\${object_pds}" "\${object_use}" "\${object_target}" "\${object_status}"; do
    if [[ -z "\${object_id}" ]]; then
        echo "Failed to map adopted object IDs" >&2
        cat "\${tmpdir}/after_adopt_objects.txt"
        exit 1
    fi
done

echo "role \${object_control} control" > "\${object_node}"
echo "role \${object_pds} pds" > "\${object_node}"
echo "role \${object_use} use" > "\${object_node}"
echo "role \${object_target} target" > "\${object_node}"
echo "role \${object_status} status" > "\${object_node}"
echo "class \${object_control} packet" > "\${object_node}"
echo "class \${object_pds} code" > "\${object_node}"
echo "class \${object_use} code" > "\${object_node}"
echo "class \${object_target} surface" > "\${object_node}"
echo "class \${object_status} state" > "\${object_node}"
echo "gpu_use \${object_control} control_ccb" > "\${object_node}"
echo "gpu_use \${object_pds} pds_code" > "\${object_node}"
echo "gpu_use \${object_use} use_code" > "\${object_node}"
echo "gpu_use \${object_target} render_target" > "\${object_node}"
echo "gpu_use \${object_status} status_block" > "\${object_node}"
for object_id in "\${object_control}" "\${object_pds}" "\${object_use}" "\${object_target}" "\${object_status}"; do
    echo "context \${object_id} pf" > "\${object_node}"
    echo "pf_bind \${object_id}" > "\${object_node}"
done
cat "\${objects_node}" > "\${tmpdir}/after_role_bind_objects.txt"
cat "\${gtt_node}" > "\${tmpdir}/after_role_bind_gtt.txt"

echo "create pf" > "\${submit_node}"
cat "\${submits_node}" > "\${tmpdir}/after_create_submits.txt"
submit_id=\$(awk '
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
' "\${tmpdir}/after_create_submits.txt")

if [[ -z "\${submit_id}" ]]; then
    echo "Failed to identify SGX submit ID" >&2
    cat "\${tmpdir}/after_create_submits.txt"
    exit 1
fi

echo "kick \${submit_id} ${kick_cmd_type} ${kick_ccb_offset} ${kick_ctrl_flags} ${kick_num_ta_status} ${kick_num_3d_status} ${kick_ta_dependency}" > "\${submit_node}"
cat "\${submits_node}" > "\${tmpdir}/after_kick_submits.txt"

echo "attach \${submit_id} control \${object_control}" > "\${submit_node}"
echo "attach \${submit_id} pds \${object_pds}" > "\${submit_node}"
echo "attach \${submit_id} use \${object_use}" > "\${submit_node}"
echo "attach \${submit_id} target \${object_target}" > "\${submit_node}"
echo "attach \${submit_id} status \${object_status}" > "\${submit_node}"
echo "ccb \${submit_id} ${ccb_write_offset} ${ccb_read_offset} ${ccb_service} ${ccb_cache} ${ccb_data0} ${ccb_data1}" > "\${submit_node}"
cat "\${submits_node}" > "\${tmpdir}/after_ccb_submits.txt"
echo "queue \${submit_id} ${queue_base_offset} ${queue_size} ${queue_packet_size} ${queue_packet_count}" > "\${submit_node}"
cat "\${submits_node}" > "\${tmpdir}/after_queue_submits.txt"
echo "status \${submit_id} ${status_ta_offset} ${status_3d_offset} ${status_stride} ${status_seed}" > "\${submit_node}"
cat "\${submits_node}" > "\${tmpdir}/after_status_submits.txt"
echo "sync \${submit_id} ${sync_offset} ${sync_complete_seq} ${sync_pending_seq} ${sync_flags}" > "\${submit_node}"
cat "\${submits_node}" > "\${tmpdir}/after_sync_submits.txt"
echo "owner \${submit_id} ${owner_offset} ${owner_state} ${owner_issue_cookie} ${owner_complete_cookie}" > "\${submit_node}"
cat "\${submits_node}" > "\${tmpdir}/after_owner_submits.txt"
echo "validate \${submit_id}" > "\${submit_node}"
cat "\${submits_node}" > "\${tmpdir}/after_validate_submits.txt"

if [[ "${issue_enable}" == "1" ]]; then
    echo "issue \${submit_id} ${issue_state} ${issue_issued_packets}" > "\${submit_node}"
    cat "\${submits_node}" > "\${tmpdir}/after_issue_submits.txt"
    echo "validate \${submit_id}" > "\${submit_node}"
    cat "\${submits_node}" > "\${tmpdir}/after_issue_validate_submits.txt"
fi

if [[ "${transition_enable}" == "1" ]]; then
    echo "transition \${submit_id} ${transition_running_state} ${transition_running_completed_packets}" > "\${submit_node}"
    cat "\${submits_node}" > "\${tmpdir}/after_transition_running_submits.txt"
    echo "validate \${submit_id}" > "\${submit_node}"
    cat "\${submits_node}" > "\${tmpdir}/after_transition_running_validate_submits.txt"
    echo "transition \${submit_id} ${transition_complete_state} ${transition_complete_completed_packets}" > "\${submit_node}"
    cat "\${submits_node}" > "\${tmpdir}/after_transition_complete_submits.txt"
    echo "validate \${submit_id}" > "\${submit_node}"
    cat "\${submits_node}" > "\${tmpdir}/after_transition_complete_validate_submits.txt"
fi

if [[ "${refill_enable}" == "1" ]]; then
    if [[ "${refill_cycles}" == "1" ]]; then
        echo "issue \${submit_id} ${refill_issue_state} ${refill_issued_packets}" > "\${submit_node}"
        cat "\${submits_node}" > "\${tmpdir}/after_refill_issue_submits.txt"
        echo "validate \${submit_id}" > "\${submit_node}"
        cat "\${submits_node}" > "\${tmpdir}/after_refill_issue_validate_submits.txt"
        echo "transition \${submit_id} ${refill_running_state} ${refill_running_completed_packets}" > "\${submit_node}"
        cat "\${submits_node}" > "\${tmpdir}/after_refill_running_submits.txt"
        echo "validate \${submit_id}" > "\${submit_node}"
        cat "\${submits_node}" > "\${tmpdir}/after_refill_running_validate_submits.txt"
        echo "transition \${submit_id} ${refill_complete_state} ${refill_complete_completed_packets}" > "\${submit_node}"
        cat "\${submits_node}" > "\${tmpdir}/after_refill_complete_submits.txt"
        echo "validate \${submit_id}" > "\${submit_node}"
        cat "\${submits_node}" > "\${tmpdir}/after_refill_complete_validate_submits.txt"
    else
        for cycle in \$(seq 1 "${refill_cycles}"); do
            echo "issue \${submit_id} ${refill_issue_state} ${refill_issued_packets}" > "\${submit_node}"
            cat "\${submits_node}" > "\${tmpdir}/after_refill_cycle_\${cycle}_issue_submits.txt"
            echo "validate \${submit_id}" > "\${submit_node}"
            cat "\${submits_node}" > "\${tmpdir}/after_refill_cycle_\${cycle}_issue_validate_submits.txt"
            echo "transition \${submit_id} ${refill_running_state} ${refill_running_completed_packets}" > "\${submit_node}"
            cat "\${submits_node}" > "\${tmpdir}/after_refill_cycle_\${cycle}_running_submits.txt"
            echo "validate \${submit_id}" > "\${submit_node}"
            cat "\${submits_node}" > "\${tmpdir}/after_refill_cycle_\${cycle}_running_validate_submits.txt"
            echo "transition \${submit_id} ${refill_complete_state} ${refill_complete_completed_packets}" > "\${submit_node}"
            cat "\${submits_node}" > "\${tmpdir}/after_refill_cycle_\${cycle}_complete_submits.txt"
            echo "validate \${submit_id}" > "\${submit_node}"
            cat "\${submits_node}" > "\${tmpdir}/after_refill_cycle_\${cycle}_complete_validate_submits.txt"
        done
    fi
fi

for pid in "\${probe_pids[@]}"; do
    wait "\${pid}"
done
probe_pids=()

cat "\${objects_node}" > "\${tmpdir}/after_probe_exit_objects.txt"
cat "\${gtt_node}" > "\${tmpdir}/after_probe_exit_gtt.txt"
echo "validate \${submit_id}" > "\${submit_node}"
cat "\${submits_node}" > "\${tmpdir}/after_probe_exit_validate_submits.txt"

echo "drop \${submit_id}" > "\${submit_node}"
for object_id in "\${object_control}" "\${object_pds}" "\${object_use}" "\${object_target}" "\${object_status}"; do
    echo "drop \${object_id}" > "\${object_node}"
done
cat "\${submits_node}" > "\${tmpdir}/after_drop_submits.txt"
cat "\${objects_node}" > "\${tmpdir}/after_drop_objects.txt"
cat "\${gtt_node}" > "\${tmpdir}/after_drop_gtt.txt"

echo "REMOTE_BIN=${target_bin}"
echo "QUEUE_MODE=${queue_mode}"
echo "OBJECT_NODE=\${object_node}"
echo "OBJECTS_NODE=\${objects_node}"
echo "SUBMIT_NODE=\${submit_node}"
echo "SUBMITS_NODE=\${submits_node}"
echo "HOLD_SECONDS=${hold_seconds}"
echo "ISSUE_ENABLE=${issue_enable}"
echo "ISSUE_STATE=${issue_state}"
echo "ISSUE_ISSUED_PACKETS=${issue_issued_packets}"
echo "TRANSITION_ENABLE=${transition_enable}"
echo "TRANSITION_RUNNING_STATE=${transition_running_state}"
echo "TRANSITION_RUNNING_COMPLETED_PACKETS=${transition_running_completed_packets}"
echo "TRANSITION_COMPLETE_STATE=${transition_complete_state}"
echo "TRANSITION_COMPLETE_COMPLETED_PACKETS=${transition_complete_completed_packets}"
echo "REFILL_ENABLE=${refill_enable}"
echo "REFILL_ISSUE_STATE=${refill_issue_state}"
echo "REFILL_ISSUED_PACKETS=${refill_issued_packets}"
echo "REFILL_RUNNING_STATE=${refill_running_state}"
echo "REFILL_RUNNING_COMPLETED_PACKETS=${refill_running_completed_packets}"
echo "REFILL_COMPLETE_STATE=${refill_complete_state}"
echo "REFILL_COMPLETE_COMPLETED_PACKETS=${refill_complete_completed_packets}"
echo "REFILL_CYCLES=${refill_cycles}"
echo "RANGE_IDS=\${range_ids[*]}"
echo "OBJECT_IDS=\${object_control} \${object_pds} \${object_use} \${object_target} \${object_status}"
echo "SUBMIT_ID=\${submit_id}"
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
echo "SUBMITS_AFTER_CREATE"
cat "\${tmpdir}/after_create_submits.txt"
echo "SUBMITS_AFTER_KICK"
cat "\${tmpdir}/after_kick_submits.txt"
echo "SUBMITS_AFTER_CCB"
cat "\${tmpdir}/after_ccb_submits.txt"
echo "SUBMITS_AFTER_QUEUE"
cat "\${tmpdir}/after_queue_submits.txt"
echo "SUBMITS_AFTER_STATUS"
cat "\${tmpdir}/after_status_submits.txt"
echo "SUBMITS_AFTER_SYNC"
cat "\${tmpdir}/after_sync_submits.txt"
echo "SUBMITS_AFTER_OWNER"
cat "\${tmpdir}/after_owner_submits.txt"
echo "SUBMITS_AFTER_VALIDATE"
cat "\${tmpdir}/after_validate_submits.txt"
if [[ "${issue_enable}" == "1" ]]; then
    echo "SUBMITS_AFTER_ISSUE"
    cat "\${tmpdir}/after_issue_submits.txt"
    echo "SUBMITS_AFTER_ISSUE_VALIDATE"
    cat "\${tmpdir}/after_issue_validate_submits.txt"
fi
if [[ "${transition_enable}" == "1" ]]; then
    echo "SUBMITS_AFTER_TRANSITION_RUNNING"
    cat "\${tmpdir}/after_transition_running_submits.txt"
    echo "SUBMITS_AFTER_TRANSITION_RUNNING_VALIDATE"
    cat "\${tmpdir}/after_transition_running_validate_submits.txt"
    echo "SUBMITS_AFTER_TRANSITION_COMPLETE"
    cat "\${tmpdir}/after_transition_complete_submits.txt"
    echo "SUBMITS_AFTER_TRANSITION_COMPLETE_VALIDATE"
    cat "\${tmpdir}/after_transition_complete_validate_submits.txt"
fi
if [[ "${refill_enable}" == "1" ]]; then
    if [[ "${refill_cycles}" == "1" ]]; then
        echo "SUBMITS_AFTER_REFILL_ISSUE"
        cat "\${tmpdir}/after_refill_issue_submits.txt"
        echo "SUBMITS_AFTER_REFILL_ISSUE_VALIDATE"
        cat "\${tmpdir}/after_refill_issue_validate_submits.txt"
        echo "SUBMITS_AFTER_REFILL_RUNNING"
        cat "\${tmpdir}/after_refill_running_submits.txt"
        echo "SUBMITS_AFTER_REFILL_RUNNING_VALIDATE"
        cat "\${tmpdir}/after_refill_running_validate_submits.txt"
        echo "SUBMITS_AFTER_REFILL_COMPLETE"
        cat "\${tmpdir}/after_refill_complete_submits.txt"
        echo "SUBMITS_AFTER_REFILL_COMPLETE_VALIDATE"
        cat "\${tmpdir}/after_refill_complete_validate_submits.txt"
    else
        for cycle in \$(seq 1 "${refill_cycles}"); do
            echo "SUBMITS_AFTER_REFILL_CYCLE_\${cycle}_ISSUE"
            cat "\${tmpdir}/after_refill_cycle_\${cycle}_issue_submits.txt"
            echo "SUBMITS_AFTER_REFILL_CYCLE_\${cycle}_ISSUE_VALIDATE"
            cat "\${tmpdir}/after_refill_cycle_\${cycle}_issue_validate_submits.txt"
            echo "SUBMITS_AFTER_REFILL_CYCLE_\${cycle}_RUNNING"
            cat "\${tmpdir}/after_refill_cycle_\${cycle}_running_submits.txt"
            echo "SUBMITS_AFTER_REFILL_CYCLE_\${cycle}_RUNNING_VALIDATE"
            cat "\${tmpdir}/after_refill_cycle_\${cycle}_running_validate_submits.txt"
            echo "SUBMITS_AFTER_REFILL_CYCLE_\${cycle}_COMPLETE"
            cat "\${tmpdir}/after_refill_cycle_\${cycle}_complete_submits.txt"
            echo "SUBMITS_AFTER_REFILL_CYCLE_\${cycle}_COMPLETE_VALIDATE"
            cat "\${tmpdir}/after_refill_cycle_\${cycle}_complete_validate_submits.txt"
        done
    fi
fi
echo "OBJECTS_AFTER_PROBE_EXIT"
cat "\${tmpdir}/after_probe_exit_objects.txt"
echo "GTT_STATE_AFTER_PROBE_EXIT"
cat "\${tmpdir}/after_probe_exit_gtt.txt"
echo "SUBMITS_AFTER_PROBE_EXIT_VALIDATE"
cat "\${tmpdir}/after_probe_exit_validate_submits.txt"
echo "SUBMITS_AFTER_DROP"
cat "\${tmpdir}/after_drop_submits.txt"
echo "OBJECTS_AFTER_DROP"
cat "\${tmpdir}/after_drop_objects.txt"
echo "GTT_STATE_AFTER_DROP"
cat "\${tmpdir}/after_drop_gtt.txt"
for idx in \$(seq 1 "${probe_count}"); do
    echo "DRM_PROBE_LOG_\${idx}"
    cat "\${tmpdir}/drm_probe_\${idx}.log"
done
echo "DMESG_TAIL"
dmesg | egrep 'lab object|lab submit|lab pf_|gma500' | tail -n 120 || true
EOF
