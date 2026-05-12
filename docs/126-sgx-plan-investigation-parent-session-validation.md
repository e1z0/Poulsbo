# Submit `plan` Investigation Parent Session Validation

## Summary

The new guarded-deploy parent wrapper is now locally validated against the same
blocked VAIO state as the direct investigation and recovery-session wrappers.
That gives the repo one saved canonical parent bundle for the next risky full
cycle, even before the target is recovered.

## Added Script

- [scripts/run-vaio-sgx-plan-guarded-deploy-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-guarded-deploy-session.sh)

## Local Validation

Local checks passed:

- `bash -n scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`
- `./scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`

The canonical blocked parent bundle is:

- [logs/vaio-plan/20260512-000636-sgx-plan-guarded-deploy](/root/Poulsbo/logs/vaio-plan/20260512-000636-sgx-plan-guarded-deploy)

It currently summarizes as:

- `session_outcome=deploy_failed_or_blocked`
- `deploy_outcome=blocked_preflight`
- `recovery_outcome=skipped`
- `last_overall=ssh_port_down`
- `final_driver_srcversion=3D21987EBD495B4E78FBBA7`

## Session Summary

The session-summary helper now recognizes this family as:

- `kind=plan_deploy`

so the next live full-cycle run will be visible alongside recovery, deploy, and
investigation sessions without extra manual sorting.
