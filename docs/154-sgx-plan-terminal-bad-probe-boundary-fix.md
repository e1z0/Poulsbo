# SGX `plan` Terminal-Bad Probe Boundary Fix

## Summary

The saved `plan` diagnosis around
[20260512-180606-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-180606-sgx-plan-session)
had been interpreted too aggressively.

The first bisect probe:

- `phase=plan_query`

timed out and left the VAIO at:

- `overall=ssh_port_down`

The later bisect probe:

- `phase=descriptor_query`

did not start on a healthy target. Its phase log is only:

```text
ssh: connect to host 10.1.11.218 port 22: No route to host
lost connection
```

So `descriptor_query` was not valid new boundary evidence. It was only a
follow-up probe issued after the earlier `plan_query` probe had already taken
the target down.

This batch fixes that in two places:

1. future bisects now stop as soon as a bad probe leaves the target unhealthy
2. the offline summary/resolver path now backfills the same rule onto older
   saved bundles

That means the current saved recovery boundary moves back from
`descriptor_query` to the real terminal bad probe:

- `resolved_profile=plan_query`
- `resolved_phase=plan_query`
- `resolved_reason=phase_timeout`

## Runner Changes

Updated:

- [scripts/run-vaio-sgx-submit-op-phase-bisect.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-bisect.sh)
- [scripts/run-vaio-sgx-submit-op-phase-investigation.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-investigation.sh)

The bisect runner now records and respects an authoritative terminal bad probe.
If a bad probe leaves the target in anything other than `remote_usable`, it:

- marks that probe as `authoritative_terminal`
- stops the bisect immediately
- persists:
  - `authoritative_terminal_bad`
  - `authoritative_last_started_phase`
  - `authoritative_last_good_phase`
  - `authoritative_first_bad_phase`
  - `authoritative_first_bad_reason`

The investigation wrapper now recognizes
`bisect_terminated_on_unhealthy_bad_probe` and skips the pointless confirm
matrix rerun on an already-dead target.

## Offline Backfill

Updated:

- [scripts/lib-vaio-plan-summary.sh](/root/Poulsbo/scripts/lib-vaio-plan-summary.sh)
- [scripts/summarize-vaio-plan-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-plan-sessions.sh)

The shared summary library now computes effective bisect evidence from saved
probe summaries, even for older bundles that predate the new authoritative
fields.

For the `180606` session, the effective evidence is now:

- `evidence_last_started=plan_query`
- `evidence_last_good=-`
- `evidence_first_bad=plan_query`
- `evidence_reason=phase_timeout`

instead of the earlier incorrect:

- `evidence_last_started=descriptor_query`
- `evidence_first_bad=descriptor_query`
- `evidence_reason=phase_failed`

## Result

The shared current-boundary resolver now lands on:

- `resolved_profile=plan_query`
- `resolved_phases=plan_query`
- `resolved_phase=plan_query`
- `resolved_reason=phase_timeout`
- `source_name=sgx-plan-session`
- `source_timestamp=20260512-180606`
- `source_row_evidence_first_bad=plan_query`
- `source_row_evidence_reason=phase_timeout`

The offline summary now shows the same corrected evidence directly on the
`180606` row:

- `evidence_last_started=plan_query`
- `evidence_first_bad=plan_query`
- `evidence_reason=phase_timeout`

And the next narrowed rerun is back to:

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_query ./scripts/run-vaio-sgx-plan-session.sh
```

Equivalent profile form:

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=plan_query ./scripts/run-vaio-sgx-plan-session.sh
```

## Validation

Validated locally with:

- `bash -n scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
- `bash -n scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
- `bash -n scripts/lib-vaio-plan-summary.sh`
- `bash -n scripts/summarize-vaio-plan-sessions.sh`
- `./scripts/resolve-vaio-plan-current-phase.sh | rg '^(resolved_|source_|source_row_)'`
- `MAX_SESSIONS=6 ./scripts/summarize-vaio-plan-sessions.sh`

The VAIO itself is still down at the end of this batch:

- `ping_status=fail`
- `tcp_22_status=closed`
- `ssh_pubkey_status=no_route`
- `overall=ssh_port_down`
