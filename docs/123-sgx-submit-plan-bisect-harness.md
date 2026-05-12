# Submit `plan` Prefix-Bisect Harness

## Summary

The first `plan`-block hardware run took the VAIO down, and the stage-matrix
rerun path was still broader than necessary for the first recovery window. The
next useful local-only step was a prefix-bisect harness that finds the earliest
failing cumulative stage with fewer reruns than the full matrix.

## Added Script

- [scripts/run-vaio-sgx-submit-op-phase-bisect.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-bisect.sh)

It uses the same focused typed submit-op path as the phase matrix, but instead
of testing every prefix in order it probes cumulative prefixes through a binary
search over the ordered stage list.

## Behavior

The bisect harness:

1. checks target health
2. captures pre-state
3. runs `STOP_AFTER_STAGE=<stage>` probes on selected cumulative prefixes
4. checks target health after each probe
5. narrows to the earliest bad prefix
6. captures post-state and writes one structured result bundle

The ordered stage list is the same as the phase matrix:

- `sync`
- `owner`
- `report`
- `manifest`
- `descriptor`
- `plan`
- `prepare`
- `arm`
- `emit`
- `validate`
- `launch`
- `issue`
- `running`
- `complete`

The result file records:

- `probe_count`
- `probe_order`
- `last_good_phase`
- `first_bad_phase`
- `first_bad_reason`

## Relationship To Recovery
The direct bisect wrapper remains available for manual use, but the dedicated
recovery-session wrapper now defaults to the composed investigation harness
instead:

- [docs/124-sgx-submit-plan-investigation-harness.md](/root/Poulsbo/docs/124-sgx-submit-plan-investigation-harness.md)

The older exhaustive path still exists and remains useful as a fallback:

- [scripts/run-vaio-sgx-submit-op-phase-matrix.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-op-phase-matrix.sh)

## Intended Use

To run the bisect harness directly:

```bash
./scripts/run-vaio-sgx-submit-op-phase-bisect.sh
```

If the search should be restricted to a narrower stage window:

```bash
PHASES=plan,prepare,arm,emit ./scripts/run-vaio-sgx-submit-op-phase-bisect.sh
```

## Local Verification

Local checks passed:

- `bash -n scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
- `bash -n scripts/run-vaio-sgx-plan-recovery-session.sh`
- `./scripts/run-vaio-sgx-submit-op-phase-bisect.sh`

That direct run produced a real blocked session bundle at:

- [logs/vaio-ioctl/20260511-235723-sgx-submit-op-phase-bisect](/root/Poulsbo/logs/vaio-ioctl/20260511-235723-sgx-submit-op-phase-bisect)

with:

- `session_outcome=blocked_preflight`
- `last_state=ssh_port_down`

No hardware validation was possible in this batch because the VAIO was still at
`overall=ssh_port_down`.
