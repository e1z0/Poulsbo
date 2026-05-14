# SGX Submit TA Request Work-Packet Object

Date: 2026-05-14
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 090E915D8E9ED7786CCAD27`

## Goal

The validated live submit-op stack already had:

- `psb_sgx_lab_submit_sgx_ta_request_sequence`

That layer unified the selected request packet plus the request-side
refresh/count/ticket progression, but the live `issue` and `transition` entry
points still had to split their final logging and execution boundary between:

- `request_sequence`
- its nested `request_packet`

This batch lifts that seam into one explicit request-work-packet object above
`sgx_ta_request_sequence`.

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal live top object:

- `psb_sgx_lab_submit_sgx_ta_request_work_packet`

New shared helpers:

- `poulsbo_sgx_build_submit_sgx_ta_request_work_packet_locked()`
- `poulsbo_sgx_apply_submit_sgx_ta_request_work_packet_locked()`
- `poulsbo_sgx_validate_built_submit_sgx_ta_request_work_packet_locked()`
- `poulsbo_sgx_execute_submit_sgx_ta_request_work_packet()`

The new `sgx_ta_request_work_packet` object now keeps together:

- the existing `sgx_ta_request_sequence`
- the selected request-side public boundary:
  - `requested_submit_id`
  - `resolved_submit_id`
  - `ioctl_op`
  - `op_kind`
  - `owner_state`
  - `packet_count`
  - `dependency_fields_present`
  - `use_focus_request`
  - `focus_resolved`
- the selected request-side read/write/CCB/ticket view:
  - `selected_read_offset`
  - `selected_write_offset`
  - `selected_ccb_offset`
  - `selected_outstanding_packets`
  - `selected_wrapped`
  - `selected_seq`
  - `selected_cookie`
  - `epoch`
  - `global_issue_ticket`
  - `global_running_ticket`
  - `global_complete_ticket`
- the request-side progression result:
  - `refresh_views`
  - `increment_issue_count`
  - `increment_transition_count`
  - `update_last_epoch_jiffies`
  - `update_last_issue_jiffies`
  - `update_last_transition_jiffies`
  - `issue_advanced`
  - `running_advanced`
  - `complete_advanced`
  - `active_running_changed`
  - `next_issue_count`
  - `next_transition_count`
  - `next_state`

That moves the shared live SGX top seam one level higher again:

- `issue_submit()` compatibility path: `sgx_ta_request_work_packet`
- `transition_submit()` compatibility path: `sgx_ta_request_work_packet`
- submit-op ioctl path: `sgx_ta_request_work_packet`

The old standalone `poulsbo_sgx_execute_submit_sgx_ta_request_sequence()`
helper is removed. The live execute path is now owned entirely by the new
work-packet helper.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
- `./scripts/docker-build-vaio-gma500.sh`

Hardware proof set:

- guarded deploy:
  [logs/vaio-deploy/20260514-113632-sgx-ta-request-work-packet-deploy](/root/Poulsbo/logs/vaio-deploy/20260514-113632-sgx-ta-request-work-packet-deploy)
- exact no-redeploy `plan` proof:
  [logs/vaio-plan-session/20260514-113831-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260514-113831-sgx-plan-session)
- focused typed submit:
  [logs/vaio-ioctl/20260514-114042-sgx-submit-ta-request-work-packet-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-114042-sgx-submit-ta-request-work-packet-live/summary.log)
- typed multi-submit:
  [logs/vaio-ioctl/20260514-114119-sgx-multi-submit-ta-request-work-packet-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260514-114119-sgx-multi-submit-ta-request-work-packet-live/summary.log)

Final live module:

- `driver_srcversion=090E915D8E9ED7786CCAD27`
- `override_module_size=481988`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Focused Evidence

- the exact `complete` proof passed on the deployed module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=090E915D8E9ED7786CCAD27`
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
  - submit A final:
    - `submit_resolved_id=4`
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_resolved_id=5`
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

`sgx_ta_request_work_packet` is now the validated live top request/work object
above `sgx_ta_request_sequence`. The new module survives guarded reboot, exact
`complete`, focused submit, and typed multi-submit on hardware, while keeping
the VAIO headless and remotely usable.
