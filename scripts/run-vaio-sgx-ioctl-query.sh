#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_dir="${TARGET_DIR:-/root/poulsbo-workloads}"
target_probe_bin="${TARGET_PROBE_BIN:-${target_dir}/drm_probe}"
target_query_bin="${TARGET_QUERY_BIN:-${target_dir}/psb_sgx_lab_query}"
target_probe_stage="${TARGET_PROBE_STAGE:-/root/drm_probe.$$.$RANDOM.new}"
target_query_stage="${TARGET_QUERY_STAGE:-/root/psb_sgx_lab_query.$$.$RANDOM.new}"
local_probe_bin="${LOCAL_PROBE_BIN:-$(pwd)/workloads/drm_probe}"
local_query_bin="${LOCAL_QUERY_BIN:-$(pwd)/workloads/psb_sgx_lab_query}"
target_node="${TARGET_NODE:-/dev/dri/card0}"
sysfs_dir="${SYSFS_DIR:-/sys/devices/pci0000:00/0000:00:02.0}"
hold_seconds="${HOLD_SECONDS:-15}"
probe_count="${PROBE_COUNT:-5}"

if [[ "${probe_count}" -lt 5 ]]; then
    echo "PROBE_COUNT must be at least 5" >&2
    exit 1
fi

for bin in "$local_probe_bin" "$local_query_bin"; do
    if [[ ! -f "$bin" ]]; then
        echo "Local binary not found: $bin" >&2
        exit 1
    fi
done

scp "$local_probe_bin" "${target_user}@${target_host}:${target_probe_stage}"
scp "$local_query_bin" "${target_user}@${target_host}:${target_query_stage}"

ssh "${target_user}@${target_host}" "bash -s" <<EOF
set -euo pipefail

mkdir -p "${target_dir}"
install -m 0755 -T "${target_probe_stage}" "${target_probe_bin}"
install -m 0755 -T "${target_query_stage}" "${target_query_bin}"
rm -f "${target_probe_stage}" "${target_query_stage}"

object_node="${sysfs_dir}/poulsbo_sgx_object"
objects_node="${sysfs_dir}/poulsbo_sgx_objects"
submit_node="${sysfs_dir}/poulsbo_sgx_submit"
submits_node="${sysfs_dir}/poulsbo_sgx_submits"
submit_state_node="${sysfs_dir}/poulsbo_sgx_submit_state"
coord_node="${sysfs_dir}/poulsbo_sgx_coord_state"
gtt_node="${sysfs_dir}/poulsbo_gtt_state"
tmpdir=\$(mktemp -d)
probe_pids=()
submit_id=""
object_ids=()

cleanup() {
    local pid
    if [[ -n "\${submit_id}" ]]; then
        echo "drop \${submit_id}" > "\${submit_node}" 2>/dev/null || true
    fi
    for object_id in "\${object_ids[@]:-}"; do
        echo "drop \${object_id}" > "\${object_node}" 2>/dev/null || true
    done
    for pid in "\${probe_pids[@]:-}"; do
        kill "\${pid}" 2>/dev/null || true
        wait "\${pid}" 2>/dev/null || true
    done
    rm -rf "\${tmpdir}"
}
trap cleanup EXIT

for node in "\${object_node}" "\${objects_node}" "\${submit_node}" \
            "\${submits_node}" "\${submit_state_node}" "\${coord_node}" \
            "\${gtt_node}"; do
    if [[ ! -e "\${node}" ]]; then
        echo "Missing sysfs node: \${node}" >&2
        exit 1
    fi
done

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
END { print max_id + 0 }
' "\${tmpdir}/before_gtt.txt")

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

cat "\${gtt_node}" > "\${tmpdir}/during_gtt.txt"
range_ids=()
while read -r candidate_range_id; do
    [[ -n "\${candidate_range_id}" ]] || continue
    if echo "adopt \${candidate_range_id}" > "\${object_node}" 2>/dev/null; then
        range_ids+=("\${candidate_range_id}")
        if [[ "\${#range_ids[@]}" -ge "${probe_count}" ]]; then
            break
        fi
    fi
done < <(awk -v min_id="\${max_before_range_id}" '
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

if [[ "\${#range_ids[@]}" -lt "${probe_count}" ]]; then
    for candidate_range_id in \$(seq \$((max_before_range_id + 1)) \$((max_before_range_id + 128))); do
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
    echo "Expected ${probe_count} fresh SGX-adoptable ranges, found \${#range_ids[@]}" >&2
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
object_ids=("\${object_control}" "\${object_pds}" "\${object_use}" "\${object_target}" "\${object_status}")

for object_id in "\${object_ids[@]}"; do
    if [[ -z "\${object_id}" ]]; then
        echo "Failed to map adopted object IDs" >&2
        cat "\${tmpdir}/after_adopt_objects.txt" >&2
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
for object_id in "\${object_ids[@]}"; do
    echo "context \${object_id} pf" > "\${object_node}"
    echo "pf_bind \${object_id}" > "\${object_node}"
done

echo "create pf" > "\${submit_node}"
submit_id=\$(awk '
/^submit id=/ {
    for (i = 1; i <= NF; i++) {
        if (\$i ~ /^id=/) {
            split(\$i, a, "=")
            last = a[2]
        }
    }
}
END {
    if (last != "")
        print last
}
' "\${submits_node}")

if [[ -z "\${submit_id}" ]]; then
    echo "Failed to identify SGX submit ID" >&2
    cat "\${submits_node}" >&2
    exit 1
fi

echo "kick \${submit_id} ta 0x20 0x1 1 1 0" > "\${submit_node}"
echo "attach \${submit_id} control \${object_control}" > "\${submit_node}"
echo "attach \${submit_id} pds \${object_pds}" > "\${submit_node}"
echo "attach \${submit_id} use \${object_use}" > "\${submit_node}"
echo "attach \${submit_id} target \${object_target}" > "\${submit_node}"
echo "attach \${submit_id} status \${object_status}" > "\${submit_node}"
echo "ccb \${submit_id} 0x20 0x0 0x1000 0x0 0xc0de0001 0xc0de0002" > "\${submit_node}"
echo "queue \${submit_id} 0x20 0x40 0x20 1" > "\${submit_node}"
echo "status \${submit_id} 0x80 0xc0 0x4 0x5a500000" > "\${submit_node}"
echo "sync \${submit_id} 0x20 0x40 0x41 0x1" > "\${submit_node}"
echo "owner \${submit_id} 0x40 queued 0x101 0x100" > "\${submit_node}"
echo "validate \${submit_id}" > "\${submit_node}"
echo "focus \${submit_id}" > "\${submit_node}"

cat "\${submit_state_node}" > "\${tmpdir}/submit_state_base.txt"
cat "\${coord_node}" > "\${tmpdir}/coord_state_base.txt"
"${target_query_bin}" "${target_node}" "\${submit_id}" > "\${tmpdir}/query_by_id_base.txt"
"${target_query_bin}" "${target_node}" focus > "\${tmpdir}/query_focus_base.txt"

echo "issue \${submit_id} queued 1" > "\${submit_node}"
echo "validate \${submit_id}" > "\${submit_node}"
cat "\${submit_state_node}" > "\${tmpdir}/submit_state_after_issue.txt"
cat "\${coord_node}" > "\${tmpdir}/coord_state_after_issue.txt"
"${target_query_bin}" "${target_node}" "\${submit_id}" > "\${tmpdir}/query_by_id_after_issue.txt"

for pid in "\${probe_pids[@]}"; do
    wait "\${pid}"
done
probe_pids=()

echo "REMOTE_PROBE_BIN=${target_probe_bin}"
echo "REMOTE_QUERY_BIN=${target_query_bin}"
echo "SUBMIT_ID=\${submit_id}"
echo "OBJECT_IDS=\${object_ids[*]}"
echo "SUBMIT_STATE_BASE"
cat "\${tmpdir}/submit_state_base.txt"
echo "COORD_STATE_BASE"
cat "\${tmpdir}/coord_state_base.txt"
echo "IOCTL_QUERY_BY_ID_BASE"
cat "\${tmpdir}/query_by_id_base.txt"
echo "IOCTL_QUERY_FOCUS_BASE"
cat "\${tmpdir}/query_focus_base.txt"
echo "SUBMIT_STATE_AFTER_ISSUE"
cat "\${tmpdir}/submit_state_after_issue.txt"
echo "COORD_STATE_AFTER_ISSUE"
cat "\${tmpdir}/coord_state_after_issue.txt"
echo "IOCTL_QUERY_BY_ID_AFTER_ISSUE"
cat "\${tmpdir}/query_by_id_after_issue.txt"
echo "DMESG_TAIL"
dmesg | egrep 'lab submit|lab object|gma500' | tail -n 120 || true
EOF
