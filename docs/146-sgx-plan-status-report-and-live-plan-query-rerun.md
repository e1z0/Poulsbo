# SGX `plan` Status Report And Live `plan_query` Rerun

## Summary

The new offline plan-session summary was useful, but the operator still had to
mentally pick the current reference bundles before each next rerun. This batch
adds one current-status report on top of that summary, then uses it to drive
the next narrowed live recovery rerun.

## Added Script

- [scripts/report-vaio-plan-status.sh](/root/Poulsbo/scripts/report-vaio-plan-status.sh)

It combines:

- current VAIO health from
  [scripts/check-vaio-target-health.sh](/root/Poulsbo/scripts/check-vaio-target-health.sh)
- saved `plan` bundle history from
  [scripts/summarize-vaio-plan-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-plan-sessions.sh)

into one current-state report.

## Current-State Output

The report now prints:

- current target reachability
- latest explicit-window blocked session
- latest confirmed live failure
- latest remote-usable phase-evidence session
- latest guarded-deploy parent
- current recovery source row dereferenced from the resolved source bundle
- recommended offline summary command
- recommended next live command

During this batch, while the VAIO was briefly back at `overall=remote_usable`,
the report showed:

- `current_overall=remote_usable`
- latest explicit-window reference:
  - `20260512-151303`
  - `requested=post_descriptor:explicit[plan_op,plan_query]`
  - `child=post_descriptor:request_only[plan_op,plan_query]`
- latest confirmed live failure:
  - `20260512-111909`
  - `failed_phase=sync`
  - `failed_step=submit_op_kick_focus`
- latest remote-usable phase-evidence reference:
  - `20260512-154733`
  - `requested=post_descriptor:explicit[plan_op,plan_query]`
  - bisect child:
    - `last_good_phase=plan_op`
    - `first_bad_phase=plan_query`
    - `first_bad_reason=phase_timeout`
- latest guarded-deploy candidate:
  - `module_srcversion=0EEEE302E3FDA9F59B7CFB9`

## Live Follow-up

Using that report, I immediately ran the next narrowed live recovery session:

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_op,plan_query ./scripts/run-vaio-sgx-plan-session.sh
```

The live parent bundle is:

- [logs/vaio-plan-session/20260512-154733-sgx-plan-session-plan-op-plan-query-live](/root/Poulsbo/logs/vaio-plan-session/20260512-154733-sgx-plan-session-plan-op-plan-query-live)

The important live result is that `plan_op` is now cleared on the loaded
candidate `srcversion 0EEEE302E3FDA9F59B7CFB9`.

The saved bisect log shows:

- `probe_1_phase=plan_op`
- `probe_1_status=good`
- `probe_1_reason=ok`
- `probe_1_health=remote_usable`

and the corresponding stage log reached:

- `STAGE_OK=sync`
- `STAGE_OK=owner`
- `STAGE_OK=report`
- `STAGE_OK=manifest`
- `STAGE_OK=descriptor_op`
- `STAGE_OK=descriptor_query`
- `STAGE_OK=plan_op`
- `STOP_AFTER_STAGE_REACHED=plan_op`

The next probe, `plan_query`, started and again reached:

- `STAGE_OK=sync`
- `STAGE_OK=owner`
- `STAGE_OK=report`
- `STAGE_OK=manifest`
- `STAGE_OK=descriptor_op`
- `STAGE_OK=descriptor_query`

before the VAIO dropped off the network again.

That moves the current live boundary forward one full step:

- `plan_op` is no longer the blocker
- the current live question is now inside `plan_query`

## Current State

By the end of this batch the target had dropped again:

- `ping_status=fail`
- `tcp_22_status=closed`
- `ssh_pubkey_status=no_route`
- `overall=ssh_port_down`

So the next preferred live command, after recovery, is narrower still:

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_query ./scripts/run-vaio-sgx-plan-session.sh
```

The reporter now derives that directly from the newest remote-usable bisect
evidence instead of falling back to the older `interrupted` parent heuristic,
so it no longer regresses to the older `descriptor_query` recommendation when a
newer `validation_failed` parent bundle already contains a better child bisect
answer.

The current report now also emits:

- `recommended_next_phase=plan_query`
- `recommended_next_phase_profile=plan_query`
- `recommended_next_profile_command=PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=plan_query ./scripts/run-vaio-sgx-plan-session.sh`

And the newer follow-up now also lets the report dereference that source bundle
back into its own saved summary row directly, through:

- `current_recovery_source_row_present`
- `current_recovery_source_row_requested`
- `current_recovery_source_row_resolved`
- `current_recovery_source_row_child`
- `current_recovery_source_row_failed_phase`
- `current_recovery_source_row_failed_step`
- `current_recovery_source_row_evidence_last_good`
- `current_recovery_source_row_evidence_first_bad`
- `current_recovery_source_row_evidence_reason`

The same current-boundary decision is now shared with the operator entrypoints
through [scripts/resolve-vaio-plan-current-phase.sh](/root/Poulsbo/scripts/resolve-vaio-plan-current-phase.sh), so recovery-mode `auto` and the
status report no longer have separate ideas of what “current boundary” means.

## Local Verification

Local checks passed:

- `bash -n scripts/report-vaio-plan-status.sh`
- `bash -n scripts/summarize-vaio-plan-sessions.sh`
- `OUTPUT_FORMAT=tsv MAX_SESSIONS=5 ./scripts/summarize-vaio-plan-sessions.sh`
- `./scripts/report-vaio-plan-status.sh`

No new kernel-tree files changed in this batch.
