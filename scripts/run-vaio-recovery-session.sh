#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
phase_source_lib="${PHASE_SOURCE_LIB:-${script_dir}/lib-vaio-plan-phase-source.sh}"

session_name="${SESSION_NAME:-recovery}"
session_kind="${SESSION_KIND:-recovery}"
poll_interval_secs="${POLL_INTERVAL_SECS:-10}"
max_polls="${MAX_POLLS:-0}"
logs_root="${LOGS_ROOT:-${repo_root}/logs/vaio-recovery}"
session_timestamp="${SESSION_TIMESTAMP:-$(date +%Y%m%d-%H%M%S)}"
started_at="$(date -Iseconds)"
session_dir="$("${script_dir}/allocate-session-dir.sh" "$logs_root" "$session_name" "$session_timestamp")"
health_log="${session_dir}/health-timeline.log"
state_log="${session_dir}/remote-state.log"
validation_label="${VALIDATION_LABEL:-depclear_slot}"
validation_log_basename="${VALIDATION_LOG_BASENAME:-depclear-slot-validation.log}"
validation_log="${session_dir}/${validation_log_basename}"
manifest_file="${session_dir}/session.env"
result_file="${session_dir}/session-result.env"
health_script="${HEALTH_SCRIPT:-${script_dir}/watch-vaio-target-health.sh}"
state_script="${STATE_SCRIPT:-${script_dir}/show-vaio-gma500-state.sh}"
validation_script="${VALIDATION_SCRIPT:-${script_dir}/run-vaio-sgx-dependency-slot-clear.sh}"
validation_phase_profile_request="${PHASE_PROFILE:-}"
validation_phase_selection_request="${PHASE_SELECTION:-}"
validation_phases_request="${PHASES:-}"
validation_phase_profile_request_original="${VALIDATION_PHASE_PROFILE_REQUEST_ORIGINAL:-}"
validation_phase_profile_base="${VALIDATION_PHASE_PROFILE_BASE:-}"
timeout_bin="${TIMEOUT_BIN:-timeout}"
state_timeout_secs="${STATE_TIMEOUT_SECS:-60}"
validation_timeout_secs="${VALIDATION_TIMEOUT_SECS:-1800}"

usage() {
    cat <<'EOF'
Usage:
  run-vaio-recovery-session.sh

Behavior:
  Creates a timestamped local session directory, watches the VAIO health state
  until overall=remote_usable, captures remote module/rollback state, then runs
  one post-recovery validation command. All outputs are saved under one session
  directory.

Environment:
  SESSION_NAME        Session suffix. Default: recovery
  SESSION_KIND        Structured session kind. Default: recovery
  POLL_INTERVAL_SECS  Seconds between health polls. Default: 10
  MAX_POLLS           Stop after this many polls. 0 means wait forever.
  LOGS_ROOT           Base directory for session logs.
  HEALTH_SCRIPT       Override health watcher path.
  STATE_SCRIPT        Override remote-state capture path.
  VALIDATION_SCRIPT   Override post-recovery validation path.
  TIMEOUT_BIN         Override timeout(1) command. Default: timeout
  VALIDATION_LABEL    Human-readable validation label. Default: depclear_slot
  VALIDATION_LOG_BASENAME
                      Validation log filename. Default:
                      depclear-slot-validation.log
  STATE_TIMEOUT_SECS  State capture timeout in seconds. Default: 60
  VALIDATION_TIMEOUT_SECS
                      Validation child timeout in seconds. Default: 1800
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if [[ ! -x "$health_script" ]]; then
    echo "health watcher is not executable: $health_script" >&2
    exit 1
fi

if [[ ! -x "$state_script" ]]; then
    echo "state script is not executable: $state_script" >&2
    exit 1
fi

if [[ ! -x "$validation_script" ]]; then
    echo "validation script is not executable: $validation_script" >&2
    exit 1
fi

if ! command -v "$timeout_bin" >/dev/null 2>&1; then
    echo "timeout command not found: $timeout_bin" >&2
    exit 1
fi

if [[ ! -f "$phase_source_lib" ]]; then
    echo "phase-source helper is missing: $phase_source_lib" >&2
    exit 1
fi

# shellcheck source=/root/Poulsbo/scripts/lib-vaio-plan-phase-source.sh
source "$phase_source_lib"
vaio_plan_phase_source_init_vars "validation_phase_source"
vaio_plan_phase_source_assign_from_environment "VALIDATION_PHASE_SOURCE" "validation_phase_source"

for seconds_var in state_timeout_secs validation_timeout_secs; do
    if [[ "${!seconds_var}" -lt 1 ]]; then
        echo "${seconds_var} must be at least 1" >&2
        exit 1
    fi
done

touch "$health_log"

{
    echo "timestamp=${session_timestamp}"
    echo "started_at=${started_at}"
    echo "session_kind=${session_kind}"
    echo "session_name=${session_name}"
    echo "session_dir=${session_dir}"
    echo "poll_interval_secs=${poll_interval_secs}"
    echo "max_polls=${max_polls}"
    echo "health_script=${health_script}"
    echo "state_script=${state_script}"
    echo "validation_script=${validation_script}"
    echo "validation_label=${validation_label}"
    echo "validation_log_basename=${validation_log_basename}"
    echo "validation_phase_profile_request=${validation_phase_profile_request}"
    echo "validation_phase_selection_request=${validation_phase_selection_request}"
    echo "validation_phases_request=${validation_phases_request}"
    echo "validation_phase_profile_request_original=${validation_phase_profile_request_original}"
    echo "validation_phase_profile_base=${validation_phase_profile_base}"
    vaio_plan_phase_source_emit "validation_phase_source" "validation_phase_source"
    echo "timeout_bin=${timeout_bin}"
    echo "state_timeout_secs=${state_timeout_secs}"
    echo "validation_timeout_secs=${validation_timeout_secs}"
    echo "local_module_path=${repo_root}/build/devnull-vaio-gma500-src/gma500_gfx.ko"
    if [[ -f "${repo_root}/build/devnull-vaio-gma500-src/gma500_gfx.ko" ]]; then
        echo "local_module_srcversion=$(modinfo -F srcversion "${repo_root}/build/devnull-vaio-gma500-src/gma500_gfx.ko" 2>/dev/null || echo unknown)"
    else
        echo "local_module_srcversion=missing"
    fi
} >"$manifest_file"

echo "session_dir=${session_dir}"
echo "health_log=${health_log}"
echo "state_log=${state_log}"
echo "${validation_label}_log=${validation_log}"
echo "result_file=${result_file}"

watcher_rc="unknown"
last_overall="unknown"
state_rc="unknown"
validation_rc="unknown"
state_driver_srcversion="unknown"
validation_child_session_dir=""
validation_child_result_file=""
validation_phase_profile=""
validation_phase_selection=""
validation_phases=""
validation_failed_phase=""
validation_failed_reason=""
validation_failed_step=""
validation_failed_rc=""
session_outcome="interrupted"

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
        echo "watcher_rc=${watcher_rc}"
        echo "last_overall=${last_overall:-unknown}"
        echo "final_driver_srcversion=${state_driver_srcversion:-unknown}"
        echo "state_rc=${state_rc}"
        echo "validation_rc=${validation_rc}"
        echo "validation_label=${validation_label}"
        echo "validation_phase_profile_request=${validation_phase_profile_request}"
        echo "validation_phase_selection_request=${validation_phase_selection_request}"
        echo "validation_phases_request=${validation_phases_request}"
        echo "validation_phase_profile_request_original=${validation_phase_profile_request_original}"
        echo "validation_phase_profile_base=${validation_phase_profile_base}"
        echo "validation_phase_profile=${validation_phase_profile}"
        echo "validation_phase_selection=${validation_phase_selection}"
        echo "validation_phases=${validation_phases}"
        vaio_plan_phase_source_emit "validation_phase_source" "validation_phase_source"
        echo "validation_child_session_dir=${validation_child_session_dir}"
        echo "validation_child_result_file=${validation_child_result_file}"
        echo "validation_failed_phase=${validation_failed_phase}"
        echo "validation_failed_reason=${validation_failed_reason}"
        echo "validation_failed_step=${validation_failed_step}"
        echo "validation_failed_rc=${validation_failed_rc}"
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

set +e
STOP_ON_OVERALL=remote_usable \
POLL_INTERVAL_SECS="$poll_interval_secs" \
MAX_POLLS="$max_polls" \
LOG_FILE="$health_log" \
"$health_script"
watcher_rc=$?
set -e

last_overall="$(awk '
    /^timestamp=/ {
        for (i = 1; i <= NF; i++) {
            if ($i ~ /^overall=/) {
                split($i, parts, "=")
                value = parts[2]
            }
        }
    }
    END {
        print value
    }
' "$health_log")"

if [[ "$watcher_rc" -ne 0 ]]; then
    printf '%s\n' \
        "skipped: target never reached overall=remote_usable during this session" \
        >"$state_log"
    printf '%s\n' \
        "skipped: ${validation_label} not attempted because target never reached overall=remote_usable during this session" \
        >"$validation_log"
    {
        echo "session_kind=${session_kind}"
        echo "session_outcome=blocked_before_remote_usable"
        echo "watcher_rc=${watcher_rc}"
        echo "last_overall=${last_overall:-unknown}"
        echo "final_driver_srcversion=unknown"
        echo "state_rc=skipped"
        echo "validation_rc=skipped"
        echo "validation_phase_profile_request=${validation_phase_profile_request}"
        echo "validation_phase_selection_request=${validation_phase_selection_request}"
        echo "validation_phases_request=${validation_phases_request}"
        echo "validation_phase_profile_request_original=${validation_phase_profile_request_original}"
        echo "validation_phase_profile_base=${validation_phase_profile_base}"
        echo "validation_phase_profile=${validation_phase_profile_request}"
        echo "validation_phase_selection=request_only"
        echo "validation_phases=${validation_phases_request}"
        vaio_plan_phase_source_emit "validation_phase_source" "validation_phase_source"
        echo "finished_at=$(date -Iseconds)"
    } >"$result_file"
    exit "$watcher_rc"
fi

run_and_capture state_rc "$state_log" \
    "$timeout_bin" --foreground "${state_timeout_secs}s" \
    "$state_script"

state_overall="$(awk -F= '$1 == "overall" { value = $2 } END { print value }' "$state_log")"
state_driver_srcversion="$(awk -F= '$1 == "driver_srcversion" { value = $2 } END { print value }' "$state_log")"
if [[ -n "${state_overall}" ]]; then
    last_overall="${state_overall}"
fi

run_and_capture validation_rc "$validation_log" \
    "$timeout_bin" --foreground "${validation_timeout_secs}s" \
    "$validation_script"

validation_child_session_dir="$(child_value "$validation_log" session_dir)"
validation_child_result_from_log="$(child_value "$validation_log" result_file)"
validation_child_result_file="${validation_child_result_from_log:-${validation_child_session_dir}/session-result.env}"
if [[ -n "${validation_child_result_file}" && -f "${validation_child_result_file}" ]]; then
    validation_phase_profile="$(extract_last_value "$validation_child_result_file" phase_profile)"
    validation_phase_selection="$(extract_last_value "$validation_child_result_file" phase_selection)"
    validation_phases="$(extract_last_value "$validation_child_result_file" phases)"
fi
if [[ -z "$validation_phase_profile" ]]; then
    validation_phase_profile="${validation_phase_profile_request}"
fi
if [[ -z "$validation_phase_selection" ]]; then
    validation_phase_selection="${validation_phase_selection_request}"
fi
if [[ -z "$validation_phases" ]]; then
    validation_phases="${validation_phases_request}"
fi
validation_failed_phase=""
validation_failed_reason=""
validation_failed_step=""
validation_failed_rc=""
if [[ -n "${validation_child_result_file}" && -f "${validation_child_result_file}" ]]; then
    validation_failed_phase="$(extract_last_value "$validation_child_result_file" confirm_last_phase)"
    if [[ -z "$validation_failed_phase" ]]; then
        validation_failed_phase="$(extract_last_value "$validation_child_result_file" last_phase)"
    fi
    if [[ -z "$validation_failed_phase" ]]; then
        validation_failed_phase="$(extract_last_value "$validation_child_result_file" first_bad_phase)"
    fi

    validation_failed_reason="$(extract_last_value "$validation_child_result_file" confirm_last_phase_reason)"
    if [[ -z "$validation_failed_reason" ]]; then
        validation_failed_reason="$(extract_last_value "$validation_child_result_file" last_phase_reason)"
    fi
    if [[ -z "$validation_failed_reason" ]]; then
        validation_failed_reason="$(extract_last_value "$validation_child_result_file" first_bad_reason)"
    fi

    validation_failed_step="$(extract_last_value "$validation_child_result_file" confirm_last_phase_failed_step)"
    if [[ -z "$validation_failed_step" ]]; then
        validation_failed_step="$(extract_last_value "$validation_child_result_file" last_phase_failed_step)"
    fi
    if [[ -z "$validation_failed_step" ]]; then
        validation_failed_step="$(extract_last_value "$validation_child_result_file" first_bad_failed_step)"
    fi

    validation_failed_rc="$(extract_last_value "$validation_child_result_file" confirm_last_phase_failed_rc)"
    if [[ -z "$validation_failed_rc" ]]; then
        validation_failed_rc="$(extract_last_value "$validation_child_result_file" last_phase_failed_rc)"
    fi
    if [[ -z "$validation_failed_rc" ]]; then
        validation_failed_rc="$(extract_last_value "$validation_child_result_file" first_bad_failed_rc)"
    fi
fi

if [[ "$validation_rc" -eq 124 && -z "$validation_failed_reason" ]]; then
    validation_failed_reason="validation_timeout"
fi

session_outcome="completed"
if [[ "$state_rc" -ne 0 ]]; then
    session_outcome="state_capture_failed"
fi
if [[ "$validation_rc" -ne 0 ]]; then
    session_outcome="validation_failed"
fi

{
    echo "session_kind=${session_kind}"
    echo "session_outcome=${session_outcome}"
    echo "watcher_rc=${watcher_rc}"
    echo "last_overall=${last_overall:-unknown}"
    echo "final_driver_srcversion=${state_driver_srcversion:-unknown}"
    echo "state_rc=${state_rc}"
    echo "validation_rc=${validation_rc}"
    echo "validation_label=${validation_label}"
    echo "validation_phase_profile_request=${validation_phase_profile_request}"
    echo "validation_phase_selection_request=${validation_phase_selection_request}"
    echo "validation_phases_request=${validation_phases_request}"
    echo "validation_phase_profile_request_original=${validation_phase_profile_request_original}"
    echo "validation_phase_profile_base=${validation_phase_profile_base}"
    echo "validation_phase_profile=${validation_phase_profile}"
    echo "validation_phase_selection=${validation_phase_selection}"
    echo "validation_phases=${validation_phases}"
    vaio_plan_phase_source_emit "validation_phase_source" "validation_phase_source"
    echo "validation_child_session_dir=${validation_child_session_dir}"
    echo "validation_child_result_file=${validation_child_result_file}"
    echo "validation_failed_phase=${validation_failed_phase}"
    echo "validation_failed_reason=${validation_failed_reason}"
    echo "validation_failed_step=${validation_failed_step}"
    echo "validation_failed_rc=${validation_failed_rc}"
    echo "finished_at=$(date -Iseconds)"
} >"$result_file"

if [[ "$state_rc" -ne 0 ]]; then
    exit "$state_rc"
fi

if [[ "$validation_rc" -ne 0 ]]; then
    exit "$validation_rc"
fi
