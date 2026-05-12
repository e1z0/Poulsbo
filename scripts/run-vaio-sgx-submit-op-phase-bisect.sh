#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

session_name="${SESSION_NAME:-sgx-submit-op-phase-bisect}"
session_kind="sgx_submit_op_phase_bisect"
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
  run-vaio-sgx-submit-op-phase-bisect.sh

Behavior:
  Runs the focused typed submit-op path against progressively selected prefix
  stages and uses a prefix bisect to identify the earliest stage whose full
  prefix fails or leaves the target unhealthy. One timestamped session bundle
  is written under logs/vaio-ioctl/.

Environment:
  SESSION_NAME    Session suffix. Default: sgx-submit-op-phase-bisect
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
post_state_rc="unknown"
final_driver_srcversion="unknown"
final_override_module_size="unknown"
session_outcome="interrupted"
last_good_phase="none"
first_bad_phase="none"
first_bad_reason="unknown"
first_bad_failed_step=""
first_bad_failed_rc=""
first_bad_failed_log=""
first_bad_remote_tmpdir=""
authoritative_terminal_bad="0"
authoritative_last_started_phase="none"
authoritative_last_good_phase="none"
authoritative_first_bad_phase="none"
authoritative_first_bad_reason="unknown"

extract_last_value() {
    local file="$1"
    local key="$2"
    awk -F= -v want="$key" '$1 == want { value = substr($0, index($0, "=") + 1) } END { print value }' "$file"
}

extract_log_last_value() {
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
    local probe_order_csv
    if [[ -f "$result_file" ]]; then
        return 0
    fi

    probe_order_csv=""
    if declare -p probe_order >/dev/null 2>&1 && [[ "${#probe_order[@]}" -gt 0 ]]; then
        probe_order_csv="$(IFS=,; printf '%s' "${probe_order[*]}")"
    fi

    {
        echo "session_kind=${session_kind}"
        echo "session_outcome=${session_outcome}"
        echo "phase_profile=${phase_profile}"
        echo "phase_selection=${phase_selection}"
        echo "phases=${phases_csv}"
        echo "preflight_health_rc=${preflight_health_rc:-unknown}"
        echo "preflight_overall=${preflight_overall}"
        echo "pre_state_rc=${pre_state_rc}"
        echo "probe_count=${probe_count:-0}"
        echo "probe_order=${probe_order_csv}"
        echo "last_good_phase=${last_good_phase}"
        echo "first_bad_phase=${first_bad_phase}"
        echo "first_bad_reason=${first_bad_reason}"
        echo "first_bad_failed_step=${first_bad_failed_step}"
        echo "first_bad_failed_rc=${first_bad_failed_rc}"
        echo "first_bad_failed_log=${first_bad_failed_log}"
        echo "first_bad_remote_tmpdir=${first_bad_remote_tmpdir}"
        echo "authoritative_terminal_bad=${authoritative_terminal_bad}"
        echo "authoritative_last_started_phase=${authoritative_last_started_phase}"
        echo "authoritative_last_good_phase=${authoritative_last_good_phase}"
        echo "authoritative_first_bad_phase=${authoritative_first_bad_phase}"
        echo "authoritative_first_bad_reason=${authoritative_first_bad_reason}"
        echo "post_state_rc=${post_state_rc}"
        echo "last_overall=${last_overall}"
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

declare -A probe_status=()
declare -A probe_phase_rc=()
declare -A probe_health_rc=()
declare -A probe_health_overall=()
declare -A probe_reason=()
declare -A probe_failed_step=()
declare -A probe_failed_rc=()
declare -A probe_failed_log=()
declare -A probe_remote_tmpdir=()
declare -A probe_authoritative_terminal=()
probe_count=0
probe_order=()

probe_last_status=""
probe_last_reason=""

run_probe() {
    local idx="$1"
    local phase="${phases[$idx]}"
    local phase_log
    local health_log
    local probe_tag
    local health_overall
    local phase_rc
    local health_rc

    if [[ -n "${probe_status[$idx]:-}" ]]; then
        probe_last_status="${probe_status[$idx]}"
        probe_last_reason="${probe_reason[$idx]}"
        return 0
    fi

    probe_count=$((probe_count + 1))
    probe_tag="$(printf '%02d-%s' "$probe_count" "$phase")"
    phase_log="${session_dir}/${probe_tag}.log"
    health_log="${session_dir}/${probe_tag}-health.log"

    echo "probe=${probe_count} phase=${phase}" >>"$summary_log"
    echo "probe=${probe_count} phase=${phase}"

    run_timed_submit_phase phase_rc "$phase" "$phase_log"

    run_timed_health_probe health_rc "$health_log"
    health_overall="$(extract_last_value "$health_log" "overall")"
    if [[ -z "$health_overall" ]]; then
        health_overall="unknown"
    fi

    if [[ "$phase_rc" -eq 124 ]]; then
        probe_status[$idx]="bad"
        probe_reason[$idx]="phase_timeout"
    elif [[ "$phase_rc" -ne 0 ]]; then
        probe_status[$idx]="bad"
        probe_reason[$idx]="phase_failed"
    elif [[ "$health_rc" -eq 124 ]]; then
        probe_status[$idx]="bad"
        probe_reason[$idx]="health_timeout"
    elif [[ "$health_rc" -ne 0 || "$health_overall" != "remote_usable" ]]; then
        probe_status[$idx]="bad"
        probe_reason[$idx]="phase_unhealthy"
    else
        probe_status[$idx]="good"
        probe_reason[$idx]="ok"
    fi

    probe_phase_rc[$idx]="$phase_rc"
    probe_health_rc[$idx]="$health_rc"
    probe_health_overall[$idx]="$health_overall"
    probe_failed_step[$idx]="$(extract_log_last_value "$phase_log" "FAILED_STEP")"
    probe_failed_rc[$idx]="$(extract_log_last_value "$phase_log" "FAILED_RC")"
    probe_failed_log[$idx]="$(extract_log_last_value "$phase_log" "FAILED_LOG")"
    probe_remote_tmpdir[$idx]="$(extract_log_last_value "$phase_log" "REMOTE_TMPDIR_PRESERVED")"
    probe_authoritative_terminal[$idx]=0
    if [[ "${probe_status[$idx]}" == "bad" && "$health_overall" != "remote_usable" ]]; then
        probe_authoritative_terminal[$idx]=1
    fi
    probe_order+=("$phase")

    {
        echo "probe_${probe_count}_phase=${phase}"
        echo "probe_${probe_count}_status=${probe_status[$idx]}"
        echo "probe_${probe_count}_reason=${probe_reason[$idx]}"
        echo "probe_${probe_count}_phase_rc=${phase_rc}"
        echo "probe_${probe_count}_health_rc=${health_rc}"
        echo "probe_${probe_count}_health=${health_overall}"
        echo "probe_${probe_count}_failed_step=${probe_failed_step[$idx]}"
        echo "probe_${probe_count}_failed_rc=${probe_failed_rc[$idx]}"
        echo "probe_${probe_count}_failed_log=${probe_failed_log[$idx]}"
        echo "probe_${probe_count}_remote_tmpdir=${probe_remote_tmpdir[$idx]}"
        echo "probe_${probe_count}_authoritative_terminal=${probe_authoritative_terminal[$idx]}"
    } >>"$summary_log"

    probe_last_status="${probe_status[$idx]}"
    probe_last_reason="${probe_reason[$idx]}"
}

session_outcome="all_prefixes_healthy"
first_bad_index=-1
last_good_index=-1
first_bad_reason="none"

lo=0
hi=$((${#phases[@]} - 1))
while [[ "$lo" -le "$hi" ]]; do
    mid=$(((lo + hi) / 2))
    run_probe "$mid"
    if [[ "$probe_last_status" == "good" ]]; then
        last_good_index="$mid"
        lo=$((mid + 1))
    else
        first_bad_index="$mid"
        first_bad_reason="$probe_last_reason"
        if [[ "${probe_authoritative_terminal[$mid]}" == "1" ]]; then
            authoritative_terminal_bad="1"
            session_outcome="bisect_terminated_on_unhealthy_bad_probe"
            break
        fi
        hi=$((mid - 1))
        session_outcome="bisect_identified_bad_prefix"
    fi
done

post_state_log="${session_dir}/post-state.log"
run_timed_state_capture post_state_rc "$post_state_log"

final_driver_srcversion="$(extract_last_value "$post_state_log" "driver_srcversion")"
final_override_module_size="$(extract_last_value "$post_state_log" "override_module_size")"
last_overall="$(extract_last_value "$post_state_log" "overall")"
if [[ -z "$last_overall" ]]; then
    last_overall="$preflight_overall"
fi

if [[ "$last_good_index" -ge 0 ]]; then
    last_good_phase="${phases[$last_good_index]}"
else
    last_good_phase="none"
fi

if [[ "$first_bad_index" -ge 0 ]]; then
    first_bad_phase="${phases[$first_bad_index]}"
    first_bad_failed_step="${probe_failed_step[$first_bad_index]}"
    first_bad_failed_rc="${probe_failed_rc[$first_bad_index]}"
    first_bad_failed_log="${probe_failed_log[$first_bad_index]}"
    first_bad_remote_tmpdir="${probe_remote_tmpdir[$first_bad_index]}"
else
    first_bad_phase="none"
    first_bad_failed_step=""
    first_bad_failed_rc=""
    first_bad_failed_log=""
    first_bad_remote_tmpdir=""
fi

if [[ "${#probe_order[@]}" -gt 0 ]]; then
    authoritative_last_started_phase="${probe_order[-1]}"
fi
if [[ "$last_good_phase" != "none" ]]; then
    authoritative_last_good_phase="$last_good_phase"
fi
if [[ "$first_bad_phase" != "none" ]]; then
    authoritative_first_bad_phase="$first_bad_phase"
    authoritative_first_bad_reason="$first_bad_reason"
fi
if [[ "$authoritative_terminal_bad" == "1" ]]; then
    authoritative_last_started_phase="$first_bad_phase"
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
    echo "probe_count=${probe_count}"
    echo "probe_order=$(IFS=,; echo "${probe_order[*]}")"
    echo "last_good_phase=${last_good_phase}"
    echo "first_bad_phase=${first_bad_phase}"
    echo "first_bad_reason=${first_bad_reason}"
    echo "first_bad_failed_step=${first_bad_failed_step}"
    echo "first_bad_failed_rc=${first_bad_failed_rc}"
    echo "first_bad_failed_log=${first_bad_failed_log}"
    echo "first_bad_remote_tmpdir=${first_bad_remote_tmpdir}"
    echo "authoritative_terminal_bad=${authoritative_terminal_bad}"
    echo "authoritative_last_started_phase=${authoritative_last_started_phase}"
    echo "authoritative_last_good_phase=${authoritative_last_good_phase}"
    echo "authoritative_first_bad_phase=${authoritative_first_bad_phase}"
    echo "authoritative_first_bad_reason=${authoritative_first_bad_reason}"
    echo "post_state_rc=${post_state_rc}"
    echo "final_driver_srcversion=${final_driver_srcversion}"
    echo "final_override_module_size=${final_override_module_size}"
    echo "finished_at=$(date -Iseconds)"
} >"$result_file"

cat "$result_file"

if [[ "$session_outcome" != "all_prefixes_healthy" ]]; then
    exit 1
fi
