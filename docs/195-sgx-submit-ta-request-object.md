# SGX Submit TA Request Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 02F0B8378744F4DA3D89A13`

## Goal

The validated SGX-shaped submit stack already had a shared top job object:

- `psb_sgx_lab_submit_sgx_ta_job`

That layer unified the full typed submit contract and dependency summary, but
one top live seam still remained open-coded:

- the public request itself:
  - issue vs transition
  - requested owner state
  - requested packet count
- the top execution-effect summary:
  - whether the request is issue-active or transition-active
  - whether it refreshes views
  - whether it increments issue/transition counts
  - which scheduler edge advances next

were still spread between the callers and the nested job builder.

This batch adds that explicit request layer above `sgx_ta_job`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal top object:

- `psb_sgx_lab_submit_sgx_ta_request`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_request_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_request_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_request()`
- `poulsbo_sgx_validate_submit_sgx_ta_request_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_request_locked()`

The new `sgx_ta_request` object now keeps together:

- the existing `sgx_ta_job`
- the public request boundary:
  - `submit_id`
  - `op_kind`
  - `owner_state`
  - `packet_count`
- the top execution-effect summary:
  - `issue_active`
  - `transition_active`
  - `refresh_views`
  - `increment_issue_count`
  - `increment_transition_count`
  - `issue_advanced`
  - `running_advanced`
  - `complete_advanced`
  - `active_running_changed`
  - `next_state`

That moves the top live SGX path one level higher again:

- validator: `sgx_ta_request`
- `issue_submit()`: `sgx_ta_request`
- `transition_submit()`: `sgx_ta_request`

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical live proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-203614-sgx-ta-request-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-203614-sgx-ta-request-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-203802-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-203802-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-203935-sgx-submit-ta-request-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-203935-sgx-submit-ta-request-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-204040-sgx-multi-submit-ta-request-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-204040-sgx-multi-submit-ta-request-live/summary.log)

Final live module:

- `driver_srcversion=02F0B8378744F4DA3D89A13`
- `override_module_size=470992`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed again:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=02F0B8378744F4DA3D89A13`
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

- final typed multi-submit state retired cleanly:
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
    - `dependency[1]_observed_complete_ticket=5`

## Result

The SGX-shaped kernel-internal runtime stack now has twenty-eight validated
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
- `psb_sgx_lab_submit_sgx_ta_scheduler`
- `psb_sgx_lab_submit_sgx_ta_sequence`
- `psb_sgx_lab_submit_sgx_ta_work_queue`
- `psb_sgx_lab_submit_sgx_ta_job`
- `psb_sgx_lab_submit_sgx_ta_request`

This is still lab-mode execution, not real SGX rendering, but the shared top
layer now owns the public request boundary and the top execution-effect summary
above the typed submit contract.

The next useful step is the first explicit SGX scheduler-facing queue item,
microkernel-facing request packet, or real work-queue object above
`sgx_ta_request`.
