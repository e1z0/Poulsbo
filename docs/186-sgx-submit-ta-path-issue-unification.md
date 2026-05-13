# SGX Submit TA Path Issue Unification

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 5B50AE965337A8FBA71E50E`

## Goal

The validated SGX-shaped submit stack already had one shared top object for the
transition-side live path:

- `psb_sgx_lab_submit_sgx_ta_path`

But one split still remained:

- the top validator stopped at `sgx_ta_path`
- `transition_submit()` consumed `sgx_ta_path`
- `issue_submit()` still built only `sgx_ta_dispatch`

This batch does not add a new layer. It finishes the live-path unification so
both live mutation entrypoints now consume the same top typed path object.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal helper:

- `poulsbo_sgx_submit_sgx_ta_path_issue_status_locked()`

Live-path change:

- `issue_submit()` now builds `psb_sgx_lab_submit_sgx_ta_path`
- it now validates issue readiness through `sgx_ta_path`
- it now consumes `path->dispatch.issue_image`
  instead of a separately built `sgx_ta_dispatch`

That means the live top-layer SGX path is now consistent:

- validator: `sgx_ta_path`
- issue path: `sgx_ta_path`
- transition path: `sgx_ta_path`

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical live proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-170524-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-170524-guarded-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-170719-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-170719-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-170719-sgx-submit-ta-path-issue-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-170719-sgx-submit-ta-path-issue-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-170719-sgx-multi-submit-ta-path-issue-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-170719-sgx-multi-submit-ta-path-issue-live/summary.log)

Final live module:

- `driver_srcversion=5B50AE965337A8FBA71E50E`
- `override_module_size=462900`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed again:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=5B50AE965337A8FBA71E50E`
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
  - `dependency[1]_observed_complete_ticket=4`
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
    - `dependency[1]_observed_complete_ticket=4`

## Result

The SGX-shaped kernel-internal runtime stack still has nineteen validated
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

This is still lab-mode execution, not real SGX rendering, but the live top
layer is now internally consistent: steady-state validation, `issue_submit()`,
and `transition_submit()` all route through the same `sgx_ta_path` object.

The next useful step is the first explicit SGX apply or commit object above
`sgx_ta_path`, so the typed path stops at describing the mutation and starts
owning the actual object-write and submit-state update plan too.
