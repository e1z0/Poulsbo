# Poulsbo

Reverse-engineering bootstrap for a full open Linux graphics stack on Intel GMA 500 / Poulsbo:

- kernel display support on top of upstream `gma500`
- SGX535 render support via new kernel uAPI and command submission
- Mesa Gallium driver for GLES 2.0 first, desktop GL 2.x where realistic
- Xorg acceleration through `modesetting` + `glamor`

This repository is not pretending the driver already exists. It is the project skeleton needed to build it without wasting months on vague planning.

## What This Repo Contains

- a phased reverse-engineering plan
- kernel/Mesa/Xorg design notes
- clean-room operating rules
- testing and bring-up criteria
- target capture scripts
- a starter MMIO register database and annotation tool

## Current Technical Position

- Upstream Linux still ships and maintains `drivers/gpu/drm/gma500/`, but it is effectively a display/KMS driver, not a full render stack.
- The current upstream driver exposes modesetting and GEM, but no real SGX535 userspace render path.
- Xorg already has the right high-level path: `modesetting` plus `glamor`. The missing piece is a working Mesa driver.
- Mesa's current open PowerVR work targets newer Rogue GPUs, not SGX535.
- The SGX lab path still reaches execution-shaped
  `prepare -> arm -> emit -> launch -> dispatch -> retired`, keeps typed
  memory-backed `report`, `live`, `result`, `manifest`, `descriptor`, `plan`,
  and `issue-plan` blocks, and now routes the top submit-op execution boundary
  through kernel-internal
  `psb_sgx_lab_submit_sgx_ta_request_work_response`,
  `psb_sgx_lab_submit_sgx_ta_submit_op_reply`,
  `psb_sgx_lab_submit_sgx_ta_submit_op_control`,
  `psb_sgx_lab_submit_sgx_ta_submit_op_ioctl`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_entry`,
  `psb_sgx_lab_submit_sgx_ta_submit_op_call`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_entrypoint`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_route`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_result`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_dispatch`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_handler`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_handler`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_wrapper`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_entry`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_call`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_dispatch`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_result`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_response`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_return`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_resolution`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_operation`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_control`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_path`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_report`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_trace`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_frontend`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_endpoint`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_gateway`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_portal`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_bridge`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_relay`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_terminal`, and now
  `psb_sgx_lab_submit_sgx_ta_submit_op_request_ioctl`. The top
  submit-op request path is now intentionally split four ways:
  the parsed sysfs submit-store seam now sits directly on `request_bridge`,
  the ioctl public caller seam now sits directly on `request_ioctl`,
  `request_ioctl` now sits directly on
  `request_terminal` and owns the final ioctl `ISSUE` / `TRANSITION` switch
  seam, `request_bridge` keeps the copied outer wrapper seam above
  `request_portal`, `request_relay` keeps the copied ioctl-side request /
  response / outcome return seam above `request_portal`, and
  `request_terminal` lifts the final ioctl-side `ISSUE` / `TRANSITION`
  caller seam above `request_relay`. Hardware
  validation on the current batch is saved at
  [logs/vaio-plan/20260516-000927-sgx-ta-submit-op-request-alias-cleanup-2-deploy](/root/Poulsbo/logs/vaio-plan/20260516-000927-sgx-ta-submit-op-request-alias-cleanup-2-deploy),
  [logs/vaio-plan-session/20260516-001447-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260516-001447-sgx-plan-session),
  [logs/vaio-ioctl/20260516-001211-sgx-submit-ta-submit-op-request-alias-cleanup-2-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260516-001211-sgx-submit-ta-submit-op-request-alias-cleanup-2-live/summary.log),
  and
  [logs/vaio-ioctl/20260516-001249-sgx-multi-submit-ta-submit-op-request-alias-cleanup-2-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260516-001249-sgx-multi-submit-ta-submit-op-request-alias-cleanup-2-live/summary.log),
  leaving the VAIO on override `srcversion C2F79F87DD5B96B4419D95A`, size
  `509052`, still headless, with the validated deploy bundle recording
  `kdump_service_active=active`. See
  [docs/183-sgx-submit-ta-schedule-object.md](/root/Poulsbo/docs/183-sgx-submit-ta-schedule-object.md),
  [docs/184-sgx-submit-ta-dispatch-object.md](/root/Poulsbo/docs/184-sgx-submit-ta-dispatch-object.md),
  [docs/185-sgx-submit-ta-path-object.md](/root/Poulsbo/docs/185-sgx-submit-ta-path-object.md),
  [docs/186-sgx-submit-ta-path-issue-unification.md](/root/Poulsbo/docs/186-sgx-submit-ta-path-issue-unification.md),
  [docs/187-sgx-submit-ta-commit-object.md](/root/Poulsbo/docs/187-sgx-submit-ta-commit-object.md),
  [docs/188-sgx-submit-ta-event-object.md](/root/Poulsbo/docs/188-sgx-submit-ta-event-object.md),
  [docs/189-sgx-submit-ta-work-item-object.md](/root/Poulsbo/docs/189-sgx-submit-ta-work-item-object.md),
  [docs/190-sgx-submit-ta-op-object.md](/root/Poulsbo/docs/190-sgx-submit-ta-op-object.md),
  [docs/191-sgx-submit-ta-scheduler-object.md](/root/Poulsbo/docs/191-sgx-submit-ta-scheduler-object.md),
  [docs/192-sgx-submit-ta-sequence-object.md](/root/Poulsbo/docs/192-sgx-submit-ta-sequence-object.md),
  [docs/193-sgx-submit-ta-work-queue-object.md](/root/Poulsbo/docs/193-sgx-submit-ta-work-queue-object.md),
  [docs/194-sgx-submit-ta-job-object.md](/root/Poulsbo/docs/194-sgx-submit-ta-job-object.md),
  [docs/195-sgx-submit-ta-request-object.md](/root/Poulsbo/docs/195-sgx-submit-ta-request-object.md),
  [docs/196-sgx-submit-ta-ioctl-request-object.md](/root/Poulsbo/docs/196-sgx-submit-ta-ioctl-request-object.md),
  [docs/197-sgx-submit-ta-submit-op-request-object.md](/root/Poulsbo/docs/197-sgx-submit-ta-submit-op-request-object.md),
  [docs/198-sgx-submit-ta-queue-item-object.md](/root/Poulsbo/docs/198-sgx-submit-ta-queue-item-object.md),
  [docs/199-sgx-submit-ta-queue-command-object.md](/root/Poulsbo/docs/199-sgx-submit-ta-queue-command-object.md),
  [docs/200-sgx-submit-ta-scheduler-entry-object.md](/root/Poulsbo/docs/200-sgx-submit-ta-scheduler-entry-object.md),
  [docs/201-sgx-submit-ta-queue-stream-object.md](/root/Poulsbo/docs/201-sgx-submit-ta-queue-stream-object.md),
  [docs/202-sgx-submit-ta-queue-packet-object.md](/root/Poulsbo/docs/202-sgx-submit-ta-queue-packet-object.md),
  [docs/203-sgx-submit-ta-request-packet-object.md](/root/Poulsbo/docs/203-sgx-submit-ta-request-packet-object.md),
  [docs/204-sgx-submit-ta-request-sequence-object.md](/root/Poulsbo/docs/204-sgx-submit-ta-request-sequence-object.md),
  [docs/205-sgx-stable-lab-milestone.md](/root/Poulsbo/docs/205-sgx-stable-lab-milestone.md),
  [docs/206-sgx-submit-ta-request-work-packet-object.md](/root/Poulsbo/docs/206-sgx-submit-ta-request-work-packet-object.md),
  [docs/207-sgx-submit-ta-request-execution-object.md](/root/Poulsbo/docs/207-sgx-submit-ta-request-execution-object.md),
  [docs/208-sgx-submit-ta-request-work-entry-object.md](/root/Poulsbo/docs/208-sgx-submit-ta-request-work-entry-object.md),
  [docs/209-sgx-submit-ta-request-work-result-object.md](/root/Poulsbo/docs/209-sgx-submit-ta-request-work-result-object.md),
  [docs/210-sgx-submit-ta-request-work-record-object.md](/root/Poulsbo/docs/210-sgx-submit-ta-request-work-record-object.md),
  [docs/211-sgx-submit-ta-request-work-request-object.md](/root/Poulsbo/docs/211-sgx-submit-ta-request-work-request-object.md),
  [docs/212-sgx-submit-ta-request-work-operation-object.md](/root/Poulsbo/docs/212-sgx-submit-ta-request-work-operation-object.md),
  [docs/213-sgx-submit-ta-request-work-event-object.md](/root/Poulsbo/docs/213-sgx-submit-ta-request-work-event-object.md),
  [docs/214-sgx-submit-ta-request-work-state-object.md](/root/Poulsbo/docs/214-sgx-submit-ta-request-work-state-object.md),
  [docs/215-sgx-submit-ta-request-work-action-object.md](/root/Poulsbo/docs/215-sgx-submit-ta-request-work-action-object.md),
  [docs/216-sgx-submit-ta-request-work-task-object.md](/root/Poulsbo/docs/216-sgx-submit-ta-request-work-task-object.md),
  [docs/217-sgx-submit-ta-request-work-run-object.md](/root/Poulsbo/docs/217-sgx-submit-ta-request-work-run-object.md),
  [docs/218-sgx-submit-ta-request-work-invocation-object.md](/root/Poulsbo/docs/218-sgx-submit-ta-request-work-invocation-object.md),
  [docs/219-sgx-submit-ta-request-work-submit-op-object.md](/root/Poulsbo/docs/219-sgx-submit-ta-request-work-submit-op-object.md),
  [docs/220-sgx-submit-ta-request-work-outcome-object.md](/root/Poulsbo/docs/220-sgx-submit-ta-request-work-outcome-object.md),
  [docs/221-sgx-submit-ta-request-work-response-object.md](/root/Poulsbo/docs/221-sgx-submit-ta-request-work-response-object.md),
  [docs/222-sgx-submit-ta-submit-op-reply-object.md](/root/Poulsbo/docs/222-sgx-submit-ta-submit-op-reply-object.md),
  [docs/223-sgx-submit-ta-submit-op-control-object.md](/root/Poulsbo/docs/223-sgx-submit-ta-submit-op-control-object.md),
  [docs/224-sgx-submit-ta-submit-op-ioctl-object.md](/root/Poulsbo/docs/224-sgx-submit-ta-submit-op-ioctl-object.md),
  and
  [docs/225-sgx-submit-ta-submit-op-entry-object.md](/root/Poulsbo/docs/225-sgx-submit-ta-submit-op-entry-object.md),
  and
  [docs/226-sgx-submit-ta-submit-op-call-object.md](/root/Poulsbo/docs/226-sgx-submit-ta-submit-op-call-object.md),
  and
  [docs/227-sgx-submit-ta-submit-op-entrypoint-object.md](/root/Poulsbo/docs/227-sgx-submit-ta-submit-op-entrypoint-object.md),
  and
  [docs/228-sgx-submit-ta-submit-op-route-object.md](/root/Poulsbo/docs/228-sgx-submit-ta-submit-op-route-object.md),
  and
  [docs/229-sgx-submit-ta-submit-op-result-object.md](/root/Poulsbo/docs/229-sgx-submit-ta-submit-op-result-object.md),
  and
  [docs/230-sgx-submit-ta-submit-op-dispatch-object.md](/root/Poulsbo/docs/230-sgx-submit-ta-submit-op-dispatch-object.md),
  and
  [docs/231-sgx-submit-ta-submit-op-handler-object.md](/root/Poulsbo/docs/231-sgx-submit-ta-submit-op-handler-object.md),
  and
  [docs/232-sgx-submit-ta-submit-op-request-handler-object.md](/root/Poulsbo/docs/232-sgx-submit-ta-submit-op-request-handler-object.md),
  and
  [docs/233-sgx-submit-ta-submit-op-request-wrapper-object.md](/root/Poulsbo/docs/233-sgx-submit-ta-submit-op-request-wrapper-object.md),
  [docs/234-sgx-submit-ta-submit-op-request-entry-object.md](/root/Poulsbo/docs/234-sgx-submit-ta-submit-op-request-entry-object.md),
  and
  [docs/235-sgx-submit-ta-submit-op-request-call-object.md](/root/Poulsbo/docs/235-sgx-submit-ta-submit-op-request-call-object.md),
  and
  [docs/236-sgx-submit-ta-submit-op-request-dispatch-object.md](/root/Poulsbo/docs/236-sgx-submit-ta-submit-op-request-dispatch-object.md),
  and
  [docs/237-sgx-submit-ta-submit-op-request-result-object.md](/root/Poulsbo/docs/237-sgx-submit-ta-submit-op-request-result-object.md),
  and
  [docs/238-sgx-submit-ta-submit-op-request-response-object.md](/root/Poulsbo/docs/238-sgx-submit-ta-submit-op-request-response-object.md),
  and
  [docs/239-sgx-submit-ta-submit-op-request-return-object.md](/root/Poulsbo/docs/239-sgx-submit-ta-submit-op-request-return-object.md),
  and
  [docs/240-sgx-submit-ta-submit-op-request-resolution-object.md](/root/Poulsbo/docs/240-sgx-submit-ta-submit-op-request-resolution-object.md),
  and
  [docs/241-sgx-submit-ta-submit-op-request-operation-object.md](/root/Poulsbo/docs/241-sgx-submit-ta-submit-op-request-operation-object.md),
  and
  [docs/242-sgx-submit-ta-submit-op-request-control-object.md](/root/Poulsbo/docs/242-sgx-submit-ta-submit-op-request-control-object.md),
  and
  [docs/243-sgx-submit-ta-submit-op-request-path-object.md](/root/Poulsbo/docs/243-sgx-submit-ta-submit-op-request-path-object.md),
  and
  [docs/244-sgx-submit-ta-submit-op-request-report-object.md](/root/Poulsbo/docs/244-sgx-submit-ta-submit-op-request-report-object.md),
  and
  [docs/245-sgx-submit-ta-submit-op-request-trace-object.md](/root/Poulsbo/docs/245-sgx-submit-ta-submit-op-request-trace-object.md),
  and
  [docs/246-sgx-submit-ta-submit-op-request-frontend-object.md](/root/Poulsbo/docs/246-sgx-submit-ta-submit-op-request-frontend-object.md),
  and
  [docs/247-sgx-submit-ta-submit-op-request-endpoint-object.md](/root/Poulsbo/docs/247-sgx-submit-ta-submit-op-request-endpoint-object.md),
  and
  [docs/248-sgx-submit-ta-submit-op-request-gateway-object.md](/root/Poulsbo/docs/248-sgx-submit-ta-submit-op-request-gateway-object.md),
  [docs/249-sgx-submit-ta-submit-op-request-portal-object.md](/root/Poulsbo/docs/249-sgx-submit-ta-submit-op-request-portal-object.md),
  and
  [docs/250-sgx-submit-ta-submit-op-request-bridge-object.md](/root/Poulsbo/docs/250-sgx-submit-ta-submit-op-request-bridge-object.md),
  and
  [docs/251-sgx-submit-ta-submit-op-request-relay-object.md](/root/Poulsbo/docs/251-sgx-submit-ta-submit-op-request-relay-object.md),
  and
  [docs/252-sgx-submit-ta-submit-op-request-terminal-object.md](/root/Poulsbo/docs/252-sgx-submit-ta-submit-op-request-terminal-object.md),
  and
  [docs/253-sgx-submit-ta-submit-op-request-adapter-object.md](/root/Poulsbo/docs/253-sgx-submit-ta-submit-op-request-adapter-object.md),
  and
  [docs/254-sgx-submit-ta-submit-op-request-alias-object.md](/root/Poulsbo/docs/254-sgx-submit-ta-submit-op-request-alias-object.md),
  and
  [docs/255-sgx-submit-ta-submit-op-request-public-object.md](/root/Poulsbo/docs/255-sgx-submit-ta-submit-op-request-public-object.md),
  and
  [docs/256-sgx-submit-ta-submit-op-request-ioctl-object.md](/root/Poulsbo/docs/256-sgx-submit-ta-submit-op-request-ioctl-object.md),
  and
  [docs/257-sgx-submit-ta-submit-op-request-entrypoint-object.md](/root/Poulsbo/docs/257-sgx-submit-ta-submit-op-request-entrypoint-object.md),
  and
  [docs/258-sgx-submit-ta-submit-op-request-origin-object.md](/root/Poulsbo/docs/258-sgx-submit-ta-submit-op-request-origin-object.md),
  and
  [docs/259-sgx-submit-ta-submit-op-request-command-object.md](/root/Poulsbo/docs/259-sgx-submit-ta-submit-op-request-command-object.md),
  and
  [docs/260-sgx-submit-ta-submit-op-request-sysfs-object.md](/root/Poulsbo/docs/260-sgx-submit-ta-submit-op-request-sysfs-object.md),
  [docs/261-sgx-submit-ta-submit-op-request-store-object.md](/root/Poulsbo/docs/261-sgx-submit-ta-submit-op-request-store-object.md),
  [docs/262-sgx-submit-ta-submit-op-request-store-cleanup.md](/root/Poulsbo/docs/262-sgx-submit-ta-submit-op-request-store-cleanup.md),
  [docs/263-sgx-submit-ta-submit-op-request-ioctl-cleanup.md](/root/Poulsbo/docs/263-sgx-submit-ta-submit-op-request-ioctl-cleanup.md),
  and
  [docs/264-sgx-submit-ta-submit-op-request-alias-cleanup.md](/root/Poulsbo/docs/264-sgx-submit-ta-submit-op-request-alias-cleanup.md),
  and
  [docs/265-sgx-submit-ta-submit-op-request-store-cleanup-2.md](/root/Poulsbo/docs/265-sgx-submit-ta-submit-op-request-store-cleanup-2.md),
  and
  [docs/266-sgx-submit-ta-submit-op-request-store-cleanup-3.md](/root/Poulsbo/docs/266-sgx-submit-ta-submit-op-request-store-cleanup-3.md),
  and
  [docs/267-sgx-submit-ta-submit-op-request-origin-cleanup.md](/root/Poulsbo/docs/267-sgx-submit-ta-submit-op-request-origin-cleanup.md),
  and
  [docs/268-sgx-submit-ta-submit-op-request-entrypoint-cleanup.md](/root/Poulsbo/docs/268-sgx-submit-ta-submit-op-request-entrypoint-cleanup.md),
  and
  [docs/269-sgx-submit-ta-submit-op-request-alias-cleanup-2.md](/root/Poulsbo/docs/269-sgx-submit-ta-submit-op-request-alias-cleanup-2.md).
- The stable milestone for today is not a desktop-ready driver; it is a pinned VAIO SGX lab baseline on `srcversion 92CB8FBC111248DC82C2825`. That exact build was restored, redeployed by [logs/vaio-deploy/20260514-082755-sgx-stable-milestone-deploy](/root/Poulsbo/logs/vaio-deploy/20260514-082755-sgx-stable-milestone-deploy), and then held through a three-cycle exact/focused/multi soak driven by [scripts/run-vaio-sgx-stable-soak-session.sh](/root/Poulsbo/scripts/run-vaio-sgx-stable-soak-session.sh) in [logs/vaio-stability/20260514-083309-sgx-stable-soak](/root/Poulsbo/logs/vaio-stability/20260514-083309-sgx-stable-soak), finishing `overall=remote_usable`. See [docs/205-sgx-stable-lab-milestone.md](/root/Poulsbo/docs/205-sgx-stable-lab-milestone.md).
- For offline triage between live reruns, the preferred comparison view is now [scripts/summarize-vaio-plan-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-plan-sessions.sh), which flattens top-level `plan` sessions, guarded deploy parents, recovery wrappers, and direct submit-op phase investigations into one diagnosis table and now carries bisect evidence directly as `evidence_last_started`, `evidence_last_good`, `evidence_first_bad`, and `evidence_reason`. See [docs/145-sgx-plan-diagnosis-summary-script.md](/root/Poulsbo/docs/145-sgx-plan-diagnosis-summary-script.md).
- The preferred single-command current-state view remains [scripts/report-vaio-plan-status.sh](/root/Poulsbo/scripts/report-vaio-plan-status.sh). It now combines live headless state with saved diagnosis bundles, treats the old `0EEEE302E3FDA9F59B7CFB9` module as unsafe for recovery-side `plan_query*` work, advances from successful exact evidence, and preserves a clean terminal `complete` state once the exact ladder is exhausted. On the current `plan` line the saved boundary therefore resolves to `complete` from the latest clean terminal session at [logs/vaio-plan-session/20260515-184012-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260515-184012-sgx-plan-session), on live override `C1DDCA50137885C2202893B`. See [docs/146-sgx-plan-status-report-and-live-plan-query-rerun.md](/root/Poulsbo/docs/146-sgx-plan-status-report-and-live-plan-query-rerun.md), [docs/147-sgx-plan-current-boundary-resolver-and-auto-recovery-default.md](/root/Poulsbo/docs/147-sgx-plan-current-boundary-resolver-and-auto-recovery-default.md), [docs/148-sgx-plan-request-resolved-source-provenance.md](/root/Poulsbo/docs/148-sgx-plan-request-resolved-source-provenance.md), [docs/149-sgx-plan-header-driven-summary-parser-hardening.md](/root/Poulsbo/docs/149-sgx-plan-header-driven-summary-parser-hardening.md), [docs/150-sgx-plan-shared-summary-library.md](/root/Poulsbo/docs/150-sgx-plan-shared-summary-library.md), [docs/151-sgx-plan-structured-phase-source-columns.md](/root/Poulsbo/docs/151-sgx-plan-structured-phase-source-columns.md), [docs/152-sgx-plan-persisted-source-row-propagation.md](/root/Poulsbo/docs/152-sgx-plan-persisted-source-row-propagation.md), [docs/153-sgx-plan-phase-source-helper-and-evidence-selector-hardening.md](/root/Poulsbo/docs/153-sgx-plan-phase-source-helper-and-evidence-selector-hardening.md), [docs/154-sgx-plan-terminal-bad-probe-boundary-fix.md](/root/Poulsbo/docs/154-sgx-plan-terminal-bad-probe-boundary-fix.md), [docs/205-sgx-stable-lab-milestone.md](/root/Poulsbo/docs/205-sgx-stable-lab-milestone.md), [docs/206-sgx-submit-ta-request-work-packet-object.md](/root/Poulsbo/docs/206-sgx-submit-ta-request-work-packet-object.md), [docs/207-sgx-submit-ta-request-execution-object.md](/root/Poulsbo/docs/207-sgx-submit-ta-request-execution-object.md), [docs/208-sgx-submit-ta-request-work-entry-object.md](/root/Poulsbo/docs/208-sgx-submit-ta-request-work-entry-object.md), [docs/209-sgx-submit-ta-request-work-result-object.md](/root/Poulsbo/docs/209-sgx-submit-ta-request-work-result-object.md), [docs/210-sgx-submit-ta-request-work-record-object.md](/root/Poulsbo/docs/210-sgx-submit-ta-request-work-record-object.md), [docs/211-sgx-submit-ta-request-work-request-object.md](/root/Poulsbo/docs/211-sgx-submit-ta-request-work-request-object.md), [docs/212-sgx-submit-ta-request-work-operation-object.md](/root/Poulsbo/docs/212-sgx-submit-ta-request-work-operation-object.md), [docs/213-sgx-submit-ta-request-work-event-object.md](/root/Poulsbo/docs/213-sgx-submit-ta-request-work-event-object.md), [docs/214-sgx-submit-ta-request-work-state-object.md](/root/Poulsbo/docs/214-sgx-submit-ta-request-work-state-object.md), [docs/215-sgx-submit-ta-request-work-action-object.md](/root/Poulsbo/docs/215-sgx-submit-ta-request-work-action-object.md), [docs/216-sgx-submit-ta-request-work-task-object.md](/root/Poulsbo/docs/216-sgx-submit-ta-request-work-task-object.md), [docs/217-sgx-submit-ta-request-work-run-object.md](/root/Poulsbo/docs/217-sgx-submit-ta-request-work-run-object.md), [docs/218-sgx-submit-ta-request-work-invocation-object.md](/root/Poulsbo/docs/218-sgx-submit-ta-request-work-invocation-object.md), [docs/219-sgx-submit-ta-request-work-submit-op-object.md](/root/Poulsbo/docs/219-sgx-submit-ta-request-work-submit-op-object.md), [docs/220-sgx-submit-ta-request-work-outcome-object.md](/root/Poulsbo/docs/220-sgx-submit-ta-request-work-outcome-object.md), [docs/221-sgx-submit-ta-request-work-response-object.md](/root/Poulsbo/docs/221-sgx-submit-ta-request-work-response-object.md), [docs/222-sgx-submit-ta-submit-op-reply-object.md](/root/Poulsbo/docs/222-sgx-submit-ta-submit-op-reply-object.md), [docs/223-sgx-submit-ta-submit-op-control-object.md](/root/Poulsbo/docs/223-sgx-submit-ta-submit-op-control-object.md), [docs/224-sgx-submit-ta-submit-op-ioctl-object.md](/root/Poulsbo/docs/224-sgx-submit-ta-submit-op-ioctl-object.md), [docs/225-sgx-submit-ta-submit-op-entry-object.md](/root/Poulsbo/docs/225-sgx-submit-ta-submit-op-entry-object.md), [docs/226-sgx-submit-ta-submit-op-call-object.md](/root/Poulsbo/docs/226-sgx-submit-ta-submit-op-call-object.md), [docs/227-sgx-submit-ta-submit-op-entrypoint-object.md](/root/Poulsbo/docs/227-sgx-submit-ta-submit-op-entrypoint-object.md), [docs/228-sgx-submit-ta-submit-op-route-object.md](/root/Poulsbo/docs/228-sgx-submit-ta-submit-op-route-object.md), [docs/229-sgx-submit-ta-submit-op-result-object.md](/root/Poulsbo/docs/229-sgx-submit-ta-submit-op-result-object.md), [docs/230-sgx-submit-ta-submit-op-dispatch-object.md](/root/Poulsbo/docs/230-sgx-submit-ta-submit-op-dispatch-object.md), [docs/231-sgx-submit-ta-submit-op-handler-object.md](/root/Poulsbo/docs/231-sgx-submit-ta-submit-op-handler-object.md), [docs/232-sgx-submit-ta-submit-op-request-handler-object.md](/root/Poulsbo/docs/232-sgx-submit-ta-submit-op-request-handler-object.md), [docs/233-sgx-submit-ta-submit-op-request-wrapper-object.md](/root/Poulsbo/docs/233-sgx-submit-ta-submit-op-request-wrapper-object.md), [docs/234-sgx-submit-ta-submit-op-request-entry-object.md](/root/Poulsbo/docs/234-sgx-submit-ta-submit-op-request-entry-object.md), [docs/235-sgx-submit-ta-submit-op-request-call-object.md](/root/Poulsbo/docs/235-sgx-submit-ta-submit-op-request-call-object.md), [docs/236-sgx-submit-ta-submit-op-request-dispatch-object.md](/root/Poulsbo/docs/236-sgx-submit-ta-submit-op-request-dispatch-object.md), [docs/237-sgx-submit-ta-submit-op-request-result-object.md](/root/Poulsbo/docs/237-sgx-submit-ta-submit-op-request-result-object.md), [docs/238-sgx-submit-ta-submit-op-request-response-object.md](/root/Poulsbo/docs/238-sgx-submit-ta-submit-op-request-response-object.md), [docs/239-sgx-submit-ta-submit-op-request-return-object.md](/root/Poulsbo/docs/239-sgx-submit-ta-submit-op-request-return-object.md), [docs/240-sgx-submit-ta-submit-op-request-resolution-object.md](/root/Poulsbo/docs/240-sgx-submit-ta-submit-op-request-resolution-object.md), [docs/241-sgx-submit-ta-submit-op-request-operation-object.md](/root/Poulsbo/docs/241-sgx-submit-ta-submit-op-request-operation-object.md]), [docs/242-sgx-submit-ta-submit-op-request-control-object.md](/root/Poulsbo/docs/242-sgx-submit-ta-submit-op-request-control-object.md), [docs/243-sgx-submit-ta-submit-op-request-path-object.md](/root/Poulsbo/docs/243-sgx-submit-ta-submit-op-request-path-object.md]), [docs/244-sgx-submit-ta-submit-op-request-report-object.md](/root/Poulsbo/docs/244-sgx-submit-ta-submit-op-request-report-object.md), [docs/245-sgx-submit-ta-submit-op-request-trace-object.md](/root/Poulsbo/docs/245-sgx-submit-ta-submit-op-request-trace-object.md]), [docs/246-sgx-submit-ta-submit-op-request-frontend-object.md](/root/Poulsbo/docs/246-sgx-submit-ta-submit-op-request-frontend-object.md]), [docs/247-sgx-submit-ta-submit-op-request-endpoint-object.md](/root/Poulsbo/docs/247-sgx-submit-ta-submit-op-request-endpoint-object.md]), [docs/248-sgx-submit-ta-submit-op-request-gateway-object.md](/root/Poulsbo/docs/248-sgx-submit-ta-submit-op-request-gateway-object.md]), [docs/249-sgx-submit-ta-submit-op-request-portal-object.md](/root/Poulsbo/docs/249-sgx-submit-ta-submit-op-request-portal-object.md]), [docs/250-sgx-submit-ta-submit-op-request-bridge-object.md](/root/Poulsbo/docs/250-sgx-submit-ta-submit-op-request-bridge-object.md]), [docs/251-sgx-submit-ta-submit-op-request-relay-object.md](/root/Poulsbo/docs/251-sgx-submit-ta-submit-op-request-relay-object.md]), [docs/252-sgx-submit-ta-submit-op-request-terminal-object.md](/root/Poulsbo/docs/252-sgx-submit-ta-submit-op-request-terminal-object.md]), [docs/253-sgx-submit-ta-submit-op-request-adapter-object.md](/root/Poulsbo/docs/253-sgx-submit-ta-submit-op-request-adapter-object.md]), [docs/254-sgx-submit-ta-submit-op-request-alias-object.md](/root/Poulsbo/docs/254-sgx-submit-ta-submit-op-request-alias-object.md]), [docs/255-sgx-submit-ta-submit-op-request-public-object.md](/root/Poulsbo/docs/255-sgx-submit-ta-submit-op-request-public-object.md]), [docs/256-sgx-submit-ta-submit-op-request-ioctl-object.md](/root/Poulsbo/docs/256-sgx-submit-ta-submit-op-request-ioctl-object.md]), [docs/257-sgx-submit-ta-submit-op-request-entrypoint-object.md](/root/Poulsbo/docs/257-sgx-submit-ta-submit-op-request-entrypoint-object.md]), [docs/258-sgx-submit-ta-submit-op-request-origin-object.md](/root/Poulsbo/docs/258-sgx-submit-ta-submit-op-request-origin-object.md]), and [docs/259-sgx-submit-ta-submit-op-request-command-object.md](/root/Poulsbo/docs/259-sgx-submit-ta-submit-op-request-command-object.md]).

That means the real work is:

1. reverse engineer SGX535 command submission and memory management
2. extend `gma500` with a render uAPI
3. write a Mesa Gallium driver
4. validate Xorg acceleration on top

## Repo Layout

- [docs/00-overview.md](/root/Poulsbo/docs/00-overview.md)
- [docs/01-reverse-engineering-plan.md](/root/Poulsbo/docs/01-reverse-engineering-plan.md)
- [docs/02-kernel-driver-design.md](/root/Poulsbo/docs/02-kernel-driver-design.md)
- [docs/03-mesa-driver-design.md](/root/Poulsbo/docs/03-mesa-driver-design.md)
- [docs/04-xorg-path.md](/root/Poulsbo/docs/04-xorg-path.md)
- [docs/05-hardware-notes.md](/root/Poulsbo/docs/05-hardware-notes.md)
- [docs/06-clean-room-rules.md](/root/Poulsbo/docs/06-clean-room-rules.md)
- [docs/07-testing-plan.md](/root/Poulsbo/docs/07-testing-plan.md)
- [docs/08-upstream-code-map.md](/root/Poulsbo/docs/08-upstream-code-map.md)
- [docs/09-first-hardware-session.md](/root/Poulsbo/docs/09-first-hardware-session.md)
- [docs/10-ubuntu-18.04-target.md](/root/Poulsbo/docs/10-ubuntu-18.04-target.md)
- [docs/11-progress-log.md](/root/Poulsbo/docs/11-progress-log.md)
- [docs/12-active-todo.md](/root/Poulsbo/docs/12-active-todo.md)
- [docs/13-gem-mmu-audit.md](/root/Poulsbo/docs/13-gem-mmu-audit.md)
- [docs/14-source-triage.md](/root/Poulsbo/docs/14-source-triage.md)
- [docs/15-pf-pd-bind-lab.md](/root/Poulsbo/docs/15-pf-pd-bind-lab.md)
- [docs/16-sgx-lab-objects.md](/root/Poulsbo/docs/16-sgx-lab-objects.md)
- [docs/17-sgx-lab-submit-path.md](/root/Poulsbo/docs/17-sgx-lab-submit-path.md)
- [docs/18-sgx-lab-kick-packet.md](/root/Poulsbo/docs/18-sgx-lab-kick-packet.md)
- [docs/19-sgx-object-address-semantics.md](/root/Poulsbo/docs/19-sgx-object-address-semantics.md)
- [docs/20-sgx-control-block-model.md](/root/Poulsbo/docs/20-sgx-control-block-model.md)
- [docs/21-sgx-status-block-model.md](/root/Poulsbo/docs/21-sgx-status-block-model.md)
- [docs/22-sgx-queue-model.md](/root/Poulsbo/docs/22-sgx-queue-model.md)
- [docs/23-sgx-queue-wrap-model.md](/root/Poulsbo/docs/23-sgx-queue-wrap-model.md)
- [docs/24-sgx-sync-block-model.md](/root/Poulsbo/docs/24-sgx-sync-block-model.md)
- [docs/25-vaio-ssh-stall-recovery.md](/root/Poulsbo/docs/25-vaio-ssh-stall-recovery.md)
- [docs/26-sgx-owner-block-model.md](/root/Poulsbo/docs/26-sgx-owner-block-model.md)
- [docs/27-sgx-transition-model.md](/root/Poulsbo/docs/27-sgx-transition-model.md)
- [docs/28-sgx-issue-model.md](/root/Poulsbo/docs/28-sgx-issue-model.md)
- [docs/29-sgx-queue-cycle-model.md](/root/Poulsbo/docs/29-sgx-queue-cycle-model.md)
- [docs/30-sgx-multi-epoch-cycle-model.md](/root/Poulsbo/docs/30-sgx-multi-epoch-cycle-model.md)
- [docs/31-sgx-multi-submit-isolation.md](/root/Poulsbo/docs/31-sgx-multi-submit-isolation.md)
- [docs/32-sgx-global-submit-coordination.md](/root/Poulsbo/docs/32-sgx-global-submit-coordination.md)
- [docs/33-sgx-running-owner-arbitration.md](/root/Poulsbo/docs/33-sgx-running-owner-arbitration.md)
- [docs/34-sgx-completion-dependency-gating.md](/root/Poulsbo/docs/34-sgx-completion-dependency-gating.md)
- [docs/35-sgx-source-submit-dependency.md](/root/Poulsbo/docs/35-sgx-source-submit-dependency.md)
- [docs/36-sgx-dependency-state-block.md](/root/Poulsbo/docs/36-sgx-dependency-state-block.md)
- [docs/37-sgx-live-dependency-block.md](/root/Poulsbo/docs/37-sgx-live-dependency-block.md)
- [docs/38-sgx-dependency-source-drop-state.md](/root/Poulsbo/docs/38-sgx-dependency-source-drop-state.md)
- [docs/39-sgx-satisfied-source-drop-state.md](/root/Poulsbo/docs/39-sgx-satisfied-source-drop-state.md)
- [docs/40-sgx-unrelated-submit-stability.md](/root/Poulsbo/docs/40-sgx-unrelated-submit-stability.md)
- [docs/41-sgx-multi-source-dependency-gating.md](/root/Poulsbo/docs/41-sgx-multi-source-dependency-gating.md)
- [docs/42-sgx-per-lane-ticket-gating.md](/root/Poulsbo/docs/42-sgx-per-lane-ticket-gating.md)
- [docs/43-sgx-third-dependency-lane.md](/root/Poulsbo/docs/43-sgx-third-dependency-lane.md)
- [docs/44-sgx-indexed-dependency-interface.md](/root/Poulsbo/docs/44-sgx-indexed-dependency-interface.md)
- [docs/45-sgx-fourth-indexed-dependency-slot.md](/root/Poulsbo/docs/45-sgx-fourth-indexed-dependency-slot.md)
- [docs/46-sgx-dependency-slot-clear.md](/root/Poulsbo/docs/46-sgx-dependency-slot-clear.md)
- [docs/47-vaio-auto-resume-helper.md](/root/Poulsbo/docs/47-vaio-auto-resume-helper.md)
- [docs/48-vaio-boot-rollback-guard.md](/root/Poulsbo/docs/48-vaio-boot-rollback-guard.md)
- [docs/49-vaio-state-inspection-helper.md](/root/Poulsbo/docs/49-vaio-state-inspection-helper.md)
- [docs/50-vaio-health-timeline-watcher.md](/root/Poulsbo/docs/50-vaio-health-timeline-watcher.md)
- [docs/51-vaio-recovery-session-wrapper.md](/root/Poulsbo/docs/51-vaio-recovery-session-wrapper.md)
- [docs/52-vaio-guarded-deploy-session-wrapper.md](/root/Poulsbo/docs/52-vaio-guarded-deploy-session-wrapper.md)
- [docs/53-vaio-session-summary-helper.md](/root/Poulsbo/docs/53-vaio-session-summary-helper.md)
- [docs/54-sgx-dependency-slot-clear-validation.md](/root/Poulsbo/docs/54-sgx-dependency-slot-clear-validation.md)
- [docs/55-vaio-rollback-guard-healthy-boot-validation.md](/root/Poulsbo/docs/55-vaio-rollback-guard-healthy-boot-validation.md)
- [docs/56-vaio-rollback-guard-autorestore-validation.md](/root/Poulsbo/docs/56-vaio-rollback-guard-autorestore-validation.md)
- [docs/57-sgx-lab-ioctl-query.md](/root/Poulsbo/docs/57-sgx-lab-ioctl-query.md)
- [docs/58-sgx-lab-submit-op-ioctl.md](/root/Poulsbo/docs/58-sgx-lab-submit-op-ioctl.md)
- [docs/59-sgx-lab-transition-op-ioctl.md](/root/Poulsbo/docs/59-sgx-lab-transition-op-ioctl.md)
- [docs/60-sgx-lab-lifecycle-op-ioctl.md](/root/Poulsbo/docs/60-sgx-lab-lifecycle-op-ioctl.md)
- [docs/61-sgx-lab-object-op-ioctl.md](/root/Poulsbo/docs/61-sgx-lab-object-op-ioctl.md)
- [docs/62-sgx-lab-object-role-class-ioctl.md](/root/Poulsbo/docs/62-sgx-lab-object-role-class-ioctl.md)
- [docs/63-sgx-lab-submit-attach-op-ioctl.md](/root/Poulsbo/docs/63-sgx-lab-submit-attach-op-ioctl.md)
- [docs/64-sgx-lab-submit-kick-op-ioctl.md](/root/Poulsbo/docs/64-sgx-lab-submit-kick-op-ioctl.md)
- [docs/65-sgx-lab-submit-ccb-op-ioctl.md](/root/Poulsbo/docs/65-sgx-lab-submit-ccb-op-ioctl.md)
- [docs/66-sgx-lab-submit-queue-op-ioctl.md](/root/Poulsbo/docs/66-sgx-lab-submit-queue-op-ioctl.md)
- [docs/67-sgx-lab-submit-status-op-ioctl.md](/root/Poulsbo/docs/67-sgx-lab-submit-status-op-ioctl.md)
- [docs/68-sgx-lab-submit-sync-op-ioctl.md](/root/Poulsbo/docs/68-sgx-lab-submit-sync-op-ioctl.md)
- [docs/69-sgx-lab-submit-owner-op-ioctl.md](/root/Poulsbo/docs/69-sgx-lab-submit-owner-op-ioctl.md)
- [docs/70-sgx-lab-submit-dependency-slot-op-ioctl.md](/root/Poulsbo/docs/70-sgx-lab-submit-dependency-slot-op-ioctl.md)
- [docs/71-sgx-lab-multi-submit-submit-op-validation.md](/root/Poulsbo/docs/71-sgx-lab-multi-submit-submit-op-validation.md)
- [docs/72-sgx-lab-full-object-submit-ioctl-validation.md](/root/Poulsbo/docs/72-sgx-lab-full-object-submit-ioctl-validation.md)
- [docs/73-sgx-lab-range-query-ioctl.md](/root/Poulsbo/docs/73-sgx-lab-range-query-ioctl.md)
- [docs/74-sgx-lab-query-tools-raw-ioctl-validation.md](/root/Poulsbo/docs/74-sgx-lab-query-tools-raw-ioctl-validation.md)
- [docs/75-sgx-lab-sgx-state-query-ioctl.md](/root/Poulsbo/docs/75-sgx-lab-sgx-state-query-ioctl.md)
- [docs/76-sgx-lab-gtt-state-query-ioctl.md](/root/Poulsbo/docs/76-sgx-lab-gtt-state-query-ioctl.md)
- [docs/77-sgx-lab-cleanup-op-ioctl.md](/root/Poulsbo/docs/77-sgx-lab-cleanup-op-ioctl.md)
- [docs/78-sgx-lab-object-query-ioctl.md](/root/Poulsbo/docs/78-sgx-lab-object-query-ioctl.md)
- [docs/79-sgx-lab-submit-query-index-ioctl.md](/root/Poulsbo/docs/79-sgx-lab-submit-query-index-ioctl.md)
- [docs/80-sgx-lab-typed-state-dump-validation.md](/root/Poulsbo/docs/80-sgx-lab-typed-state-dump-validation.md)
- [docs/81-sgx-lab-dump-tool-validation.md](/root/Poulsbo/docs/81-sgx-lab-dump-tool-validation.md)
- [docs/82-sgx-lab-dump-json-validation.md](/root/Poulsbo/docs/82-sgx-lab-dump-json-validation.md)
- [docs/83-sgx-lab-cleanup-nosysfs-validation.md](/root/Poulsbo/docs/83-sgx-lab-cleanup-nosysfs-validation.md)
- [docs/84-sgx-lab-object-op-nosysfs-validation.md](/root/Poulsbo/docs/84-sgx-lab-object-op-nosysfs-validation.md)
- [docs/85-sgx-lab-submit-op-nosysfs-validation.md](/root/Poulsbo/docs/85-sgx-lab-submit-op-nosysfs-validation.md)
- [docs/86-sgx-lab-multi-submit-nosysfs-validation.md](/root/Poulsbo/docs/86-sgx-lab-multi-submit-nosysfs-validation.md)
- [docs/87-sgx-dependency-wrapper-typed-defaults.md](/root/Poulsbo/docs/87-sgx-dependency-wrapper-typed-defaults.md)
- [docs/88-sgx-focused-wrapper-typed-defaults.md](/root/Poulsbo/docs/88-sgx-focused-wrapper-typed-defaults.md)
- [docs/89-sgx-multi-submit-wrapper-mode-gate.md](/root/Poulsbo/docs/89-sgx-multi-submit-wrapper-mode-gate.md)
- [docs/90-sgx-legacy-mode-gate-validation.md](/root/Poulsbo/docs/90-sgx-legacy-mode-gate-validation.md)
- [docs/91-sgx-mode-compare-session-validation.md](/root/Poulsbo/docs/91-sgx-mode-compare-session-validation.md)
- [docs/92-sgx-dependency-wrapper-mode-compare-validation.md](/root/Poulsbo/docs/92-sgx-dependency-wrapper-mode-compare-validation.md)
- [docs/93-sgx-indexed-dependency-array-mode-compare-validation.md](/root/Poulsbo/docs/93-sgx-indexed-dependency-array-mode-compare-validation.md)
- [docs/94-sgx-relative-dependency-ticket-fix.md](/root/Poulsbo/docs/94-sgx-relative-dependency-ticket-fix.md)
- [docs/95-sgx-compare-reset-and-legacy-range-baseline-fix.md](/root/Poulsbo/docs/95-sgx-compare-reset-and-legacy-range-baseline-fix.md)
- [docs/96-sgx-focused-compare-reset-and-range-query-fix.md](/root/Poulsbo/docs/96-sgx-focused-compare-reset-and-range-query-fix.md)
- [docs/97-sgx-generic-multi-submit-compare-reset-follow-up.md](/root/Poulsbo/docs/97-sgx-generic-multi-submit-compare-reset-follow-up.md)
- [docs/98-sgx-lab-range-query-gpuva-validation.md](/root/Poulsbo/docs/98-sgx-lab-range-query-gpuva-validation.md)
- [docs/99-sgx-lab-range-query-gpuva-metadata-validation.md](/root/Poulsbo/docs/99-sgx-lab-range-query-gpuva-metadata-validation.md)
- [docs/100-sgx-lab-range-query-gpuva-submit-ref-validation.md](/root/Poulsbo/docs/100-sgx-lab-range-query-gpuva-submit-ref-validation.md)
- [docs/101-sgx-lab-submit-query-gpuva-validation.md](/root/Poulsbo/docs/101-sgx-lab-submit-query-gpuva-validation.md)
- [docs/102-sgx-lab-submit-query-gpuva-range-validation.md](/root/Poulsbo/docs/102-sgx-lab-submit-query-gpuva-range-validation.md)
- [docs/103-sgx-lab-submit-query-gpuva-object-validation.md](/root/Poulsbo/docs/103-sgx-lab-submit-query-gpuva-object-validation.md)
- [docs/104-sgx-lab-submit-query-gpuva-multi-ref-validation.md](/root/Poulsbo/docs/104-sgx-lab-submit-query-gpuva-multi-ref-validation.md)
- [docs/105-sgx-lab-submit-query-gpuva-match-validation.md](/root/Poulsbo/docs/105-sgx-lab-submit-query-gpuva-match-validation.md)
- [docs/106-sgx-lab-submit-query-gpuva-list-validation.md](/root/Poulsbo/docs/106-sgx-lab-submit-query-gpuva-list-validation.md)
- [docs/107-sgx-lab-range-query-submit-match-validation.md](/root/Poulsbo/docs/107-sgx-lab-range-query-submit-match-validation.md)
- [docs/108-sgx-lab-submit-prepare-op-ioctl.md](/root/Poulsbo/docs/108-sgx-lab-submit-prepare-op-ioctl.md)
- [docs/109-sgx-lab-submit-arm-op-ioctl.md](/root/Poulsbo/docs/109-sgx-lab-submit-arm-op-ioctl.md)
- [docs/110-sgx-lab-submit-emit-op-ioctl.md](/root/Poulsbo/docs/110-sgx-lab-submit-emit-op-ioctl.md)
- [docs/111-sgx-lab-submit-launch-op-ioctl.md](/root/Poulsbo/docs/111-sgx-lab-submit-launch-op-ioctl.md)
- [docs/112-sgx-lab-submit-dispatch-state.md](/root/Poulsbo/docs/112-sgx-lab-submit-dispatch-state.md)
- [docs/113-sgx-submit-cleanup-deadlock-fix.md](/root/Poulsbo/docs/113-sgx-submit-cleanup-deadlock-fix.md)
- [docs/114-sgx-multi-submit-legacy-guardrail.md](/root/Poulsbo/docs/114-sgx-multi-submit-legacy-guardrail.md)
- [docs/115-sgx-deadlock-fix-candidate-validation.md](/root/Poulsbo/docs/115-sgx-deadlock-fix-candidate-validation.md)
- [docs/116-sgx-lab-submit-report-op-ioctl.md](/root/Poulsbo/docs/116-sgx-lab-submit-report-op-ioctl.md)
- [docs/117-sgx-lab-ccb-record-expansion.md](/root/Poulsbo/docs/117-sgx-lab-ccb-record-expansion.md)
- [docs/118-sgx-lab-submit-manifest-op-ioctl.md](/root/Poulsbo/docs/118-sgx-lab-submit-manifest-op-ioctl.md)
- [docs/119-sgx-lab-submit-descriptor-op-ioctl.md](/root/Poulsbo/docs/119-sgx-lab-submit-descriptor-op-ioctl.md)
- [docs/120-sgx-lab-submit-plan-op-ioctl.md](/root/Poulsbo/docs/120-sgx-lab-submit-plan-op-ioctl.md)
- [docs/121-sgx-submit-plan-crash-isolation-harness.md](/root/Poulsbo/docs/121-sgx-submit-plan-crash-isolation-harness.md)
- [docs/122-sgx-plan-recovery-session-wrapper.md](/root/Poulsbo/docs/122-sgx-plan-recovery-session-wrapper.md)
- [docs/123-sgx-submit-plan-bisect-harness.md](/root/Poulsbo/docs/123-sgx-submit-plan-bisect-harness.md)
- [docs/124-sgx-submit-plan-investigation-harness.md](/root/Poulsbo/docs/124-sgx-submit-plan-investigation-harness.md)
- [docs/125-sgx-plan-guarded-deploy-session-wrapper.md](/root/Poulsbo/docs/125-sgx-plan-guarded-deploy-session-wrapper.md)
- [docs/126-sgx-plan-investigation-parent-session-validation.md](/root/Poulsbo/docs/126-sgx-plan-investigation-parent-session-validation.md)
- [docs/127-sgx-plan-session-entrypoint.md](/root/Poulsbo/docs/127-sgx-plan-session-entrypoint.md)
- [docs/128-sgx-submit-control-layout-hardening.md](/root/Poulsbo/docs/128-sgx-submit-control-layout-hardening.md)
- [docs/129-sgx-submit-status-layout-hardening.md](/root/Poulsbo/docs/129-sgx-submit-status-layout-hardening.md)
- [docs/130-sgx-submit-validator-stack-split.md](/root/Poulsbo/docs/130-sgx-submit-validator-stack-split.md)
- [docs/131-sgx-submit-refresh-preflight.md](/root/Poulsbo/docs/131-sgx-submit-refresh-preflight.md)
- [docs/132-sgx-submit-report-struct-normalization.md](/root/Poulsbo/docs/132-sgx-submit-report-struct-normalization.md)
- [docs/133-sgx-submit-fixed-block-struct-normalization.md](/root/Poulsbo/docs/133-sgx-submit-fixed-block-struct-normalization.md)
- [docs/134-sgx-submit-dependency-block-struct-normalization.md](/root/Poulsbo/docs/134-sgx-submit-dependency-block-struct-normalization.md)
- [docs/135-sgx-plan-sync-failure-clean-rerun.md](/root/Poulsbo/docs/135-sgx-plan-sync-failure-clean-rerun.md)
- [docs/136-sgx-plan-kick-failure-clean-rerun.md](/root/Poulsbo/docs/136-sgx-plan-kick-failure-clean-rerun.md)
- [docs/137-sgx-plan-timeout-hardening-and-post-descriptor-crash.md](/root/Poulsbo/docs/137-sgx-plan-timeout-hardening-and-post-descriptor-crash.md)
- [docs/138-sgx-submit-word-block-size-hardening.md](/root/Poulsbo/docs/138-sgx-submit-word-block-size-hardening.md)
- [docs/139-sgx-plan-pipefree-timeout-capture.md](/root/Poulsbo/docs/139-sgx-plan-pipefree-timeout-capture.md)
- [docs/sources.md](/root/Poulsbo/docs/sources.md)
- [scripts/collect_target_state.sh](/root/Poulsbo/scripts/collect_target_state.sh)
- [scripts/make_trace_dir.sh](/root/Poulsbo/scripts/make_trace_dir.sh)
- [scripts/annotate_mmio.py](/root/Poulsbo/scripts/annotate_mmio.py)
- [scripts/compare_mmio_logs.py](/root/Poulsbo/scripts/compare_mmio_logs.py)
- [scripts/update_register_db.py](/root/Poulsbo/scripts/update_register_db.py)
- [scripts/docker-build-bionic.sh](/root/Poulsbo/scripts/docker-build-bionic.sh)
- [scripts/docker-build-bionic-i386.sh](/root/Poulsbo/scripts/docker-build-bionic-i386.sh)
- [scripts/docker-run-bionic.sh](/root/Poulsbo/scripts/docker-run-bionic.sh)
- [scripts/docker-build-workloads.sh](/root/Poulsbo/scripts/docker-build-workloads.sh)
- [scripts/docker-build-module.sh](/root/Poulsbo/scripts/docker-build-module.sh)
- [scripts/docker-build-gma500-kernel-tree.sh](/root/Poulsbo/scripts/docker-build-gma500-kernel-tree.sh)
- [scripts/docker-build-vaio-gma500.sh](/root/Poulsbo/scripts/docker-build-vaio-gma500.sh)
- [scripts/deploy-vaio-gma500.sh](/root/Poulsbo/scripts/deploy-vaio-gma500.sh)
- [scripts/run-vaio-lab-2d.sh](/root/Poulsbo/scripts/run-vaio-lab-2d.sh)
- [scripts/run-vaio-gtt-state.sh](/root/Poulsbo/scripts/run-vaio-gtt-state.sh)
- [scripts/run-vaio-pf-bind.sh](/root/Poulsbo/scripts/run-vaio-pf-bind.sh)
- [scripts/run-vaio-sgx-object.sh](/root/Poulsbo/scripts/run-vaio-sgx-object.sh)
- [scripts/run-vaio-sgx-object-typed.sh](/root/Poulsbo/scripts/run-vaio-sgx-object-typed.sh)
- [scripts/run-vaio-sgx-object-mode-compare.sh](/root/Poulsbo/scripts/run-vaio-sgx-object-mode-compare.sh)
- [scripts/run-vaio-sgx-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit.sh)
- [scripts/run-vaio-sgx-submit-typed.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-typed.sh)
- [scripts/run-vaio-sgx-submit-mode-compare.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit-mode-compare.sh)
- [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
- [scripts/run-vaio-sgx-multi-submit-default.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit-default.sh)
- [scripts/run-vaio-sgx-multi-submit-typed.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit-typed.sh)
- [scripts/run-vaio-sgx-multi-submit-mode-compare.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit-mode-compare.sh)
- [scripts/run-vaio-sgx-indexed-dependency-array-mode-compare.sh](/root/Poulsbo/scripts/run-vaio-sgx-indexed-dependency-array-mode-compare.sh)
- [scripts/run-vaio-sgx-dependency-slot-clear-mode-compare.sh](/root/Poulsbo/scripts/run-vaio-sgx-dependency-slot-clear-mode-compare.sh)
- [scripts/run-vaio-sgx-dependency-drop.sh](/root/Poulsbo/scripts/run-vaio-sgx-dependency-drop.sh)
- [scripts/run-vaio-sgx-dependency-satisfied-drop.sh](/root/Poulsbo/scripts/run-vaio-sgx-dependency-satisfied-drop.sh)
- [scripts/run-vaio-sgx-dependency-unrelated-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-dependency-unrelated-submit.sh)
- [scripts/run-vaio-sgx-multi-source-dependency.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-source-dependency.sh)
- [scripts/run-vaio-sgx-multi-source-ticket-dependency.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-source-ticket-dependency.sh)
- [scripts/run-vaio-sgx-multi-source-ticket-dependency-mode-compare.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-source-ticket-dependency-mode-compare.sh)
- [scripts/run-vaio-sgx-third-dependency-lane.sh](/root/Poulsbo/scripts/run-vaio-sgx-third-dependency-lane.sh)
- [scripts/run-vaio-sgx-indexed-dependency-array.sh](/root/Poulsbo/scripts/run-vaio-sgx-indexed-dependency-array.sh)
- [scripts/run-vaio-sgx-fourth-dependency-slot.sh](/root/Poulsbo/scripts/run-vaio-sgx-fourth-dependency-slot.sh)
- [scripts/run-vaio-sgx-dependency-slot-clear.sh](/root/Poulsbo/scripts/run-vaio-sgx-dependency-slot-clear.sh)
- [scripts/run-vaio-sgx-ioctl-query.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-query.sh)
- [scripts/run-vaio-sgx-ioctl-range-query.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-range-query.sh)
- [scripts/run-vaio-sgx-ioctl-range-query-submit-match.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-range-query-submit-match.sh)
- [scripts/run-vaio-sgx-ioctl-sgx-state.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-sgx-state.sh)
- [scripts/run-vaio-sgx-ioctl-object-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-object-op.sh)
- [scripts/run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
- [scripts/wait-vaio-target-ready.sh](/root/Poulsbo/scripts/wait-vaio-target-ready.sh)
- [scripts/arm-vaio-gma500-rollback.sh](/root/Poulsbo/scripts/arm-vaio-gma500-rollback.sh)
- [scripts/disarm-vaio-gma500-rollback.sh](/root/Poulsbo/scripts/disarm-vaio-gma500-rollback.sh)
- [scripts/show-vaio-gma500-state.sh](/root/Poulsbo/scripts/show-vaio-gma500-state.sh)
- [scripts/watch-vaio-target-health.sh](/root/Poulsbo/scripts/watch-vaio-target-health.sh)
- [scripts/run-vaio-recovery-session.sh](/root/Poulsbo/scripts/run-vaio-recovery-session.sh)
- [scripts/run-vaio-guarded-deploy-session.sh](/root/Poulsbo/scripts/run-vaio-guarded-deploy-session.sh)
- [scripts/run-vaio-rollback-autorestore-session.sh](/root/Poulsbo/scripts/run-vaio-rollback-autorestore-session.sh)
- [scripts/summarize-vaio-sessions.sh](/root/Poulsbo/scripts/summarize-vaio-sessions.sh)
- [scripts/check-vaio-target-health.sh](/root/Poulsbo/scripts/check-vaio-target-health.sh)
- [scripts/revert-vaio-gma500.sh](/root/Poulsbo/scripts/revert-vaio-gma500.sh)
- [data/psb_registers.csv](/root/Poulsbo/data/psb_registers.csv)
- [notes/trace-session-template.md](/root/Poulsbo/notes/trace-session-template.md)
- [workloads/README.md](/root/Poulsbo/workloads/README.md)
- [module/README.md](/root/Poulsbo/module/README.md)
- [patches/ubuntu-4.15/0001-drm-gma500-add-sgx-lab-instrumentation.patch](/root/Poulsbo/patches/ubuntu-4.15/0001-drm-gma500-add-sgx-lab-instrumentation.patch)

## Immediate Use

On a Poulsbo target running Linux:

```bash
./scripts/collect_target_state.sh
./scripts/make_trace_dir.sh "first-linux-baseline"
```

For register log annotation:

```bash
python3 scripts/annotate_mmio.py traces/sample-mmio.log
```

For Ubuntu 18.04 container builds:

```bash
./scripts/docker-build-bionic.sh
./scripts/docker-build-bionic-i386.sh
./scripts/docker-build-workloads.sh
./scripts/docker-build-module.sh
./scripts/docker-build-gma500-kernel-tree.sh
```

If you need a non-default Docker socket or want host networking for old-package resolution:

```bash
DOCKER_HOST=unix:///path/to/docker.sock DOCKER_BUILD_NETWORK=host ./scripts/docker-build-bionic.sh
DOCKER_HOST=unix:///path/to/docker.sock DOCKER_BUILD_NETWORK=host ./scripts/docker-build-bionic-i386.sh
DOCKER_HOST=unix:///path/to/docker.sock DOCKER_RUN_NETWORK=host ./scripts/docker-build-workloads.sh
DOCKER_HOST=unix:///path/to/docker.sock DOCKER_RUN_NETWORK=host KVER=4.15.0-213-generic ./scripts/docker-build-module.sh
DOCKER_HOST=unix:///path/to/docker.sock DOCKER_RUN_NETWORK=host ./scripts/docker-build-gma500-kernel-tree.sh
```

The first real Ubuntu 4.15 `gma500` instrumentation patch lives in
[patches/ubuntu-4.15/0001-drm-gma500-add-sgx-lab-instrumentation.patch](/root/Poulsbo/patches/ubuntu-4.15/0001-drm-gma500-add-sgx-lab-instrumentation.patch).

The first Ubuntu 5.4 Poulsbo target patch lives in
[patches/ubuntu-5.4/0001-drm-gma500-add-sgx-lab-instrumentation.patch](/root/Poulsbo/patches/ubuntu-5.4/0001-drm-gma500-add-sgx-lab-instrumentation.patch).

## Build And Deploy Rule

The VAIO target is for deployment and testing only.

- build on this workstation
- copy the resulting module to the target
- install it under `/lib/modules/<kernel>/updates/`
- reboot the target to activate it

Do not build kernel modules on the Poulsbo machine itself.

For the current Sony VAIO VGN-P11Z target:

```bash
./scripts/docker-build-bionic-i386.sh
./scripts/docker-build-vaio-gma500.sh
./scripts/deploy-vaio-gma500.sh
./scripts/deploy-vaio-gma500-guarded.sh
./scripts/run-vaio-lab-2d.sh
./scripts/run-vaio-gtt-state.sh
./scripts/docker-build-workloads-i386.sh
./scripts/run-vaio-x11-probe.sh
./scripts/run-vaio-dri2-probe.sh
./scripts/run-vaio-drm-probe.sh
./scripts/run-vaio-pf-bind.sh
./scripts/run-vaio-sgx-object.sh
./scripts/run-vaio-sgx-submit.sh
./scripts/run-vaio-sgx-multi-submit.sh
./scripts/run-vaio-sgx-multi-source-dependency.sh
./scripts/run-vaio-sgx-multi-source-ticket-dependency.sh
./scripts/run-vaio-sgx-third-dependency-lane.sh
./scripts/run-vaio-sgx-indexed-dependency-array.sh
./scripts/run-vaio-sgx-fourth-dependency-slot.sh
./scripts/run-vaio-sgx-dependency-slot-clear.sh
./scripts/run-vaio-sgx-ioctl-query.sh
./scripts/run-vaio-sgx-ioctl-range-query.sh
./scripts/run-vaio-sgx-ioctl-sgx-state.sh
./scripts/run-vaio-sgx-ioctl-object-op.sh
./scripts/run-vaio-sgx-ioctl-submit-op.sh
./scripts/wait-vaio-target-ready.sh ./scripts/run-vaio-sgx-dependency-slot-clear.sh
./scripts/arm-vaio-gma500-rollback.sh
./scripts/disarm-vaio-gma500-rollback.sh
./scripts/show-vaio-gma500-state.sh
./scripts/watch-vaio-target-health.sh
./scripts/run-vaio-recovery-session.sh
./scripts/run-vaio-guarded-deploy-session.sh
./scripts/run-vaio-rollback-autorestore-session.sh
./scripts/summarize-vaio-sessions.sh
./scripts/check-vaio-target-health.sh
```

The deploy helper defaults to `root@10.1.11.218` and installs the locally built
`build/devnull-vaio-gma500-src/gma500_gfx.ko` for kernel `5.4.0-150-generic`.
The guarded variant runs the target-health probe before and after reboot so we
do not blindly redeploy into a half-usable VAIO state. It now also arms a
boot-time rollback guard by default and only clears it after post-boot SSH
health returns to `overall=remote_usable`. Before replacing the override, it
also preserves the previous override on the target so rollback can restore the
last known-good instrumented module instead of always falling back to stock.
That rollback path is now validated on hardware in both the healthy-confirmed
and auto-restore cases.

When the target is partially reachable but not usable, run the health probe:

```bash
./scripts/check-vaio-target-health.sh
```

It classifies the current VAIO state into cases like:

- `remote_usable`
- `host_down`
- `ssh_port_down`
- `ssh_not_usable`
- `ssh_stalled_post_kex`

`remote_usable` is based on real SSH command execution succeeding, not only on
ICMP reachability.

The current known-bad recovery case is documented in
[docs/25-vaio-ssh-stall-recovery.md](/root/Poulsbo/docs/25-vaio-ssh-stall-recovery.md).
The original `depclear_slot` validation blocker is documented in
[docs/46-sgx-dependency-slot-clear.md](/root/Poulsbo/docs/46-sgx-dependency-slot-clear.md).
The auto-resume helper for that blocked state is documented in
[docs/47-vaio-auto-resume-helper.md](/root/Poulsbo/docs/47-vaio-auto-resume-helper.md).
The new boot-time rollback guard for future risky deploys is documented in
[docs/48-vaio-boot-rollback-guard.md](/root/Poulsbo/docs/48-vaio-boot-rollback-guard.md).
The post-recovery remote-state inspection helper is documented in
[docs/49-vaio-state-inspection-helper.md](/root/Poulsbo/docs/49-vaio-state-inspection-helper.md).
The timestamped recovery watcher is documented in
[docs/50-vaio-health-timeline-watcher.md](/root/Poulsbo/docs/50-vaio-health-timeline-watcher.md).
The one-command recovery-session wrapper is documented in
[docs/51-vaio-recovery-session-wrapper.md](/root/Poulsbo/docs/51-vaio-recovery-session-wrapper.md).
The guarded-deploy session wrapper is documented in
[docs/52-vaio-guarded-deploy-session-wrapper.md](/root/Poulsbo/docs/52-vaio-guarded-deploy-session-wrapper.md).
The cross-session summary helper is documented in
[docs/53-vaio-session-summary-helper.md](/root/Poulsbo/docs/53-vaio-session-summary-helper.md).
The completed hardware validation of `depclear_slot` is documented in
[docs/54-sgx-dependency-slot-clear-validation.md](/root/Poulsbo/docs/54-sgx-dependency-slot-clear-validation.md).
The healthy-boot validation of the rollback guard is documented in
[docs/55-vaio-rollback-guard-healthy-boot-validation.md](/root/Poulsbo/docs/55-vaio-rollback-guard-healthy-boot-validation.md).
The failure-path auto-restore validation of the rollback guard is documented in
[docs/56-vaio-rollback-guard-autorestore-validation.md](/root/Poulsbo/docs/56-vaio-rollback-guard-autorestore-validation.md).
The first bounded SGX lab ioctl query surface is documented in
[docs/57-sgx-lab-ioctl-query.md](/root/Poulsbo/docs/57-sgx-lab-ioctl-query.md).
The first bounded SGX lab submit-op ioctl surface is documented in
[docs/58-sgx-lab-submit-op-ioctl.md](/root/Poulsbo/docs/58-sgx-lab-submit-op-ioctl.md).
The later `transition` validation on that same submit-op ioctl surface is
documented in
[docs/59-sgx-lab-transition-op-ioctl.md](/root/Poulsbo/docs/59-sgx-lab-transition-op-ioctl.md).
The later lifecycle extension on that same submit-op ioctl surface is
documented in
[docs/60-sgx-lab-lifecycle-op-ioctl.md](/root/Poulsbo/docs/60-sgx-lab-lifecycle-op-ioctl.md).
The first bounded SGX lab object-op ioctl surface is documented in
[docs/61-sgx-lab-object-op-ioctl.md](/root/Poulsbo/docs/61-sgx-lab-object-op-ioctl.md).
The later object `role` / `class` extension on that same object-op ioctl
surface is documented in
[docs/62-sgx-lab-object-role-class-ioctl.md](/root/Poulsbo/docs/62-sgx-lab-object-role-class-ioctl.md).
The later submit-slot `attach` / `detach` extension on the submit-op ioctl
surface is documented in
[docs/63-sgx-lab-submit-attach-op-ioctl.md](/root/Poulsbo/docs/63-sgx-lab-submit-attach-op-ioctl.md).
The later submit `kick` extension on that same submit-op ioctl surface is
documented in
[docs/64-sgx-lab-submit-kick-op-ioctl.md](/root/Poulsbo/docs/64-sgx-lab-submit-kick-op-ioctl.md).
The later submit `ccb` extension on that same submit-op ioctl surface is
documented in
[docs/65-sgx-lab-submit-ccb-op-ioctl.md](/root/Poulsbo/docs/65-sgx-lab-submit-ccb-op-ioctl.md).
The later submit `queue` extension on that same submit-op ioctl surface is
documented in
[docs/66-sgx-lab-submit-queue-op-ioctl.md](/root/Poulsbo/docs/66-sgx-lab-submit-queue-op-ioctl.md).
The later submit `status` extension on that same submit-op ioctl surface is
documented in
[docs/67-sgx-lab-submit-status-op-ioctl.md](/root/Poulsbo/docs/67-sgx-lab-submit-status-op-ioctl.md).
The later submit `sync` extension on that same submit-op ioctl surface is
documented in
[docs/68-sgx-lab-submit-sync-op-ioctl.md](/root/Poulsbo/docs/68-sgx-lab-submit-sync-op-ioctl.md).
The later submit `owner` extension on that same submit-op ioctl surface is
documented in
[docs/69-sgx-lab-submit-owner-op-ioctl.md](/root/Poulsbo/docs/69-sgx-lab-submit-owner-op-ioctl.md).
The later submit `prepare` materialization extension on that same submit-op
ioctl surface is documented in
[docs/108-sgx-lab-submit-prepare-op-ioctl.md](/root/Poulsbo/docs/108-sgx-lab-submit-prepare-op-ioctl.md).
The later dependency-slot extension on that same submit-op ioctl surface is
documented in
[docs/70-sgx-lab-submit-dependency-slot-op-ioctl.md](/root/Poulsbo/docs/70-sgx-lab-submit-dependency-slot-op-ioctl.md).
The later multi-submit validation with submit and dependency setup both on the
submit-op ioctl surface is documented in
[docs/71-sgx-lab-multi-submit-submit-op-validation.md](/root/Poulsbo/docs/71-sgx-lab-multi-submit-submit-op-validation.md).
The later follow-up that moved the same four-lane scenario onto the object-op,
submit-op, and dependency-slot ioctl paths together is documented in
[docs/72-sgx-lab-full-object-submit-ioctl-validation.md](/root/Poulsbo/docs/72-sgx-lab-full-object-submit-ioctl-validation.md).

The lab helper drives the live fbdev acceleration path through the PCI-device
sysfs control `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_lab_2d`. The
default sequence issues a full-screen clear, a fillrect, a copyarea, and a
wide mono imageblit. It also captures `poulsbo_sgx_state` before and after the
sequence, then prints the resulting `lab_2d`, `2D fillrect`, `2D copyarea`,
`2D imageblit`, and `SGX irq` kernel log lines from the target.

The SGX state view is exposed as
`/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_state`. Reading it captures a
fresh live snapshot of SGX, MMU, BIF, and 2D register state, including:

- event status and host enable registers
- BIF/MMU control, fault, and directory-base registers
- 2D status registers
- key driver-side context such as `gatt_start`, `mmu_gatt_start`, and the MMU
  driver's cached `bif_ctrl`

The GTT/MMU state helper runs [scripts/run-vaio-gtt-state.sh](/root/Poulsbo/scripts/run-vaio-gtt-state.sh)
against `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_gtt_state`. That view
summarizes:

- live GTT resource ranges such as `fb`, `cursor`, and `gem`
- each range's `in_gart`, `mmapping`, `npage`, `roll`, and pin/unpin totals
- MMU operation counters for insert, remove, flush, and context-set paths
- the last MMU operation address, page count, type, and hardware context

The first explicit SGX-aware bind control is exposed as
`/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_bind`. It currently supports:

- `pf_bind <range_id>`
- `pf_unbind <range_id>`

The helper [scripts/run-vaio-pf-bind.sh](/root/Poulsbo/scripts/run-vaio-pf-bind.sh)
copies the 32-bit [drm_probe.c](/root/Poulsbo/workloads/drm_probe.c) binary to
the VAIO, holds a live dumb GEM buffer open, identifies its `gtt_range`
`debug_id`, binds that range into `pf_pd`, unbinds it again, and prints the
before/during/after GTT/MMU state snapshots.

The first kernel-owned SGX lab object layer is exposed as:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_object`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_objects`

That path adopts a live `gtt_range` behind a kernel-owned object ID, keeps a
real GEM reference, and lets the object outlive userspace close until explicit
drop. The helper [scripts/run-vaio-sgx-object.sh](/root/Poulsbo/scripts/run-vaio-sgx-object.sh)
validates the full adopt/bind/unbind/drop lifetime on the VAIO.

The first lab submission plus kick-packet layer is exposed as:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

It adds per-object SGX role and expected-context metadata plus a multi-object
PF/default submission descriptor with a validate-only TA-style kick packet. The
helper [scripts/run-vaio-sgx-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-submit.sh)
exercises five live adopted objects, assigns them `control` / `pds` / `use` /
`target` / `status` roles plus `packet` / `code` / `code` / `surface` /
`state` object classes and `control_ccb` / `pds_code` / `use_code` /
`render_target` / `status_block` GPU-use semantics, then validates the submit
path twice on real hardware: once while userspace holders are alive and once
after they exit.

The helper now defaults to `QUEUE_MODE=wrap`, which configures:

- `cmd_type=ta ccb_offset=0x80 ctrl_flags=0x1 ta_status=1 3d_status=1 ta_dependency=0`
- a minimal control block with `ccb write_offset=0x20 read_offset=0x60 service=0x1000 cache=0x0 data0=0xc0de0001 data1=0xc0de0002`
- a wrapped queue block with `base_offset=0x20 size=0x80 packet_size=0x20 packet_count=2`
- a split queue result of `queue_tail_packet_count=1 queue_head_packet_count=1 queue_wrap=1`
- a separate status block with `ta_offset=0x80 3d_offset=0xc0 stride=0x4 seed=0x5a500000`

The same helper also still supports `QUEUE_MODE=nowrap` for the earlier linear
case. The live submit view resolves `control_gpu_va`, `pds_gpu_va`,
`use_gpu_va`, `target_gpu_va`, and `status_gpu_va`, plus the configured CCB,
queue, queue-split, sync, owner, and status fields and counters. The current
helper also programs:

- a validate-only sync block in the `status` object with:

- `sync_offset`
- mirrored queue `read_offset` and `write_offset`
- `sync_complete_seq`
- `sync_pending_seq`
- `sync_flags`
- `queue_epoch` mirrored at `sync_offset + 20`

- a second validate-only owner block in the `status` object with:

- `owner_offset`
- `owner_state`
- `owner_issue_cookie`
- `owner_complete_cookie`
- `queue_epoch` mirrored at `owner_offset + 12`

so later queue-ownership and completion experiments can build on explicit
memory-backed progress and ownership words instead of inferring everything from
queue geometry alone. It now also defaults to `ISSUE_ENABLE=1`, which starts
from a smaller `owner_state=queued` workload and drives a validate-only
write-side growth phase through:

- `issue submit_id owner_state issued_packets`

The live submit view now also exposes:

- `issue_count`
- `last_issue_jiffies`

After that, the same helper still defaults to `TRANSITION_ENABLE=1` and drives
the completion-side state machine through:

- `transition submit_id running completed_packets`
- `transition submit_id complete completed_packets`

In wrap mode the default sequence starts with one queued packet, issues one
more packet to create a wrapped queue view, then retires one packet into
`running` and the final packet into `complete`. In nowrap mode it starts with
one queued packet, issues one more packet to fill the queue, then retires both
packets across the same two completion-side transitions. The live submit view
also exposes:

- `transition_count`
- `last_transition_jiffies`
- `queue_epoch`
- `last_epoch_jiffies`

The helper now also defaults to `REFILL_ENABLE=1`, which reuses the same
persistent submit object for a second cycle after `complete`:

- `issue submit_id queued refill_issued_packets`
- `transition submit_id running refill_running_completed_packets`
- `transition submit_id complete refill_complete_completed_packets`

That refill path is still validate-only, but it now increments `queue_epoch`
only on a true `complete -> queued` refill from an empty queue and records
`last_epoch_jiffies` when that happens.

The same helper also supports:

- `REFILL_CYCLES=<n>`

so one run can validate more than one refill epoch on the same persistent
submit object. The current tree has already validated `REFILL_CYCLES=3` in both
wrap and nowrap queue modes on real hardware.

The submit control also now supports:

- `focus submit_id`
- `focus clear`

and the per-submit state view:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit_state`

That focused view avoids the `PAGE_SIZE` truncation of the global
`poulsbo_sgx_submits` dump and lets the repo's multi-submit helper inspect one
live submit at a time.

The same submit path now also exposes:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_coord_state`

and per-submit global coordination fields:

- `global_issue_ticket`
- `global_running_ticket`
- `global_complete_ticket`

The sync block mirrors the per-submit global issue ticket, the owner block
mirrors the per-submit running and completion tickets, and the global
coordination node tracks the last submit IDs and counters that advanced those
tickets plus the currently active running owner.

The dedicated [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
validator uses that node to keep two persistent submits alive at once, drive
submit A and submit B separately, and confirm their epoch, sync, owner,
validation, and global-ticket counters stay isolated while the shared
coordination counters advance monotonically. It now also proves that a second
submit gets `-EBUSY` if it tries to enter `running` while another submit owns
the active running lane, and it proves that a submit with a non-zero
`ta_dependency` gets `-EAGAIN` until the global completion ticket reaches the
required value. The same helper now also binds that dependency to a concrete
source submit through `depend <submit_id> <dependency_submit_id>`, so submit B
can be validated as waiting specifically on submit A rather than on an
anonymous global ticket threshold. It now also snapshots that dependency into
the status object through `depstate <submit_id> <offset>`, so the helper can
validate an explicit dependency block before and after submit A releases the
gate. That dependency block is now also live-updated on source-submit
completion, so after one setup-time `depstate` command the helper can observe
the block flip from unsatisfied to satisfied without another explicit refresh.
The same path now also has an explicit source-drop state: if the bound source
submit is dropped before satisfying the dependency, the block moves to
`dependency_flags=0x1b` and the dependent submit keeps returning `-EAGAIN`
through the new
[scripts/run-vaio-sgx-dependency-drop.sh](/root/Poulsbo/scripts/run-vaio-sgx-dependency-drop.sh)
validator instead of collapsing into a generic missing-submit failure.
It is now also validated that once the dependency is already satisfied, the
same source can be dropped and the dependent submit still runs normally under
`dependency_flags=0x1f` through
[scripts/run-vaio-sgx-dependency-satisfied-drop.sh](/root/Poulsbo/scripts/run-vaio-sgx-dependency-satisfied-drop.sh).
That satisfied-plus-missing state is now also proven to stay source-specific
after an unrelated submit completes: the dependent submit keeps its pinned
`dependency_observed_complete_ticket` and `dependency_flags=0x1f` through
[scripts/run-vaio-sgx-dependency-unrelated-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-dependency-unrelated-submit.sh).
The same path now also supports a second concrete dependency lane through
`depend2`, `depstate2`, and `depticket2`. Between
[scripts/run-vaio-sgx-multi-source-dependency.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-source-dependency.sh)
and
[scripts/run-vaio-sgx-multi-source-ticket-dependency.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-source-ticket-dependency.sh),
the repo now validates both shared-threshold two-source gating and per-lane
ticket semantics where one lane is already released while the other remains
blocked until a later completion ticket from its own bound source submit.
The same submit path now also supports a third lane through `depend3`,
`depticket3`, and `depstate3`, and
[scripts/run-vaio-sgx-third-dependency-lane.sh](/root/Poulsbo/scripts/run-vaio-sgx-third-dependency-lane.sh)
now validates one dependent submit waiting on three concrete source submits
with lane-specific completion tickets `1`, `2`, and `3`.

That same helper now also supports `USE_IOCTL_SUBMIT_SETUP=1` alongside
`USE_IOCTL_DEPENDENCY_SETUP=1`, and the four-lane dependency-slot-clear
scenario is now validated on the VAIO with both enabled. It now also supports
`USE_IOCTL_OBJECT_SETUP=1`, and the same scenario is validated with object
setup, submit setup, and indexed dependency-slot mutation all on bounded
ioctl paths. That means the multi-submit object lifecycle/configuration path,
submit lifecycle plus packet/block setup path, and indexed dependency-slot
mutation path all work through the bounded object-op and submit-op ioctl
surfaces, not only through the older sysfs write path.

That same runner now also supports `USE_IOCTL_RANGE_DISCOVERY=1` through the
new 32-bit `psb_sgx_lab_range_query` helper. In that mode it stages the helper
to the VAIO, enumerates live GEM ranges through the bounded range-query ioctl,
and no longer depends on scraping the page-sized `poulsbo_gtt_state` dump in
the hot path to discover fresh range IDs. The older sysfs path remains as a
fallback mode, and the runner still self-cleans stale lab objects and submits
before each run and uses a unique staged remote binary path.

The two remaining custom SGX lab query helpers,
[psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c) and
[psb_sgx_lab_range_query.c](/root/Poulsbo/workloads/psb_sgx_lab_range_query.c),
now also use raw `ioctl()` instead of libdrm's `drmIoctl()`. That closes the
query-side retry-behavior audit for the current custom lab tools and is
validated directly on the VAIO in
[docs/74-sgx-lab-query-tools-raw-ioctl-validation.md](/root/Poulsbo/docs/74-sgx-lab-query-tools-raw-ioctl-validation.md).

The bounded range-query path now also resolves a live range by GPU VA, not
only by query index or explicit `range_id`. That new path is validated in:

- [docs/98-sgx-lab-range-query-gpuva-validation.md](/root/Poulsbo/docs/98-sgx-lab-range-query-gpuva-validation.md)
- [logs/vaio-ioctl/20260511-150817-sgx-range-query-gpuva-live](/root/Poulsbo/logs/vaio-ioctl/20260511-150817-sgx-range-query-gpuva-live)

The canonical live run used GPU VA `0x807c0000`, resolved it back to range `5`
with offset `0x1000`, then proved SGX ownership transitions on that same VA:

- before adopt:
  - `range_sgx_object_id=0`
- after adopt:
  - `range_sgx_object_id=2`
- after userspace holder exit:
  - `range_sgx_object_id=2`
- after object drop:
  - `range_sgx_object_id=0`

That same GPU-VA path now also carries SGX object semantics directly in the
typed range query result:

- [docs/99-sgx-lab-range-query-gpuva-metadata-validation.md](/root/Poulsbo/docs/99-sgx-lab-range-query-gpuva-metadata-validation.md)
- [logs/vaio-ioctl/20260511-151741-sgx-range-query-gpuva-metadata-live](/root/Poulsbo/logs/vaio-ioctl/20260511-151741-sgx-range-query-gpuva-metadata-live)

The canonical metadata run kept the same GPU VA `0x807c0000` and proved the
full semantic transition on that one address:

- before adopt:
  - `generic / generic / generic / auto`
- after object metadata setup:
  - `control / packet / control_ccb / pf`
- after userspace holder exit:
  - `control / packet / control_ccb / pf`
- after object drop:
  - `generic / generic / generic / auto`

That same GPU-VA path now also carries live submit-reference state directly in
the typed range query result:

- [docs/100-sgx-lab-range-query-gpuva-submit-ref-validation.md](/root/Poulsbo/docs/100-sgx-lab-range-query-gpuva-submit-ref-validation.md)
- [logs/vaio-ioctl/20260511-152849-sgx-range-query-gpuva-submit-ref-live](/root/Poulsbo/logs/vaio-ioctl/20260511-152849-sgx-range-query-gpuva-submit-ref-live)

The canonical submit-ref run kept the same GPU VA `0x807c0000` and proved the
full object-plus-submit transition on that one address:

- before adopt:
  - `range_sgx_submit_ref_count=0`
  - `range_sgx_first_submit_slot_name=none`
- after object metadata setup:
  - `control / packet / control_ccb / pf`
  - `range_sgx_submit_ref_count=0`
- after typed submit `control` attach:
  - `range_sgx_submit_ref_count=1`
  - `range_sgx_first_submit_id=2`
  - `range_sgx_first_submit_slot_name=control`
- after userspace holder exit:
  - `range_sgx_submit_ref_count=1`
  - `range_sgx_first_submit_slot_name=control`
- after submit drop:
  - `range_sgx_submit_ref_count=0`
  - `range_sgx_first_submit_slot_name=none`
  - object semantics still `control / packet / control_ccb / pf`
- after object drop:
  - `generic / generic / generic / auto`
  - `range_sgx_submit_ref_count=0`

That same bounded range-query path now also supports selecting one concrete
submit-reference ordinal on the shared-object GPU-VA path itself. That
follow-up is documented in:

- [docs/107-sgx-lab-range-query-submit-match-validation.md](/root/Poulsbo/docs/107-sgx-lab-range-query-submit-match-validation.md)
- [logs/vaio-ioctl/20260511-165702-sgx-range-query-submit-match-live](/root/Poulsbo/logs/vaio-ioctl/20260511-165702-sgx-range-query-submit-match-live)

The canonical live run reused the same control-object GPU VA `0x807c0000` and
proved the range-side counterpart to `gpuva-match`:

- while submit `1` and submit `2` both reference that VA:
  - plain `gpuva` reports `range_sgx_submit_ref_count=2`
  - `gpuva-submit 0` resolves submit `1`
  - `gpuva-submit 1` resolves submit `2`
- after userspace holder exit:
  - `gpuva-submit 1` still resolves submit `2`
- after typed submit-`1` drop:
  - plain `gpuva` reports one remaining submit reference
  - `gpuva-submit 0` resolves submit `2`
  - `gpuva-submit 1` no longer resolves a selected submit
- after typed submit-`2` drop:
  - plain `gpuva` reports zero submit references

That closes the range-side inspection gap for shared-object submit selection,
so both query families can now disambiguate concrete live owners from the same
GPU VA.

The bounded submit-query path now also supports the inverse lookup directly:
start from one GPU VA and resolve the live submit, slot, and adopted object
without going through the range-query output first. That milestone is
documented in:

- [docs/101-sgx-lab-submit-query-gpuva-validation.md](/root/Poulsbo/docs/101-sgx-lab-submit-query-gpuva-validation.md)
- [logs/vaio-ioctl/20260511-155105-sgx-submit-query-gpuva-live](/root/Poulsbo/logs/vaio-ioctl/20260511-155105-sgx-submit-query-gpuva-live)

The canonical live run used the same GPU VA `0x807c0000` and proved:

- while the attached submit exists:
  - `submit_resolved_id=7`
  - `submit_resolved_slot_name=control`
  - `submit_resolved_object_id=31`
  - `submit_query_gpu_va_offset=0x00001000`
- after userspace holder exit:
  - the same `submit / slot / object / offset` tuple still resolves cleanly
- after typed submit drop:
  - `submit_ioctl_ret=-1 errno=2`

This run intentionally uses an attached-but-unvalidated submit, which is enough
for GPU-VA ownership lookup and avoids forcing later queue/block setup just to
prove the read-side resolution path.

That same direct GPU-VA submit-query path now also returns the backing
`gtt_range` ID and type in the same typed response. That follow-up is
documented in:

- [docs/102-sgx-lab-submit-query-gpuva-range-validation.md](/root/Poulsbo/docs/102-sgx-lab-submit-query-gpuva-range-validation.md)
- [logs/vaio-ioctl/20260511-160747-sgx-submit-query-gpuva-range-live](/root/Poulsbo/logs/vaio-ioctl/20260511-160747-sgx-submit-query-gpuva-range-live)

The canonical live run kept the same GPU VA `0x807c0000` and additionally
proved:

- while the attached submit exists:
  - `submit_resolved_range_id=7`
  - `submit_resolved_range_type_name=gem`
- after userspace holder exit:
  - the same `submit / slot / object / range / type / offset` tuple still
    resolves
- after typed submit drop:
  - `submit_ioctl_ret=-1 errno=2`

That same direct GPU-VA submit-query path now also returns the matched
object’s semantic metadata in the same typed response. That follow-up is
documented in:

- [docs/103-sgx-lab-submit-query-gpuva-object-validation.md](/root/Poulsbo/docs/103-sgx-lab-submit-query-gpuva-object-validation.md)
- [logs/vaio-ioctl/20260511-161543-sgx-submit-query-gpuva-object-live](/root/Poulsbo/logs/vaio-ioctl/20260511-161543-sgx-submit-query-gpuva-object-live)

The canonical live run kept the same GPU VA `0x807c0000` and additionally
proved:

- while the attached submit exists:
  - `submit_resolved_object_role_name=control`
  - `submit_resolved_object_class_name=packet`
  - `submit_resolved_object_gpu_use_name=control_ccb`
  - `submit_resolved_object_expected_context_name=pf`
- after userspace holder exit:
  - the same `submit / slot / object / range / role / class / gpu_use /
    context / offset` tuple still resolves
- after typed submit drop:
  - `submit_ioctl_ret=-1 errno=2`

That same direct GPU-VA submit-query path now also reports how many live
submits currently match the same GPU VA. That follow-up is documented in:

- [docs/104-sgx-lab-submit-query-gpuva-multi-ref-validation.md](/root/Poulsbo/docs/104-sgx-lab-submit-query-gpuva-multi-ref-validation.md)
- [logs/vaio-ioctl/20260511-162454-sgx-submit-query-gpuva-multi-ref-live](/root/Poulsbo/logs/vaio-ioctl/20260511-162454-sgx-submit-query-gpuva-multi-ref-live)

The canonical live run reused the same control-object GPU VA `0x807c0000` and
proved the shared-object handoff directly from `QUERY_SUBMIT gpuva`:

- while submit A and submit B both reference that VA:
  - `submit_resolved_id=1`
  - `submit_matching_submit_count=2`
- after userspace holder exit:
  - submit A still resolves first
  - `submit_matching_submit_count=2`
- after typed submit-A drop:
  - the same GPU VA now resolves submit B
  - `submit_resolved_id=2`
  - `submit_matching_submit_count=1`
- after typed submit-B drop:
  - `submit_ioctl_ret=-1 errno=2`

The same direct GPU-VA submit-query path now also supports selecting one
concrete match ordinal while multiple submits still reference that same VA.
That follow-up is documented in:

- [docs/105-sgx-lab-submit-query-gpuva-match-validation.md](/root/Poulsbo/docs/105-sgx-lab-submit-query-gpuva-match-validation.md)
- [logs/vaio-ioctl/20260511-163739-sgx-submit-query-gpuva-match-live](/root/Poulsbo/logs/vaio-ioctl/20260511-163739-sgx-submit-query-gpuva-match-live)

The canonical live run kept the same control-object GPU VA `0x807c0000` and
proved:

- while submit A and submit B both reference that VA:
  - `gpuva-match 0` resolves submit A
  - `gpuva-match 1` resolves submit B
  - `submit_matching_submit_count=2`
- after userspace holder exit:
  - `gpuva-match 1` still resolves submit B
- after typed submit-A drop:
  - `gpuva-match 0` resolves submit B
  - `gpuva-match 1` fails cleanly with `errno=2`
- after typed submit-B drop:
  - the plain `gpuva` lookup also fails cleanly with `errno=2`

The same typed path now also has a canonical wrapper-level enumeration mode for
that shared-VA state. That follow-up is documented in:

- [docs/106-sgx-lab-submit-query-gpuva-list-validation.md](/root/Poulsbo/docs/106-sgx-lab-submit-query-gpuva-list-validation.md)
- [logs/vaio-ioctl/20260511-164757-sgx-submit-query-gpuva-list-live](/root/Poulsbo/logs/vaio-ioctl/20260511-164757-sgx-submit-query-gpuva-list-live)

The canonical live run proves:

- while submit A and submit B both reference that VA:
  - list entry `0` resolves submit A
  - list entry `1` resolves submit B
- after userspace holder exit:
  - the same two-entry list remains stable
- after typed submit-A drop:
  - the list contracts cleanly to one entry for submit B
- after final cleanup:
  - the underlying direct lookup path returns to `ENOENT`

The read-side SGX snapshot path is now also available through the bounded
`QUERY_SGX_STATE` ioctl plus the 32-bit
[psb_sgx_lab_sgx_state_query.c](/root/Poulsbo/workloads/psb_sgx_lab_sgx_state_query.c)
helper. The live validator
[run-vaio-sgx-ioctl-sgx-state.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-sgx-state.sh)
proves that ioctl-captured, ioctl-cached, and sysfs-captured snapshots match
across the real 2D lab sequence on the VAIO. That milestone is documented in
[docs/75-sgx-lab-sgx-state-query-ioctl.md](/root/Poulsbo/docs/75-sgx-lab-sgx-state-query-ioctl.md).

The global GTT/MMU state path is now also available through the bounded
`QUERY_GTT_STATE` ioctl plus the 32-bit
[psb_sgx_lab_gtt_state_query.c](/root/Poulsbo/workloads/psb_sgx_lab_gtt_state_query.c)
helper. The live validator
[run-vaio-sgx-ioctl-gtt-state.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-gtt-state.sh)
proves that the typed ioctl view matches the existing `poulsbo_gtt_state` dump
before, during, and after held live GEM ranges on the VAIO. That milestone is
documented in
[docs/76-sgx-lab-gtt-state-query-ioctl.md](/root/Poulsbo/docs/76-sgx-lab-gtt-state-query-ioctl.md).

The bounded object-op and submit-op surfaces now also expose typed `cleanup`
verbs. The focused live validator
[run-vaio-sgx-ioctl-cleanup.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-cleanup.sh)
proves that they can remove seeded stale submits, clear focused-submit state,
drop adopted objects, and unbind a PF-bound object on the VAIO. The same
cleanup path is now preferred by the ioctl-based runners before they fall back
to the older sysfs drop loops. That milestone is documented in
[docs/77-sgx-lab-cleanup-op-ioctl.md](/root/Poulsbo/docs/77-sgx-lab-cleanup-op-ioctl.md).

The object-side read path is now also available through the bounded
`QUERY_OBJECT` ioctl plus the 32-bit
[psb_sgx_lab_object_query.c](/root/Poulsbo/workloads/psb_sgx_lab_object_query.c)
helper. The live validator
[run-vaio-sgx-ioctl-object-query.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-object-query.sh)
proves that typed object-ID and object-index queries match the live
`poulsbo_sgx_objects` dump, survive userspace close until explicit cleanup, and
then fail cleanly with `ENOENT` after cleanup. That milestone is documented in
[docs/78-sgx-lab-object-query-ioctl.md](/root/Poulsbo/docs/78-sgx-lab-object-query-ioctl.md).

The submit-side read path now also supports bounded live-index lookup through
`QUERY_SUBMIT`. The same 32-bit
[psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c) helper now
supports coordination-only readout through `coord`, plus explicit submit ID,
focused-submit lookup, and `index <n>`. The live validator
[run-vaio-sgx-ioctl-submit-query.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-query.sh)
proves that typed submit-ID, focus, and submit-index queries agree on a live
two-submit state, survive userspace close, and then fail cleanly with `ENOENT`
after typed cleanup. That milestone is documented in
[docs/79-sgx-lab-submit-query-index-ioctl.md](/root/Poulsbo/docs/79-sgx-lab-submit-query-index-ioctl.md).

The remaining human-readable SGX lab inspection path is now also covered by a
typed no-sysfs dump flow. The live validator
[run-vaio-sgx-ioctl-state-dump.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-state-dump.sh)
uses only the bounded query and mutation ioctls to stage a live state, dump
typed coord/object/submit/SGX/GTT state, verify persistence after userspace
close, and then verify cleanup returns the live counts to zero. That milestone
is documented in
[docs/80-sgx-lab-typed-state-dump-validation.md](/root/Poulsbo/docs/80-sgx-lab-typed-state-dump-validation.md).

That same path now also has a standalone 32-bit dump tool:
[psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c). The live
validator stages the tool to the VAIO and proves it matches the same no-sysfs
state path during and after cleanup. That milestone is documented in
[docs/81-sgx-lab-dump-tool-validation.md](/root/Poulsbo/docs/81-sgx-lab-dump-tool-validation.md).

That same dump tool now also has a validated structured JSON mode. The live
state-dump validator proves that `capture-json` and `json` report the same
two-submit and five-object staged state as the text dump, and then return the
same zeroed live counts after typed cleanup. That milestone is documented in
[docs/82-sgx-lab-dump-json-validation.md](/root/Poulsbo/docs/82-sgx-lab-dump-json-validation.md).

The focused cleanup validator now also uses only typed coord, submit, object,
range, and standalone dump reads for verification. It no longer needs
`poulsbo_sgx_coord_state`, `poulsbo_sgx_submits`, `poulsbo_sgx_objects`, or
`poulsbo_gtt_state` to prove submit cleanup, focused-submit clearing, object
cleanup, and PF-unbind cleanup on the VAIO. That milestone is documented in
[docs/83-sgx-lab-cleanup-nosysfs-validation.md](/root/Poulsbo/docs/83-sgx-lab-cleanup-nosysfs-validation.md).

The focused object-op validator now also uses only typed object, range, and
global GTT/MMU reads for verification. It no longer needs
`poulsbo_sgx_objects` or `poulsbo_gtt_state` to prove adopt, metadata
mutation, PF bind/unbind, object survival after userspace close, and final
drop on the VAIO. That milestone is documented in
[docs/84-sgx-lab-object-op-nosysfs-validation.md](/root/Poulsbo/docs/84-sgx-lab-object-op-nosysfs-validation.md).

The focused submit-op validator now also uses only typed object, range,
submit, coord, and global GTT/MMU helpers for both setup and verification. It
no longer needs `poulsbo_sgx_object`, `poulsbo_sgx_objects`,
`poulsbo_sgx_submit`, `poulsbo_sgx_submits`, `poulsbo_sgx_submit_state`,
`poulsbo_sgx_coord_state`, or `poulsbo_gtt_state` to prove five-object setup,
typed attach, typed kick/queue/block programming, submit progression, submit
drop, and final object cleanup on the VAIO. That milestone is documented in
[docs/85-sgx-lab-submit-op-nosysfs-validation.md](/root/Poulsbo/docs/85-sgx-lab-submit-op-nosysfs-validation.md).

The higher-level multi-submit runner now also has a validated typed
verification mode. With `USE_IOCTL_VERIFICATION=1` plus the already-validated
typed object, submit, dependency, and range setup paths, the full four-lane
dependency-slot-clear scenario no longer needs the old SGX or GTT sysfs text
nodes for verification. That milestone is documented in
[docs/86-sgx-lab-multi-submit-nosysfs-validation.md](/root/Poulsbo/docs/86-sgx-lab-multi-submit-nosysfs-validation.md).

The same submit-op path now also has hardware-validated typed `prepare` and
`arm` stages. The focused submit-op validator and the typed multi-submit runner
now materialize the configured control/status image through `prepare`, then
snapshot that prepared state through `arm` before the first progression step.
Those milestones are documented in
[docs/108-sgx-lab-submit-prepare-op-ioctl.md](/root/Poulsbo/docs/108-sgx-lab-submit-prepare-op-ioctl.md)
and [docs/109-sgx-lab-submit-arm-op-ioctl.md](/root/Poulsbo/docs/109-sgx-lab-submit-arm-op-ioctl.md).

The X11 helper copies the 32-bit [x11_egl_probe.c](/root/Poulsbo/workloads/x11_egl_probe.c)
binary to the VAIO and runs it against the live `:0` display with the LightDM
server auth cookie. That gives a minimal userspace check of the current
Xorg/EGL/GLES path without requiring host-side `eglinfo`.

The DRI2 helper runs a second 32-bit probe,
[x11_dri2_probe.c](/root/Poulsbo/workloads/x11_dri2_probe.c), against the same
display. It asks the X server directly for DRI2 version/connect/auth state so we
can distinguish “bad cookie / bad `/dev/dri` permissions” from “screen never
became DRI2-capable”.

The DRM helper runs [drm_probe.c](/root/Poulsbo/workloads/drm_probe.c) directly
against `/dev/dri/card0`. On the current VAIO state it confirms KMS and dumb
buffers work, while PRIME/render-node/render-uAPI support is still absent. If a
second positional argument is supplied, the probe keeps the dumb buffer open for
that many seconds before destroying it; the PF-bind helper uses that hold mode.

The new SGX lab query helper runs
[psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c) against
`/dev/dri/card0`. It uses the new bounded lab ioctls to query typed SGX submit
and coordination state by explicit submit ID, by focused submit, by live
submit index, by GPU VA, or as a coord-only typed readout, so the lab path no
longer depends only on parsing sysfs text for read-side inspection.

The new SGX lab object-op helper runs
[psb_sgx_lab_object_op.c](/root/Poulsbo/workloads/psb_sgx_lab_object_op.c)
against `/dev/dri/card0`. It uses a bounded write-side lab ioctl to run:

- `adopt`
- `role`
- `class`
- `gpu_use`
- `context`
- `pf_bind`
- `pf_unbind`
- `drop`
- `cleanup`

It now also uses raw `ioctl()` for the custom lab object request so the
object-side lab path is not coupled to libdrm retry behavior, and the
multi-submit runner can safely use it for higher-level object setup.

The submit-side companion runs
[psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
against `/dev/dri/card0`. It uses the new bounded write-side lab ioctl to run
the existing validated submit lifecycle and progression operations:

- `create`
- `focus`
- `focus-clear`
- `attach`
- `detach`
- `kick`
- `ccb`
- `queue`
- `status`
- `sync`
- `owner`
- `cleanup`
- `depend-slot`
- `depticket-slot`
- `depstate-slot`
- `depclear-slot`
- `validate`
- `issue`
- `transition`
- `drop`

It supports explicit submit IDs or focused-submit lookup where the chosen op
needs an existing submit. It now uses raw `ioctl()` for the custom lab submit
request instead of libdrm's `drmIoctl()`, because the lab path intentionally
uses `-EAGAIN` for dependency-gated transitions and libdrm retries that error.

The compare-session layer now also has a dedicated reset helper:

- [scripts/run-vaio-sgx-lab-reset.sh](/root/Poulsbo/scripts/run-vaio-sgx-lab-reset.sh)

The compare wrappers call it before the typed branch and again between the
typed and legacy branches, and the direct legacy multi-submit path now uses the
typed range-query helper for before/during live GEM baselines when that path is
available. That fixes warm-target compare failures caused by:

- stale SGX lab objects surviving just long enough to poison the legacy branch
- truncated `poulsbo_gtt_state` baselines hiding high live range IDs

The completed validation bundle and write-up are:

- [logs/vaio-compare/20260511-141658-sgx-dependency-slot-clear-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-141658-sgx-dependency-slot-clear-mode-compare)
- [docs/95-sgx-compare-reset-and-legacy-range-baseline-fix.md](/root/Poulsbo/docs/95-sgx-compare-reset-and-legacy-range-baseline-fix.md)

The same reset-assisted warm-target path is now also revalidated for the
focused wrappers:

- focused object compare:
  - [logs/vaio-compare/20260511-142409-sgx-object-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-142409-sgx-object-mode-compare)
- focused submit compare:
  - [logs/vaio-compare/20260511-144832-sgx-submit-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-144832-sgx-submit-mode-compare)
- write-up:
  - [docs/96-sgx-focused-compare-reset-and-range-query-fix.md](/root/Poulsbo/docs/96-sgx-focused-compare-reset-and-range-query-fix.md)

That follow-up also fixed the focused legacy submit helper so it now uses the
typed range-query helper for warm-target fresh-range discovery, and corrected
the `range_type_name` / `range_ioctl_ret` parser bugs that had been breaking
both the focused submit compare path and the reset helper's mmapping-baseline
check.

The generic multi-submit compare wrapper is now also revalidated through the
same reset-assisted path:

- [logs/vaio-compare/20260511-145551-sgx-multi-submit-mode-compare](/root/Poulsbo/logs/vaio-compare/20260511-145551-sgx-multi-submit-mode-compare)
- [docs/97-sgx-generic-multi-submit-compare-reset-follow-up.md](/root/Poulsbo/docs/97-sgx-generic-multi-submit-compare-reset-follow-up.md)

That run completed cleanly with `pre_reset_rc=0`, `inter_reset_rc=0`,
typed/default IDs `133/134`, explicit legacy IDs `135/136`, and final target
state still `overall=remote_usable`.

The SGX lab submit path now has a persistent post-`issue` execution snapshot
on top of the earlier `prepare`, `arm`, `emit`, and `launch` stages:

- [docs/110-sgx-lab-submit-emit-op-ioctl.md](/root/Poulsbo/docs/110-sgx-lab-submit-emit-op-ioctl.md)
- [docs/111-sgx-lab-submit-launch-op-ioctl.md](/root/Poulsbo/docs/111-sgx-lab-submit-launch-op-ioctl.md)
- [docs/112-sgx-lab-submit-dispatch-state.md](/root/Poulsbo/docs/112-sgx-lab-submit-dispatch-state.md)

The focused typed submit path and the typed multi-submit path are now both
revalidated through:

- `prepare`
- `arm`
- `validate`
- `emit`
- `validate`
- `launch`
- `validate`
- `issue`
- `transition`

That means the current bounded lab path can now prove one materialized image,
one armed snapshot, one emitted execution record, one launched issue-plan, and
one persistent dispatched execution snapshot that survives
`issue -> running -> complete`, all on real hardware.

## Documentation Rule

This project now treats documentation as part of the deliverable.

For every meaningful driver, tooling, probe, or deployment change:

- update the relevant technical document
- append the result to [docs/11-progress-log.md](/root/Poulsbo/docs/11-progress-log.md)
- update remaining work in [docs/12-active-todo.md](/root/Poulsbo/docs/12-active-todo.md)

The progress log records what changed and what was validated. The active TODO
tracks what is still missing, what is blocked, and what should be done next.

## Execution Order

1. Capture and archive a clean Linux baseline from real hardware.
2. Stand up a reference proprietary stack on an isolated legacy OS image.
3. Collect black-box traces for simple GLES workloads.
4. Implement kernel-side reset, fault decode, and minimal submit path.
5. Bring up a Mesa driver with clear-color, triangle, texture, and FBO milestones.
6. Validate Xorg `modesetting` + `glamor`.

## Non-Goals For The First Driver

- Vulkan
- Wayland compositor tuning
- video decode
- perfect power management
- complete desktop GL beyond what the hardware can realistically support

The first acceptable target is a stable open GLES 2.0 class stack with working Xorg acceleration and sane fault handling.

Recent operator hardening around the blocked `plan` line is also documented:

- interrupted parent sessions now still write fallback `session-result.env`
- timeout-sensitive local wrapper capture is pipe-free
- rollback autorestore session directories are collision-safe
- older incomplete bundles now summarize as `interrupted_or_incomplete`

See:

- [docs/140-sgx-session-result-finalization-and-summary-hardening.md](/root/Poulsbo/docs/140-sgx-session-result-finalization-and-summary-hardening.md)
- [docs/159-vaio-kdump-panic-capture-and-reboot-validation.md](/root/Poulsbo/docs/159-vaio-kdump-panic-capture-and-reboot-validation.md)
- [docs/160-sgx-plan-fixed-module-through-complete.md](/root/Poulsbo/docs/160-sgx-plan-fixed-module-through-complete.md)
- [docs/161-sgx-lab-submit-result-op-ioctl.md](/root/Poulsbo/docs/161-sgx-lab-submit-result-op-ioctl.md)
- [docs/162-sgx-submit-exec-record-refactor.md](/root/Poulsbo/docs/162-sgx-submit-exec-record-refactor.md)
- [docs/163-sgx-submit-live-view-exec-record-unification.md](/root/Poulsbo/docs/163-sgx-submit-live-view-exec-record-unification.md)
- [docs/164-sgx-submit-view-record-collection.md](/root/Poulsbo/docs/164-sgx-submit-view-record-collection.md)
- [docs/165-sgx-lab-submit-live-op-ioctl.md](/root/Poulsbo/docs/165-sgx-lab-submit-live-op-ioctl.md)
- [docs/166-sgx-lab-submit-issue-plan-op-ioctl.md](/root/Poulsbo/docs/166-sgx-lab-submit-issue-plan-op-ioctl.md)
- [docs/167-sgx-submit-exec-packet-structure.md](/root/Poulsbo/docs/167-sgx-submit-exec-packet-structure.md)
- [docs/168-sgx-submit-cmdta-kick-object.md](/root/Poulsbo/docs/168-sgx-submit-cmdta-kick-object.md)
- [docs/169-sgx-submit-ccb-kick-object.md](/root/Poulsbo/docs/169-sgx-submit-ccb-kick-object.md)
- [docs/170-sgx-submit-sgx-ccb-kick-object.md](/root/Poulsbo/docs/170-sgx-submit-sgx-ccb-kick-object.md)
- [docs/171-sgx-submit-cmdta-shared-object.md](/root/Poulsbo/docs/171-sgx-submit-cmdta-shared-object.md)
- [docs/172-sgx-submit-device-sync-objects.md](/root/Poulsbo/docs/172-sgx-submit-device-sync-objects.md)
- [docs/173-sgx-submit-cmdta-object.md](/root/Poulsbo/docs/173-sgx-submit-cmdta-object.md)
- [docs/174-sgx-submit-ta-kick-object.md](/root/Poulsbo/docs/174-sgx-submit-ta-kick-object.md)
- [docs/175-sgx-submit-ta-command-object.md](/root/Poulsbo/docs/175-sgx-submit-ta-command-object.md)
- [docs/176-sgx-submit-queued-ta-command-object.md](/root/Poulsbo/docs/176-sgx-submit-queued-ta-command-object.md)
- [docs/177-sgx-submit-ta-submission-object.md](/root/Poulsbo/docs/177-sgx-submit-ta-submission-object.md)
- [docs/178-sgx-submit-ta-execution-object.md](/root/Poulsbo/docs/178-sgx-submit-ta-execution-object.md)
- [docs/179-sgx-submit-ta-timeline-object.md](/root/Poulsbo/docs/179-sgx-submit-ta-timeline-object.md)
- [docs/180-sgx-submit-ta-runtime-object.md](/root/Poulsbo/docs/180-sgx-submit-ta-runtime-object.md)
- [docs/181-sgx-submit-ta-gate-object.md](/root/Poulsbo/docs/181-sgx-submit-ta-gate-object.md)
- [docs/182-sgx-submit-ta-control-object.md](/root/Poulsbo/docs/182-sgx-submit-ta-control-object.md)
- [docs/183-sgx-submit-ta-schedule-object.md](/root/Poulsbo/docs/183-sgx-submit-ta-schedule-object.md)
- [docs/184-sgx-submit-ta-dispatch-object.md](/root/Poulsbo/docs/184-sgx-submit-ta-dispatch-object.md)
- [docs/185-sgx-submit-ta-path-object.md](/root/Poulsbo/docs/185-sgx-submit-ta-path-object.md)
- [docs/186-sgx-submit-ta-path-issue-unification.md](/root/Poulsbo/docs/186-sgx-submit-ta-path-issue-unification.md)
- [docs/187-sgx-submit-ta-commit-object.md](/root/Poulsbo/docs/187-sgx-submit-ta-commit-object.md)
- [docs/188-sgx-submit-ta-event-object.md](/root/Poulsbo/docs/188-sgx-submit-ta-event-object.md)
- [docs/189-sgx-submit-ta-work-item-object.md](/root/Poulsbo/docs/189-sgx-submit-ta-work-item-object.md)
- [docs/190-sgx-submit-ta-op-object.md](/root/Poulsbo/docs/190-sgx-submit-ta-op-object.md)
- [docs/191-sgx-submit-ta-scheduler-object.md](/root/Poulsbo/docs/191-sgx-submit-ta-scheduler-object.md)
- [docs/192-sgx-submit-ta-sequence-object.md](/root/Poulsbo/docs/192-sgx-submit-ta-sequence-object.md)
- [docs/193-sgx-submit-ta-work-queue-object.md](/root/Poulsbo/docs/193-sgx-submit-ta-work-queue-object.md)
- [docs/194-sgx-submit-ta-job-object.md](/root/Poulsbo/docs/194-sgx-submit-ta-job-object.md)
- [docs/195-sgx-submit-ta-request-object.md](/root/Poulsbo/docs/195-sgx-submit-ta-request-object.md)

Recent VAIO panic-capture validation is now in place too:

- `scripts/run-vaio-panic-capture-setup-session.sh` installed `kdump-tools`,
  `kexec-tools`, and `makedumpfile`, then validated boot with
  `crashkernel=128M panic=10 panic_on_oops=1`
- `scripts/run-vaio-panic-test-session.sh` forced a real panic and the VAIO
  auto-rebooted back to `overall=remote_usable`
- Ubuntu kdump on this target writes the saved crash as
  `dump.<timestamp>` plus `dmesg.<timestamp>` under `/var/crash/<timestamp>/`,
  and the collector/fetch/state path now recognizes that layout directly

The current SGX-shaped kernel-internal execution stack is now:

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
- `psb_sgx_lab_submit_sgx_ta_submission`
- `psb_sgx_lab_submit_sgx_ta_execution`
- `psb_sgx_lab_submit_sgx_ta_timeline`
- `psb_sgx_lab_submit_sgx_ta_runtime`
- `psb_sgx_lab_submit_sgx_ta_gate`
- `psb_sgx_lab_submit_sgx_ta_control`
- `psb_sgx_lab_submit_sgx_ta_schedule`
- `psb_sgx_lab_submit_sgx_ta_dispatch`
- `psb_sgx_lab_submit_sgx_ta_path`
- `psb_sgx_lab_submit_sgx_ta_commit`
- `psb_sgx_lab_submit_sgx_ta_event`
- `psb_sgx_lab_submit_sgx_ta_work_item`
- `psb_sgx_lab_submit_sgx_ta_op`
- `psb_sgx_lab_submit_sgx_ta_scheduler`
- `psb_sgx_lab_submit_sgx_ta_sequence`
- `psb_sgx_lab_submit_sgx_ta_work_queue`
- `psb_sgx_lab_submit_sgx_ta_job`
- `psb_sgx_lab_submit_sgx_ta_request`
- `psb_sgx_lab_submit_sgx_ta_ioctl_request`
- `psb_sgx_lab_submit_sgx_ta_submit_op_request`
- `psb_sgx_lab_submit_sgx_ta_queue_item`
- `psb_sgx_lab_submit_sgx_ta_queue_command`
- `psb_sgx_lab_submit_sgx_ta_scheduler_entry`
- `psb_sgx_lab_submit_sgx_ta_queue_stream`
- `psb_sgx_lab_submit_sgx_ta_queue_packet`
- `psb_sgx_lab_submit_sgx_ta_request_packet`

The shared SGX layer now explicitly models device-sync objects and the
hardware destination-sync list through:

- `psb_sgx_lab_submit_sgx_device_sync_object`
- `psb_sgx_lab_submit_sgx_hwdevice_sync_list`

The newest validated hardware batch for that line is:

- [docs/168-sgx-submit-cmdta-kick-object.md](/root/Poulsbo/docs/168-sgx-submit-cmdta-kick-object.md)
- [docs/169-sgx-submit-ccb-kick-object.md](/root/Poulsbo/docs/169-sgx-submit-ccb-kick-object.md)
- [docs/170-sgx-submit-sgx-ccb-kick-object.md](/root/Poulsbo/docs/170-sgx-submit-sgx-ccb-kick-object.md)
- [docs/171-sgx-submit-cmdta-shared-object.md](/root/Poulsbo/docs/171-sgx-submit-cmdta-shared-object.md)
- [docs/172-sgx-submit-device-sync-objects.md](/root/Poulsbo/docs/172-sgx-submit-device-sync-objects.md)
- [docs/173-sgx-submit-cmdta-object.md](/root/Poulsbo/docs/173-sgx-submit-cmdta-object.md)
- [docs/174-sgx-submit-ta-kick-object.md](/root/Poulsbo/docs/174-sgx-submit-ta-kick-object.md)
- [docs/175-sgx-submit-ta-command-object.md](/root/Poulsbo/docs/175-sgx-submit-ta-command-object.md)
- [docs/176-sgx-submit-queued-ta-command-object.md](/root/Poulsbo/docs/176-sgx-submit-queued-ta-command-object.md)
- [docs/177-sgx-submit-ta-submission-object.md](/root/Poulsbo/docs/177-sgx-submit-ta-submission-object.md)
- [docs/178-sgx-submit-ta-execution-object.md](/root/Poulsbo/docs/178-sgx-submit-ta-execution-object.md)
- [docs/179-sgx-submit-ta-timeline-object.md](/root/Poulsbo/docs/179-sgx-submit-ta-timeline-object.md)
- [docs/180-sgx-submit-ta-runtime-object.md](/root/Poulsbo/docs/180-sgx-submit-ta-runtime-object.md)
- [docs/181-sgx-submit-ta-gate-object.md](/root/Poulsbo/docs/181-sgx-submit-ta-gate-object.md)
- [docs/182-sgx-submit-ta-control-object.md](/root/Poulsbo/docs/182-sgx-submit-ta-control-object.md)
- [docs/183-sgx-submit-ta-schedule-object.md](/root/Poulsbo/docs/183-sgx-submit-ta-schedule-object.md)
- [docs/184-sgx-submit-ta-dispatch-object.md](/root/Poulsbo/docs/184-sgx-submit-ta-dispatch-object.md)
- [docs/185-sgx-submit-ta-path-object.md](/root/Poulsbo/docs/185-sgx-submit-ta-path-object.md)
- [docs/186-sgx-submit-ta-path-issue-unification.md](/root/Poulsbo/docs/186-sgx-submit-ta-path-issue-unification.md)
- [docs/187-sgx-submit-ta-commit-object.md](/root/Poulsbo/docs/187-sgx-submit-ta-commit-object.md)
- [docs/188-sgx-submit-ta-event-object.md](/root/Poulsbo/docs/188-sgx-submit-ta-event-object.md)
- [docs/189-sgx-submit-ta-work-item-object.md](/root/Poulsbo/docs/189-sgx-submit-ta-work-item-object.md)
- [docs/190-sgx-submit-ta-op-object.md](/root/Poulsbo/docs/190-sgx-submit-ta-op-object.md)
- [docs/191-sgx-submit-ta-scheduler-object.md](/root/Poulsbo/docs/191-sgx-submit-ta-scheduler-object.md)
- [docs/192-sgx-submit-ta-sequence-object.md](/root/Poulsbo/docs/192-sgx-submit-ta-sequence-object.md)
- [docs/193-sgx-submit-ta-work-queue-object.md](/root/Poulsbo/docs/193-sgx-submit-ta-work-queue-object.md)
- [docs/194-sgx-submit-ta-job-object.md](/root/Poulsbo/docs/194-sgx-submit-ta-job-object.md)
- [docs/195-sgx-submit-ta-request-object.md](/root/Poulsbo/docs/195-sgx-submit-ta-request-object.md)
