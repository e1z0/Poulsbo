# SGX Submit TA Submit-Op Control Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 3052308F8E83D766FBBE045`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_submit_op_reply`

That layer unified the caller-visible submit-op reply image, but the shared
execute path still kept the remaining ioctl control boundary outside the top
SGX-shaped object. This batch lifts that last submit-op control seam into one
explicit `sgx_ta_submit_op_control` object above `sgx_ta_submit_op_reply`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_control`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_submit_op_control_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_submit_op_control_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_control_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_control()`

The new `sgx_ta_submit_op_control` object now keeps together:

- the existing `sgx_ta_submit_op_reply`
- the copied caller-visible submit-op request image:
  - `ioctl_op`
- the remaining top control boundary:
  - `active`
  - `dependency_fields_present`
  - `use_focus_request`
  - `focus_valid`
  - `focus_resolved`
  - `issue_request`
  - `transition_request`
  - `submit_id`
  - `requested_submit_id`
  - `selected_submit_id`
  - `resolved_submit_id`
  - `op_kind`

That moves the shared live SGX top seam one level higher again:

- submit-op ioctl control path: `sgx_ta_submit_op_control`
- `issue_submit()` compatibility path: `sgx_ta_submit_op_control`
- `transition_submit()` compatibility path: `sgx_ta_submit_op_control`

The old standalone
`poulsbo_sgx_execute_submit_sgx_ta_submit_op_reply()` helper is now replaced
by the new submit-op-control execute helper. The live execute path now
validates and applies one shared control object whose nested reply still owns
the caller-visible response image, while the top control object now owns the
copied request image and final `resolved_submit_id` return boundary.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy plus exact recovery wrapper:
  [logs/vaio-plan/20260514-185945-sgx-ta-submit-op-control-deploy](/root/Poulsbo/logs/vaio-plan/20260514-185945-sgx-ta-submit-op-control-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-190601-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-190601-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-190712-sgx-submit-ta-submit-op-control-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-190712-sgx-submit-ta-submit-op-control-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-190759-sgx-multi-submit-ta-submit-op-control-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-190759-sgx-multi-submit-ta-submit-op-control-live/summary.log)

Final live module:

- `driver_srcversion=3052308F8E83D766FBBE045`
- `override_module_size=493892`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=3052308F8E83D766FBBE045`
- focused typed submit stayed terminally clean:
  - `submit_resolved_id=7`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- typed multi-submit also stayed clean:
  - `SUBMIT_A_ID=8`
  - `SUBMIT_B_ID=9`
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
    - `dependency[1]_observed_complete_ticket=13`

## Conclusion

`sgx_ta_submit_op_control` is now the validated live top submit-op control
object above `sgx_ta_submit_op_reply`. The final module survives guarded
reboot, exact `complete`, focused submit, and typed multi-submit on hardware,
while keeping the VAIO headless and remotely usable.
