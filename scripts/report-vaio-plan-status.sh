#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
logs_root="${LOGS_ROOT:-${repo_root}/logs}"
health_script="${HEALTH_SCRIPT:-${script_dir}/check-vaio-target-health.sh}"
state_script="${STATE_SCRIPT:-${script_dir}/show-vaio-gma500-state.sh}"
summary_script="${SUMMARY_SCRIPT:-${script_dir}/summarize-vaio-plan-sessions.sh}"
current_phase_script="${CURRENT_PHASE_SCRIPT:-${script_dir}/resolve-vaio-plan-current-phase.sh}"
summary_lib="${SUMMARY_LIB:-${script_dir}/lib-vaio-plan-summary.sh}"
phase_source_lib="${PHASE_SOURCE_LIB:-${script_dir}/lib-vaio-plan-phase-source.sh}"
unsafe_plan_query_srcversion="${UNSAFE_PLAN_QUERY_SRCVERSION:-0EEEE302E3FDA9F59B7CFB9}"

usage() {
    cat <<'EOF'
Usage:
  report-vaio-plan-status.sh

Behavior:
  Prints one current-state report for the SGX `plan` investigation line. It
  combines live target health with the most relevant saved `plan` bundles:

  - latest explicit-window blocked session
  - latest confirmed live failure
  - latest remote-usable interrupted session
  - latest remote-usable phase-evidence session
  - latest guarded-deploy parent

Environment:
  LOGS_ROOT       Base logs directory. Default: <repo>/logs
  HEALTH_SCRIPT   Override target health probe path.
  STATE_SCRIPT    Override remote-state probe path.
  SUMMARY_SCRIPT  Override plan-session summary helper path.
  CURRENT_PHASE_SCRIPT  Override current-boundary resolver path.
  SUMMARY_LIB     Override shared plan-summary helper path.
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

for required in "$health_script" "$state_script" "$summary_script" "$current_phase_script"; do
    if [[ ! -x "$required" ]]; then
        echo "required executable missing: $required" >&2
        exit 1
    fi
done

for helper in "$summary_lib" "$phase_source_lib"; do
    if [[ ! -f "$helper" ]]; then
        echo "required helper missing: $helper" >&2
        exit 1
    fi
done

# shellcheck source=/root/Poulsbo/scripts/lib-vaio-plan-summary.sh
source "$summary_lib"
# shellcheck source=/root/Poulsbo/scripts/lib-vaio-plan-phase-source.sh
source "$phase_source_lib"

tmp_summary="$(mktemp)"
tmp_health="$(mktemp)"
tmp_state="$(mktemp)"
tmp_current="$(mktemp)"
trap 'rm -f "$tmp_summary" "$tmp_health" "$tmp_state" "$tmp_current"' EXIT

get_value() {
    local file="$1"
    local key="$2"
    vaio_kv_last_value "$file" "$key"
}

run_and_capture() {
    local outfile="$1"
    shift
    set +e
    "$@" >"$outfile" 2>&1
    local rc=$?
    set -e
    cat "$outfile"
    return "$rc"
}

phase_list_contains_plan_query() {
    local phases_csv="$1"
    local phase
    local old_ifs="$IFS"

    IFS=','
    for phase in $phases_csv; do
        case "$phase" in
            plan_query|plan_query_fetch|plan_query_basic|plan_query_offset|plan_query_config|plan_query_snapshot)
                IFS="$old_ifs"
                return 0
                ;;
        esac
    done
    IFS="$old_ifs"
    return 1
}

emit_session_block() {
    local prefix="$1"
    local row="$2"

    if [[ -z "$row" ]]; then
        echo "${prefix}_present=0"
        return 0
    fi

    echo "${prefix}_present=1"
    echo "${prefix}_kind=$(vaio_plan_summary_pick_field "$row" "kind")"
    echo "${prefix}_timestamp=$(vaio_plan_summary_pick_field "$row" "timestamp")"
    echo "${prefix}_name=$(vaio_plan_summary_pick_field "$row" "name")"
    echo "${prefix}_outcome=$(vaio_plan_summary_pick_field "$row" "outcome")"
    echo "${prefix}_last_state=$(vaio_plan_summary_pick_field "$row" "last_state")"
    echo "${prefix}_requested=$(vaio_plan_summary_pick_field "$row" "requested")"
    echo "${prefix}_resolved=$(vaio_plan_summary_pick_field "$row" "resolved")"
    echo "${prefix}_child=$(vaio_plan_summary_pick_field "$row" "child")"
    echo "${prefix}_phase_source=$(vaio_plan_summary_pick_field "$row" "phase_source")"
    echo "${prefix}_phase_source_reason=$(vaio_plan_summary_pick_field "$row" "phase_source_reason")"
    echo "${prefix}_phase_source_kind=$(vaio_plan_summary_pick_field "$row" "phase_source_kind")"
    echo "${prefix}_phase_source_name=$(vaio_plan_summary_pick_field "$row" "phase_source_name")"
    echo "${prefix}_phase_source_timestamp=$(vaio_plan_summary_pick_field "$row" "phase_source_timestamp")"
    echo "${prefix}_phase_source_session_dir=$(vaio_plan_summary_pick_field "$row" "phase_source_session_dir")"
    echo "${prefix}_failed_phase=$(vaio_plan_summary_pick_field "$row" "failed_phase")"
    echo "${prefix}_failed_step=$(vaio_plan_summary_pick_field "$row" "failed_step")"
    echo "${prefix}_module_srcversion=$(vaio_plan_summary_pick_field "$row" "module_srcversion")"
    echo "${prefix}_session_dir=$(vaio_plan_summary_pick_field "$row" "session_dir")"
    echo "${prefix}_evidence_last_started=$(vaio_plan_summary_pick_field "$row" "evidence_last_started")"
    echo "${prefix}_evidence_last_good=$(vaio_plan_summary_pick_field "$row" "evidence_last_good")"
    echo "${prefix}_evidence_first_bad=$(vaio_plan_summary_pick_field "$row" "evidence_first_bad")"
    echo "${prefix}_evidence_reason=$(vaio_plan_summary_pick_field "$row" "evidence_reason")"
}

OUTPUT_FORMAT=tsv "$summary_script" >"$tmp_summary"
vaio_plan_summary_init_columns "$tmp_summary"
run_and_capture "$tmp_health" "$health_script" || true

current_overall="$(get_value "$tmp_health" "overall")"
ping_status="$(get_value "$tmp_health" "ping_status")"
tcp_22_status="$(get_value "$tmp_health" "tcp_22_status")"
ssh_pubkey_status="$(get_value "$tmp_health" "ssh_pubkey_status")"

latest_explicit_row="$(
    vaio_plan_summary_pick_latest_session \
        "$tmp_summary" \
        $'^plan_session\t.*\tblocked_before_remote_usable\t.*\t.*:explicit\\['
)"
latest_confirmed_failure_row="$(
    vaio_plan_summary_pick_latest_session \
        "$tmp_summary" \
        $'^plan_session\t.*\tvalidation_failed\tremote_usable\t.*\t.*\tsync\t'
)"
latest_remote_usable_interrupted_row="$(
    vaio_plan_summary_pick_latest_session \
        "$tmp_summary" \
        $'^plan_session\t.*\tinterrupted(_or_incomplete)?\tremote_usable\t'
)"
latest_remote_usable_phase_evidence_row="$(vaio_plan_summary_pick_latest_phase_evidence_row "$tmp_summary")"
latest_plan_deploy_row="$(
    vaio_plan_summary_pick_latest_session \
        "$tmp_summary" \
        $'^plan_deploy\t'
)"

latest_remote_usable_interrupted_session_dir=""
latest_remote_usable_interrupted_bisect_summary=""
latest_remote_usable_interrupted_last_started_phase=""
latest_remote_usable_interrupted_effective_evidence=""
latest_remote_usable_interrupted_last_good_phase=""
latest_remote_usable_phase_evidence_session_dir=""
latest_remote_usable_phase_evidence_bisect_summary=""
latest_remote_usable_phase_evidence_bisect_result=""
latest_remote_usable_phase_evidence_effective_evidence=""
latest_remote_usable_phase_evidence_last_started_phase=""
latest_remote_usable_phase_evidence_last_good_phase=""
latest_remote_usable_phase_evidence_first_bad_phase=""
latest_remote_usable_phase_evidence_first_bad_reason=""
latest_remote_usable_phase_evidence_first_bad_failed_step=""
current_recovery_profile=""
current_recovery_phases=""
current_recovery_phase=""
current_recovery_reason=""
vaio_plan_phase_source_init_vars "current_recovery_source"
current_recovery_source_row=""
current_live_driver_srcversion="unknown"
current_live_driver_module_loaded="unknown"
current_live_drm_card0_present="unknown"
current_live_lightdm_active="unknown"
current_live_xorg_process_count="unknown"
current_recovery_live_module_safe="unknown"
current_recovery_live_module_warning=""
current_recovery_effective_module_srcversion=""

echo "current_overall=${current_overall:-unknown}"
echo "current_ping_status=${ping_status:-unknown}"
echo "current_tcp_22_status=${tcp_22_status:-unknown}"
echo "current_ssh_pubkey_status=${ssh_pubkey_status:-unknown}"
emit_session_block "latest_explicit_window" "$latest_explicit_row"
emit_session_block "latest_confirmed_failure" "$latest_confirmed_failure_row"
emit_session_block "latest_remote_usable_interrupted" "$latest_remote_usable_interrupted_row"
emit_session_block "latest_remote_usable_phase_evidence" "$latest_remote_usable_phase_evidence_row"
emit_session_block "latest_plan_deploy" "$latest_plan_deploy_row"

if [[ -n "$latest_remote_usable_interrupted_row" ]]; then
    latest_remote_usable_interrupted_session_dir="$(vaio_plan_summary_pick_field "$latest_remote_usable_interrupted_row" "session_dir")"
    latest_remote_usable_interrupted_bisect_summary="$(vaio_plan_latest_bisect_summary "$latest_remote_usable_interrupted_session_dir" "$repo_root")"
    latest_remote_usable_interrupted_effective_evidence="$(vaio_plan_extract_effective_bisect_evidence "$latest_remote_usable_interrupted_bisect_summary" "$(vaio_plan_latest_bisect_result "$latest_remote_usable_interrupted_bisect_summary")")"
    latest_remote_usable_interrupted_last_started_phase="$(awk -F= '$1 == "last_started" { print substr($0, index($0, "=") + 1); exit }' <<<"$latest_remote_usable_interrupted_effective_evidence")"
    latest_remote_usable_interrupted_last_good_phase="$(awk -F= '$1 == "last_good" { print substr($0, index($0, "=") + 1); exit }' <<<"$latest_remote_usable_interrupted_effective_evidence")"
fi

if [[ -n "$latest_remote_usable_phase_evidence_row" ]]; then
    latest_remote_usable_phase_evidence_session_dir="$(vaio_plan_summary_pick_field "$latest_remote_usable_phase_evidence_row" "session_dir")"
    latest_remote_usable_phase_evidence_bisect_summary="$(vaio_plan_latest_bisect_summary "$latest_remote_usable_phase_evidence_session_dir" "$repo_root")"
    latest_remote_usable_phase_evidence_bisect_result="$(vaio_plan_latest_bisect_result "$latest_remote_usable_phase_evidence_bisect_summary")"
    latest_remote_usable_phase_evidence_effective_evidence="$(vaio_plan_extract_effective_bisect_evidence "$latest_remote_usable_phase_evidence_bisect_summary" "$latest_remote_usable_phase_evidence_bisect_result")"
    latest_remote_usable_phase_evidence_last_started_phase="$(awk -F= '$1 == "last_started" { print substr($0, index($0, "=") + 1); exit }' <<<"$latest_remote_usable_phase_evidence_effective_evidence")"
    latest_remote_usable_phase_evidence_last_good_phase="$(awk -F= '$1 == "last_good" { print substr($0, index($0, "=") + 1); exit }' <<<"$latest_remote_usable_phase_evidence_effective_evidence")"
    latest_remote_usable_phase_evidence_first_bad_phase="$(awk -F= '$1 == "first_bad" { print substr($0, index($0, "=") + 1); exit }' <<<"$latest_remote_usable_phase_evidence_effective_evidence")"
    latest_remote_usable_phase_evidence_first_bad_reason="$(awk -F= '$1 == "reason" { print substr($0, index($0, "=") + 1); exit }' <<<"$latest_remote_usable_phase_evidence_effective_evidence")"
    latest_remote_usable_phase_evidence_first_bad_failed_step="$(get_value "$latest_remote_usable_phase_evidence_bisect_result" "first_bad_failed_step")"
fi

echo "latest_remote_usable_interrupted_bisect_summary=${latest_remote_usable_interrupted_bisect_summary}"
echo "latest_remote_usable_interrupted_last_started_phase=${latest_remote_usable_interrupted_last_started_phase}"
echo "latest_remote_usable_interrupted_last_good_phase=${latest_remote_usable_interrupted_last_good_phase}"
echo "latest_remote_usable_phase_evidence_bisect_summary=${latest_remote_usable_phase_evidence_bisect_summary}"
echo "latest_remote_usable_phase_evidence_bisect_result=${latest_remote_usable_phase_evidence_bisect_result}"
echo "latest_remote_usable_phase_evidence_last_started_phase=${latest_remote_usable_phase_evidence_last_started_phase}"
echo "latest_remote_usable_phase_evidence_last_good_phase=${latest_remote_usable_phase_evidence_last_good_phase}"
echo "latest_remote_usable_phase_evidence_first_bad_phase=${latest_remote_usable_phase_evidence_first_bad_phase}"
echo "latest_remote_usable_phase_evidence_first_bad_reason=${latest_remote_usable_phase_evidence_first_bad_reason}"
echo "latest_remote_usable_phase_evidence_first_bad_failed_step=${latest_remote_usable_phase_evidence_first_bad_failed_step}"

"$current_phase_script" >"$tmp_current"
current_recovery_profile="$(vaio_kv_last_value "$tmp_current" "resolved_profile")"
current_recovery_phases="$(vaio_kv_last_value "$tmp_current" "resolved_phases")"
current_recovery_phase="$(vaio_kv_last_value "$tmp_current" "resolved_phase")"
current_recovery_reason="$(vaio_kv_last_value "$tmp_current" "resolved_reason")"
vaio_plan_phase_source_assign_from_resolver_file "$tmp_current" "current_recovery_source"

echo "current_recovery_profile=${current_recovery_profile}"
echo "current_recovery_phases=${current_recovery_phases}"
echo "current_recovery_phase=${current_recovery_phase}"
echo "current_recovery_reason=${current_recovery_reason}"
echo "current_recovery_source_kind=${current_recovery_source_kind}"
echo "current_recovery_source_name=${current_recovery_source_name}"
echo "current_recovery_source_timestamp=${current_recovery_source_timestamp}"
echo "current_recovery_source_session_dir=${current_recovery_source_session_dir}"

if vaio_plan_phase_source_row_present "current_recovery_source"; then
    echo "current_recovery_source_row_present=1"
    echo "current_recovery_source_row_kind=${current_recovery_source_kind}"
    echo "current_recovery_source_row_timestamp=${current_recovery_source_timestamp}"
    echo "current_recovery_source_row_name=${current_recovery_source_name}"
    echo "current_recovery_source_row_outcome=${current_recovery_source_row_outcome}"
    echo "current_recovery_source_row_last_state=${current_recovery_source_row_last_state}"
    echo "current_recovery_source_row_requested=${current_recovery_source_row_requested}"
    echo "current_recovery_source_row_resolved=${current_recovery_source_row_resolved}"
    echo "current_recovery_source_row_child=${current_recovery_source_row_child}"
    echo "current_recovery_source_row_phase_source=${current_recovery_source_row_phase_source}"
    echo "current_recovery_source_row_phase_source_reason=${current_recovery_reason}"
    echo "current_recovery_source_row_phase_source_kind=${current_recovery_source_kind}"
    echo "current_recovery_source_row_phase_source_name=${current_recovery_source_name}"
    echo "current_recovery_source_row_phase_source_timestamp=${current_recovery_source_timestamp}"
    echo "current_recovery_source_row_phase_source_session_dir=${current_recovery_source_session_dir}"
    echo "current_recovery_source_row_failed_phase=${current_recovery_source_row_failed_phase}"
    echo "current_recovery_source_row_failed_step=${current_recovery_source_row_failed_step}"
    echo "current_recovery_source_row_module_srcversion=${current_recovery_source_row_module_srcversion}"
    echo "current_recovery_source_row_session_dir=${current_recovery_source_session_dir}"
    echo "current_recovery_source_row_evidence_last_started=${current_recovery_source_row_evidence_last_started}"
    echo "current_recovery_source_row_evidence_last_good=${current_recovery_source_row_evidence_last_good}"
    echo "current_recovery_source_row_evidence_first_bad=${current_recovery_source_row_evidence_first_bad}"
    echo "current_recovery_source_row_evidence_reason=${current_recovery_source_row_evidence_reason}"
elif [[ -n "$current_recovery_source_kind" && -n "$current_recovery_source_timestamp" && -n "$current_recovery_source_name" ]]; then
    current_recovery_source_row="$(
        vaio_plan_summary_find_row \
            "$tmp_summary" \
            "kind=${current_recovery_source_kind}" \
            "timestamp=${current_recovery_source_timestamp}" \
            "name=${current_recovery_source_name}"
    )"
fi
if [[ -z "$current_recovery_source_row" && -n "$current_recovery_source_session_dir" ]] \
    && ! vaio_plan_phase_source_row_present "current_recovery_source"; then
    current_recovery_source_row="$(
        vaio_plan_summary_find_row \
            "$tmp_summary" \
            "session_dir=${current_recovery_source_session_dir}"
    )"
fi

if ! vaio_plan_phase_source_row_present "current_recovery_source"; then
    emit_session_block "current_recovery_source_row" "$current_recovery_source_row"
fi

if [[ "${current_overall}" == "remote_usable" ]]; then
    if run_and_capture "$tmp_state" "$state_script"; then
        current_live_driver_srcversion="$(get_value "$tmp_state" "driver_srcversion")"
        current_live_driver_module_loaded="$(get_value "$tmp_state" "driver_module_loaded")"
        current_live_drm_card0_present="$(get_value "$tmp_state" "drm_card0_present")"
        current_live_lightdm_active="$(get_value "$tmp_state" "lightdm_active")"
        current_live_xorg_process_count="$(get_value "$tmp_state" "xorg_process_count")"
    fi
fi

current_recovery_effective_module_srcversion="$current_live_driver_srcversion"
if [[ -z "$current_recovery_effective_module_srcversion" || \
      "$current_recovery_effective_module_srcversion" == "unknown" ]]; then
    current_recovery_effective_module_srcversion="$current_recovery_source_row_module_srcversion"
fi

if [[ "$current_recovery_effective_module_srcversion" == "$unsafe_plan_query_srcversion" ]] && \
    phase_list_contains_plan_query "$current_recovery_phases"; then
    current_recovery_live_module_safe=0
    current_recovery_live_module_warning="known_unsafe_plan_query_live_module"
else
    current_recovery_live_module_safe=1
fi

echo "current_live_driver_srcversion=${current_live_driver_srcversion:-unknown}"
echo "current_live_driver_module_loaded=${current_live_driver_module_loaded:-unknown}"
echo "current_live_drm_card0_present=${current_live_drm_card0_present:-unknown}"
echo "current_live_lightdm_active=${current_live_lightdm_active:-unknown}"
echo "current_live_xorg_process_count=${current_live_xorg_process_count:-unknown}"
echo "current_recovery_effective_module_srcversion=${current_recovery_effective_module_srcversion:-unknown}"
echo "current_recovery_live_module_safe=${current_recovery_live_module_safe}"
echo "current_recovery_live_module_warning=${current_recovery_live_module_warning}"

echo "recommended_offline_summary_command=MAX_SESSIONS=10 ./scripts/summarize-vaio-plan-sessions.sh"
echo "recommended_next_phase=${current_recovery_phase}"
echo "recommended_next_phase_profile=${current_recovery_profile}"
if [[ -n "${current_recovery_phases}" ]]; then
    echo "recommended_next_phases=${current_recovery_phases}"
fi
if [[ "${current_recovery_live_module_safe}" == "0" ]]; then
    if [[ -n "${current_recovery_phases}" ]]; then
        echo "recommended_next_command=PLAN_SESSION_MODE=deploy PLAN_PHASES=${current_recovery_phases} ./scripts/run-vaio-sgx-plan-session.sh"
    else
        echo "recommended_next_command=PLAN_SESSION_MODE=deploy ./scripts/run-vaio-sgx-plan-session.sh"
    fi
    if [[ -n "${current_recovery_profile}" ]]; then
        echo "recommended_next_profile_command=PLAN_SESSION_MODE=deploy PLAN_PHASE_PROFILE=${current_recovery_profile} ./scripts/run-vaio-sgx-plan-session.sh"
    fi
    echo "recommended_next_guarded_deploy_command=./scripts/run-vaio-sgx-plan-guarded-deploy-session.sh"
elif [[ -n "${current_recovery_phases}" && "${current_recovery_phases}" != "${current_recovery_phase}" ]]; then
    echo "recommended_next_command=PLAN_SESSION_MODE=recovery PLAN_PHASES=${current_recovery_phases} ./scripts/run-vaio-sgx-plan-session.sh"
elif [[ -n "${current_recovery_phase}" ]]; then
    echo "recommended_next_command=PLAN_SESSION_MODE=recovery PLAN_PHASES=${current_recovery_phase} ./scripts/run-vaio-sgx-plan-session.sh"
else
    echo "recommended_next_command=PLAN_SESSION_MODE=recovery ./scripts/run-vaio-sgx-plan-session.sh"
fi
if [[ "${current_recovery_live_module_safe}" != "0" && -n "${current_recovery_profile}" ]]; then
    echo "recommended_next_profile_command=PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=${current_recovery_profile} ./scripts/run-vaio-sgx-plan-session.sh"
fi
