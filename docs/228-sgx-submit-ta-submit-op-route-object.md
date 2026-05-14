# SGX Submit TA Submit-Op Route Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 7698778D2FCDF9EDC3DE460`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_submit_op_entrypoint`

That layer unified the outer issue-versus-transition submit-op entry seam, but
the common route that hands one selected request through the shared execute
path still sat outside the top SGX-shaped object in the submit-op wrappers.
This batch lifts that outer route seam into one explicit
`sgx_ta_submit_op_route` object above `sgx_ta_submit_op_entrypoint`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_route`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_submit_op_route_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_submit_op_route_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_route_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_route()`

The new `sgx_ta_submit_op_route` object now keeps together:

- the existing `sgx_ta_submit_op_entrypoint`
- the copied caller-visible submit-op request image:
  - `request_op`
- the copied caller-visible submit-op response image:
  - `response_op`
- the remaining top route boundary:
  - `active`
  - `dependency_fields_present`
  - `use_focus_request`
  - `focus_valid`
  - `focus_resolved`
  - `issue_request`
  - `transition_request`
  - `issue_entry`
  - `transition_entry`
  - `issue_call`
  - `transition_call`
  - `issue_entrypoint`
  - `transition_entrypoint`
  - `issue_route`
  - `transition_route`
  - `submit_id`
  - `requested_submit_id`
  - `selected_submit_id`
  - `resolved_submit_id`
  - `ioctl_op`
  - `entry_op`
  - `call_op`
  - `entrypoint_op`
  - `route_op`
  - `op_kind`

That moves the shared live SGX top seam one level higher again:

- submit-op ioctl path: `sgx_ta_submit_op_route`
- `issue_submit()` compatibility path: `sgx_ta_submit_op_route`
- `transition_submit()` compatibility path: `sgx_ta_submit_op_route`

The old submit-op-entrypoint execute helper is now gone. The shared wrappers
now build, validate, apply, and return one top route object whose nested
entrypoint layer still owns the outer issue-versus-transition seam, while the
new top layer owns the final selected-route boundary itself and the copied
`response_op.resolved_submit_id` return path.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- first guarded deploy wrapper that exposed the stale local deploy artifact
  path:
  [logs/vaio-plan/20260514-203042-sgx-ta-submit-op-route-deploy](/root/Poulsbo/logs/vaio-plan/20260514-203042-sgx-ta-submit-op-route-deploy)
- rebuilt local deploy artifact and final guarded rerun:
  [logs/vaio-plan/20260514-203347-sgx-ta-submit-op-route-rerun-deploy](/root/Poulsbo/logs/vaio-plan/20260514-203347-sgx-ta-submit-op-route-rerun-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-203656-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-203656-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-203809-sgx-submit-ta-submit-op-route-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-203809-sgx-submit-ta-submit-op-route-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-203853-sgx-multi-submit-ta-submit-op-route-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-203853-sgx-multi-submit-ta-submit-op-route-live/summary.log)

Final live module:

- `driver_srcversion=7698778D2FCDF9EDC3DE460`
- `override_module_size=496000`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=7698778D2FCDF9EDC3DE460`
- focused typed submit stayed terminally clean:
  - `submit_resolved_id=6`
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
    - `dependency[1]_observed_complete_ticket=6`

## Conclusion

`sgx_ta_submit_op_route` is now the validated live top submit-op route object
above `sgx_ta_submit_op_entrypoint`. The final module survives guarded reboot,
exact `complete`, focused submit, and typed multi-submit on hardware, while
keeping the VAIO headless and remotely usable.
