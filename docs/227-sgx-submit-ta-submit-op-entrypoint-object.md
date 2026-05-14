# SGX Submit TA Submit-Op Entrypoint Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion B03227CA0E30D87724E8B38`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_submit_op_call`

That layer unified the caller-facing call seam, but the top
issue-versus-transition submit-op entrypoint still sat outside the top
SGX-shaped object in the shared wrapper path. This batch lifts that outer
entrypoint seam into one explicit `sgx_ta_submit_op_entrypoint` object above
`sgx_ta_submit_op_call`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_entrypoint`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_submit_op_entrypoint_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_submit_op_entrypoint_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_entrypoint_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_entrypoint()`

The new `sgx_ta_submit_op_entrypoint` object now keeps together:

- the existing `sgx_ta_submit_op_call`
- the copied caller-visible submit-op request image:
  - `request_op`
- the copied caller-visible submit-op response image:
  - `response_op`
- the remaining top entrypoint boundary:
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
  - `submit_id`
  - `requested_submit_id`
  - `selected_submit_id`
  - `resolved_submit_id`
  - `ioctl_op`
  - `entry_op`
  - `call_op`
  - `entrypoint_op`
  - `op_kind`

That moves the shared live SGX top seam one level higher again:

- submit-op ioctl path: `sgx_ta_submit_op_entrypoint`
- `issue_submit()` compatibility path: `sgx_ta_submit_op_entrypoint`
- `transition_submit()` compatibility path: `sgx_ta_submit_op_entrypoint`

The old submit-op-call execute helper is now gone. The shared wrappers now
build, validate, apply, and return one top entrypoint object whose nested call
layer still owns the caller-visible call seam, while the new top layer owns
the final entrypoint split itself.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy plus exact recovery wrapper:
  [logs/vaio-plan/20260514-201736-sgx-ta-submit-op-entrypoint-deploy](/root/Poulsbo/logs/vaio-plan/20260514-201736-sgx-ta-submit-op-entrypoint-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-201933-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-201933-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-202053-sgx-submit-ta-submit-op-entrypoint-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-202053-sgx-submit-ta-submit-op-entrypoint-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-202206-sgx-multi-submit-ta-submit-op-entrypoint-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-202206-sgx-multi-submit-ta-submit-op-entrypoint-live/summary.log)

Final live module:

- `driver_srcversion=B03227CA0E30D87724E8B38`
- `override_module_size=495392`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=B03227CA0E30D87724E8B38`
- focused typed submit stayed terminally clean:
  - `submit_resolved_id=5`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- typed multi-submit also stayed clean:
  - `SUBMIT_A_ID=7`
  - `SUBMIT_B_ID=8`
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

`sgx_ta_submit_op_entrypoint` is now the validated live top submit-op
entrypoint object above `sgx_ta_submit_op_call`. The final module survives
guarded reboot, exact `complete`, focused submit, and typed multi-submit on
hardware, while keeping the VAIO headless and remotely usable.
