#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

session_name="${SESSION_NAME:-sgx-plan-guarded-deploy}"
session_kind="sgx_submit_plan_guarded_deploy"
logs_root="${LOGS_ROOT:-${repo_root}/logs/vaio-plan}"
session_timestamp="${SESSION_TIMESTAMP:-$(date +%Y%m%d-%H%M%S)}"
started_at="$(date -Iseconds)"
session_dir="$("${script_dir}/allocate-session-dir.sh" "$logs_root" "$session_name" "$session_timestamp")"
summary_log="${session_dir}/summary.log"
manifest_file="${session_dir}/session.env"
result_file="${session_dir}/session-result.env"
children_root="${session_dir}/children"
deploy_script="${DEPLOY_SCRIPT:-${script_dir}/run-vaio-guarded-deploy-session.sh}"
recovery_script="${RECOVERY_SCRIPT:-${script_dir}/run-vaio-sgx-plan-recovery-session.sh}"
local_module_path="${LOCAL_MODULE:-${repo_root}/build/devnull-vaio-gma500-src/gma500_gfx.ko}"
phase_profile="${PLAN_PHASE_PROFILE:-${PHASE_PROFILE:-full}}"
phases_override="${PLAN_PHASES:-${PHASES:-}}"
phase_selection="profile"
if [[ -n "$phases_override" ]]; then
    phase_selection="explicit"
fi

usage() {
    cat <<'EOF'
Usage:
  run-vaio-sgx-plan-guarded-deploy-session.sh

Behavior:
  Runs one guarded deploy session and, only if that succeeds, immediately runs
  the SGX submit `plan` recovery-session investigation. One parent bundle is
  written under logs/vaio-plan/.

Environment:
  SESSION_NAME    Session suffix. Default: sgx-plan-guarded-deploy
  LOGS_ROOT       Base directory for logs.
  DEPLOY_SCRIPT   Override guarded deploy-session wrapper.
  RECOVERY_SCRIPT Override SGX plan recovery-session wrapper.
  LOCAL_MODULE    Override local module path recorded in the manifest.
  PLAN_PHASE_PROFILE
                  Plan-specific recovery-side phase profile. Default: full
  PLAN_PHASES     Plan-specific explicit phase list. Overrides
                  PLAN_PHASE_PROFILE.
  PHASES          Explicit phase list forwarded to the recovery child.
                  PLAN_PHASES is the preferred alias.
  PHASE_PROFILE   Recovery-side submit phase profile. Default: full
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

for required in "$deploy_script" "$recovery_script"; do
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
    echo "children_root=${children_root}"
    echo "deploy_script=${deploy_script}"
    echo "recovery_script=${recovery_script}"
    echo "phase_profile=${phase_profile}"
    echo "phase_selection=${phase_selection}"
    echo "phases_override=${phases_override}"
    echo "local_module_path=${local_module_path}"
    if [[ -f "$local_module_path" ]]; then
        echo "local_module_srcversion=$(modinfo -F srcversion "$local_module_path" 2>/dev/null || echo unknown)"
    else
        echo "local_module_srcversion=missing"
    fi
} >"$manifest_file"

echo "session_dir=${session_dir}"
echo "summary_log=${summary_log}"
echo "result_file=${result_file}"

deploy_rc="unknown"
deploy_session_dir=""
deploy_result_file=""
deploy_outcome="interrupted"
deploy_last_state="unknown"
deploy_final_driver_srcversion="unknown"
recovery_rc="skipped"
recovery_session_dir="skipped"
recovery_result_file="skipped"
recovery_outcome="skipped"
recovery_last_state="skipped"
recovery_final_driver_srcversion="skipped"
recovery_requested_phase_profile="${phase_profile}"
recovery_requested_phase_selection="${phase_selection}"
recovery_requested_phases="${phases_override}"
recovery_phase_profile="skipped"
recovery_phase_selection="skipped"
recovery_phases="skipped"
recovery_failed_step="skipped"
recovery_failed_rc="skipped"
recovery_failed_phase="skipped"
recovery_failed_reason="skipped"
session_outcome="interrupted"
last_overall="unknown"
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
        echo "deploy_rc=${deploy_rc}"
        echo "deploy_session_dir=${deploy_session_dir}"
        echo "deploy_result_file=${deploy_result_file}"
        echo "deploy_outcome=${deploy_outcome}"
        echo "recovery_rc=${recovery_rc}"
        echo "recovery_session_dir=${recovery_session_dir}"
        echo "recovery_result_file=${recovery_result_file}"
        echo "recovery_outcome=${recovery_outcome}"
        echo "recovery_requested_phase_profile=${recovery_requested_phase_profile}"
        echo "recovery_requested_phase_selection=${recovery_requested_phase_selection}"
        echo "recovery_requested_phases=${recovery_requested_phases}"
        echo "recovery_phase_profile=${recovery_phase_profile}"
        echo "recovery_phase_selection=${recovery_phase_selection}"
        echo "recovery_phases=${recovery_phases}"
        echo "recovery_failed_phase=${recovery_failed_phase}"
        echo "recovery_failed_reason=${recovery_failed_reason}"
        echo "recovery_failed_step=${recovery_failed_step}"
        echo "recovery_failed_rc=${recovery_failed_rc}"
        echo "last_overall=${last_overall}"
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

run_and_capture deploy_rc "${session_dir}/deploy.log" \
    env \
    LOGS_ROOT="$children_root" \
    SESSION_NAME="guarded-deploy" \
    "$deploy_script"

deploy_session_dir="$(child_value "${session_dir}/deploy.log" session_dir)"
deploy_result_from_log="$(child_value "${session_dir}/deploy.log" result_file)"
deploy_result_file="${deploy_result_from_log:-${deploy_session_dir}/session-result.env}"
deploy_outcome="$(extract_last_value "$deploy_result_file" session_outcome)"
deploy_last_state="$(extract_last_value "$deploy_result_file" last_overall)"
if [[ -z "$deploy_last_state" ]]; then
    deploy_last_state="$(extract_last_value "$deploy_result_file" preflight_overall)"
fi
deploy_final_driver_srcversion="$(extract_last_value "$deploy_result_file" final_driver_srcversion)"

{
    echo "deploy_rc=${deploy_rc}"
    echo "deploy_session_dir=${deploy_session_dir}"
    echo "deploy_result_file=${deploy_result_file}"
    echo "deploy_outcome=${deploy_outcome}"
    echo "deploy_last_state=${deploy_last_state}"
    echo "deploy_final_driver_srcversion=${deploy_final_driver_srcversion}"
} >>"$summary_log"

session_outcome="deploy_failed_or_blocked"
last_overall="${deploy_last_state:-}"
final_driver_srcversion="${deploy_final_driver_srcversion:-}"

if [[ "$deploy_outcome" == "completed" ]]; then
    run_and_capture recovery_rc "${session_dir}/recovery.log" \
        env \
        LOGS_ROOT="$children_root" \
        SESSION_NAME="plan-recovery" \
        PLAN_PHASE_PROFILE="$phase_profile" \
        PLAN_PHASES="$phases_override" \
        PHASE_PROFILE="$phase_profile" \
        PHASES="$phases_override" \
        "$recovery_script"

    recovery_session_dir="$(child_value "${session_dir}/recovery.log" session_dir)"
    recovery_result_from_log="$(child_value "${session_dir}/recovery.log" result_file)"
    recovery_result_file="${recovery_result_from_log:-${recovery_session_dir}/session-result.env}"
    recovery_outcome="$(extract_last_value "$recovery_result_file" session_outcome)"
    recovery_last_state="$(extract_last_value "$recovery_result_file" last_overall)"
    recovery_final_driver_srcversion="$(extract_last_value "$recovery_result_file" final_driver_srcversion)"
    recovery_requested_phase_profile="$(extract_last_value "$recovery_result_file" validation_phase_profile_request)"
    recovery_requested_phase_selection="$(extract_last_value "$recovery_result_file" validation_phase_selection_request)"
    recovery_requested_phases="$(extract_last_value "$recovery_result_file" validation_phases_request)"
    recovery_phase_profile="$(extract_last_value "$recovery_result_file" validation_phase_profile)"
    recovery_phase_selection="$(extract_last_value "$recovery_result_file" validation_phase_selection)"
    recovery_phases="$(extract_last_value "$recovery_result_file" validation_phases)"
    if [[ -z "$recovery_requested_phase_profile" ]]; then
        recovery_requested_phase_profile="$phase_profile"
    fi
    if [[ -z "$recovery_requested_phase_selection" ]]; then
        recovery_requested_phase_selection="$phase_selection"
    fi
    if [[ -z "$recovery_requested_phases" ]]; then
        recovery_requested_phases="$phases_override"
    fi
    recovery_failed_step="$(extract_last_value "$recovery_result_file" confirm_last_phase_failed_step)"
    recovery_failed_rc="$(extract_last_value "$recovery_result_file" confirm_last_phase_failed_rc)"
    recovery_failed_phase="$(extract_last_value "$recovery_result_file" confirm_last_phase)"
    recovery_failed_reason="$(extract_last_value "$recovery_result_file" confirm_last_phase_reason)"
    if [[ -z "$recovery_failed_phase" ]]; then
        recovery_failed_phase="$(extract_last_value "$recovery_result_file" validation_failed_phase)"
    fi
    if [[ -z "$recovery_failed_reason" ]]; then
        recovery_failed_reason="$(extract_last_value "$recovery_result_file" validation_failed_reason)"
    fi
    if [[ -z "$recovery_failed_step" ]]; then
        recovery_failed_step="$(extract_last_value "$recovery_result_file" validation_failed_step)"
    fi
    if [[ -z "$recovery_failed_rc" ]]; then
        recovery_failed_rc="$(extract_last_value "$recovery_result_file" validation_failed_rc)"
    fi

    {
        echo "recovery_rc=${recovery_rc}"
        echo "recovery_session_dir=${recovery_session_dir}"
        echo "recovery_result_file=${recovery_result_file}"
        echo "recovery_outcome=${recovery_outcome}"
        echo "recovery_requested_phase_profile=${recovery_requested_phase_profile}"
        echo "recovery_requested_phase_selection=${recovery_requested_phase_selection}"
        echo "recovery_requested_phases=${recovery_requested_phases}"
        echo "recovery_last_state=${recovery_last_state}"
        echo "recovery_final_driver_srcversion=${recovery_final_driver_srcversion}"
        echo "recovery_phase_profile=${recovery_phase_profile}"
        echo "recovery_phase_selection=${recovery_phase_selection}"
        echo "recovery_phases=${recovery_phases}"
        echo "recovery_failed_phase=${recovery_failed_phase}"
        echo "recovery_failed_reason=${recovery_failed_reason}"
        echo "recovery_failed_step=${recovery_failed_step}"
        echo "recovery_failed_rc=${recovery_failed_rc}"
    } >>"$summary_log"

    session_outcome="recovery_completed"
    if [[ "$recovery_outcome" != "completed" ]]; then
        session_outcome="recovery_blocked_or_failed"
    fi
    last_overall="${recovery_last_state:-$last_overall}"
    final_driver_srcversion="${recovery_final_driver_srcversion:-$final_driver_srcversion}"
    exit_code=0
    if [[ "$recovery_rc" -ne 0 ]]; then
        exit_code=1
    fi
fi

{
    echo "session_kind=${session_kind}"
    echo "session_outcome=${session_outcome}"
    echo "deploy_rc=${deploy_rc}"
    echo "deploy_session_dir=${deploy_session_dir}"
    echo "deploy_result_file=${deploy_result_file}"
    echo "deploy_outcome=${deploy_outcome}"
    echo "recovery_rc=${recovery_rc}"
    echo "recovery_session_dir=${recovery_session_dir}"
    echo "recovery_result_file=${recovery_result_file}"
    echo "recovery_outcome=${recovery_outcome}"
    echo "recovery_requested_phase_profile=${recovery_requested_phase_profile}"
    echo "recovery_requested_phase_selection=${recovery_requested_phase_selection}"
    echo "recovery_requested_phases=${recovery_requested_phases}"
    echo "recovery_phase_profile=${recovery_phase_profile}"
    echo "recovery_phase_selection=${recovery_phase_selection}"
    echo "recovery_phases=${recovery_phases}"
    echo "recovery_failed_phase=${recovery_failed_phase}"
    echo "recovery_failed_reason=${recovery_failed_reason}"
    echo "recovery_failed_step=${recovery_failed_step}"
    echo "recovery_failed_rc=${recovery_failed_rc}"
    echo "last_overall=${last_overall}"
    echo "final_driver_srcversion=${final_driver_srcversion}"
    echo "finished_at=$(date -Iseconds)"
} >"$result_file"

cat "$result_file"
exit "$exit_code"
