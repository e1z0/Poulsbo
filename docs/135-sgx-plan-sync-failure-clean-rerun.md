# SGX `plan` Sync Failure Clean Rerun

Superseded by [docs/136-sgx-plan-kick-failure-clean-rerun.md](/root/Poulsbo/docs/136-sgx-plan-kick-failure-clean-rerun.md).
This note captures the earlier clean rerun before the phase harnesses and
parent wrappers were updated to preserve stderr and explicit failed-step
metadata.

## Summary

The `plan` candidate is no longer blocked on generic recovery uncertainty.
On May 12, 2026, the VAIO was back at `overall=remote_usable`, running
override module `srcversion 2BE09D0DC1CCBDB51D2328E`, and the top-level
`plan` investigation path was rerun twice through
`./scripts/run-vaio-sgx-plan-session.sh`.

The first live recovery-mode run already isolated the first bad cumulative
phase to `sync`, but the focused submit-op runner still leaked local shell
expansion noise from the remote heredoc wrapper. The second rerun fixed that
runner bug and confirmed the same result cleanly.

So the current blocker is now explicit:

- `prepare` prefix: healthy
- `report` prefix: healthy
- `sync` prefix: first failing cumulative phase
- target health during failure: still `overall=remote_usable`

## Runner Fixes

The clean rerun included one coherent runner-side cleanup batch:

- `scripts/run-vaio-sgx-ioctl-submit-op.sh`
  - fixed the remaining remote-heredoc `stage_name` expansion leak in
    `mark_stage_complete()`
- `scripts/run-vaio-recovery-session.sh`
  - now records `final_driver_srcversion` in `session-result.env`
- `scripts/run-vaio-guarded-deploy-session.sh`
  - now records `last_overall` and `final_driver_srcversion`
- `scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`
  - now prefers the child deploy session's `last_overall` instead of
    incorrectly reusing only `preflight_overall`
- `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
  - now records `preflight_overall` and `last_overall`
- `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
  - now records `preflight_overall` and `last_overall`

That left the parent investigation and top-level `plan` session bundles with
fully populated:

- `preflight_overall`
- `last_overall`
- `final_driver_srcversion`

## Live Validation

### First live recovery-mode `plan` session

Top-level bundle:

- [logs/vaio-plan-session/20260512-104039-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-104039-sgx-plan-session)

Useful result:

- top-level session completed its recovery branch
- first bad cumulative phase was already `sync`
- target stayed `overall=remote_usable`
- parent result already carried `final_driver_srcversion=2BE09D0DC1CCBDB51D2328E`

Limitation:

- the focused submit-op runner still emitted
  `stage_name: unbound variable` noise from the remote heredoc wrapper

### Clean live recovery-mode `plan` session

Top-level bundle:

- [logs/vaio-plan-session/20260512-104449-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-104449-sgx-plan-session)

Child bundles:

- bisect:
  [logs/vaio-plan-session/20260512-104449-sgx-plan-session/children/20260512-104502-recovery/children/20260512-104502-bisect](/root/Poulsbo/logs/vaio-plan-session/20260512-104449-sgx-plan-session/children/20260512-104502-recovery/children/20260512-104502-bisect)
- confirm:
  [logs/vaio-plan-session/20260512-104449-sgx-plan-session/children/20260512-104502-recovery/children/20260512-104638-confirm](/root/Poulsbo/logs/vaio-plan-session/20260512-104449-sgx-plan-session/children/20260512-104502-recovery/children/20260512-104638-confirm)

Validated results:

- top-level:
  - `session_outcome=validation_failed`
  - `plan_session_mode=recovery`
  - `preflight_overall=remote_usable`
  - `last_overall=remote_usable`
  - `final_driver_srcversion=2BE09D0DC1CCBDB51D2328E`
- bisect:
  - `session_outcome=bisect_identified_bad_prefix`
  - `probe_order=prepare,report,sync`
  - `last_good_phase=none`
  - `first_bad_phase=sync`
  - `first_bad_reason=phase_failed`
  - `preflight_overall=remote_usable`
  - `last_overall=remote_usable`
- confirm:
  - `session_outcome=phase_failed`
  - `last_phase=sync`
  - `last_phase_rc=1`
  - `last_phase_health=remote_usable`
  - `preflight_overall=remote_usable`
  - `last_overall=remote_usable`

The rerun produced no more heredoc `unbound variable` noise from the focused
submit-op wrapper.

## Conclusion

The `plan` candidate is now live enough to support bounded diagnosis instead
of broad recovery work.

Current state:

- target health: `overall=remote_usable`
- live override module: `srcversion 2BE09D0DC1CCBDB51D2328E`
- first failing cumulative phase: `sync`
- earlier cumulative prefixes `prepare` and `report`: healthy

So the next implementation step is not more recovery plumbing. It is
kernel-side and runner-side analysis centered on the `sync` phase itself.
