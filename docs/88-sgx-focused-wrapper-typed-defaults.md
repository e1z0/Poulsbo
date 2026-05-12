# SGX Focused Wrapper Typed Defaults

## What Changed

Added focused typed wrappers:

- `scripts/run-vaio-sgx-object-typed.sh`
- `scripts/run-vaio-sgx-submit-typed.sh`

and changed the older focused helper names so they now default onto those
typed paths:

- `scripts/run-vaio-sgx-object.sh`
- `scripts/run-vaio-sgx-submit.sh`

The older sysfs-heavy script bodies remain available through explicit opt-in:

- `SGX_OBJECT_DRIVER_MODE=legacy`
- `SGX_SUBMIT_DRIVER_MODE=legacy`

The typed wrappers keep compatibility with the old probe-binary variables:

- `LOCAL_BIN`
- `TARGET_BIN`
- `TARGET_STAGE`

by mapping them onto the already-validated typed validators:

- `scripts/run-vaio-sgx-ioctl-object-op.sh`
- `scripts/run-vaio-sgx-ioctl-submit-op.sh`

## Hardware Validation

Validated on May 11, 2026 on `10.1.11.218` with:

- object wrapper:
  - `logs/vaio-ioctl/20260511-125525-sgx-object-wrapper-typed-default-live-rerun`
- submit wrapper:
  - `logs/vaio-ioctl/20260511-125341-sgx-submit-wrapper-typed-default-live`

Final target state after both runs:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

### Object Wrapper

The default `run-vaio-sgx-object.sh` path now executed the typed object-op
stack:

- `REMOTE_OP_BIN=/root/poulsbo-workloads/psb_sgx_lab_object_op`
- `REMOTE_OBJECT_QUERY_BIN=/root/poulsbo-workloads/psb_sgx_lab_object_query`
- `REMOTE_RANGE_QUERY_BIN=/root/poulsbo-workloads/psb_sgx_lab_range_query`
- `REMOTE_GTT_STATE_BIN=/root/poulsbo-workloads/psb_sgx_lab_gtt_state_query`

The saved run adopted and mutated one live GEM-backed object:

- `RANGE_ID=7`
- `OBJECT_ID=123`

The typed wrapper then proved the same focused object path as the dedicated
typed validator:

- post-adopt:
  - `gtt_state_range_sgx_object_count=1`
  - `gtt_state_range_pf_bound_count=0`
- post-metadata:
  - `object_role_name=control`
  - `object_class_name=packet`
  - `object_gpu_use_name=control_ccb`
  - `object_expected_context_name=pf`
- post-PF-bind:
  - `object_pf_bind_count=1`
  - `range_pf_bind_count=1`
  - `gtt_state_range_pf_bound_count=1`
- post-PF-unbind:
  - `object_pf_bind_count=0`
  - `range_pf_bind_count=0`
  - `gtt_state_range_pf_bound_count=0`
- post-drop:
  - `object_ioctl_ret=-1 errno=2`
  - `gtt_state_range_sgx_object_count=0`

### Submit Wrapper

The default `run-vaio-sgx-submit.sh` path now executed the typed submit-op
stack:

- `REMOTE_SUBMIT_OP_BIN=/root/poulsbo-workloads/psb_sgx_lab_submit_op`
- `REMOTE_QUERY_BIN=/root/poulsbo-workloads/psb_sgx_lab_query`
- `REMOTE_OBJECT_OP_BIN=/root/poulsbo-workloads/psb_sgx_lab_object_op`
- `REMOTE_OBJECT_QUERY_BIN=/root/poulsbo-workloads/psb_sgx_lab_object_query`
- `REMOTE_RANGE_QUERY_BIN=/root/poulsbo-workloads/psb_sgx_lab_range_query`
- `REMOTE_GTT_STATE_BIN=/root/poulsbo-workloads/psb_sgx_lab_gtt_state_query`

The saved run staged one five-object submit:

- `SUBMIT_ID=32`
- `RANGE_IDS=7 9 125 126 127`
- `OBJECT_IDS=117 118 119 120 121`

The default wrapper then proved the full focused typed submit progression:

- post-object setup:
  - `gtt_state_range_sgx_object_count=5`
  - `gtt_state_range_pf_bound_count=5`
- post-owner setup:
  - `submit_owner_state_name=queued`
- post-validate:
  - `submit_validate_count=1`
- post-issue:
  - `submit_issue_count=1`
  - `coord_issue_submit_id=32`
- post-running:
  - `submit_transition_count=1`
  - `submit_owner_state_name=running`
  - `coord_running_submit_id=32`
- post-complete:
  - `submit_transition_count=2`
  - `submit_owner_state_name=complete`
  - `coord_complete_submit_id=32`
- post-drop and final cleanup:
  - `coord_live_submit_count=0`
  - `gtt_state_range_sgx_object_count=0`
  - `gtt_state_range_pf_bound_count=0`

## Conclusion

The older focused helper names now default onto the bounded typed object and
submit validators instead of silently routing callers into the sysfs-heavy
bring-up path. The remaining direct sysfs behavior for those helpers is now
explicit and opt-in through `SGX_OBJECT_DRIVER_MODE=legacy` and
`SGX_SUBMIT_DRIVER_MODE=legacy`.
