#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

session_name="${SESSION_NAME:-panic-capture-setup}"
session_kind="panic_capture_setup"
logs_root="${LOGS_ROOT:-${repo_root}/logs/vaio-panic}"
session_timestamp="${SESSION_TIMESTAMP:-$(date +%Y%m%d-%H%M%S)}"
started_at="$(date -Iseconds)"
session_dir="$("${script_dir}/allocate-session-dir.sh" "$logs_root" "$session_name" "$session_timestamp")"
preflight_health_log="${session_dir}/preflight-health.log"
pre_state_log="${session_dir}/pre-state.log"
setup_log="${session_dir}/panic-capture-setup.log"
reboot_health_log="${session_dir}/reboot-health.log"
post_state_log="${session_dir}/post-state.log"
artifact_log="${session_dir}/panic-artifacts.log"
artifact_dir="${session_dir}/panic-artifacts"
manifest_file="${session_dir}/session.env"
result_file="${session_dir}/session-result.env"

health_script="${HEALTH_SCRIPT:-${script_dir}/check-vaio-target-health.sh}"
state_script="${STATE_SCRIPT:-${script_dir}/show-vaio-gma500-state.sh}"
setup_script="${SETUP_SCRIPT:-${script_dir}/configure-vaio-panic-capture.sh}"
fetch_script="${FETCH_SCRIPT:-${script_dir}/fetch-vaio-panic-artifacts.sh}"
watch_script="${WATCH_SCRIPT:-${script_dir}/watch-vaio-target-health.sh}"
target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
reboot_after_setup="${REBOOT_AFTER_SETUP:-1}"
post_reboot_timeout_secs="${POST_REBOOT_TIMEOUT_SECS:-300}"
post_reboot_poll_secs="${POST_REBOOT_POLL_SECS:-5}"
health_timeout_secs="${HEALTH_TIMEOUT_SECS:-15}"

usage() {
    cat <<'EOF'
Usage:
  run-vaio-panic-capture-setup-session.sh

Behavior:
  Creates a timestamped session bundle, captures current target state, applies
  panic reboot plus kdump configuration, reboots when required, then captures
  post-setup state and fetches panic-artifact metadata.

Environment:
  SESSION_NAME             Session suffix. Default: panic-capture-setup
  LOGS_ROOT                Base directory for session logs.
  HEALTH_SCRIPT            Override health probe path.
  STATE_SCRIPT             Override remote-state probe path.
  SETUP_SCRIPT             Override setup script path.
  FETCH_SCRIPT             Override artifact-fetch path.
  REBOOT_AFTER_SETUP       Reboot when the setup requires it. Default: 1
  POST_REBOOT_TIMEOUT_SECS Timeout waiting for remote usability. Default: 300
  POST_REBOOT_POLL_SECS    Seconds between post-reboot polls. Default: 5
  HEALTH_TIMEOUT_SECS      Health probe timeout. Default: 15
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

for required in "$health_script" "$state_script" "$setup_script" "$fetch_script" "$watch_script"; do
    if [[ ! -x "$required" ]]; then
        echo "required executable missing: $required" >&2
        exit 1
    fi
done

{
    echo "timestamp=${session_timestamp}"
    echo "started_at=${started_at}"
    echo "session_kind=${session_kind}"
    echo "session_name=${session_name}"
    echo "session_dir=${session_dir}"
    echo "health_script=${health_script}"
    echo "state_script=${state_script}"
    echo "setup_script=${setup_script}"
    echo "fetch_script=${fetch_script}"
    echo "target_host=${target_host}"
    echo "target_user=${target_user}"
    echo "reboot_after_setup=${reboot_after_setup}"
    echo "post_reboot_timeout_secs=${post_reboot_timeout_secs}"
    echo "post_reboot_poll_secs=${post_reboot_poll_secs}"
    echo "health_timeout_secs=${health_timeout_secs}"
} >"$manifest_file"

echo "session_dir=${session_dir}"
echo "preflight_health_log=${preflight_health_log}"
echo "pre_state_log=${pre_state_log}"
echo "setup_log=${setup_log}"
echo "reboot_health_log=${reboot_health_log}"
echo "post_state_log=${post_state_log}"
echo "artifact_log=${artifact_log}"
echo "result_file=${result_file}"

preflight_overall="unknown"
pre_state_rc="unknown"
setup_rc="unknown"
reboot_rc="skipped"
post_state_rc="skipped"
artifact_fetch_rc="skipped"
reboot_required="unknown"
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

health_value() {
    local file="$1"
    local key="$2"
    awk -F= -v want="$key" '$1 == want { value = substr($0, index($0, "=") + 1) } END { print value }' "$file"
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
        echo "setup_rc=${setup_rc}"
        echo "reboot_required=${reboot_required}"
        echo "reboot_rc=${reboot_rc}"
        echo "post_state_rc=${post_state_rc}"
        echo "artifact_fetch_rc=${artifact_fetch_rc}"
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
preflight_overall="$(health_value "$preflight_health_log" overall)"

if [[ "$preflight_overall" != "remote_usable" ]]; then
    printf '%s\n' \
        "skipped: pre-state not captured because preflight overall=${preflight_overall}" \
        >"$pre_state_log"
    printf '%s\n' \
        "skipped: setup not attempted because preflight overall=${preflight_overall}" \
        >"$setup_log"
    printf '%s\n' \
        "skipped: reboot watch not attempted because setup was not attempted" \
        >"$reboot_health_log"
    printf '%s\n' \
        "skipped: post-state not captured because setup was not attempted" \
        >"$post_state_log"
    printf '%s\n' \
        "skipped: panic artifacts not fetched because setup was not attempted" \
        >"$artifact_log"
    {
        echo "session_kind=${session_kind}"
        echo "session_outcome=blocked_preflight"
        echo "preflight_overall=${preflight_overall}"
        echo "last_overall=${preflight_overall}"
        echo "finished_at=$(date -Iseconds)"
    } >"$result_file"
    exit 2
fi

run_and_capture pre_state_rc "$pre_state_log" "$state_script"
run_and_capture setup_rc "$setup_log" "$setup_script"
reboot_required="$(health_value "$setup_log" reboot_required)"

session_outcome="completed"

if [[ "$setup_rc" -ne 0 ]]; then
    printf '%s\n' \
        "skipped: reboot watch not attempted because setup exited with rc=${setup_rc}" \
        >"$reboot_health_log"
    printf '%s\n' \
        "skipped: post-state not captured because setup exited with rc=${setup_rc}" \
        >"$post_state_log"
    printf '%s\n' \
        "skipped: panic artifacts not fetched because setup exited with rc=${setup_rc}" \
        >"$artifact_log"
    session_outcome="setup_failed"
else
    if [[ "$reboot_after_setup" == "1" && "$reboot_required" == "1" ]]; then
        set +e
        timeout "$health_timeout_secs" ssh \
            -o BatchMode=yes \
            -o ConnectTimeout=5 \
            "${target_user}@${target_host}" "reboot" >/dev/null 2>&1
        set -e

        deadline=$(( $(date +%s) + post_reboot_timeout_secs ))
        reboot_rc=1
        : >"$reboot_health_log"

        while (( $(date +%s) < deadline )); do
            health_output="$("$health_script")"
            timestamp_line="$(date -Iseconds)"
            printf 'timestamp=%s\n%s\n' "$timestamp_line" "$health_output" >>"$reboot_health_log"
            printf 'timestamp=%s\n%s\n' "$timestamp_line" "$health_output"

            overall="$(printf '%s\n' "$health_output" | awk -F= '$1 == "overall" { print $2 }')"
            if [[ "$overall" == "remote_usable" ]]; then
                reboot_rc=0
                break
            fi
            sleep "$post_reboot_poll_secs"
        done

        if [[ "$reboot_rc" -ne 0 ]]; then
            printf '%s\n' \
                "skipped: post-state not captured because target did not return after reboot" \
                >"$post_state_log"
            printf '%s\n' \
                "skipped: panic artifacts not fetched because target did not return after reboot" \
                >"$artifact_log"
            session_outcome="reboot_wait_failed"
        fi
    else
        printf '%s\n' "skipped: reboot not required by setup" >"$reboot_health_log"
        reboot_rc=skipped
    fi
fi

if [[ "$setup_rc" -eq 0 && "$session_outcome" == "completed" ]]; then
    run_and_capture post_state_rc "$post_state_log" "$state_script"
    if [[ "$post_state_rc" -eq 0 ]]; then
        post_state_overall="$(health_value "$post_state_log" overall)"
        post_state_driver_srcversion="$(health_value "$post_state_log" driver_srcversion)"
    fi
    if [[ "$post_state_rc" -ne 0 ]]; then
        session_outcome="post_state_capture_failed"
    fi
fi

if [[ "$setup_rc" -eq 0 && "$session_outcome" == "completed" ]]; then
    run_and_capture artifact_fetch_rc "$artifact_log" env DEST_DIR="$artifact_dir" "$fetch_script"
    if [[ "$artifact_fetch_rc" -ne 0 ]]; then
        session_outcome="artifact_fetch_failed"
    fi
fi

{
    echo "session_kind=${session_kind}"
    echo "session_outcome=${session_outcome}"
    echo "preflight_overall=${preflight_overall}"
    echo "last_overall=${post_state_overall:-${preflight_overall}}"
    echo "final_driver_srcversion=${post_state_driver_srcversion:-unknown}"
    echo "pre_state_rc=${pre_state_rc}"
    echo "setup_rc=${setup_rc}"
    echo "reboot_required=${reboot_required}"
    echo "reboot_rc=${reboot_rc}"
    echo "post_state_rc=${post_state_rc}"
    echo "artifact_fetch_rc=${artifact_fetch_rc}"
    echo "finished_at=$(date -Iseconds)"
} >"$result_file"

if [[ "$pre_state_rc" -ne 0 ]]; then
    exit "$pre_state_rc"
fi
if [[ "$setup_rc" -ne 0 ]]; then
    exit "$setup_rc"
fi
if [[ "$reboot_rc" != "skipped" && "$reboot_rc" -ne 0 ]]; then
    exit "$reboot_rc"
fi
if [[ "$post_state_rc" != "skipped" && "$post_state_rc" -ne 0 ]]; then
    exit "$post_state_rc"
fi
if [[ "$artifact_fetch_rc" != "skipped" && "$artifact_fetch_rc" -ne 0 ]]; then
    exit "$artifact_fetch_rc"
fi
