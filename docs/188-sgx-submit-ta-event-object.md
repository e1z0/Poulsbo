# SGX Submit TA Event Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 42F7BCB76CD9DEFD437A03A`

## Goal

The validated SGX-shaped submit stack already had one shared apply-side top
object:

- `psb_sgx_lab_submit_sgx_ta_commit`

That closed the split around the queue/CCB/sync/owner write plan and the
next-submit-state apply image, but one top-layer live split still remained:

- `sgx_ta_commit` owned the write/apply image
- `issue_submit()` and `transition_submit()` still carried their own
  event-side count/timestamp/refresh bookkeeping inline

This batch adds the first explicit SGX-shaped scheduler event object above
`sgx_ta_commit`, so the live top layer now owns both the apply image and the
event-side submit accounting plan.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal top object:

- `psb_sgx_lab_submit_sgx_ta_event`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_event_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_event_locked()`

The new `sgx_ta_event` object now keeps together:

- the existing `sgx_ta_commit`
- the live issue-vs-transition event kind
- the event-side count increment plan
- the event-side timestamp update plan
- the refresh-views plan after apply

That means the live top-layer SGX path is now:

- validator: `sgx_ta_event`
- `issue_submit()`: `sgx_ta_event`
- `transition_submit()`: `sgx_ta_event`

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical live proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-181949-sgx-ta-event-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-181949-sgx-ta-event-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-182148-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-182148-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-182251-sgx-submit-ta-event-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-182251-sgx-submit-ta-event-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-182331-sgx-multi-submit-ta-event-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-182331-sgx-multi-submit-ta-event-live/summary.log)

Final live module:

- `driver_srcversion=42F7BCB76CD9DEFD437A03A`
- `override_module_size=464220`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed again:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=42F7BCB76CD9DEFD437A03A`
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
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`

## Typed Multi-Submit Evidence

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

The SGX-shaped kernel-internal runtime stack now has twenty-one validated
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
- `psb_sgx_lab_submit_sgx_ta_dispatch`
- `psb_sgx_lab_submit_sgx_ta_path`
- `psb_sgx_lab_submit_sgx_ta_commit`
- `psb_sgx_lab_submit_sgx_ta_event`

This is still lab-mode execution, not real SGX rendering, but the typed top
layer now crosses one more real boundary: the shared SGX top object no longer
stops at the write/apply image. `sgx_ta_event` also owns the live event-side
submit accounting plan for both mutation entrypoints.

The next useful step is the first explicit SGX sequence, scheduler, or
microkernel-facing work-item object above `sgx_ta_event`.
