# SGX `plan_query` Autoload Gap And Live Rerun

## Summary

This batch closed one operational gap in the focused submit runner and one real
summary-layer bug, then re-ran the current live `plan` boundary on the booted
VAIO.

The important split is:

1. the booted machine had the override module file installed but no live DRM
   device because `gma500_gfx` had not been loaded
2. after manually loading `gma500_gfx`, the real `plan_query` timeout still
   reproduced on the already-loaded candidate

So the missing DRM node was a real boot/runtime issue, but it was not the root
cause of the `plan` failure itself.

## Boot Inspection

The VAIO came back reachable with:

- `overall=remote_usable`

The loaded override state on disk was still the current `plan` candidate:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `driver_srcversion=0EEEE302E3FDA9F59B7CFB9`
- `rollback_pending_present=0`

But the graphics stack was not actually live after boot:

- `/dev/dri` missing
- `/sys/class/drm` missing
- `lsmod | grep gma500` empty
- `lspci -nnk` still listed `Kernel modules: gma500_gfx`

Manual recovery proved that the module itself still loaded:

```bash
modprobe gma500_gfx
```

After that:

- `lsmod` showed `gma500_gfx`
- `/dev/dri/card0` existed
- `/sys/class/drm/card0` existed
- `dmesg` showed normal `gma500` init on `0000:00:02.0`

## First Live Rerun: Missing DRM Node

The first recovery-mode rerun on the booted machine is:

- [logs/vaio-plan-session/20260512-193915-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-193915-sgx-plan-session)

It failed before any real SGX diagnosis because the target had no DRM node at
the time the focused submit path started:

- `selected_failed_phase=plan_query`
- `selected_failed_reason=phase_failed`
- `selected_failed_step=gtt_before`
- `drm_open_errno=2`

The target stayed healthy:

- `last_overall=remote_usable`

That established the operational regression cleanly:

- override file present after boot
- `gma500_gfx` not yet loaded
- no `/dev/dri/card0`

## Runner Follow-Up

Updated:

- [scripts/run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)

The focused submit runner now self-heals that boot state before the first
`gtt_before` probe:

- detects missing target DRM node
- runs `modprobe gma500_gfx`
- waits for the target node to appear
- records:
  - `TARGET_DRM_NODE_MISSING`
  - `AUTOLOAD_GMA500_MODULE`
  - `TARGET_DRM_NODE_READY`

That prevents future no-redeploy `plan` reruns from failing early for the wrong
reason when the override module is installed but not yet bound.

## Second Live Rerun: Real `plan_query` Failure Still Reproduces

After manually loading `gma500_gfx`, the same no-redeploy recovery rerun was
performed again:

- [logs/vaio-plan-session/20260512-194235-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-194235-sgx-plan-session)

This time the machine started with a live DRM node and still failed on the same
logical boundary:

- `selected_failed_phase=plan_query`
- `selected_failed_reason=phase_timeout`
- `selected_failed_step=`
- `final_driver_srcversion=0EEEE302E3FDA9F59B7CFB9`

Its bisect child:

- [logs/vaio-plan-session/20260512-194235-sgx-plan-session/children/20260512-194247-recovery/children/20260512-194247-bisect](/root/Poulsbo/logs/vaio-plan-session/20260512-194235-sgx-plan-session/children/20260512-194247-recovery/children/20260512-194247-bisect)

saved the terminal bad probe as:

- `probe_order=plan_query`
- `first_bad_phase=plan_query`
- `first_bad_reason=phase_timeout`
- `authoritative_terminal_bad=1`

So the real live boundary is still `plan_query`.

## `plan_query` Subphase Split

Updated:

- [scripts/run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
- [scripts/run-vaio-sgx-submit-op-phase-matrix.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-matrix.sh)
- [scripts/run-vaio-sgx-submit-op-phase-bisect.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-bisect.sh)
- [scripts/resolve-sgx-submit-phase-profile.sh](/root/Poulsbo/scripts/resolve-sgx-submit-phase-profile.sh)
- [scripts/lib-vaio-plan-summary.sh](/root/Poulsbo/scripts/lib-vaio-plan-summary.sh)

The old coarse `plan_query` stop point is now split into:

- `plan_query_fetch`
- `plan_query_basic`
- `plan_query_snapshot`

The named profile:

- `PLAN_PHASE_PROFILE=plan_query`

now expands to:

- `PLAN_PHASES=plan_query_fetch,plan_query_basic,plan_query_snapshot`

That makes the next rerun narrow enough to answer whether the crash happens on:

- the post-`plan` query fetch itself
- the basic offset/config assertions
- the full `plan` snapshot readback

## Summary-Layer `module_srcversion` Fix

Updated:

- [scripts/summarize-vaio-plan-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-plan-sessions.sh)

The offline plan summary had been preferring:

- `local_module_srcversion`

from the wrapper manifest over the actual live:

- `final_driver_srcversion`

from the session result.

That made the newest remote-usable evidence rows show the local build artifact
`6E95EDFFFB5384C8E58F25B` instead of the actually loaded candidate
`0EEEE302E3FDA9F59B7CFB9`.

The precedence is now:

1. `final_driver_srcversion`
2. `post-state.log` `driver_srcversion`
3. `pre-state.log` `driver_srcversion`
4. `source_module_srcversion`
5. `local_module_srcversion`

So the current saved live boundary now reports the real loaded module identity.

## Result

The current saved recovery boundary is now resolved as:

- `resolved_profile=plan_query`
- `resolved_phases=plan_query_fetch,plan_query_basic,plan_query_snapshot`
- `resolved_phase=plan_query`
- `resolved_reason=phase_timeout`
- `source_timestamp=20260512-194235`
- `source_row_module_srcversion=0EEEE302E3FDA9F59B7CFB9`
- `source_row_evidence_first_bad=plan_query`
- `source_row_evidence_reason=phase_timeout`

The current report now recommends:

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_query_fetch,plan_query_basic,plan_query_snapshot ./scripts/run-vaio-sgx-plan-session.sh
```

Equivalent profile form:

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=plan_query ./scripts/run-vaio-sgx-plan-session.sh
```

## Validation

Validated with:

- `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `bash -n scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
- `bash -n scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
- `bash -n scripts/resolve-sgx-submit-phase-profile.sh`
- `bash -n scripts/lib-vaio-plan-summary.sh`
- `bash -n scripts/report-vaio-plan-status.sh`
- `bash -n scripts/summarize-vaio-plan-sessions.sh`
- `./scripts/resolve-vaio-plan-current-phase.sh | rg '^(resolved_|source_)'`
- `./scripts/report-vaio-plan-status.sh | rg '^(current_recovery_|recommended_next_)'`
- `MAX_SESSIONS=10 ./scripts/summarize-vaio-plan-sessions.sh`

At the end of this batch the target is again down:

- `ping_status=fail`
- `tcp_22_status=closed`
- `ssh_pubkey_status=no_route`
- `overall=ssh_port_down`
