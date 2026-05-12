# SGX `plan` Structured Phase-Source Columns

## Summary

The `plan` operator stack already preserved source provenance in saved bundle
results:

- `selected_phase_source_reason`
- `selected_phase_source_kind`
- `selected_phase_source_name`
- `selected_phase_source_timestamp`
- `selected_phase_source_session_dir`

But the offline summary still collapsed that back down to one lossy display
string:

- `phase_source=phase_timeout@20260512-154733`

That was enough for quick scanning, but it threw away the source identity that
the resolver and report already knew explicitly.

## Changes

Updated:

- [scripts/summarize-vaio-plan-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-plan-sessions.sh)
- [scripts/report-vaio-plan-status.sh](/root/Poulsbo/scripts/report-vaio-plan-status.sh)

The plan summary now keeps the old compact `phase_source` field for quick
human scanning, but also exports the structured columns directly:

- `phase_source_reason`
- `phase_source_kind`
- `phase_source_name`
- `phase_source_timestamp`
- `phase_source_session_dir`

The current-state report now emits those same structured source fields for each
saved row it surfaces, for example:

- `latest_remote_usable_phase_evidence_phase_source_reason`
- `latest_remote_usable_phase_evidence_phase_source_kind`
- `latest_remote_usable_phase_evidence_phase_source_name`
- `latest_remote_usable_phase_evidence_phase_source_timestamp`
- `latest_remote_usable_phase_evidence_phase_source_session_dir`

The current-state report now also dereferences the resolved source bundle back
into its own saved summary row:

- `current_recovery_source_row_present`
- `current_recovery_source_row_requested`
- `current_recovery_source_row_resolved`
- `current_recovery_source_row_child`
- `current_recovery_source_row_phase_source_reason`
- `current_recovery_source_row_phase_source_kind`
- `current_recovery_source_row_phase_source_name`
- `current_recovery_source_row_evidence_last_good`
- `current_recovery_source_row_evidence_first_bad`
- `current_recovery_source_row_evidence_reason`

The summary now also backfills those fields for older rows when possible:

- if a saved result already carried explicit source fields, it uses them
- if the row itself is the live phase-evidence source bundle, it fills the
  structured source columns from the row’s own bisect evidence plus its own
  session identity
- if only a source session path is available, it infers `phase_source_kind`
  and `phase_source_name` from that path

## Current Reference Row

The canonical blocked top-level reference still reads:

- `requested=auto:profile[plan_query]`
- `resolved=plan_query:profile[plan_query]`
- `child=plan_query:request_only[plan_query]`
- `phase_source=phase_timeout@20260512-154733`

But it now also exposes the same source identity structurally:

- `phase_source_reason=phase_timeout`
- `phase_source_kind=plan_session`
- `phase_source_name=sgx-plan-session-plan-op-plan-query-live`
- `phase_source_timestamp=20260512-154733`
- `phase_source_session_dir=logs/vaio-plan-session/20260512-154733-sgx-plan-session-plan-op-plan-query-live`

And the report can now show that same source bundle’s own row directly, so one
status output includes both:

- the currently selected recovery boundary
- the exact saved summary row that established that boundary

## Validation

Validated locally with:

- `bash -n scripts/summarize-vaio-plan-sessions.sh`
- `bash -n scripts/report-vaio-plan-status.sh`
- `OUTPUT_FORMAT=tsv MAX_SESSIONS=2 ./scripts/summarize-vaio-plan-sessions.sh`
- `./scripts/report-vaio-plan-status.sh`

Current target state is still blocked:

- `ping_status=fail`
- `tcp_22_status=closed`
- `ssh_pubkey_status=no_route`
- `overall=ssh_port_down`

## Result

The offline summary, current-state report, and saved bundle results now expose
the same phase-source provenance model instead of mixing a structured resolver
with a lossy summary string.
