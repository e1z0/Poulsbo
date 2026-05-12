#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

session_name="${SESSION_NAME:-sgx-multi-submit-mode-compare}"
session_kind="${SESSION_KIND:-sgx_multi_submit_mode_compare}"
logs_root="${LOGS_ROOT:-${repo_root}/logs/vaio-compare}"
timestamp="$(date +%Y%m%d-%H%M%S)"
started_at="$(date -Iseconds)"
session_dir="${logs_root}/${timestamp}-${session_name}"
preflight_log="${session_dir}/preflight-health.log"
pre_state_log="${session_dir}/pre-state.log"
typed_log="${session_dir}/multi-submit-typed.log"
typed_health_log="${session_dir}/typed-health.log"
legacy_log="${session_dir}/multi-submit-legacy.log"
legacy_health_log="${session_dir}/legacy-health.log"
post_state_log="${session_dir}/post-state.log"
summary_log="${session_dir}/mode-summary.log"
manifest_file="${session_dir}/session.env"
result_file="${session_dir}/session-result.env"
health_script="${HEALTH_SCRIPT:-${script_dir}/check-vaio-target-health.sh}"
state_script="${STATE_SCRIPT:-${script_dir}/show-vaio-gma500-state.sh}"
multi_submit_script="${MULTI_SUBMIT_SCRIPT:-${script_dir}/run-vaio-sgx-multi-submit-default.sh}"
reset_script="${RESET_SCRIPT:-${script_dir}/run-vaio-sgx-lab-reset.sh}"
pre_reset_log="${session_dir}/pre-reset.log"
inter_reset_log="${session_dir}/inter-reset.log"

usage() {
    cat <<'EOF'
Usage:
  run-vaio-sgx-multi-submit-mode-compare.sh

Behavior:
  Creates one timestamped session directory, confirms the VAIO is usable,
  captures pre-state, runs the default typed multi-submit path, confirms the
  target is still healthy, runs the explicit legacy multi-submit path, confirms
  the target is still healthy again, then captures post-state and a concise
  mode summary.

Environment:
  SESSION_NAME         Session suffix. Default: sgx-multi-submit-mode-compare
  SESSION_KIND         Session kind stored in session metadata.
  LOGS_ROOT            Base directory for session logs.
  HEALTH_SCRIPT        Override health probe path.
  STATE_SCRIPT         Override remote-state capture path.
  MULTI_SUBMIT_SCRIPT  Override multi-submit runner path.
  RESET_SCRIPT         Override typed SGX lab reset helper path.
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

for required in "$health_script" "$state_script" "$multi_submit_script" "$reset_script"; do
    if [[ ! -x "$required" ]]; then
        echo "required executable missing: $required" >&2
        exit 1
    fi
done

mkdir -p "$session_dir"

{
    echo "timestamp=${timestamp}"
    echo "started_at=${started_at}"
    echo "session_kind=${session_kind}"
    echo "session_name=${session_name}"
    echo "session_dir=${session_dir}"
    echo "health_script=${health_script}"
    echo "state_script=${state_script}"
    echo "multi_submit_script=${multi_submit_script}"
    echo "reset_script=${reset_script}"
} >"$manifest_file"

echo "session_dir=${session_dir}"
echo "preflight_log=${preflight_log}"
echo "pre_state_log=${pre_state_log}"
echo "typed_log=${typed_log}"
echo "legacy_log=${legacy_log}"
echo "pre_reset_log=${pre_reset_log}"
echo "inter_reset_log=${inter_reset_log}"
echo "summary_log=${summary_log}"
echo "result_file=${result_file}"

extract_last_value() {
    local file="$1"
    local key="$2"
    awk -F= -v want="$key" '$1 == want { value = $2 } END { print value }' "$file"
}

extract_first_line() {
    local file="$1"
    local pattern="$2"
    awk -v want="$pattern" 'index($0, want) == 1 { print; exit }' "$file"
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

capture_mode_summary() {
    {
        echo "typed_remote_bin=$(extract_last_value "$typed_log" "REMOTE_BIN")"
        echo "typed_submit_setup=$(extract_last_value "$typed_log" "SUBMIT_SETUP")"
        echo "typed_object_setup=$(extract_last_value "$typed_log" "OBJECT_SETUP")"
        echo "typed_range_discovery=$(extract_last_value "$typed_log" "RANGE_DISCOVERY")"
        echo "typed_dependency_setup=$(extract_last_value "$typed_log" "DEPENDENCY_SETUP")"
        echo "typed_verification_mode=$(extract_last_value "$typed_log" "VERIFICATION_MODE")"
        echo "typed_dependency_interface=$(extract_last_value "$typed_log" "DEPENDENCY_INTERFACE")"
        echo "typed_submit_a_id=$(extract_last_value "$typed_log" "SUBMIT_A_ID")"
        echo "typed_submit_b_id=$(extract_last_value "$typed_log" "SUBMIT_B_ID")"
        echo "typed_submit_c_id=$(extract_last_value "$typed_log" "SUBMIT_C_ID")"
        echo "typed_submit_d_id=$(extract_last_value "$typed_log" "SUBMIT_D_ID")"
        echo "typed_submit_e_id=$(extract_last_value "$typed_log" "SUBMIT_E_ID")"
        echo "typed_range_ids=$(extract_last_value "$typed_log" "RANGE_IDS")"
        echo "typed_submit_validate_count=$(extract_last_value "$typed_log" "submit_validate_count")"
        echo "typed_submit_issue_count=$(extract_last_value "$typed_log" "submit_issue_count")"
        echo "typed_submit_transition_count=$(extract_last_value "$typed_log" "submit_transition_count")"
        echo "typed_submit_owner_state=$(extract_last_value "$typed_log" "submit_owner_state_name")"
        echo "typed_live_submit_count=$(extract_last_value "$typed_log" "coord_live_submit_count")"
        echo "typed_live_object_count=$(extract_last_value "$typed_log" "coord_live_object_count")"
        echo "typed_range_sgx_object_count=$(extract_last_value "$typed_log" "gtt_state_range_sgx_object_count")"
        echo "typed_range_pf_bound_count=$(extract_last_value "$typed_log" "gtt_state_range_pf_bound_count")"
        echo "typed_health_overall=$(extract_last_value "$typed_health_log" "overall")"
        echo "legacy_remote_bin=$(extract_last_value "$legacy_log" "REMOTE_BIN")"
        echo "legacy_dependency_interface=$(extract_last_value "$legacy_log" "DEPENDENCY_INTERFACE")"
        echo "legacy_submit_a_id=$(extract_last_value "$legacy_log" "SUBMIT_A_ID")"
        echo "legacy_submit_b_id=$(extract_last_value "$legacy_log" "SUBMIT_B_ID")"
        echo "legacy_submit_c_id=$(extract_last_value "$legacy_log" "SUBMIT_C_ID")"
        echo "legacy_submit_d_id=$(extract_last_value "$legacy_log" "SUBMIT_D_ID")"
        echo "legacy_submit_e_id=$(extract_last_value "$legacy_log" "SUBMIT_E_ID")"
        echo "legacy_range_ids=$(extract_last_value "$legacy_log" "RANGE_IDS")"
        echo "legacy_has_submits_after_drop=$(extract_first_line "$legacy_log" "SUBMITS_AFTER_DROP")"
        echo "legacy_has_objects_after_drop=$(extract_first_line "$legacy_log" "OBJECTS_AFTER_DROP")"
        echo "legacy_has_coord_after_drop=$(extract_first_line "$legacy_log" "COORD_STATE_AFTER_DROP")"
        echo "legacy_health_overall=$(extract_last_value "$legacy_health_log" "overall")"
    } >"$summary_log"
}

run_and_capture preflight_rc "$preflight_log" "$health_script"
preflight_overall="$(extract_last_value "$preflight_log" "overall")"

if [[ "$preflight_overall" != "remote_usable" ]]; then
    {
        echo "session_kind=${session_kind}"
        echo "session_outcome=blocked_preflight"
        echo "preflight_overall=${preflight_overall}"
        echo "finished_at=$(date -Iseconds)"
    } >"$result_file"
    exit 2
fi

run_and_capture pre_state_rc "$pre_state_log" "$state_script"
run_and_capture pre_reset_rc "$pre_reset_log" "$reset_script"
run_and_capture typed_rc "$typed_log" "$multi_submit_script"

if [[ "$typed_rc" -eq 0 ]]; then
    run_and_capture typed_health_rc "$typed_health_log" "$health_script"
else
    : >"$typed_health_log"
    typed_health_rc="skipped"
fi
typed_health_overall="$(extract_last_value "$typed_health_log" "overall")"

run_and_capture inter_reset_rc "$inter_reset_log" "$reset_script"
run_and_capture legacy_rc "$legacy_log" env SGX_MULTI_SUBMIT_DRIVER_MODE=legacy "$multi_submit_script"

if [[ "$legacy_rc" -eq 0 ]]; then
    run_and_capture legacy_health_rc "$legacy_health_log" "$health_script"
else
    : >"$legacy_health_log"
    legacy_health_rc="skipped"
fi
legacy_health_overall="$(extract_last_value "$legacy_health_log" "overall")"

run_and_capture post_state_rc "$post_state_log" "$state_script"

final_driver_srcversion="$(extract_last_value "$post_state_log" "driver_srcversion")"
final_override_module_size="$(extract_last_value "$post_state_log" "override_module_size")"

capture_mode_summary

last_overall="${legacy_health_overall:-${typed_health_overall:-${preflight_overall}}}"
session_outcome="completed"
if [[ "$pre_state_rc" -ne 0 ]]; then
    session_outcome="pre_state_capture_failed"
fi
if [[ "${pre_reset_rc}" -ne 0 ]]; then
    session_outcome="pre_reset_failed"
fi
if [[ "$typed_rc" -ne 0 ]]; then
    session_outcome="typed_run_failed"
fi
if [[ "$typed_rc" -eq 0 && "$typed_health_overall" != "remote_usable" ]]; then
    session_outcome="typed_run_unhealthy"
fi
if [[ "${inter_reset_rc}" -ne 0 ]]; then
    session_outcome="inter_reset_failed"
fi
if [[ "$legacy_rc" -ne 0 ]]; then
    session_outcome="legacy_run_failed"
fi
if [[ "$legacy_rc" -eq 0 && "$legacy_health_overall" != "remote_usable" ]]; then
    session_outcome="legacy_run_unhealthy"
fi
if [[ "$post_state_rc" -ne 0 ]]; then
    session_outcome="post_state_capture_failed"
fi

{
    echo "session_kind=${session_kind}"
    echo "session_outcome=${session_outcome}"
    echo "preflight_overall=${preflight_overall}"
    echo "pre_state_rc=${pre_state_rc}"
    echo "pre_reset_rc=${pre_reset_rc}"
    echo "typed_rc=${typed_rc}"
    echo "typed_health_overall=${typed_health_overall:-unknown}"
    echo "inter_reset_rc=${inter_reset_rc}"
    echo "legacy_rc=${legacy_rc}"
    echo "legacy_health_overall=${legacy_health_overall:-unknown}"
    echo "last_overall=${last_overall:-unknown}"
    echo "post_state_rc=${post_state_rc}"
    echo "final_driver_srcversion=${final_driver_srcversion:-unknown}"
    echo "final_override_module_size=${final_override_module_size:-unknown}"
    echo "finished_at=$(date -Iseconds)"
} >"$result_file"

if [[ "$pre_state_rc" -ne 0 ]]; then
    exit "$pre_state_rc"
fi
if [[ "${pre_reset_rc}" -ne 0 ]]; then
    exit "${pre_reset_rc}"
fi
if [[ "$typed_rc" -ne 0 ]]; then
    exit "$typed_rc"
fi
if [[ "$typed_health_overall" != "remote_usable" ]]; then
    exit 3
fi
if [[ "${inter_reset_rc}" -ne 0 ]]; then
    exit "${inter_reset_rc}"
fi
if [[ "$legacy_rc" -ne 0 ]]; then
    exit "$legacy_rc"
fi
if [[ "$legacy_health_overall" != "remote_usable" ]]; then
    exit 4
fi
if [[ "$post_state_rc" -ne 0 ]]; then
    exit "$post_state_rc"
fi
