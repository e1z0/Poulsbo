#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
health_script="${HEALTH_SCRIPT:-${script_dir}/check-vaio-target-health.sh}"
poll_interval_secs="${POLL_INTERVAL_SECS:-10}"
max_polls="${MAX_POLLS:-0}"

usage() {
    cat <<'EOF'
Usage:
  wait-vaio-target-ready.sh [command...]

Behavior:
  Polls the VAIO health probe until it reports overall=remote_usable.
  If a command is provided, runs it immediately after the target becomes usable.

Environment:
  POLL_INTERVAL_SECS   Seconds between polls. Default: 10
  MAX_POLLS            Stop after this many polls. 0 means wait forever.
  HEALTH_SCRIPT        Override health probe path.
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if [[ ! -x "$health_script" ]]; then
    echo "health script is not executable: $health_script" >&2
    exit 1
fi

poll_count=0

while :; do
    poll_count=$((poll_count + 1))
    health_output="$("$health_script")"
    overall="$(printf '%s\n' "$health_output" | awk -F= '$1 == "overall" { print $2 }')"

    echo "poll=${poll_count}"
    printf '%s\n' "$health_output"

    if [[ "$overall" == "remote_usable" ]]; then
        if [[ "$#" -gt 0 ]]; then
            exec "$@"
        fi
        exit 0
    fi

    if [[ "$max_polls" -gt 0 && "$poll_count" -ge "$max_polls" ]]; then
        echo "target did not become remote_usable within ${max_polls} polls" >&2
        exit 1
    fi

    sleep "$poll_interval_secs"
done
