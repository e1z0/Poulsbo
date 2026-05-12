#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

session_name="${SESSION_NAME:-guarded-deploy}"
session_kind="guarded_deploy"
logs_root="${LOGS_ROOT:-${repo_root}/logs/vaio-deploy}"
session_timestamp="${SESSION_TIMESTAMP:-$(date +%Y%m%d-%H%M%S)}"
started_at="$(date -Iseconds)"
session_dir="$("${script_dir}/allocate-session-dir.sh" "$logs_root" "$session_name" "$session_timestamp")"
preflight_health_log="${session_dir}/preflight-health.log"
pre_state_log="${session_dir}/pre-state.log"
deploy_log="${session_dir}/guarded-deploy.log"
post_state_log="${session_dir}/post-state.log"
manifest_file="${session_dir}/session.env"
result_file="${session_dir}/session-result.env"
health_script="${HEALTH_SCRIPT:-${script_dir}/check-vaio-target-health.sh}"
state_script="${STATE_SCRIPT:-${script_dir}/show-vaio-gma500-state.sh}"
deploy_script="${DEPLOY_SCRIPT:-${script_dir}/deploy-vaio-gma500-guarded.sh}"
local_module_path="${LOCAL_MODULE:-${repo_root}/build/devnull-vaio-gma500-src/gma500_gfx.ko}"
auto_rollback_guard="${AUTO_ROLLBACK_GUARD:-1}"
rollback_grace_secs="${ROLLBACK_GRACE_SECS:-180}"

usage() {
    cat <<'EOF'
Usage:
  run-vaio-guarded-deploy-session.sh

Behavior:
  Creates a timestamped local deploy-session directory, records preflight
  target health, captures pre-deploy target state when usable, runs the guarded
  deploy flow, and captures post-deploy target state on success.

Environment:
  SESSION_NAME          Session suffix. Default: guarded-deploy
  LOGS_ROOT             Base directory for session logs.
  HEALTH_SCRIPT         Override health probe path.
  STATE_SCRIPT          Override remote-state capture path.
  DEPLOY_SCRIPT         Override guarded deploy path.
  LOCAL_MODULE          Override local module path recorded in the manifest.
  AUTO_ROLLBACK_GUARD   Forwarded policy note for this session. Default: 1
  ROLLBACK_GRACE_SECS   Forwarded policy note for this session. Default: 180
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if [[ ! -x "$health_script" ]]; then
    echo "health script is not executable: $health_script" >&2
    exit 1
fi

if [[ ! -x "$state_script" ]]; then
    echo "state script is not executable: $state_script" >&2
    exit 1
fi

if [[ ! -x "$deploy_script" ]]; then
    echo "deploy script is not executable: $deploy_script" >&2
    exit 1
fi

{
    echo "timestamp=${session_timestamp}"
    echo "started_at=${started_at}"
    echo "session_kind=${session_kind}"
    echo "session_name=${session_name}"
    echo "session_dir=${session_dir}"
    echo "health_script=${health_script}"
    echo "state_script=${state_script}"
    echo "deploy_script=${deploy_script}"
    echo "local_module_path=${local_module_path}"
    if [[ -f "$local_module_path" ]]; then
        echo "local_module_srcversion=$(modinfo -F srcversion "$local_module_path" 2>/dev/null || echo unknown)"
    else
        echo "local_module_srcversion=missing"
    fi
    echo "auto_rollback_guard=${auto_rollback_guard}"
    echo "rollback_grace_secs=${rollback_grace_secs}"
} >"$manifest_file"

echo "session_dir=${session_dir}"
echo "preflight_health_log=${preflight_health_log}"
echo "pre_state_log=${pre_state_log}"
echo "deploy_log=${deploy_log}"
echo "post_state_log=${post_state_log}"
echo "result_file=${result_file}"

preflight_overall="unknown"
pre_state_rc="unknown"
deploy_rc="unknown"
post_state_rc="skipped"
post_state_overall=""
post_state_driver_srcversion=""
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
        echo "last_overall=${post_state_overall:-${preflight_overall}}"
        echo "final_driver_srcversion=${post_state_driver_srcversion:-unknown}"
        echo "pre_state_rc=${pre_state_rc}"
        echo "deploy_rc=${deploy_rc}"
        echo "post_state_rc=${post_state_rc}"
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

run_and_capture preflight_health_rc "$preflight_health_log" "$health_script"
preflight_overall="$(awk -F= '$1 == "overall" { print $2 }' "$preflight_health_log" | tail -n 1)"

if [[ "$preflight_overall" != "remote_usable" ]]; then
    printf '%s\n' \
        "skipped: pre-deploy state not captured because preflight overall=${preflight_overall}" \
        >"$pre_state_log"
    printf '%s\n' \
        "skipped: guarded deploy not attempted because preflight overall=${preflight_overall}" \
        >"$deploy_log"
    printf '%s\n' \
        "skipped: post-deploy state not captured because guarded deploy was not attempted" \
        >"$post_state_log"
    {
        echo "session_kind=${session_kind}"
        echo "session_outcome=blocked_preflight"
        echo "preflight_overall=${preflight_overall}"
        echo "last_overall=${preflight_overall}"
        echo "final_driver_srcversion=unknown"
        echo "pre_state_rc=skipped"
        echo "deploy_rc=skipped"
        echo "post_state_rc=skipped"
        echo "finished_at=$(date -Iseconds)"
    } >"$result_file"
    exit 2
fi

run_and_capture pre_state_rc "$pre_state_log" "$state_script"

run_and_capture deploy_rc "$deploy_log" "$deploy_script"

session_outcome="completed"

if [[ "$pre_state_rc" -ne 0 ]]; then
    session_outcome="pre_state_capture_failed"
fi

if [[ "$deploy_rc" -ne 0 ]]; then
    printf '%s\n' \
        "skipped: post-deploy state not captured because guarded deploy exited with rc=${deploy_rc}" \
        >"$post_state_log"
    session_outcome="deploy_failed"
else
    run_and_capture post_state_rc "$post_state_log" "$state_script"
    if [[ "$post_state_rc" -eq 0 ]]; then
        post_state_overall="$(awk -F= '$1 == "overall" { value = $2 } END { print value }' "$post_state_log")"
        post_state_driver_srcversion="$(awk -F= '$1 == "driver_srcversion" { value = $2 } END { print value }' "$post_state_log")"
    fi
    if [[ "$post_state_rc" -ne 0 ]]; then
        session_outcome="post_state_capture_failed"
    fi
fi

{
    echo "session_kind=${session_kind}"
    echo "session_outcome=${session_outcome}"
    echo "preflight_overall=${preflight_overall}"
    echo "last_overall=${post_state_overall:-${preflight_overall}}"
    echo "final_driver_srcversion=${post_state_driver_srcversion:-unknown}"
    echo "pre_state_rc=${pre_state_rc}"
    echo "deploy_rc=${deploy_rc}"
    echo "post_state_rc=${post_state_rc}"
    echo "finished_at=$(date -Iseconds)"
} >"$result_file"

if [[ "$pre_state_rc" -ne 0 ]]; then
    exit "$pre_state_rc"
fi

if [[ "$deploy_rc" -ne 0 ]]; then
    exit "$deploy_rc"
fi

if [[ "$post_state_rc" != "skipped" && "$post_state_rc" -ne 0 ]]; then
    exit "$post_state_rc"
fi
