#!/usr/bin/env bash
set -euo pipefail

SHARED_CONTROL_SUBMITS=2 exec "$(dirname "$0")/run-vaio-sgx-ioctl-range-query.sh"
