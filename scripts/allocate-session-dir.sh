#!/usr/bin/env bash
set -euo pipefail

if [[ "$#" -ne 3 ]]; then
    echo "usage: allocate-session-dir.sh <logs_root> <session_name> <timestamp>" >&2
    exit 1
fi

logs_root="$1"
session_name="$2"
timestamp="$3"
base_dir="${logs_root}/${timestamp}-${session_name}"
session_dir="$base_dir"
suffix=1

mkdir -p "$logs_root"

while ! mkdir "$session_dir" 2>/dev/null; do
    session_dir="${base_dir}-${suffix}"
    suffix=$((suffix + 1))
done

printf '%s\n' "$session_dir"
