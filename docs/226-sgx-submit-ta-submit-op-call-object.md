# SGX Submit TA Submit-Op Call Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion BB4FE7048611B98999236B9`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_submit_op_entry`

That layer unified the copied request-and-reply images and the common
issue-versus-transition entry seam, but the caller-facing submit-op call
boundary still sat outside the top SGX-shaped object. This batch lifts that
outer compatibility seam into one explicit `sgx_ta_submit_op_call` object
above `sgx_ta_submit_op_entry`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_call`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_submit_op_call_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_submit_op_call_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_call_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_call()`

The new `sgx_ta_submit_op_call` object now keeps together:

- the existing `sgx_ta_submit_op_entry`
- the copied caller-visible submit-op request image:
  - `request_op`
- the copied caller-visible submit-op response image:
  - `response_op`
- the remaining top call boundary:
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
  - `submit_id`
  - `requested_submit_id`
  - `selected_submit_id`
  - `resolved_submit_id`
  - `ioctl_op`
  - `entry_op`
  - `call_op`
  - `op_kind`

That moves the shared live SGX top seam one level higher again:

- submit-op ioctl path: `sgx_ta_submit_op_call`
- `issue_submit()` compatibility path: `sgx_ta_submit_op_call`
- `transition_submit()` compatibility path: `sgx_ta_submit_op_call`

The old submit-op-entry execute helper is now gone. The live execute path now
validates and applies one shared top call object whose nested entry layer still
owns the copied request/reply image plus the entry split, while the new top
layer owns the final caller-facing call seam itself.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy plus exact recovery wrapper:
  [logs/vaio-plan/20260514-195517-sgx-ta-submit-op-call-deploy](/root/Poulsbo/logs/vaio-plan/20260514-195517-sgx-ta-submit-op-call-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-195738-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-195738-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-195905-sgx-submit-ta-submit-op-call-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-195905-sgx-submit-ta-submit-op-call-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-200019-sgx-multi-submit-ta-submit-op-call-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-200019-sgx-multi-submit-ta-submit-op-call-live/summary.log)

Final live module:

- `driver_srcversion=BB4FE7048611B98999236B9`
- `override_module_size=495524`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=BB4FE7048611B98999236B9`
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

`sgx_ta_submit_op_call` is now the validated live top submit-op call object
above `sgx_ta_submit_op_entry`. The final module survives guarded reboot,
exact `complete`, focused submit, and typed multi-submit on hardware, while
keeping the VAIO headless and remotely usable.
