# VAIO Rollback Guard Healthy-Boot Validation

This note records the first real-hardware validation of the guarded deploy
rollback path on the Sony VAIO VGN-P11Z target.

## Context

[docs/48-vaio-boot-rollback-guard.md](/root/Poulsbo/docs/48-vaio-boot-rollback-guard.md)
captured the design and local script validation for the rollback guard while
the target was still unstable.

The healthy-boot half of that design is now validated through the live deploy
bundle at:

- [logs/vaio-deploy/20260511-065750-rollback-guard-live](/root/Poulsbo/logs/vaio-deploy/20260511-065750-rollback-guard-live)

The session result is:

- [session-result.env](/root/Poulsbo/logs/vaio-deploy/20260511-065750-rollback-guard-live/session-result.env)
  with:
  - `session_outcome=completed`
  - `preflight_overall=remote_usable`
  - `pre_state_rc=0`
  - `deploy_rc=0`
  - `post_state_rc=0`

## What Was Validated

This was a guarded redeploy of the already validated override module:

- module `srcversion`: `2B42B80E462B9A3C0BAAC9B`
- target kernel: `5.4.0-150-generic`

The goal of this run was not to trigger rollback. It was to prove the normal
healthy path:

1. preflight requires `overall=remote_usable`
2. rollback guard arms before reboot
3. the current override is preserved as a rollback backup
4. the target reboots and returns to `overall=remote_usable`
5. the wrapper clears the pending rollback marker
6. the target remains on the intended override module

## Observed Hardware Result

The guarded deploy log is:

- [guarded-deploy.log](/root/Poulsbo/logs/vaio-deploy/20260511-065750-rollback-guard-live/guarded-deploy.log)

Key observed events:

- preflight started from:
  - `overall=remote_usable`
- the wrapper armed the rollback guard:
  - `rollback_guard=armed`
  - `rollback_pending_file=/var/lib/poulsbo-rollback/pending.env`
  - `rollback_backup_module_path=/var/lib/poulsbo-rollback/gma500_gfx.previous.ko`
- the currently installed override was preserved:
  - `rollback_backup=updated`
- the wrapper expanded the post-reboot wait window:
  - `effective_post_reboot_timeout_secs=360`
- after reboot, the target returned to:
  - `overall=remote_usable`
- the wrapper then cleared the guard:
  - `rollback_guard=cleared`

The final post-state is captured in:

- [post-state.log](/root/Poulsbo/logs/vaio-deploy/20260511-065750-rollback-guard-live/post-state.log)

That final state shows:

- `driver_srcversion=2B42B80E462B9A3C0BAAC9B`
- `rollback_service_enabled=enabled`
- `rollback_service_active=inactive`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

## Conclusion

The rollback guard is now validated for the healthy deploy/reboot path on real
hardware.

What remains unvalidated is narrower:

- the deliberate failure-path test where the guard is left pending long enough
  to restore the previous override automatically

That later failure-path validation is now recorded in:

- [docs/56-vaio-rollback-guard-autorestore-validation.md](/root/Poulsbo/docs/56-vaio-rollback-guard-autorestore-validation.md)
