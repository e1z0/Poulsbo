#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export SESSION_NAME="${SESSION_NAME:-sgx-multi-source-ticket-dependency-mode-compare}"
export SESSION_KIND="${SESSION_KIND:-sgx_multi_source_ticket_dependency_mode_compare}"
export MULTI_SUBMIT_SCRIPT="${MULTI_SUBMIT_SCRIPT:-${script_dir}/run-vaio-sgx-multi-source-ticket-dependency.sh}"

exec "${script_dir}/run-vaio-sgx-multi-submit-mode-compare.sh" "$@"
