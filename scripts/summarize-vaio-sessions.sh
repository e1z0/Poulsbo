#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
logs_root="${LOGS_ROOT:-${repo_root}/logs}"
max_sessions="${MAX_SESSIONS:-20}"

usage() {
    cat <<'EOF'
Usage:
  summarize-vaio-sessions.sh

Behavior:
  Scans saved VAIO recovery, deploy, rollback, and compare session bundles and
  prints a compact summary table with kind, timestamp, outcome, last state,
  phase profile/selection, local module srcversion, and session path.

Environment:
  LOGS_ROOT      Base logs directory. Default: <repo>/logs
  MAX_SESSIONS   Maximum sessions to print. 0 means all. Default: 20
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

get_value() {
    local file="$1"
    local key="$2"
    if [[ ! -f "$file" ]]; then
        return 0
    fi
    awk -F= -v key="$key" '$1 == key { print substr($0, index($0, "=") + 1) }' "$file" | tail -n 1
}

map_kind() {
    local dir="$1"
    local kind="${2:-}"
    if [[ -n "$kind" ]]; then
        printf '%s\n' "$kind"
        return 0
    fi
    case "$dir" in
        */vaio-recovery/*) printf '%s\n' "recovery" ;;
        */vaio-deploy/*) printf '%s\n' "guarded_deploy" ;;
        */vaio-plan/*) printf '%s\n' "plan_deploy" ;;
        */vaio-plan-session/*) printf '%s\n' "plan_session" ;;
        */vaio-compare/*) printf '%s\n' "compare" ;;
        */vaio-rollback/*) printf '%s\n' "rollback" ;;
        *) printf '%s\n' "unknown" ;;
    esac
}

infer_last_state_from_dir() {
    local session_dir="$1"
    local file
    local value=""

    for file in \
        "${session_dir}/post-state.log" \
        "${session_dir}/state.log" \
        "${session_dir}/mid-state.log" \
        "${session_dir}/remote-state.log" \
        "${session_dir}/pre-state.log" \
        "${session_dir}/preflight-health.log" \
        "${session_dir}/health-timeline.log" \
        "${session_dir}/rollback-watch.log"
    do
        if [[ -f "$file" ]]; then
            value="$(awk -F= '$1 == "overall" { value = $2 } END { print value }' "$file")"
            if [[ -n "$value" ]]; then
                printf '%s\n' "$value"
                return 0
            fi
        fi
    done

    printf '%s\n' "unknown"
}

map_last_state() {
    local result_file="$1"
    local session_dir="$2"
    local last_overall
    local preflight_overall
    local typed_health_overall
    local legacy_health_overall

    last_overall="$(get_value "$result_file" last_overall)"
    preflight_overall="$(get_value "$result_file" preflight_overall)"
    typed_health_overall="$(get_value "$result_file" typed_health_overall)"
    legacy_health_overall="$(get_value "$result_file" legacy_health_overall)"

    if [[ -n "$last_overall" ]]; then
        printf '%s\n' "$last_overall"
    elif [[ -n "$typed_health_overall" || -n "$legacy_health_overall" ]]; then
        if [[ -n "$typed_health_overall" && "$typed_health_overall" == "$legacy_health_overall" ]]; then
            printf '%s\n' "$typed_health_overall"
        elif [[ -n "$typed_health_overall" && -z "$legacy_health_overall" ]]; then
            printf '%s\n' "$typed_health_overall"
        elif [[ -z "$typed_health_overall" && -n "$legacy_health_overall" ]]; then
            printf '%s\n' "$legacy_health_overall"
        else
            printf '%s\n' "${typed_health_overall:-unknown}/${legacy_health_overall:-unknown}"
        fi
    elif [[ -n "$preflight_overall" ]]; then
        printf '%s\n' "$preflight_overall"
    else
        infer_last_state_from_dir "$session_dir"
    fi
}

map_outcome() {
    local result_file="$1"
    local session_dir="$2"
    local outcome
    outcome="$(get_value "$result_file" session_outcome)"
    if [[ -n "$outcome" ]]; then
        printf '%s\n' "$outcome"
    elif [[ -d "$session_dir" ]]; then
        printf '%s\n' "interrupted_or_incomplete"
    else
        printf '%s\n' "no_result"
    fi
}

map_name() {
    local dir="$1"
    local name="${2:-}"
    if [[ -n "$name" ]]; then
        printf '%s\n' "$name"
        return 0
    fi
    printf '%s\n' "$(basename "$dir")"
}

map_timestamp() {
    local dir="$1"
    local timestamp="${2:-}"
    if [[ -n "$timestamp" ]]; then
        printf '%s\n' "$timestamp"
        return 0
    fi
    printf '%s\n' "$(basename "$dir" | cut -d- -f1-2)"
}

map_module_srcversion() {
    local manifest_file="$1"
    local result_file="$2"
    local session_dir="$3"
    local value
    value="$(get_value "$manifest_file" local_module_srcversion)"
    if [[ -z "$value" ]]; then
        value="$(get_value "$manifest_file" source_module_srcversion)"
    fi
    if [[ -z "$value" ]]; then
        value="$(get_value "$result_file" final_driver_srcversion)"
    fi
    if [[ -z "$value" && -f "${session_dir}/post-state.log" ]]; then
        value="$(get_value "${session_dir}/post-state.log" driver_srcversion)"
    fi
    if [[ -z "$value" && -f "${session_dir}/pre-state.log" ]]; then
        value="$(get_value "${session_dir}/pre-state.log" driver_srcversion)"
    fi
    if [[ -n "$value" ]]; then
        printf '%s\n' "$value"
    else
        printf '%s\n' "unknown"
    fi
}

map_profile() {
    local manifest_file="$1"
    local result_file="$2"
    local profile=""
    local selection=""

    profile="$(get_value "$result_file" selected_phase_profile)"
    selection="$(get_value "$result_file" selected_phase_selection)"
    if [[ -z "$profile" ]]; then
        profile="$(get_value "$result_file" recovery_requested_phase_profile)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$result_file" recovery_requested_phase_selection)"
    fi
    if [[ -z "$profile" ]]; then
        profile="$(get_value "$result_file" recovery_phase_profile)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$result_file" recovery_phase_selection)"
    fi
    if [[ -z "$profile" ]]; then
        profile="$(get_value "$result_file" validation_phase_profile)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$result_file" validation_phase_selection)"
    fi
    if [[ -z "$profile" ]]; then
        profile="$(get_value "$result_file" phase_profile)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$result_file" phase_selection)"
    fi

    if [[ -z "$profile" ]]; then
        profile="$(get_value "$manifest_file" phase_profile)"
    fi
    if [[ -z "$profile" ]]; then
        profile="$(get_value "$manifest_file" validation_phase_profile_request)"
    fi
    if [[ -z "$profile" ]]; then
        profile="$(get_value "$manifest_file" phase_profile)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$manifest_file" phase_selection)"
    fi

    if [[ "$profile" == "auto" ]]; then
        profile=""
    fi

    if [[ -n "$profile" && -n "$selection" ]]; then
        printf '%s\n' "${profile}:${selection}"
    elif [[ -n "$profile" ]]; then
        printf '%s\n' "$profile"
    elif [[ -n "$selection" ]]; then
        printf '%s\n' "$selection"
    else
        printf '%s\n' "-"
    fi
}

map_finished_at() {
    local result_file="$1"
    local session_dir="$2"
    local value
    value="$(get_value "$result_file" finished_at)"
    if [[ -n "$value" ]]; then
        printf '%s\n' "$value"
    elif [[ -d "$session_dir" ]]; then
        date -Iseconds -r "$session_dir"
    else
        printf '%s\n' "-"
    fi
}

map_dir_display() {
    local dir="$1"
    printf '%s\n' "${dir#${repo_root}/}"
}

map_count_limit() {
    local count="$1"
    if [[ "$max_sessions" -gt 0 && "$count" -gt "$max_sessions" ]]; then
        return 1
    fi
    return 0
}

map_sort_candidates() {
    find "$logs_root" -mindepth 3 -maxdepth 3 -type f -name 'session.env' \
        | while IFS= read -r manifest_file; do
            session_dir="$(dirname "$manifest_file")"
            timestamp="$(basename "$session_dir" | awk -F- '{ print $1 "-" $2 }')"
            printf '%s\t%s\n' "$timestamp" "$manifest_file"
        done \
        | sort -r \
        | cut -f2-
}

printf '%-14s %-15s %-24s %-28s %-14s %-17s %-16s %-24s %s\n' \
    "kind" "timestamp" "name" "outcome" "last_state" "phase_profile" "module_srcversion" "finished_at" "session_dir"

count=0
while IFS= read -r manifest_file; do
    session_dir="$(dirname "$manifest_file")"
    result_file="${session_dir}/session-result.env"

    kind="$(map_kind "$session_dir" "$(get_value "$manifest_file" session_kind)")"
    timestamp_value="$(map_timestamp "$session_dir" "$(get_value "$manifest_file" timestamp)")"
    name="$(map_name "$session_dir" "$(get_value "$manifest_file" session_name)")"
    outcome="$(map_outcome "$result_file" "$session_dir")"
    last_state="$(map_last_state "$result_file" "$session_dir")"
    phase_profile="$(map_profile "$manifest_file" "$result_file")"
    module_srcversion="$(map_module_srcversion "$manifest_file" "$result_file" "$session_dir")"
    finished_at="$(map_finished_at "$result_file" "$session_dir")"
    dir_display="$(map_dir_display "$session_dir")"

    count=$((count + 1))
    if ! map_count_limit "$count"; then
        break
    fi

    printf '%-14s %-15s %-24s %-28s %-14s %-17s %-16s %-24s %s\n' \
        "$kind" "$timestamp_value" "$name" "$outcome" "$last_state" "$phase_profile" "$module_srcversion" "$finished_at" "$dir_display"
done < <(map_sort_candidates)
