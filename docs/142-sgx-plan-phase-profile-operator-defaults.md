# SGX `plan` Phase-Profile Operator Defaults

## Summary

The repo already had the finer post-`descriptor` subphases, but the operator
layer still depended on manually spelling `PHASES=` on every rerun. This batch
adds named phase profiles and wires them through the plan-specific wrappers.

## Shared Resolver

Added:

- [scripts/resolve-sgx-submit-phase-profile.sh](/root/Poulsbo/scripts/resolve-sgx-submit-phase-profile.sh)

Supported profiles:

- `current`
  - resolved dynamically from the newest remote-usable `plan` phase evidence
- `full`
  - `sync,owner,report,manifest,descriptor_op,descriptor_query,plan_op,plan_query,prepare_op,prepare_query,arm,emit,validate,launch,issue,running,complete`
- `post_descriptor`
  - `descriptor_query,plan_op,plan_query,prepare_op,prepare_query`
- `plan_query`
  - `plan_query`

## Generic Phase Tools

The phase tools now accept either:

- `PHASE_PROFILE=<name>`
- or explicit `PHASES=<csv>`

with `PHASES` taking precedence.

Updated:

- [scripts/run-vaio-sgx-submit-op-phase-matrix.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-matrix.sh)
- [scripts/run-vaio-sgx-submit-op-phase-bisect.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-bisect.sh)
- [scripts/run-vaio-sgx-submit-op-phase-investigation.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-investigation.sh)

Each now records:

- `phase_profile`
- `phase_selection`
- resolved `phases`

in its session manifest.

## Plan Wrapper Defaults

Plan-specific wrappers now choose better defaults automatically.

Recovery wrapper:

- [scripts/run-vaio-sgx-plan-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-recovery-session.sh)
- default: `PHASE_PROFILE=current`

Guarded-deploy parent:

- [scripts/run-vaio-sgx-plan-guarded-deploy-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-guarded-deploy-session.sh)
- default recovery-side profile: `PHASE_PROFILE=full`

Top-level operator entrypoint:

- [scripts/run-vaio-sgx-plan-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-session.sh)
- new selector: `PLAN_PHASE_PROFILE=auto|current|full|post_descriptor|plan_query`

`PLAN_PHASE_PROFILE=auto` now resolves to:

- `current` when `selected_mode=recovery`
- `full` when `selected_mode=deploy`

The top-level result now also records:

- `selected_phase_profile`

## Intended Use

Preferred recovery-mode rerun on the already-known boundary:

```bash
PLAN_SESSION_MODE=recovery ./scripts/run-vaio-sgx-plan-session.sh
```

That now automatically means:

- `selected_phase_profile=plan_query` on the current saved live boundary

Preferred full-cycle rerun after a fresh deploy:

```bash
PLAN_SESSION_MODE=deploy ./scripts/run-vaio-sgx-plan-session.sh
```

That now automatically means:

- `selected_phase_profile=full`

Explicit override remains available:

```bash
PLAN_PHASE_PROFILE=current ./scripts/run-vaio-sgx-plan-session.sh
PLAN_PHASE_PROFILE=post_descriptor ./scripts/run-vaio-sgx-plan-session.sh
PLAN_PHASE_PROFILE=plan_query ./scripts/run-vaio-sgx-plan-session.sh
PHASES=plan_op,plan_query,prepare_op,prepare_query ./scripts/run-vaio-sgx-plan-session.sh
```

## Local Verification

Local checks passed:

- `bash -n` on the touched wrappers and phase tools
- `./scripts/resolve-sgx-submit-phase-profile.sh full`
- `./scripts/resolve-sgx-submit-phase-profile.sh post_descriptor`
- `./scripts/resolve-sgx-submit-phase-profile.sh plan_query`
- `./scripts/resolve-vaio-plan-current-phase.sh`
- direct blocked investigation with `PHASE_PROFILE=post_descriptor`:
  - [logs/vaio-ioctl/20260512-144134-sgx-submit-op-phase-investigation-profile-check](/root/Poulsbo/logs/vaio-ioctl/20260512-144134-sgx-submit-op-phase-investigation-profile-check)
  - manifest recorded:
    - `phase_profile=post_descriptor`
    - `phase_selection=profile`
    - `phases=descriptor_query,plan_op,plan_query,prepare_op,prepare_query`
- bounded blocked top-level recovery run:
  - [logs/vaio-plan-session/20260512-144400-sgx-plan-session-profile-check-bounded](/root/Poulsbo/logs/vaio-plan-session/20260512-144400-sgx-plan-session-profile-check-bounded)
  - result recorded:
    - `selected_mode=recovery`
    - `selected_phase_profile=post_descriptor`
    - `session_outcome=blocked_before_remote_usable`

No new hardware rerun was attempted in this batch.
