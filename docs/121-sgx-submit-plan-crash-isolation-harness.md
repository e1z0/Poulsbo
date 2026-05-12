# Submit `plan` Crash-Isolation Harness

## Summary

The first `plan`-block hardware attempt took the VAIO down hard enough that the
machine never returned during bounded health polling. The next useful work is
controlled isolation of the failing phase after console recovery.

This batch adds that isolation layer.

## Code

Focused runner changes:

- `scripts/run-vaio-sgx-ioctl-submit-op.sh`

New wrapper:

- `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`

## Focused Runner Changes

`run-vaio-sgx-ioctl-submit-op.sh` now accepts:

- `STOP_AFTER_STAGE=<stage>`

Valid stages are:

- `sync`
- `owner`
- `report`
- `manifest`
- `descriptor`
- `descriptor_op`
- `descriptor_query`
- `plan`
- `plan_op`
- `plan_query`
- `prepare`
- `prepare_op`
- `prepare_query`
- `arm`
- `emit`
- `validate`
- `launch`
- `issue`
- `running`
- `complete`

After each successful stage the runner now prints:

- `STAGE_OK=<stage>`

If `STOP_AFTER_STAGE` matches, the runner prints:

- `STOP_AFTER_STAGE_REACHED=<stage>`

and exits cleanly through its normal cleanup path.

## Phase-Matrix Wrapper

`run-vaio-sgx-submit-op-phase-matrix.sh` creates one timestamped session bundle
under `logs/vaio-ioctl/` and then:

1. checks target health
2. captures pre-state
3. reruns the focused typed submit-op path once per stage with
   `STOP_AFTER_STAGE=<stage>`
4. probes target health after every stage
5. stops on the first phase failure or the first unhealthy target state
6. captures post-state and writes a structured `session-result.env`

Default phase order:

- `sync`
- `owner`
- `report`
- `manifest`
- `descriptor_op`
- `descriptor_query`
- `plan_op`
- `plan_query`
- `prepare_op`
- `prepare_query`
- `arm`
- `emit`
- `validate`
- `launch`
- `issue`
- `running`
- `complete`

It also still supports older coarse aliases such as:

- `PHASES=plan,prepare,arm`

and now supports narrower post-`descriptor` reruns such as:

- `PHASES=descriptor_query,plan_op,plan_query,prepare_op,prepare_query`

## Local Verification

Local checks passed:

- `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `bash -n scripts/run-vaio-sgx-submit-op-phase-matrix.sh`

No hardware validation was possible in this batch because the VAIO was still
down after the earlier `plan` attempt.

## Next Use

After console recovery, the first recommended run is:

```bash
./scripts/run-vaio-sgx-submit-op-phase-matrix.sh
```

If that identifies a narrower failure window, follow with:

```bash
PHASES=descriptor_query,plan_op,plan_query,prepare_op,prepare_query ./scripts/run-vaio-sgx-submit-op-phase-matrix.sh
```
