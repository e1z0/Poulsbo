#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export USE_IOCTL_OBJECT_SETUP="${USE_IOCTL_OBJECT_SETUP:-1}"
export USE_IOCTL_SUBMIT_SETUP="${USE_IOCTL_SUBMIT_SETUP:-1}"
export USE_IOCTL_DEPENDENCY_SETUP="${USE_IOCTL_DEPENDENCY_SETUP:-1}"
export USE_IOCTL_RANGE_DISCOVERY="${USE_IOCTL_RANGE_DISCOVERY:-1}"
export USE_IOCTL_VERIFICATION="${USE_IOCTL_VERIFICATION:-1}"

"${script_dir}/run-vaio-sgx-multi-submit.sh"
