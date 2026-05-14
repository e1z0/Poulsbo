#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

session_name="${SESSION_NAME:-sgx-stable-soak}"
session_kind="sgx_stable_soak"
logs_root="${LOGS_ROOT:-${repo_root}/logs/vaio-stability}"
session_timestamp="${SESSION_TIMESTAMP:-$(date +%Y%m%d-%H%M%S)}"
started_at="$(date -Iseconds)"
session_dir="$("${script_dir}/allocate-session-dir.sh" "$logs_root" "$session_name" "$session_timestamp")"
children_root="${session_dir}/children"
summary_log="${session_dir}/summary.log"
manifest_file="${session_dir}/session.env"
result_file="${session_dir}/session-result.env"
health_script="${HEALTH_SCRIPT:-${script_dir}/check-vaio-target-health.sh}"
state_script="${STATE_SCRIPT:-${script_dir}/show-vaio-gma500-state.sh}"
plan_script="${PLAN_SCRIPT:-${script_dir}/run-vaio-sgx-plan-session.sh}"
focused_script="${FOCUSED_SCRIPT:-${script_dir}/run-vaio-sgx-submit-typed.sh}"
multi_script="${MULTI_SCRIPT:-${script_dir}/run-vaio-sgx-multi-submit-typed.sh}"
cycles="${CYCLES:-3}"

usage() {
    cat <<'EOF'
Usage:
  run-vaio-sgx-stable-soak-session.sh

Behavior:
  Runs repeated stability cycles against the live VAIO target. Each cycle runs:
    1. exact complete plan recovery
    2. focused typed submit proof
    3. typed multi-submit proof
  All child outputs are bundled under one session directory.

Environment:
  SESSION_NAME    Session suffix. Default: sgx-stable-soak
  LOGS_ROOT       Base directory for logs.
  CYCLES          Number of soak cycles. Default: 3
  HEALTH_SCRIPT   Override health probe path.
  STATE_SCRIPT    Override remote-state capture path.
  PLAN_SCRIPT     Override plan-session wrapper.
  FOCUSED_SCRIPT  Override focused typed submit wrapper.
  MULTI_SCRIPT    Override typed multi-submit wrapper.
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

for required in "$health_script" "$state_script" "$plan_script" \
    "$focused_script" "$multi_script"; do
    if [[ ! -x "$required" ]]; then
        echo "required executable missing: $required" >&2
        exit 1
    fi
done

if [[ "$cycles" -lt 1 ]]; then
    echo "CYCLES must be at least 1" >&2
    exit 1
fi

{
    echo "timestamp=${session_timestamp}"
    echo "started_at=${started_at}"
    echo "session_kind=${session_kind}"
    echo "session_name=${session_name}"
    echo "session_dir=${session_dir}"
    echo "children_root=${children_root}"
    echo "health_script=${health_script}"
    echo "state_script=${state_script}"
    echo "plan_script=${plan_script}"
    echo "focused_script=${focused_script}"
    echo "multi_script=${multi_script}"
    echo "cycles=${cycles}"
    echo "local_module_path=${repo_root}/build/devnull-vaio-gma500-src/gma500_gfx.ko"
    if [[ -f "${repo_root}/build/devnull-vaio-gma500-src/gma500_gfx.ko" ]]; then
        echo "local_module_srcversion=$(modinfo -F srcversion "${repo_root}/build/devnull-vaio-gma500-src/gma500_gfx.ko" 2>/dev/null || echo unknown)"
    else
        echo "local_module_srcversion=missing"
    fi
} >"$manifest_file"

echo "session_dir=${session_dir}" | tee "$summary_log"
echo "summary_log=${summary_log}" | tee -a "$summary_log"
echo "result_file=${result_file}" | tee -a "$summary_log"

extract_last_value() {
    local file="$1"
    local key="$2"
    awk -F= -v want="$key" '$1 == want { value = substr($0, index($0, "=") + 1) } END { print value }' "$file"
}

append_log() {
    local file="$1"
    while IFS= read -r line; do
        printf '%s\n' "$line" | tee -a "$summary_log"
    done <"$file"
}

last_overall="unknown"
final_driver_srcversion="unknown"
session_outcome="interrupted"
failed_cycle=""
failed_step=""
failed_rc=""

preflight_log="${session_dir}/preflight-health.log"
"$health_script" >"$preflight_log" 2>&1
append_log "$preflight_log"
last_overall="$(extract_last_value "$preflight_log" overall)"
if [[ "$last_overall" != "remote_usable" ]]; then
    session_outcome="blocked_preflight"
    failed_step="preflight_health"
    failed_rc="1"
    {
        echo "session_kind=${session_kind}"
        echo "session_outcome=${session_outcome}"
        echo "cycles=${cycles}"
        echo "completed_cycles=0"
        echo "last_overall=${last_overall}"
        echo "final_driver_srcversion=unknown"
        echo "failed_cycle=${failed_cycle}"
        echo "failed_step=${failed_step}"
        echo "failed_rc=${failed_rc}"
        echo "finished_at=$(date -Iseconds)"
    } >"$result_file"
    exit 2
fi

completed_cycles=0

for cycle in $(seq 1 "$cycles"); do
    cycle_dir="${children_root}/cycle${cycle}"
    mkdir -p "$cycle_dir"
    echo "cycle=${cycle}" | tee -a "$summary_log"

    plan_driver_log="${cycle_dir}/plan-driver.log"
    set +e
    SESSION_NAME="sgx-stable-plan-cycle${cycle}" \
    LOGS_ROOT="${cycle_dir}/plan-session" \
    PLAN_SESSION_MODE=recovery \
    PLAN_PHASE_PROFILE=complete \
    "$plan_script" >"$plan_driver_log" 2>&1
    plan_rc=$?
    set -e
    append_log "$plan_driver_log"
    if [[ "$plan_rc" -ne 0 ]]; then
        session_outcome="failed"
        failed_cycle="$cycle"
        failed_step="plan"
        failed_rc="$plan_rc"
        break
    fi

    focused_dir="$("${script_dir}/allocate-session-dir.sh" "${cycle_dir}/focused" "sgx-submit-typed-cycle${cycle}" "$session_timestamp")"
    focused_log="${focused_dir}/summary.log"
    set +e
    "$focused_script" >"$focused_log" 2>&1
    focused_rc=$?
    set -e
    append_log "$focused_log"
    if [[ "$focused_rc" -ne 0 ]]; then
        session_outcome="failed"
        failed_cycle="$cycle"
        failed_step="focused"
        failed_rc="$focused_rc"
        break
    fi

    multi_dir="$("${script_dir}/allocate-session-dir.sh" "${cycle_dir}/multi" "sgx-multi-submit-typed-cycle${cycle}" "$session_timestamp")"
    multi_log="${multi_dir}/summary.log"
    set +e
    "$multi_script" >"$multi_log" 2>&1
    multi_rc=$?
    set -e
    append_log "$multi_log"
    if [[ "$multi_rc" -ne 0 ]]; then
        session_outcome="failed"
        failed_cycle="$cycle"
        failed_step="multi"
        failed_rc="$multi_rc"
        break
    fi

    cycle_health_log="${cycle_dir}/post-cycle-health.log"
    "$health_script" >"$cycle_health_log" 2>&1
    append_log "$cycle_health_log"
    last_overall="$(extract_last_value "$cycle_health_log" overall)"
    if [[ "$last_overall" != "remote_usable" ]]; then
        session_outcome="failed"
        failed_cycle="$cycle"
        failed_step="post_cycle_health"
        failed_rc="1"
        break
    fi

    completed_cycles="$cycle"
done

if [[ "$session_outcome" == "interrupted" ]]; then
    session_outcome="completed"
fi

final_state_log="${session_dir}/final-state.log"
set +e
"$state_script" >"$final_state_log" 2>&1
state_rc=$?
set -e
append_log "$final_state_log"
if [[ "$state_rc" -eq 0 ]]; then
    final_driver_srcversion="$(extract_last_value "$final_state_log" driver_srcversion)"
    last_overall="remote_usable"
else
    if [[ "$session_outcome" == "completed" ]]; then
        session_outcome="post_state_capture_failed"
        failed_step="final_state"
        failed_rc="$state_rc"
    fi
fi

{
    echo "session_kind=${session_kind}"
    echo "session_outcome=${session_outcome}"
    echo "cycles=${cycles}"
    echo "completed_cycles=${completed_cycles}"
    echo "last_overall=${last_overall}"
    echo "final_driver_srcversion=${final_driver_srcversion}"
    echo "failed_cycle=${failed_cycle}"
    echo "failed_step=${failed_step}"
    echo "failed_rc=${failed_rc}"
    echo "finished_at=$(date -Iseconds)"
} >"$result_file"

if [[ "$session_outcome" != "completed" ]]; then
    exit 1
fi
