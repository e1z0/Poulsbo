#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

session_name="${SESSION_NAME:-sgx-submit-op-phase-investigation}"
session_kind="sgx_submit_op_phase_investigation"
logs_root="${LOGS_ROOT:-${repo_root}/logs/vaio-ioctl}"
session_timestamp="${SESSION_TIMESTAMP:-$(date +%Y%m%d-%H%M%S)}"
started_at="$(date -Iseconds)"
session_dir="$("${script_dir}/allocate-session-dir.sh" "$logs_root" "$session_name" "$session_timestamp")"
summary_log="${session_dir}/summary.log"
manifest_file="${session_dir}/session.env"
result_file="${session_dir}/session-result.env"
children_root="${session_dir}/children"
bisect_script="${BISECT_SCRIPT:-${script_dir}/run-vaio-sgx-submit-op-phase-bisect.sh}"
matrix_script="${MATRIX_SCRIPT:-${script_dir}/run-vaio-sgx-submit-op-phase-matrix.sh}"
phase_profile_script="${PHASE_PROFILE_SCRIPT:-${script_dir}/resolve-sgx-submit-phase-profile.sh}"
phase_profile="${PHASE_PROFILE:-full}"
phases_override="${PHASES:-}"
local_module_path="${LOCAL_MODULE:-${repo_root}/build/devnull-vaio-gma500-src/gma500_gfx.ko}"
phase_timeout_secs="${PHASE_TIMEOUT_SECS:-300}"
health_timeout_secs="${HEALTH_TIMEOUT_SECS:-60}"
state_timeout_secs="${STATE_TIMEOUT_SECS:-60}"

usage() {
    cat <<'EOF'
Usage:
  run-vaio-sgx-submit-op-phase-investigation.sh

Behavior:
  Runs the focused submit-op prefix bisect first. If the bisect finds a bad
  cumulative prefix, reruns a narrowed confirmation pass on the previous-good
  and first-bad stop stages. One timestamped investigation bundle is written
  under logs/vaio-ioctl/.

Environment:
  SESSION_NAME    Session suffix. Default: sgx-submit-op-phase-investigation
  LOGS_ROOT       Base directory for logs.
  BISECT_SCRIPT   Override prefix-bisect script.
  MATRIX_SCRIPT   Override exhaustive confirmation script.
  PHASE_PROFILE   Named phase profile. Default: full
  PHASES          Explicit comma-separated ordered phase list. Overrides
                  PHASE_PROFILE.
  LOCAL_MODULE    Override local module path recorded in the manifest.
  PHASE_TIMEOUT_SECS
                  Forwarded per-phase submit timeout in seconds.
  HEALTH_TIMEOUT_SECS
                  Forwarded per-health-probe timeout in seconds.
  STATE_TIMEOUT_SECS
                  Forwarded per-state-capture timeout in seconds.
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

for required in "$bisect_script" "$matrix_script" "$phase_profile_script"; do
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

{
    echo "timestamp=${session_timestamp}"
    echo "started_at=${started_at}"
    echo "session_kind=${session_kind}"
    echo "session_name=${session_name}"
    echo "session_dir=${session_dir}"
    echo "children_root=${children_root}"
    echo "bisect_script=${bisect_script}"
    echo "matrix_script=${matrix_script}"
    echo "phase_profile_script=${phase_profile_script}"
    echo "phase_profile=${phase_profile}"
    echo "phase_selection=${phase_selection}"
    echo "phases=${phases_csv}"
    echo "local_module_path=${local_module_path}"
    echo "phase_timeout_secs=${phase_timeout_secs}"
    echo "health_timeout_secs=${health_timeout_secs}"
    echo "state_timeout_secs=${state_timeout_secs}"
    if [[ -f "$local_module_path" ]]; then
        echo "local_module_srcversion=$(modinfo -F srcversion "$local_module_path" 2>/dev/null || echo unknown)"
    else
        echo "local_module_srcversion=missing"
    fi
} >"$manifest_file"

echo "session_dir=${session_dir}"
echo "summary_log=${summary_log}"
echo "result_file=${result_file}"

bisect_rc="unknown"
bisect_session_dir=""
bisect_result_file=""
bisect_outcome="interrupted"
bisect_last_good_phase="none"
bisect_first_bad_phase="none"
bisect_first_bad_reason="unknown"
bisect_first_bad_failed_step=""
bisect_first_bad_failed_rc=""
bisect_first_bad_failed_log=""
bisect_first_bad_remote_tmpdir=""
bisect_last_overall="unknown"
bisect_preflight_overall="unknown"
bisect_final_driver_srcversion="unknown"
session_outcome="interrupted"
confirm_rc="skipped"
confirm_session_dir="skipped"
confirm_result_file="skipped"
confirm_outcome="skipped"
confirm_phases="none"
confirm_last_phase="skipped"
confirm_last_phase_reason="skipped"
confirm_last_phase_failed_step="skipped"
confirm_last_phase_failed_rc="skipped"
confirm_last_phase_failed_log="skipped"
confirm_last_phase_remote_tmpdir="skipped"
last_overall="unknown"
preflight_overall="unknown"
final_driver_srcversion="unknown"
exit_code=1

extract_last_value() {
    local file="$1"
    local key="$2"
    awk -F= -v want="$key" '$1 == want { value = substr($0, index($0, "=") + 1) } END { print value }' "$file"
}

child_value() {
    local log_file="$1"
    local key="$2"
    awk -F= -v want="$key" '$1 == want { print substr($0, index($0, "=") + 1); exit }' "$log_file"
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
        echo "bisect_rc=${bisect_rc}"
        echo "bisect_session_dir=${bisect_session_dir}"
        echo "bisect_result_file=${bisect_result_file}"
        echo "bisect_outcome=${bisect_outcome}"
        echo "bisect_last_good_phase=${bisect_last_good_phase}"
        echo "bisect_first_bad_phase=${bisect_first_bad_phase}"
        echo "bisect_first_bad_reason=${bisect_first_bad_reason}"
        echo "bisect_first_bad_failed_step=${bisect_first_bad_failed_step}"
        echo "bisect_first_bad_failed_rc=${bisect_first_bad_failed_rc}"
        echo "bisect_first_bad_failed_log=${bisect_first_bad_failed_log}"
        echo "bisect_first_bad_remote_tmpdir=${bisect_first_bad_remote_tmpdir}"
        echo "preflight_overall=${preflight_overall}"
        echo "last_overall=${last_overall}"
        echo "final_driver_srcversion=${final_driver_srcversion}"
        echo "confirm_rc=${confirm_rc}"
        echo "confirm_phases=${confirm_phases}"
        echo "confirm_session_dir=${confirm_session_dir}"
        echo "confirm_result_file=${confirm_result_file}"
        echo "confirm_outcome=${confirm_outcome}"
        echo "confirm_last_phase=${confirm_last_phase}"
        echo "confirm_last_phase_reason=${confirm_last_phase_reason}"
        echo "confirm_last_phase_failed_step=${confirm_last_phase_failed_step}"
        echo "confirm_last_phase_failed_rc=${confirm_last_phase_failed_rc}"
        echo "confirm_last_phase_failed_log=${confirm_last_phase_failed_log}"
        echo "confirm_last_phase_remote_tmpdir=${confirm_last_phase_remote_tmpdir}"
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

run_and_capture bisect_rc "${session_dir}/bisect.log" \
    env \
    LOGS_ROOT="$children_root" \
    SESSION_NAME="bisect" \
    PHASES="$phases_csv" \
    PHASE_TIMEOUT_SECS="$phase_timeout_secs" \
    HEALTH_TIMEOUT_SECS="$health_timeout_secs" \
    STATE_TIMEOUT_SECS="$state_timeout_secs" \
    "$bisect_script"

bisect_session_dir="$(child_value "${session_dir}/bisect.log" session_dir)"
bisect_result_from_log="$(child_value "${session_dir}/bisect.log" result_file)"
bisect_result_file="${bisect_result_from_log:-${bisect_session_dir}/session-result.env}"
bisect_outcome="$(extract_last_value "$bisect_result_file" session_outcome)"
bisect_last_good_phase="$(extract_last_value "$bisect_result_file" last_good_phase)"
bisect_first_bad_phase="$(extract_last_value "$bisect_result_file" first_bad_phase)"
bisect_first_bad_reason="$(extract_last_value "$bisect_result_file" first_bad_reason)"
bisect_first_bad_failed_step="$(extract_last_value "$bisect_result_file" first_bad_failed_step)"
bisect_first_bad_failed_rc="$(extract_last_value "$bisect_result_file" first_bad_failed_rc)"
bisect_first_bad_failed_log="$(extract_last_value "$bisect_result_file" first_bad_failed_log)"
bisect_first_bad_remote_tmpdir="$(extract_last_value "$bisect_result_file" first_bad_remote_tmpdir)"
bisect_last_overall="$(extract_last_value "$bisect_result_file" last_overall)"
bisect_preflight_overall="$(extract_last_value "$bisect_result_file" preflight_overall)"
bisect_final_driver_srcversion="$(extract_last_value "$bisect_result_file" final_driver_srcversion)"

{
    echo "bisect_rc=${bisect_rc}"
    echo "bisect_session_dir=${bisect_session_dir}"
    echo "bisect_result_file=${bisect_result_file}"
    echo "bisect_outcome=${bisect_outcome}"
    echo "bisect_last_good_phase=${bisect_last_good_phase}"
    echo "bisect_first_bad_phase=${bisect_first_bad_phase}"
    echo "bisect_first_bad_reason=${bisect_first_bad_reason}"
    echo "bisect_first_bad_failed_step=${bisect_first_bad_failed_step}"
    echo "bisect_first_bad_failed_rc=${bisect_first_bad_failed_rc}"
    echo "bisect_first_bad_failed_log=${bisect_first_bad_failed_log}"
    echo "bisect_first_bad_remote_tmpdir=${bisect_first_bad_remote_tmpdir}"
    echo "bisect_last_overall=${bisect_last_overall}"
    echo "bisect_preflight_overall=${bisect_preflight_overall}"
    echo "bisect_final_driver_srcversion=${bisect_final_driver_srcversion}"
} >>"$summary_log"

session_outcome="investigation_failed"
last_overall="${bisect_last_overall:-}"
preflight_overall="${bisect_preflight_overall:-}"
final_driver_srcversion="${bisect_final_driver_srcversion:-}"

case "$bisect_outcome" in
    blocked_preflight)
        session_outcome="blocked_preflight"
        exit_code=2
        ;;
    all_prefixes_healthy)
        session_outcome="no_bad_prefix_found"
        exit_code=0
        ;;
    bisect_identified_bad_prefix)
        if [[ -n "$bisect_last_good_phase" && "$bisect_last_good_phase" != "none" ]]; then
            confirm_phases="${bisect_last_good_phase},${bisect_first_bad_phase}"
        else
            confirm_phases="${bisect_first_bad_phase}"
        fi

        run_and_capture confirm_rc "${session_dir}/confirm.log" \
            env \
            LOGS_ROOT="$children_root" \
            SESSION_NAME="confirm" \
            PHASES="$confirm_phases" \
            PHASE_TIMEOUT_SECS="$phase_timeout_secs" \
            HEALTH_TIMEOUT_SECS="$health_timeout_secs" \
            STATE_TIMEOUT_SECS="$state_timeout_secs" \
            "$matrix_script"

        confirm_session_dir="$(child_value "${session_dir}/confirm.log" session_dir)"
        confirm_result_from_log="$(child_value "${session_dir}/confirm.log" result_file)"
        confirm_result_file="${confirm_result_from_log:-${confirm_session_dir}/session-result.env}"
        confirm_outcome="$(extract_last_value "$confirm_result_file" session_outcome)"
        confirm_last_phase="$(extract_last_value "$confirm_result_file" last_phase)"
        confirm_last_phase_reason="$(extract_last_value "$confirm_result_file" last_phase_reason)"
        confirm_last_phase_failed_step="$(extract_last_value "$confirm_result_file" last_phase_failed_step)"
        confirm_last_phase_failed_rc="$(extract_last_value "$confirm_result_file" last_phase_failed_rc)"
        confirm_last_phase_failed_log="$(extract_last_value "$confirm_result_file" last_phase_failed_log)"
        confirm_last_phase_remote_tmpdir="$(extract_last_value "$confirm_result_file" last_phase_remote_tmpdir)"
        confirm_last_overall="$(extract_last_value "$confirm_result_file" last_overall)"
        confirm_final_driver_srcversion="$(extract_last_value "$confirm_result_file" final_driver_srcversion)"

        {
            echo "confirm_rc=${confirm_rc}"
            echo "confirm_phases=${confirm_phases}"
            echo "confirm_session_dir=${confirm_session_dir}"
            echo "confirm_result_file=${confirm_result_file}"
            echo "confirm_outcome=${confirm_outcome}"
            echo "confirm_last_phase=${confirm_last_phase}"
            echo "confirm_last_phase_reason=${confirm_last_phase_reason}"
            echo "confirm_last_phase_failed_step=${confirm_last_phase_failed_step}"
            echo "confirm_last_phase_failed_rc=${confirm_last_phase_failed_rc}"
            echo "confirm_last_phase_failed_log=${confirm_last_phase_failed_log}"
            echo "confirm_last_phase_remote_tmpdir=${confirm_last_phase_remote_tmpdir}"
            echo "confirm_last_overall=${confirm_last_overall}"
            echo "confirm_final_driver_srcversion=${confirm_final_driver_srcversion}"
        } >>"$summary_log"

        if [[ -n "$confirm_last_overall" ]]; then
            last_overall="$confirm_last_overall"
        fi
        if [[ -n "$confirm_final_driver_srcversion" ]]; then
            final_driver_srcversion="$confirm_final_driver_srcversion"
        fi

        if [[ "$confirm_outcome" == "phase_failed" || "$confirm_outcome" == "phase_unhealthy" ]]; then
            if [[ "$confirm_last_phase" == "$bisect_first_bad_phase" ]]; then
                session_outcome="confirmed_bad_prefix"
            else
                session_outcome="confirm_inconclusive"
            fi
        elif [[ "$confirm_outcome" == "completed" ]]; then
            session_outcome="confirm_inconclusive"
        else
            session_outcome="confirm_inconclusive"
        fi
        exit_code=1
        ;;
    bisect_terminated_on_unhealthy_bad_probe)
        session_outcome="terminal_bad_prefix_identified"
        confirm_rc="skipped"
        confirm_session_dir="skipped_target_lost"
        confirm_result_file="skipped_target_lost"
        confirm_outcome="skipped_target_lost"
        if [[ -n "$bisect_first_bad_phase" && "$bisect_first_bad_phase" != "none" ]]; then
            confirm_phases="${bisect_first_bad_phase}"
            confirm_last_phase="${bisect_first_bad_phase}"
            confirm_last_phase_reason="${bisect_first_bad_reason}"
            confirm_last_phase_failed_step="${bisect_first_bad_failed_step}"
            confirm_last_phase_failed_rc="${bisect_first_bad_failed_rc}"
            confirm_last_phase_failed_log="${bisect_first_bad_failed_log}"
            confirm_last_phase_remote_tmpdir="${bisect_first_bad_remote_tmpdir}"
        fi
        exit_code=1
        ;;
    *)
        session_outcome="investigation_failed"
        exit_code=1
        ;;
esac

{
    echo "session_kind=${session_kind}"
    echo "session_outcome=${session_outcome}"
    echo "phase_profile=${phase_profile}"
    echo "phase_selection=${phase_selection}"
    echo "phases=${phases_csv}"
    echo "bisect_rc=${bisect_rc}"
    echo "bisect_session_dir=${bisect_session_dir}"
    echo "bisect_result_file=${bisect_result_file}"
    echo "bisect_outcome=${bisect_outcome}"
    echo "bisect_last_good_phase=${bisect_last_good_phase}"
    echo "bisect_first_bad_phase=${bisect_first_bad_phase}"
    echo "bisect_first_bad_reason=${bisect_first_bad_reason}"
    echo "bisect_first_bad_failed_step=${bisect_first_bad_failed_step}"
    echo "bisect_first_bad_failed_rc=${bisect_first_bad_failed_rc}"
    echo "bisect_first_bad_failed_log=${bisect_first_bad_failed_log}"
    echo "bisect_first_bad_remote_tmpdir=${bisect_first_bad_remote_tmpdir}"
    echo "preflight_overall=${preflight_overall}"
    echo "last_overall=${last_overall}"
    echo "final_driver_srcversion=${final_driver_srcversion}"
    echo "confirm_rc=${confirm_rc}"
    echo "confirm_phases=${confirm_phases}"
    echo "confirm_session_dir=${confirm_session_dir}"
    echo "confirm_result_file=${confirm_result_file}"
    echo "confirm_outcome=${confirm_outcome}"
    echo "confirm_last_phase=${confirm_last_phase}"
    echo "confirm_last_phase_reason=${confirm_last_phase_reason}"
    echo "confirm_last_phase_failed_step=${confirm_last_phase_failed_step}"
    echo "confirm_last_phase_failed_rc=${confirm_last_phase_failed_rc}"
    echo "confirm_last_phase_failed_log=${confirm_last_phase_failed_log}"
    echo "confirm_last_phase_remote_tmpdir=${confirm_last_phase_remote_tmpdir}"
    echo "finished_at=$(date -Iseconds)"
} >"$result_file"

cat "$result_file"
exit "$exit_code"
