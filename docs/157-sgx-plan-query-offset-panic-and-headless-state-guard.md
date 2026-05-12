# `plan_query_offset` live narrowing, headless state probe, and unsafe-module guard

On May 12, 2026, the VAIO was brought back in an intentionally headless state:
`lightdm` disabled, no auto-started Xorg, but the SGX override module still on
disk. That exposed a real gap in the target-state probe and also let the next
`plan` rerun narrow the live failure one step further.

## Headless-state probe update

Updated:

- `scripts/show-vaio-gma500-state.sh`

The probe now reports:

- whether `gma500_gfx` is actually loaded
- DRM class/card presence:
  - `drm_sys_class_present`
  - `drm_card_entries`
  - `drm_connector_entries`
  - `dri_dev_dir_present`
  - `dri_dev_nodes`
  - `drm_card0_present`
- headless/display-manager state:
  - `lightdm_unit_present`
  - `lightdm_enabled`
  - `lightdm_active`
  - `system_default_target`
  - `xorg_process_count`

Live state before the rerun on `10.1.11.218`:

- `driver_srcversion=0EEEE302E3FDA9F59B7CFB9`
- `driver_module_loaded=1`
- `drm_card0_present=1`
- `lightdm_active=inactive`
- `xorg_process_count=0`

So the machine was healthy and headless, not merely “missing X”.

## Exact live narrowing

Updated:

- `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `scripts/resolve-sgx-submit-phase-profile.sh`
- `scripts/lib-vaio-plan-summary.sh`
- `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
- `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`

`plan_query_basic` is no longer a single coarse stage. It now expands to:

- `plan_query_offset`
- `plan_query_config`

The live rerun is:

- `logs/vaio-plan-session/20260512-210302-sgx-plan-session`

Saved result:

- `selected_failed_phase=plan_query_offset`
- `selected_failed_reason=phase_timeout`
- `final_driver_srcversion=0EEEE302E3FDA9F59B7CFB9`

The child bisect result is:

- `logs/vaio-plan-session/20260512-210302-sgx-plan-session/children/20260512-210314-recovery/children/20260512-210314-bisect/session-result.env`

It recorded:

- `probe_order=plan_query_offset`
- `first_bad_phase=plan_query_offset`
- `first_bad_reason=phase_timeout`
- `authoritative_terminal_bad=1`

The captured phase log still cleared:

- `STAGE_OK=sync`
- `STAGE_OK=owner`
- `STAGE_OK=report`
- `STAGE_OK=manifest`
- `STAGE_OK=descriptor_op`
- `STAGE_OK=descriptor_query`
- `STAGE_OK=plan_op`

before the machine dropped.

## Console panic and conclusion

The console panic after that rerun showed:

- `Kernel panic - not syncing: stack-protector: Kernel stack is corrupted in: poulsbo_sgx_fill_submit_query_locked`
- call path through:
  - `psb_ioctl_sgx_lab_query_submit`
  - `poulsbo_sgx_fill_submit_query_locked`

That makes the current loaded override `srcversion=0EEEE302E3FDA9F59B7CFB9`
unsafe for further no-redeploy `plan_query*` recovery runs. It also matches the
already-fixed local `plan` word-count overrun work from
`docs/138-sgx-submit-word-block-size-hardening.md`: the crash occurs in the
read-side submit query path exactly where a stale oversized plan read would
corrupt the stack.

## Operator guard

Updated:

- `scripts/run-vaio-sgx-plan-recovery-session.sh`
- `scripts/report-vaio-plan-status.sh`
- `scripts/run-vaio-sgx-plan-session.sh`

Behavior now:

- recovery-side `plan_query*` reruns are refused on the known-unsafe live
  module `0EEEE302E3FDA9F59B7CFB9`
- the status report switches the next recommendation from `recovery` to
  `deploy`

Current report state:

- `current_recovery_profile=plan_query_offset`
- `current_recovery_phases=plan_query_offset`
- `current_recovery_live_module_safe=0`
- `current_recovery_live_module_warning=known_unsafe_plan_query_live_module`

Current next commands:

```bash
PLAN_SESSION_MODE=deploy PLAN_PHASES=plan_query_offset ./scripts/run-vaio-sgx-plan-session.sh
```

```bash
PLAN_SESSION_MODE=deploy PLAN_PHASE_PROFILE=plan_query_offset ./scripts/run-vaio-sgx-plan-session.sh
```

Equivalent guarded-deploy parent:

```bash
./scripts/run-vaio-sgx-plan-guarded-deploy-session.sh
```
