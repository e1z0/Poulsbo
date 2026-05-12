# SGX Plan Pipe-Free Timeout Capture

Date: 2026-05-12

## Live Rerun Result

The VAIO came back to `overall=remote_usable`, still running override
`srcversion 0EEEE302E3FDA9F59B7CFB9`, so the current candidate was rerun
without redeploy through:

```bash
PLAN_SESSION_MODE=recovery ./scripts/run-vaio-sgx-plan-session.sh
```

The new parent bundle is:

- `logs/vaio-plan-session/20260512-132138-sgx-plan-session`

The first cumulative probe was still `phase=prepare`, and the live submit
runner again advanced through:

- `STAGE_OK=sync`
- `STAGE_OK=owner`
- `STAGE_OK=report`
- `STAGE_OK=manifest`
- `STAGE_OK=descriptor`

before the target dropped off the network again.

That means the local `plan` dword-count fix did not regress the already-good
prefix; the failure boundary is still at or after the `descriptor -> plan /
prepare` edge on the current guarded-deploy candidate.

## Wrapper Bug Found

This rerun exposed a new local orchestration bug.

The timed focused child actually died, but the wrapper stack stayed hung
locally because several parent scripts were capturing child output with:

- `... | tee logfile`

When timed-out descendants kept the pipe write side open, `tee` never saw EOF,
so the parent bisect / investigation / recovery / operator wrappers failed to
finish even though the timed child had already been killed.

## Fix

The wrapper stack now uses direct file capture plus explicit replay instead of
pipeline-based `tee` in the critical child paths.

Updated scripts:

- `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
- `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
- `scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
- `scripts/run-vaio-recovery-session.sh`
- `scripts/run-vaio-sgx-plan-session.sh`
- `scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`
- `scripts/run-vaio-guarded-deploy-session.sh`

The new local pattern is:

- run child with stdout/stderr redirected straight to a log file
- capture the real child return code directly
- replay the finished log with `cat`

That removes `tee` from the timeout-critical paths and prevents pipe-hold
hangs after target loss.

## Local Validation

Validated locally:

- `bash -n` on all touched wrapper scripts
- cleanup of the already-hung local wrapper processes from the failed rerun

## Current Hardware State

After the partial live rerun, the VAIO is back to:

- `ping_status=fail`
- `tcp_22_status=closed`
- `ssh_pubkey_status=no_route`
- `overall=ssh_port_down`

So the wrapper-side timeout-capture fix is ready, but the next bounded result
still depends on target recovery.
