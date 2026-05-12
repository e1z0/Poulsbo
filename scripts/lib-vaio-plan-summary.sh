#!/usr/bin/env bash
# shellcheck shell=bash

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "lib-vaio-plan-summary.sh is meant to be sourced, not executed directly" >&2
    exit 1
fi

if [[ -n "${_VAIO_PLAN_SUMMARY_LIB_SH:-}" ]]; then
    return 0
fi
_VAIO_PLAN_SUMMARY_LIB_SH=1

declare -gA VAIO_PLAN_SUMMARY_COLUMNS=()

vaio_plan_summary_init_columns() {
    local summary_file="$1"
    local header
    local index=0

    VAIO_PLAN_SUMMARY_COLUMNS=()
    IFS=$'\t' read -r -a header <"$summary_file"
    for name in "${header[@]}"; do
        index=$((index + 1))
        VAIO_PLAN_SUMMARY_COLUMNS["$name"]="$index"
    done
}

vaio_plan_summary_pick_field() {
    local row="$1"
    local field_name="$2"
    local idx="${VAIO_PLAN_SUMMARY_COLUMNS[$field_name]:-}"

    if [[ -z "$idx" ]]; then
        echo "missing summary column: $field_name" >&2
        exit 1
    fi
    awk -F'\t' -v idx="$idx" '{ print $idx }' <<<"$row"
}

vaio_plan_summary_pick_latest_session() {
    local summary_file="$1"
    local pattern="$2"
    local result=""

    while IFS= read -r line; do
        [[ -n "$line" ]] || continue
        [[ "$line" == kind$'\t'* ]] && continue
        if [[ "$line" =~ $pattern ]]; then
            result="$line"
            break
        fi
    done <"$summary_file"

    printf '%s\n' "$result"
}

vaio_plan_summary_find_row() {
    local summary_file="$1"
    shift
    local line=""
    local matcher=""
    local field_name=""
    local expected=""
    local actual=""
    local matched=""

    while IFS= read -r line; do
        [[ -n "$line" ]] || continue
        [[ "$line" == kind$'\t'* ]] && continue
        matched=1
        for matcher in "$@"; do
            field_name="${matcher%%=*}"
            expected="${matcher#*=}"
            actual="$(vaio_plan_summary_pick_field "$line" "$field_name")"
            if [[ "$actual" != "$expected" ]]; then
                matched=0
                break
            fi
        done
        if [[ "$matched" == 1 ]]; then
            printf '%s\n' "$line"
            return 0
        fi
    done <"$summary_file"

    printf '%s\n' ""
}

vaio_plan_summary_pick_latest_phase_evidence_row() {
    local summary_file="$1"
    local line=""
    local kind=""
    local outcome=""
    local last_state=""
    local evidence_last_started=""
    local evidence_last_good=""
    local evidence_first_bad=""
    local evidence_reason=""
    local phase_source_reason=""

    while IFS= read -r line; do
        [[ -n "$line" ]] || continue
        [[ "$line" == kind$'\t'* ]] && continue

        kind="$(vaio_plan_summary_pick_field "$line" "kind")"
        outcome="$(vaio_plan_summary_pick_field "$line" "outcome")"
        last_state="$(vaio_plan_summary_pick_field "$line" "last_state")"

        [[ "$kind" == "plan_session" ]] || continue
        [[ "$last_state" == "remote_usable" ]] || continue
        [[ "$outcome" =~ ^(completed|recovery_completed|validation_failed|interrupted|interrupted_or_incomplete)$ ]] || continue

        evidence_last_started="$(vaio_plan_summary_pick_field "$line" "evidence_last_started")"
        evidence_last_good="$(vaio_plan_summary_pick_field "$line" "evidence_last_good")"
        evidence_first_bad="$(vaio_plan_summary_pick_field "$line" "evidence_first_bad")"
        evidence_reason="$(vaio_plan_summary_pick_field "$line" "evidence_reason")"
        phase_source_reason="$(vaio_plan_summary_pick_field "$line" "phase_source_reason")"

        if [[ ( -n "$evidence_last_started" && "$evidence_last_started" != "-" ) \
            || ( -n "$evidence_last_good" && "$evidence_last_good" != "-" ) \
            || ( -n "$evidence_first_bad" && "$evidence_first_bad" != "-" ) \
            || ( -n "$evidence_reason" && "$evidence_reason" != "-" ) \
            || ( -n "$phase_source_reason" && "$phase_source_reason" != "-" ) ]]; then
            printf '%s\n' "$line"
            return 0
        fi
    done <"$summary_file"

    printf '%s\n' ""
}

vaio_plan_latest_bisect_summary() {
    local session_dir="$1"
    local base_root="${2:-${repo_root:-}}"
    local full_dir="$session_dir"
    local candidate=""

    if [[ "$full_dir" != /* && -n "$base_root" ]]; then
        full_dir="${base_root}/${full_dir}"
    fi
    [[ -d "$full_dir" ]] || return 0

    candidate="$(
        find "$full_dir" -type f -name 'summary.log' 2>/dev/null \
            | while IFS= read -r file; do
                if rg -q '^probe=' "$file"; then
                    printf '%s\n' "$file"
                fi
            done \
            | xargs -r ls -1t 2>/dev/null \
            | head -n 1
    )"
    printf '%s\n' "$candidate"
}

vaio_plan_latest_bisect_result() {
    local summary_file="$1"
    local candidate=""

    [[ -n "$summary_file" ]] || return 0
    candidate="$(dirname "$summary_file")/session-result.env"
    if [[ -f "$candidate" ]]; then
        printf '%s\n' "$candidate"
    fi
}

vaio_plan_phase_successor() {
    local phase="$1"
    local full_ladder=""
    local current=""
    local previous=""

    full_ladder="$(vaio_plan_profile_to_phases full)"
    IFS=',' read -r -a _vaio_plan_full_phases <<<"$full_ladder"
    for current in "${_vaio_plan_full_phases[@]}"; do
        if [[ "$previous" == "$phase" ]]; then
            printf '%s\n' "$current"
            return 0
        fi
        previous="$current"
    done
    printf '%s\n' ""
}

vaio_plan_extract_bisect_last_started_phase() {
    local summary_file="$1"
    [[ -f "$summary_file" ]] || return 0
    awk -F'[ =]' '/^probe=/ { phase = $4 } END { print phase }' "$summary_file"
}

vaio_plan_extract_bisect_last_good_phase() {
    local summary_file="$1"
    [[ -f "$summary_file" ]] || return 0
    awk -F= '
        /^probe_[0-9]+_phase=/ {
            split($1, a, "_")
            phase[a[2]] = $2
        }
        /^probe_[0-9]+_status=good$/ {
            split($1, a, "_")
            last_good_index = a[2]
        }
        END {
            if (last_good_index != "") {
                print phase[last_good_index]
            }
        }
    ' "$summary_file"
}

vaio_plan_extract_effective_bisect_evidence() {
    local summary_file="$1"
    local result_file="$2"
    local raw_last_started=""
    local raw_last_good=""
    local raw_first_bad=""
    local raw_reason=""
    local authoritative_terminal_bad=""
    local authoritative_last_started=""
    local authoritative_last_good=""
    local authoritative_first_bad=""
    local authoritative_reason=""

    [[ -f "$summary_file" ]] || return 0

    raw_last_started="$(vaio_plan_extract_bisect_last_started_phase "$summary_file")"
    raw_last_good="$(vaio_plan_extract_bisect_last_good_phase "$summary_file")"
    if [[ -f "$result_file" ]]; then
        raw_first_bad="$(awk -F= '$1 == "first_bad_phase" { print substr($0, index($0, "=") + 1) }' "$result_file" | tail -n 1)"
        raw_reason="$(awk -F= '$1 == "first_bad_reason" { print substr($0, index($0, "=") + 1) }' "$result_file" | tail -n 1)"
        authoritative_terminal_bad="$(awk -F= '$1 == "authoritative_terminal_bad" { print substr($0, index($0, "=") + 1) }' "$result_file" | tail -n 1)"
        authoritative_last_started="$(awk -F= '$1 == "authoritative_last_started_phase" { print substr($0, index($0, "=") + 1) }' "$result_file" | tail -n 1)"
        authoritative_last_good="$(awk -F= '$1 == "authoritative_last_good_phase" { print substr($0, index($0, "=") + 1) }' "$result_file" | tail -n 1)"
        authoritative_first_bad="$(awk -F= '$1 == "authoritative_first_bad_phase" { print substr($0, index($0, "=") + 1) }' "$result_file" | tail -n 1)"
        authoritative_reason="$(awk -F= '$1 == "authoritative_first_bad_reason" { print substr($0, index($0, "=") + 1) }' "$result_file" | tail -n 1)"
    fi

    awk -F= \
        -v raw_last_started="$raw_last_started" \
        -v raw_last_good="$raw_last_good" \
        -v raw_first_bad="$raw_first_bad" \
        -v raw_reason="$raw_reason" \
        -v authoritative_terminal_bad="$authoritative_terminal_bad" \
        -v authoritative_last_started="$authoritative_last_started" \
        -v authoritative_last_good="$authoritative_last_good" \
        -v authoritative_first_bad="$authoritative_first_bad" \
        -v authoritative_reason="$authoritative_reason" '
        function idx_for(key, suffix,    m) {
            if (match(key, "^probe_([0-9]+)_" suffix "$", m)) {
                return m[1] + 0
            }
            return 0
        }
        function norm(value) {
            return value == "" ? "-" : value
        }
        {
            idx = idx_for($1, "phase")
            if (idx > 0) {
                phase[idx] = substr($0, index($0, "=") + 1)
                if (idx > max_idx) {
                    max_idx = idx
                }
                next
            }
            idx = idx_for($1, "status")
            if (idx > 0) {
                status[idx] = substr($0, index($0, "=") + 1)
                next
            }
            idx = idx_for($1, "reason")
            if (idx > 0) {
                reason[idx] = substr($0, index($0, "=") + 1)
                next
            }
            idx = idx_for($1, "health")
            if (idx > 0) {
                health[idx] = substr($0, index($0, "=") + 1)
                next
            }
            idx = idx_for($1, "authoritative_terminal")
            if (idx > 0) {
                probe_authoritative_terminal[idx] = substr($0, index($0, "=") + 1)
                next
            }
        }
        END {
            effective_last_started = raw_last_started
            effective_last_good = raw_last_good
            effective_first_bad = raw_first_bad
            effective_reason = raw_reason
            terminal = 0
            last_good_before_terminal = ""

            if (authoritative_terminal_bad == "1" && authoritative_first_bad != "" && authoritative_first_bad != "none") {
                terminal = 1
                effective_last_started = authoritative_last_started
                effective_last_good = authoritative_last_good
                effective_first_bad = authoritative_first_bad
                effective_reason = authoritative_reason
            } else {
                for (idx = 1; idx <= max_idx; idx++) {
                    if (phase[idx] != "") {
                        effective_last_started = phase[idx]
                    }
                    if (status[idx] == "good" && phase[idx] != "") {
                        last_good_before_terminal = phase[idx]
                    }
                    if (status[idx] == "bad" && ((probe_authoritative_terminal[idx] == "1") || (health[idx] != "" && health[idx] != "remote_usable" && health[idx] != "-"))) {
                        terminal = 1
                        effective_last_started = phase[idx]
                        effective_last_good = last_good_before_terminal
                        effective_first_bad = phase[idx]
                        if (reason[idx] != "") {
                            effective_reason = reason[idx]
                        }
                        break
                    }
                }
            }

            print "last_started=" norm(effective_last_started)
            print "last_good=" norm(effective_last_good)
            print "first_bad=" norm(effective_first_bad)
            print "reason=" norm(effective_reason)
            print "authoritative_terminal_bad=" terminal
        }
    ' "$summary_file"
}

vaio_plan_profile_to_phases() {
    local profile="$1"

    case "$profile" in
        full)
            printf '%s\n' 'sync,owner,report,manifest,descriptor_op,descriptor_query,plan_op,plan_query_fetch,plan_query_offset,plan_query_config,plan_query_snapshot,prepare_op,prepare_query,arm,emit,validate,launch,issue,running,complete'
            ;;
        post_descriptor)
            printf '%s\n' 'descriptor_query,plan_op,plan_query_fetch,plan_query_offset,plan_query_config,plan_query_snapshot,prepare_op,prepare_query'
            ;;
        plan_query)
            printf '%s\n' 'plan_query_fetch,plan_query_offset,plan_query_config,plan_query_snapshot'
            ;;
        plan_query_fetch)
            printf '%s\n' 'plan_query_fetch'
            ;;
        plan_query_basic)
            printf '%s\n' 'plan_query_offset,plan_query_config'
            ;;
        plan_query_offset)
            printf '%s\n' 'plan_query_offset'
            ;;
        plan_query_config)
            printf '%s\n' 'plan_query_config'
            ;;
        plan_query_snapshot)
            printf '%s\n' 'plan_query_snapshot'
            ;;
        prepare_op)
            printf '%s\n' 'prepare_op'
            ;;
        prepare_query)
            printf '%s\n' 'prepare_query'
            ;;
        arm)
            printf '%s\n' 'arm'
            ;;
        emit)
            printf '%s\n' 'emit'
            ;;
        validate)
            printf '%s\n' 'validate'
            ;;
        launch)
            printf '%s\n' 'launch'
            ;;
        issue)
            printf '%s\n' 'issue'
            ;;
        running)
            printf '%s\n' 'running'
            ;;
        complete)
            printf '%s\n' 'complete'
            ;;
        *)
            printf '%s\n' ""
            ;;
    esac
}

vaio_plan_resolve_current_phase_from_summary() {
    local summary_file="$1"
    local latest_phase_evidence_row=""
    local resolved_profile="post_descriptor"
    local resolved_phases=""
    local resolved_phase=""
    local resolved_reason="fallback_post_descriptor"
    local source_kind=""
    local source_name=""
    local source_timestamp=""
    local source_session_dir=""
    local evidence_last_started=""
    local evidence_last_good=""
    local evidence_first_bad=""
    local evidence_reason=""
    local source_row_requested=""
    local source_row_resolved=""
    local source_row_child=""
    local source_row_outcome=""
    local source_row_last_state=""
    local source_row_phase_source=""
    local source_row_failed_phase=""
    local source_row_failed_step=""
    local source_row_module_srcversion=""
    local advanced_from_phase=""
    local next_phase=""

    vaio_plan_summary_init_columns "$summary_file"
    latest_phase_evidence_row="$(vaio_plan_summary_pick_latest_phase_evidence_row "$summary_file")"

    resolved_phases="$(vaio_plan_profile_to_phases "$resolved_profile")"

    if [[ -n "$latest_phase_evidence_row" ]]; then
        source_kind="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "kind")"
        source_timestamp="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "timestamp")"
        source_name="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "name")"
        source_session_dir="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "session_dir")"
        source_row_requested="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "requested")"
        source_row_resolved="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "resolved")"
        source_row_child="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "child")"
        source_row_outcome="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "outcome")"
        source_row_last_state="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "last_state")"
        source_row_phase_source="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "phase_source")"
        source_row_failed_phase="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "failed_phase")"
        source_row_failed_step="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "failed_step")"
        source_row_module_srcversion="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "module_srcversion")"
        evidence_last_started="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "evidence_last_started")"
        evidence_last_good="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "evidence_last_good")"
        evidence_first_bad="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "evidence_first_bad")"
        evidence_reason="$(vaio_plan_summary_pick_field "$latest_phase_evidence_row" "evidence_reason")"

        if [[ -n "$evidence_first_bad" && "$evidence_first_bad" != "-" ]]; then
            if [[ "$evidence_first_bad" != "none" ]]; then
                resolved_phase="$evidence_first_bad"
            fi
            if [[ -n "$evidence_reason" && "$evidence_reason" != "-" && "$evidence_reason" != "none" ]]; then
                resolved_reason="$evidence_reason"
            elif [[ -n "$resolved_phase" ]]; then
                resolved_reason="latest_phase_evidence_first_bad"
            fi
        elif [[ -n "$evidence_last_started" && "$evidence_last_started" != "-" ]] \
            && [[ "$evidence_last_started" != "$evidence_last_good" ]]; then
            resolved_phase="$evidence_last_started"
            resolved_reason="latest_phase_evidence_last_started"
        fi

        if [[ -z "$resolved_phase" ]]; then
            if [[ -n "$evidence_last_good" && "$evidence_last_good" != "-" && "$evidence_last_good" != "none" ]]; then
                advanced_from_phase="$evidence_last_good"
            elif [[ -n "$evidence_last_started" && "$evidence_last_started" != "-" && "$evidence_last_started" != "none" ]]; then
                advanced_from_phase="$evidence_last_started"
            fi

            if [[ -n "$advanced_from_phase" ]]; then
                next_phase="$(vaio_plan_phase_successor "$advanced_from_phase")"
                if [[ -n "$next_phase" ]]; then
                    resolved_phase="$next_phase"
                    resolved_reason="latest_phase_evidence_advanced"
                else
                    resolved_phase="$advanced_from_phase"
                    resolved_reason="latest_phase_evidence_terminal"
                fi
            fi
        fi
    fi

    local resolved_phase_profile=""
    resolved_phase_profile="$(vaio_plan_profile_to_phases "$resolved_phase")"
    if [[ -n "$resolved_phase_profile" ]]; then
        resolved_profile="$resolved_phase"
        resolved_phases="$resolved_phase_profile"
    elif [[ -n "$resolved_phase" ]]; then
        resolved_profile="post_descriptor"
        resolved_phases="$resolved_phase"
    fi

    printf '%s\n' \
        "resolved_profile=${resolved_profile}" \
        "resolved_phases=${resolved_phases}" \
        "resolved_phase=${resolved_phase}" \
        "resolved_reason=${resolved_reason}" \
        "source_kind=${source_kind}" \
        "source_name=${source_name}" \
        "source_timestamp=${source_timestamp}" \
        "source_session_dir=${source_session_dir}" \
        "source_row_requested=${source_row_requested}" \
        "source_row_resolved=${source_row_resolved}" \
        "source_row_child=${source_row_child}" \
        "source_row_outcome=${source_row_outcome}" \
        "source_row_last_state=${source_row_last_state}" \
        "source_row_phase_source=${source_row_phase_source}" \
        "source_row_failed_phase=${source_row_failed_phase}" \
        "source_row_failed_step=${source_row_failed_step}" \
        "source_row_module_srcversion=${source_row_module_srcversion}" \
        "source_row_evidence_last_started=${evidence_last_started}" \
        "source_row_evidence_last_good=${evidence_last_good}" \
        "source_row_evidence_first_bad=${evidence_first_bad}" \
        "source_row_evidence_reason=${evidence_reason}"
}
