#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

session_name="${SESSION_NAME:-autorestore}"
session_kind="rollback_autorestore"
logs_root="${LOGS_ROOT:-${repo_root}/logs/vaio-rollback}"
session_timestamp="${SESSION_TIMESTAMP:-$(date +%Y%m%d-%H%M%S)}"
started_at="$(date -Iseconds)"
session_dir="$("${script_dir}/allocate-session-dir.sh" "$logs_root" "$session_name" "$session_timestamp")"
manifest_file="${session_dir}/session.env"
result_file="${session_dir}/session-result.env"
preflight_health_log="${session_dir}/preflight-health.log"
pre_state_log="${session_dir}/pre-state.log"
phase1_deploy_log="${session_dir}/phase1-guarded-deploy.log"
mid_state_log="${session_dir}/mid-state.log"
rollback_watch_log="${session_dir}/rollback-watch.log"
post_state_log="${session_dir}/post-state.log"
candidate_module="${session_dir}/gma500_gfx.rollback-candidate.ko"

health_script="${HEALTH_SCRIPT:-${script_dir}/check-vaio-target-health.sh}"
watch_script="${WATCH_SCRIPT:-${script_dir}/watch-vaio-target-health.sh}"
state_script="${STATE_SCRIPT:-${script_dir}/show-vaio-gma500-state.sh}"
deploy_script="${DEPLOY_SCRIPT:-${script_dir}/deploy-vaio-gma500-guarded.sh}"
source_module="${SOURCE_MODULE:-${repo_root}/build/devnull-vaio-gma500-src/gma500_gfx.ko}"
rollback_grace_secs="${ROLLBACK_GRACE_SECS:-60}"
post_reboot_timeout_secs="${POST_REBOOT_TIMEOUT_SECS:-300}"
poll_interval_secs="${POLL_INTERVAL_SECS:-5}"
phase1_health_timeout_secs="${PHASE1_HEALTH_TIMEOUT_SECS:-15}"

usage() {
    cat <<'EOF'
Usage:
  run-vaio-rollback-autorestore-session.sh

Behavior:
  Creates a stripped candidate copy of the current module, performs a guarded
  deploy while intentionally leaving the rollback guard armed after the first
  healthy boot, waits for the automatic rollback reboot, and then captures the
  final restored target state.

Environment:
  SESSION_NAME               Session suffix. Default: autorestore
  LOGS_ROOT                  Base directory for session logs.
  SOURCE_MODULE              Source module to copy and strip.
  ROLLBACK_GRACE_SECS        Seconds before the target-side rollback fires.
                             Default: 60
  POST_REBOOT_TIMEOUT_SECS   Total timeout for the guarded deploy phase.
                             Default: 300
  POLL_INTERVAL_SECS         Seconds between health polls during rollback watch.
                             Default: 5
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

for required in "$health_script" "$watch_script" "$state_script" "$deploy_script"; do
    if [[ ! -x "$required" ]]; then
        echo "required executable missing: $required" >&2
        exit 1
    fi
done

if [[ ! -f "$source_module" ]]; then
    echo "source module not found: $source_module" >&2
    exit 1
fi

cp "$source_module" "$candidate_module"
strip --strip-debug "$candidate_module"

source_size="$(stat -c %s "$source_module")"
candidate_size="$(stat -c %s "$candidate_module")"
source_srcversion="$(modinfo -F srcversion "$source_module" 2>/dev/null || echo unknown)"
candidate_srcversion="$(modinfo -F srcversion "$candidate_module" 2>/dev/null || echo unknown)"

if [[ "$source_size" == "$candidate_size" ]]; then
    echo "candidate strip did not change module size; refusing autorestore test" >&2
    exit 1
fi

{
    echo "timestamp=${session_timestamp}"
    echo "started_at=${started_at}"
    echo "session_kind=${session_kind}"
    echo "session_name=${session_name}"
    echo "session_dir=${session_dir}"
    echo "source_module=${source_module}"
    echo "source_module_srcversion=${source_srcversion}"
    echo "source_module_size=${source_size}"
    echo "candidate_module=${candidate_module}"
    echo "candidate_module_srcversion=${candidate_srcversion}"
    echo "candidate_module_size=${candidate_size}"
    echo "rollback_grace_secs=${rollback_grace_secs}"
    echo "post_reboot_timeout_secs=${post_reboot_timeout_secs}"
    echo "poll_interval_secs=${poll_interval_secs}"
} >"$manifest_file"

echo "session_dir=${session_dir}"
echo "candidate_module=${candidate_module}"
echo "candidate_module_size=${candidate_size}"
echo "source_module_size=${source_size}"

preflight_overall="unknown"
pre_state_rc="unknown"
phase1_deploy_rc="unknown"
mid_state_rc="unknown"
rollback_watch_rc="unknown"
post_state_rc="unknown"
final_override_size="unknown"
final_pending_present="unknown"
final_backup_present="unknown"
final_driver_srcversion="unknown"
session_outcome="interrupted"

run_and_capture() {
    local __rc_var="$1"
    local outfile="$2"
    local rc
    shift 2

    set +e
    "$@" >"$outfile" 2>&1
    rc=$?
    set -e

    cat "$outfile"
    printf -v "$__rc_var" '%s' "$rc"
}

write_fallback_result() {
    local rc="$1"
    if [[ -f "$result_file" ]]; then
        return 0
    fi

    {
        echo "session_kind=${session_kind}"
        echo "session_outcome=${session_outcome}"
        echo "preflight_overall=${preflight_overall}"
        echo "pre_state_rc=${pre_state_rc}"
        echo "phase1_deploy_rc=${phase1_deploy_rc}"
        echo "mid_state_rc=${mid_state_rc}"
        echo "rollback_watch_rc=${rollback_watch_rc}"
        echo "post_state_rc=${post_state_rc}"
        echo "source_module_size=${source_size}"
        echo "candidate_module_size=${candidate_size}"
        echo "final_override_size=${final_override_size}"
        echo "final_pending_present=${final_pending_present}"
        echo "final_backup_present=${final_backup_present}"
        echo "final_driver_srcversion=${final_driver_srcversion}"
        echo "fallback_exit_rc=${rc}"
        echo "finished_at=$(date -Iseconds)"
    } >"$result_file"
}

on_exit() {
    local rc=$?
    write_fallback_result "$rc"
    return 0
}

trap on_exit EXIT

run_and_capture preflight_rc "$preflight_health_log" "$health_script"
preflight_overall="$(awk -F= '$1 == "overall" { print $2 }' "$preflight_health_log" | tail -n 1)"

if [[ "$preflight_overall" != "remote_usable" ]]; then
    printf '%s\n' \
        "skipped: pre-deploy state not captured because preflight overall=${preflight_overall}" \
        >"$pre_state_log"
    printf '%s\n' \
        "skipped: phase1 deploy not attempted because preflight overall=${preflight_overall}" \
        >"$phase1_deploy_log"
    printf '%s\n' \
        "skipped: mid-state not captured because phase1 deploy was not attempted" \
        >"$mid_state_log"
    printf '%s\n' \
        "skipped: rollback watch not run because phase1 deploy was not attempted" \
        >"$rollback_watch_log"
    printf '%s\n' \
        "skipped: post-state not captured because rollback watch was not run" \
        >"$post_state_log"
    {
        echo "session_kind=${session_kind}"
        echo "session_outcome=blocked_preflight"
        echo "preflight_overall=${preflight_overall}"
        echo "finished_at=$(date -Iseconds)"
    } >"$result_file"
    exit 2
fi

run_and_capture pre_state_rc "$pre_state_log" "$state_script"
run_and_capture phase1_deploy_rc "$phase1_deploy_log" env \
    LOCAL_MODULE="$candidate_module" \
    AUTO_ROLLBACK_GUARD=1 \
    DISARM_ROLLBACK_GUARD_ON_SUCCESS=0 \
    ROLLBACK_GRACE_SECS="$rollback_grace_secs" \
    POST_REBOOT_TIMEOUT_SECS="$post_reboot_timeout_secs" \
    HEALTH_TIMEOUT_SECS="$phase1_health_timeout_secs" \
    "$deploy_script"

if [[ "$phase1_deploy_rc" -ne 0 ]]; then
    printf '%s\n' \
        "skipped: mid-state not captured because phase1 deploy exited with rc=${phase1_deploy_rc}" \
        >"$mid_state_log"
    printf '%s\n' \
        "skipped: rollback watch not run because phase1 deploy exited with rc=${phase1_deploy_rc}" \
        >"$rollback_watch_log"
    printf '%s\n' \
        "skipped: post-state not captured because rollback watch was not run" \
        >"$post_state_log"
    {
        echo "session_kind=${session_kind}"
        echo "session_outcome=phase1_deploy_failed"
        echo "preflight_overall=${preflight_overall}"
        echo "pre_state_rc=${pre_state_rc}"
        echo "phase1_deploy_rc=${phase1_deploy_rc}"
        echo "finished_at=$(date -Iseconds)"
    } >"$result_file"
    exit "$phase1_deploy_rc"
fi

run_and_capture mid_state_rc "$mid_state_log" "$state_script"

deadline=$(( $(date +%s) + rollback_grace_secs + post_reboot_timeout_secs ))
seen_post_remote=0
seen_drop_after_remote=0
rollback_watch_rc=1

while (( $(date +%s) < deadline )); do
    health_output="$("$health_script")"
    timestamp_line="$(date -Iseconds)"
    printf 'timestamp=%s\n%s\n' "$timestamp_line" "$health_output" >>"$rollback_watch_log"
    printf 'timestamp=%s\n%s\n' "$timestamp_line" "$health_output"

    overall="$(printf '%s\n' "$health_output" | awk -F= '$1 == "overall" { print $2 }')"

    if [[ "$overall" == "remote_usable" ]]; then
        if [[ "$seen_drop_after_remote" == "1" ]]; then
            rollback_watch_rc=0
            break
        fi
        seen_post_remote=1
    else
        if [[ "$seen_post_remote" == "1" ]]; then
            seen_drop_after_remote=1
        fi
    fi

    sleep "$poll_interval_secs"
done

run_and_capture post_state_rc "$post_state_log" "$state_script"

final_override_size="$(awk -F= '$1 == "override_module_size" { print $2 }' "$post_state_log" | tail -n 1)"
final_pending_present="$(awk -F= '$1 == "rollback_pending_present" { print $2 }' "$post_state_log" | tail -n 1)"
final_backup_present="$(awk -F= '$1 == "rollback_backup_present" { print $2 }' "$post_state_log" | tail -n 1)"
final_driver_srcversion="$(awk -F= '$1 == "driver_srcversion" { print $2 }' "$post_state_log" | tail -n 1)"

session_outcome="completed"
if [[ "$pre_state_rc" -ne 0 ]]; then
    session_outcome="pre_state_capture_failed"
fi
if [[ "$mid_state_rc" -ne 0 ]]; then
    session_outcome="mid_state_capture_failed"
fi
if [[ "$rollback_watch_rc" -ne 0 ]]; then
    session_outcome="rollback_watch_failed"
fi
if [[ "$post_state_rc" -ne 0 ]]; then
    session_outcome="post_state_capture_failed"
fi
if [[ "$rollback_watch_rc" -eq 0 ]]; then
    if [[ "$final_override_size" != "$source_size" ]]; then
        session_outcome="restore_size_mismatch"
    fi
    if [[ "$final_pending_present" != "0" ]]; then
        session_outcome="rollback_pending_not_cleared"
    fi
fi

{
    echo "session_kind=${session_kind}"
    echo "session_outcome=${session_outcome}"
    echo "preflight_overall=${preflight_overall}"
    echo "pre_state_rc=${pre_state_rc}"
    echo "phase1_deploy_rc=${phase1_deploy_rc}"
    echo "mid_state_rc=${mid_state_rc}"
    echo "rollback_watch_rc=${rollback_watch_rc}"
    echo "post_state_rc=${post_state_rc}"
    echo "source_module_size=${source_size}"
    echo "candidate_module_size=${candidate_size}"
    echo "final_override_size=${final_override_size:-unknown}"
    echo "final_pending_present=${final_pending_present:-unknown}"
    echo "final_backup_present=${final_backup_present:-unknown}"
    echo "final_driver_srcversion=${final_driver_srcversion:-unknown}"
    echo "finished_at=$(date -Iseconds)"
} >"$result_file"

if [[ "$pre_state_rc" -ne 0 ]]; then
    exit "$pre_state_rc"
fi
if [[ "$phase1_deploy_rc" -ne 0 ]]; then
    exit "$phase1_deploy_rc"
fi
if [[ "$mid_state_rc" -ne 0 ]]; then
    exit "$mid_state_rc"
fi
if [[ "$rollback_watch_rc" -ne 0 ]]; then
    exit "$rollback_watch_rc"
fi
if [[ "$post_state_rc" -ne 0 ]]; then
    exit "$post_state_rc"
fi
if [[ "$session_outcome" != "completed" ]]; then
    exit 1
fi
