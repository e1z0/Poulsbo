# SGX `plan` Persisted Source-Row Propagation

## Summary

The `plan` operator stack already had enough information to resolve the
current narrowed recovery boundary from saved live evidence:

- `resolved_profile=plan_query`
- `resolved_reason=phase_timeout`
- `source_name=sgx-plan-session-plan-op-plan-query-live`
- `source_timestamp=20260512-154733`

The report layer could also dereference that source bundle back into its own
saved summary row. But that row identity still stopped at the report. Parent
session results did not persist the source row itself, which meant blocked
recovery-mode runs only saved the resolved phase boundary and not the exact
row that established it.

This batch closes that gap.

## Changes

Updated:

- [scripts/lib-vaio-plan-summary.sh](/root/Poulsbo/scripts/lib-vaio-plan-summary.sh)
- [scripts/resolve-vaio-plan-current-phase.sh](/root/Poulsbo/scripts/resolve-vaio-plan-current-phase.sh)
- [scripts/report-vaio-plan-status.sh](/root/Poulsbo/scripts/report-vaio-plan-status.sh)
- [scripts/run-vaio-sgx-plan-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-session.sh)
- [scripts/run-vaio-sgx-plan-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-recovery-session.sh)
- [scripts/run-vaio-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-recovery-session.sh)

The shared current-boundary resolver now emits structured source-row fields in
addition to the already-existing boundary and source identity:

- `source_row_requested`
- `source_row_resolved`
- `source_row_child`
- `source_row_outcome`
- `source_row_last_state`
- `source_row_phase_source`
- `source_row_failed_phase`
- `source_row_failed_step`
- `source_row_module_srcversion`
- `source_row_evidence_last_started`
- `source_row_evidence_last_good`
- `source_row_evidence_first_bad`
- `source_row_evidence_reason`

The plan-specific parent wrappers now persist those values directly into saved
session results:

- top-level parent:
  - `selected_phase_source_row_*`
- recovery wrapper:
  - `validation_phase_source_row_*`

The current-state report continues to expose the same row as:

- `current_recovery_source_row_*`

So the same source row is now available from all three views:

- the shared resolver
- saved parent session results
- the current-state report

## Current Canonical Source Row

The current narrowed recovery boundary remains the same:

- `plan_query`

It still comes from the same saved live evidence bundle:

- `source_kind=plan_session`
- `source_name=sgx-plan-session-plan-op-plan-query-live`
- `source_timestamp=20260512-154733`
- `source_session_dir=logs/vaio-plan-session/20260512-154733-sgx-plan-session-plan-op-plan-query-live`

The resolved source row is:

- `source_row_requested=post_descriptor:explicit[plan_op,plan_query]`
- `source_row_resolved=post_descriptor:explicit[plan_op,plan_query]`
- `source_row_child=post_descriptor:explicit[plan_op,plan_query]`
- `source_row_outcome=validation_failed`
- `source_row_last_state=remote_usable`
- `source_row_phase_source=phase_timeout@20260512-154733`
- `source_row_module_srcversion=6E95EDFFFB5384C8E58F25B`
- `source_row_evidence_last_started=plan_query`
- `source_row_evidence_last_good=plan_op`
- `source_row_evidence_first_bad=plan_query`
- `source_row_evidence_reason=phase_timeout`

## Saved Parent Result Proof

A fresh blocked top-level parent now saves that source row directly:

- [logs/vaio-plan-session/20260512-173551-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-173551-sgx-plan-session)

Key persisted fields in its `session-result.env`:

- `session_outcome=blocked_before_remote_usable`
- `selected_mode=recovery`
- `selected_phase_profile=plan_query`
- `selected_phases=plan_query`
- `selected_phase_source_reason=phase_timeout`
- `selected_phase_source_kind=plan_session`
- `selected_phase_source_name=sgx-plan-session-plan-op-plan-query-live`
- `selected_phase_source_timestamp=20260512-154733`
- `selected_phase_source_row_requested=post_descriptor:explicit[plan_op,plan_query]`
- `selected_phase_source_row_resolved=post_descriptor:explicit[plan_op,plan_query]`
- `selected_phase_source_row_child=post_descriptor:explicit[plan_op,plan_query]`
- `selected_phase_source_row_phase_source=phase_timeout@20260512-154733`
- `selected_phase_source_row_module_srcversion=6E95EDFFFB5384C8E58F25B`
- `selected_phase_source_row_evidence_last_good=plan_op`
- `selected_phase_source_row_evidence_first_bad=plan_query`
- `selected_phase_source_row_evidence_reason=phase_timeout`
- `last_overall=ssh_port_down`

## Validation

Validated locally with:

- `bash -n scripts/lib-vaio-plan-summary.sh`
- `bash -n scripts/resolve-vaio-plan-current-phase.sh`
- `bash -n scripts/report-vaio-plan-status.sh`
- `bash -n scripts/run-vaio-sgx-plan-session.sh`
- `bash -n scripts/run-vaio-sgx-plan-recovery-session.sh`
- `bash -n scripts/run-vaio-recovery-session.sh`
- `./scripts/resolve-vaio-plan-current-phase.sh | rg '^(resolved_|source_|source_row_)'`
- `./scripts/report-vaio-plan-status.sh | rg '^(current_recovery_|current_recovery_source_row_|recommended_next_)'`
- `MAX_POLLS=1 POLL_INTERVAL_SECS=1 PLAN_SESSION_MODE=recovery ./scripts/run-vaio-sgx-plan-session.sh`

Current target state is still blocked:

- `ping_status=fail`
- `tcp_22_status=closed`
- `ssh_pubkey_status=no_route`
- `overall=ssh_port_down`

## Result

The narrowed `plan` recovery boundary is still `plan_query`, but it is no
longer just a report-time interpretation. The exact saved summary row that
established that boundary is now persisted into parent session results, which
makes blocked recovery-mode bundles self-describing without needing a separate
summary lookup.
