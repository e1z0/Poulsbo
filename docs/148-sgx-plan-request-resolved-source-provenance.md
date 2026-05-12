# SGX `plan` Request/Resolved/Source Provenance

## Summary

The operator stack already knew the current live recovery boundary had narrowed
to `plan_query`, but saved sessions still blurred together three different
things:

- what the operator asked for
- what the wrapper actually resolved to
- which saved live bundle supplied that resolution

This batch separates those explicitly.

## Script Changes

Updated:

- [scripts/run-vaio-sgx-plan-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-session.sh)
- [scripts/run-vaio-sgx-plan-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-recovery-session.sh)
- [scripts/run-vaio-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-recovery-session.sh)
- [scripts/summarize-vaio-plan-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-plan-sessions.sh)
- [scripts/report-vaio-plan-status.sh](/root/Poulsbo/scripts/report-vaio-plan-status.sh)

The shared boundary resolver from
[docs/147-sgx-plan-current-boundary-resolver-and-auto-recovery-default.md](/root/Poulsbo/docs/147-sgx-plan-current-boundary-resolver-and-auto-recovery-default.md)
is still the source of truth, but now its provenance is preserved in saved
results.

## New Saved Fields

Top-level `plan` session results now keep:

- `selected_phase_profile_request`
- `selected_phase_selection_request`
- `selected_phases_request`
- `selected_phase_profile_base`
- `selected_phase_profile`
- `selected_phase_selection`
- `selected_phases`
- `selected_phase_source_reason`
- `selected_phase_source_timestamp`
- `selected_phase_source_session_dir`

The generic recovery wrapper now also preserves corresponding validation-side
fields:

- `validation_phase_profile_request_original`
- `validation_phase_profile_base`
- `validation_phase_source_reason`
- `validation_phase_source_kind`
- `validation_phase_source_name`
- `validation_phase_source_timestamp`
- `validation_phase_source_session_dir`

## Summary Output

[scripts/summarize-vaio-plan-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-plan-sessions.sh)
now distinguishes:

- `requested`
- `resolved`
- `child`
- `phase_source`
- `phase_source_reason`
- `phase_source_kind`
- `phase_source_name`
- `phase_source_timestamp`
- `phase_source_session_dir`

instead of only one merged parent view.

The current canonical blocked auto-resolution row is now:

- session:
  - [logs/vaio-plan-session/20260512-164446-sgx-plan-session-provenance-v2-check](/root/Poulsbo/logs/vaio-plan-session/20260512-164446-sgx-plan-session-provenance-v2-check)
- summary fields:
  - `requested=auto:profile[plan_query]`
  - `resolved=plan_query:profile[plan_query]`
  - `child=plan_query:request_only[plan_query]`
  - `phase_source=phase_timeout@20260512-154733`
  - `phase_source_kind=plan_session`
  - `phase_source_name=sgx-plan-session-plan-op-plan-query-live`

The same saved parent result now also records:

- `selected_phase_source_kind=plan_session`
- `selected_phase_source_name=sgx-plan-session-plan-op-plan-query-live`

That row now cleanly answers:

- the operator requested recovery-mode `auto`
- the wrapper resolved that to `plan_query`
- it did so because the saved live `20260512-154733` bisect bundle says
  `plan_op` is good and `plan_query` is first bad with `phase_timeout`

## Status Report

[scripts/report-vaio-plan-status.sh](/root/Poulsbo/scripts/report-vaio-plan-status.sh)
now agrees with the same provenance model. Its current output includes:

- `current_recovery_profile=plan_query`
- `current_recovery_phase=plan_query`
- `current_recovery_reason=phase_timeout`
- `current_recovery_source_kind=plan_session`
- `current_recovery_source_name=sgx-plan-session-plan-op-plan-query-live`
- `current_recovery_source_timestamp=20260512-154733`
- `current_recovery_source_session_dir=logs/vaio-plan-session/20260512-154733-sgx-plan-session-plan-op-plan-query-live`

## Local Verification

Local checks passed:

- `bash -n` on the touched scripts
- `OUTPUT_FORMAT=tsv MAX_SESSIONS=4 ./scripts/summarize-vaio-plan-sessions.sh`
- `./scripts/report-vaio-plan-status.sh`
- blocked top-level recovery-mode validation:
  - [logs/vaio-plan-session/20260512-163520-sgx-plan-session-provenance-check-rerun](/root/Poulsbo/logs/vaio-plan-session/20260512-163520-sgx-plan-session-provenance-check-rerun)

No new kernel-tree files changed in this batch.
