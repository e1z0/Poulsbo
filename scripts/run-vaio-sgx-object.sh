#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
object_driver_mode="${SGX_OBJECT_DRIVER_MODE:-typed}"

case "${object_driver_mode}" in
    typed)
        exec "${script_dir}/run-vaio-sgx-object-typed.sh"
        ;;
    legacy)
        ;;
    *)
        echo "SGX_OBJECT_DRIVER_MODE must be 'typed' or 'legacy'" >&2
        exit 1
        ;;
esac

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_dir="${TARGET_DIR:-/root/poulsbo-workloads}"
target_bin="${TARGET_BIN:-${target_dir}/drm_probe}"
target_stage="${TARGET_STAGE:-/root/drm_probe.new}"
local_bin="${LOCAL_BIN:-$(pwd)/workloads/drm_probe}"
target_node="${TARGET_NODE:-/dev/dri/card0}"
hold_seconds="${HOLD_SECONDS:-15}"
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

object_node="${sysfs_dir}/poulsbo_sgx_object"
objects_node="${sysfs_dir}/poulsbo_sgx_objects"
gtt_node="${sysfs_dir}/poulsbo_gtt_state"
tmpdir=\$(mktemp -d)
probe_pid=""
object_id=""
range_id=""

cleanup() {
    if [[ -n "\${probe_pid}" ]]; then
        kill "\${probe_pid}" 2>/dev/null || true
        wait "\${probe_pid}" 2>/dev/null || true
    fi
    rm -rf "\${tmpdir}"
}
trap cleanup EXIT

for node in "\${object_node}" "\${objects_node}" "\${gtt_node}"; do
    if [[ ! -e "\${node}" ]]; then
        echo "Missing sysfs node: \${node}" >&2
        exit 1
    fi
done

cat "\${gtt_node}" > "\${tmpdir}/before_gtt.txt"
cat "\${objects_node}" > "\${tmpdir}/before_objects.txt"

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

cat "\${gtt_node}" > "\${tmpdir}/during_gtt.txt"
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
    id = name = mmapping = stolen = sgx_object_id = ""
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
        } else if (\$i ~ /^sgx_object_id=/) {
            split(\$i, a, "=")
            sgx_object_id = a[2]
        }
    }
    if (name == "gem" && mmapping == "1" && stolen == "0" &&
        sgx_object_id == "0" && !before[id]) {
        print id
        exit
    }
}
' "\${tmpdir}/before_gtt.txt" "\${tmpdir}/during_gtt.txt")

if [[ -z "\${range_id}" ]]; then
    echo "Failed to identify a live GEM range to adopt" >&2
    echo "DRM_PROBE_HOLD_LOG"
    cat "\${tmpdir}/drm_probe_hold.log"
    exit 1
fi

echo "adopt \${range_id}" > "\${object_node}"
cat "\${objects_node}" > "\${tmpdir}/after_adopt_objects.txt"
object_id=\$(awk -v want_range="\${range_id}" '
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
    if (range_id == want_range) {
        print object_id
        exit
    }
}
' "\${tmpdir}/after_adopt_objects.txt")

if [[ -z "\${object_id}" ]]; then
    echo "Failed to identify adopted SGX object for range \${range_id}" >&2
    cat "\${tmpdir}/after_adopt_objects.txt"
    exit 1
fi

echo "pf_bind \${object_id}" > "\${object_node}"
cat "\${gtt_node}" > "\${tmpdir}/after_bind_gtt.txt"
cat "\${objects_node}" > "\${tmpdir}/after_bind_objects.txt"

echo "pf_unbind \${object_id}" > "\${object_node}"
cat "\${gtt_node}" > "\${tmpdir}/after_unbind_gtt.txt"
cat "\${objects_node}" > "\${tmpdir}/after_unbind_objects.txt"

wait "\${probe_pid}"
probe_pid=""

cat "\${gtt_node}" > "\${tmpdir}/after_probe_exit_gtt.txt"
cat "\${objects_node}" > "\${tmpdir}/after_probe_exit_objects.txt"

echo "drop \${object_id}" > "\${object_node}"
cat "\${gtt_node}" > "\${tmpdir}/after_drop_gtt.txt"
cat "\${objects_node}" > "\${tmpdir}/after_drop_objects.txt"

echo "REMOTE_BIN=${target_bin}"
echo "OBJECT_NODE=\${object_node}"
echo "OBJECTS_NODE=\${objects_node}"
echo "HOLD_SECONDS=${hold_seconds}"
echo "RANGE_ID=\${range_id}"
echo "OBJECT_ID=\${object_id}"
echo "GTT_STATE_BEFORE"
cat "\${tmpdir}/before_gtt.txt"
echo "OBJECTS_BEFORE"
cat "\${tmpdir}/before_objects.txt"
echo "GTT_STATE_DURING"
cat "\${tmpdir}/during_gtt.txt"
echo "OBJECTS_AFTER_ADOPT"
cat "\${tmpdir}/after_adopt_objects.txt"
echo "GTT_STATE_AFTER_BIND"
cat "\${tmpdir}/after_bind_gtt.txt"
echo "OBJECTS_AFTER_BIND"
cat "\${tmpdir}/after_bind_objects.txt"
echo "GTT_STATE_AFTER_UNBIND"
cat "\${tmpdir}/after_unbind_gtt.txt"
echo "OBJECTS_AFTER_UNBIND"
cat "\${tmpdir}/after_unbind_objects.txt"
echo "GTT_STATE_AFTER_PROBE_EXIT"
cat "\${tmpdir}/after_probe_exit_gtt.txt"
echo "OBJECTS_AFTER_PROBE_EXIT"
cat "\${tmpdir}/after_probe_exit_objects.txt"
echo "GTT_STATE_AFTER_DROP"
cat "\${tmpdir}/after_drop_gtt.txt"
echo "OBJECTS_AFTER_DROP"
cat "\${tmpdir}/after_drop_objects.txt"
echo "DRM_PROBE_HOLD_LOG"
cat "\${tmpdir}/drm_probe_hold.log"
echo "DMESG_TAIL"
dmesg | egrep 'lab object|lab pf_|gma500' | tail -n 80 || true
EOF
