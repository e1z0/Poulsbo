# SGX Submit TA Gate Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 643122821ACD1D37A506EB0`

## Goal

The SGX-shaped submit validator already had an explicit top runtime object:

- `psb_sgx_lab_submit_sgx_ta_runtime`

That still left one live semantic gap. The actual transition into
`OWNER_RUNNING` was still driven by ad hoc dependency gating logic in the
transition path, while the validator only knew the raw runtime fields.

This batch adds one explicit gate object above runtime:

- `psb_sgx_lab_submit_sgx_ta_gate`

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal objects:

- `struct psb_sgx_lab_submit_sgx_gate_slot`
- `struct psb_sgx_lab_submit_sgx_ta_gate`

New internal builders/helpers:

- `poulsbo_sgx_build_submit_sgx_ta_gate_locked()`
- `poulsbo_sgx_submit_sgx_ta_gate_status_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_gate_locked()`

The validator now builds `sgx_ta_gate` above `sgx_ta_runtime`, and the live
`transition -> running` path now uses the same typed gate object instead of the
older slot-by-slot gate helper. The old unused helper was removed after the
first local build so the batch stays warning-free.

Semantically, the new gate object makes these states explicit:

- active dependency slots
- source-bound versus source-missing dependencies
- gated versus ungated dependencies
- satisfied versus blocked dependencies
- whether the running slot is already busy
- whether the submit can enter running right now

That keeps the repaired `blocked but valid before issue` rule from the runtime
batch, while making the actual transition gate explicit and shared.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical live proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-151922-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-151922-guarded-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-152113-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-152113-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-152220-sgx-submit-ta-gate-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-152220-sgx-submit-ta-gate-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-152220-sgx-multi-submit-ta-gate-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-152220-sgx-multi-submit-ta-gate-live/summary.log)

Final live module:

- `driver_srcversion=643122821ACD1D37A506EB0`
- `override_module_size=458160`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed again:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=643122821ACD1D37A506EB0`
- focused typed submit stayed terminally clean:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - `submit_dependency_active_count=0`

## Typed Multi-Submit Evidence

- submit B still remains blocked-but-valid before dependency release:
  - `SUBMIT_B_STILL_IDLE_BEFORE_DEPENDENCY_RELEASE`
  - `submit_issue_count=0`
  - `submit_transition_count=0`
  - `submit_dependency_active_count=1`
  - `dependency[1]_required_ticket=3`
  - `dependency[1]_observed_complete_ticket=0`
- submit B later observes dependency completion and stays valid:
  - `SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_A`
  - `submit_dependency_active_count=1`
  - `dependency[1]_required_ticket=3`
  - `dependency[1]_observed_complete_ticket=5`
- final typed multi-submit state retired cleanly:
  - `SUBMIT_A_ID=3`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - `SUBMIT_B_ID=4`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_dependency_active_count=1`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`

## Result

The SGX-shaped kernel-internal runtime stack now has fifteen validated layers:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`
- `psb_sgx_lab_submit_ccb_kick`
- `psb_sgx_lab_submit_sgx_ccb_kick`
- `psb_sgx_lab_submit_sgx_cmdta_shared`
- `psb_sgx_lab_submit_sgx_cmdta`
- `psb_sgx_lab_submit_sgx_ta_kick`
- `psb_sgx_lab_submit_sgx_ta_command`
- `psb_sgx_lab_submit_sgx_queued_ta_command`
- `psb_sgx_lab_submit_sgx_ta_submission`
- `psb_sgx_lab_submit_sgx_ta_execution`
- `psb_sgx_lab_submit_sgx_ta_timeline`
- `psb_sgx_lab_submit_sgx_ta_runtime`
- `psb_sgx_lab_submit_sgx_ta_gate`

This is still lab-mode execution, not real SGX rendering, but the runtime gate
that governs transition into running is now a first-class SGX-shaped object
shared by both the validator and the live transition path.

The next useful step is the next more microkernel-facing SGX control object
above this repaired `sgx_ta_gate` layer.
