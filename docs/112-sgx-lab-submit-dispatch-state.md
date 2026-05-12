# SGX Lab Submit Dispatch State

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Kernel: `5.4.0-150-generic`
Module: `gma500_gfx` override `srcversion D73A90E161CB5B4680FD013`

## What Changed

The bounded typed SGX submit path now keeps one persistent post-`issue`
execution snapshot named `dispatch`.

That snapshot is recorded when `issue` consumes the launched image, then
updated through later `running` and `complete` transitions instead of being
discarded immediately. The typed query and dump helpers now expose:

- `dispatch_valid`
- `dispatch_count`
- `dispatch_prepare_count`
- `dispatch_arm_count`
- `dispatch_emit_count`
- `dispatch_launch_count`
- dispatched queue, CCB, sync, owner, ticket, GPU-VA, and object-ID fields
- `dispatch_dependency_active_count`
- `last_dispatch_jiffies`

The focused submit validator was also fixed to use live coordination-ticket
baselines instead of assuming a cold target where global issue/running/complete
tickets all start at `0`.

## Validation

Canonical live bundles:

- [logs/vaio-ioctl/20260511-192912-sgx-submit-dispatch-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-192912-sgx-submit-dispatch-live/summary.log)
- [logs/vaio-ioctl/20260511-192947-sgx-multi-submit-dispatch-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-192947-sgx-multi-submit-dispatch-live/summary.log)

Focused typed submit-op result:

- coordination baseline before launch was `issue/running/complete = 6/6/6`
- the same submit ended at `7/7/7`
- final live submit state still completed cleanly:
  - `submit_validate_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - `submit_owner_state_name=complete`
- final dispatch snapshot stayed live after `issue` and both transitions:
  - `submit_dispatch_valid=1`
  - `submit_dispatch_count=1`
  - `submit_dispatch_prepare_count=1`
  - `submit_dispatch_arm_count=1`
  - `submit_dispatch_emit_count=1`
  - `submit_dispatch_launch_count=1`
  - `submit_dispatch_owner_state_name=complete`
  - `submit_dispatch_issued_packets=1`
  - `submit_dispatch_completed_packets=2`
  - `submit_dispatch_queue_packet_count=0`
  - `submit_dispatch_sync_complete_seq=0x42`
  - `submit_dispatch_sync_pending_seq=0x42`
  - `submit_dispatch_owner_issue_cookie=0x102`
  - `submit_dispatch_owner_complete_cookie=0x102`
  - `submit_dispatch_global_issue_ticket=7`
  - `submit_dispatch_global_running_ticket=7`
  - `submit_dispatch_global_complete_ticket=7`

Typed multi-submit result:

- canonical live submit IDs were `SUBMIT_A_ID=7` and `SUBMIT_B_ID=8`
- `COORD_COMPLETE_TICKET_BASELINE=7`
- final dispatch-bearing submit state was still clean after dependency-gated
  progression and typed cleanup:
  - submit A:
    - `submit_prepare_count=1`
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_dispatch_owner_state_name=complete`
    - `submit_dispatch_completed_packets=1`
    - `submit_dispatch_queue_packet_count=0`
    - `submit_dispatch_dependency_active_count=0`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B:
    - `submit_prepare_count=1`
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_dispatch_owner_state_name=complete`
    - `submit_dispatch_completed_packets=1`
    - `submit_dispatch_queue_packet_count=0`
    - `submit_dispatch_dependency_active_count=1`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
- final cleanup returned:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`

## Conclusion

The bounded lab path now proves five distinct execution-shaped checkpoints on
real hardware:

- `prepare`
- `arm`
- `emit`
- `launch`
- `dispatch`

`dispatch` is the first persistent post-`issue` execution snapshot in the lab
stack. It survives through `running` and `complete`, which is the right base
for the next step: a more SGX-shaped execution structure, not more read-side
inspection plumbing.
