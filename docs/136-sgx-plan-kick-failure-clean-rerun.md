# SGX `plan` Kick Failure Clean Rerun

## Summary

Superseded on May 12, 2026 by
[docs/137-sgx-plan-timeout-hardening-and-post-descriptor-crash.md](/root/Poulsbo/docs/137-sgx-plan-timeout-hardening-and-post-descriptor-crash.md)
for current operator state. This note remains accurate for the earlier
`2BE09D0DC1CCBDB51D2328E` live candidate and its direct `kick` rejection.

The current `plan` candidate is no longer blocked on either target recovery
or generic phase labels.

On May 12, 2026, the VAIO stayed at `overall=remote_usable` on override module
`srcversion 2BE09D0DC1CCBDB51D2328E`, size `429820`, and the top-level
recovery-mode operator entrypoint was rerun after one coherent runner-side
capture batch:

- `PLAN_SESSION_MODE=recovery ./scripts/run-vaio-sgx-plan-session.sh`

The resulting parent session now carries the real live blocker directly:

- `selected_failed_phase=sync`
- `selected_failed_step=submit_op_kick_focus`
- `selected_failed_rc=1`

So the current blocker is no longer “phase `sync` fails somewhere later”.
It is:

- the live candidate stays healthy and reachable
- the focused typed submit path fails inside the `sync` prefix
- the concrete failing substep is `submit_op_kick_focus`
- the failing ioctl returns `submit_op_ret=-1 errno=22`

## Runner Fixes

This rerun included one coherent propagation batch across the investigation
stack.

- `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
  - now captures both stdout and stderr from the focused submit runner
  - now records `first_bad_failed_step`, `first_bad_failed_rc`,
    `first_bad_failed_log`, and `first_bad_remote_tmpdir`
- `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
  - now captures both stdout and stderr from the focused submit runner
  - now records `last_phase_failed_step`, `last_phase_failed_rc`,
    `last_phase_failed_log`, and `last_phase_remote_tmpdir`
- `scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
  - now preserves the child failed-step fields from both bisect and confirm
- `scripts/run-vaio-recovery-session.sh`
  - now captures stderr from the validation child
  - now propagates `validation_failed_phase`, `validation_failed_step`, and
    `validation_failed_rc` from the child investigation result
- `scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`
  - now prefers the recovery child’s explicit failed-phase and failed-step
    fields when they exist
- `scripts/run-vaio-sgx-plan-session.sh`
  - now surfaces the selected child’s failed phase, step, and rc directly in
    the top-level parent result

## Live Validation

Top-level parent bundle:

- [logs/vaio-plan-session/20260512-111909-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-111909-sgx-plan-session)

Child bundles:

- recovery wrapper:
  [logs/vaio-plan-session/20260512-111909-sgx-plan-session/children/20260512-111915-recovery](/root/Poulsbo/logs/vaio-plan-session/20260512-111909-sgx-plan-session/children/20260512-111915-recovery)
- bisect:
  [logs/vaio-plan-session/20260512-111909-sgx-plan-session/children/20260512-111923-recovery/children/20260512-111923-bisect](/root/Poulsbo/logs/vaio-plan-session/20260512-111909-sgx-plan-session/children/20260512-111923-recovery/children/20260512-111923-bisect)
- confirm:
  [logs/vaio-plan-session/20260512-111909-sgx-plan-session/children/20260512-111923-recovery/children/20260512-112103-confirm](/root/Poulsbo/logs/vaio-plan-session/20260512-111909-sgx-plan-session/children/20260512-111923-recovery/children/20260512-112103-confirm)

Validated results:

- top-level parent:
  - `session_outcome=validation_failed`
  - `plan_session_mode=recovery`
  - `preflight_overall=remote_usable`
  - `last_overall=remote_usable`
  - `final_driver_srcversion=2BE09D0DC1CCBDB51D2328E`
  - `selected_failed_phase=sync`
  - `selected_failed_step=submit_op_kick_focus`
  - `selected_failed_rc=1`
- recovery wrapper:
  - `session_outcome=validation_failed`
  - `validation_failed_phase=sync`
  - `validation_failed_step=submit_op_kick_focus`
  - `validation_failed_rc=1`
- bisect:
  - `session_outcome=bisect_identified_bad_prefix`
  - `probe_order=prepare,report,sync`
  - `last_good_phase=none`
  - `first_bad_phase=sync`
  - `first_bad_reason=phase_failed`
  - `first_bad_failed_step=submit_op_kick_focus`
  - `first_bad_failed_rc=1`
  - `preflight_overall=remote_usable`
  - `last_overall=remote_usable`
- confirm:
  - `session_outcome=phase_failed`
  - `last_phase=sync`
  - `last_phase_rc=1`
  - `last_phase_health=remote_usable`
  - `last_phase_failed_step=submit_op_kick_focus`
  - `last_phase_failed_rc=1`

The captured failing ioctl output is stable across the direct prefixes:

- `submit_op_name=kick`
- `submit_selector=focus`
- `submit_op_ret=-1 errno=22`
- `submit_cmd_type_name=ta`
- `submit_ccb_offset=0x20`
- `submit_ctrl_flags=0x1`
- `submit_num_ta_status_vals=1`
- `submit_num_3d_status_vals=1`

## Corrected Diagnosis

The earlier clean rerun in [docs/135-sgx-plan-sync-failure-clean-rerun.md](/root/Poulsbo/docs/135-sgx-plan-sync-failure-clean-rerun.md)
proved that the target stayed healthy and that the parent phase label was
stable, but it did not preserve stderr from the focused submit runner. That
left the repo state saying that `prepare` and `report` were healthy.

The corrected live rerun shows that this was incomplete.

What is true now:

- the target is healthy and remotely usable
- the current phase ordering still makes the parent label `first_bad_phase=sync`
- the real failure is earlier inside that cumulative prefix
- every probed prefix (`prepare`, `report`, and `sync`) dies at the same
  substep:
  - `submit_op_kick_focus`

So the next implementation step is not more wrapper work. It is kernel-side
analysis centered on why the typed focused `kick focus ta 0x20 0x1 1 1 0`
configuration now returns `-EINVAL` on the current `plan` candidate.
