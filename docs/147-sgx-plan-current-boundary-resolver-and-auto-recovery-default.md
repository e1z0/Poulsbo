# SGX `plan` Current-Boundary Resolver And Auto-Recovery Default

## Summary

The saved `plan` evidence had already moved the live boundary forward to
`plan_query`, but the operator stack still treated that as advisory state. The
status report knew how to recommend `plan_query`, while the recovery wrappers
still defaulted to the broader `post_descriptor` window unless the operator
spelled the narrower phase list by hand.

This batch centralizes that choice.

## Added Script

- [scripts/resolve-vaio-plan-current-phase.sh](/root/Poulsbo/scripts/resolve-vaio-plan-current-phase.sh)

It reads the saved `plan` session summary and resolves one current recovery
boundary from the newest remote-usable phase-evidence bundle.

Current output keys:

- `resolved_profile`
- `resolved_phases`
- `resolved_phase`
- `resolved_reason`
- `source_kind`
- `source_name`
- `source_timestamp`
- `source_session_dir`
- `source_row_requested`
- `source_row_resolved`
- `source_row_child`
- `source_row_evidence_last_good`
- `source_row_evidence_first_bad`
- `source_row_evidence_reason`

On the current saved live evidence it resolves to:

- `resolved_profile=plan_query`
- `resolved_phases=plan_query`
- `resolved_phase=plan_query`
- `resolved_reason=phase_timeout`
- `source_kind=plan_session`
- `source_name=sgx-plan-session-plan-op-plan-query-live`
- `source_timestamp=20260512-154733`
- `source_row_requested=post_descriptor:explicit[plan_op,plan_query]`
- `source_row_resolved=post_descriptor:explicit[plan_op,plan_query]`
- `source_row_child=post_descriptor:explicit[plan_op,plan_query]`
- `source_row_evidence_last_good=plan_op`
- `source_row_evidence_first_bad=plan_query`
- `source_row_evidence_reason=phase_timeout`

## Operator Changes

Updated:

- [scripts/run-vaio-sgx-plan-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-session.sh)
- [scripts/run-vaio-sgx-plan-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-recovery-session.sh)
- [scripts/report-vaio-plan-status.sh](/root/Poulsbo/scripts/report-vaio-plan-status.sh)
- [scripts/summarize-vaio-plan-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-plan-sessions.sh)
- [scripts/resolve-sgx-submit-phase-profile.sh](/root/Poulsbo/scripts/resolve-sgx-submit-phase-profile.sh)

The resulting behavior is now:

- `PLAN_PHASE_PROFILE=current` is valid for the plan-specific wrappers
- recovery-mode `auto` now resolves through the shared current-boundary helper
  instead of staying pinned to `post_descriptor`
- the status report uses the same helper for its recommendation lines
- the plan summary now carries per-row bisect evidence directly:
  - `evidence_last_started`
  - `evidence_last_good`
  - `evidence_first_bad`
  - `evidence_reason`

## Current Practical Meaning

The current saved live evidence still says:

- `plan_op` is good
- `plan_query` is the first bad narrow phase

So the operator can now use either of these equivalent commands:

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_query ./scripts/run-vaio-sgx-plan-session.sh
```

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=plan_query ./scripts/run-vaio-sgx-plan-session.sh
```

and recovery-mode `auto` now resolves to the same narrowed boundary through the
shared helper instead of reverting to the older broader `post_descriptor`
window.

## Local Verification

Local checks passed:

- `bash -n` on the touched scripts
- `./scripts/resolve-sgx-submit-phase-profile.sh plan_query`
- `./scripts/resolve-vaio-plan-current-phase.sh`
- `OUTPUT_FORMAT=tsv MAX_SESSIONS=12 ./scripts/summarize-vaio-plan-sessions.sh`
- `./scripts/report-vaio-plan-status.sh`
- blocked recovery-mode top-level operator check:
  - [logs/vaio-plan-session/20260512-162324-sgx-plan-session-current-auto-check](/root/Poulsbo/logs/vaio-plan-session/20260512-162324-sgx-plan-session-current-auto-check)
  - result recorded:
    - `selected_phase_profile=plan_query`
    - `selected_phase_selection=profile`
    - `selected_phases=plan_query`
    - `selected_child_phase_profile=plan_query`
    - `selected_child_phase_selection=request_only`
    - `selected_child_phases=plan_query`

No new kernel-tree files changed in this batch.
