#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
multi_submit_driver_mode="${SGX_MULTI_SUBMIT_DRIVER_MODE:-typed}"

case "${multi_submit_driver_mode}" in
    typed)
        exec "${script_dir}/run-vaio-sgx-multi-submit-typed.sh"
        ;;
    legacy)
        export ALLOW_DIRECT_LEGACY_MULTI_SUBMIT="${ALLOW_DIRECT_LEGACY_MULTI_SUBMIT:-1}"
        exec "${script_dir}/run-vaio-sgx-multi-submit.sh"
        ;;
    *)
        echo "SGX_MULTI_SUBMIT_DRIVER_MODE must be 'typed' or 'legacy'" >&2
        exit 1
        ;;
esac
