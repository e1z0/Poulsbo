# SGX `plan` Post-Descriptor Subphase Isolation

## Summary

The current live `plan` candidate is already past the older `kick` rejection.
The useful remaining question is narrower:

- does the next failure happen in the `plan` write itself
- in the immediate `plan` readback and validation
- in the first `prepare` materialization write
- or in the immediate `prepare` readback

This batch sharpens the focused submit runner and the phase wrappers so the
next live rerun can answer that directly.

## Code

Updated scripts:

- [scripts/run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
- [scripts/run-vaio-sgx-submit-op-phase-matrix.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-matrix.sh)
- [scripts/run-vaio-sgx-submit-op-phase-bisect.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-bisect.sh)
- [scripts/run-vaio-sgx-submit-op-phase-investigation.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-investigation.sh)

## Focused Runner Changes

The focused runner now accepts these finer stop markers in addition to the
older coarse names:

- `descriptor_op`
- `descriptor_query`
- `plan_op`
- `plan_query`
- `prepare_op`
- `prepare_query`

Backward compatibility is kept:

- `descriptor` still stops after `descriptor_query`
- `plan` still stops after `plan_query`
- `prepare` still stops after `prepare_query`

The `descriptor`, `plan`, and `prepare` sections also now use named capture and
assert steps:

- `submit_op_descriptor_focus`
- `query_by_id_after_descriptor`
- `submit_op_plan_focus`
- `query_by_id_after_plan`
- `submit_op_prepare_focus`
- `query_by_id_after_prepare`

That means the next live rerun can preserve one explicit failed-step name even
when the boundary is inside the old coarse `prepare` prefix.

## Wrapper Changes

The default phase order for matrix, bisect, and investigation is now:

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

This keeps the older recovery entrypoints intact, but makes the default
operator path finer exactly where the current live boundary sits.

## Expected Next Answer

After the next recovery-mode rerun, the earliest bad cumulative prefix should
distinguish between:

- `descriptor_query`
- `plan_op`
- `plan_query`
- `prepare_op`
- `prepare_query`

instead of reporting only the old coarse `prepare` boundary.

## Local Verification

Local checks passed:

- `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `bash -n scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
- `bash -n scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
- `bash -n scripts/run-vaio-sgx-submit-op-phase-investigation.sh`

No new hardware rerun was attempted in this batch.
