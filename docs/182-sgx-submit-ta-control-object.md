# SGX Submit TA Control Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion B77A6FEA8B9671946B32272`

## Goal

The SGX-shaped submit validator already had an explicit top runtime gate:

- `psb_sgx_lab_submit_sgx_ta_gate`

That still left one real control-layer split:

- the validator could prove runtime state and running-slot gating
- the live `issue` path still decided launch/emit coherence separately
- the live `transition -> running` path only consumed the gate side

This batch adds one higher object:

- `psb_sgx_lab_submit_sgx_ta_control`

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal objects:

- `struct psb_sgx_lab_submit_sgx_issue_request`
- `struct psb_sgx_lab_submit_sgx_ta_control`

New internal builders/helpers:

- `poulsbo_sgx_build_submit_sgx_ta_control_locked()`
- `poulsbo_sgx_submit_sgx_ta_control_running_status_locked()`
- `poulsbo_sgx_submit_sgx_ta_control_issue_status_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_control_locked()`
- `poulsbo_sgx_validate_submit_sgx_ta_control_locked()`

The older launch-match logic was also split into reusable pieces:

- `poulsbo_sgx_submit_launch_base_matches_locked()`
- `poulsbo_sgx_submit_issue_plan_matches_launch_locked()`

The new `sgx_ta_control` object builds above `sgx_ta_runtime` and
`sgx_ta_gate`, then makes the top execution-control states explicit:

- whether the current emit snapshot still matches live object identity
- whether a launch snapshot is present and still unconsumed
- whether the launch snapshot is still coherent with the current emit state
- whether the running slot is already owned by this submit
- whether a requested issue can proceed
- whether an issue request that targets `OWNER_RUNNING` also satisfies the
  running gate

The live `issue` path now consumes that typed control object instead of
rechecking launch/issue state through a separate narrow helper, and the live
`transition -> running` path now consumes the same top control layer instead
of only the lower gate.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Canonical live proof set:

- guarded deploy:
  [logs/vaio-deploy/20260513-153601-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-153601-guarded-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260513-153751-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-153751-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260513-153922-sgx-submit-ta-control-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-153922-sgx-submit-ta-control-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260513-154028-sgx-multi-submit-ta-control-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-154028-sgx-multi-submit-ta-control-live/summary.log)

Final live module:

- `driver_srcversion=B77A6FEA8B9671946B32272`
- `override_module_size=459916`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed again:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=B77A6FEA8B9671946B32272`
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

The SGX-shaped kernel-internal runtime stack now has sixteen validated layers:

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

This is still lab-mode execution, not real SGX rendering, but the top control
decision that governs both launch-time issue readiness and transition into
running is now one shared SGX-shaped object instead of split path-specific
logic.

The next useful step is the next more microkernel-facing SGX scheduling or
control object above this repaired `sgx_ta_control` layer.
