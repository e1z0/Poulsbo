# SGX Multi-Submit Legacy Guardrail

Date: 2026-05-11

## What Changed

The raw implementation entrypoint
[run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
now refuses direct legacy/sysfs execution by default.

That script still serves two valid roles:

- typed execution when called through
  [run-vaio-sgx-multi-submit-typed.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit-typed.sh)
  with the full ioctl setup and verification path
- explicitly opted-in legacy execution when
  `ALLOW_DIRECT_LEGACY_MULTI_SUBMIT=1`

The default dispatcher
[run-vaio-sgx-multi-submit-default.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit-default.sh)
now sets that opt-in automatically only for the explicit
`SGX_MULTI_SUBMIT_DRIVER_MODE=legacy` branch.

## Why

This prevents the exact foot-gun that re-triggered the old submit-sysfs
deadlock on a rollback-restored legacy override: calling the raw implementation
script directly instead of going through the typed/default wrapper layer.

The new behavior forces callers toward:

- `./scripts/run-vaio-sgx-multi-submit-default.sh`
- `./scripts/run-vaio-sgx-multi-submit-typed.sh`

and requires an explicit opt-in for:

```bash
ALLOW_DIRECT_LEGACY_MULTI_SUBMIT=1 ./scripts/run-vaio-sgx-multi-submit.sh
```

## Verification

Local verification passed:

- `bash -n scripts/run-vaio-sgx-multi-submit.sh`
- `bash -n scripts/run-vaio-sgx-multi-submit-default.sh`
- direct invocation now exits early with the expected guard message instead of
  attempting legacy target mutation

This change is local runner hardening only; it does not change the kernel tree
and does not need a kernel-module redeploy by itself.
