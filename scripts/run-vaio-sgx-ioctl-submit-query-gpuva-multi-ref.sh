#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SHARED_CONTROL_SUBMITS=2 "${script_dir}/run-vaio-sgx-ioctl-submit-query-gpuva.sh" "$@"
