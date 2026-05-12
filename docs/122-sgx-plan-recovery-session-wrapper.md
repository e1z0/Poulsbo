# Submit `plan` Recovery Session Wrapper

## Summary

The first end-to-end hardware attempt for the new memory-backed submit `plan`
block took the VAIO down before it could be narrowed to one stage. The repo now
has a stage-matrix harness, a lighter prefix-bisect harness, and a composed
investigation harness, and the next useful step is to package the whole
post-recovery flow into one saved session instead of several manual commands.

## Added Script

- [scripts/run-vaio-sgx-plan-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-recovery-session.sh)

It is a thin typed wrapper around the generic recovery-session driver:

- [scripts/run-vaio-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-recovery-session.sh)

with these defaults:

- `SESSION_KIND=sgx_submit_plan_recovery`
- `SESSION_NAME=sgx-plan-recovery`
- `VALIDATION_LABEL=submit_op_phase_investigation`
- `VALIDATION_LOG_BASENAME=submit-op-phase-investigation.log`
- `VALIDATION_SCRIPT=./scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
- `PLAN_PHASE_PROFILE=post_descriptor`

## Behavior

One command now captures the intended first post-recovery workflow:

1. wait for `overall=remote_usable`
2. capture remote driver and rollback state
3. run the submit-op investigation harness
4. write one structured session bundle under `logs/vaio-recovery/`

That means the next real recovery window will produce:

- `health-timeline.log`
- `remote-state.log`
- `submit-op-phase-investigation.log`
- `session.env`
- `session-result.env`

The direct bisect fallback is still available by overriding:

```bash
VALIDATION_SCRIPT=./scripts/run-vaio-sgx-submit-op-phase-bisect.sh \
VALIDATION_LABEL=submit_op_phase_bisect \
VALIDATION_LOG_BASENAME=submit-op-phase-bisect.log \
./scripts/run-vaio-sgx-plan-recovery-session.sh
```

The older exhaustive fallback is also still available by overriding:

```bash
VALIDATION_SCRIPT=./scripts/run-vaio-sgx-submit-op-phase-matrix.sh \
VALIDATION_LABEL=submit_op_phase_matrix \
VALIDATION_LOG_BASENAME=submit-op-phase-matrix.log \
./scripts/run-vaio-sgx-plan-recovery-session.sh
```

For the full next risky iteration, there is now also a parent wrapper that
starts at guarded deploy:

- [docs/125-sgx-plan-guarded-deploy-session-wrapper.md](/root/Poulsbo/docs/125-sgx-plan-guarded-deploy-session-wrapper.md)

For day-to-day use there is now also an automatic top-level selector that
chooses recovery versus full-cycle deploy:

- [docs/127-sgx-plan-session-entrypoint.md](/root/Poulsbo/docs/127-sgx-plan-session-entrypoint.md)

## Intended Use

After console recovery, the first recommended command is:

```bash
./scripts/run-vaio-sgx-plan-recovery-session.sh
```

If the full phase list is too broad, narrow it immediately:

```bash
PLAN_PHASE_PROFILE=post_descriptor ./scripts/run-vaio-sgx-plan-recovery-session.sh
```

If you need an explicit one-off phase list instead of the named profile:

```bash
PLAN_PHASES=descriptor_query,plan_op,plan_query,prepare_op,prepare_query ./scripts/run-vaio-sgx-plan-recovery-session.sh
```

The latest semantics follow-up also preserves the request-side phase selection
separately from the child recovery wrapper's internal blocked-target
`request_only` state. See:

- [docs/144-sgx-plan-phase-selection-semantics.md](/root/Poulsbo/docs/144-sgx-plan-phase-selection-semantics.md)

## Local Verification

Local checks passed:

- `bash -n scripts/run-vaio-recovery-session.sh`
- `bash -n scripts/run-vaio-sgx-plan-recovery-session.sh`
- `MAX_POLLS=1 POLL_INTERVAL_SECS=1 ./scripts/run-vaio-sgx-plan-recovery-session.sh`

That bounded run produced a real blocked session bundle at:

- [logs/vaio-recovery/20260512-000636-sgx-plan-recovery](/root/Poulsbo/logs/vaio-recovery/20260512-000636-sgx-plan-recovery)

with:

- `session_outcome=blocked_before_remote_usable`
- `last_state=ssh_port_down`

No hardware validation was possible in this batch because the VAIO was still at
`overall=ssh_port_down`.
