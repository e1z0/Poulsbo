#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

session_name="${SESSION_NAME:-panic-test}"
session_kind="panic_test"
logs_root="${LOGS_ROOT:-${repo_root}/logs/vaio-panic-test}"
session_timestamp="${SESSION_TIMESTAMP:-$(date +%Y%m%d-%H%M%S)}"
started_at="$(date -Iseconds)"
session_dir="$("${script_dir}/allocate-session-dir.sh" "$logs_root" "$session_name" "$session_timestamp")"
preflight_health_log="${session_dir}/preflight-health.log"
pre_state_log="${session_dir}/pre-state.log"
trigger_log="${session_dir}/panic-trigger.log"
reboot_health_log="${session_dir}/reboot-health.log"
post_state_log="${session_dir}/post-state.log"
artifact_log="${session_dir}/panic-artifacts.log"
artifact_dir="${session_dir}/panic-artifacts"
manifest_file="${session_dir}/session.env"
result_file="${session_dir}/session-result.env"

health_script="${HEALTH_SCRIPT:-${script_dir}/check-vaio-target-health.sh}"
state_script="${STATE_SCRIPT:-${script_dir}/show-vaio-gma500-state.sh}"
fetch_script="${FETCH_SCRIPT:-${script_dir}/fetch-vaio-panic-artifacts.sh}"
target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
trigger_timeout_secs="${TRIGGER_TIMEOUT_SECS:-15}"
wait_for_drop_secs="${WAIT_FOR_DROP_SECS:-120}"
post_reboot_timeout_secs="${POST_REBOOT_TIMEOUT_SECS:-900}"
poll_interval_secs="${POLL_INTERVAL_SECS:-5}"

usage() {
    cat <<'EOF'
Usage:
  run-vaio-panic-test-session.sh

Behavior:
  Intentionally triggers a kernel panic through sysrq `c`, waits for the VAIO
  to drop and return, then fetches the newest panic artifacts.

Environment:
  SESSION_NAME             Session suffix. Default: panic-test
  LOGS_ROOT                Base directory for session logs.
  HEALTH_SCRIPT            Override health probe path.
  STATE_SCRIPT             Override remote-state probe path.
  FETCH_SCRIPT             Override artifact-fetch path.
  TRIGGER_TIMEOUT_SECS     SSH timeout for the panic trigger. Default: 15
  WAIT_FOR_DROP_SECS       Seconds to wait for the host to drop. Default: 120
  POST_REBOOT_TIMEOUT_SECS Seconds to wait for remote usability. Default: 900
  POLL_INTERVAL_SECS       Seconds between polls. Default: 5
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

for required in "$health_script" "$state_script" "$fetch_script"; do
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
    echo "fetch_script=${fetch_script}"
    echo "target_host=${target_host}"
    echo "target_user=${target_user}"
    echo "trigger_timeout_secs=${trigger_timeout_secs}"
    echo "wait_for_drop_secs=${wait_for_drop_secs}"
    echo "post_reboot_timeout_secs=${post_reboot_timeout_secs}"
    echo "poll_interval_secs=${poll_interval_secs}"
} >"$manifest_file"

echo "session_dir=${session_dir}"
echo "preflight_health_log=${preflight_health_log}"
echo "pre_state_log=${pre_state_log}"
echo "trigger_log=${trigger_log}"
echo "reboot_health_log=${reboot_health_log}"
echo "post_state_log=${post_state_log}"
echo "artifact_log=${artifact_log}"
echo "result_file=${result_file}"

preflight_overall="unknown"
pre_state_rc="unknown"
trigger_rc="unknown"
drop_seen=0
reboot_rc=1
post_state_rc="skipped"
artifact_fetch_rc="skipped"
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
        echo "trigger_rc=${trigger_rc}"
        echo "drop_seen=${drop_seen}"
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
        "skipped: panic trigger not attempted because preflight overall=${preflight_overall}" \
        >"$trigger_log"
    printf '%s\n' \
        "skipped: reboot watch not attempted because panic trigger was not attempted" \
        >"$reboot_health_log"
    printf '%s\n' \
        "skipped: post-state not captured because panic trigger was not attempted" \
        >"$post_state_log"
    printf '%s\n' \
        "skipped: panic artifacts not fetched because panic trigger was not attempted" \
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

set +e
timeout "$trigger_timeout_secs" ssh \
    -o BatchMode=yes \
    -o ConnectTimeout=5 \
    "${target_user}@${target_host}" \
    "sysctl -w kernel.sysrq=1 >/dev/null; echo c > /proc/sysrq-trigger" >"$trigger_log" 2>&1
trigger_rc=$?
set -e
cat "$trigger_log"

: >"$reboot_health_log"
drop_deadline=$(( $(date +%s) + wait_for_drop_secs ))
while (( $(date +%s) < drop_deadline )); do
    health_output="$("$health_script")"
    timestamp_line="$(date -Iseconds)"
    printf 'timestamp=%s\n%s\n' "$timestamp_line" "$health_output" >>"$reboot_health_log"
    printf 'timestamp=%s\n%s\n' "$timestamp_line" "$health_output"
    overall="$(printf '%s\n' "$health_output" | awk -F= '$1 == "overall" { print $2 }')"
    if [[ "$overall" != "remote_usable" ]]; then
        drop_seen=1
        break
    fi
    sleep "$poll_interval_secs"
done

if [[ "$drop_seen" == "1" ]]; then
    reboot_deadline=$(( $(date +%s) + post_reboot_timeout_secs ))
    while (( $(date +%s) < reboot_deadline )); do
        health_output="$("$health_script")"
        timestamp_line="$(date -Iseconds)"
        printf 'timestamp=%s\n%s\n' "$timestamp_line" "$health_output" >>"$reboot_health_log"
        printf 'timestamp=%s\n%s\n' "$timestamp_line" "$health_output"
        overall="$(printf '%s\n' "$health_output" | awk -F= '$1 == "overall" { print $2 }')"
        if [[ "$overall" == "remote_usable" ]]; then
            reboot_rc=0
            break
        fi
        sleep "$poll_interval_secs"
    done
fi

session_outcome="completed"
if [[ "$drop_seen" != "1" ]]; then
    session_outcome="panic_drop_not_observed"
    printf '%s\n' \
        "skipped: post-state not captured because the host never dropped after the panic trigger" \
        >"$post_state_log"
    printf '%s\n' \
        "skipped: panic artifacts not fetched because the host never dropped after the panic trigger" \
        >"$artifact_log"
elif [[ "$reboot_rc" -ne 0 ]]; then
    session_outcome="reboot_wait_failed"
    printf '%s\n' \
        "skipped: post-state not captured because the host did not return after the panic trigger" \
        >"$post_state_log"
    printf '%s\n' \
        "skipped: panic artifacts not fetched because the host did not return after the panic trigger" \
        >"$artifact_log"
else
    run_and_capture post_state_rc "$post_state_log" "$state_script"
    if [[ "$post_state_rc" -eq 0 ]]; then
        post_state_overall="$(health_value "$post_state_log" overall)"
        post_state_driver_srcversion="$(health_value "$post_state_log" driver_srcversion)"
    else
        session_outcome="post_state_capture_failed"
    fi
fi

if [[ "$session_outcome" == "completed" ]]; then
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
    echo "trigger_rc=${trigger_rc}"
    echo "drop_seen=${drop_seen}"
    echo "reboot_rc=${reboot_rc}"
    echo "post_state_rc=${post_state_rc}"
    echo "artifact_fetch_rc=${artifact_fetch_rc}"
    echo "finished_at=$(date -Iseconds)"
} >"$result_file"

if [[ "$pre_state_rc" -ne 0 ]]; then
    exit "$pre_state_rc"
fi
if [[ "$drop_seen" != "1" ]]; then
    exit 3
fi
if [[ "$reboot_rc" -ne 0 ]]; then
    exit "$reboot_rc"
fi
if [[ "$post_state_rc" != "skipped" && "$post_state_rc" -ne 0 ]]; then
    exit "$post_state_rc"
fi
if [[ "$artifact_fetch_rc" != "skipped" && "$artifact_fetch_rc" -ne 0 ]]; then
    exit "$artifact_fetch_rc"
fi
