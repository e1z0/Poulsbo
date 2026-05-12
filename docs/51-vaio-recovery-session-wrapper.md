# VAIO Recovery Session Wrapper

This note records a single-command recovery workflow added while the VAIO is
still blocked in `overall=ssh_port_down`.

## Why This Exists

The repo now has separate helpers for:

- health probing
- timestamped health watching
- wait-until-usable behavior
- rollback-guard handling
- remote state inspection
- the pending `depclear_slot` validator

The missing piece was one wrapper that ties those together into a single,
timestamped local session directory for the next real recovery window.

## Added Script

- [scripts/run-vaio-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-recovery-session.sh)

It:

1. creates a timestamped session directory under `logs/vaio-recovery/`
2. writes a small `session.env` manifest
3. waits for `overall=remote_usable` using the timestamped watcher
4. captures the remote `gma500` and rollback-guard state
5. runs one post-recovery validation command

By default it writes:

- `health-timeline.log`
- `remote-state.log`
- `depclear-slot-validation.log`
- `session.env`
- `session-result.env`

The wrapper is now validation-agnostic. The default remains the staged
`depclear_slot` path, but callers can now override:

- `SESSION_KIND`
- `VALIDATION_LABEL`
- `VALIDATION_LOG_BASENAME`
- `VALIDATION_SCRIPT`

## Intended Use

After the VAIO is recovered from the console, leave this running locally:

```bash
./scripts/run-vaio-recovery-session.sh
```

Useful overrides:

```bash
SESSION_NAME=after-console-fix ./scripts/run-vaio-recovery-session.sh
MAX_POLLS=120 POLL_INTERVAL_SECS=5 ./scripts/run-vaio-recovery-session.sh
```

## Local Validation

Local checks passed:

- `bash -n scripts/run-vaio-recovery-session.sh`
- `MAX_POLLS=1 POLL_INTERVAL_SECS=1 ./scripts/run-vaio-recovery-session.sh`
  against the current blocked target state

That bounded test created a real session directory and stopped cleanly after
one blocked poll instead of hanging indefinitely. The wrapper now also records
a structured session result plus explicit skipped-state markers when the target
never reaches `overall=remote_usable`.

The target is still currently at:

- `ping_status=ok`
- `tcp_22_status=closed`
- `ssh_pubkey_status=connection_refused`
- `overall=ssh_port_down`

So this wrapper is staged locally and waiting for the next real recovery
window.

## Next Step

Use this wrapper during the next console recovery so the repo captures one
coherent recovery session instead of several separate helper invocations.
