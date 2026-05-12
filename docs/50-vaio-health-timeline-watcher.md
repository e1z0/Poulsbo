# VAIO Health Timeline Watcher

This note records a timestamped watcher added while the VAIO is still blocked
in `overall=ssh_port_down`.

## Why This Exists

The current repo already has:

- one-shot health probing through
  [scripts/check-vaio-target-health.sh](/root/Poulsbo/scripts/check-vaio-target-health.sh)
- wait-until-usable behavior through
  [scripts/wait-vaio-target-ready.sh](/root/Poulsbo/scripts/wait-vaio-target-ready.sh)

The missing piece was a compact state timeline for:

- console recovery
- rollback testing
- guarded deploy observation across several reboot phases

## Added Script

- [scripts/watch-vaio-target-health.sh](/root/Poulsbo/scripts/watch-vaio-target-health.sh)

It:

- polls the existing health probe repeatedly
- prints one compact timestamped line every poll
- prints the full probe block on state transitions
- can optionally print the full probe block every poll
- can optionally append the whole timeline to a log file
- can optionally stop when `overall` reaches a chosen value

## Useful Examples

Watch until the target becomes usable:

```bash
STOP_ON_OVERALL=remote_usable ./scripts/watch-vaio-target-health.sh
```

Watch for a bounded number of polls and save a log:

```bash
LOG_FILE=logs/vaio-health.log MAX_POLLS=60 POLL_INTERVAL_SECS=5 \
  ./scripts/watch-vaio-target-health.sh
```

Print the full probe every poll instead of only on transitions:

```bash
PRINT_FULL_EACH_POLL=1 MAX_POLLS=5 ./scripts/watch-vaio-target-health.sh
```

## Local Validation

Local checks passed:

- `bash -n scripts/watch-vaio-target-health.sh`
- `MAX_POLLS=2 POLL_INTERVAL_SECS=1 ./scripts/watch-vaio-target-health.sh`
  against the current blocked target state

The current observed target state is still:

- `ping_status=ok`
- `tcp_22_status=closed`
- `ssh_pubkey_status=connection_refused`
- `overall=ssh_port_down`

## Next Step

Use this watcher during the next console recovery or guarded deploy so the repo
has a real timestamped target-health timeline instead of only isolated manual
probes.
