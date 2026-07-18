# SGX submit TA submit-op request-endpoint cleanup

## Status

This batch is in progress only. It is deployed live on hardware, but it has
not yet completed the exact `complete`, focused typed submit, or typed
multi-submit proof cycle, and it is not committed.

## Goal

Remove the redundant shared top request wrapper so both parsed sysfs and ioctl
`ISSUE` / `TRANSITION` submit-entry paths target `request_frontend` directly.

## Local kernel change

Updated:

- `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`

Removed locally:

- `psb_sgx_lab_submit_sgx_ta_submit_op_request_endpoint`
- `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_endpoint()`

The new local top request path is:

- parsed sysfs `issue` path -> `sgx_ta_submit_op_request_frontend`
- parsed sysfs `transition` path -> `sgx_ta_submit_op_request_frontend`
- ioctl `ISSUE` path -> `sgx_ta_submit_op_request_frontend`
- ioctl `TRANSITION` path -> `sgx_ta_submit_op_request_frontend`

That leaves the uncommitted live top request boundary on:

- `request_frontend`

## Local build state

Local rebuild passed for the current uncommitted candidate:

- `srcversion=E3FF2FBD2A5915FB93A17F2`
- `override_module_size=506156`

## Live deploy state

The guarded deploy and reboot recovery for this uncommitted candidate completed
successfully in:

- `logs/vaio-plan/20260516-091311-sgx-ta-submit-op-request-endpoint-cleanup-deploy`

The target recovered on the new live override:

- `driver_srcversion=E3FF2FBD2A5915FB93A17F2`
- `override_module_size=506156`
- `driver_module_loaded=1`
- `overall=remote_usable`
- `lightdm_active=inactive`
- `kdump_service_active=active`

## Proof status

Still pending for this batch:

- exact no-redeploy `plan` proof at `complete`
- focused typed submit proof
- typed multi-submit proof
- patch refresh
- kernel subrepo commit
- top-level docs commit

The last committed validated baseline is still the previous batch:

- kernel subrepo `c1c41ae2f`
- top-level `ee5dcad1`
- validation bundle:
  - `logs/vaio-plan/20260516-015443-sgx-ta-submit-op-request-gateway-cleanup-deploy`
  - `logs/vaio-plan-session/20260516-015719-sgx-plan-session`
  - `logs/vaio-ioctl/20260516-015719-sgx-submit-ta-submit-op-request-gateway-cleanup-live/summary.log`
  - `logs/vaio-ioctl/20260516-015719-sgx-multi-submit-ta-submit-op-request-gateway-cleanup-live/summary.log`

## Resume steps

Run these next, in order:

1. Exact proof:

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete ./scripts/run-vaio-sgx-plan-session.sh
```

2. Focused typed submit proof:

```bash
d=$(./scripts/allocate-session-dir.sh /root/Poulsbo/logs/vaio-ioctl sgx-submit-ta-submit-op-request-endpoint-cleanup-live $(date +%Y%m%d-%H%M%S)); ./scripts/run-vaio-sgx-submit.sh >"$d/summary.log" 2>&1; printf '%s\n' "$d"
```

3. Typed multi-submit proof:

```bash
d=$(./scripts/allocate-session-dir.sh /root/Poulsbo/logs/vaio-ioctl sgx-multi-submit-ta-submit-op-request-endpoint-cleanup-live $(date +%Y%m%d-%H%M%S)); ./scripts/run-vaio-sgx-multi-submit-typed.sh >"$d/summary.log" 2>&1; printf '%s\n' "$d"
```

4. If all three stay green:

- refresh `patches/ubuntu-5.4/0001-drm-gma500-add-sgx-lab-instrumentation.patch`
- commit the kernel subrepo cleanup
- commit the top-level docs batch
