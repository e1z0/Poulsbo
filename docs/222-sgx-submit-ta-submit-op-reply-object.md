# SGX Submit TA Submit-Op Reply Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion A417E135FB014B70E256095`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_request_work_response`

That layer unified the returned request/work response boundary, but the live
execute path still returned a nested SGX response object while the caller-
visible ioctl reply image was reconstructed outside that top layer. This batch
lifts that remaining submit-op reply seam into one explicit
`sgx_ta_submit_op_reply` object above `sgx_ta_request_work_response`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_reply`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_submit_op_reply_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_submit_op_reply_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_submit_op_reply_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_reply()`

The new `sgx_ta_submit_op_reply` object now keeps together:

- the existing `sgx_ta_request_work_response`
- the caller-visible submit-op reply image:
  - `response_op`
- the remaining top reply boundary:
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

- submit-op ioctl path compatibility image: `sgx_ta_submit_op_reply`
- `issue_submit()` compatibility path: `sgx_ta_submit_op_reply`
- `transition_submit()` compatibility path: `sgx_ta_submit_op_reply`

The old standalone
`poulsbo_sgx_execute_submit_sgx_ta_request_work_response()` helper is now
replaced by the new submit-op-reply execute helper. The live execute path now
validates and applies one shared reply object whose `response_op` carries the
final `resolved_submit_id` exactly as the ioctl caller would see it.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy:
  [logs/vaio-plan/20260514-184208-sgx-plan-guarded-deploy/children/20260514-184208-guarded-deploy](/root/Poulsbo/logs/vaio-plan/20260514-184208-sgx-plan-guarded-deploy/children/20260514-184208-guarded-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-184356-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-184356-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-184506-sgx-submit-ta-submit-op-reply-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-184506-sgx-submit-ta-submit-op-reply-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-184547-sgx-multi-submit-ta-submit-op-reply-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-184547-sgx-multi-submit-ta-submit-op-reply-live/summary.log)

Final live module:

- `driver_srcversion=A417E135FB014B70E256095`
- `override_module_size=493616`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=A417E135FB014B70E256095`
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

`sgx_ta_submit_op_reply` is now the validated live top submit-op reply object
above `sgx_ta_request_work_response`. The final module survives guarded reboot,
exact `complete`, focused submit, and typed multi-submit on hardware, while
keeping the VAIO headless and remotely usable.
