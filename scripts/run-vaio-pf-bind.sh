#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_dir="${TARGET_DIR:-/root/poulsbo-workloads}"
target_bin="${TARGET_BIN:-${target_dir}/drm_probe}"
target_stage="${TARGET_STAGE:-/root/drm_probe.new}"
local_bin="${LOCAL_BIN:-$(pwd)/workloads/drm_probe}"
target_node="${TARGET_NODE:-/dev/dri/card0}"
hold_seconds="${HOLD_SECONDS:-20}"
sysfs_dir="${SYSFS_DIR:-/sys/devices/pci0000:00/0000:00:02.0}"

if [[ ! -f "$local_bin" ]]; then
    echo "Local probe not found: $local_bin" >&2
    exit 1
fi

scp "$local_bin" "${target_user}@${target_host}:${target_stage}"
ssh "${target_user}@${target_host}" "bash -s" <<EOF
set -euo pipefail

mkdir -p "${target_dir}"
install -m 0755 "${target_stage}" "${target_bin}"
rm -f "${target_stage}"

bind_node="${sysfs_dir}/poulsbo_sgx_bind"
gtt_node="${sysfs_dir}/poulsbo_gtt_state"
tmpdir=\$(mktemp -d)
probe_pid=""

cleanup() {
    if [[ -n "\${probe_pid}" ]]; then
        kill "\${probe_pid}" 2>/dev/null || true
        wait "\${probe_pid}" 2>/dev/null || true
    fi
    rm -rf "\${tmpdir}"
}
trap cleanup EXIT

if [[ ! -e "\${bind_node}" ]]; then
    echo "Missing sysfs control: \${bind_node}" >&2
    exit 1
fi

if [[ ! -e "\${gtt_node}" ]]; then
    echo "Missing GTT state view: \${gtt_node}" >&2
    exit 1
fi

cat "\${gtt_node}" > "\${tmpdir}/before.txt"
"${target_bin}" "${target_node}" "${hold_seconds}" > "\${tmpdir}/drm_probe_hold.log" 2>&1 &
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

cat "\${gtt_node}" > "\${tmpdir}/during.txt"
range_id=\$(awk '
FNR == NR && /^range id=/ {
    id = name = ""
    for (i = 1; i <= NF; i++) {
        if (\$i ~ /^id=/) {
            split(\$i, a, "=")
            id = a[2]
        } else if (\$i ~ /^name=/) {
            split(\$i, a, "=")
            name = a[2]
        }
    }
    if (name == "gem")
        before[id] = 1
    next
}
/^range id=/ {
    id = name = mmapping = stolen = ""
    for (i = 1; i <= NF; i++) {
        if (\$i ~ /^id=/) {
            split(\$i, a, "=")
            id = a[2]
        } else if (\$i ~ /^name=/) {
            split(\$i, a, "=")
            name = a[2]
        } else if (\$i ~ /^mmapping=/) {
            split(\$i, a, "=")
            mmapping = a[2]
        } else if (\$i ~ /^stolen=/) {
            split(\$i, a, "=")
            stolen = a[2]
        }
    }
    if (name == "gem" && mmapping == "1" && stolen == "0" && !before[id]) {
        print id
        exit
    }
}
' "\${tmpdir}/before.txt" "\${tmpdir}/during.txt")

if [[ -z "\${range_id}" ]]; then
    range_id=\$(awk '
/^range id=/ {
    id = name = mmapping = stolen = ""
    for (i = 1; i <= NF; i++) {
        if (\$i ~ /^id=/) {
            split(\$i, a, "=")
            id = a[2]
        } else if (\$i ~ /^name=/) {
            split(\$i, a, "=")
            name = a[2]
        } else if (\$i ~ /^mmapping=/) {
            split(\$i, a, "=")
            mmapping = a[2]
        } else if (\$i ~ /^stolen=/) {
            split(\$i, a, "=")
            stolen = a[2]
        }
    }
    if (name == "gem" && mmapping == "1" && stolen == "0")
        last = id
}
END {
    if (last != "")
        print last
}
' "\${tmpdir}/during.txt")
fi

if [[ -z "\${range_id}" ]]; then
    echo "Failed to identify a live GEM range for PF bind" >&2
    echo "DRM_PROBE_HOLD_LOG"
    cat "\${tmpdir}/drm_probe_hold.log"
    exit 1
fi

echo "REMOTE_BIN=${target_bin}"
echo "BIND_NODE=\${bind_node}"
echo "HOLD_SECONDS=${hold_seconds}"
echo "RANGE_ID=\${range_id}"
echo "GTT_STATE_BEFORE"
cat "\${tmpdir}/before.txt"
echo "GTT_STATE_DURING"
cat "\${tmpdir}/during.txt"
echo "PF_BIND"
echo "pf_bind \${range_id}" > "\${bind_node}"
echo "GTT_STATE_AFTER_BIND"
cat "\${gtt_node}"
echo "PF_UNBIND"
echo "pf_unbind \${range_id}" > "\${bind_node}"
echo "GTT_STATE_AFTER_UNBIND"
cat "\${gtt_node}"

wait "\${probe_pid}"
probe_pid=""

echo "GTT_STATE_AFTER_PROBE_EXIT"
cat "\${gtt_node}"
echo "DRM_PROBE_HOLD_LOG"
cat "\${tmpdir}/drm_probe_hold.log"
EOF
