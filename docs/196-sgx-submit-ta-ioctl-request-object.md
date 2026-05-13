# SGX Submit TA Ioctl Request Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion CAB474CD8A51DA1D0BE821D`

## Goal

The validated SGX-shaped submit stack already had a shared top request object:

- `psb_sgx_lab_submit_sgx_ta_request`

That layer unified the typed submit contract and the public request semantics,
but one real top seam still remained open-coded:

- the ioctl-facing request wrapper:
  - raw ioctl op
  - requested vs resolved submit ID
  - whether dependency fields were actually present in the public request
- the mapping from that public ioctl seam onto the internal TA request op kind

This batch lifts that seam into one explicit top object above `sgx_ta_request`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal top object:

- `psb_sgx_lab_submit_sgx_ta_ioctl_request`

New shared helpers:

- `poulsbo_sgx_map_submit_sgx_ta_ioctl_op_kind()`
- `poulsbo_sgx_build_submit_sgx_ta_ioctl_request_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_ioctl_request_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_ioctl_request_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_ioctl_request()`
- `poulsbo_sgx_issue_submit_op_request()`
- `poulsbo_sgx_transition_submit_op_request()`

The new `sgx_ta_ioctl_request` object now keeps together:

- the existing `sgx_ta_request`
- the public ioctl boundary:
  - `ioctl_op`
  - `requested_submit_id`
  - `resolved_submit_id`
  - `dependency_fields_present`
  - `focus_resolved`
- the seam state copied out for top validation:
  - `op_kind`
  - `owner_state`
  - `packet_count`
  - `active`

That moves the live SGX top seam one level higher again:

- validator: `sgx_ta_ioctl_request`
- `issue_submit()` compatibility path: `sgx_ta_ioctl_request`
- `transition_submit()` compatibility path: `sgx_ta_ioctl_request`
- submit-op ioctl path: `sgx_ta_ioctl_request`

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

First live draft:

- guarded deploy:
  [logs/vaio-deploy/20260513-212418-sgx-ta-ioctl-request-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-212418-sgx-ta-ioctl-request-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-212617-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-212617-sgx-plan-session)

That first draft booted and stayed remotely usable, but the exact `complete`
proof failed at `issue` with `submit_op_ret=-1 errno=22`. The bad assumption
was in the new top validator: it re-ran the deeper built-request validator at
the wrong seam. The fix was to keep `sgx_ta_ioctl_request` validation at the
public ioctl seam only, and not treat deeper internal launch-time state as part
of the ioctl contract itself.

Canonical fixed proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-213143-sgx-ta-ioctl-request-rerun-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-213143-sgx-ta-ioctl-request-rerun-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-213332-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-213332-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-213452-sgx-submit-ta-ioctl-request-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-213452-sgx-submit-ta-ioctl-request-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-213532-sgx-multi-submit-ta-ioctl-request-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-213532-sgx-multi-submit-ta-ioctl-request-live/summary.log)

Final live module:

- `driver_srcversion=CAB474CD8A51DA1D0BE821D`
- `override_module_size=471808`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the fixed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=CAB474CD8A51DA1D0BE821D`
- focused typed submit stayed terminally clean:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`

## Typed Multi-Submit Evidence

- final typed multi-submit state also retired cleanly:
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`

## Result

The SGX-shaped kernel-internal runtime stack now has twenty-nine validated
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
- `psb_sgx_lab_submit_sgx_ta_ioctl_request`

This is still lab-mode execution, not real SGX rendering, but the shared top
layer now owns the public ioctl request seam too, and the first bad draft was
caught by the exact `complete` proof before it could be misclassified as a
deeper scheduler or queue regression.

The next useful step is the first explicit SGX microkernel-facing request
packet, scheduler queue item, or real work-queue entry above
`sgx_ta_ioctl_request`.
