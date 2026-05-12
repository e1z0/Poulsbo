#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
health_script="${HEALTH_SCRIPT:-${script_dir}/check-vaio-target-health.sh}"
poll_interval_secs="${POLL_INTERVAL_SECS:-10}"
max_polls="${MAX_POLLS:-0}"
stop_on_overall="${STOP_ON_OVERALL:-}"
print_full_each_poll="${PRINT_FULL_EACH_POLL:-0}"
log_file="${LOG_FILE:-}"

usage() {
    cat <<'EOF'
Usage:
  watch-vaio-target-health.sh

Behavior:
  Polls the VAIO health probe repeatedly and prints a timestamped health
  timeline. Compact one-line summaries are printed every poll. Full probe
  details are printed on state transitions or on every poll when
  PRINT_FULL_EACH_POLL=1.

Environment:
  POLL_INTERVAL_SECS    Seconds between polls. Default: 10
  MAX_POLLS             Stop after this many polls. 0 means wait forever.
  STOP_ON_OVERALL       Stop successfully when overall=<value>.
  PRINT_FULL_EACH_POLL  Print full probe output every poll. Default: 0
  LOG_FILE              Optional file to append watcher output to.
  HEALTH_SCRIPT         Override health probe path.
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

log_line() {
    local line="$1"
    echo "$line"
    if [[ -n "$log_file" ]]; then
        printf '%s\n' "$line" >>"$log_file"
    fi
}

poll_count=0
previous_signature=""

while :; do
    poll_count=$((poll_count + 1))
    timestamp="$(date -Iseconds)"
    health_output="$("$health_script")"

    overall="$(printf '%s\n' "$health_output" | awk -F= '$1 == "overall" { print $2 }')"
    ping_status="$(printf '%s\n' "$health_output" | awk -F= '$1 == "ping_status" { print $2 }')"
    tcp_status="$(printf '%s\n' "$health_output" | awk -F= '$1 == "tcp_22_status" { print $2 }')"
    ssh_status="$(printf '%s\n' "$health_output" | awk -F= '$1 == "ssh_pubkey_status" { print $2 }')"
    signature="${overall}|${ping_status}|${tcp_status}|${ssh_status}"

    log_line "timestamp=${timestamp} poll=${poll_count} overall=${overall} ping_status=${ping_status} tcp_22_status=${tcp_status} ssh_pubkey_status=${ssh_status}"

    if [[ "$print_full_each_poll" == "1" || "$signature" != "$previous_signature" ]]; then
        while IFS= read -r line; do
            log_line "  ${line}"
        done <<<"$health_output"
    fi

    previous_signature="$signature"

    if [[ -n "$stop_on_overall" && "$overall" == "$stop_on_overall" ]]; then
        exit 0
    fi

    if [[ "$max_polls" -gt 0 && "$poll_count" -ge "$max_polls" ]]; then
        exit 1
    fi

    sleep "$poll_interval_secs"
done
