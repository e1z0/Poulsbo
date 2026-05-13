# SGX Submit TA Path Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion A84CAC0D63EBA733BE134C6`

## Goal

The validated SGX-shaped submit stack already had an explicit launch-side top
object:

- `psb_sgx_lab_submit_sgx_ta_dispatch`

That still left one important split:

- the top submit validator stopped at `sgx_ta_dispatch`
- `issue_submit()` consumed the typed dispatch image
- `transition_submit()` still rebuilt the queue-header, CCB, sync, and owner
  writeback image inline from scalar transition state

This batch adds one higher object:

- `psb_sgx_lab_submit_sgx_ta_path`

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal objects:

- `struct psb_sgx_lab_submit_sgx_transition_image`
- `struct psb_sgx_lab_submit_sgx_ta_path`

New internal builders/helpers:

- `poulsbo_sgx_build_submit_sgx_ta_path_locked()`
- `poulsbo_sgx_submit_sgx_ta_path_transition_status_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_path_locked()`
- `poulsbo_sgx_validate_submit_sgx_ta_path_locked()`

The new `sgx_ta_path` object builds above `sgx_ta_dispatch` and makes the
transition-side writeback image explicit:

- the final queue-header image for the transition snapshot
- the transition CCB record image
- the transition sync image
- the transition owner image
- the transition queue/sync/owner region bounds

The top validator now moves from `sgx_ta_dispatch` to `sgx_ta_path`, and the
live `transition_submit()` path now consumes that typed transition image
instead of rebuilding queue/sync/owner/CCB writeback through open-coded local
state.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical live proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-164652-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-164652-guarded-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-164848-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-164848-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-165147-sgx-submit-ta-path-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-165147-sgx-submit-ta-path-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-165002-sgx-multi-submit-ta-path-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-165002-sgx-multi-submit-ta-path-live/summary.log)

Final live module:

- `driver_srcversion=A84CAC0D63EBA733BE134C6`
- `override_module_size=462848`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed again:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=A84CAC0D63EBA733BE134C6`
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

- submit B still remains blocked-but-valid before dependency release:
  - `submit_dependency_active_count=1`
  - `dependency[1]_required_ticket=3`
  - `dependency[1]_observed_complete_ticket=0`
- submit B later observes dependency completion and still remains valid:
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

The SGX-shaped kernel-internal runtime stack now has nineteen validated
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

This is still lab-mode execution, not real SGX rendering, but the transition
side now uses the same typed top-layer path model as the rest of the SGX
shaped submit stack instead of separate live-path bookkeeping.

The next useful step is the next more microkernel-facing SGX scheduling or
dispatch object above this repaired `sgx_ta_path` layer.
