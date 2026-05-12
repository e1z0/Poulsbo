#!/usr/bin/env bash
# shellcheck shell=bash

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "lib-vaio-plan-phase-source.sh is meant to be sourced, not executed directly" >&2
    exit 1
fi

if [[ -n "${_VAIO_PLAN_PHASE_SOURCE_LIB_SH:-}" ]]; then
    return 0
fi
_VAIO_PLAN_PHASE_SOURCE_LIB_SH=1

declare -ga VAIO_PLAN_PHASE_SOURCE_FIELDS=(
    reason
    kind
    name
    timestamp
    session_dir
)

declare -ga VAIO_PLAN_PHASE_SOURCE_ROW_FIELDS=(
    requested
    resolved
    child
    outcome
    last_state
    phase_source
    failed_phase
    failed_step
    module_srcversion
    evidence_last_started
    evidence_last_good
    evidence_first_bad
    evidence_reason
)

vaio_kv_last_value() {
    local file="$1"
    local key="$2"

    if [[ ! -f "$file" ]]; then
        return 0
    fi

    awk -F= -v want="$key" '$1 == want { value = substr($0, index($0, "=") + 1) } END { print value }' "$file"
}

vaio_plan_phase_source_init_vars() {
    local var_prefix="$1"
    local field=""
    local var_name=""

    for field in "${VAIO_PLAN_PHASE_SOURCE_FIELDS[@]}"; do
        var_name="${var_prefix}_${field}"
        printf -v "$var_name" '%s' ""
    done

    for field in "${VAIO_PLAN_PHASE_SOURCE_ROW_FIELDS[@]}"; do
        var_name="${var_prefix}_row_${field}"
        printf -v "$var_name" '%s' ""
    done
}

vaio_plan_phase_source_assign_from_resolver_file() {
    local file="$1"
    local var_prefix="$2"
    local field=""
    local key=""
    local var_name=""

    printf -v "${var_prefix}_reason" '%s' "$(vaio_kv_last_value "$file" "resolved_reason")"
    printf -v "${var_prefix}_kind" '%s' "$(vaio_kv_last_value "$file" "source_kind")"
    printf -v "${var_prefix}_name" '%s' "$(vaio_kv_last_value "$file" "source_name")"
    printf -v "${var_prefix}_timestamp" '%s' "$(vaio_kv_last_value "$file" "source_timestamp")"
    printf -v "${var_prefix}_session_dir" '%s' "$(vaio_kv_last_value "$file" "source_session_dir")"

    for field in "${VAIO_PLAN_PHASE_SOURCE_ROW_FIELDS[@]}"; do
        key="source_row_${field}"
        var_name="${var_prefix}_row_${field}"
        printf -v "$var_name" '%s' "$(vaio_kv_last_value "$file" "$key")"
    done
}

vaio_plan_phase_source_assign_from_environment() {
    local env_prefix="$1"
    local var_prefix="$2"
    local field=""
    local env_var=""
    local var_name=""

    for field in "${VAIO_PLAN_PHASE_SOURCE_FIELDS[@]}"; do
        env_var="${env_prefix}_${field^^}"
        var_name="${var_prefix}_${field}"
        printf -v "$var_name" '%s' "${!env_var:-}"
    done

    for field in "${VAIO_PLAN_PHASE_SOURCE_ROW_FIELDS[@]}"; do
        env_var="${env_prefix}_ROW_${field^^}"
        var_name="${var_prefix}_row_${field}"
        printf -v "$var_name" '%s' "${!env_var:-}"
    done
}

vaio_plan_phase_source_emit() {
    local key_prefix="$1"
    local var_prefix="$2"
    local field=""
    local var_name=""

    for field in "${VAIO_PLAN_PHASE_SOURCE_FIELDS[@]}"; do
        var_name="${var_prefix}_${field}"
        printf '%s=%s\n' "${key_prefix}_${field}" "${!var_name}"
    done

    for field in "${VAIO_PLAN_PHASE_SOURCE_ROW_FIELDS[@]}"; do
        var_name="${var_prefix}_row_${field}"
        printf '%s=%s\n' "${key_prefix}_row_${field}" "${!var_name}"
    done
}

vaio_plan_phase_source_env_args() {
    local env_prefix="$1"
    local var_prefix="$2"
    local field=""
    local var_name=""

    for field in "${VAIO_PLAN_PHASE_SOURCE_FIELDS[@]}"; do
        var_name="${var_prefix}_${field}"
        printf '%s=%s\n' "${env_prefix}_${field^^}" "${!var_name}"
    done

    for field in "${VAIO_PLAN_PHASE_SOURCE_ROW_FIELDS[@]}"; do
        var_name="${var_prefix}_row_${field}"
        printf '%s=%s\n' "${env_prefix}_ROW_${field^^}" "${!var_name}"
    done
}

vaio_plan_phase_source_row_present() {
    local var_prefix="$1"
    local requested_var="${var_prefix}_row_requested"
    local resolved_var="${var_prefix}_row_resolved"
    local child_var="${var_prefix}_row_child"

    [[ -n "${!requested_var}" || -n "${!resolved_var}" || -n "${!child_var}" ]]
}
