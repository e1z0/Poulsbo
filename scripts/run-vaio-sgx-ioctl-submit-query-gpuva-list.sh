#!/usr/bin/env bash
set -euo pipefail

SHARED_CONTROL_SUBMITS=2 ENUMERATE_MATCHES=1 exec "$(dirname "$0")/run-vaio-sgx-ioctl-submit-query-gpuva.sh"
