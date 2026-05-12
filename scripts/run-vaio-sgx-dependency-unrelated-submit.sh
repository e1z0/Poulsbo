#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export DROP_SOURCE_AFTER_RELEASE="${DROP_SOURCE_AFTER_RELEASE:-1}"
export RUN_UNRELATED_AFTER_SATISFIED_DROP="${RUN_UNRELATED_AFTER_SATISFIED_DROP:-1}"
export PROBE_COUNT="${PROBE_COUNT:-15}"
export SUBMIT_A_REFILL_CYCLES="${SUBMIT_A_REFILL_CYCLES:-0}"
export SUBMIT_B_REFILL_CYCLES="${SUBMIT_B_REFILL_CYCLES:-0}"
export SUBMIT_C_REFILL_CYCLES="${SUBMIT_C_REFILL_CYCLES:-0}"

"${script_dir}/run-vaio-sgx-multi-submit-default.sh"
