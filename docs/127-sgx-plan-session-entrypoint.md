# Submit `plan` Session Entrypoint

## Summary

The recovery stack now has a good deploy-side wrapper and a good recovery-side
wrapper, but the operator still had to choose between them manually. This batch
adds one top-level entrypoint that probes current target health and selects the
right parent flow automatically.

## Added Script

- [scripts/run-vaio-sgx-plan-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-session.sh)

## Behavior

The entrypoint first runs the normal health probe and then:

- chooses `recovery` when the target is not currently `overall=remote_usable`
- chooses `deploy` when the target is currently `overall=remote_usable`

The chosen child session is then run under one parent bundle in
`logs/vaio-plan-session/`.

The parent result file records:

- `plan_session_mode`
- `plan_phase_profile`
- `preflight_overall`
- `selected_mode`
- `selected_script`
- `selected_phase_profile`
- `selected_phase_selection`
- `selected_phases`
- `selected_child_phase_profile`
- `selected_child_phase_selection`
- `selected_child_phases`
- `selected_session_dir`
- `selected_outcome`
- `last_overall`
- `final_driver_srcversion`

Overrides remain available:

- `PLAN_SESSION_MODE=auto`
- `PLAN_SESSION_MODE=recovery`
- `PLAN_SESSION_MODE=deploy`
- `PLAN_PHASE_PROFILE=auto`
- `PLAN_PHASE_PROFILE=full`
- `PLAN_PHASE_PROFILE=post_descriptor`
- `PLAN_PHASES=<csv>`

The recent session wrappers now also accept:

- `SESSION_TIMESTAMP=<YYYYMMDD-HHMMSS>`

through a shared collision-safe allocator, so repeated test runs forced into the
same timestamp no longer trample each other.

## Intended Use

This is now the preferred operator entrypoint:

```bash
./scripts/run-vaio-sgx-plan-session.sh
```

If the target is still down, it will select the recovery path. If the target is
already usable, it will select the full-cycle guarded-deploy path.

With the default `PLAN_PHASE_PROFILE=auto`, it now also chooses:

- `selected_phase_profile=post_descriptor` for recovery
- `selected_phase_profile=full` for deploy

To force the recovery branch:

```bash
PLAN_SESSION_MODE=recovery ./scripts/run-vaio-sgx-plan-session.sh
```

To force the full-cycle deploy branch:

```bash
PLAN_SESSION_MODE=deploy ./scripts/run-vaio-sgx-plan-session.sh
```

To force the narrower post-`descriptor` profile even through the top-level
selector:

```bash
PLAN_PHASE_PROFILE=post_descriptor ./scripts/run-vaio-sgx-plan-session.sh
```

To force one explicit phase list through the top-level selector:

```bash
PLAN_PHASES=plan_op,plan_query ./scripts/run-vaio-sgx-plan-session.sh
```

The latest semantics follow-up also keeps the top-level operator choice
separate from the child recovery wrapper's internal state, so blocked runs can
now show `selected_phase_selection=explicit` together with
`selected_child_phase_selection=request_only`. See:

- [docs/144-sgx-plan-phase-selection-semantics.md](/root/Poulsbo/docs/144-sgx-plan-phase-selection-semantics.md)

## Local Verification

Local checks passed:

- `bash -n scripts/run-vaio-sgx-plan-session.sh`
- repeated same-timestamp runs now allocate distinct directories instead of
  colliding

Canonical blocked-target collision test bundles:

- [logs/vaio-plan-session/20260512-001500-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-001500-sgx-plan-session)
- [logs/vaio-plan-session/20260512-001500-sgx-plan-session-1](/root/Poulsbo/logs/vaio-plan-session/20260512-001500-sgx-plan-session-1)

Those two runs used the same:

- `SESSION_TIMESTAMP=20260512-001500`

and now both finish cleanly with:

- `session_outcome=blocked_before_remote_usable`
- `last_overall=ssh_port_down`

No hardware validation was possible in this batch because the VAIO was still at
`overall=ssh_port_down`.
