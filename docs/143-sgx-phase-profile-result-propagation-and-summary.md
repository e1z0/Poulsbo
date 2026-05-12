# SGX Phase-Profile Result Propagation And Summary

## Summary

The new `full` versus `post_descriptor` phase profiles were useful, but still
too buried in child manifests. This batch propagates that metadata upward and
surfaces it in the session summary output.

## Code

Updated scripts:

- [scripts/run-vaio-sgx-submit-op-phase-matrix.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-matrix.sh)
- [scripts/run-vaio-sgx-submit-op-phase-bisect.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-bisect.sh)
- [scripts/run-vaio-sgx-submit-op-phase-investigation.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-investigation.sh)
- [scripts/run-vaio-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-recovery-session.sh)
- [scripts/run-vaio-sgx-plan-guarded-deploy-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-guarded-deploy-session.sh)
- [scripts/run-vaio-sgx-plan-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-session.sh)
- [scripts/summarize-vaio-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-sessions.sh)

## Result Propagation

The phase tools now persist in their own `session-result.env`:

- `phase_profile`
- `phase_selection`
- `phases`

The generic recovery wrapper now copies those forward as:

- `validation_phase_profile`
- `validation_phase_selection`
- `validation_phases`

The later semantics follow-up also preserves the original request-side values:

- `validation_phase_profile_request`
- `validation_phase_selection_request`
- `validation_phases_request`

The guarded deploy parent now copies the recovery-side view as:

- `recovery_requested_phase_profile`
- `recovery_requested_phase_selection`
- `recovery_requested_phases`
- `recovery_phase_profile`
- `recovery_phase_selection`
- `recovery_phases`

The top-level plan session now records:

- `selected_phase_profile`
- `selected_phase_selection`
- `selected_phases`
- `selected_child_phase_profile`
- `selected_child_phase_selection`
- `selected_child_phases`

So a saved parent result now carries the actual phase-profile choice instead of
requiring manual inspection of child manifests, while still keeping the
operator-facing request separate from the child wrapper's own internal state.

## Summary Output

[scripts/summarize-vaio-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-sessions.sh)
now includes a `phase_profile` column.

It resolves profile metadata in this order:

- `selected_phase_profile`
- `recovery_phase_profile`
- `validation_phase_profile`
- `phase_profile`

and falls back to the saved selection mode when only an explicit phase list is
known.

It now renders:

- `profile:selection` when both are known
- `profile` when only the named profile is known
- `selection` when only the selection mode is known

## Local Verification

Local checks passed:

- `bash -n` on the touched scripts
- `MAX_POLLS=1 POLL_INTERVAL_SECS=1 PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=auto ./scripts/run-vaio-sgx-plan-session.sh`
- `PHASE_PROFILE=post_descriptor ./scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
- `MAX_SESSIONS=6 ./scripts/summarize-vaio-sessions.sh`

Validated blocked bundles:

- direct investigation profile check:
  [logs/vaio-ioctl/20260512-144134-sgx-submit-op-phase-investigation-profile-check](/root/Poulsbo/logs/vaio-ioctl/20260512-144134-sgx-submit-op-phase-investigation-profile-check)
- bounded top-level profile check:
  [logs/vaio-plan-session/20260512-144400-sgx-plan-session-profile-check-bounded](/root/Poulsbo/logs/vaio-plan-session/20260512-144400-sgx-plan-session-profile-check-bounded)
- explicit plan-phase alias semantics check:
  [logs/vaio-plan-session/20260512-151303-sgx-plan-session-plan-phases-alias-semantics-check](/root/Poulsbo/logs/vaio-plan-session/20260512-151303-sgx-plan-session-plan-phases-alias-semantics-check)

The first records:

- `phase_profile=post_descriptor`
- `phase_selection=profile`
- `phases=descriptor_query,plan_op,plan_query,prepare_op,prepare_query`

The second records:

- `selected_mode=recovery`
- `selected_phase_profile=post_descriptor`
- `selected_phase_selection=request_only`
- `selected_phases=descriptor_query,plan_op,plan_query,prepare_op,prepare_query`

The child recovery result also now preserves:

- `validation_phase_profile=post_descriptor`
- `validation_phase_selection=request_only`
- `validation_phases=descriptor_query,plan_op,plan_query,prepare_op,prepare_query`

And the summary helper now surfaces the same metadata directly, for example:

- `sgx_submit_plan_session ... phase_profile=post_descriptor:explicit`
- `sgx_submit_op_phase_investigation ... phase_profile=post_descriptor:profile`

The later explicit-alias run also records the intended split:

- top-level parent:
  - `selected_phase_profile=post_descriptor`
  - `selected_phase_selection=explicit`
  - `selected_phases=plan_op,plan_query`
  - `selected_child_phase_selection=request_only`
- child recovery:
  - `validation_phase_selection_request=explicit`
  - `validation_phase_selection=request_only`

No new hardware rerun was attempted in this batch.
