# SGX `plan` Timeout Hardening And Post-Descriptor Crash

## Summary

The old live `kick` rejection is no longer the active blocker on the current
`plan` line.

On May 12, 2026, a new guarded deploy session loaded override module
`srcversion 0EEEE302E3FDA9F59B7CFB9`, size `429756`, on the VAIO and started
the top-level `plan` recovery investigation automatically:

- parent session:
  [logs/vaio-plan/20260512-113722-sgx-plan-guarded-deploy](/root/Poulsbo/logs/vaio-plan/20260512-113722-sgx-plan-guarded-deploy)

Before the machine dropped off the network, the investigation log showed that
the focused cumulative prefix had already advanced past the earlier `kick`
failure point:

- `STAGE_OK=sync`
- `STAGE_OK=owner`
- `STAGE_OK=report`
- `STAGE_OK=manifest`
- `STAGE_OK=descriptor`

So the current live failure is later than the old `submit_op_kick_focus`
`-EINVAL` rejection on `srcversion 2BE09D0DC1CCBDB51D2328E`.

The downside is that the target died before the old investigation wrappers
could emit a final structured answer, which exposed a second real bug in the
operator stack: mid-phase target loss could leave the bisect / matrix /
investigation chain hanging without a completed `session-result.env`.

## Script Hardening

The wrapper stack now has explicit timeout-bounded execution and reason
propagation.

Updated scripts:

- `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
- `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
- `scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
- `scripts/run-vaio-recovery-session.sh`
- `scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`
- `scripts/run-vaio-sgx-plan-session.sh`

New behavior:

- per-phase focused submit runs are now bounded by `PHASE_TIMEOUT_SECS`
- phase health probes are now bounded by `HEALTH_TIMEOUT_SECS`
- state capture is now bounded by `STATE_TIMEOUT_SECS`
- recovery validation children are now bounded by
  `VALIDATION_TIMEOUT_SECS`
- bisect now records `first_bad_reason`
  - including `phase_timeout` and `health_timeout`
- matrix now records `last_phase_reason`
  - including `phase_timeout`, `phase_failed`, `health_timeout`,
    `phase_unhealthy`
- investigation now preserves `confirm_last_phase_reason`
- recovery and top-level plan sessions now propagate the selected child’s
  failure reason, not just phase / step / rc

## Local Validation

The timeout path is now validated locally with stubbed health, state, and
submit helpers.

Validated outcomes:

- direct bisect:
  - `session_outcome=bisect_identified_bad_prefix`
  - `first_bad_phase=sync`
  - `first_bad_reason=phase_timeout`
- direct investigation:
  - `bisect_first_bad_reason=phase_timeout`
  - `confirm_outcome=phase_timeout`
  - `confirm_last_phase_reason=phase_timeout`
- recovery wrapper:
  - `session_outcome=validation_failed`
  - `validation_failed_phase=sync`
  - `validation_failed_reason=phase_timeout`

This closes the hanging-wrapper gap even when a child phase never returns
cleanly.

## Live State

The current real hardware state after the post-descriptor crash is:

- `ping_status=fail`
- `tcp_22_status=closed`
- `ssh_pubkey_status=no_route`
- `overall=ssh_port_down`

So there is still no complete hardware answer yet for the new `plan`
candidate. What we do know is narrower and useful:

- the old live `kick` failure is no longer first
- the new candidate survives through `descriptor`
- the next failure is at or after `plan`
- the latest local runner follow-up now splits that boundary into:
  - `descriptor_query`
  - `plan_op`
  - `plan_query`
  - `prepare_op`
  - `prepare_query`
- the operator stack now has bounded timeout reporting for the next rerun

## Next Step

After console recovery, rerun the standard operator path:

```bash
./scripts/run-vaio-sgx-plan-session.sh
```

If the machine comes back on the already-loaded candidate and you want to avoid
an immediate redeploy:

```bash
PLAN_SESSION_MODE=recovery ./scripts/run-vaio-sgx-plan-session.sh
```
