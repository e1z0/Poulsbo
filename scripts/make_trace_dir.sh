#!/usr/bin/env bash
set -euo pipefail

label="${1:-trace-session}"
timestamp="$(date -u +%Y%m%dT%H%M%SZ)"
safe_label="$(printf '%s' "$label" | tr ' /' '__')"
dir="traces/${timestamp}-${safe_label}"

mkdir -p "$dir"/{raw,annotated,screenshots,notes,artifacts}

cat >"${dir}/README.md" <<EOF
# ${safe_label}

- created_utc: ${timestamp}
- label: ${label}

## Checklist

- hardware and firmware version recorded
- driver version recorded
- exact workload recorded
- raw traces placed in \`raw/\`
- annotated traces placed in \`annotated/\`
- screenshots placed in \`screenshots/\`
- summary notes placed in \`notes/\`
EOF

echo "Created ${dir}"
