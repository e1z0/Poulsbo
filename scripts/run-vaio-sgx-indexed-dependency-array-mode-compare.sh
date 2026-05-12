#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export SESSION_NAME="${SESSION_NAME:-sgx-indexed-dependency-array-mode-compare}"
export SESSION_KIND="${SESSION_KIND:-sgx_indexed_dependency_array_mode_compare}"
export MULTI_SUBMIT_SCRIPT="${MULTI_SUBMIT_SCRIPT:-${script_dir}/run-vaio-sgx-indexed-dependency-array.sh}"

exec "${script_dir}/run-vaio-sgx-multi-submit-mode-compare.sh" "$@"
