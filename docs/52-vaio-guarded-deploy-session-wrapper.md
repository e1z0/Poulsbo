# VAIO Guarded Deploy Session Wrapper

This note records a single-command guarded-deploy workflow added while the VAIO
is still blocked in `overall=ssh_port_down`.

## Why This Exists

The repo already has:

- the guarded deploy script
- the rollback guard
- the health probe
- the remote state inspection helper

What was still missing was a structured local session bundle around one guarded
deploy attempt. Without that, a blocked preflight or failed deploy still mostly
left a terminal transcript instead of explicit session artifacts.

## Added Script

- [scripts/run-vaio-guarded-deploy-session.sh](/root/Poulsbo/scripts/run-vaio-guarded-deploy-session.sh)

It:

1. creates a timestamped local session directory under `logs/vaio-deploy/`
2. records a small local manifest
3. captures preflight target health
4. captures pre-deploy target state when the target is usable
5. runs the guarded deploy flow
6. captures post-deploy target state on success
7. writes a structured session result file

By default it writes:

- `preflight-health.log`
- `pre-state.log`
- `guarded-deploy.log`
- `post-state.log`
- `session.env`
- `session-result.env`

## Current Blocked-Target Behavior

When preflight health is not `overall=remote_usable`, the wrapper now:

- writes explicit skipped markers for pre-state, deploy, and post-state
- records `session_outcome=blocked_preflight`
- exits with a non-zero status instead of silently doing nothing

That is the useful behavior while the VAIO is still down now.

## Intended Use

Once the VAIO is usable again, run:

```bash
./scripts/run-vaio-guarded-deploy-session.sh
```

Useful overrides:

```bash
SESSION_NAME=after-recovery ./scripts/run-vaio-guarded-deploy-session.sh
AUTO_ROLLBACK_GUARD=1 ROLLBACK_GRACE_SECS=180 \
  ./scripts/run-vaio-guarded-deploy-session.sh
```

## Local Validation

Local checks passed:

- `bash -n scripts/run-vaio-guarded-deploy-session.sh`
- `./scripts/run-vaio-guarded-deploy-session.sh` against the current blocked
  target state

The bounded blocked-state validation produced a real local session bundle with:

- `session_outcome=blocked_preflight`
- `preflight_overall=ssh_port_down`

The current observed target state is still:

- `ping_status=ok`
- `tcp_22_status=closed`
- `ssh_pubkey_status=connection_refused`
- `overall=ssh_port_down`

## Next Step

Use this wrapper for the first guarded deploy after console recovery so the
deployment attempt is captured as one coherent local session bundle.
