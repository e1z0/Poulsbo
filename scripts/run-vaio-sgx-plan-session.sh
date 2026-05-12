#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
phase_source_lib="${PHASE_SOURCE_LIB:-${script_dir}/lib-vaio-plan-phase-source.sh}"

session_name="${SESSION_NAME:-sgx-plan-session}"
session_kind="sgx_submit_plan_session"
logs_root="${LOGS_ROOT:-${repo_root}/logs/vaio-plan-session}"
session_timestamp="${SESSION_TIMESTAMP:-$(date +%Y%m%d-%H%M%S)}"
started_at="$(date -Iseconds)"
session_dir="$("${script_dir}/allocate-session-dir.sh" "$logs_root" "$session_name" "$session_timestamp")"
summary_log="${session_dir}/summary.log"
manifest_file="${session_dir}/session.env"
result_file="${session_dir}/session-result.env"
children_root="${session_dir}/children"
health_script="${HEALTH_SCRIPT:-${script_dir}/check-vaio-target-health.sh}"
recovery_script="${RECOVERY_SCRIPT:-${script_dir}/run-vaio-sgx-plan-recovery-session.sh}"
deploy_script="${DEPLOY_SCRIPT:-${script_dir}/run-vaio-sgx-plan-guarded-deploy-session.sh}"
local_module_path="${LOCAL_MODULE:-${repo_root}/build/devnull-vaio-gma500-src/gma500_gfx.ko}"
plan_session_mode="${PLAN_SESSION_MODE:-auto}"
phase_profile_script="${PHASE_PROFILE_SCRIPT:-${script_dir}/resolve-sgx-submit-phase-profile.sh}"
current_phase_script="${CURRENT_PHASE_SCRIPT:-${script_dir}/resolve-vaio-plan-current-phase.sh}"
plan_phase_profile="${PLAN_PHASE_PROFILE:-${PHASE_PROFILE:-auto}}"
plan_phases_override="${PLAN_PHASES:-${PHASES:-}}"

usage() {
    cat <<'EOF'
Usage:
  run-vaio-sgx-plan-session.sh

Behavior:
  Probes current VAIO health and selects one of two existing parent flows:
  recovery investigation or full-cycle guarded deploy. One timestamped bundle
  is written under logs/vaio-plan-session/.

Environment:
  SESSION_NAME       Session suffix. Default: sgx-plan-session
  LOGS_ROOT          Base directory for logs.
  HEALTH_SCRIPT      Override health probe path.
  RECOVERY_SCRIPT    Override plan recovery-session wrapper.
  DEPLOY_SCRIPT      Override plan guarded-deploy wrapper.
  PLAN_SESSION_MODE  One of: auto, recovery, deploy. Default: auto
  PLAN_PHASE_PROFILE One of: auto, current, full, post_descriptor, plan_query,
                     plan_query_fetch, plan_query_basic,
                     plan_query_offset, plan_query_config,
                     plan_query_snapshot, prepare_op, prepare_query,
                     arm, emit, validate, launch, issue, running, complete.
                     Default: auto
  PLAN_PHASES        Explicit plan-specific phase list. Overrides
                     PLAN_PHASE_PROFILE.
  LOCAL_MODULE       Override local module path recorded in the manifest.
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

case "$plan_session_mode" in
    auto|recovery|deploy)
        ;;
    *)
        echo "PLAN_SESSION_MODE must be one of: auto recovery deploy" >&2
        exit 1
        ;;
esac

case "$plan_phase_profile" in
    auto|current|full|post_descriptor|plan_query|plan_query_fetch|plan_query_basic|plan_query_offset|plan_query_config|plan_query_snapshot|prepare_op|prepare_query|arm|emit|validate|launch|issue|running|complete)
        ;;
    *)
        echo "PLAN_PHASE_PROFILE must be one of: auto current full post_descriptor plan_query plan_query_fetch plan_query_basic plan_query_offset plan_query_config plan_query_snapshot prepare_op prepare_query arm emit validate launch issue running complete" >&2
        exit 1
        ;;
esac

for required in "$health_script" "$recovery_script" "$deploy_script" "$phase_profile_script" "$current_phase_script"; do
    if [[ ! -x "$required" ]]; then
        echo "required executable missing: $required" >&2
        exit 1
    fi
done

if [[ ! -f "$phase_source_lib" ]]; then
    echo "phase-source helper is missing: $phase_source_lib" >&2
    exit 1
fi

# shellcheck source=/root/Poulsbo/scripts/lib-vaio-plan-phase-source.sh
source "$phase_source_lib"

{
    echo "timestamp=${session_timestamp}"
    echo "started_at=${started_at}"
    echo "session_kind=${session_kind}"
    echo "session_name=${session_name}"
    echo "session_dir=${session_dir}"
    echo "children_root=${children_root}"
    echo "health_script=${health_script}"
    echo "recovery_script=${recovery_script}"
    echo "deploy_script=${deploy_script}"
    echo "plan_session_mode=${plan_session_mode}"
    echo "plan_phase_profile=${plan_phase_profile}"
    echo "plan_phases_override=${plan_phases_override}"
    echo "phase_profile_script=${phase_profile_script}"
    echo "current_phase_script=${current_phase_script}"
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

preflight_overall="unknown"
selected_mode="unknown"
selected_script=""
selected_rc="unknown"
selected_session_dir=""
selected_result_file=""
selected_outcome="interrupted"
selected_phase_profile_request="$plan_phase_profile"
selected_phase_selection_request="unknown"
selected_phases_request="$plan_phases_override"
selected_phase_profile_base="unknown"
selected_phase_profile="unknown"
selected_phase_selection="unknown"
selected_phases=""
vaio_plan_phase_source_init_vars "selected_phase_source"
selected_child_phase_profile=""
selected_child_phase_selection=""
selected_child_phases=""
last_overall="unknown"
final_driver_srcversion="unknown"
selected_failed_phase=""
selected_failed_reason=""
selected_failed_step=""
selected_failed_rc=""
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
        echo "plan_session_mode=${plan_session_mode}"
        echo "preflight_overall=${preflight_overall}"
        echo "selected_mode=${selected_mode}"
        echo "selected_script=${selected_script}"
        echo "selected_phase_profile_request=${selected_phase_profile_request}"
        echo "selected_phase_selection_request=${selected_phase_selection_request}"
        echo "selected_phases_request=${selected_phases_request}"
        echo "selected_phase_profile_base=${selected_phase_profile_base}"
        echo "selected_phase_profile=${selected_phase_profile}"
        echo "selected_phase_selection=${selected_phase_selection}"
        echo "selected_phases=${selected_phases}"
        vaio_plan_phase_source_emit "selected_phase_source" "selected_phase_source"
        echo "selected_child_phase_profile=${selected_child_phase_profile}"
        echo "selected_child_phase_selection=${selected_child_phase_selection}"
        echo "selected_child_phases=${selected_child_phases}"
        echo "selected_rc=${selected_rc}"
        echo "selected_session_dir=${selected_session_dir}"
        echo "selected_result_file=${selected_result_file}"
        echo "selected_outcome=${selected_outcome}"
        echo "last_overall=${last_overall}"
        echo "final_driver_srcversion=${final_driver_srcversion}"
        echo "selected_failed_phase=${selected_failed_phase}"
        echo "selected_failed_reason=${selected_failed_reason}"
        echo "selected_failed_step=${selected_failed_step}"
        echo "selected_failed_rc=${selected_failed_rc}"
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

health_log="${session_dir}/preflight-health.log"
run_and_capture preflight_health_rc "$health_log" "$health_script"
preflight_overall="$(extract_last_value "$health_log" "overall")"

selected_mode="$plan_session_mode"
if [[ "$plan_session_mode" == "auto" ]]; then
    if [[ "$preflight_overall" == "remote_usable" ]]; then
        selected_mode="deploy"
    else
        selected_mode="recovery"
    fi
fi

selected_script="$recovery_script"
selected_label="recovery"
if [[ "$selected_mode" == "deploy" ]]; then
    selected_script="$deploy_script"
    selected_label="deploy"
fi

selected_phase_profile="$plan_phase_profile"
if [[ "$selected_phase_profile" == "auto" ]]; then
    if [[ "$selected_mode" == "recovery" ]]; then
        selected_phase_profile="current"
    else
        selected_phase_profile="full"
    fi
fi
selected_phase_profile_base="$selected_phase_profile"

if [[ -n "$plan_phases_override" ]]; then
    selected_phase_selection_request="explicit"
    selected_phase_selection="explicit"
    selected_phases="$plan_phases_override"
else
    selected_phase_selection_request="profile"
    selected_phase_selection="profile"
    if [[ "$selected_phase_profile" == "current" ]]; then
        tmp_current="$(mktemp)"
        "$current_phase_script" >"$tmp_current"
        vaio_plan_phase_source_assign_from_resolver_file "$tmp_current" "selected_phase_source"
        selected_phase_profile="$(vaio_kv_last_value "$tmp_current" "resolved_profile")"
        selected_phases="$(vaio_kv_last_value "$tmp_current" "resolved_phases")"
        rm -f "$tmp_current"
    else
        selected_phases="$("$phase_profile_script" "$selected_phase_profile")"
    fi
fi

{
    echo "preflight_overall=${preflight_overall}"
    echo "selected_mode=${selected_mode}"
    echo "selected_script=${selected_script}"
    echo "selected_phase_profile_request=${selected_phase_profile_request}"
    echo "selected_phase_selection_request=${selected_phase_selection_request}"
    echo "selected_phases_request=${selected_phases_request}"
    echo "selected_phase_profile_base=${selected_phase_profile_base}"
    echo "selected_phase_profile=${selected_phase_profile}"
    echo "selected_phase_selection=${selected_phase_selection}"
    echo "selected_phases=${selected_phases}"
    vaio_plan_phase_source_emit "selected_phase_source" "selected_phase_source"
} >>"$summary_log"

run_and_capture selected_rc "${session_dir}/${selected_label}.log" \
    env \
    LOGS_ROOT="$children_root" \
    SESSION_NAME="$selected_label" \
    PLAN_PHASE_PROFILE="$selected_phase_profile" \
    PLAN_PHASES="$selected_phases" \
    PHASE_PROFILE="$selected_phase_profile" \
    PHASES="$selected_phases" \
    "$selected_script"

selected_session_dir="$(child_value "${session_dir}/${selected_label}.log" session_dir)"
selected_result_from_log="$(child_value "${session_dir}/${selected_label}.log" result_file)"
selected_result_file="${selected_result_from_log:-${selected_session_dir}/session-result.env}"
selected_outcome="$(extract_last_value "$selected_result_file" session_outcome)"
last_overall="$(extract_last_value "$selected_result_file" last_overall)"
final_driver_srcversion="$(extract_last_value "$selected_result_file" final_driver_srcversion)"
selected_child_phase_profile="$(extract_last_value "$selected_result_file" validation_phase_profile)"
selected_child_phase_selection="$(extract_last_value "$selected_result_file" validation_phase_selection)"
selected_child_phases="$(extract_last_value "$selected_result_file" validation_phases)"
if [[ -z "$selected_child_phase_profile" ]]; then
    selected_child_phase_profile="$(extract_last_value "$selected_result_file" recovery_phase_profile)"
fi
if [[ -z "$selected_child_phase_selection" ]]; then
    selected_child_phase_selection="$(extract_last_value "$selected_result_file" recovery_phase_selection)"
fi
if [[ -z "$selected_child_phases" ]]; then
    selected_child_phases="$(extract_last_value "$selected_result_file" recovery_phases)"
fi
selected_failed_phase="$(extract_last_value "$selected_result_file" recovery_failed_phase)"
selected_failed_reason="$(extract_last_value "$selected_result_file" recovery_failed_reason)"
selected_failed_step="$(extract_last_value "$selected_result_file" recovery_failed_step)"
selected_failed_rc="$(extract_last_value "$selected_result_file" recovery_failed_rc)"

if [[ -z "$selected_failed_phase" ]]; then
    selected_failed_phase="$(extract_last_value "$selected_result_file" validation_failed_phase)"
fi
if [[ -z "$selected_failed_reason" ]]; then
    selected_failed_reason="$(extract_last_value "$selected_result_file" validation_failed_reason)"
fi
if [[ -z "$selected_failed_step" ]]; then
    selected_failed_step="$(extract_last_value "$selected_result_file" validation_failed_step)"
fi
if [[ -z "$selected_failed_rc" ]]; then
    selected_failed_rc="$(extract_last_value "$selected_result_file" validation_failed_rc)"
fi
if [[ -z "$selected_failed_phase" ]]; then
    selected_failed_phase="$(extract_last_value "$selected_result_file" confirm_last_phase)"
fi
if [[ -z "$selected_failed_reason" ]]; then
    selected_failed_reason="$(extract_last_value "$selected_result_file" confirm_last_phase_reason)"
fi
if [[ -z "$selected_failed_step" ]]; then
    selected_failed_step="$(extract_last_value "$selected_result_file" confirm_last_phase_failed_step)"
fi
if [[ -z "$selected_failed_rc" ]]; then
    selected_failed_rc="$(extract_last_value "$selected_result_file" confirm_last_phase_failed_rc)"
fi

if [[ -z "$last_overall" ]]; then
    last_overall="$preflight_overall"
fi

session_outcome="${selected_outcome:-interrupted}"

{
    echo "selected_rc=${selected_rc}"
    echo "selected_session_dir=${selected_session_dir}"
    echo "selected_result_file=${selected_result_file}"
    echo "selected_outcome=${selected_outcome}"
    echo "selected_phase_profile_request=${selected_phase_profile_request}"
    echo "selected_phase_selection_request=${selected_phase_selection_request}"
    echo "selected_phases_request=${selected_phases_request}"
    echo "selected_phase_profile_base=${selected_phase_profile_base}"
    echo "selected_phase_profile=${selected_phase_profile}"
    echo "selected_phase_selection=${selected_phase_selection}"
    echo "selected_phases=${selected_phases}"
    vaio_plan_phase_source_emit "selected_phase_source" "selected_phase_source"
    echo "selected_child_phase_profile=${selected_child_phase_profile}"
    echo "selected_child_phase_selection=${selected_child_phase_selection}"
    echo "selected_child_phases=${selected_child_phases}"
    echo "last_overall=${last_overall}"
    echo "final_driver_srcversion=${final_driver_srcversion}"
    echo "selected_failed_phase=${selected_failed_phase}"
    echo "selected_failed_reason=${selected_failed_reason}"
    echo "selected_failed_step=${selected_failed_step}"
    echo "selected_failed_rc=${selected_failed_rc}"
} >>"$summary_log"

{
    echo "session_kind=${session_kind}"
    echo "session_outcome=${session_outcome}"
    echo "plan_session_mode=${plan_session_mode}"
    echo "preflight_overall=${preflight_overall}"
    echo "selected_mode=${selected_mode}"
    echo "selected_script=${selected_script}"
    echo "selected_rc=${selected_rc}"
    echo "selected_session_dir=${selected_session_dir}"
    echo "selected_result_file=${selected_result_file}"
    echo "selected_outcome=${selected_outcome}"
    echo "selected_phase_profile_request=${selected_phase_profile_request}"
    echo "selected_phase_selection_request=${selected_phase_selection_request}"
    echo "selected_phases_request=${selected_phases_request}"
    echo "selected_phase_profile_base=${selected_phase_profile_base}"
    echo "selected_phase_profile=${selected_phase_profile}"
    echo "selected_phase_selection=${selected_phase_selection}"
    echo "selected_phases=${selected_phases}"
    vaio_plan_phase_source_emit "selected_phase_source" "selected_phase_source"
    echo "selected_child_phase_profile=${selected_child_phase_profile}"
    echo "selected_child_phase_selection=${selected_child_phase_selection}"
    echo "selected_child_phases=${selected_child_phases}"
    echo "last_overall=${last_overall}"
    echo "final_driver_srcversion=${final_driver_srcversion}"
    echo "selected_failed_phase=${selected_failed_phase}"
    echo "selected_failed_reason=${selected_failed_reason}"
    echo "selected_failed_step=${selected_failed_step}"
    echo "selected_failed_rc=${selected_failed_rc}"
    echo "finished_at=$(date -Iseconds)"
} >"$result_file"

cat "$result_file"
exit "$selected_rc"
