#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

session_name="${SESSION_NAME:-sgx-submit-op-phase-matrix}"
session_kind="sgx_submit_op_phase_matrix"
logs_root="${LOGS_ROOT:-${repo_root}/logs/vaio-ioctl}"
session_timestamp="${SESSION_TIMESTAMP:-$(date +%Y%m%d-%H%M%S)}"
started_at="$(date -Iseconds)"
session_dir="$("${script_dir}/allocate-session-dir.sh" "$logs_root" "$session_name" "$session_timestamp")"
summary_log="${session_dir}/summary.log"
manifest_file="${session_dir}/session.env"
result_file="${session_dir}/session-result.env"
health_script="${HEALTH_SCRIPT:-${script_dir}/check-vaio-target-health.sh}"
state_script="${STATE_SCRIPT:-${script_dir}/show-vaio-gma500-state.sh}"
submit_script="${SUBMIT_SCRIPT:-${script_dir}/run-vaio-sgx-ioctl-submit-op.sh}"
phase_profile_script="${PHASE_PROFILE_SCRIPT:-${script_dir}/resolve-sgx-submit-phase-profile.sh}"
phase_profile="${PHASE_PROFILE:-full}"
phases_override="${PHASES:-}"
timeout_bin="${TIMEOUT_BIN:-timeout}"
phase_timeout_secs="${PHASE_TIMEOUT_SECS:-300}"
health_timeout_secs="${HEALTH_TIMEOUT_SECS:-60}"
state_timeout_secs="${STATE_TIMEOUT_SECS:-60}"

usage() {
    cat <<'EOF'
Usage:
  run-vaio-sgx-submit-op-phase-matrix.sh

Behavior:
  Runs the focused typed submit-op path repeatedly, stopping after one named
  stage each time via STOP_AFTER_STAGE. After every phase it records target
  health, stops on the first failed phase or unhealthy target, and saves one
  timestamped session bundle.

Environment:
  SESSION_NAME    Session suffix. Default: sgx-submit-op-phase-matrix
  LOGS_ROOT       Base directory for logs.
  HEALTH_SCRIPT   Override health probe script.
  STATE_SCRIPT    Override remote-state script.
  SUBMIT_SCRIPT   Override focused submit-op runner.
  PHASE_PROFILE   Named phase profile. Default: full
  PHASES          Explicit comma-separated ordered phase list. Overrides
                  PHASE_PROFILE.
  TIMEOUT_BIN     Override timeout(1) command. Default: timeout
  PHASE_TIMEOUT_SECS
                  Per-phase submit timeout in seconds. Default: 300
  HEALTH_TIMEOUT_SECS
                  Per-health-probe timeout in seconds. Default: 60
  STATE_TIMEOUT_SECS
                  Per-state-capture timeout in seconds. Default: 60
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

for required in "$health_script" "$state_script" "$submit_script" "$phase_profile_script"; do
    if [[ ! -x "$required" ]]; then
        echo "required executable missing: $required" >&2
        exit 1
    fi
done

phase_selection="explicit"
phases_csv="$phases_override"
if [[ -z "$phases_csv" ]]; then
    phase_selection="profile"
    phases_csv="$("$phase_profile_script" "$phase_profile")"
fi

if ! command -v "$timeout_bin" >/dev/null 2>&1; then
    echo "timeout command not found: $timeout_bin" >&2
    exit 1
fi

for seconds_var in phase_timeout_secs health_timeout_secs state_timeout_secs; do
    if [[ "${!seconds_var}" -lt 1 ]]; then
        echo "${seconds_var} must be at least 1" >&2
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
    echo "submit_script=${submit_script}"
    echo "phase_profile_script=${phase_profile_script}"
    echo "phase_profile=${phase_profile}"
    echo "phase_selection=${phase_selection}"
    echo "phases=${phases_csv}"
    echo "timeout_bin=${timeout_bin}"
    echo "phase_timeout_secs=${phase_timeout_secs}"
    echo "health_timeout_secs=${health_timeout_secs}"
    echo "state_timeout_secs=${state_timeout_secs}"
} >"$manifest_file"

echo "session_dir=${session_dir}"
echo "summary_log=${summary_log}"
echo "result_file=${result_file}"

preflight_overall="unknown"
last_overall="unknown"
pre_state_rc="unknown"
last_phase=""
last_phase_rc="unknown"
last_phase_health="unknown"
last_phase_reason="interrupted"
post_state_rc="unknown"
final_driver_srcversion="unknown"
final_override_module_size="unknown"
session_outcome="interrupted"

extract_last_value() {
    local file="$1"
    local key="$2"
    awk -F= -v want="$key" '$1 == want { value = substr($0, index($0, "=") + 1) } END { print value }' "$file"
}

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

run_timed_submit_phase() {
    local __rc_var="$1"
    local phase="$2"
    local outfile="$3"

    run_and_capture "$__rc_var" "$outfile" \
        "$timeout_bin" --foreground "${phase_timeout_secs}s" \
        env STOP_AFTER_STAGE="$phase" "$submit_script"
}

run_timed_health_probe() {
    local __rc_var="$1"
    local outfile="$2"

    run_and_capture "$__rc_var" "$outfile" \
        "$timeout_bin" --foreground "${health_timeout_secs}s" \
        "$health_script"
}

run_timed_state_capture() {
    local __rc_var="$1"
    local outfile="$2"

    run_and_capture "$__rc_var" "$outfile" \
        "$timeout_bin" --foreground "${state_timeout_secs}s" \
        "$state_script"
}

write_fallback_result() {
    local rc="$1"
    if [[ -f "$result_file" ]]; then
        return 0
    fi

    {
        echo "session_kind=${session_kind}"
        echo "session_outcome=${session_outcome}"
        echo "phase_profile=${phase_profile}"
        echo "phase_selection=${phase_selection}"
        echo "phases=${phases_csv}"
        echo "preflight_overall=${preflight_overall}"
        echo "last_overall=${last_overall}"
        echo "pre_state_rc=${pre_state_rc}"
        echo "last_phase=${last_phase}"
        echo "last_phase_rc=${last_phase_rc}"
        echo "last_phase_health=${last_phase_health}"
        echo "last_phase_reason=${last_phase_reason}"
        echo "last_phase_failed_step=$(extract_last_value "$summary_log" "phase_${last_phase}_failed_step")"
        echo "last_phase_failed_rc=$(extract_last_value "$summary_log" "phase_${last_phase}_failed_rc")"
        echo "last_phase_failed_log=$(extract_last_value "$summary_log" "phase_${last_phase}_failed_log")"
        echo "last_phase_remote_tmpdir=$(extract_last_value "$summary_log" "phase_${last_phase}_remote_tmpdir")"
        echo "post_state_rc=${post_state_rc}"
        echo "final_driver_srcversion=${final_driver_srcversion}"
        echo "final_override_module_size=${final_override_module_size}"
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

is_valid_phase() {
    case "$1" in
        sync|owner|report|manifest|descriptor|descriptor_op|descriptor_query|plan|plan_op|plan_query|plan_query_fetch|plan_query_basic|plan_query_offset|plan_query_config|plan_query_snapshot|prepare|prepare_op|prepare_query|arm|emit|validate|launch|issue|running|complete)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

IFS=',' read -r -a phases <<<"$phases_csv"
if [[ "${#phases[@]}" -eq 0 ]]; then
    echo "PHASES must not be empty" >&2
    exit 1
fi
for phase in "${phases[@]}"; do
    if ! is_valid_phase "$phase"; then
        echo "Invalid phase in PHASES: $phase" >&2
        exit 1
    fi
done

preflight_log="${session_dir}/preflight-health.log"
pre_state_log="${session_dir}/pre-state.log"

run_timed_health_probe preflight_health_rc "$preflight_log"
preflight_overall="$(extract_last_value "$preflight_log" "overall")"
if [[ -z "$preflight_overall" ]]; then
    preflight_overall="unknown"
fi

if [[ "$preflight_health_rc" -ne 0 || "$preflight_overall" != "remote_usable" ]]; then
    {
        echo "session_kind=${session_kind}"
        echo "session_outcome=blocked_preflight"
        echo "phase_profile=${phase_profile}"
        echo "phase_selection=${phase_selection}"
        echo "phases=${phases_csv}"
        echo "preflight_health_rc=${preflight_health_rc}"
        echo "preflight_overall=${preflight_overall}"
        echo "last_overall=${preflight_overall}"
        echo "final_driver_srcversion=unknown"
        echo "finished_at=$(date -Iseconds)"
    } >"$result_file"
    exit 2
fi

run_timed_state_capture pre_state_rc "$pre_state_log"

last_phase=""
last_phase_rc=0
last_phase_health="remote_usable"
last_phase_reason="ok"
session_outcome="completed"

for phase in "${phases[@]}"; do
    phase_log="${session_dir}/${phase}.log"
    health_log="${session_dir}/${phase}-health.log"

    echo "phase=${phase}" >>"$summary_log"
    echo "phase=${phase}"

    run_timed_submit_phase phase_rc "$phase" "$phase_log"

    run_timed_health_probe health_rc "$health_log"
    health_overall="$(extract_last_value "$health_log" "overall")"
    if [[ -z "$health_overall" ]]; then
        health_overall="unknown"
    fi

    phase_reason="ok"
    if [[ "$phase_rc" -eq 124 ]]; then
        phase_reason="phase_timeout"
    elif [[ "$phase_rc" -ne 0 ]]; then
        phase_reason="phase_failed"
    elif [[ "$health_rc" -eq 124 ]]; then
        phase_reason="health_timeout"
    elif [[ "$health_rc" -ne 0 || "$health_overall" != "remote_usable" ]]; then
        phase_reason="phase_unhealthy"
    fi

    {
        echo "phase_${phase}_rc=${phase_rc}"
        echo "phase_${phase}_health=${health_overall}"
        echo "phase_${phase}_reason=${phase_reason}"
        echo "phase_${phase}_failed_step=$(extract_last_value "$phase_log" "FAILED_STEP")"
        echo "phase_${phase}_failed_rc=$(extract_last_value "$phase_log" "FAILED_RC")"
        echo "phase_${phase}_failed_log=$(extract_last_value "$phase_log" "FAILED_LOG")"
        echo "phase_${phase}_remote_tmpdir=$(extract_last_value "$phase_log" "REMOTE_TMPDIR_PRESERVED")"
    } >>"$summary_log"

    last_phase="$phase"
    last_phase_rc="$phase_rc"
    last_phase_health="$health_overall"
    last_phase_reason="$phase_reason"

    if [[ "$phase_rc" -eq 124 ]]; then
        session_outcome="phase_timeout"
        break
    fi
    if [[ "$phase_rc" -ne 0 ]]; then
        session_outcome="phase_failed"
        break
    fi
    if [[ "$health_rc" -eq 124 ]]; then
        session_outcome="health_timeout"
        break
    fi
    if [[ "$health_rc" -ne 0 || "$health_overall" != "remote_usable" ]]; then
        session_outcome="phase_unhealthy"
        break
    fi
done

post_state_log="${session_dir}/post-state.log"
run_timed_state_capture post_state_rc "$post_state_log"

final_driver_srcversion="$(extract_last_value "$post_state_log" "driver_srcversion")"
final_override_module_size="$(extract_last_value "$post_state_log" "override_module_size")"
last_overall="$(extract_last_value "$post_state_log" "overall")"
if [[ -z "$last_overall" ]]; then
    last_overall="$last_phase_health"
fi

{
    echo "session_kind=${session_kind}"
    echo "session_outcome=${session_outcome}"
    echo "phase_profile=${phase_profile}"
    echo "phase_selection=${phase_selection}"
    echo "phases=${phases_csv}"
    echo "preflight_overall=${preflight_overall}"
    echo "last_overall=${last_overall}"
    echo "pre_state_rc=${pre_state_rc}"
    echo "last_phase=${last_phase}"
    echo "last_phase_rc=${last_phase_rc}"
    echo "last_phase_health=${last_phase_health}"
    echo "last_phase_reason=${last_phase_reason}"
    echo "last_phase_failed_step=$(extract_last_value "$summary_log" "phase_${last_phase}_failed_step")"
    echo "last_phase_failed_rc=$(extract_last_value "$summary_log" "phase_${last_phase}_failed_rc")"
    echo "last_phase_failed_log=$(extract_last_value "$summary_log" "phase_${last_phase}_failed_log")"
    echo "last_phase_remote_tmpdir=$(extract_last_value "$summary_log" "phase_${last_phase}_remote_tmpdir")"
    echo "post_state_rc=${post_state_rc}"
    echo "final_driver_srcversion=${final_driver_srcversion}"
    echo "final_override_module_size=${final_override_module_size}"
    echo "finished_at=$(date -Iseconds)"
} >"$result_file"

cat "$result_file"

if [[ "$session_outcome" != "completed" ]]; then
    exit 1
fi
