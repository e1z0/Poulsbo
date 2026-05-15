# SGX Submit TA Submit-Op Request Relay Object

Date: 2026-05-15
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 1E8624FB85488982E4A60C3`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_bridge`
- `psb_sgx_lab_submit_sgx_ta_submit_op_request_portal`

The narrowed `request_bridge` rerun proved that the outer wrapper seam could be
lifted cleanly while keeping `psb_ioctl_sgx_lab_submit_op()` on the already
validated `request_portal` path. This batch lifts the ioctl-side
`ISSUE` / `TRANSITION` caller-visible return seam into a dedicated
`request_relay` object, while keeping the outer wrapper seam on
`request_bridge`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_relay`

New shared helper:

- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_relay()`

The new `sgx_ta_submit_op_request_relay` object now keeps together:

- the existing `sgx_ta_submit_op_request_portal`
- the copied caller-visible submit-op request image:
  - `request_op`
- the copied caller-visible submit-op response image:
  - `response_op`
- the copied final effect-visible request/work outcome:
  - `outcome`
- the narrowed ioctl-side request-relay boundary:
  - `active`
  - `dependency_fields_present`
  - `issue_request_relay`
  - `transition_request_relay`
  - `resolved_submit_id`
  - `request_relay_op`

That means the shared live boundary is now split cleanly:

- submit-op ioctl path:
  - `ISSUE` / `TRANSITION` now use `sgx_ta_submit_op_request_relay`
- `issue_submit()` compatibility path:
  - still uses `sgx_ta_submit_op_request_bridge`
- `transition_submit()` compatibility path:
  - still uses `sgx_ta_submit_op_request_bridge`

So the validated live top pair is now:

- `request_bridge` for the outer wrapper seam
- `request_relay` for the ioctl-side return seam

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy wrapper:
  [logs/vaio-plan/20260515-131136-sgx-ta-submit-op-request-relay-deploy](/root/Poulsbo/logs/vaio-plan/20260515-131136-sgx-ta-submit-op-request-relay-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260515-131351-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260515-131351-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260515-131821-sgx-submit-ta-submit-op-request-relay-live-full/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260515-131821-sgx-submit-ta-submit-op-request-relay-live-full/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260515-131508-sgx-multi-submit-ta-submit-op-request-relay-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260515-131508-sgx-multi-submit-ta-submit-op-request-relay-live/summary.log)

Final live module:

- `driver_srcversion=1E8624FB85488982E4A60C3`
- `override_module_size=508660`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` recovery proof passed on the deployed module:
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=1E8624FB85488982E4A60C3`
- focused typed submit stayed terminally clean:
  - `submit_resolved_id=11`
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- typed multi-submit also stayed clean:
  - `SUBMIT_A_ID=5`
  - `SUBMIT_B_ID=6`
  - submit A final:
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=4`

## Conclusion

`sgx_ta_submit_op_request_relay` is now the validated live top ioctl-side
submit-op request return object above `sgx_ta_submit_op_request_portal`, while
`sgx_ta_submit_op_request_bridge` remains the validated outer wrapper seam.
The split top pair survives guarded reboot, exact `complete`, focused submit,
and typed multi-submit on hardware, and leaves the VAIO headless and remotely
usable.
