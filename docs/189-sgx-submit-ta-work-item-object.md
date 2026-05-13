# SGX Submit TA Work Item Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 2077F2C05141C6DFD29769B`

## Goal

The validated SGX-shaped submit stack already had one shared live event object:

- `psb_sgx_lab_submit_sgx_ta_event`

That closed the split around the event-side count/timestamp/refresh plan, but
one live top-layer split still remained:

- `sgx_ta_event` owned the event-side accounting image
- `issue_submit()` and `transition_submit()` still carried separate request
  validation and pre-apply execution rules inline

This batch adds the first explicit SGX-shaped live work item object above
`sgx_ta_event`, so the top layer now owns both the event image and the
issue-vs-transition request execution rules.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal top object:

- `psb_sgx_lab_submit_sgx_ta_work_item`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_work_item_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_work_item_locked()`

The new `sgx_ta_work_item` object now keeps together:

- the existing `sgx_ta_event`
- the issue-vs-transition request kind
- the request owner state and packet count
- the pre-apply validation requirement
- the stale-build disarm rule for issue-side work

That means the live top-layer SGX path is now:

- validator: `sgx_ta_work_item`
- `issue_submit()`: `sgx_ta_work_item`
- `transition_submit()`: `sgx_ta_work_item`

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical live proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-183718-sgx-ta-work-item-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-183718-sgx-ta-work-item-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-183909-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-183909-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-184011-sgx-submit-ta-work-item-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-184011-sgx-submit-ta-work-item-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-184054-sgx-multi-submit-ta-work-item-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-184054-sgx-multi-submit-ta-work-item-live/summary.log)

Final live module:

- `driver_srcversion=2077F2C05141C6DFD29769B`
- `override_module_size=464604`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed again:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=2077F2C05141C6DFD29769B`
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

The SGX-shaped kernel-internal runtime stack now has twenty-two validated
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
- `psb_sgx_lab_submit_sgx_ta_work_item`

This is still lab-mode execution, not real SGX rendering, but the typed top
layer now crosses another real live boundary: the shared object no longer stops
at the event accounting plan. `sgx_ta_work_item` also owns the issue-vs-
transition request execution rules above that event image.

The next useful step is the first explicit SGX sequence, scheduler, or
microkernel-facing work queue object above `sgx_ta_work_item`.
