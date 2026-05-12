# VAIO Rollback Guard Auto-Restore Validation

This note records the real-hardware validation of the guarded deploy failure
path on the Sony VAIO VGN-P11Z target.

## Context

[docs/48-vaio-boot-rollback-guard.md](/root/Poulsbo/docs/48-vaio-boot-rollback-guard.md)
captured the rollback-guard design and local script validation.
[docs/55-vaio-rollback-guard-healthy-boot-validation.md](/root/Poulsbo/docs/55-vaio-rollback-guard-healthy-boot-validation.md)
validated the normal healthy deploy/reboot path.

This milestone validates the remaining half: leave the rollback guard armed
after the first healthy boot, wait for the target-side timeout, and prove that
the previous override is restored automatically.

The live session bundle is:

- [logs/vaio-rollback/20260511-070400-autorestore-live](/root/Poulsbo/logs/vaio-rollback/20260511-070400-autorestore-live)

The session result is:

- [session-result.env](/root/Poulsbo/logs/vaio-rollback/20260511-070400-autorestore-live/session-result.env)
  with:
  - `session_outcome=completed`
  - `rollback_watch_rc=0`
  - `final_override_size=366368`
  - `final_pending_present=0`
  - `final_backup_present=1`

## What Was Validated

The session wrapper
[scripts/run-vaio-rollback-autorestore-session.sh](/root/Poulsbo/scripts/run-vaio-rollback-autorestore-session.sh)
used a deliberately modified candidate override:

- source module path:
  `build/devnull-vaio-gma500-src/gma500_gfx.ko`
- source `srcversion`: `2B42B80E462B9A3C0BAAC9B`
- source size: `366368`
- stripped candidate size: `364748`
- candidate `srcversion`: still `2B42B80E462B9A3C0BAAC9B`

That size difference was intentional. It made the rollback result visible even
though the module `srcversion` stayed the same.

The session then:

1. armed the rollback guard with `ROLLBACK_GRACE_SECS=60`
2. deployed the stripped candidate
3. intentionally left the guard armed after the first healthy boot
4. waited for the target-side rollback service to fire
5. verified that the previous override was restored automatically

## Observed Hardware Result

The phase-1 guarded deploy log is:

- [phase1-guarded-deploy.log](/root/Poulsbo/logs/vaio-rollback/20260511-070400-autorestore-live/phase1-guarded-deploy.log)

The mid-state after the first healthy boot is:

- [mid-state.log](/root/Poulsbo/logs/vaio-rollback/20260511-070400-autorestore-live/mid-state.log)

That mid-state proves the machine really came back on the candidate override:

- `override_module_size=364748`
- `rollback_pending_present=1`
- `rollback_pending_grace_secs=60`
- `rollback_backup_present=1`
- `rollback_backup_size=366368`

The rollback-watch timeline is:

- [rollback-watch.log](/root/Poulsbo/logs/vaio-rollback/20260511-070400-autorestore-live/rollback-watch.log)

Key observed events from that timeline:

- the target stayed `overall=remote_usable` through `2026-05-11T07:10:11+03:00`
- the target then dropped to `overall=ssh_port_down` at:
  - `2026-05-11T07:10:34+03:00`
  - `2026-05-11T07:10:51+03:00`
- the target returned to `overall=remote_usable` at
  `2026-05-11T07:11:10+03:00`

The target-side journal confirms why:

- `systemd[1]: Started Rollback unconfirmed Poulsbo override module.`
  at `07:08:55`
- `poulsbo-rollback[6301]: confirmation missing; restoring /var/lib/poulsbo-rollback/gma500_gfx.previous.ko to /lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko and rebooting`
  at `07:09:55`

After the restore reboot, the service started again at `07:10:45` and exited
quickly because the pending marker had already been removed.

The final post-state is:

- [post-state.log](/root/Poulsbo/logs/vaio-rollback/20260511-070400-autorestore-live/post-state.log)

That final state shows:

- `driver_srcversion=2B42B80E462B9A3C0BAAC9B`
- `override_module_size=366368`
- `rollback_pending_present=0`
- `rollback_backup_present=1`
- `rollback_service_enabled=enabled`
- `rollback_service_active=inactive`

## Conclusion

The rollback guard is now validated on real hardware in both directions:

- healthy boot with explicit confirmation
- unconfirmed boot with automatic previous-override restore

That means the VAIO deploy safety path is no longer a design-only safeguard.
It is a validated recovery mechanism for future risky module boots.
