# SGX Stable Lab Milestone

## Goal

Freeze a usable SGX lab milestone for the VAIO today instead of continuing to
chase more risky top-layer kernel objects.

This milestone is not a finished end-user graphics driver. It is a pinned,
repeatable SGX lab baseline that can be redeployed, revalidated, and recovered
without changing the kernel line again first.

## Pinned Baseline

- top-level commit:
  - `8556d312`
- kernel subrepo commit:
  - `a89b91f8b`
- pinned validated live module:
  - `driver_srcversion=92CB8FBC111248DC82C2825`
  - `override_module_size=481200`

That exact module was already the latest clean SGX `ta_request_sequence`
validation line. The work here was to restore it as the active baseline after
the later uncommitted `request_work_packet` experiment failed a guarded reboot.

## What Changed

- restored the workspace to the pinned kernel line instead of continuing the
  uncommitted `request_work_packet` batch
- rebuilt the exact local module and confirmed it still resolved to
  `srcversion 92CB8FBC111248DC82C2825`
- redeployed that same module with a longer rollback-confirmation window
- added [scripts/run-vaio-sgx-stable-soak-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-stable-soak-session.sh)
  to bundle repeated stability proof cycles under one parent session

The soak wrapper runs, per cycle:

1. exact `complete` no-redeploy `plan` proof
2. focused typed submit proof
3. typed multi-submit proof

## Validation

Restored guarded deploy:

- [logs/vaio-deploy/20260514-082755-sgx-stable-milestone-deploy](/root/Poulsbo/logs/vaio-deploy/20260514-082755-sgx-stable-milestone-deploy)
- final live state:
  - `driver_srcversion=92CB8FBC111248DC82C2825`
  - `override_module_size=481200`
  - `lightdm_active=inactive`
  - `kdump_service_active=active`
  - `overall=remote_usable`

Canonical exact proof on that line remains:

- [logs/vaio-plan-session/20260514-002126-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-002126-sgx-plan-session)
- `session_outcome=completed`
- `selected_phase_profile=complete`

Three-cycle stability soak:

- [logs/vaio-stability/20260514-083309-sgx-stable-soak](/root/Poulsbo/logs/vaio-stability/20260514-083309-sgx-stable-soak)
- wrapper result:
  - `session_outcome=completed`
  - `cycles=3`
  - `completed_cycles=3`
  - `last_overall=remote_usable`
  - `final_driver_srcversion=92CB8FBC111248DC82C2825`

The soak held the same module through all three cycles without losing remote
usability.

## Working Definition

For this repo, the stable milestone now means:

- the VAIO is reachable and headless-safe
- panic capture and rollback remain armed
- the exact `complete` proof is repeatable without redeploy
- focused typed submit proof is repeatable
- typed multi-submit proof is repeatable
- the pinned module can survive repeated proof cycles unchanged

## Operator Commands

Exact proof on the pinned line:

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete ./scripts/run-vaio-sgx-plan-session.sh
```

Three-cycle stable-soak proof:

```bash
CYCLES=3 ./scripts/run-vaio-sgx-stable-soak-session.sh
```

## Next Step

Future kernel work should branch from this pinned stable line. The next change
should only move forward if it can beat or at least preserve this exact
baseline.
