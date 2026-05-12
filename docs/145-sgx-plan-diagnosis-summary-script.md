# SGX `plan` Diagnosis Summary Script

## Summary

The generic session summary is good for broad repo-wide health, but the current
`plan` work needed a narrower offline view. The relevant evidence is scattered
across several different result formats:

- top-level `plan` sessions
- guarded deploy parents
- recovery wrappers
- direct submit-op phase investigations

This batch adds one plan-specific summarizer that flattens those bundles into a
single diagnosis table.

## Added Script

- [scripts/summarize-vaio-plan-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-plan-sessions.sh)

## Output

The script prints one comparable row per saved `plan`-related session with:

- `kind`
- `timestamp`
- `name`
- `outcome`
- `last_state`
- `requested`
- `resolved`
- `child`
- `phase_source`
- `phase_source_reason`
- `phase_source_kind`
- `phase_source_name`
- `phase_source_timestamp`
- `phase_source_session_dir`
- `failed_phase`
- `failed_step`
- `module_srcversion`
- `session_dir`
- `evidence_last_started`
- `evidence_last_good`
- `evidence_first_bad`
- `evidence_reason`

`requested` is the operator-facing phase window, and `child` is the deeper
recovery-side or validation-side view when one exists.

Examples:

- `post_descriptor:explicit[plan_op,plan_query]`
- `post_descriptor:request_only[plan_op,plan_query]`
- `post_descriptor:profile[descriptor_query,plan_op,plan_query,prepare_op,prepare_query]`

That lets one table show both:

- the newer blocked explicit-window session semantics
- the older real live failure at `sync -> submit_op_kick_focus`
- the newer bisect boundary directly, for example:
  - `evidence_last_good=plan_op`
  - `evidence_first_bad=plan_query`
  - `evidence_reason=phase_timeout`
- and now the request-versus-resolution provenance for current-boundary reruns,
  for example:
  - `requested=auto:profile[plan_query]`
  - `resolved=plan_query:profile[plan_query]`
  - `phase_source=phase_timeout@20260512-154733`
  - `phase_source_kind=plan_session`
  - `phase_source_name=sgx-plan-session-plan-op-plan-query-live`

## Local Verification

Local checks passed:

- `bash -n scripts/summarize-vaio-plan-sessions.sh`
- `MAX_SESSIONS=10 ./scripts/summarize-vaio-plan-sessions.sh`
- `MAX_SESSIONS=30 ./scripts/summarize-vaio-plan-sessions.sh | rg '20260512-111909|20260512-151303'`

The canonical current view is produced by:

```bash
MAX_SESSIONS=10 ./scripts/summarize-vaio-plan-sessions.sh
```

and now shows, in one place:

- the latest blocked explicit-window parent:
  - `20260512-151303`
  - `requested=post_descriptor:explicit[plan_op,plan_query]`
  - `child=post_descriptor:request_only[plan_op,plan_query]`
- the older live confirmed-bad session:
  - `20260512-111909`
  - `failed_phase=sync`
  - `failed_step=submit_op_kick_focus`

- and the blocked current-boundary source identity:
  - `phase_source_reason=phase_timeout`
  - `phase_source_kind=plan_session`
  - `phase_source_name=sgx-plan-session-plan-op-plan-query-live`

No new hardware rerun was attempted in this batch.
