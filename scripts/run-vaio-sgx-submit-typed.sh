#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ -n "${LOCAL_BIN:-}" && -z "${LOCAL_PROBE_BIN:-}" ]]; then
    export LOCAL_PROBE_BIN="${LOCAL_BIN}"
fi
if [[ -n "${TARGET_BIN:-}" && -z "${TARGET_PROBE_BIN:-}" ]]; then
    export TARGET_PROBE_BIN="${TARGET_BIN}"
fi
if [[ -n "${TARGET_STAGE:-}" && -z "${TARGET_PROBE_STAGE:-}" ]]; then
    export TARGET_PROBE_STAGE="${TARGET_STAGE}"
fi

exec "${script_dir}/run-vaio-sgx-ioctl-submit-op.sh"
