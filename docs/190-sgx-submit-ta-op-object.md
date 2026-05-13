# SGX Submit TA Op Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion F387C89C0831778420098EB`

## Goal

The validated SGX-shaped submit stack already had one shared live work-item
object:

- `psb_sgx_lab_submit_sgx_ta_work_item`

That closed the split around the live event image and the issue-vs-transition
execution rules, but the public request boundary was still duplicated:

- `issue_submit()` and `transition_submit()` still owned separate allocation,
  lock, build, apply, and logging setup
- the submit-op ioctl path still open-coded separate field validation for
  `ISSUE` and `TRANSITION`

This batch adds the first explicit SGX-shaped live submit-op object above
`sgx_ta_work_item`, so the top layer now reaches the public issue/transition
request boundary.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal top object:

- `psb_sgx_lab_submit_sgx_ta_op`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_op_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_op_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_op()`
- `poulsbo_sgx_validate_submit_sgx_ta_ioctl_op_request()`

The new `sgx_ta_op` object now keeps together:

- the existing `sgx_ta_work_item`
- the public request kind
- the request owner state
- the request packet count
- the top validator boundary for the live submit-op path

That means the live top-layer SGX path is now:

- validator: `sgx_ta_op`
- `issue_submit()`: `sgx_ta_op`
- `transition_submit()`: `sgx_ta_op`
- submit-op ioctl `ISSUE`/`TRANSITION` request validation: `sgx_ta_op`

This batch also tightens one real mismatch at the public boundary:

- ioctl `ISSUE` now rejects `owner_state=complete`
- ioctl `ISSUE` now rejects `packet_count=0`

That matches the actual live builder rules which already required queued or
running ownership and a non-zero issue packet count.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical live proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-185946-sgx-ta-op-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-185946-sgx-ta-op-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-190135-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-190135-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-190259-sgx-submit-ta-op-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-190259-sgx-submit-ta-op-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-190345-sgx-multi-submit-ta-op-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-190345-sgx-multi-submit-ta-op-live/summary.log)

Final live module:

- `driver_srcversion=F387C89C0831778420098EB`
- `override_module_size=463956`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed again:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=F387C89C0831778420098EB`
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
  - `SUBMIT_A_ID=4`
  - `SUBMIT_B_ID=5`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
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
    - `dependency[1]_observed_complete_ticket=6`

## Result

The SGX-shaped kernel-internal runtime stack now has twenty-three validated
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
- `psb_sgx_lab_submit_sgx_ta_op`

This is still lab-mode execution, not real SGX rendering, but the shared top
layer now crosses another real live boundary: it no longer stops at the event
image or the work-item execution rules. `sgx_ta_op` now owns the public
issue-vs-transition request boundary above that work item too.

The next useful step is the first explicit SGX sequence, scheduler, or
microkernel-facing work queue object above `sgx_ta_op`.
