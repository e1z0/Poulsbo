# Submit `plan` Guarded-Deploy Session Wrapper

## Summary

The repo now has a guarded deploy-session wrapper and a post-boot `plan`
investigation wrapper, but the most useful command for the next live attempt is
the one that composes them. This batch adds that parent session driver.

## Added Script

- [scripts/run-vaio-sgx-plan-guarded-deploy-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-guarded-deploy-session.sh)

It writes one parent bundle under `logs/vaio-plan/` and runs:

1. one guarded deploy session
2. then, only on successful deploy completion, one `plan` recovery-session
   investigation

## Behavior

The parent result file records:

- `deploy_outcome`
- `deploy_session_dir`
- `recovery_outcome`
- `recovery_session_dir`
- `recovery_requested_phase_profile`
- `recovery_requested_phase_selection`
- `recovery_requested_phases`
- `recovery_phase_profile`
- `recovery_phase_selection`
- `recovery_phases`
- `last_overall`
- `final_driver_srcversion`

So one command can now capture the entire next risky `plan` iteration from
deploy through post-boot SGX investigation.

## Intended Use

Once the target is usable again, the preferred full-cycle command is:

```bash
./scripts/run-vaio-sgx-plan-guarded-deploy-session.sh
```

For day-to-day use there is now also an automatic top-level selector:

- [docs/127-sgx-plan-session-entrypoint.md](/root/Poulsbo/docs/127-sgx-plan-session-entrypoint.md)

If the post-boot investigation should start with a narrower stage window:

```bash
PLAN_PHASE_PROFILE=post_descriptor ./scripts/run-vaio-sgx-plan-guarded-deploy-session.sh
```

If you want one explicit post-boot phase list instead of a named profile:

```bash
PLAN_PHASES=plan_op,plan_query ./scripts/run-vaio-sgx-plan-guarded-deploy-session.sh
```

The latest semantics follow-up also keeps the requested recovery phase choice
separate from the child recovery wrapper's internal state. See:

- [docs/144-sgx-plan-phase-selection-semantics.md](/root/Poulsbo/docs/144-sgx-plan-phase-selection-semantics.md)

## Local Verification

Local checks passed:

- `bash -n scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`

No hardware validation was possible in this batch because the VAIO was still at
`overall=ssh_port_down`. A blocked-target parent bundle is still useful because
it proves the session structure and summary integration.

The canonical blocked parent bundle is:

- [logs/vaio-plan/20260512-000636-sgx-plan-guarded-deploy](/root/Poulsbo/logs/vaio-plan/20260512-000636-sgx-plan-guarded-deploy)

with:

- `session_outcome=deploy_failed_or_blocked`
- `deploy_outcome=blocked_preflight`
- `recovery_outcome=skipped`
- `last_overall=ssh_port_down`
