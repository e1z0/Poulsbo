# SGX Submit TA Submit-Op Entry Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 12046BB01C587A8CAAB2BE3`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_submit_op_ioctl`

That layer unified the copied request-and-reply ioctl image, but the common
issue-versus-transition entry boundary still sat outside the top SGX-shaped
object. This batch lifts that outer compatibility entry seam into one
explicit `sgx_ta_submit_op_entry` object above `sgx_ta_submit_op_ioctl`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_entry`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_submit_op_entry_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_submit_op_entry_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_entry_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_entry()`

The new `sgx_ta_submit_op_entry` object now keeps together:

- the existing `sgx_ta_submit_op_ioctl`
- the copied caller-visible submit-op request image:
  - `request_op`
- the copied caller-visible submit-op response image:
  - `response_op`
- the remaining top entry boundary:
  - `active`
  - `dependency_fields_present`
  - `use_focus_request`
  - `focus_valid`
  - `focus_resolved`
  - `issue_request`
  - `transition_request`
  - `issue_entry`
  - `transition_entry`
  - `submit_id`
  - `requested_submit_id`
  - `selected_submit_id`
  - `resolved_submit_id`
  - `ioctl_op`
  - `entry_op`
  - `op_kind`

That moves the shared live SGX top seam one level higher again:

- submit-op ioctl path: `sgx_ta_submit_op_entry`
- `issue_submit()` compatibility path: `sgx_ta_submit_op_entry`
- `transition_submit()` compatibility path: `sgx_ta_submit_op_entry`

The old submit-op-ioctl execute helper is now gone. The live execute path now
validates and applies one shared top entry object whose nested ioctl layer
still owns the full copied request/reply image, while the new top layer owns
the final issue-versus-transition entry split itself.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy plus exact recovery wrapper:
  [logs/vaio-plan/20260514-194105-sgx-ta-submit-op-entry-deploy](/root/Poulsbo/logs/vaio-plan/20260514-194105-sgx-ta-submit-op-entry-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-194317-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-194317-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-194443-sgx-submit-ta-submit-op-entry-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-194443-sgx-submit-ta-submit-op-entry-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-194541-sgx-multi-submit-ta-submit-op-entry-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-194541-sgx-multi-submit-ta-submit-op-entry-live/summary.log)

Final live module:

- `driver_srcversion=12046BB01C587A8CAAB2BE3`
- `override_module_size=494580`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=12046BB01C587A8CAAB2BE3`
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

`sgx_ta_submit_op_entry` is now the validated live top submit-op entry object
above `sgx_ta_submit_op_ioctl`. The final module survives guarded reboot,
exact `complete`, focused submit, and typed multi-submit on hardware, while
keeping the VAIO headless and remotely usable.
