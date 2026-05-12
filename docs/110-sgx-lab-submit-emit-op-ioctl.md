# SGX Lab Submit Emit Op Ioctl

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion 333CBAAFFFDE848128DC9EC`

## Goal

`prepare` gave the lab path one coherent materialized submit image in backing
control and status memory. `arm` then committed one issue-side snapshot so
`issue` no longer ran from arbitrary mutable submit state.

One execution-shaped gap still remained:

- there was no kernel-owned snapshot of the exact armed image about to be used
  for issue
- callers could prove prepared state and armed state, but not one explicit
  emitted execution record
- there was no separate point where the kernel could freeze the current armed
  submit image, expose that frozen state to typed query tools, and then make
  `issue` consume only that emitted record

This batch adds that checkpoint as typed `emit`.

## Changes

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New submit-op opcode:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_EMIT`

New typed query fields:

- `emit_valid`
- `emit_count`
- `emit_prepare_count`
- `emit_arm_count`
- `emit_context`
- `emit_cmd_type`
- `emit_owner_state`
- `emit_ctrl_flags`
- `emit_num_ta_status_vals`
- `emit_num_3d_status_vals`
- `emit_ta_dependency`
- `emit_control_object_id`
- `emit_pds_object_id`
- `emit_use_object_id`
- `emit_target_object_id`
- `emit_status_object_id`
- `emit_ccb_offset`
- `emit_ccb_write_offset`
- `emit_ccb_read_offset`
- `emit_ccb_service_address`
- `emit_ccb_cache_control`
- `emit_ccb_data0`
- `emit_ccb_data1`
- `emit_queue_base_offset`
- `emit_queue_size`
- `emit_queue_packet_size`
- `emit_queue_packet_count`
- `emit_queue_tail_packet_count`
- `emit_queue_head_packet_count`
- `emit_sync_complete_seq`
- `emit_sync_pending_seq`
- `emit_sync_flags`
- `emit_owner_issue_cookie`
- `emit_owner_complete_cookie`
- `emit_queue_epoch`
- `emit_global_issue_ticket`
- `emit_dependency_active_count`
- `emit_control_gpu_va`
- `emit_pds_gpu_va`
- `emit_use_gpu_va`
- `emit_target_gpu_va`
- `emit_status_gpu_va`
- `last_emit_jiffies`

Implementation changes:

- `emit` now requires an already armed submit image
- `emit` snapshots the current armed image into the submit descriptor,
  including:
  - object IDs
  - resolved GPU VAs
  - queue geometry
  - CCB words and offsets
  - sync and owner state
  - issue-side ticket state
  - active dependency-lane count
- `emit` increments `emit_count` and records `last_emit_jiffies`
- any later submit-shaping mutation still disarms the submit, which now also
  invalidates the emitted snapshot
- `issue` now requires a valid emitted snapshot
- `issue` rejects stale state if the current submit no longer matches the
  emitted record
- successful `issue` consumes the emitted snapshot

Userspace:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)

New submit-op command:

- `emit <submit-selector>`

The typed query and dump helpers now expose emitted-state counters and snapshot
metadata so callers can prove:

- the submit was prepared
- the submit was armed
- the submit was emitted
- the same emitted record was then consumed by `issue`

Validation runners:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
- [run-vaio-sgx-multi-submit-typed.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit-typed.sh)

Focused submit-op runner changes:

- after typed `arm`, it now issues:
  - `validate focus`
  - `emit focus`
  - `validate focus`
- it now proves:
  - `submit_emit_valid=1`
  - `submit_emit_count=1`
  - `submit_emit_cmd_type_name=ta`
  - `submit_emit_owner_state_name=queued`
  - `submit_emit_control_object_id=1`
  - `submit_emit_status_object_id=5`
- after issue it proves:
  - `submit_emit_valid=0`

Typed multi-submit runner changes:

- every typed issue step now becomes:
  - `prepare`
  - `arm`
  - `validate`
  - `emit`
  - `validate`
  - `issue`
- refill cycles use the same sequence
- dependency-gated and refill-driven submits now prove the same emitted-state
  lifecycle instead of only prepared-state or armed-state snapshots

## Validation

Local checks:

- `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `bash -n scripts/run-vaio-sgx-multi-submit.sh`
- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`
- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`

Guarded deploy:

- previous live module:
  - `driver_srcversion=333CBAAFFFDE848128DC9EC`
  - `override_module_size=402932`
- rebuilt live module:
  - `driver_srcversion=333CBAAFFFDE848128DC9EC`
  - `override_module_size=402932`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`
  - `rollback_backup_size=402932`

Canonical live bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260511-181000-sgx-submit-emit-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-181000-sgx-submit-emit-op-live)
- typed multi-submit:
  - [logs/vaio-ioctl/20260511-181100-sgx-multi-submit-emit-live](/root/Poulsbo/logs/vaio-ioctl/20260511-181100-sgx-multi-submit-emit-live)

Final target state after this batch:

- `overall=remote_usable`
- `driver_srcversion=333CBAAFFFDE848128DC9EC`
- `override_module_size=402932`
- `rollback_pending_present=0`

Focused single-submit evidence from `output.log`:

- post-prepare query:
  - `submit_prepare_count=1`
  - `submit_emit_valid=0`
- post-arm query:
  - `submit_arm_count=1`
  - `submit_emit_valid=0`
- post-emit query:
  - `submit_emit_valid=1`
  - `submit_emit_count=1`
  - `submit_emit_cmd_type_name=ta`
  - `submit_emit_owner_state_name=queued`
  - `submit_emit_control_object_id=1`
  - `submit_emit_status_object_id=5`
  - `submit_last_emit_jiffies=4294937765`
- post-issue query:
  - `submit_issue_count=1`
  - `submit_emit_valid=0`
- final state:
  - `submit_validate_count=1`
  - `submit_transition_count=2`
  - `submit_owner_state_name=complete`
- final typed cleanup returned:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=5`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

Kernel log evidence from the same bundle:

- `lab submit prepare submit_id=1 packet_count=1 write_offset=0x20 prepare_count=1`
- `lab submit arm submit_id=1 prepare_count=1 packet_count=1 write_offset=0x20 global_issue_ticket=0`
- `lab submit emit submit_id=1 arm_count=1 packet_count=1 global_issue_ticket=0 dependency_active_count=0`
- then:
  - `lab submit issue submit_id=1 ... global_issue_ticket=1`
  - `lab submit transition submit_id=1 state=running ...`
  - `lab submit transition submit_id=1 state=complete ...`

Typed multi-submit evidence from `output.log`:

- typed setup remained fully enabled:
  - `SUBMIT_SETUP=ioctl`
  - `OBJECT_SETUP=ioctl`
  - `RANGE_DISCOVERY=ioctl`
  - `DEPENDENCY_SETUP=ioctl`
  - `VERIFICATION_MODE=ioctl`
- typed dependency-gated progression now emits every live submit before issue:
  - submit A final state:
    - `submit_prepare_count=1`
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final state:
    - `submit_prepare_count=1`
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
- while active, emitted-state queries proved:
  - submit A:
    - `submit_emit_valid=1`
    - `submit_emit_count=3`
    - `submit_last_emit_jiffies=4294947894`
  - submit B:
    - `submit_emit_valid=1`
    - `submit_emit_count=2`
    - `submit_emit_dependency_active_count=1`
    - `submit_last_emit_jiffies=4294948037`
- kernel log evidence showed the full emitted progression:
  - `lab submit emit submit_id=2 arm_count=1 ... dependency_active_count=0`
  - `lab submit emit submit_id=2 arm_count=2 ... dependency_active_count=0`
  - `lab submit emit submit_id=2 arm_count=3 ... dependency_active_count=0`
  - `lab submit emit submit_id=3 arm_count=1 ... dependency_active_count=1`
  - `lab submit emit submit_id=3 arm_count=2 ... dependency_active_count=1`
- final typed cleanup returned:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

## Result

The bounded submit-op ioctl path now has one explicit emitted execution record
between “the submit is armed” and “the submit is issued.”

That closes the next execution-shaped lab gap:

- `prepare` materializes one coherent image
- `arm` commits the exact prepared image intended for issue
- `emit` snapshots one kernel-owned execution record from that armed image
- `issue` now consumes only that emitted record and invalidates it afterward

This is still validate-only SGX lab work, not real SGX execution. But it
pushes the lab path one step closer to a real execution model: callers can now
prove one explicit emitted submit image, its dependency activity, and its
consumption by `issue` on real hardware instead of stopping at prepared-state
or armed-state snapshots.
