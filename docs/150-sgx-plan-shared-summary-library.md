# SGX `plan` Shared Summary Library

## Goal

The `plan` operator stack had grown three copies of the same offline parsing
logic:

- summary TSV header parsing
- latest-session selection
- bisect evidence extraction
- current-boundary resolution

That duplication had already caused one real regression. After the summary TSV
schema expanded, one consumer started reading `plan_op` as the boundary reason
because it still depended on fixed column positions. The header-driven fix in
[docs/149-sgx-plan-header-driven-summary-parser-hardening.md](/root/Poulsbo/docs/149-sgx-plan-header-driven-summary-parser-hardening.md)
made the individual scripts correct again, but the same logic was still spread
across the resolver, reporter, and offline summary.

## Changes

Added a shared shell library:

- [scripts/lib-vaio-plan-summary.sh](/root/Poulsbo/scripts/lib-vaio-plan-summary.sh)

It now centralizes:

- header-aware TSV column parsing
- newest-session selection by regex
- latest bisect summary/result discovery
- bisect `last_started` / `last_good` extraction
- named `full` / `post_descriptor` / `plan_query` phase-profile expansion
- current live recovery-boundary resolution from saved `plan_session` evidence

The following tools now source that one helper instead of carrying local copies
of the same parsing code:

- [scripts/resolve-vaio-plan-current-phase.sh](/root/Poulsbo/scripts/resolve-vaio-plan-current-phase.sh)
- [scripts/report-vaio-plan-status.sh](/root/Poulsbo/scripts/report-vaio-plan-status.sh)
- [scripts/summarize-vaio-plan-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-plan-sessions.sh)

## Current Boundary

The shared resolver still selects the same saved live boundary as before:

- `resolved_profile=plan_query`
- `resolved_phases=plan_query`
- `resolved_phase=plan_query`
- `resolved_reason=phase_timeout`
- `source_kind=plan_session`
- `source_name=sgx-plan-session-plan-op-plan-query-live`
- `source_timestamp=20260512-154733`
- `source_session_dir=logs/vaio-plan-session/20260512-154733-sgx-plan-session-plan-op-plan-query-live`

That same source now flows consistently through:

- the top-level operator session
- the current-state report
- the offline diagnosis summary

The offline summary now exposes that source both ways:

- compact legacy view:
  - `phase_source=phase_timeout@20260512-154733`
- structured view:
  - `phase_source_reason=phase_timeout`
  - `phase_source_kind=plan_session`
  - `phase_source_name=sgx-plan-session-plan-op-plan-query-live`
  - `phase_source_timestamp=20260512-154733`

## Validation

Validated locally with:

- `bash -n scripts/lib-vaio-plan-summary.sh`
- `bash -n scripts/resolve-vaio-plan-current-phase.sh`
- `bash -n scripts/report-vaio-plan-status.sh`
- `bash -n scripts/summarize-vaio-plan-sessions.sh`
- `./scripts/resolve-vaio-plan-current-phase.sh`
- `./scripts/report-vaio-plan-status.sh`
- `OUTPUT_FORMAT=tsv MAX_SESSIONS=2 ./scripts/summarize-vaio-plan-sessions.sh`

Current target state is still blocked:

- `ping_status=fail`
- `tcp_22_status=closed`
- `ssh_pubkey_status=no_route`
- `overall=ssh_port_down`

## Result

There is now one source of truth for offline `plan` boundary parsing. Future
summary-schema changes or bisect-evidence tweaks only need to be implemented in
one place instead of being copied across multiple wrappers.
