# SGX Submit TA Schedule Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 801A7DA2CBDDDCBEBC3AFEF`

## Goal

The SGX-shaped submit validator already had an explicit top control object:

- `psb_sgx_lab_submit_sgx_ta_control`

That still left one important gap above the control layer. The live
`transition_submit()` path was still recalculating owner-state progression,
queue advancement, sync completion, and ticket movement inline:

- running admission already consumed `sgx_ta_control`
- issue readiness already consumed `sgx_ta_control`
- queue/read/write/ticket progression still lived in ad hoc transition logic

This batch adds one higher object:

- `psb_sgx_lab_submit_sgx_ta_schedule`

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal objects:

- `struct psb_sgx_lab_submit_sgx_transition_request`
- `struct psb_sgx_lab_submit_sgx_ta_schedule`

New internal builders/helpers:

- `poulsbo_sgx_build_submit_sgx_ta_schedule_locked()`
- `poulsbo_sgx_submit_sgx_ta_schedule_transition_status_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_schedule_locked()`
- `poulsbo_sgx_validate_submit_sgx_ta_schedule_locked()`

The new `sgx_ta_schedule` object builds above `sgx_ta_control` and makes the
top scheduling/progression state explicit:

- whether a queued submit is currently issueable
- whether a running submit can reissue
- whether a queued submit can enter running
- whether a running submit can stay running
- whether a running submit can complete
- if a transition is requested, the exact resulting queue/read/write offsets,
  packet counts, sync flags, cookies, and global ticket movement

The live `transition_submit()` path now consumes that typed schedule object
instead of recomputing progression inline from raw submit fields. The validator
also moved from `sgx_ta_control` to `sgx_ta_schedule`, so the same top-layer
progression rules are checked offline and used live.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical live proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-160724-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-160724-guarded-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-160909-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-160909-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-161024-sgx-submit-ta-schedule-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-161024-sgx-submit-ta-schedule-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-161102-sgx-multi-submit-ta-schedule-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-161102-sgx-multi-submit-ta-schedule-live/summary.log)

Final live module:

- `driver_srcversion=801A7DA2CBDDDCBEBC3AFEF`
- `override_module_size=461036`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed again:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=801A7DA2CBDDDCBEBC3AFEF`
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
- submit B later observes dependency completion and still remains valid:
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

The SGX-shaped kernel-internal runtime stack now has seventeen validated
layers:

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
- `psb_sgx_lab_submit_sgx_ta_control`
- `psb_sgx_lab_submit_sgx_ta_schedule`

This is still lab-mode execution, not real SGX rendering, but the top
scheduling and owner-state progression rules are now one shared SGX-shaped
object instead of a mix of typed control checks plus open-coded transition
math.

The next useful step is the next more microkernel-facing SGX scheduler or
dispatch object above this repaired `sgx_ta_schedule` layer.
