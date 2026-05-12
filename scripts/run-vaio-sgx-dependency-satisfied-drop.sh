#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export DROP_SOURCE_AFTER_RELEASE="${DROP_SOURCE_AFTER_RELEASE:-1}"
export SUBMIT_A_REFILL_CYCLES="${SUBMIT_A_REFILL_CYCLES:-0}"
export SUBMIT_B_REFILL_CYCLES="${SUBMIT_B_REFILL_CYCLES:-0}"

"${script_dir}/run-vaio-sgx-multi-submit-default.sh"
