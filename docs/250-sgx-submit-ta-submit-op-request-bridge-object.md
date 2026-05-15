# SGX Submit TA Submit-Op Request Bridge Object

Date: 2026-05-15
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion A8034211F7CCD771E65D255`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_portal`

The first wider `request_bridge` draft tried to lift the next full top
submit-op request seam above `request_portal`, but that version failed guarded
deploy and left the VAIO with SSH down. This batch narrows the lift to the
outer request-wrapper seam only, keeping `psb_ioctl_sgx_lab_submit_op()` on
the already validated `request_portal` path while moving the compatibility
wrapper entrypoints onto one explicit top `request_bridge` object.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_bridge`

New shared helper:

- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_bridge()`

The new `sgx_ta_submit_op_request_bridge` object now keeps together:

- the existing `sgx_ta_submit_op_request_portal`
- the copied caller-visible submit-op request image:
  - `request_op`
- the copied caller-visible submit-op response image:
  - `response_op`
- the copied final effect-visible request/work outcome:
  - `outcome`
- the narrowed top request-bridge boundary:
  - `active`
  - `dependency_fields_present`
  - `issue_request_bridge`
  - `transition_request_bridge`
  - `resolved_submit_id`
  - `request_bridge_op`

That means the shared live boundary now changes only for the outer wrapper
entrypoints:

- submit-op ioctl path: still `sgx_ta_submit_op_request_portal`
- `issue_submit()` compatibility path: now `sgx_ta_submit_op_request_bridge`
- `transition_submit()` compatibility path:
  now `sgx_ta_submit_op_request_bridge`

So the narrowed rerun proves the outer wrapper seam can be lifted cleanly
without also changing the already validated ioctl-side top boundary.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`
- `modinfo -F srcversion build/devnull-vaio-gma500-src/gma500_gfx.ko`

Hardware proof set:

- guarded deploy wrapper:
  [logs/vaio-plan/20260515-112156-sgx-ta-submit-op-request-bridge-rerun-deploy](/root/Poulsbo/logs/vaio-plan/20260515-112156-sgx-ta-submit-op-request-bridge-rerun-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260515-112805-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260515-112805-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260515-113121-sgx-submit-ta-submit-op-request-bridge-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260515-113121-sgx-submit-ta-submit-op-request-bridge-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260515-113121-sgx-multi-submit-ta-submit-op-request-bridge-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260515-113121-sgx-multi-submit-ta-submit-op-request-bridge-live/summary.log)

Final live module:

- `driver_srcversion=A8034211F7CCD771E65D255`
- `override_module_size=508140`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` recovery proof passed on the deployed module:
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=A8034211F7CCD771E65D255`
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
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=6`

## Conclusion

`sgx_ta_submit_op_request_bridge` is now the validated live top outer-wrapper
submit-op request object above `sgx_ta_submit_op_request_portal`. The narrowed
rerun survives guarded reboot, exact `complete`, focused submit, and typed
multi-submit on hardware, while keeping the ioctl-side top boundary pinned to
the already validated `request_portal` path and leaving the VAIO headless and
remotely usable.
