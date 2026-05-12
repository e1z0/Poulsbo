#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
logs_root="${LOGS_ROOT:-${repo_root}/logs}"
summary_script="${SUMMARY_SCRIPT:-${script_dir}/summarize-vaio-plan-sessions.sh}"
summary_lib="${SUMMARY_LIB:-${script_dir}/lib-vaio-plan-summary.sh}"

usage() {
    cat <<'EOF'
Usage:
  resolve-vaio-plan-current-phase.sh

Behavior:
  Resolves the current best SGX `plan` recovery boundary from saved session
  evidence. It prefers the newest remote-usable phase-evidence `plan_session`
  row with bisect evidence, and falls back to `post_descriptor` when no newer
  boundary is available.

Output:
  Emits env-style key/value lines:
    resolved_profile
    resolved_phases
    resolved_phase
    resolved_reason
    source_kind
    source_name
    source_timestamp
    source_session_dir
    source_row_requested
    source_row_resolved
    source_row_child
    source_row_outcome
    source_row_last_state
    source_row_phase_source
    source_row_failed_phase
    source_row_failed_step
    source_row_module_srcversion
    source_row_evidence_last_started
    source_row_evidence_last_good
    source_row_evidence_first_bad
    source_row_evidence_reason

Environment:
  LOGS_ROOT       Base logs directory. Default: <repo>/logs
  SUMMARY_SCRIPT  Override plan-session summary helper path.
  SUMMARY_LIB     Override shared plan-summary helper path.
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if [[ ! -x "$summary_script" ]]; then
    echo "required executable missing: $summary_script" >&2
    exit 1
fi

if [[ ! -f "$summary_lib" ]]; then
    echo "required helper missing: $summary_lib" >&2
    exit 1
fi

# shellcheck source=/root/Poulsbo/scripts/lib-vaio-plan-summary.sh
source "$summary_lib"

tmp_summary="$(mktemp)"
trap 'rm -f "$tmp_summary"' EXIT

OUTPUT_FORMAT=tsv "$summary_script" >"$tmp_summary"
vaio_plan_resolve_current_phase_from_summary "$tmp_summary"
