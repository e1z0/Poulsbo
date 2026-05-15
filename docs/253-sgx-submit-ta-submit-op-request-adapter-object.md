# SGX Submit TA Submit-Op Request Adapter Object

Date: 2026-05-15
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion F6713BCD2555003695EFC34`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_bridge`
- `psb_sgx_lab_submit_sgx_ta_submit_op_request_relay`
- `psb_sgx_lab_submit_sgx_ta_submit_op_request_terminal`
- `psb_sgx_lab_submit_sgx_ta_submit_op_request_portal`

The narrowed `request_bridge` rerun proved that the outer wrapper seam could be
lifted cleanly while keeping `psb_ioctl_sgx_lab_submit_op()` on the already
validated `request_portal` path. The later `request_relay` and
`request_terminal` batches split the ioctl-side copied return seam and final
caller seam above `request_portal`. This batch lifts the outer compatibility
caller seam into a dedicated `request_adapter` object, while keeping the
ioctl-side path on the already validated `request_terminal` split.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_adapter`

New shared helper:

- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_adapter()`

The new `sgx_ta_submit_op_request_adapter` object now keeps together:

- the existing `sgx_ta_submit_op_request_bridge`
- the copied caller-visible submit-op request image:
  - `request_op`
- the copied caller-visible submit-op response image:
  - `response_op`
- the copied final effect-visible request/work outcome:
  - `outcome`
- the narrowed outer request-adapter boundary:
  - `active`
  - `dependency_fields_present`
  - `issue_request_adapter`
  - `transition_request_adapter`
  - `resolved_submit_id`
  - `request_adapter_op`

That means the shared live top split is now:

- submit-op ioctl path:
  - still uses `sgx_ta_submit_op_request_terminal`
- copied ioctl-side request / response / outcome seam:
  - still uses `sgx_ta_submit_op_request_relay`
- `issue_submit()` compatibility path:
  - now uses `sgx_ta_submit_op_request_adapter`
- `transition_submit()` compatibility path:
  - now uses `sgx_ta_submit_op_request_adapter`

So the validated live split is now:

- `request_adapter` for the outer compatibility caller seam
- `request_bridge` for the outer copied wrapper seam
- `request_terminal` for the final ioctl-side caller seam
- `request_relay` for the copied ioctl-side return seam

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy wrapper:
  [logs/vaio-plan/20260515-143634-sgx-ta-submit-op-request-adapter-deploy](/root/Poulsbo/logs/vaio-plan/20260515-143634-sgx-ta-submit-op-request-adapter-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260515-143830-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260515-143830-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260515-143830-sgx-submit-ta-submit-op-request-adapter-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260515-143830-sgx-submit-ta-submit-op-request-adapter-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260515-143830-sgx-multi-submit-ta-submit-op-request-adapter-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260515-143830-sgx-multi-submit-ta-submit-op-request-adapter-live/summary.log)

Final live module:

- `driver_srcversion=F6713BCD2555003695EFC34`
- `override_module_size=508892`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` recovery proof passed on the deployed module:
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=F6713BCD2555003695EFC34`
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
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`

## Conclusion

`sgx_ta_submit_op_request_adapter` is now the validated live top outer
compatibility caller object above `sgx_ta_submit_op_request_bridge`, while the
ioctl-side split remains on `sgx_ta_submit_op_request_terminal` and
`sgx_ta_submit_op_request_relay`. The split top quartet survives guarded
reboot, exact `complete`, focused submit, and typed multi-submit on hardware,
and leaves the VAIO headless and remotely usable.
