# SGX Submit TA Submit Op Request Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 32C3455CEA624602FC2F106`

## Goal

The validated SGX-shaped submit stack already had a shared ioctl-facing top
object:

- `psb_sgx_lab_submit_sgx_ta_ioctl_request`

That layer unified the typed request boundary with the public ioctl seam, but
one higher public seam still remained split across callers:

- the copied raw `drm_psb_sgx_lab_submit_op` packet itself
- the normalized operation kind derived from that packet
- the final resolved-submit seam above the nested ioctl request object
- the top apply path that still treated the raw packet and the nested request
  as separate responsibilities

This batch lifts that last public submit-op seam into one explicit top object
above `sgx_ta_ioctl_request`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_submit_op_request_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_submit_op_request_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_request_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request()`
- `poulsbo_sgx_issue_submit_op_request()`
- `poulsbo_sgx_transition_submit_op_request()`

The new `sgx_ta_submit_op_request` object now keeps together:

- the existing `sgx_ta_ioctl_request`
- the copied public submit-op packet:
  - `op`
  - `op_kind`
- the top resolution seam:
  - `resolved_submit_id`
  - `focus_resolved`
  - `use_focus_request`
  - `dependency_fields_present`
- the top execution request split:
  - `issue_request`
  - `transition_request`
  - `active`

That moves the live SGX top seam one level higher again:

- validator: `sgx_ta_submit_op_request`
- `issue_submit()` compatibility path: `sgx_ta_submit_op_request`
- `transition_submit()` compatibility path: `sgx_ta_submit_op_request`
- submit-op ioctl path: `sgx_ta_submit_op_request`

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-215042-sgx-ta-submit-op-request-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-215042-sgx-ta-submit-op-request-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-215228-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-215228-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-215330-sgx-submit-ta-submit-op-request-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-215330-sgx-submit-ta-submit-op-request-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-215411-sgx-multi-submit-ta-submit-op-request-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-215411-sgx-multi-submit-ta-submit-op-request-live/summary.log)

Final live module:

- `driver_srcversion=32C3455CEA624602FC2F106`
- `override_module_size=472188`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=32C3455CEA624602FC2F106`
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

The SGX-shaped kernel-internal runtime stack now has thirty validated layers:

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
- `psb_sgx_lab_submit_sgx_ta_submit_op_request`

This is still lab-mode execution, not real SGX rendering, but the shared top
layer now owns the full copied public submit-op packet and resolved-submit seam
too, so the ioctl entrypoints and the compatibility mutation entrypoints now
meet at one stricter typed top object instead of splitting that public packet
contract across the wrapper and nested request layers.

The next useful step is the first explicit SGX microkernel-facing request
packet, scheduler queue item, or real work-queue entry above
`sgx_ta_submit_op_request`.
