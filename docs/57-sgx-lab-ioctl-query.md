# SGX Lab Ioctl Query Validation

This note records the first bounded SGX lab ioctl surface and its live
validation on the Sony VAIO VGN-P11Z target.

## Context

The SGX lab submit work had reached the point where submit, coordination,
queue, sync, owner, and dependency state could only be inspected through
PCI-device sysfs text nodes such as:

- `poulsbo_sgx_submit_state`
- `poulsbo_sgx_submits`
- `poulsbo_sgx_coord_state`

That was good enough for early bring-up, but it kept the lab path tied to
shell parsing. The next step was to add one bounded read-only ioctl surface for
existing SGX lab state without pretending to define a real render ABI.

The implementation adds:

- shared header:
  [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
- kernel query ioctls in
  [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
- typed userspace probe:
  [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
- live target runner:
  [run-vaio-sgx-ioctl-query.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-query.sh)

## Ioctl Surface

The shared header defines two lab-only commands:

- `DRM_IOCTL_PSB_SGX_LAB_QUERY_SUBMIT`
- `DRM_IOCTL_PSB_SGX_LAB_QUERY_COORD`

`QUERY_SUBMIT` returns typed state for one existing lab submit, either by
explicit `submit_id` or by the currently focused submit through
`DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_FOCUS`.

`QUERY_COORD` returns typed driver-global SGX lab coordination state, including:

- focused submit identity
- live submit count
- live object count
- global issue ticket and last-issue submit
- global running ticket and active-running submit
- global completion ticket and last-complete submit

The submit query returns the current validated lab fields that matter today:

- context and command type
- owner state and last validate status
- queue and CCB offsets
- sync and owner block state
- global issue/running/complete tickets
- attached object IDs
- resolved GPU virtual addresses
- indexed dependency slot state

## Hardware Validation

The live bundle is:

- [logs/vaio-ioctl/20260511-072823-sgx-query-live](/root/Poulsbo/logs/vaio-ioctl/20260511-072823-sgx-query-live)

The target state for this run was:

- kernel: `5.4.0-150-generic`
- override module path:
  `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- override module `srcversion`:
  `0A70DFC7732D064F1658113`

The runner:

1. copied `drm_probe` and `psb_sgx_lab_query` to the VAIO
2. created five live dumb GEM ranges
3. adopted them as SGX lab objects
4. created and configured one PF submit
5. validated the baseline state through sysfs
6. queried the same state through the new ioctls
7. advanced the submit once with `issue`
8. re-queried the updated state through both sysfs and ioctls

## Observed Result

The saved log is:

- [ioctl-query.log](/root/Poulsbo/logs/vaio-ioctl/20260511-072823-sgx-query-live/ioctl-query.log)

Key results:

- the live runner created and focused:
  - `SUBMIT_ID=2`
- base sysfs state showed:
  - `owner_state=queued`
  - `validate_count=1`
  - `control_gpu_va=0x81127000`
  - `status_gpu_va=0x81137000`
- `IOCTL_QUERY_BY_ID_BASE` matched that same submit:
  - `submit_resolved_id=2`
  - `submit_context_name=pf`
  - `submit_cmd_type_name=ta`
  - `submit_owner_state_name=queued`
  - `submit_validate_count=1`
  - `submit_issue_count=0`
  - `submit_global_issue_ticket=0`
  - `submit_control_gpu_va=0x81127000`
  - `submit_status_gpu_va=0x81137000`
- `IOCTL_QUERY_FOCUS_BASE` resolved the same submit through the focus flag:
  - `submit_resolved_id=2`
- the coordination ioctl also matched the global sysfs state:
  - `coord_live_submit_count=1`
  - `coord_live_object_count=5`
- after one `issue` step, the sysfs state advanced to:
  - `validate_count=2`
  - `issue_count=1`
  - `queue_packet_count=2`
  - `sync_pending_seq=0x42`
  - `owner_issue_cookie=0x102`
  - `global_issue_ticket=2`
- `IOCTL_QUERY_BY_ID_AFTER_ISSUE` matched those updated fields exactly:
  - `submit_validate_count=2`
  - `submit_issue_count=1`
  - `submit_queue_packet_count=2`
  - `submit_sync_pending_seq=0x42`
  - `submit_owner_issue_cookie=0x102`
  - `submit_global_issue_ticket=2`
- the coordination ioctl matched the updated coord state too:
  - `coord_issue_ticket=2`
  - `coord_issue_submit_id=2`
  - `coord_live_submit_count=1`
  - `coord_live_object_count=5`

## Conclusion

The first bounded SGX lab ioctl surface is now implemented and validated on
real hardware.

SGX lab submit-state inspection is no longer sysfs-only. The next interface
question is narrower:

- keep submit mutation on sysfs and use ioctls only for typed query, or
- add one similarly bounded write-side ioctl for existing validated lab submit
  operations
