#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
logs_root="${LOGS_ROOT:-${repo_root}/logs}"
max_sessions="${MAX_SESSIONS:-20}"
output_format="${OUTPUT_FORMAT:-table}"
summary_lib="${SUMMARY_LIB:-${script_dir}/lib-vaio-plan-summary.sh}"

usage() {
    cat <<'EOF'
Usage:
  summarize-vaio-plan-sessions.sh

Behavior:
  Scans saved SGX `plan` investigation bundles and prints a compact table for
  offline triage. It flattens the relevant plan-session, plan-deploy,
  plan-recovery, and direct submit-op phase-investigation results into one
  comparable view.

Environment:
  LOGS_ROOT      Base logs directory. Default: <repo>/logs
  MAX_SESSIONS   Maximum sessions to print. 0 means all. Default: 20
  OUTPUT_FORMAT  One of: table, tsv. Default: table
  SUMMARY_LIB    Override shared plan-summary helper path.
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

case "$output_format" in
    table|tsv)
        ;;
    *)
        echo "OUTPUT_FORMAT must be one of: table tsv" >&2
        exit 1
        ;;
esac

if [[ ! -f "$summary_lib" ]]; then
    echo "required helper missing: $summary_lib" >&2
    exit 1
fi

# shellcheck source=/root/Poulsbo/scripts/lib-vaio-plan-summary.sh
source "$summary_lib"

get_value() {
    local file="$1"
    local key="$2"
    if [[ ! -f "$file" ]]; then
        return 0
    fi
    awk -F= -v key="$key" '$1 == key { print substr($0, index($0, "=") + 1) }' "$file" | tail -n 1
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
        "${session_dir}/health-timeline.log"
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

map_kind() {
    local session_kind="$1"
    case "$session_kind" in
        sgx_submit_plan_session) printf '%s\n' "plan_session" ;;
        sgx_submit_plan_guarded_deploy) printf '%s\n' "plan_deploy" ;;
        sgx_submit_plan_recovery) printf '%s\n' "plan_recovery" ;;
        sgx_submit_op_phase_investigation) printf '%s\n' "phase_investigation" ;;
        *) printf '%s\n' "$session_kind" ;;
    esac
}

map_timestamp() {
    local session_dir="$1"
    local timestamp="${2:-}"
    if [[ -n "$timestamp" ]]; then
        printf '%s\n' "$timestamp"
    else
        printf '%s\n' "$(basename "$session_dir" | cut -d- -f1-2)"
    fi
}

map_name() {
    local session_dir="$1"
    local name="${2:-}"
    if [[ -n "$name" ]]; then
        printf '%s\n' "$name"
    else
        printf '%s\n' "$(basename "$session_dir")"
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

map_last_state() {
    local result_file="$1"
    local session_dir="$2"
    local value
    value="$(get_value "$result_file" last_overall)"
    if [[ -n "$value" ]]; then
        printf '%s\n' "$value"
        return 0
    fi
    value="$(get_value "$result_file" preflight_overall)"
    if [[ -n "$value" ]]; then
        printf '%s\n' "$value"
        return 0
    fi
    infer_last_state_from_dir "$session_dir"
}

map_module_srcversion() {
    local manifest_file="$1"
    local result_file="$2"
    local session_dir="$3"
    local value
    value="$(get_value "$result_file" final_driver_srcversion)"
    if [[ -z "$value" ]]; then
        value="$(get_value "${session_dir}/post-state.log" driver_srcversion)"
    fi
    if [[ -z "$value" ]]; then
        value="$(get_value "${session_dir}/pre-state.log" driver_srcversion)"
    fi
    if [[ -z "$value" ]]; then
        value="$(get_value "$manifest_file" source_module_srcversion)"
    fi
    if [[ -z "$value" ]]; then
        value="$(get_value "$manifest_file" local_module_srcversion)"
    fi
    if [[ -n "$value" ]]; then
        printf '%s\n' "$value"
    else
        printf '%s\n' "unknown"
    fi
}

format_phase_view() {
    local profile="$1"
    local selection="$2"
    local phases="$3"
    local preserve_auto="${4:-0}"
    local view=""

    if [[ "$profile" == "auto" && "$preserve_auto" != "1" ]]; then
        profile=""
    fi

    if [[ -n "$profile" && -n "$selection" ]]; then
        view="${profile}:${selection}"
    elif [[ -n "$profile" ]]; then
        view="$profile"
    elif [[ -n "$selection" ]]; then
        view="$selection"
    fi

    if [[ -n "$phases" ]]; then
        if [[ -n "$view" ]]; then
            view="${view}[${phases}]"
        else
            view="[${phases}]"
        fi
    fi

    if [[ -n "$view" ]]; then
        printf '%s\n' "$view"
    else
        printf '%s\n' "-"
    fi
}

map_requested_phase_view() {
    local result_file="$1"
    local manifest_file="$2"
    local profile=""
    local selection=""
    local phases=""

    profile="$(get_value "$result_file" selected_phase_profile_request)"
    selection="$(get_value "$result_file" selected_phase_selection_request)"
    phases="$(get_value "$result_file" selected_phases_request)"

    if [[ -z "$profile" ]]; then
        profile="$(get_value "$result_file" recovery_requested_phase_profile)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$result_file" recovery_requested_phase_selection)"
    fi
    if [[ -z "$phases" ]]; then
        phases="$(get_value "$result_file" recovery_requested_phases)"
    fi

    if [[ -z "$profile" ]]; then
        profile="$(get_value "$result_file" validation_phase_profile_request_original)"
    fi
    if [[ -z "$profile" ]]; then
        profile="$(get_value "$result_file" validation_phase_profile_request)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$result_file" validation_phase_selection_request)"
    fi
    if [[ -z "$phases" ]]; then
        phases="$(get_value "$result_file" validation_phases_request)"
    fi

    if [[ -z "$profile" ]]; then
        profile="$(get_value "$result_file" selected_phase_profile)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$result_file" selected_phase_selection)"
    fi
    if [[ -z "$phases" ]]; then
        phases="$(get_value "$result_file" selected_phases)"
    fi

    if [[ -z "$profile" ]]; then
        profile="$(get_value "$result_file" phase_profile)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$result_file" phase_selection)"
    fi
    if [[ -z "$phases" ]]; then
        phases="$(get_value "$result_file" phases)"
    fi

    if [[ -z "$profile" ]]; then
        profile="$(get_value "$manifest_file" plan_phase_profile)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$manifest_file" phase_selection)"
    fi
    if [[ -z "$phases" ]]; then
        phases="$(get_value "$manifest_file" plan_phases_override)"
    fi
    if [[ -z "$profile" ]]; then
        profile="$(get_value "$manifest_file" validation_phase_profile_request)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$manifest_file" validation_phase_selection_request)"
    fi
    if [[ -z "$phases" ]]; then
        phases="$(get_value "$manifest_file" validation_phases_request)"
    fi

    format_phase_view "$profile" "$selection" "$phases" 1
}

map_resolved_phase_view() {
    local result_file="$1"
    local profile=""
    local selection=""
    local phases=""

    profile="$(get_value "$result_file" selected_phase_profile)"
    selection="$(get_value "$result_file" selected_phase_selection)"
    phases="$(get_value "$result_file" selected_phases)"

    if [[ -z "$profile" ]]; then
        profile="$(get_value "$result_file" recovery_phase_profile)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$result_file" recovery_phase_selection)"
    fi
    if [[ -z "$phases" ]]; then
        phases="$(get_value "$result_file" recovery_phases)"
    fi

    if [[ -z "$profile" ]]; then
        profile="$(get_value "$result_file" validation_phase_profile)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$result_file" validation_phase_selection)"
    fi
    if [[ -z "$phases" ]]; then
        phases="$(get_value "$result_file" validation_phases)"
    fi

    if [[ -z "$profile" ]]; then
        profile="$(get_value "$result_file" phase_profile)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$result_file" phase_selection)"
    fi
    if [[ -z "$phases" ]]; then
        phases="$(get_value "$result_file" phases)"
    fi

    format_phase_view "$profile" "$selection" "$phases"
}

map_child_phase_view() {
    local result_file="$1"
    local profile=""
    local selection=""
    local phases=""

    profile="$(get_value "$result_file" selected_child_phase_profile)"
    selection="$(get_value "$result_file" selected_child_phase_selection)"
    phases="$(get_value "$result_file" selected_child_phases)"

    if [[ -z "$profile" ]]; then
        profile="$(get_value "$result_file" recovery_phase_profile)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$result_file" recovery_phase_selection)"
    fi
    if [[ -z "$phases" ]]; then
        phases="$(get_value "$result_file" recovery_phases)"
    fi

    if [[ -z "$profile" ]]; then
        profile="$(get_value "$result_file" validation_phase_profile)"
    fi
    if [[ -z "$selection" ]]; then
        selection="$(get_value "$result_file" validation_phase_selection)"
    fi
    if [[ -z "$phases" ]]; then
        phases="$(get_value "$result_file" validation_phases)"
    fi

    format_phase_view "$profile" "$selection" "$phases"
}

map_failed_phase() {
    local result_file="$1"
    local value=""

    for key in \
        selected_failed_phase \
        recovery_failed_phase \
        validation_failed_phase \
        confirm_last_phase \
        last_phase \
        first_bad_phase \
        bisect_first_bad_phase
    do
        value="$(get_value "$result_file" "$key")"
        if [[ -n "$value" && "$value" != "skipped" ]]; then
            printf '%s\n' "$value"
            return 0
        fi
    done

    printf '%s\n' "-"
}

map_failed_step() {
    local result_file="$1"
    local value=""

    for key in \
        selected_failed_step \
        recovery_failed_step \
        validation_failed_step \
        confirm_last_phase_failed_step \
        last_phase_failed_step \
        first_bad_failed_step \
        bisect_first_bad_failed_step
    do
        value="$(get_value "$result_file" "$key")"
        if [[ -n "$value" && "$value" != "skipped" ]]; then
            printf '%s\n' "$value"
            return 0
        fi
    done

    printf '%s\n' "-"
}

map_phase_source_field() {
    local result_file="$1"
    local field_name="$2"
    local value=""

    value="$(get_value "$result_file" "selected_phase_source_${field_name}")"
    if [[ -z "$value" ]]; then
        value="$(get_value "$result_file" "validation_phase_source_${field_name}")"
    fi

    printf '%s\n' "$value"
}

map_phase_source_view() {
    local result_file="$1"
    local reason=""
    local timestamp=""
    local session_dir=""

    reason="$(map_phase_source_field "$result_file" reason)"
    timestamp="$(map_phase_source_field "$result_file" timestamp)"
    session_dir="$(map_phase_source_field "$result_file" session_dir)"

    build_phase_source_view "$reason" "$timestamp" "$session_dir"
}

map_bisect_evidence_from_files() {
    local summary_file="$1"
    local result_file="$2"
    local which="$3"
    local evidence_blob=""
    local value=""

    evidence_blob="$(vaio_plan_extract_effective_bisect_evidence "$summary_file" "$result_file")"
    value="$(awk -F= -v key="$which" '$1 == key { print substr($0, index($0, "=") + 1); exit }' <<<"$evidence_blob")"

    if [[ -n "$value" ]]; then
        printf '%s\n' "$value"
    else
        printf '%s\n' "-"
    fi
}

map_dir_display() {
    local dir="$1"
    printf '%s\n' "${dir#${repo_root}/}"
}

infer_phase_source_kind_from_dir() {
    local session_dir="$1"

    case "$session_dir" in
        *"/logs/vaio-plan-session/"*|logs/vaio-plan-session/*)
            printf '%s\n' "plan_session"
            ;;
        *"/logs/vaio-plan/"*|logs/vaio-plan/*)
            printf '%s\n' "plan_deploy"
            ;;
        *"/logs/vaio-recovery/"*|logs/vaio-recovery/*)
            printf '%s\n' "plan_recovery"
            ;;
        *"/logs/vaio-ioctl/"*|logs/vaio-ioctl/*)
            printf '%s\n' "phase_investigation"
            ;;
        *)
            printf '%s\n' ""
            ;;
    esac
}

infer_phase_source_name_from_dir() {
    local session_dir="$1"
    local base=""
    local stripped=""

    [[ -n "$session_dir" ]] || return 0
    base="$(basename "$session_dir")"
    stripped="$(cut -d- -f3- <<<"$base")"
    if [[ -n "$stripped" && "$stripped" != "$base" ]]; then
        printf '%s\n' "$stripped"
    else
        printf '%s\n' "$base"
    fi
}

build_phase_source_view() {
    local reason="$1"
    local timestamp="$2"
    local session_dir="$3"
    local value=""

    if [[ -n "$reason" && "$reason" != "-" ]]; then
        value="$reason"
    fi
    if [[ -n "$timestamp" && "$timestamp" != "-" ]]; then
        if [[ -n "$value" ]]; then
            value="${value}@${timestamp}"
        else
            value="$timestamp"
        fi
    elif [[ -n "$session_dir" && "$session_dir" != "-" ]]; then
        if [[ -n "$value" ]]; then
            value="${value}@$(basename "$session_dir")"
        else
            value="$(basename "$session_dir")"
        fi
    fi

    if [[ -n "$value" ]]; then
        printf '%s\n' "$value"
    else
        printf '%s\n' "-"
    fi
}

collect_manifests() {
    local root
    for root in \
        "${logs_root}/vaio-plan-session" \
        "${logs_root}/vaio-plan" \
        "${logs_root}/vaio-recovery" \
        "${logs_root}/vaio-ioctl"
    do
        if [[ -d "$root" ]]; then
            find "$root" -mindepth 2 -maxdepth 2 -type f -name 'session.env'
        fi
    done
}

tmp_file="$(mktemp)"

while IFS= read -r manifest_file; do
    session_dir="$(dirname "$manifest_file")"
    result_file="${session_dir}/session-result.env"
    session_kind="$(get_value "$manifest_file" session_kind)"
    if [[ -z "$session_kind" ]]; then
        session_kind="$(get_value "$result_file" session_kind)"
    fi

    case "$session_kind" in
        sgx_submit_plan_session|sgx_submit_plan_guarded_deploy|sgx_submit_plan_recovery|sgx_submit_op_phase_investigation)
            ;;
        *)
            continue
            ;;
    esac

    timestamp="$(map_timestamp "$session_dir" "$(get_value "$manifest_file" timestamp)")"
    name="$(map_name "$session_dir" "$(get_value "$manifest_file" session_name)")"
    kind="$(map_kind "$session_kind")"
    outcome="$(map_outcome "$result_file" "$session_dir")"
    last_state="$(map_last_state "$result_file" "$session_dir")"
    requested="$(map_requested_phase_view "$result_file" "$manifest_file")"
    resolved="$(map_resolved_phase_view "$result_file")"
    child="$(map_child_phase_view "$result_file")"
    phase_source="$(map_phase_source_view "$result_file")"
    phase_source_reason="$(map_phase_source_field "$result_file" reason)"
    phase_source_kind="$(map_phase_source_field "$result_file" kind)"
    phase_source_name="$(map_phase_source_field "$result_file" name)"
    phase_source_timestamp="$(map_phase_source_field "$result_file" timestamp)"
    phase_source_session_dir="$(map_phase_source_field "$result_file" session_dir)"
    failed_phase="$(map_failed_phase "$result_file")"
    failed_step="$(map_failed_step "$result_file")"
    module_srcversion="$(map_module_srcversion "$manifest_file" "$result_file" "$session_dir")"
    bisect_summary_file="$(vaio_plan_latest_bisect_summary "$session_dir" "$repo_root")"
    bisect_result_file="$(vaio_plan_latest_bisect_result "$bisect_summary_file")"
    evidence_last_started="$(map_bisect_evidence_from_files "$bisect_summary_file" "$bisect_result_file" last_started)"
    evidence_last_good="$(map_bisect_evidence_from_files "$bisect_summary_file" "$bisect_result_file" last_good)"
    evidence_first_bad="$(map_bisect_evidence_from_files "$bisect_summary_file" "$bisect_result_file" first_bad)"
    evidence_reason="$(map_bisect_evidence_from_files "$bisect_summary_file" "$bisect_result_file" reason)"
    finished_at="$(get_value "$result_file" finished_at)"
    if [[ -z "$finished_at" ]]; then
        finished_at="$(date -Iseconds -r "$session_dir")"
    fi
    dir_display="$(map_dir_display "$session_dir")"

    if [[ -z "$phase_source_reason" || "$phase_source_reason" == "-" ]]; then
        if [[ -n "$evidence_reason" && "$evidence_reason" != "-" ]]; then
            phase_source_reason="$evidence_reason"
        fi
    fi
    if [[ -z "$phase_source_timestamp" || "$phase_source_timestamp" == "-" ]]; then
        if [[ -n "$phase_source_reason" && "$phase_source_reason" != "-" ]]; then
            phase_source_timestamp="$timestamp"
        fi
    fi
    if [[ -z "$phase_source_session_dir" || "$phase_source_session_dir" == "-" ]]; then
        if [[ -n "$phase_source_reason" && "$phase_source_reason" != "-" ]]; then
            phase_source_session_dir="$dir_display"
        fi
    fi
    if [[ -z "$phase_source_kind" || "$phase_source_kind" == "-" ]]; then
        phase_source_kind="$(infer_phase_source_kind_from_dir "$phase_source_session_dir")"
    fi
    if [[ -z "$phase_source_name" || "$phase_source_name" == "-" ]]; then
        phase_source_name="$(infer_phase_source_name_from_dir "$phase_source_session_dir")"
    fi
    phase_source="$(build_phase_source_view "$phase_source_reason" "$phase_source_timestamp" "$phase_source_session_dir")"

    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$timestamp" \
        "$kind" \
        "$timestamp" \
        "$name" \
        "$outcome" \
        "$last_state" \
        "$requested" \
        "$resolved" \
        "$child" \
        "$phase_source" \
        "${phase_source_reason:--}" \
        "${phase_source_kind:--}" \
        "${phase_source_name:--}" \
        "${phase_source_timestamp:--}" \
        "${phase_source_session_dir:--}" \
        "$failed_phase" \
        "$failed_step" \
        "$module_srcversion" \
        "$dir_display" \
        "$evidence_last_started" \
        "$evidence_last_good" \
        "$evidence_first_bad" \
        "$evidence_reason" \
        >>"$tmp_file"
done < <(collect_manifests)

output_file="$(mktemp)"
trap 'rm -f "$tmp_file" "$output_file"' EXIT

{
    printf 'kind\ttimestamp\tname\toutcome\tlast_state\trequested\tresolved\tchild\tphase_source\tphase_source_reason\tphase_source_kind\tphase_source_name\tphase_source_timestamp\tphase_source_session_dir\tfailed_phase\tfailed_step\tmodule_srcversion\tsession_dir\tevidence_last_started\tevidence_last_good\tevidence_first_bad\tevidence_reason\n'
    if [[ -s "$tmp_file" ]]; then
        if [[ "$max_sessions" -gt 0 ]]; then
            sort -r "$tmp_file" | head -n "$max_sessions" | cut -f2-
        else
            sort -r "$tmp_file" | cut -f2-
        fi
    fi
} >"$output_file"

if [[ "$output_format" == "tsv" ]]; then
    cat "$output_file"
else
    column -t -s $'\t' "$output_file"
fi
