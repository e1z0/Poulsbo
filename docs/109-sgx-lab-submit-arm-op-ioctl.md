# SGX Lab Submit Arm Op Ioctl

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion 270B70E360AEFF73D0C52F3`

## Goal

`prepare` gave the lab path one coherent materialized submit image in backing
control and status memory. The next execution-shaped gap was still open:

- `issue` could run directly after arbitrary submit-shaping mutations
- there was no explicit “this prepared image is the one I intend to issue”
  checkpoint
- there was no stale-image rejection if queue or sync state drifted between
  materialization and issue

This batch adds that checkpoint as typed `arm`.

## Changes

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New submit-op opcode:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_ARM`

New typed query fields:

- `arm_valid`
- `arm_count`
- `arm_prepare_count`
- `arm_queue_packet_count`
- `arm_queue_tail_packet_count`
- `arm_queue_head_packet_count`
- `arm_ccb_read_offset`
- `arm_ccb_write_offset`
- `arm_sync_complete_seq`
- `arm_sync_pending_seq`
- `arm_owner_state`
- `arm_owner_issue_cookie`
- `arm_owner_complete_cookie`
- `arm_queue_epoch`
- `arm_global_issue_ticket`
- `last_arm_jiffies`

Implementation changes:

- `arm` now requires an already prepared submit image
- `arm` snapshots the current prepared queue, sync, owner, and issue-side state
  into the submit descriptor
- `issue` now requires a valid armed snapshot
- `issue` rejects stale armed state if current submit state no longer matches
  the armed snapshot
- successful `issue` consumes the armed snapshot
- `transition` also clears the armed snapshot so the next progression step must
  start from a fresh `prepare -> arm`
- all submit-shaping mutations now disarm:
  - object attach or detach
  - kick
  - CCB
  - queue
  - status
  - sync
  - owner
  - dependency-slot mutations
  - `prepare` itself before rematerialization

Userspace:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)

New submit-op command:

- `arm <submit-selector>`

The typed query and dump helpers now expose armed-state counters and the last
armed snapshot metadata, so callers can prove:

- the submit was prepared
- the submit was armed
- the same armed image was consumed by issue

Validation runners:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)

Focused submit-op runner changes:

- after typed `prepare`, it now issues:
  - `arm focus`
- then proves:
  - `submit_arm_valid=1`
  - `submit_arm_count=1`
  - `submit_arm_prepare_count=1`
  - `submit_arm_queue_packet_count=1`
  - `submit_arm_owner_state_name=queued`
- after issue it proves:
  - `submit_arm_valid=0`

Typed multi-submit runner changes:

- every typed issue step now becomes:
  - `prepare`
  - `arm`
  - `validate`
  - `issue`
- refill cycles use the same sequence
- dependency-gated and refill-driven submits now prove the same armed-state
  lifecycle instead of only prepared-state materialization

## Validation

Local checks:

- `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `bash -n scripts/run-vaio-sgx-multi-submit.sh`
- `bash -n scripts/run-vaio-sgx-submit-typed.sh`
- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`
- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`

Guarded deploy:

- previous live module:
  - `driver_srcversion=AE18527FD5F6C10BE6C85A5`
  - `override_module_size=390608`
- new live module:
  - `driver_srcversion=270B70E360AEFF73D0C52F3`
  - `override_module_size=396320`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`
  - `rollback_backup_size=390608`

Canonical live bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260511-175155-sgx-submit-arm-op-live](/root/Poulsbo/logs/vaio-ioctl/20260511-175155-sgx-submit-arm-op-live)
- typed multi-submit:
  - [logs/vaio-ioctl/20260511-175233-sgx-multi-submit-arm-live](/root/Poulsbo/logs/vaio-ioctl/20260511-175233-sgx-multi-submit-arm-live)

Focused single-submit evidence from `output.log`:

- post-prepare query:
  - `submit_prepare_count=1`
  - `submit_arm_valid=0`
- post-arm query:
  - `submit_arm_valid=1`
  - `submit_arm_count=1`
  - `submit_arm_prepare_count=1`
  - `submit_arm_queue_packet_count=1`
  - `submit_arm_owner_state_name=queued`
- post-issue query:
  - `submit_issue_count=1`
  - `submit_arm_valid=0`
- final submit state:
  - `submit_validate_count=1`
  - `submit_transition_count=2`
  - `submit_owner_state_name=complete`

Kernel log evidence from the same bundle:

- `lab submit prepare submit_id=1 packet_count=1 write_offset=0x20 prepare_count=1`
- `lab submit arm submit_id=1 prepare_count=1 packet_count=1 write_offset=0x20 global_issue_ticket=0`
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
- typed progression now arms every live submit before issue:
  - submit A:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
  - submit B:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_validate_count=23`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
  - submit C:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
  - submit D:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
  - submit E:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- all post-issue or post-transition queries returned:
  - `submit_arm_valid=0`
- final typed cleanup:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

Final target state:

- `overall=remote_usable`
- `driver_srcversion=270B70E360AEFF73D0C52F3`
- `override_module_size=396320`
- `rollback_pending_present=0`

## Result

The bounded submit-op ioctl path now has an explicit progression fence between
prepared state and issued state.

That closes the next real lab gap:

- `prepare` materializes one coherent image
- `arm` commits the exact prepared image intended for issue
- `issue` now consumes only a fresh armed image and rejects stale state

This is still validate-only SGX lab work, not real execution. But it is the
first step that makes the submit path behave like an execution-oriented state
machine instead of a bag of mutable fields plus progression calls.
