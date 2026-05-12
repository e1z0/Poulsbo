#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
target_kernel="${TARGET_KERNEL:-5.4.0-150-generic}"
reboot_after_deploy="${REBOOT_AFTER_DEPLOY:-1}"
post_reboot_timeout_secs="${POST_REBOOT_TIMEOUT_SECS:-180}"
post_reboot_poll_secs="${POST_REBOOT_POLL_SECS:-5}"
health_timeout_secs="${HEALTH_TIMEOUT_SECS:-15}"
auto_rollback_guard="${AUTO_ROLLBACK_GUARD:-1}"
rollback_grace_secs="${ROLLBACK_GRACE_SECS:-180}"
disarm_rollback_guard_on_success="${DISARM_ROLLBACK_GUARD_ON_SUCCESS:-1}"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
health_script="${HEALTH_SCRIPT:-${repo_root}/scripts/check-vaio-target-health.sh}"
deploy_script="${DEPLOY_SCRIPT:-${repo_root}/scripts/deploy-vaio-gma500.sh}"
rollback_arm_script="${ROLLBACK_ARM_SCRIPT:-${repo_root}/scripts/arm-vaio-gma500-rollback.sh}"
rollback_disarm_script="${ROLLBACK_DISARM_SCRIPT:-${repo_root}/scripts/disarm-vaio-gma500-rollback.sh}"
show_state_script="${SHOW_STATE_SCRIPT:-${repo_root}/scripts/show-vaio-gma500-state.sh}"

if [[ ! -x "$health_script" ]]; then
    echo "Health script not found or not executable: $health_script" >&2
    exit 1
fi

if [[ ! -x "$deploy_script" ]]; then
    echo "Deploy script not found or not executable: $deploy_script" >&2
    exit 1
fi

if [[ "$auto_rollback_guard" == "1" ]]; then
    if [[ ! -x "$rollback_arm_script" ]]; then
        echo "Rollback arm script not found or not executable: $rollback_arm_script" >&2
        exit 1
    fi
    if [[ ! -x "$rollback_disarm_script" ]]; then
        echo "Rollback disarm script not found or not executable: $rollback_disarm_script" >&2
        exit 1
    fi
fi

if [[ ! -x "$show_state_script" ]]; then
    echo "State inspection script not found or not executable: $show_state_script" >&2
    exit 1
fi

tmpdir="$(mktemp -d)"
cleanup() {
    rm -rf "$tmpdir"
}
trap cleanup EXIT

health_run() {
    local label="$1"
    local output_file="$tmpdir/${label}.txt"

    TARGET_HOST="$target_host" \
    TARGET_USER="$target_user" \
    TIMEOUT_SECS="$health_timeout_secs" \
    "$health_script" | tee "$output_file"
}

health_value() {
    local file="$1"
    local key="$2"

    awk -F= -v key="$key" '$1 == key { print $2 }' "$file" | tail -n 1
}

preflight_file="$tmpdir/preflight.txt"
health_run preflight >/dev/null
preflight_overall="$(health_value "$preflight_file" overall)"

echo "preflight_overall=${preflight_overall}"

if [[ "$preflight_overall" != "remote_usable" ]]; then
    echo "Refusing to deploy: target is not remotely usable." >&2
    echo "See docs/25-vaio-ssh-stall-recovery.md for console recovery." >&2
    exit 2
fi

echo "REMOTE_MODULE_BEFORE"
timeout "$health_timeout_secs" ssh \
    -o BatchMode=yes \
    -o ConnectTimeout=5 \
    "${target_user}@${target_host}" \
    "modinfo -n gma500_gfx; modinfo -F srcversion gma500_gfx || true"

if [[ "$auto_rollback_guard" == "1" ]]; then
    echo "Arming rollback guard on ${target_user}@${target_host}"
    TARGET_HOST="$target_host" \
    TARGET_USER="$target_user" \
    TARGET_KERNEL="$target_kernel" \
    ROLLBACK_GRACE_SECS="$rollback_grace_secs" \
    "$rollback_arm_script"
fi

"$deploy_script"

if [[ "$reboot_after_deploy" != "1" ]]; then
    echo "reboot_after_deploy=0"
    echo "Deployment complete without reboot."
    exit 0
fi

echo "Issuing reboot on ${target_user}@${target_host}"
set +e
timeout "$health_timeout_secs" ssh \
    -o BatchMode=yes \
    -o ConnectTimeout=5 \
    "${target_user}@${target_host}" "reboot" >/dev/null 2>&1
set -e

deadline=$(( $(date +%s) + post_reboot_timeout_secs ))
effective_post_reboot_timeout_secs="$post_reboot_timeout_secs"

if [[ "$auto_rollback_guard" == "1" ]]; then
    min_timeout_secs=$(( rollback_grace_secs + 180 ))
    if (( effective_post_reboot_timeout_secs < min_timeout_secs )); then
        effective_post_reboot_timeout_secs="$min_timeout_secs"
    fi
fi

echo "effective_post_reboot_timeout_secs=${effective_post_reboot_timeout_secs}"
deadline=$(( $(date +%s) + effective_post_reboot_timeout_secs ))
last_overall="unknown"

while (( $(date +%s) < deadline )); do
    if TARGET_HOST="$target_host" \
       TARGET_USER="$target_user" \
       TIMEOUT_SECS="$health_timeout_secs" \
       "$health_script" >"$tmpdir/post.txt"; then
        cat "$tmpdir/post.txt"
        last_overall="$(health_value "$tmpdir/post.txt" overall)"
        if [[ "$last_overall" == "remote_usable" ]]; then
            if [[ "$auto_rollback_guard" == "1" && "$disarm_rollback_guard_on_success" == "1" ]]; then
                echo "Clearing rollback guard on ${target_user}@${target_host}"
                TARGET_HOST="$target_host" \
                TARGET_USER="$target_user" \
                "$rollback_disarm_script"
            elif [[ "$auto_rollback_guard" == "1" ]]; then
                echo "Leaving rollback guard armed on ${target_user}@${target_host}"
            fi
            echo "REMOTE_MODULE_AFTER"
            TARGET_HOST="$target_host" \
            TARGET_USER="$target_user" \
            TARGET_KERNEL="$target_kernel" \
            "$show_state_script"
            echo "Post-reboot target is remotely usable again."
            exit 0
        fi
        if [[ "$last_overall" == "ssh_stalled_post_kex" &&
              "$auto_rollback_guard" != "1" ]]; then
            echo "Post-reboot target reached ssh_stalled_post_kex." >&2
            echo "See docs/25-vaio-ssh-stall-recovery.md for console recovery." >&2
            exit 3
        fi
    fi
    sleep "$post_reboot_poll_secs"
done

echo "Timed out waiting for remote usability after reboot." >&2
echo "last_overall=${last_overall}" >&2
if [[ "$auto_rollback_guard" == "1" ]]; then
    echo "Rollback guard remains armed; if the target finishes booting far enough to run systemd, it should remove the override module after ${rollback_grace_secs} seconds and reboot." >&2
fi
echo "See docs/25-vaio-ssh-stall-recovery.md for console recovery." >&2
exit 4
