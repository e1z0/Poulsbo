#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"

cmd1="${1:-clear 0}"
cmd2="${2:-fill 32 32 200 96 0xffff copy}"
cmd3="${3:-copy 32 32 320 160 200 96}"
cmd4="${4:-mono 0 0 1600 16 0xffffff 0 checker}"
marker="poulsbo-lab-start-$$-$(date +%s)"

ssh "${target_user}@${target_host}" 'bash -s' <<EOF
set -euo pipefail
attr=\$(find /sys -name poulsbo_lab_2d -print -quit)
state_attr=\$(find /sys -name poulsbo_sgx_state -print -quit)
loaded=\$(modinfo -n gma500_gfx)

printf 'ATTR=%s\n' "\$attr"
printf 'STATE_ATTR=%s\n' "\$state_attr"
printf 'LOADED=%s\n' "\$loaded"
printf 'SRCVERSION=%s\n' "\$(modinfo -F srcversion "\$loaded")"
printf 'STATUS\n'
cat "\$attr"
printf 'SGX_STATE_BEFORE\n'
cat "\$state_attr"

printf '<6>%s\n' '${marker}' > /dev/kmsg
echo '${cmd1}' > "\$attr"
echo '${cmd2}' > "\$attr"
echo '${cmd3}' > "\$attr"
echo '${cmd4}' > "\$attr"
sleep 1
printf 'SGX_STATE_AFTER\n'
cat "\$state_attr"

printf 'LOGS\n'
dmesg | awk '/${marker}/{flag=1;next} flag && /lab_2d|2D fillrect|2D copyarea|2D imageblit|SGX irq/'
EOF
