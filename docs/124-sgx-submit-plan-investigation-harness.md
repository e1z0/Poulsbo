# Submit `plan` Investigation Harness

## Summary

The repo now has both a prefix-bisect wrapper and an exhaustive phase matrix,
but the best first recovery command is the one that composes them into one
workflow. This batch adds that composed investigation harness.

## Added Script

- [scripts/run-vaio-sgx-submit-op-phase-investigation.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-investigation.sh)

It runs:

1. the prefix bisect first
2. then a narrowed confirmation pass only when the bisect identifies a bad
   cumulative prefix

## Behavior

The investigation harness creates one parent bundle under `logs/vaio-ioctl/`
and then launches child sessions underneath it:

- `children/*-bisect`
- `children/*-confirm`

Its result file records:

- `bisect_outcome`
- `bisect_last_good_phase`
- `bisect_first_bad_phase`
- `bisect_first_bad_reason`
- `confirm_phases`
- `confirm_outcome`

When the target is healthy enough to reach the runner, the intended outcomes
are:

- `no_bad_prefix_found`
- `confirmed_bad_prefix`
- `confirm_inconclusive`

## Recovery Wrapper Default

The dedicated recovery-session wrapper now defaults to this investigation path:

- [scripts/run-vaio-sgx-plan-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-recovery-session.sh)

with:

- `VALIDATION_LABEL=submit_op_phase_investigation`
- `VALIDATION_LOG_BASENAME=submit-op-phase-investigation.log`
- `VALIDATION_SCRIPT=./scripts/run-vaio-sgx-submit-op-phase-investigation.sh`

Fallbacks remain available:

- direct bisect:
  - [scripts/run-vaio-sgx-submit-op-phase-bisect.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-bisect.sh)
- exhaustive matrix:
  - [scripts/run-vaio-sgx-submit-op-phase-matrix.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-matrix.sh)

There is now also a higher-level full-cycle parent wrapper for the next risky
deploy:

- [docs/125-sgx-plan-guarded-deploy-session-wrapper.md](/root/Poulsbo/docs/125-sgx-plan-guarded-deploy-session-wrapper.md)

## Intended Use

After console recovery, the preferred first command is:

```bash
./scripts/run-vaio-sgx-plan-recovery-session.sh
```

To run the investigation harness directly:

```bash
./scripts/run-vaio-sgx-submit-op-phase-investigation.sh
```

To narrow the search window up front:

```bash
PHASES=descriptor_query,plan_op,plan_query,prepare_op,prepare_query ./scripts/run-vaio-sgx-submit-op-phase-investigation.sh
```

## Local Verification

Local checks passed:

- `bash -n scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
- `bash -n scripts/run-vaio-sgx-plan-recovery-session.sh`
- `./scripts/run-vaio-sgx-submit-op-phase-investigation.sh`

That direct run produced a real blocked investigation bundle at:

- [logs/vaio-ioctl/20260512-000636-sgx-submit-op-phase-investigation](/root/Poulsbo/logs/vaio-ioctl/20260512-000636-sgx-submit-op-phase-investigation)

with:

- `session_outcome=blocked_preflight`
- `bisect_outcome=blocked_preflight`
- `confirm_outcome=skipped`
- `preflight_overall=ssh_port_down`
- `last_overall=ssh_port_down`
- `final_driver_srcversion=3D21987EBD495B4E78FBBA7`

The parent investigation result now also records:

- `preflight_overall`
- `last_overall`
- `final_driver_srcversion`

No hardware validation was possible in this batch because the VAIO was still at
`overall=ssh_port_down`.
