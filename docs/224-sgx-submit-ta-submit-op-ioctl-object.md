# SGX Submit TA Submit-Op Ioctl Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 5884B548EB5E1D6C7698955`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_submit_op_control`

That layer unified the top submit-op control seam, but the full
`psb_ioctl_sgx_lab_submit_op()` request-and-reply boundary still sat outside
the top SGX-shaped object. This batch lifts that last outer ioctl seam into
one explicit `sgx_ta_submit_op_ioctl` object above `sgx_ta_submit_op_control`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_ioctl`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_submit_op_ioctl_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_submit_op_ioctl_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_ioctl_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_ioctl()`

The new `sgx_ta_submit_op_ioctl` object now keeps together:

- the existing `sgx_ta_submit_op_control`
- the copied caller-visible submit-op request image:
  - `request_op`
- the copied caller-visible submit-op response image:
  - `response_op`
- the remaining top ioctl boundary:
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
  - `ioctl_op`
  - `op_kind`

That moves the shared live SGX top seam one level higher again:

- submit-op ioctl control path: `sgx_ta_submit_op_ioctl`
- `issue_submit()` compatibility path: `sgx_ta_submit_op_ioctl`
- `transition_submit()` compatibility path: `sgx_ta_submit_op_ioctl`

The old standalone
`poulsbo_sgx_execute_submit_sgx_ta_submit_op_control()` helper is now
replaced by the new submit-op-ioctl execute helper. The live execute path now
validates and applies one shared ioctl object whose nested control still owns
the reply and control layers, while the top ioctl object now owns the copied
request image and copied response image for the full caller-visible
`psb_ioctl_sgx_lab_submit_op()` boundary.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy plus exact recovery wrapper:
  [logs/vaio-plan/20260514-191918-sgx-ta-submit-op-ioctl-deploy](/root/Poulsbo/logs/vaio-plan/20260514-191918-sgx-ta-submit-op-ioctl-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-192144-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-192144-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-192303-sgx-submit-ta-submit-op-ioctl-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-192303-sgx-submit-ta-submit-op-ioctl-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-192344-sgx-multi-submit-ta-submit-op-ioctl-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-192344-sgx-multi-submit-ta-submit-op-ioctl-live/summary.log)

Final live module:

- `driver_srcversion=5884B548EB5E1D6C7698955`
- `override_module_size=494168`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=5884B548EB5E1D6C7698955`
- focused typed submit stayed terminally clean:
  - `submit_resolved_id=3`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- typed multi-submit also stayed clean:
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
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=6`

## Conclusion

`sgx_ta_submit_op_ioctl` is now the validated live top submit-op ioctl object
above `sgx_ta_submit_op_control`. The final module survives guarded reboot,
exact `complete`, focused submit, and typed multi-submit on hardware, while
keeping the VAIO headless and remotely usable.
