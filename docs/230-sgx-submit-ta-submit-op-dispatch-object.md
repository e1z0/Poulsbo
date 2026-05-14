# SGX Submit TA Submit-Op Dispatch Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion DC5593F1A1D8FF7ED0012E8`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_submit_op_result`

That layer unified the copied request/response/outcome seam, but the shared
issue-versus-transition dispatch boundary still sat open-coded in the
compatibility wrappers. This batch lifts that outer dispatch seam into one
explicit `sgx_ta_submit_op_dispatch` object above `sgx_ta_submit_op_result`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_dispatch`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_submit_op_dispatch_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_submit_op_dispatch_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_dispatch_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_dispatch()`

The new `sgx_ta_submit_op_dispatch` object now keeps together:

- the existing `sgx_ta_submit_op_result`
- the copied caller-visible submit-op request image:
  - `request_op`
- the copied caller-visible submit-op response image:
  - `response_op`
- the copied final effect-visible request/work outcome:
  - `outcome`
- the remaining top dispatch boundary:
  - `active`
  - `dependency_fields_present`
  - `issue_dispatch`
  - `transition_dispatch`
  - `resolved_submit_id`
  - `dispatch_op`

That moves the shared live SGX top seam one level higher again:

- submit-op ioctl path: `sgx_ta_submit_op_dispatch`
- `issue_submit()` compatibility path: `sgx_ta_submit_op_dispatch`
- `transition_submit()` compatibility path: `sgx_ta_submit_op_dispatch`

The old submit-op-result execute helper is now gone. The shared wrappers now
build, validate, apply, and return one top dispatch object whose nested result
layer still owns the copied request/response/outcome seam, while the new top
layer owns the final issue-versus-transition dispatch boundary itself.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy wrapper:
  [logs/vaio-plan/20260514-212415-sgx-ta-submit-op-dispatch-deploy](/root/Poulsbo/logs/vaio-plan/20260514-212415-sgx-ta-submit-op-dispatch-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-212600-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-212600-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-212715-sgx-submit-ta-submit-op-dispatch-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-212715-sgx-submit-ta-submit-op-dispatch-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-212758-sgx-multi-submit-ta-submit-op-dispatch-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-212758-sgx-multi-submit-ta-submit-op-dispatch-live/summary.log)

Final live module:

- `driver_srcversion=DC5593F1A1D8FF7ED0012E8`
- `override_module_size=497712`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=DC5593F1A1D8FF7ED0012E8`
- focused typed submit stayed terminally clean:
  - `submit_resolved_id=4`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- typed multi-submit also stayed clean:
  - `SUBMIT_A_ID=6`
  - `SUBMIT_B_ID=7`
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

## Conclusion

`sgx_ta_submit_op_dispatch` is now the validated live top submit-op dispatch
object above `sgx_ta_submit_op_result`. The final module survives guarded
reboot, exact `complete`, focused submit, and typed multi-submit on hardware,
while keeping the VAIO headless and remotely usable.
