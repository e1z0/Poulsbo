#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export RUN_SECONDARY_DEPENDENCY_SOURCE="${RUN_SECONDARY_DEPENDENCY_SOURCE:-1}"
export PROBE_COUNT="${PROBE_COUNT:-15}"
export SUBMIT_A_REFILL_CYCLES="${SUBMIT_A_REFILL_CYCLES:-0}"
export SUBMIT_B_REFILL_CYCLES="${SUBMIT_B_REFILL_CYCLES:-0}"
export SUBMIT_C_REFILL_CYCLES="${SUBMIT_C_REFILL_CYCLES:-0}"

"${script_dir}/run-vaio-sgx-multi-submit-default.sh"
