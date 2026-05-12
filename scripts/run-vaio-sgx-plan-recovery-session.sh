#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
recovery_script="${RECOVERY_SCRIPT:-${script_dir}/run-vaio-recovery-session.sh}"
validation_script="${VALIDATION_SCRIPT:-${script_dir}/run-vaio-sgx-submit-op-phase-investigation.sh}"
phase_profile_script="${PHASE_PROFILE_SCRIPT:-${script_dir}/resolve-sgx-submit-phase-profile.sh}"
current_phase_script="${CURRENT_PHASE_SCRIPT:-${script_dir}/resolve-vaio-plan-current-phase.sh}"
phase_source_lib="${PHASE_SOURCE_LIB:-${script_dir}/lib-vaio-plan-phase-source.sh}"
health_script="${HEALTH_SCRIPT:-${script_dir}/check-vaio-target-health.sh}"
state_script="${STATE_SCRIPT:-${script_dir}/show-vaio-gma500-state.sh}"
plan_phase_profile="${PLAN_PHASE_PROFILE:-${PHASE_PROFILE:-current}}"
plan_phases_override="${PLAN_PHASES:-${PHASES:-}}"
unsafe_plan_query_srcversion="${UNSAFE_PLAN_QUERY_SRCVERSION:-0EEEE302E3FDA9F59B7CFB9}"
allow_known_unsafe_plan_query_recovery="${ALLOW_KNOWN_UNSAFE_PLAN_QUERY_RECOVERY:-0}"

usage() {
    cat <<'EOF'
Usage:
  run-vaio-sgx-plan-recovery-session.sh

Behavior:
  Runs the generic VAIO recovery-session wrapper with the staged submit-op
  investigation validator, so the first post-recovery SGX submit rerun is
  captured as one structured session bundle.

Environment:
  RECOVERY_SCRIPT          Override generic recovery-session wrapper.
  VALIDATION_SCRIPT        Override investigation validator.
  SESSION_NAME             Session suffix. Default: sgx-plan-recovery
  SESSION_KIND             Structured session kind. Default:
                           sgx_submit_plan_recovery
  VALIDATION_LABEL         Validation label. Default:
                           submit_op_phase_investigation
  VALIDATION_LOG_BASENAME  Validation log filename. Default:
                           submit-op-phase-investigation.log
  PLAN_PHASE_PROFILE       Plan-specific investigation phase profile. Default:
                           current
  PLAN_PHASES              Plan-specific explicit phase list. Overrides
                           PLAN_PHASE_PROFILE.
  PHASE_PROFILE            Investigation phase profile. Default:
                           mirrors PLAN_PHASE_PROFILE when set.
  PHASES                   Explicit phase list forwarded to the investigation
                           validator. PLAN_PHASES is the preferred alias.
  HEALTH_SCRIPT            Override target-health probe used by the local
                           no-redeploy safety gate.
  STATE_SCRIPT             Override remote-state probe used by the local
                           no-redeploy safety gate.
  UNSAFE_PLAN_QUERY_SRCVERSION
                           Known-bad live module srcversion blocked for
                           recovery-side `plan_query*` reruns.
  ALLOW_KNOWN_UNSAFE_PLAN_QUERY_RECOVERY
                           Set to 1 to bypass that local safety gate.
  POLL_INTERVAL_SECS       Forwarded to the recovery-session wrapper.
  MAX_POLLS                Forwarded to the recovery-session wrapper.
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if [[ ! -x "$recovery_script" ]]; then
    echo "recovery wrapper is not executable: $recovery_script" >&2
    exit 1
fi

if [[ ! -x "$validation_script" ]]; then
    echo "validation script is not executable: $validation_script" >&2
    exit 1
fi

if [[ ! -x "$phase_profile_script" ]]; then
    echo "phase-profile resolver is not executable: $phase_profile_script" >&2
    exit 1
fi

if [[ ! -x "$current_phase_script" ]]; then
    echo "current-phase resolver is not executable: $current_phase_script" >&2
    exit 1
fi

if [[ ! -x "$health_script" ]]; then
    echo "health probe is not executable: $health_script" >&2
    exit 1
fi

if [[ ! -x "$state_script" ]]; then
    echo "state probe is not executable: $state_script" >&2
    exit 1
fi

if [[ ! -f "$phase_source_lib" ]]; then
    echo "phase-source helper is missing: $phase_source_lib" >&2
    exit 1
fi

# shellcheck source=/root/Poulsbo/scripts/lib-vaio-plan-phase-source.sh
source "$phase_source_lib"

extract_last_value() {
    local file="$1"
    local key="$2"
    awk -F= -v want="$key" '$1 == want { value = substr($0, index($0, "=") + 1) } END { print value }' "$file"
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

resolved_phases="$plan_phases_override"
plan_phase_selection="explicit"
resolved_plan_phase_profile="$plan_phase_profile"
vaio_plan_phase_source_init_vars "resolved_phase_source"
if [[ -z "$resolved_phases" ]]; then
    plan_phase_selection="profile"
    if [[ "$plan_phase_profile" == "current" ]]; then
        tmp_current="$(mktemp)"
        "$current_phase_script" >"$tmp_current"
        resolved_plan_phase_profile="$(vaio_kv_last_value "$tmp_current" "resolved_profile")"
        resolved_phases="$(vaio_kv_last_value "$tmp_current" "resolved_phases")"
        vaio_plan_phase_source_assign_from_resolver_file "$tmp_current" "resolved_phase_source"
        rm -f "$tmp_current"
    else
        resolved_phases="$("$phase_profile_script" "$plan_phase_profile")"
    fi
fi

if [[ "$allow_known_unsafe_plan_query_recovery" != "0" && "$allow_known_unsafe_plan_query_recovery" != "1" ]]; then
    echo "ALLOW_KNOWN_UNSAFE_PLAN_QUERY_RECOVERY must be 0 or 1" >&2
    exit 1
fi

if [[ "$allow_known_unsafe_plan_query_recovery" == "0" ]] && \
    phase_list_contains_plan_query "$resolved_phases"; then
    tmp_health="$(mktemp)"
    tmp_state="$(mktemp)"

    if "$health_script" >"$tmp_health" 2>&1; then
        current_overall="$(extract_last_value "$tmp_health" "overall")"
        if [[ "$current_overall" == "remote_usable" ]] && \
            "$state_script" >"$tmp_state" 2>&1; then
            current_driver_srcversion="$(extract_last_value "$tmp_state" "driver_srcversion")"
            current_driver_module_loaded="$(extract_last_value "$tmp_state" "driver_module_loaded")"

            if [[ "$current_driver_module_loaded" == "1" && \
                  "$current_driver_srcversion" == "$unsafe_plan_query_srcversion" ]]; then
                echo "Refusing recovery-side ${resolved_phases} on known-unsafe live module srcversion ${current_driver_srcversion}" >&2
                echo "Use a guarded deploy of the fixed tree instead:" >&2
                echo "  PLAN_SESSION_MODE=deploy PLAN_PHASES=${resolved_phases} ./scripts/run-vaio-sgx-plan-session.sh" >&2
                echo "  PLAN_SESSION_MODE=deploy PLAN_PHASE_PROFILE=${resolved_plan_phase_profile} ./scripts/run-vaio-sgx-plan-session.sh" >&2
                exit 3
            fi
        fi
    fi
    rm -f "$tmp_health" "$tmp_state"
fi

declare -a validation_phase_source_env=()
mapfile -t validation_phase_source_env < <(
    vaio_plan_phase_source_env_args "VALIDATION_PHASE_SOURCE" "resolved_phase_source"
)

exec env \
    SESSION_NAME="${SESSION_NAME:-sgx-plan-recovery}" \
    SESSION_KIND="${SESSION_KIND:-sgx_submit_plan_recovery}" \
    VALIDATION_LABEL="${VALIDATION_LABEL:-submit_op_phase_investigation}" \
    VALIDATION_LOG_BASENAME="${VALIDATION_LOG_BASENAME:-submit-op-phase-investigation.log}" \
    VALIDATION_PHASE_PROFILE_REQUEST_ORIGINAL="${plan_phase_profile}" \
    VALIDATION_PHASE_PROFILE_BASE="${resolved_plan_phase_profile}" \
    "${validation_phase_source_env[@]}" \
    PLAN_PHASE_PROFILE="${resolved_plan_phase_profile}" \
    PLAN_PHASES="${resolved_phases}" \
    PHASE_PROFILE="${resolved_plan_phase_profile}" \
    PHASE_SELECTION="${plan_phase_selection}" \
    PHASES="${resolved_phases}" \
    VALIDATION_SCRIPT="$validation_script" \
    "$recovery_script"
