# VAIO Auto-Resume Helper

This note records the small local helper added while the VAIO is blocked in
`overall=ssh_port_down`.

## Why This Exists

[docs/46-sgx-dependency-slot-clear.md](/root/Poulsbo/docs/46-sgx-dependency-slot-clear.md)
already records the current blocker: the new local `depclear_slot` module is
built, deployed, and waiting for hardware validation, but the target needs
console recovery first.

The useful next improvement is not more unvalidated kernel work. It is reducing
the delay between console recovery and the pending validation run.

## Helper

Added:

- [scripts/wait-vaio-target-ready.sh](/root/Poulsbo/scripts/wait-vaio-target-ready.sh)

It:

- polls `scripts/check-vaio-target-health.sh`
- waits until `overall=remote_usable`
- optionally runs a command immediately after the target becomes usable

## Intended Use

After recovering the VAIO from its local console, this will wait and then start
the pending clear-slot validator automatically:

```bash
./scripts/wait-vaio-target-ready.sh ./scripts/run-vaio-sgx-dependency-slot-clear.sh
```

If you only want to watch the health state without launching a follow-up
command:

```bash
./scripts/wait-vaio-target-ready.sh
```

Useful environment overrides:

- `POLL_INTERVAL_SECS=5`
- `MAX_POLLS=60`
- `TARGET_HOST=10.1.11.218`

## Validation

Local checks passed:

- `bash -n scripts/wait-vaio-target-ready.sh`
- `MAX_POLLS=1 POLL_INTERVAL_SECS=1 ./scripts/wait-vaio-target-ready.sh`
  against the current blocked target state, which exited cleanly after one poll
  with `overall=ssh_port_down`

The helper was not end-to-end validated in this step because the VAIO still
remains at:

- `ping_status=ok`
- `tcp_22_status=closed`
- `ssh_pubkey_status=connection_refused`
- `overall=ssh_port_down`

## Next Step

The next step is unchanged:

- recover the VAIO on the console
- wait for `overall=remote_usable`
- run `./scripts/run-vaio-sgx-dependency-slot-clear.sh`
