# SGX Submit Queued TA Command Object

Date: 2026-05-13
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion 2C7400755C1D450933F7344`

## Goal

The internal SGX-shaped submit stack already had:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`
- `psb_sgx_lab_submit_ccb_kick`
- `psb_sgx_lab_submit_sgx_ccb_kick`
- `psb_sgx_lab_submit_sgx_cmdta_shared`
- `psb_sgx_lab_submit_sgx_cmdta`
- `psb_sgx_lab_submit_sgx_ta_kick`
- `psb_sgx_lab_submit_sgx_ta_command`

That still left the queue-placement side split across layers: the validated TA
command object did not yet carry the queue header image or the optional
launch-time `issue-plan` state as one explicit queued execution object. This
batch adds that next layer:

- `psb_sgx_lab_submit_sgx_queued_ta_command`

## What Changed

Kernel-side:

- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New internal object:

- `struct psb_sgx_lab_submit_sgx_queued_ta_command`

New internal builder:

- `poulsbo_sgx_build_submit_sgx_queued_ta_command_locked()`

The new `sgx_queued_ta_command` object consumes one normalized `ccb_kick`,
embeds the current `sgx_ta_command`, materializes the live queue-header words,
and copies the typed `issue-plan` image when the launch path has one. That
means one validated internal object now covers:

- the queued TA command image
- the queue header that points at that command
- the `queue_base_offset + read_offset == ccb_offset` placement relation
- the optional typed `issue-plan` state used by the launch-time path

The central submit validator now consumes `sgx_queued_ta_command` instead of
stopping at `sgx_ta_command`. That ties the validated TA command, queued CCB
payload, queue placement, and optional `issue-plan` state together as one
checked SGX-shaped execution object.

## Validation

Local checks:

- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- `./scripts/docker-build-vaio-gma500.sh`

First guarded deploy attempt:

- [logs/vaio-deploy/20260513-062738-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-062738-guarded-deploy)

That first deploy did not panic the kernel, but it did miss the default
rollback confirmation window. The target later proved remotely usable on the
new module during the pending-rollback boot, then the rollback service forced a
clean reboot back to the older confirmed override.

Successful guarded deploy rerun:

- [logs/vaio-deploy/20260513-070613-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-070613-guarded-deploy)

The successful rerun used:

- `ROLLBACK_GRACE_SECS=900`
- `POST_REBOOT_TIMEOUT_SECS=1800`

That rerun cleared the rollback guard on first healthy return and left the VAIO
on the new module:

- `driver_srcversion=2C7400755C1D450933F7344`
- `override_module_size=451936`
- `lightdm_active=inactive`
- `kdump_service_active=active`
- `overall=remote_usable`

Canonical live bundles:

- exact no-redeploy `plan` regression proof:
  - [logs/vaio-plan-session/20260513-070759-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-070759-sgx-plan-session)
- focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-070912-sgx-submit-queued-ta-command-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-070912-sgx-submit-queued-ta-command-live/summary.log)
- typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-070951-sgx-multi-submit-queued-ta-command-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-070951-sgx-multi-submit-queued-ta-command-live/summary.log)

## Focused Evidence

- the exact `complete` regression proof stayed terminally clean on the new
  module:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=2C7400755C1D450933F7344`
- focused typed submit still completed cleanly:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
- final focused execution-shaped views stayed coherent:
  - `submit_live_stage_name=retired`
  - `submit_result_stage_name=retired`
  - `submit_manifest_stage_name=retired`
  - `submit_descriptor_stage_name=retired`
  - `submit_plan_stage_name=retired`
  - `submit_issue_plan_stage_name=retired`
  - `submit_issue_plan_dependency_active_count=0`
  - final cleanup:
    - `submit_dependency_active_count=0`
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`

## Typed Multi-Submit Evidence

- submit A still finished terminally clean on the new module:
  - `SUBMIT_A_ID=3`
  - `submit_prepare_count=1`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_dispatch_count=3`
  - `submit_retired_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
  - `submit_dependency_active_count=0`
- submit B retained dependency state through the normalized queued TA-command
  line and still retired cleanly:
  - `SUBMIT_B_ID=4`
  - `submit_prepare_count=1`
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_dispatch_count=2`
  - `submit_retired_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
  - `submit_result_dependency_active_count=1`
  - `submit_manifest_dependency_active_count=1`
  - `submit_descriptor_dependency_active_count=1`
  - `submit_plan_dependency_active_count=1`
  - `submit_dependency_active_count=1`
- final cleanup returned to zero live SGX state:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

## Result

The SGX-shaped kernel-internal execution stack now has ten validated layers:

- `psb_sgx_lab_submit_exec_record`
- `psb_sgx_lab_submit_exec_packet`
- `psb_sgx_lab_submit_cmdta_kick`
- `psb_sgx_lab_submit_ccb_kick`
- `psb_sgx_lab_submit_sgx_ccb_kick`
- `psb_sgx_lab_submit_sgx_cmdta_shared`
- `psb_sgx_lab_submit_sgx_cmdta`
- `psb_sgx_lab_submit_sgx_ta_kick`
- `psb_sgx_lab_submit_sgx_ta_command`
- `psb_sgx_lab_submit_sgx_queued_ta_command`

This is still lab-model execution, not real SGX rendering, but it is now the
first full queued TA command object above `sgx_ta_command`. It keeps the
validated TA command words, the queued CCB payload, the queue header image, and
the optional `issue-plan` state in one explicit object on hardware. The next
useful step is the next microkernel-facing SGX execution object above this
`sgx_queued_ta_command` layer.
