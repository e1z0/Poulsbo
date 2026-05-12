# SGX `plan_query_basic` Live Rerun

## Summary

This batch used the split `plan_query` window on a live no-redeploy rerun and
narrowed the current hardware failure one step further:

- from `plan_query`
- to `plan_query_basic`

It also taught the operator layer about the three split `plan_query_*`
subphases as first-class named profiles, so the next profile-form rerun is now
exact instead of falling back to the broader `post_descriptor` alias.

## Live Rerun

The bounded top-level session is:

- [logs/vaio-plan-session/20260512-201327-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-201327-sgx-plan-session)

It was started with:

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_query_fetch,plan_query_basic,plan_query_snapshot ./scripts/run-vaio-sgx-plan-session.sh
```

The saved top-level result is:

- `session_outcome=validation_failed`
- `selected_failed_phase=plan_query_basic`
- `selected_failed_reason=phase_timeout`
- `final_driver_srcversion=0EEEE302E3FDA9F59B7CFB9`

## What Cleared Before The Failure

The bisect child is:

- [logs/vaio-plan-session/20260512-201327-sgx-plan-session/children/20260512-201341-recovery/children/20260512-201341-bisect](/root/Poulsbo/logs/vaio-plan-session/20260512-201327-sgx-plan-session/children/20260512-201341-recovery/children/20260512-201341-bisect)

Its first and only probe was:

- `probe_order=plan_query_basic`

The live phase log for that probe shows:

- `TARGET_DRM_NODE_MISSING=/dev/dri/card0`
- `AUTOLOAD_GMA500_MODULE=gma500_gfx`
- `TARGET_DRM_NODE_READY=/dev/dri/card0`
- `STAGE_OK=sync`
- `STAGE_OK=owner`
- `STAGE_OK=report`
- `STAGE_OK=manifest`
- `STAGE_OK=descriptor_op`
- `STAGE_OK=descriptor_query`
- `STAGE_OK=plan_op`

So the autoload self-heal worked, and the actual live failure is now after:

- `descriptor_query`
- `plan_op`

inside the narrowed `plan_query_basic` probe window.

## Saved Diagnosis Result

The investigation child saved:

- `session_outcome=terminal_bad_prefix_identified`
- `bisect_outcome=bisect_terminated_on_unhealthy_bad_probe`
- `bisect_first_bad_phase=plan_query_basic`
- `bisect_first_bad_reason=phase_timeout`
- `confirm_outcome=skipped_target_lost`

The bisect child itself saved:

- `last_good_phase=none`
- `first_bad_phase=plan_query_basic`
- `first_bad_reason=phase_timeout`
- `authoritative_terminal_bad=1`

The target then returned to:

- `overall=ssh_port_down`

## Profile Follow-Up

Updated:

- [scripts/resolve-sgx-submit-phase-profile.sh](/root/Poulsbo/scripts/resolve-sgx-submit-phase-profile.sh)
- [scripts/lib-vaio-plan-summary.sh](/root/Poulsbo/scripts/lib-vaio-plan-summary.sh)
- [scripts/run-vaio-sgx-plan-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-plan-session.sh)

The operator layer now recognizes these as first-class profiles:

- `plan_query_fetch`
- `plan_query_basic`
- `plan_query_snapshot`

The shared current-boundary resolver now lands on:

- `resolved_profile=plan_query_basic`
- `resolved_phases=plan_query_basic`
- `resolved_phase=plan_query_basic`
- `resolved_reason=phase_timeout`
- `source_timestamp=20260512-201327`
- `source_row_module_srcversion=0EEEE302E3FDA9F59B7CFB9`

And the current-state report now recommends:

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_query_basic ./scripts/run-vaio-sgx-plan-session.sh
```

Equivalent profile form:

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=plan_query_basic ./scripts/run-vaio-sgx-plan-session.sh
```

## Validation

Validated with:

- `bash -n scripts/resolve-sgx-submit-phase-profile.sh`
- `bash -n scripts/lib-vaio-plan-summary.sh`
- `bash -n scripts/run-vaio-sgx-plan-session.sh`
- `./scripts/resolve-vaio-plan-current-phase.sh | rg '^(resolved_|source_)'`
- `./scripts/report-vaio-plan-status.sh | rg '^(current_recovery_|recommended_next_)'`
- `MAX_SESSIONS=6 ./scripts/summarize-vaio-plan-sessions.sh`

At the end of this batch the VAIO is down again:

- `ping_status=fail`
- `tcp_22_status=closed`
- `ssh_pubkey_status=no_route`
- `overall=ssh_port_down`
