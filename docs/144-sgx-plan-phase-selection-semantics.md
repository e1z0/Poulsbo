# SGX `plan` Phase-Selection Semantics

## Summary

The earlier phase-profile propagation batch made the `full` versus
`post_descriptor` choice visible in parent results, but it still blurred two
different meanings:

- the operator-facing selection made at the top-level `plan` wrapper
- the child recovery wrapper's internal `request_only` state when the target
  never reached `overall=remote_usable`

This follow-up keeps those two views separate and makes the session summary
render both the profile and the selection mode.

## Code

Updated scripts:

- [scripts/run-vaio-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-recovery-session.sh)
- [scripts/run-vaio-sgx-plan-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-recovery-session.sh)
- [scripts/run-vaio-sgx-plan-guarded-deploy-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-guarded-deploy-session.sh)
- [scripts/run-vaio-sgx-plan-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-session.sh)
- [scripts/summarize-vaio-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-sessions.sh)

## Result Semantics

The plan-specific wrappers now treat these aliases as the preferred interface:

- `PLAN_PHASE_PROFILE=<auto|full|post_descriptor>`
- `PLAN_PHASES=<csv>`

The recovery wrapper now forwards one more request field:

- `PHASE_SELECTION=<profile|explicit>`

and the generic recovery-session result now preserves all three request-side
keys:

- `validation_phase_profile_request`
- `validation_phase_selection_request`
- `validation_phases_request`

The top-level plan-session result now keeps the operator-facing selection in:

- `selected_phase_profile`
- `selected_phase_selection`
- `selected_phases`

and records the child recovery view separately in:

- `selected_child_phase_profile`
- `selected_child_phase_selection`
- `selected_child_phases`

That means a blocked recovery session can now say, unambiguously:

- top-level parent selected `explicit`
- child recovery stayed at `request_only`

without losing the exact requested phase list.

## Summary Output

[scripts/summarize-vaio-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-sessions.sh)
now renders the `phase_profile` column as:

- `profile:selection` when both are known
- just `profile` when no separate selection is available
- just `selection` when only that is known

So recent `plan` bundles now show values such as:

- `post_descriptor:explicit`
- `post_descriptor:request_only`
- `post_descriptor:profile`

## Local Verification

Local checks passed:

- `bash -n scripts/run-vaio-recovery-session.sh`
- `bash -n scripts/run-vaio-sgx-plan-recovery-session.sh`
- `bash -n scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`
- `bash -n scripts/run-vaio-sgx-plan-session.sh`
- `bash -n scripts/summarize-vaio-sessions.sh`
- `MAX_POLLS=1 POLL_INTERVAL_SECS=1 PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_op,plan_query ./scripts/run-vaio-sgx-plan-session.sh`
- `MAX_SESSIONS=6 ./scripts/summarize-vaio-sessions.sh`

Canonical blocked bundle:

- [logs/vaio-plan-session/20260512-151303-sgx-plan-session-plan-phases-alias-semantics-check](/root/Poulsbo/logs/vaio-plan-session/20260512-151303-sgx-plan-session-plan-phases-alias-semantics-check)

That run now records:

- top-level parent:
  - `selected_phase_profile=post_descriptor`
  - `selected_phase_selection=explicit`
  - `selected_phases=plan_op,plan_query`
  - `selected_child_phase_profile=post_descriptor`
  - `selected_child_phase_selection=request_only`
  - `selected_child_phases=plan_op,plan_query`
- child recovery result:
  - `validation_phase_profile_request=post_descriptor`
  - `validation_phase_selection_request=explicit`
  - `validation_phases_request=plan_op,plan_query`
  - `validation_phase_profile=post_descriptor`
  - `validation_phase_selection=request_only`
  - `validation_phases=plan_op,plan_query`

The session summary now shows the same bundle as:

- `sgx_submit_plan_session ... post_descriptor:explicit`

No new hardware rerun was attempted in this batch.
