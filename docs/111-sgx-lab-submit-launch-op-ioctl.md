# SGX Lab Submit Launch Op Ioctl

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion A6F49F33394FCA892D9A5B2`

## Goal

`prepare` gave the lab path one coherent backing image, `arm` froze one
pre-issue snapshot, and `emit` froze one execution-shaped record. One gap still
remained:

- there was no separate kernel-owned issue-plan checkpoint between `emit` and
  `issue`
- `issue` still had to recompute queue growth and ticket progression directly
  from the mutable live submit state
- there was no typed way to prove the exact launched issue plan before
  consuming it

This batch adds that checkpoint as typed `launch`.

## Changes

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New submit-op opcode:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_LAUNCH`

New kernel behavior:

- `launch` requires a current emitted submit image
- `launch` computes the exact issue plan that would be consumed by
  `issue owner_state packet_count`
- the plan is frozen into a persistent launch snapshot containing:
  - prepare/arm/emit ancestry
  - owner-state and issued-packet request
  - planned queue packet counts and wrapped write offset
  - planned sync pending sequence and flags
  - planned owner issue cookie
  - planned queue epoch
  - planned global issue ticket
  - active dependency count at launch time
  - resolved object IDs and GPU VAs
- `issue` now requires a valid, unconsumed launch snapshot
- `issue` consumes that snapshot instead of recomputing directly from arbitrary
  mutable submit state
- later submit reshaping still invalidates arm, emit, and launch state through
  the existing disarm path

New typed query fields:

- `launch_valid`
- `launch_consumed`
- `launch_count`
- `launch_prepare_count`
- `launch_arm_count`
- `launch_emit_count`
- `launch_context`
- `launch_cmd_type`
- `launch_owner_state`
- `launch_issued_packets`
- `launch_refilled_from_complete`
- `launch_ctrl_flags`
- `launch_num_ta_status_vals`
- `launch_num_3d_status_vals`
- `launch_ta_dependency`
- `launch_control_object_id`
- `launch_pds_object_id`
- `launch_use_object_id`
- `launch_target_object_id`
- `launch_status_object_id`
- `launch_ccb_offset`
- `launch_ccb_write_offset`
- `launch_ccb_read_offset`
- `launch_new_write_offset`
- `launch_ccb_service_address`
- `launch_ccb_cache_control`
- `launch_ccb_data0`
- `launch_ccb_data1`
- `launch_queue_base_offset`
- `launch_queue_size`
- `launch_queue_packet_size`
- `launch_queue_packet_count`
- `launch_queue_tail_packet_count`
- `launch_queue_head_packet_count`
- `launch_sync_offset`
- `launch_sync_complete_seq`
- `launch_sync_pending_seq`
- `launch_sync_flags`
- `launch_owner_offset`
- `launch_owner_issue_cookie`
- `launch_owner_complete_cookie`
- `launch_queue_epoch`
- `launch_global_issue_ticket`
- `launch_global_running_ticket`
- `launch_global_complete_ticket`
- `launch_dependency_active_count`
- `launch_control_gpu_va`
- `launch_pds_gpu_va`
- `launch_use_gpu_va`
- `launch_target_gpu_va`
- `launch_status_gpu_va`
- `last_launch_jiffies`

Userspace:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)

New submit-op command:

- `launch <submit-selector> <owner_state> <issued_packet_count>`

Userspace-side fixes in the same batch:

- `psb_sgx_lab_submit_op.c` now includes `launch` in the selector-parsing
  set, so `launch focus ...` resolves the focused submit instead of sending
  `submit_id=0`
- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
  now supports `REMOTE_SHELL_TRACE=1`
- the same focused runner now prints the captured typed submit-op result if
  `launch` or `issue` fails, instead of silently exiting out of the remote
  body

Runner changes:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
- [run-vaio-sgx-multi-submit-typed.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit-typed.sh)

Typed progression contract is now:

- `prepare`
- `arm`
- `validate`
- `emit`
- `validate`
- `launch`
- `validate`
- `issue`

## Validation

Local checks:

- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`
- `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `bash -n scripts/run-vaio-sgx-multi-submit.sh`
- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`

Guarded deploy:

- previous live module:
  - `driver_srcversion=333CBAAFFFDE848128DC9EC`
  - `override_module_size=402932`
- rebuilt live module:
  - `driver_srcversion=A6F49F33394FCA892D9A5B2`
  - `override_module_size=404800`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`
  - `rollback_backup_size=402932`

Canonical live bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260511-190148-sgx-submit-launch-op-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-190148-sgx-submit-launch-op-live/summary.log)
- typed multi-submit:
  - [logs/vaio-ioctl/20260511-190211-sgx-multi-submit-launch-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-190211-sgx-multi-submit-launch-live/summary.log)

Focused single-submit evidence:

- `SUBMIT_ID=5`
- `OBJECT_IDS=27 28 29 30 31`
- post-launch query proved:
  - `submit_launch_valid=1`
  - `submit_launch_consumed=0`
  - `submit_launch_count=1`
  - `submit_launch_prepare_count=1`
  - `submit_launch_arm_count=1`
  - `submit_launch_emit_count=1`
  - `submit_launch_cmd_type_name=ta`
  - `submit_launch_owner_state_name=queued`
  - `submit_launch_issued_packets=1`
  - `submit_launch_queue_packet_count=2`
  - `submit_launch_sync_pending_seq=0x42`
  - `submit_launch_owner_issue_cookie=0x102`
  - `submit_launch_global_issue_ticket=1`
  - `submit_launch_dependency_active_count=0`
- post-issue query proved:
  - `submit_launch_valid=1`
  - `submit_launch_consumed=1`
  - `submit_issue_count=1`
- final focused path still completed cleanly through:
  - `submit_transition_count=2`
  - `submit_owner_state_name=complete`

Typed multi-submit evidence:

- submit A final state:
  - `submit_prepare_count=1`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
- submit B final state:
  - `submit_prepare_count=1`
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
- dependency-gated launched state is real:
  - submit B launch after dependency release logged
    `dependency_active_count=1`
  - kernel `dmesg` showed the same launch-stage issue ticket and dependency
    count before each `issue`

Final target state after this batch:

- `overall=remote_usable`
- `driver_srcversion=A6F49F33394FCA892D9A5B2`
- `override_module_size=404800`
- `rollback_pending_present=0`

## Conclusion

The lab path now has four distinct execution-shaped checkpoints:

- `prepare`: materialize one coherent backing image
- `arm`: freeze one pre-issue snapshot
- `emit`: freeze one execution-shaped submit record
- `launch`: freeze the exact issue plan that `issue` will consume

That is the first point where the lab path stops treating issue-side
progression as an implicit recomputation step and starts treating it as a
first-class launched execution image.
