# SGX `plan` Phase-Source Helper And Evidence Selector Hardening

## Summary

The `plan` operator stack had two real problems:

1. the same boundary/source-row field set was being copied by hand through the
   top-level parent, the plan-specific recovery wrapper, the generic recovery
   wrapper, and the current-state report
2. the current-boundary resolver treated any newer remote-usable
   `plan_session` row as phase evidence, even if that row was only an
   in-progress parent with no real bisect result yet

The second problem surfaced immediately in a real live rerun. A new
`recovery`-mode parent at `20260512-180606` started while the VAIO was back at
`overall=remote_usable`, and the old selector tried to use that in-progress
row as the newest phase-evidence source. That self-poisoned the current
boundary back to `post_descriptor` because the new row had no actual
`first_bad` / `reason` evidence yet.

This batch fixes both issues together.

## Helper Consolidation

Added:

- [scripts/lib-vaio-plan-phase-source.sh](/root/Poulsbo/scripts/lib-vaio-plan-phase-source.sh)

The new helper centralizes the boundary/source-row field set and the common
operations on it:

- `vaio_kv_last_value`
- `vaio_plan_phase_source_init_vars`
- `vaio_plan_phase_source_assign_from_resolver_file`
- `vaio_plan_phase_source_assign_from_environment`
- `vaio_plan_phase_source_emit`
- `vaio_plan_phase_source_env_args`
- `vaio_plan_phase_source_row_present`

Updated to use it:

- [scripts/run-vaio-sgx-plan-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-session.sh)
- [scripts/run-vaio-sgx-plan-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-recovery-session.sh)
- [scripts/run-vaio-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-recovery-session.sh)
- [scripts/report-vaio-plan-status.sh](/root/Poulsbo/scripts/report-vaio-plan-status.sh)

That removes the repeated hand-maintained field lists for:

- `selected_phase_source_*`
- `selected_phase_source_row_*`
- `validation_phase_source_*`
- `validation_phase_source_row_*`
- `current_recovery_source_row_*`

## Strict Phase-Evidence Selection

Updated:

- [scripts/lib-vaio-plan-summary.sh](/root/Poulsbo/scripts/lib-vaio-plan-summary.sh)
- [scripts/report-vaio-plan-status.sh](/root/Poulsbo/scripts/report-vaio-plan-status.sh)

The shared summary library now exports:

- `vaio_plan_summary_pick_latest_phase_evidence_row()`

It only accepts remote-usable `plan_session` rows that carry actual evidence:

- `evidence_first_bad`
- or `evidence_reason`
- or `phase_source_reason`

That means a newer in-progress top-level parent no longer overrides the stable
saved live evidence row just because it already reached `remote_usable`.

## Live Rerun That Exposed The Bug

The real trigger was:

- [logs/vaio-plan-session/20260512-180606-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-180606-sgx-plan-session)

Before the selector hardening, its in-progress top-level row appeared first in
the offline summary:

- `timestamp=20260512-180606`
- `outcome=interrupted_or_incomplete`
- `last_state=remote_usable`
- `requested=current`
- `evidence_last_started=plan_query`
- `evidence_first_bad=-`
- `evidence_reason=-`

That row had no real failure evidence yet, so it should never have become the
current-boundary source.

While that `180606` session was still in flight, the fixed resolver and report
correctly stayed pinned to the stable earlier source:

- `resolved_profile=plan_query`
- `resolved_reason=phase_timeout`
- `source_name=sgx-plan-session-plan-op-plan-query-live`
- `source_timestamp=20260512-154733`

Once `180606` completed with real evidence, it legitimately became the newest
phase-evidence source. The current boundary is now:

- `resolved_profile=post_descriptor`
- `resolved_phase=descriptor_query`
- `resolved_reason=phase_failed`
- `source_name=sgx-plan-session`
- `source_timestamp=20260512-180606`

## Additional Live Progress

The same `180606` rerun also produced one more useful live datapoint before
the target dropped again.

The bisect child is:

- [logs/vaio-plan-session/20260512-180606-sgx-plan-session/children/20260512-180625-recovery/children/20260512-180625-bisect](/root/Poulsbo/logs/vaio-plan-session/20260512-180606-sgx-plan-session/children/20260512-180625-recovery/children/20260512-180625-bisect)

It first started:

- `probe=1 phase=plan_query`

And the live `plan_query` phase log reached:

- `STAGE_OK=sync`
- `STAGE_OK=owner`
- `STAGE_OK=report`
- `STAGE_OK=manifest`
- `STAGE_OK=descriptor_op`
- `STAGE_OK=descriptor_query`
- `STAGE_OK=plan_op`

The bisect result later completed with:

- `probe_order=plan_query,descriptor_query`
- `first_bad_phase=descriptor_query`
- `first_bad_reason=phase_failed`

So on the current candidate the saved diagnosis has moved:

- `plan_query` was not the first bad prefix after all
- the current earliest bad prefix is now `descriptor_query`

The confirm pass was inconclusive because the VAIO dropped back to
`overall=ssh_port_down` during the later narrowed rerun.

## Validation

Validated locally with:

- `bash -n scripts/lib-vaio-plan-phase-source.sh`
- `bash -n scripts/lib-vaio-plan-summary.sh`
- `bash -n scripts/run-vaio-sgx-plan-recovery-session.sh`
- `bash -n scripts/run-vaio-recovery-session.sh`
- `bash -n scripts/run-vaio-sgx-plan-session.sh`
- `bash -n scripts/report-vaio-plan-status.sh`
- `./scripts/resolve-vaio-plan-current-phase.sh | rg '^(resolved_|source_|source_row_)'`
- `./scripts/report-vaio-plan-status.sh | rg '^(latest_remote_usable_phase_evidence_|current_recovery_|current_recovery_source_row_|recommended_next_)'`
- `OUTPUT_FORMAT=tsv MAX_SESSIONS=4 ./scripts/summarize-vaio-plan-sessions.sh`

Key fixed outcome:

- while `20260512-180606` was still only `interrupted_or_incomplete`, the
  resolver/report correctly ignored it as current evidence
- after that same session completed with real bisect evidence, the
  resolver/report correctly adopted it as the new source:
  - `sgx-plan-session@20260512-180606`
  - `descriptor_query`
  - `phase_failed`

Current target state after the live rerun is again blocked:

- `ping_status=fail`
- `tcp_22_status=closed`
- `ssh_pubkey_status=no_route`
- `overall=ssh_port_down`

## Result

The `plan` current-boundary path is now harder to break in two ways:

- the boundary/source-row plumbing is centralized in one helper instead of
  being hand-copied through multiple wrappers
- the shared current-boundary selector no longer self-poisons from newer
  in-progress `remote_usable` parents without actual phase evidence

The active narrowed live question is now:

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASES=descriptor_query ./scripts/run-vaio-sgx-plan-session.sh
```

Equivalent profile form:

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=post_descriptor ./scripts/run-vaio-sgx-plan-session.sh
```
