# SGX Lab Submit Report Op Ioctl

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion E774FBBBD966270FEA6C67B`

## Goal

The execution-shaped lab path already had:

- `prepare`
- `arm`
- `emit`
- `launch`
- `dispatch`
- `retired`

What it still lacked was one bounded typed status block that materialized the
current execution phase directly in backing memory and could be validated
across:

- focused single-submit progression
- dependency-gated multi-submit progression
- refill cycles after a completed submit

This batch closes that gap with a typed `report` block.

## What Changed

Kernel-side:

- [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)

Typed submit-op surface:

- `DRM_PSB_SGX_LAB_SUBMIT_OP_REPORT`
- userspace command:
  - `report <submit-selector> <offset>`

Typed report state now covers:

- `report_offset`
- current stage and stage count
- current owner state
- queue packet count
- completed packet count
- active dependency count
- queue epoch
- global issue/running/complete tickets
- report flags
- config count
- `last_report_jiffies`

The report block is refreshed from live submit state after:

- `report`
- `prepare`
- `arm`
- `emit`
- `launch`
- `issue`
- `transition`
- submit-shaping mutations that already disarm execution state

This batch also fixed the two real issues the first end-to-end validation
exposed:

- userspace selector bug:
  - [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
    now routes `report focus ...` through the same selector helper as the
    other submit-ID or focused-submit ops
- refill-stage resolver bug:
  - [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
    now resolves the report stage by the newest valid stage-count lineage
    instead of blindly preferring any older persisted `retired` snapshot

Runner-side fixes in the same batch:

- [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
  now asserts the actual live report-flag behavior:
  - after `issue`: `0x1c`
  - after `transition running`: `0x10`
  - after `transition complete`: `0x30`
- [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
  now extracts snapshot values with one `awk` pass instead of `tr | awk`,
  avoiding false `141` exits from `SIGPIPE` under `set -o pipefail`

Userspace read-side coverage was extended together with the report block:

- [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- [psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)

Those helpers now expose the report state in both text and dump output.

## Validation

Local checks:

- `./scripts/docker-build-workloads-i386.sh`
- `./scripts/docker-build-vaio-gma500.sh`
- `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
- `bash -n scripts/run-vaio-sgx-multi-submit.sh`
- `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`

Guarded deploy:

- [logs/vaio-deploy/20260511-212444-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260511-212444-guarded-deploy)
- previous live override:
  - `srcversion 093DDDD6F956A0E7E690BBE`
  - `override_module_size=416544`
- rebuilt live override:
  - `srcversion E774FBBBD966270FEA6C67B`
  - `override_module_size=416736`
- rollback guard returned cleanly to:
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Canonical live bundles:

- focused typed submit-op:
  - [logs/vaio-ioctl/20260511-212632-sgx-submit-report-op-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-212632-sgx-submit-report-op-live/summary.log)
- typed multi-submit:
  - [logs/vaio-ioctl/20260511-212931-sgx-multi-submit-report-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-212931-sgx-multi-submit-report-live/summary.log)

Focused single-submit evidence:

- report configuration landed cleanly:
  - `submit_report_offset=0x120`
  - `submit_report_config_count=1`
  - initial stage:
    - `submit_report_stage_name=none`
    - `submit_report_owner_state_name=queued`
    - `submit_report_queue_packet_count=1`
- first-cycle stage progression was validated end to end:
  - `prepared`
  - `armed`
  - `emitted`
  - `launched`
  - `dispatched`
  - `retired`
- first-cycle live flag progression was validated:
  - `0x0`
  - `0x1`
  - `0x3`
  - `0x7`
  - `0x1c`
  - `0x10`
  - `0x30`
- focused progression still completed cleanly:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - final `submit_report_stage_name=retired`
  - final `submit_report_completed_packets=2`

Typed multi-submit evidence:

- the typed/default wrapper path completed cleanly on the new kernel build
- submit A final state:
  - `submit_prepare_count=1`
  - `submit_arm_count=3`
  - `submit_emit_count=3`
  - `submit_launch_count=3`
  - `submit_issue_count=3`
  - `submit_transition_count=6`
  - final `submit_report_stage_name=retired`
  - final `submit_report_stage_count=3`
- submit B final dependency-gated state:
  - `submit_prepare_count=1`
  - `submit_arm_count=2`
  - `submit_emit_count=2`
  - `submit_launch_count=2`
  - `submit_issue_count=2`
  - `submit_transition_count=4`
  - `submit_report_dependency_active_count=1` while the dependency lane was
    active
  - final `submit_report_stage_name=retired`
  - final `submit_report_stage_count=2`
- the refill-path fix is validated on real hardware:
  - after a completed cycle, later `prepare/arm/emit/launch` now reappear as
    the live report stage instead of being masked by the persisted older
    `retired` snapshot
  - the older `dispatch` and `retired` snapshots still remain query-visible as
    historical execution state
- final typed cleanup still returned:
  - `coord_live_submit_count=0`
  - `coord_live_object_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

## Conclusion

The bounded execution-shaped lab path now has one memory-backed report block
that tracks the live execution stage through:

- `prepare`
- `arm`
- `emit`
- `launch`
- `dispatch`
- `retired`

That report path is now validated on real hardware for:

- focused single-submit progression
- dependency-gated multi-submit progression
- refill-cycle stage resolution after a completed submit

The next useful batch is above this layer: build the first more SGX-shaped
execution structure that consumes the current `prepare -> arm -> emit ->
launch -> dispatch -> retired -> report` model instead of adding more generic
inspection state.
