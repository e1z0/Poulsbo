# SGX Lab Submit Prepare Op Ioctl

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion AE18527FD5F6C10BE6C85A5`

## Goal

The bounded submit-op ioctl surface could already describe one complete SGX lab
submit:

- object attachments
- kick metadata
- CCB words
- queue layout
- status layout
- sync block layout
- owner block layout
- dependency state

But that still left one gap before any more execution-oriented work: an
explicit kernel pass that materializes the current submit image into the
backing control and status objects as one coherent snapshot.

This batch adds that pass as typed `prepare`.

## Changes

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

New submit-op opcode:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_PREPARE`

New typed query fields:

- `prepare_count`
- `last_prepare_jiffies`

Implementation changes:

- `prepare` now requires:
  - control object attached
  - kick configured
  - CCB configured
  - queue configured
- if status, sync, owner, or dependency blocks are configured, `prepare`
  requires the status object too
- one kernel-side prepare pass now:
  - canonicalizes queue-derived offsets and packet split state
  - writes queue header words into the control object
  - writes the current CCB packet words at the active `ccb_offset`
  - materializes the active queue packet image into backing memory
  - rewrites status values from the configured status seed
  - rewrites the sync block from current submit state
  - rewrites the owner block from current submit state
  - refreshes any active dependency state blocks
  - re-validates the final prepared image
- the submit now records:
  - `prepare_count`
  - `last_prepare_jiffies`

Userspace:

- [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)

New submit-op command:

- `prepare <submit-selector>`

The typed query and dump tools now expose the new prepare counters so callers
can prove the materialization pass happened before issue or transition.

Validation runners:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)

Focused submit-op runner changes:

- after `kick`, `ccb`, `queue`, `status`, `sync`, and `owner`, it now issues:
  - `prepare focus`
- then proves:
  - `submit_prepare_count=1`
  - the same submit still completes cleanly through
    `validate -> issue -> transition running -> transition complete`

Typed multi-submit runner changes:

- typed submit setup now includes one `prepare` call per submit after queue and
  block setup and before the first validate
- the same typed dependency-gated multi-submit flow is then revalidated with
  prepared control/status images instead of only incremental config writes

## Validation

Local checks:

- `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `bash -n scripts/run-vaio-sgx-multi-submit.sh`
- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`
- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`

Guarded deploy:

- previous live module:
  - `driver_srcversion=933E555C1A18400A2CF86AE`
  - `override_module_size=387664`
- new live module:
  - `driver_srcversion=AE18527FD5F6C10BE6C85A5`
  - `override_module_size=390608`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`
  - `rollback_backup_size=387664`

Canonical live bundle:

- [logs/vaio-ioctl/20260511-172038-sgx-submit-prepare-live](/root/Poulsbo/logs/vaio-ioctl/20260511-172038-sgx-submit-prepare-live)

Target state from that bundle:

- `driver_srcversion=AE18527FD5F6C10BE6C85A5`
- `override_module_size=390608`
- `overall=remote_usable`

Focused single-submit evidence from `submit-op.log`:

- typed prepare op executed:
  - `submit_op_name=prepare`
- post-prepare query:
  - `submit_prepare_count=1`
  - `submit_last_prepare_jiffies=4294940548`
- the same submit still completed cleanly:
  - `submit_validate_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - `submit_owner_state_name=complete`

Kernel log evidence from the same bundle:

- `lab submit prepare submit_id=1 packet_count=1 write_offset=0x20 prepare_count=1`
- then:
  - `lab submit validate submit_id=1 context=pf status=0 count=1`
  - `lab submit issue submit_id=1 ... global_issue_ticket=1`
  - `lab submit transition submit_id=1 state=running ...`
  - `lab submit transition submit_id=1 state=complete ...`

Typed multi-submit evidence from `multi-submit.log` and `dmesg-tail.log`:

- typed setup remained fully enabled:
  - `USE_IOCTL_SUBMIT_SETUP=1`
  - `USE_IOCTL_OBJECT_SETUP=1`
  - `USE_IOCTL_RANGE_DISCOVERY=1`
  - `USE_IOCTL_DEPENDENCY_SETUP=1`
  - `USE_IOCTL_VERIFICATION=1`
- canonical live typed submits on the rerun:
  - `submit_id=5`
  - `submit_id=6`
- both were prepared exactly once before progression:
  - `lab submit prepare submit_id=5 packet_count=1 write_offset=0x0 prepare_count=1`
  - `lab submit prepare submit_id=6 packet_count=1 write_offset=0x20 prepare_count=1`
- typed live terminal state:
  - submit A:
    - `submit_prepare_count=1`
    - `submit_validate_count=11`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_owner_state_name=complete`
  - submit B:
    - `submit_prepare_count=1`
    - `submit_validate_count=11`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_owner_state_name=complete`
- final typed cleanup:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

## Result

The bounded submit-op ioctl path now has one explicit materialization step
between “submit metadata exists” and “submit progression begins.”

That matters because the lab path can now prove three things together:

- the kernel’s internal submit image is coherent
- the backing control/status objects contain the same coherent image
- the same prepared image still survives validate, dependency gating, issue,
  transition, refill, and cleanup on real hardware

This is still validate-only lab work, not real SGX execution. But it closes
the remaining gap between pure metadata configuration and an execution-shaped
prepared command image, which is the next necessary step before any real SGX
submission experiments.
