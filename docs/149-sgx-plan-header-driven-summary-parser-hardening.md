# SGX `plan` Header-Driven Summary Parser Hardening

## Summary

The `plan` operator stack had already learned to preserve request-versus-
resolution provenance, but the shared current-boundary resolver and the status
report still depended on fixed TSV column positions from
[scripts/summarize-vaio-plan-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-plan-sessions.sh).

That was the same class of bug that previously misread the current boundary when
the summary schema changed. This batch hardens the consumers instead of adding
more positional assumptions.

## Code Changes

Updated:

- [scripts/resolve-vaio-plan-current-phase.sh](/root/Poulsbo/scripts/resolve-vaio-plan-current-phase.sh)
- [scripts/report-vaio-plan-status.sh](/root/Poulsbo/scripts/report-vaio-plan-status.sh)
- [scripts/run-vaio-sgx-plan-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-session.sh)
- [scripts/run-vaio-sgx-plan-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-recovery-session.sh)
- [scripts/run-vaio-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-recovery-session.sh)

The key changes are:

- summary TSV consumers now build a header-name map before reading fields
- the current-boundary resolver now reads:
  - `kind`
  - `name`
  - `timestamp`
  - `session_dir`
  - `evidence_last_started`
  - `evidence_last_good`
  - `evidence_first_bad`
  - `evidence_reason`
  by column name instead of by position
- the resolver now emits:
  - `source_kind`
  - `source_name`
  in addition to the earlier `source_timestamp` and `source_session_dir`
- top-level `plan` parent bundles now preserve:
  - `selected_phase_source_kind`
  - `selected_phase_source_name`
- generic recovery results now preserve:
  - `validation_phase_source_kind`
  - `validation_phase_source_name`

## Current Verified Result

The shared resolver now emits:

- `resolved_profile=plan_query`
- `resolved_phases=plan_query`
- `resolved_phase=plan_query`
- `resolved_reason=phase_timeout`
- `source_kind=plan_session`
- `source_name=sgx-plan-session-plan-op-plan-query-live`
- `source_timestamp=20260512-154733`

The current blocked top-level provenance-check bundle is:

- [logs/vaio-plan-session/20260512-164446-sgx-plan-session-provenance-v2-check](/root/Poulsbo/logs/vaio-plan-session/20260512-164446-sgx-plan-session-provenance-v2-check)

Its saved parent result now includes:

- `selected_phase_profile_request=auto`
- `selected_phase_profile_base=current`
- `selected_phase_profile=plan_query`
- `selected_phase_source_reason=phase_timeout`
- `selected_phase_source_kind=plan_session`
- `selected_phase_source_name=sgx-plan-session-plan-op-plan-query-live`
- `selected_phase_source_timestamp=20260512-154733`

The status report now agrees exactly:

- `current_recovery_profile=plan_query`
- `current_recovery_reason=phase_timeout`
- `current_recovery_source_kind=plan_session`
- `current_recovery_source_name=sgx-plan-session-plan-op-plan-query-live`
- `current_recovery_source_timestamp=20260512-154733`

## Local Verification

Local checks passed:

- `bash -n` on the touched scripts
- `./scripts/resolve-vaio-plan-current-phase.sh`
- `./scripts/report-vaio-plan-status.sh`
- blocked top-level recovery-mode validation:
  - [logs/vaio-plan-session/20260512-164446-sgx-plan-session-provenance-v2-check](/root/Poulsbo/logs/vaio-plan-session/20260512-164446-sgx-plan-session-provenance-v2-check)

No new kernel-tree files changed in this batch.
