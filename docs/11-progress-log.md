# Progress Log

This file is the dated execution log for the repo. It records what was changed,
what was validated, and what conclusion was established at each step.

## 2026-05-15

### SGX submit TA submit-op request-endpoint object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_endpoint`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_request_frontend` to
  `sgx_ta_submit_op_request_endpoint`:
  - the shared top submit-op request-endpoint boundary is now returned through
    `sgx_ta_submit_op_request_endpoint`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_endpoint`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_endpoint`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_endpoint`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_request_frontend`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-endpoint boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_request_endpoint`
    - `transition_request_endpoint`
    - `resolved_submit_id`
    - `request_endpoint_op`
- The old shared outer execute / resolve / free seam is no longer open-coded
  in `poulsbo_sgx_issue_submit_op_request()` /
  `poulsbo_sgx_transition_submit_op_request()`.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260515-035735-sgx-ta-submit-op-request-endpoint-deploy`
  - final live override:
    - `driver_srcversion=0844E58E37B9982B2D11C3F`
    - `override_module_size=506656`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260515-035937-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=0844E58E37B9982B2D11C3F`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260515-040327-sgx-submit-ta-submit-op-request-endpoint-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=11`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260515-040327-sgx-multi-submit-ta-submit-op-request-endpoint-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=9`
    - `SUBMIT_B_ID=10`
    - submit A final:
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=10`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260515-035937`
- Documented in:
  - `docs/247-sgx-submit-ta-submit-op-request-endpoint-object.md`

### SGX submit TA submit-op request-frontend object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_frontend`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_request_trace` to
  `sgx_ta_submit_op_request_frontend`:
  - the shared top submit-op request-frontend boundary is now returned through
    `sgx_ta_submit_op_request_frontend`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_frontend`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_frontend`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_frontend`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_request_trace`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-frontend boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_request_frontend`
    - `transition_request_frontend`
    - `resolved_submit_id`
    - `request_frontend_op`
- The old shared outer caller-facing wrapper seam is no longer open-coded in
  `poulsbo_sgx_issue_submit_op_request()` /
  `poulsbo_sgx_transition_submit_op_request()`.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260515-034251-sgx-ta-submit-op-request-frontend-deploy`
  - final live override:
    - `driver_srcversion=77BBC4071B0DC6393A76C80`
    - `override_module_size=506420`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260515-034512-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=77BBC4071B0DC6393A76C80`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260515-034625-sgx-submit-ta-submit-op-request-frontend-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=5`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260515-034724-sgx-multi-submit-ta-submit-op-request-frontend-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=8`
    - `SUBMIT_B_ID=9`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=6`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260515-034512`
- Documented in:
  - `docs/246-sgx-submit-ta-submit-op-request-frontend-object.md`

### SGX submit TA submit-op request-trace object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_trace`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_request_report` to
  `sgx_ta_submit_op_request_trace`:
  - the shared top submit-op request-trace boundary is now returned through
    `sgx_ta_submit_op_request_trace`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_trace`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_trace`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_trace`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_request_report`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-trace boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_request_trace`
    - `transition_request_trace`
    - `resolved_submit_id`
    - `request_trace_op`
- The old shared outer wrapper seam is no longer open-coded in
  `poulsbo_sgx_issue_submit_op_request()` /
  `poulsbo_sgx_transition_submit_op_request()`.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260515-032419-sgx-ta-submit-op-request-trace-deploy`
  - final live override:
    - `driver_srcversion=02FE6E9BFD5534B9D451723`
    - `override_module_size=506048`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260515-032604-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=02FE6E9BFD5534B9D451723`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260515-032718-sgx-submit-ta-submit-op-request-trace-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=4`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260515-032832-sgx-multi-submit-ta-submit-op-request-trace-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=7`
    - `SUBMIT_B_ID=8`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260515-032604`
- Documented in:
  - `docs/245-sgx-submit-ta-submit-op-request-trace-object.md`

### SGX submit TA submit-op request-report object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_report`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_request_path` to
  `sgx_ta_submit_op_request_report`:
  - the shared top submit-op request-report boundary is now returned through
    `sgx_ta_submit_op_request_report`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_report`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_report`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_report`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_request_path`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-report boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_request_report`
    - `transition_request_report`
    - `resolved_submit_id`
    - `request_report_op`
- The old shared outer execute/select/free seam is no longer open-coded in
  `poulsbo_sgx_issue_submit_op_request()` /
  `poulsbo_sgx_transition_submit_op_request()`.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260515-031235-sgx-ta-submit-op-request-report-deploy`
  - final live override:
    - `driver_srcversion=277948F7EDD16293BBAB8EC`
    - `override_module_size=505812`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260515-031353-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=277948F7EDD16293BBAB8EC`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260515-031519-sgx-submit-ta-submit-op-request-report-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=4`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260515-031607-sgx-multi-submit-ta-submit-op-request-report-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=6`
    - `SUBMIT_B_ID=7`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260515-031353`
- Documented in:
  - `docs/244-sgx-submit-ta-submit-op-request-report-object.md`

### SGX submit TA submit-op request-path object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_path`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_request_control` to
  `sgx_ta_submit_op_request_path`:
  - the shared top submit-op request-path boundary is now returned through
    `sgx_ta_submit_op_request_path`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_path`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_path`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_path`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_request_control`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-path boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_request_path`
    - `transition_request_path`
    - `resolved_submit_id`
    - `request_path_op`
- The old outer caller-facing seam is no longer open-coded in
  `poulsbo_sgx_issue_submit_op_request()` /
  `poulsbo_sgx_transition_submit_op_request()`.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260515-024942-sgx-ta-submit-op-request-path-deploy`
  - final live override:
    - `driver_srcversion=1720744908BE47B74AD3D6A`
    - `override_module_size=505504`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260515-025343-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=1720744908BE47B74AD3D6A`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260515-025458-sgx-submit-ta-submit-op-request-path-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=7`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260515-025545-sgx-multi-submit-ta-submit-op-request-path-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=8`
    - `SUBMIT_B_ID=9`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=6`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260515-025343`
- Documented in:
  - `docs/243-sgx-submit-ta-submit-op-request-path-object.md`

### SGX submit TA submit-op request-control object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_control`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_request_operation` to
  `sgx_ta_submit_op_request_control`:
  - the shared top submit-op request-control boundary is now returned through
    `sgx_ta_submit_op_request_control`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_control`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_control`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_control`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_request_operation`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-control boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_request_control`
    - `transition_request_control`
    - `resolved_submit_id`
    - `request_control_op`
- The old outer wrapper seam is no longer open-coded in
  `poulsbo_sgx_issue_submit_op_request()` /
  `poulsbo_sgx_transition_submit_op_request()`.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260515-023022-sgx-ta-submit-op-request-control-deploy`
  - final live override:
    - `driver_srcversion=C47F2A26CB123616B78758F`
    - `override_module_size=505064`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260515-023209-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=C47F2A26CB123616B78758F`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260515-023326-sgx-submit-ta-submit-op-request-control-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=4`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260515-023422-sgx-multi-submit-ta-submit-op-request-control-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=7`
    - `SUBMIT_B_ID=8`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260515-023209`
- Documented in:
  - `docs/242-sgx-submit-ta-submit-op-request-control-object.md`

### SGX submit TA submit-op request-operation object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_operation`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_request_resolution` to
  `sgx_ta_submit_op_request_operation`:
  - the shared top submit-op request-operation boundary is now built,
    validated, applied, and returned through
    `sgx_ta_submit_op_request_operation`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_operation`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_operation`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_operation`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_request_resolution`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-operation boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_request_operation`
    - `transition_request_operation`
    - `resolved_submit_id`
    - `request_operation_op`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_resolution()` helper is
  now gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260515-021218-sgx-ta-submit-op-request-operation-deploy`
  - final live override:
    - `driver_srcversion=928BFFEE9AB2937A3205A34`
    - `override_module_size=504836`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260515-021657-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=928BFFEE9AB2937A3205A34`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260515-021801-sgx-submit-ta-submit-op-request-operation-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=7`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260515-021838-sgx-multi-submit-ta-submit-op-request-operation-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=8`
    - `SUBMIT_B_ID=9`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=6`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260515-021657`
- Documented in:
  - `docs/241-sgx-submit-ta-submit-op-request-operation-object.md`

### SGX submit TA submit-op request-resolution object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_resolution`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_request_return` to
  `sgx_ta_submit_op_request_resolution`:
  - the shared top submit-op request-resolution boundary is now built,
    validated, applied, and returned through
    `sgx_ta_submit_op_request_resolution`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_resolution`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_resolution`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_resolution`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_request_return`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-resolution boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_request_resolution`
    - `transition_request_resolution`
    - `resolved_submit_id`
    - `request_resolution_op`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_return()` helper is
  now gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260515-015254-sgx-plan-guarded-deploy`
  - final live override:
    - `driver_srcversion=1221DD3D17A576383166171`
    - `override_module_size=504420`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260515-015450-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=1221DD3D17A576383166171`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260515-015913-sgx-submit-ta-submit-op-request-resolution-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=10`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260515-015958-sgx-multi-submit-ta-submit-op-request-resolution-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=11`
    - `SUBMIT_B_ID=12`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=12`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260515-015450`
- Documented in:
  - `docs/240-sgx-submit-ta-submit-op-request-resolution-object.md`

### SGX submit TA submit-op request-return object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_return`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_request_response` to
  `sgx_ta_submit_op_request_return`:
  - the shared top submit-op request-return boundary is now built,
    validated, applied, and returned through
    `sgx_ta_submit_op_request_return`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_return`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_return`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_return`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_request_response`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-return boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_request_return`
    - `transition_request_return`
    - `resolved_submit_id`
    - `request_return_op`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_response()` helper is
  now gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260515-013605-sgx-plan-guarded-deploy`
  - final live override:
    - `driver_srcversion=D3F95FD4D0EC2226B009D58`
    - `override_module_size=504196`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260515-013801-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=D3F95FD4D0EC2226B009D58`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260515-013917-sgx-submit-ta-submit-op-request-return-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=5`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260515-014017-sgx-multi-submit-ta-submit-op-request-return-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=7`
    - `SUBMIT_B_ID=8`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260515-013801`
- Documented in:
  - `docs/239-sgx-submit-ta-submit-op-request-return-object.md`

### SGX submit TA submit-op request-response object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_response`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_request_result` to
  `sgx_ta_submit_op_request_response`:
  - the shared top submit-op request-response boundary is now built,
    validated, applied, and returned through
    `sgx_ta_submit_op_request_response`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_response`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_response`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_response`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_request_result`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-response boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_request_response`
    - `transition_request_response`
    - `resolved_submit_id`
    - `request_response_op`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_result()` helper is
  now gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260515-012406-sgx-plan-guarded-deploy`
  - final live override:
    - `driver_srcversion=FAB900381E110736AB21FFD`
    - `override_module_size=503968`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260515-012613-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=FAB900381E110736AB21FFD`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260515-012724-sgx-submit-ta-submit-op-request-response-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=5`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260515-012818-sgx-multi-submit-ta-submit-op-request-response-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=7`
    - `SUBMIT_B_ID=8`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260515-012613`
- Documented in:
  - `docs/238-sgx-submit-ta-submit-op-request-response-object.md`

### SGX submit TA submit-op request-result object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_result`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_request_dispatch` to
  `sgx_ta_submit_op_request_result`:
  - the shared top submit-op request-result boundary is now built,
    validated, applied, and returned through
    `sgx_ta_submit_op_request_result`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_result`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_result`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_result`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_request_dispatch`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-result boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_request_result`
    - `transition_request_result`
    - `resolved_submit_id`
    - `request_result_op`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_dispatch()` helper is
  now gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260515-010522-sgx-plan-guarded-deploy`
  - final live override:
    - `driver_srcversion=C1B63173499BF0FAF42F76C`
    - `override_module_size=503676`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260515-010803-sgx-plan-session`
  - `session_outcome=no_bad_prefix_found`
  - `selected_phase_profile=complete`
  - `bisect_outcome=all_prefixes_healthy`
  - `final_driver_srcversion=C1B63173499BF0FAF42F76C`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260515-010919-sgx-submit-ta-submit-op-request-result-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=6`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260515-011009-sgx-multi-submit-ta-submit-op-request-result-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=8`
    - `SUBMIT_B_ID=9`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=6`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260515-010803`
- Documented in:
  - `docs/237-sgx-submit-ta-submit-op-request-result-object.md`

### SGX submit TA submit-op request-dispatch object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_dispatch`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_request_call` to
  `sgx_ta_submit_op_request_dispatch`:
  - the shared top submit-op request-dispatch boundary is now built,
    validated, applied, and returned through
    `sgx_ta_submit_op_request_dispatch`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_dispatch`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_dispatch`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_dispatch`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_request_call`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-dispatch boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_request_dispatch`
    - `transition_request_dispatch`
    - `resolved_submit_id`
    - `request_dispatch_op`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_call()` helper is
  now gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260515-004159-sgx-plan-guarded-deploy`
  - final live override:
    - `driver_srcversion=0EAAC21E19AD43D56E796F7`
    - `override_module_size=503392`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260515-004354-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=0EAAC21E19AD43D56E796F7`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260515-004510-sgx-submit-ta-submit-op-request-dispatch-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=4`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260515-004510-sgx-multi-submit-ta-submit-op-request-dispatch-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=6`
    - `SUBMIT_B_ID=7`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260515-004354`
- Documented in:
  - `docs/236-sgx-submit-ta-submit-op-request-dispatch-object.md`

### SGX submit TA submit-op request-call object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_call`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_request_entry` to
  `sgx_ta_submit_op_request_call`:
  - the shared top submit-op request-call boundary is now built,
    validated, applied, and returned through
    `sgx_ta_submit_op_request_call`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_call`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_call`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_call`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_request_entry`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-call boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_call`
    - `transition_call`
    - `resolved_submit_id`
    - `call_op`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_entry()` helper is
  now gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260515-001922-sgx-plan-guarded-deploy`
  - final live override:
    - `driver_srcversion=3BC4CF1B1CF3FDAD1E6DEF5`
    - `override_module_size=499168`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260515-002122-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=3BC4CF1B1CF3FDAD1E6DEF5`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260515-002233-sgx-submit-ta-submit-op-request-call-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=5`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260515-002322-sgx-multi-submit-ta-submit-op-request-call-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=7`
    - `SUBMIT_B_ID=8`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260515-002122`
- Documented in:
  - `docs/235-sgx-submit-ta-submit-op-request-call-object.md`

## 2026-05-14

### SGX submit TA submit-op request-entry object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_entry`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_request_wrapper` to
  `sgx_ta_submit_op_request_entry`:
  - the shared top submit-op request-entry boundary is now built,
    validated, applied, and returned through
    `sgx_ta_submit_op_request_entry`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_entry`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_entry`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_entry`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_request_wrapper`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-entry boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_entry`
    - `transition_entry`
    - `resolved_submit_id`
    - `entry_op`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_wrapper()` helper is
  now gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260514-235311-sgx-plan-guarded-deploy`
  - final live override:
    - `driver_srcversion=5DCA8E24A0C3DF81B31F3A3`
    - `override_module_size=498876`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-235519-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=5DCA8E24A0C3DF81B31F3A3`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-235758-sgx-submit-ta-submit-op-request-entry-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=8`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-235837-sgx-multi-submit-ta-submit-op-request-entry-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=9`
    - `SUBMIT_B_ID=10`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=7`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-235519`
- Documented in:
  - `docs/234-sgx-submit-ta-submit-op-request-entry-object.md`

### SGX submit TA submit-op request-wrapper object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_wrapper`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_request_handler` to
  `sgx_ta_submit_op_request_wrapper`:
  - the shared top submit-op request-wrapper boundary is now built,
    validated, applied, and returned through
    `sgx_ta_submit_op_request_wrapper`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_wrapper`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_wrapper`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_wrapper`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_request_handler`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-wrapper boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_wrapper`
    - `transition_wrapper`
    - `resolved_submit_id`
    - `wrapper_op`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_request_handler()` helper is
  now gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260514-233803-sgx-plan-guarded-deploy`
  - final live override:
    - `driver_srcversion=97FDB7B111442F373B52DD9`
    - `override_module_size=498652`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-233951-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=97FDB7B111442F373B52DD9`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-234106-sgx-submit-ta-submit-op-request-wrapper-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=4`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-234146-sgx-multi-submit-ta-submit-op-request-wrapper-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=6`
    - `SUBMIT_B_ID=7`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-233951`
- Documented in:
  - `docs/233-sgx-submit-ta-submit-op-request-wrapper-object.md`

### SGX submit TA submit-op request-handler object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request_handler`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_handler` to `sgx_ta_submit_op_request_handler`:
  - the shared top submit-op request-handler boundary is now built,
    validated, applied, and returned through
    `sgx_ta_submit_op_request_handler`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_request_handler`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_handler`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_request_handler`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_handler`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top request-wrapper boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_request`
    - `transition_request`
    - `resolved_submit_id`
    - `request_op_kind`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_handler()` helper is now gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260514-232007-sgx-plan-guarded-deploy`
  - final live override:
    - `driver_srcversion=9C220021FAA75E1D90BF8B1`
    - `override_module_size=498304`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-232200-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=9C220021FAA75E1D90BF8B1`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-232318-sgx-submit-ta-submit-op-request-handler-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=4`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-232403-sgx-multi-submit-ta-submit-op-request-handler-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=6`
    - `SUBMIT_B_ID=7`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-232200`
- Documented in:
  - `docs/232-sgx-submit-ta-submit-op-request-handler-object.md`

### SGX submit TA submit-op handler object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_handler`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_dispatch` to `sgx_ta_submit_op_handler`:
  - the shared top submit-op handler boundary is now built, validated,
    applied, and returned through `sgx_ta_submit_op_handler`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_handler`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_handler`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_handler`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_dispatch`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top handler boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_handler`
    - `transition_handler`
    - `resolved_submit_id`
    - `handler_op`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_dispatch()` helper is now
  gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260514-225032-sgx-ta-submit-op-handler-deploy`
  - final live override:
    - `driver_srcversion=383517C47739C7E1BFCC41A`
    - `override_module_size=498072`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-225346-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=383517C47739C7E1BFCC41A`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-225459-sgx-submit-ta-submit-op-handler-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=7`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-225540-sgx-multi-submit-ta-submit-op-handler-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=8`
    - `SUBMIT_B_ID=9`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=6`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-225346`
- Documented in:
  - `docs/231-sgx-submit-ta-submit-op-handler-object.md`

### SGX submit TA submit-op dispatch object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_dispatch`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_result` to `sgx_ta_submit_op_dispatch`:
  - the shared top submit-op dispatch boundary is now built, validated,
    applied, and returned through `sgx_ta_submit_op_dispatch`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_dispatch`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_dispatch`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_dispatch`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_result`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top dispatch boundary:
    - `active`
    - `dependency_fields_present`
    - `issue_dispatch`
    - `transition_dispatch`
    - `resolved_submit_id`
    - `dispatch_op`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_result()` helper is now gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260514-212415-sgx-ta-submit-op-dispatch-deploy`
  - final live override:
    - `driver_srcversion=DC5593F1A1D8FF7ED0012E8`
    - `override_module_size=497712`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-212600-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=DC5593F1A1D8FF7ED0012E8`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-212715-sgx-submit-ta-submit-op-dispatch-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=4`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-212758-sgx-multi-submit-ta-submit-op-dispatch-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=6`
    - `SUBMIT_B_ID=7`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-212600`
- Documented in:
  - `docs/230-sgx-submit-ta-submit-op-dispatch-object.md`

### SGX submit TA submit-op result object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_result`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_route` to `sgx_ta_submit_op_result`:
  - the shared top submit-op result boundary is now built, validated,
    applied, and returned through `sgx_ta_submit_op_result`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_result`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_result`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_result`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_route`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the copied final effect-visible request/work outcome:
    - `outcome`
  - the remaining top result boundary:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_valid`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `issue_entry`
    - `transition_entry`
    - `issue_call`
    - `transition_call`
    - `issue_entrypoint`
    - `transition_entrypoint`
    - `issue_route`
    - `transition_route`
    - `issue_result`
    - `transition_result`
    - `refresh_views`
    - `refilled_from_complete`
    - `issue_advanced`
    - `running_advanced`
    - `complete_advanced`
    - `active_running_changed`
    - `selected_wrapped`
    - `submit_id`
    - `requested_submit_id`
    - `selected_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `entry_op`
    - `call_op`
    - `entrypoint_op`
    - `route_op`
    - `result_op`
    - `op_kind`
    - `owner_state`
    - `packet_count`
    - `selected_read_offset`
    - `selected_write_offset`
    - `selected_ccb_offset`
    - `selected_outstanding_packets`
    - `selected_seq`
    - `selected_cookie`
    - `epoch`
    - `global_issue_ticket`
    - `global_running_ticket`
    - `global_complete_ticket`
    - `next_issue_count`
    - `next_transition_count`
    - `next_state`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_route()` helper is now gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260514-210407-sgx-ta-submit-op-result-deploy`
  - final live override:
    - `driver_srcversion=87C32C9AB200EAC73718164`
    - `override_module_size=497412`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-210559-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=87C32C9AB200EAC73718164`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-210711-sgx-submit-ta-submit-op-result-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=4`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-210754-sgx-multi-submit-ta-submit-op-result-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=6`
    - `SUBMIT_B_ID=7`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-210559`
- Documented in:
  - `docs/229-sgx-submit-ta-submit-op-result-object.md`

### SGX submit TA submit-op route object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_route`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_entrypoint` to `sgx_ta_submit_op_route`:
  - the shared top submit-op route boundary is now built, validated,
    applied, and returned through `sgx_ta_submit_op_route`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_route`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_route`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_route`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_entrypoint`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the remaining top route boundary:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_valid`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `issue_entry`
    - `transition_entry`
    - `issue_call`
    - `transition_call`
    - `issue_entrypoint`
    - `transition_entrypoint`
    - `issue_route`
    - `transition_route`
    - `submit_id`
    - `requested_submit_id`
    - `selected_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `entry_op`
    - `call_op`
    - `entrypoint_op`
    - `route_op`
    - `op_kind`
- The first guarded deploy wrapper only rebooted the previous validated
  module because the local `build/devnull-vaio-gma500-src/gma500_gfx.ko`
  artifact was stale. After rebuilding that exact deploy path, the rerun
  deployed the new route module and all proofs passed.
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_entrypoint()` helper is now
  gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260514-203347-sgx-ta-submit-op-route-rerun-deploy`
  - final live override:
    - `driver_srcversion=7698778D2FCDF9EDC3DE460`
    - `override_module_size=496000`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-203656-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=7698778D2FCDF9EDC3DE460`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-203809-sgx-submit-ta-submit-op-route-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=6`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-203853-sgx-multi-submit-ta-submit-op-route-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=8`
    - `SUBMIT_B_ID=9`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=6`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-203656`
- Documented in:
  - `docs/228-sgx-submit-ta-submit-op-route-object.md`

### SGX submit TA submit-op entrypoint object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_entrypoint`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_call` to `sgx_ta_submit_op_entrypoint`:
  - the shared top submit-op entrypoint boundary is now built, validated,
    applied, and returned through `sgx_ta_submit_op_entrypoint`
  - the submit-op ioctl path now routes through
    `sgx_ta_submit_op_entrypoint`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_entrypoint`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_entrypoint`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_call`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the remaining top entrypoint boundary:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_valid`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `issue_entry`
    - `transition_entry`
    - `issue_call`
    - `transition_call`
    - `issue_entrypoint`
    - `transition_entrypoint`
    - `submit_id`
    - `requested_submit_id`
    - `selected_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `entry_op`
    - `call_op`
    - `entrypoint_op`
    - `op_kind`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_call()` helper is now gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260514-201736-sgx-ta-submit-op-entrypoint-deploy`
  - final live override:
    - `driver_srcversion=B03227CA0E30D87724E8B38`
    - `override_module_size=495392`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-201933-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=B03227CA0E30D87724E8B38`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-202053-sgx-submit-ta-submit-op-entrypoint-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=5`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-202206-sgx-multi-submit-ta-submit-op-entrypoint-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=7`
    - `SUBMIT_B_ID=8`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-201933`
- Documented in:
  - `docs/227-sgx-submit-ta-submit-op-entrypoint-object.md`

### SGX submit TA submit-op call object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_call`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_entry` to `sgx_ta_submit_op_call`:
  - the caller-facing submit-op call boundary is now built, validated,
    applied, and returned through `sgx_ta_submit_op_call`
  - the submit-op ioctl path now routes through `sgx_ta_submit_op_call`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_call`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_call`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_entry`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the remaining top call boundary:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_valid`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `issue_entry`
    - `transition_entry`
    - `issue_call`
    - `transition_call`
    - `submit_id`
    - `requested_submit_id`
    - `selected_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `entry_op`
    - `call_op`
    - `op_kind`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_entry()` helper is now gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260514-195517-sgx-ta-submit-op-call-deploy`
  - final live override:
    - `driver_srcversion=BB4FE7048611B98999236B9`
    - `override_module_size=495524`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-195738-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=BB4FE7048611B98999236B9`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-195905-sgx-submit-ta-submit-op-call-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=5`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-200019-sgx-multi-submit-ta-submit-op-call-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=7`
    - `SUBMIT_B_ID=8`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-195738`
- Documented in:
  - `docs/226-sgx-submit-ta-submit-op-call-object.md`

### SGX submit TA submit-op entry object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_entry`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_ioctl` to `sgx_ta_submit_op_entry`:
  - the common issue-versus-transition submit-op entry boundary is now built,
    validated, applied, and returned through `sgx_ta_submit_op_entry`
  - the submit-op ioctl path now routes through `sgx_ta_submit_op_entry`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_entry`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_entry`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_ioctl`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the remaining top entry boundary:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_valid`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `issue_entry`
    - `transition_entry`
    - `submit_id`
    - `requested_submit_id`
    - `selected_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `entry_op`
    - `op_kind`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_ioctl()` helper is now gone.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260514-194105-sgx-ta-submit-op-entry-deploy`
  - final live override:
    - `driver_srcversion=12046BB01C587A8CAAB2BE3`
    - `override_module_size=494580`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-194317-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=12046BB01C587A8CAAB2BE3`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-194443-sgx-submit-ta-submit-op-entry-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=6`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-194541-sgx-multi-submit-ta-submit-op-entry-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=8`
    - `SUBMIT_B_ID=9`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=6`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-194317`
- Documented in:
  - `docs/225-sgx-submit-ta-submit-op-entry-object.md`

### SGX submit TA submit-op ioctl object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_ioctl`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_control` to `sgx_ta_submit_op_ioctl`:
  - the full submit-op ioctl request-and-reply image is now built,
    validated, applied, and returned through `sgx_ta_submit_op_ioctl`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_ioctl`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_ioctl`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_control`
  - the copied submit-op request image:
    - `request_op`
  - the copied submit-op response image:
    - `response_op`
  - the remaining top ioctl boundary:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_valid`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `submit_id`
    - `requested_submit_id`
    - `selected_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `op_kind`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_control()` helper is now
  replaced by `poulsbo_sgx_execute_submit_sgx_ta_submit_op_ioctl()`.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260514-191918-sgx-ta-submit-op-ioctl-deploy`
  - final live override:
    - `driver_srcversion=5884B548EB5E1D6C7698955`
    - `override_module_size=494168`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-192144-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=5884B548EB5E1D6C7698955`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-192303-sgx-submit-ta-submit-op-ioctl-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=3`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-192344-sgx-multi-submit-ta-submit-op-ioctl-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=4`
    - `SUBMIT_B_ID=5`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=6`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-192144`
- Documented in:
  - `docs/224-sgx-submit-ta-submit-op-ioctl-object.md`

### SGX submit TA submit-op control object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_control`
- The shared live execution boundary now moves from
  `sgx_ta_submit_op_reply` to `sgx_ta_submit_op_control`:
  - the submit-op ioctl control image is now built, validated, applied, and
    returned through `sgx_ta_submit_op_control`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_control`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_control`
- The new top object now keeps together:
  - the existing `sgx_ta_submit_op_reply`
  - the copied submit-op request image:
    - `ioctl_op`
  - the remaining top control boundary:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_valid`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `submit_id`
    - `requested_submit_id`
    - `selected_submit_id`
    - `resolved_submit_id`
    - `op_kind`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_submit_op_reply()` helper is now
  replaced by `poulsbo_sgx_execute_submit_sgx_ta_submit_op_control()`.
- Guarded deploy plus exact recovery wrapper completed successfully in:
  - `logs/vaio-plan/20260514-185945-sgx-ta-submit-op-control-deploy`
  - final live override:
    - `driver_srcversion=3052308F8E83D766FBBE045`
    - `override_module_size=493892`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-190601-sgx-plan-session`
  - `session_outcome=completed`
  - `plan_session_mode=recovery`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=3052308F8E83D766FBBE045`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-190712-sgx-submit-ta-submit-op-control-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=7`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-190759-sgx-multi-submit-ta-submit-op-control-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=8`
    - `SUBMIT_B_ID=9`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=13`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-190601`
- Documented in:
  - `docs/223-sgx-submit-ta-submit-op-control-object.md`

### SGX submit TA submit-op reply object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_reply`
- The shared live execution boundary now moves from
  `sgx_ta_request_work_response` to `sgx_ta_submit_op_reply`:
  - the submit-op ioctl reply image is now built, validated, applied, and
    returned through `sgx_ta_submit_op_reply`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_submit_op_reply`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_submit_op_reply`
- The new top object now keeps together:
  - the existing `sgx_ta_request_work_response`
  - the caller-visible submit-op reply image:
    - `response_op`
  - the remaining top reply boundary:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_valid`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `submit_id`
    - `requested_submit_id`
    - `selected_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `op_kind`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_response()` helper is now
  replaced by `poulsbo_sgx_execute_submit_sgx_ta_submit_op_reply()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-plan/20260514-184208-sgx-plan-guarded-deploy/children/20260514-184208-guarded-deploy`
  - final live override:
    - `driver_srcversion=A417E135FB014B70E256095`
    - `override_module_size=493616`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-184356-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=A417E135FB014B70E256095`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-184506-sgx-submit-ta-submit-op-reply-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=4`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-184547-sgx-multi-submit-ta-submit-op-reply-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=6`
    - `SUBMIT_B_ID=7`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-184356`
- Documented in:
  - `docs/222-sgx-submit-ta-submit-op-reply-object.md`

### SGX submit TA request-work-response object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_work_response`
- The shared live execution boundary now moves from
  `sgx_ta_request_work_outcome` to `sgx_ta_request_work_response`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_work_response`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_work_response`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_work_response`
- The new top object now keeps together:
  - the existing `sgx_ta_request_work_outcome`
  - the copied public submit-op request image:
    - `op`
  - the remaining returned-response boundary:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_valid`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `submit_id`
    - `requested_submit_id`
    - `selected_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `op_kind`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_outcome()` helper is now
  replaced by `poulsbo_sgx_execute_submit_sgx_ta_request_work_response()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-plan/20260514-182606-sgx-plan-guarded-deploy/children/20260514-182606-guarded-deploy`
  - final live override:
    - `driver_srcversion=09985BE3AA727C3FDF53E42`
    - `override_module_size=494652`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-182908-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=09985BE3AA727C3FDF53E42`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-183020-sgx-submit-ta-request-work-response-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=6`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-183104-sgx-multi-submit-ta-request-work-response-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=8`
    - `SUBMIT_B_ID=9`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=6`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-182908`
- Documented in:
  - `docs/221-sgx-submit-ta-request-work-response-object.md`

### SGX submit TA request-work-outcome object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_work_outcome`
- The shared live execution boundary now moves from
  `sgx_ta_request_work_submit_op` to `sgx_ta_request_work_outcome`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_work_outcome`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_work_outcome`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_work_outcome`
- The new top object now keeps together:
  - the existing `sgx_ta_request_work_submit_op`
  - the copied public submit-op request image:
    - `op`
  - the remaining top execution-result boundary:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_valid`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `refresh_views`
    - `refilled_from_complete`
    - `issue_advanced`
    - `running_advanced`
    - `complete_advanced`
    - `active_running_changed`
    - `selected_wrapped`
    - `submit_id`
    - `requested_submit_id`
    - `selected_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `op_kind`
    - `owner_state`
    - `packet_count`
    - `selected_read_offset`
    - `selected_write_offset`
    - `selected_ccb_offset`
    - `selected_outstanding_packets`
    - `selected_seq`
    - `selected_cookie`
    - `epoch`
    - `global_issue_ticket`
    - `global_running_ticket`
    - `global_complete_ticket`
    - `next_issue_count`
    - `next_transition_count`
    - `next_state`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_submit_op()` helper is now
  dropped; the live execute path is owned by
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_outcome()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260514-180613-sgx-ta-request-work-outcome-deploy`
  - final live override:
    - `driver_srcversion=EB358736E802BE490602A17`
    - `override_module_size=489808`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-180758-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=EB358736E802BE490602A17`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-181225-sgx-submit-ta-request-work-outcome-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=5`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-181305-sgx-multi-submit-ta-request-work-outcome-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=6`
    - `SUBMIT_B_ID=7`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=11`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-180758`
- Documented in:
  - `docs/220-sgx-submit-ta-request-work-outcome-object.md`

### SGX submit TA request-work-submit-op object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_work_submit_op`
- The shared live execution boundary now moves from
  `sgx_ta_request_work_invocation` to `sgx_ta_request_work_submit_op`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_work_submit_op`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_work_submit_op`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_work_submit_op`
- The new top object now keeps together:
  - the existing `sgx_ta_request_work_invocation`
  - the copied public submit-op request image:
    - `op`
  - the remaining top submit-op selection boundary:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_valid`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `submit_id`
    - `requested_submit_id`
    - `selected_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `op_kind`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_invocation()` helper is now
  dropped; the live execute path is owned by
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_submit_op()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260514-174605-sgx-ta-request-work-submit-op-deploy`
  - final live override:
    - `driver_srcversion=C582E9B3D60213B4F150F67`
    - `override_module_size=493780`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-174749-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=C582E9B3D60213B4F150F67`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-174851-sgx-submit-ta-request-work-submit-op-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=2`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-174930-sgx-multi-submit-ta-request-work-submit-op-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-174749`
- Documented in:
  - `docs/219-sgx-submit-ta-request-work-submit-op-object.md`

### SGX submit TA request-work-invocation object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_work_invocation`
- The shared live execution boundary now moves from
  `sgx_ta_request_work_run` to `sgx_ta_request_work_invocation`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_work_invocation`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_work_invocation`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_work_invocation`
- The new top object now keeps together:
  - the existing `sgx_ta_request_work_run`
  - the copied public submit-op request image:
    - `op`
  - the remaining top request-selection boundary:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `submit_id`
    - `requested_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `op_kind`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_run()` helper is now
  dropped; the live execute path is owned by
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_invocation()`.
- First draft exposed a real regression in the dependent multi-submit path:
  - `logs/vaio-deploy/20260514-165414-sgx-ta-request-work-invocation-deploy`
  - `logs/vaio-ioctl/20260514-165702-sgx-multi-submit-ta-request-work-invocation-live/summary.log`
  - the new builder incorrectly flattened nested nonzero returns, which
    converted the expected blocked-transition `-EAGAIN` into `-EINVAL`
- The fixed draft then completed guarded deploy successfully in:
  - `logs/vaio-deploy/20260514-170003-sgx-ta-request-work-invocation-rerun-deploy`
  - final live override:
    - `driver_srcversion=FABCE60FE28BC5F9B747826`
    - `override_module_size=488560`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that fixed module still passed:
  - `logs/vaio-plan-session/20260514-170152-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=FABCE60FE28BC5F9B747826`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-170256-sgx-submit-ta-request-work-invocation-live-rerun/summary.log`
  - final focused proof:
    - `submit_resolved_id=2`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-170256-sgx-multi-submit-ta-request-work-invocation-live-rerun/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-170152`
- Documented in:
  - `docs/218-sgx-submit-ta-request-work-invocation-object.md`

### SGX submit TA request-work-run object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_work_run`
- The shared live execution boundary now moves from
  `sgx_ta_request_work_task` to `sgx_ta_request_work_run`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_work_run`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_work_run`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_work_run`
- The new top object now keeps together:
  - the existing `sgx_ta_request_work_task`
  - the copied public submit-op request image:
    - `op`
  - the exact top request/execution boundary:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `refresh_views`
    - `refilled_from_complete`
    - `issue_advanced`
    - `running_advanced`
    - `complete_advanced`
    - `active_running_changed`
    - `selected_wrapped`
    - `submit_id`
    - `requested_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `op_kind`
    - `owner_state`
    - `packet_count`
    - `selected_read_offset`
    - `selected_write_offset`
    - `selected_ccb_offset`
    - `selected_outstanding_packets`
    - `selected_seq`
    - `selected_cookie`
    - `epoch`
    - `global_issue_ticket`
    - `global_running_ticket`
    - `global_complete_ticket`
    - `next_issue_count`
    - `next_transition_count`
    - `next_state`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_task()` helper is now
  dropped; the live execute path is owned by
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_run()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260514-163445-sgx-ta-request-work-run-deploy`
  - final live override:
    - `driver_srcversion=A57696CAA2F570377CAE40B`
    - `override_module_size=488344`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-163630-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=A57696CAA2F570377CAE40B`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-163734-sgx-submit-ta-request-work-run-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=2`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-163734-sgx-multi-submit-ta-request-work-run-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-163630`
- Documented in:
  - `docs/217-sgx-submit-ta-request-work-run-object.md`

### SGX submit TA request-work-task object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_work_task`
- The shared live execution boundary now moves from
  `sgx_ta_request_work_action` to `sgx_ta_request_work_task`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_work_task`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_work_task`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_work_task`
- The new top object now keeps together:
  - the existing `sgx_ta_request_work_action`
  - the exact final live execution result:
    - `active`
    - `issue_request`
    - `transition_request`
    - `refresh_views`
    - `refilled_from_complete`
    - `issue_advanced`
    - `running_advanced`
    - `complete_advanced`
    - `active_running_changed`
    - `selected_wrapped`
    - `submit_id`
    - `resolved_submit_id`
    - `owner_state`
    - `packet_count`
    - `selected_read_offset`
    - `selected_write_offset`
    - `selected_ccb_offset`
    - `selected_outstanding_packets`
    - `selected_seq`
    - `selected_cookie`
    - `epoch`
    - `global_issue_ticket`
    - `global_running_ticket`
    - `global_complete_ticket`
    - `next_issue_count`
    - `next_transition_count`
    - `next_state`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_action()` helper is now
  dropped; the live execute path is owned by
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_task()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260514-162142-sgx-ta-request-work-task-deploy`
  - final live override:
    - `driver_srcversion=C75813FD38ABA9D6D10A7A9`
    - `override_module_size=487744`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-162333-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=C75813FD38ABA9D6D10A7A9`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-162436-sgx-submit-ta-request-work-task-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=2`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-162436-sgx-multi-submit-ta-request-work-task-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-162333`
- Documented in:
  - `docs/216-sgx-submit-ta-request-work-task-object.md`

### SGX submit TA request-work-action object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_work_action`
- The shared live execution boundary now moves from
  `sgx_ta_request_work_state` to `sgx_ta_request_work_action`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_work_action`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_work_action`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_work_action`
- The new top object now keeps together:
  - the existing `sgx_ta_request_work_state`
  - the copied public submit-op request image:
    - `op`
  - the remaining request-resolution boundary:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `submit_id`
    - `requested_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `op_kind`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_state()` helper is now
  dropped; the live execute path is owned by
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_action()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260514-160925-sgx-ta-request-work-action-deploy`
  - final live override:
    - `driver_srcversion=F2C027054B485A0EB632D40`
    - `override_module_size=489416`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-161114-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=F2C027054B485A0EB632D40`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-161217-sgx-submit-ta-request-work-action-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=4`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-161217-sgx-multi-submit-ta-request-work-action-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=2`
    - `SUBMIT_B_ID=3`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=4`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-161114`
- Documented in:
  - `docs/215-sgx-submit-ta-request-work-action-object.md`

### SGX submit TA request-work-state object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_work_state`
- The shared live execution boundary now moves from
  `sgx_ta_request_work_event` to `sgx_ta_request_work_state`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_work_state`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_work_state`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_work_state`
- The new top object now keeps together:
  - the existing `sgx_ta_request_work_event`
  - the exact post-apply top state image:
    - `active`
    - `issue_request`
    - `transition_request`
    - `refresh_views`
    - `refilled_from_complete`
    - `issue_advanced`
    - `running_advanced`
    - `complete_advanced`
    - `active_running_changed`
    - `selected_wrapped`
    - `submit_id`
    - `resolved_submit_id`
    - `owner_state`
    - `packet_count`
    - `selected_read_offset`
    - `selected_write_offset`
    - `selected_ccb_offset`
    - `selected_outstanding_packets`
    - `selected_seq`
    - `selected_cookie`
    - `epoch`
    - `global_issue_ticket`
    - `global_running_ticket`
    - `global_complete_ticket`
    - `next_issue_count`
    - `next_transition_count`
    - `next_state`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_event()` helper is now
  dropped; the live execute path is owned by
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_state()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260514-155500-sgx-ta-request-work-state-deploy`
  - final live override:
    - `driver_srcversion=AD04A2CE076B73340EDFAE4`
    - `override_module_size=488604`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-155648-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=AD04A2CE076B73340EDFAE4`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-155808-sgx-submit-ta-request-work-state-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=2`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-155808-sgx-multi-submit-ta-request-work-state-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-155648`
- Documented in:
  - `docs/214-sgx-submit-ta-request-work-state-object.md`

### SGX submit TA request-work-event object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_work_event`
- The shared live execution boundary now moves from
  `sgx_ta_request_work_operation` to `sgx_ta_request_work_event`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_work_event`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_work_event`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_work_event`
- The new top object now keeps together:
  - the existing `sgx_ta_request_work_operation`
  - the pure effect-visible live event state:
    - `active`
    - `issue_request`
    - `transition_request`
    - `refresh_views`
    - `refilled_from_complete`
    - `issue_advanced`
    - `running_advanced`
    - `complete_advanced`
    - `active_running_changed`
    - `selected_wrapped`
    - `submit_id`
    - `resolved_submit_id`
    - `owner_state`
    - `packet_count`
    - `selected_read_offset`
    - `selected_write_offset`
    - `selected_ccb_offset`
    - `selected_outstanding_packets`
    - `selected_seq`
    - `selected_cookie`
    - `epoch`
    - `global_issue_ticket`
    - `global_running_ticket`
    - `global_complete_ticket`
    - `next_issue_count`
    - `next_transition_count`
    - `next_state`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_operation()` helper is now
  dropped; the live execute path is owned by
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_event()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260514-154213-sgx-ta-request-work-event-deploy`
  - final live override:
    - `driver_srcversion=0B7BEF6C22715A3E3A6B5A8`
    - `override_module_size=488140`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-154358-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=0B7BEF6C22715A3E3A6B5A8`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-154457-sgx-submit-ta-request-work-event-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=2`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-154537-sgx-multi-submit-ta-request-work-event-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-154358`
- Documented in:
  - `docs/213-sgx-submit-ta-request-work-event-object.md`

### SGX submit TA request-work-operation object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_work_operation`
- The shared live execution boundary now moves from
  `sgx_ta_request_work_request` to `sgx_ta_request_work_operation`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_work_operation`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_work_operation`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_work_operation`
- The new top object now keeps together:
  - the existing `sgx_ta_request_work_request`
  - the copied public submit-op request image:
    - `op`
  - the exact effect-visible top request/work state:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `refresh_views`
    - `refilled_from_complete`
    - `issue_advanced`
    - `running_advanced`
    - `complete_advanced`
    - `active_running_changed`
    - `selected_wrapped`
    - `submit_id`
    - `requested_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `op_kind`
    - `owner_state`
    - `packet_count`
    - `selected_read_offset`
    - `selected_write_offset`
    - `selected_ccb_offset`
    - `selected_outstanding_packets`
    - `selected_seq`
    - `selected_cookie`
    - `epoch`
    - `global_issue_ticket`
    - `global_running_ticket`
    - `global_complete_ticket`
    - `next_issue_count`
    - `next_transition_count`
    - `next_state`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_request()` helper is now
  dropped; the live execute path is owned by
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_operation()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260514-152607-sgx-ta-request-work-operation-deploy`
  - final live override:
    - `driver_srcversion=112AF92C8FB3968C2AC9AA5`
    - `override_module_size=487680`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-152753-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=112AF92C8FB3968C2AC9AA5`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-152853-sgx-submit-ta-request-work-operation-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=2`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-152932-sgx-multi-submit-ta-request-work-operation-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-152753`
- Documented in:
  - `docs/212-sgx-submit-ta-request-work-operation-object.md`

### SGX submit TA request-work-request object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_work_request`
- The shared live execution boundary now moves from
  `sgx_ta_request_work_record` to `sgx_ta_request_work_request`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_work_request`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_work_request`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_work_request`
- The new top object now keeps together:
  - the existing `sgx_ta_request_work_record`
  - the copied public submit-op request image:
    - `op`
  - the selected request-side public boundary:
    - `active`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `submit_id`
    - `requested_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `op_kind`
    - `owner_state`
    - `packet_count`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_record()` helper is now
  dropped; the live execute path is owned by
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_request()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260514-151223-sgx-ta-request-work-request-deploy`
  - final live override:
    - `driver_srcversion=EF5B87D1980AA1E469E8723`
    - `override_module_size=487144`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-151409-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=EF5B87D1980AA1E469E8723`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-151513-sgx-submit-ta-request-work-request-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=4`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-151513-sgx-multi-submit-ta-request-work-request-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=2`
    - `SUBMIT_B_ID=3`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=4`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-151409`
- Documented in:
  - `docs/211-sgx-submit-ta-request-work-request-object.md`

### SGX submit TA request-work-record object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_work_record`
- The shared live execution boundary now moves from
  `sgx_ta_request_work_result` to `sgx_ta_request_work_record`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_work_record`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_work_record`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_work_record`
- The new top object now keeps together:
  - the existing `sgx_ta_request_work_result`
  - the exact effect-visible live work record:
    - `active`
    - `issue_request`
    - `transition_request`
    - `submit_id`
    - `resolved_submit_id`
    - `owner_state`
    - `packet_count`
    - `refresh_views`
    - `refilled_from_complete`
    - `issue_advanced`
    - `running_advanced`
    - `complete_advanced`
    - `active_running_changed`
    - `selected_wrapped`
    - `selected_read_offset`
    - `selected_write_offset`
    - `selected_ccb_offset`
    - `selected_outstanding_packets`
    - `selected_seq`
    - `selected_cookie`
    - `epoch`
    - `global_issue_ticket`
    - `global_running_ticket`
    - `global_complete_ticket`
    - `next_issue_count`
    - `next_transition_count`
    - `next_state`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_result()` helper is now
  dropped; the live execute path is owned by
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_record()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260514-145539-sgx-ta-request-work-record-deploy`
  - final live override:
    - `driver_srcversion=06C3675E67B2E675955E36E`
    - `override_module_size=485332`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-145730-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=06C3675E67B2E675955E36E`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-145831-sgx-submit-ta-request-work-record-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=2`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-145831-sgx-multi-submit-ta-request-work-record-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_resolved_id=3`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_resolved_id=4`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-145730`
- Documented in:
  - `docs/210-sgx-submit-ta-request-work-record-object.md`

### SGX submit TA request-work-result object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_work_result`
- The shared live execution boundary now moves from
  `sgx_ta_request_work_entry` to `sgx_ta_request_work_result`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_work_result`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_work_result`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_work_result`
- The new top object now keeps together:
  - the existing `sgx_ta_request_work_entry`
  - the effect-visible top request result:
    - `active`
    - `issue_request`
    - `transition_request`
    - `submit_id`
    - `resolved_submit_id`
    - `owner_state`
    - `packet_count`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_entry()` helper is now
  dropped; the live execute path is owned by
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_result()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260514-142930-sgx-ta-request-work-result-deploy`
  - final live override:
    - `driver_srcversion=ECD37083ADA9AA31E6D4661`
    - `override_module_size=484868`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-143114-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=ECD37083ADA9AA31E6D4661`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-143218-sgx-submit-ta-request-work-result-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=2`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-143218-sgx-multi-submit-ta-request-work-result-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_resolved_id=3`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_resolved_id=4`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-143114`
- Documented in:
  - `docs/209-sgx-submit-ta-request-work-result-object.md`

### SGX submit TA request-work-entry object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_work_entry`
- The shared live execution boundary now moves from
  `sgx_ta_request_execution` to `sgx_ta_request_work_entry`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_work_entry`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_work_entry`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_work_entry`
- The new top object now keeps together:
  - the existing `sgx_ta_request_execution`
  - the selected request-side public boundary:
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `submit_id`
    - `requested_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `op_kind`
    - `owner_state`
    - `packet_count`
  - the selected request-side execution summary:
    - `packet_active`
    - `refilled_from_complete`
    - `refresh_views`
    - `increment_issue_count`
    - `increment_transition_count`
    - `update_last_epoch_jiffies`
    - `update_last_issue_jiffies`
    - `update_last_transition_jiffies`
    - `issue_advanced`
    - `running_advanced`
    - `complete_advanced`
    - `active_running_changed`
    - `selected_read_offset`
    - `selected_write_offset`
    - `selected_ccb_offset`
    - `selected_outstanding_packets`
    - `selected_wrapped`
    - `selected_seq`
    - `selected_cookie`
    - `epoch`
    - `global_issue_ticket`
    - `global_running_ticket`
    - `global_complete_ticket`
    - `next_issue_count`
    - `next_transition_count`
    - `next_state`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_execution()` helper is now
  dropped; the live execute path is owned by
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_entry()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260514-135443-sgx-ta-request-work-entry-deploy`
  - final live override:
    - `driver_srcversion=C35518A5768B47A82D11718`
    - `override_module_size=484732`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-135631-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=C35518A5768B47A82D11718`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-135734-sgx-submit-ta-request-work-entry-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=2`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-135734-sgx-multi-submit-ta-request-work-entry-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_resolved_id=3`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_resolved_id=4`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-135631`
- Documented in:
  - `docs/208-sgx-submit-ta-request-work-entry-object.md`

### SGX submit TA request-execution object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_execution`
- The shared live execution boundary now moves from
  `sgx_ta_request_work_packet` to `sgx_ta_request_execution`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_execution`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_execution`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_execution`
- The new top object now keeps together:
  - the existing `sgx_ta_request_work_packet`
  - the copied public submit-op image:
    - `op`
  - the selected request-side public boundary:
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_resolved`
    - `issue_request`
    - `transition_request`
    - `submit_id`
    - `requested_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `op_kind`
    - `owner_state`
    - `packet_count`
- The old standalone
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_packet()` helper is now
  dropped; the live execute path is owned by
  `poulsbo_sgx_execute_submit_sgx_ta_request_execution()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260514-120212-sgx-ta-request-execution-deploy`
  - final live override:
    - `driver_srcversion=8E3405DC7D157295EE2A78D`
    - `override_module_size=482316`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-120405-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=8E3405DC7D157295EE2A78D`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-120508-sgx-submit-ta-request-execution-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=2`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-120508-sgx-multi-submit-ta-request-execution-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_resolved_id=3`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_resolved_id=4`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-120405`
- Documented in:
  - `docs/207-sgx-submit-ta-request-execution-object.md`

### SGX submit TA request-work-packet object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped live top object:
  - `psb_sgx_lab_submit_sgx_ta_request_work_packet`
- The shared live execution boundary now moves from
  `sgx_ta_request_sequence` to `sgx_ta_request_work_packet`:
  - the submit-op ioctl path builds, validates, applies, and returns
    `sgx_ta_request_work_packet`
  - `issue_submit()` compatibility flow now uses
    `sgx_ta_request_work_packet`
  - `transition_submit()` compatibility flow now uses
    `sgx_ta_request_work_packet`
- The new top object now keeps together:
  - the existing `sgx_ta_request_sequence`
  - the selected request-side read/write/CCB summary:
    - `selected_read_offset`
    - `selected_write_offset`
    - `selected_ccb_offset`
    - `selected_outstanding_packets`
    - `selected_wrapped`
    - `selected_seq`
    - `selected_cookie`
    - `epoch`
  - the selected request-side public boundary:
    - `requested_submit_id`
    - `resolved_submit_id`
    - `ioctl_op`
    - `op_kind`
    - `owner_state`
    - `packet_count`
    - `dependency_fields_present`
    - `use_focus_request`
    - `focus_resolved`
  - the request-side progression result:
    - `refresh_views`
    - `increment_issue_count`
    - `increment_transition_count`
    - `issue_advanced`
    - `running_advanced`
    - `complete_advanced`
    - `active_running_changed`
    - `next_issue_count`
    - `next_transition_count`
    - `next_state`
- The old standalone `poulsbo_sgx_execute_submit_sgx_ta_request_sequence()`
  helper is now dropped; the live execute path is owned by
  `poulsbo_sgx_execute_submit_sgx_ta_request_work_packet()`.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260514-113632-sgx-ta-request-work-packet-deploy`
  - final live override:
    - `driver_srcversion=090E915D8E9ED7786CCAD27`
    - `override_module_size=481988`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-113831-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=090E915D8E9ED7786CCAD27`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-114042-sgx-submit-ta-request-work-packet-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=3`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-114119-sgx-multi-submit-ta-request-work-packet-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_resolved_id=4`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_resolved_id=5`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=6`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-113831`
- Documented in:
  - `docs/206-sgx-submit-ta-request-work-packet-object.md`

### SGX stable lab milestone soak

- Added:
  - `scripts/run-vaio-sgx-stable-soak-session.sh`
- Restored the workspace to the last known-good SGX lab kernel line instead of
  continuing the failed uncommitted `request_work_packet` experiment:
  - pinned top-level baseline:
    - `8556d312`
  - pinned kernel-subrepo baseline:
    - `a89b91f8b`
  - pinned live override:
    - `driver_srcversion=92CB8FBC111248DC82C2825`
- Rebuilt that exact local module and confirmed the rebuilt artifact matched
  the pinned live `srcversion`.
- Redeployed the same pinned build with a longer rollback grace in:
  - `logs/vaio-deploy/20260514-082755-sgx-stable-milestone-deploy`
  - final live override:
    - `driver_srcversion=92CB8FBC111248DC82C2825`
    - `override_module_size=481200`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- Added one reusable stable-soak wrapper that runs:
  - exact `complete` plan recovery
  - focused typed submit proof
  - typed multi-submit proof
  - for a configurable number of cycles under one session tree
- Ran the new stable-soak wrapper for three full cycles in:
  - `logs/vaio-stability/20260514-083309-sgx-stable-soak`
  - final wrapper result:
    - `session_outcome=completed`
    - `cycles=3`
    - `completed_cycles=3`
    - `last_overall=remote_usable`
    - `final_driver_srcversion=92CB8FBC111248DC82C2825`
- Each soak cycle completed:
  - one exact no-redeploy `plan` proof at terminal `complete`
  - one focused typed submit proof
  - one typed multi-submit proof
- The latest stable-lab milestone therefore is now:
  - exact live proof remains pinned at:
    - `logs/vaio-plan-session/20260514-002126-sgx-plan-session`
  - restored guarded deploy is:
    - `logs/vaio-deploy/20260514-082755-sgx-stable-milestone-deploy`
  - repeatability soak is:
    - `logs/vaio-stability/20260514-083309-sgx-stable-soak`
- This is explicitly a stable SGX lab milestone, not a finished desktop-ready
  driver.
- Documented in:
  - `docs/205-sgx-stable-lab-milestone.md`

### SGX submit TA request-sequence object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_request_sequence`
- The top validator now moves from `sgx_ta_request_packet` to
  `sgx_ta_request_sequence`.
- The live top-layer execution boundary is now unified too:
  - the submit-op ioctl path builds and applies `sgx_ta_request_sequence`
  - `issue_submit()` compatibility flow builds and applies
    `sgx_ta_request_sequence`
  - `transition_submit()` compatibility flow builds and applies
    `sgx_ta_request_sequence`
- The new shared top object now owns:
  - the existing `sgx_ta_request_packet`
  - the request-side refresh and counter-update intent
  - the next issue / transition counts
  - the next scheduler-state vector
  - the issue / running / complete advancement summary
- The live kernel traces now also emit that sequence-side top summary:
  - `issue` now logs `refresh_views`, `next_issue_count`,
    `issue_advanced`, `next_issue_ticket`, and `next_issue_submit_id`
  - `transition` now logs `refresh_views`, `next_transition_count`,
    `running_advanced`, `complete_advanced`, `next_running_ticket`,
    `next_complete_ticket`, and `active_running_changed`
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260514-001931-sgx-ta-request-sequence-deploy`
  - final live override:
    - `driver_srcversion=92CB8FBC111248DC82C2825`
    - `override_module_size=481200`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on that module still passed:
  - `logs/vaio-plan-session/20260514-002126-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=92CB8FBC111248DC82C2825`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260514-002243-sgx-submit-ta-request-sequence-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=2`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260514-002324-sgx-multi-submit-ta-request-sequence-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_resolved_id=3`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_resolved_id=4`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260514-002126`
- Documented in:
  - `docs/204-sgx-submit-ta-request-sequence-object.md`

## 2026-05-13

### SGX submit TA request-packet object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_request_packet`
- The top validator now moves from `sgx_ta_queue_packet` to
  `sgx_ta_request_packet`.
- The live top-layer execution boundary is now unified too:
  - the submit-op ioctl path builds and applies `sgx_ta_request_packet`
  - `issue_submit()` compatibility flow builds and applies `sgx_ta_request_packet`
  - `transition_submit()` compatibility flow builds and applies `sgx_ta_request_packet`
- The new shared top object now owns:
  - the existing `sgx_ta_queue_packet`
  - the public request boundary
  - the selected request-side read/write/CCB/ticket summary
  - the selected next global ticket vector
- The first live draft broke the dependency-gated typed multi-submit path by
  flattening the lower queue-packet builder return code to `-EINVAL`; the
  repair was to preserve the nested `ret`, which restored the intended
  lower-layer `-EAGAIN` result for blocked dependent `running` transitions.
- First-draft evidence:
  - `logs/vaio-deploy/20260513-234413-sgx-ta-request-packet-deploy`
  - `logs/vaio-plan-session/20260513-234558-sgx-plan-session`
  - live multi-submit failure:
    - `submit_op_name=transition`
    - `submit_owner_state_name=running`
    - `submit_op_ret=-1 errno=22`
    - `submit_resolved_id=0`
- Repaired guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-235006-sgx-ta-request-packet-rerun-deploy`
  - final live override:
    - `driver_srcversion=3FA57ECCFB162117FD52520`
    - `override_module_size=480168`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the repaired module also still passed:
  - `logs/vaio-plan-session/20260513-235155-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=3FA57ECCFB162117FD52520`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-235251-sgx-submit-ta-request-packet-rerun-live/summary.log`
  - final focused proof:
    - `submit_resolved_id=8`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-235347-sgx-multi-submit-ta-request-packet-rerun-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_resolved_id=9`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_resolved_id=10`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=17`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-235155`
- Documented in:
  - `docs/203-sgx-submit-ta-request-packet-object.md`

### SGX submit TA queue-packet object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_queue_packet`
- The top validator now moves from `sgx_ta_queue_stream` to
  `sgx_ta_queue_packet`.
- The live top-layer execution boundary is now unified too:
  - the submit-op ioctl path builds and applies `sgx_ta_queue_packet`
  - `issue_submit()` compatibility flow builds and applies `sgx_ta_queue_packet`
  - `transition_submit()` compatibility flow builds and applies `sgx_ta_queue_packet`
- The new shared top object now owns:
  - the existing `sgx_ta_queue_stream`
  - the selected packet-side queue/sync summary
  - the selected queue envelope
  - the selected queue-command/scheduler effect flags
  - the selected next global ticket vector
- The first live draft over-constrained the new packet validator and rejected
  a real focused `issue` submit at `launch -> issue`; the repair was to keep
  the packet object's `global_*_ticket` fields tied to the nested scheduler
  next-state while dropping the cross-check against the selected queue-stream
  payload, which does not own that post-apply ticket vector.
- First-draft evidence:
  - `logs/vaio-deploy/20260513-231923-sgx-ta-queue-packet-deploy`
  - `logs/vaio-plan-session/20260513-232119-sgx-plan-session`
  - `logs/vaio-ioctl/20260513-232311-sgx-submit-ta-queue-packet-live/summary.log`
  - focused failure:
    - `submit issue failed`
    - `submit_op_ret=-1 errno=22`
- Repaired guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-232500-sgx-ta-queue-packet-rerun-deploy`
  - final live override:
    - `driver_srcversion=6CB410CA76314429A21CE15`
    - `override_module_size=478876`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the repaired module also still passed:
  - `logs/vaio-plan-session/20260513-232651-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=6CB410CA76314429A21CE15`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-232750-sgx-submit-ta-queue-packet-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-232833-sgx-multi-submit-ta-queue-packet-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-232651`
- Documented in:
  - `docs/202-sgx-submit-ta-queue-packet-object.md`

### SGX submit TA queue-stream object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_queue_stream`
- The top validator now moves from `sgx_ta_scheduler_entry` to
  `sgx_ta_queue_stream`.
- The live top-layer execution boundary is now unified too:
  - the submit-op ioctl path builds and applies `sgx_ta_queue_stream`
  - `issue_submit()` compatibility flow builds and applies `sgx_ta_queue_stream`
  - `transition_submit()` compatibility flow builds and applies `sgx_ta_queue_stream`
- The new shared top object now owns:
  - the existing `sgx_ta_scheduler_entry`
  - the selected `issue` or `transition` queue stream image
  - the selected `queue header + ccb record + sync + owner` payload
  - the selected queue/sync/owner region-end bounds and chosen CCB slot offset
- The first live draft over-constrained the queue-stream validator and rejected
  a real focused `issue` submit at `launch -> issue`; the repair was to keep
  the full scheduler/global-state proof in the nested `sgx_ta_scheduler_entry`
  validator while narrowing the top queue-stream validator to the fields the
  selected stream image actually owns.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-225919-sgx-ta-queue-stream-rerun-deploy`
  - final live override:
    - `driver_srcversion=D814CEE2FD7F50C9ECE148D`
    - `override_module_size=477908`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-230104-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=D814CEE2FD7F50C9ECE148D`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-230207-sgx-submit-ta-queue-stream-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-230247-sgx-multi-submit-ta-queue-stream-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-230104`
- Documented in:
  - `docs/201-sgx-submit-ta-queue-stream-object.md`

### SGX submit TA scheduler-entry object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_scheduler_entry`
- The top validator now moves from `sgx_ta_queue_command` to
  `sgx_ta_scheduler_entry`.
- The live top-layer execution boundary is now unified too:
  - the submit-op ioctl path builds and applies `sgx_ta_scheduler_entry`
  - `issue_submit()` compatibility flow builds and applies `sgx_ta_scheduler_entry`
  - `transition_submit()` compatibility flow builds and applies `sgx_ta_scheduler_entry`
- The new shared top object now owns:
  - the existing `sgx_ta_queue_command`
  - the full previous driver-global scheduler state
  - the full next driver-global scheduler state
  - the derived scheduler advance flags and submit-id change flags
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-223430-sgx-ta-scheduler-entry-deploy`
  - final live override:
    - `driver_srcversion=4B13FC7FC62A3A9F89B82A8`
    - `override_module_size=475088`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-223620-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=4B13FC7FC62A3A9F89B82A8`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-223718-sgx-submit-ta-scheduler-entry-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-223759-sgx-multi-submit-ta-scheduler-entry-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-223620`
- Documented in:
  - `docs/200-sgx-submit-ta-scheduler-entry-object.md`

### SGX submit TA queue-command object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_queue_command`
- The top validator now moves from `sgx_ta_queue_item` to
  `sgx_ta_queue_command`.
- The live top-layer execution boundary is now unified too:
  - the submit-op ioctl path builds and applies `sgx_ta_queue_command`
  - `issue_submit()` compatibility flow builds and applies `sgx_ta_queue_command`
  - `transition_submit()` compatibility flow builds and applies `sgx_ta_queue_command`
- The new shared top object now owns:
  - the existing `sgx_ta_queue_item`
  - the full pre-apply queue and global-ticket snapshot
  - the full post-apply queue and global-ticket snapshot
  - the derived queue-command advance flags
- The first local draft pulled the driver-global ticket counters from the wrong
  `drm_psb_private` field names; the local rebuild caught that immediately,
  and the fix was to use the real global counters instead.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-222055-sgx-ta-queue-command-deploy`
  - final live override:
    - `driver_srcversion=9D9E9D6EADBAAE4FD768742`
    - `override_module_size=474248`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-222252-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=9D9E9D6EADBAAE4FD768742`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-222359-sgx-submit-ta-queue-command-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-222447-sgx-multi-submit-ta-queue-command-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-222252`
- Documented in:
  - `docs/199-sgx-submit-ta-queue-command-object.md`

### SGX submit TA queue-item object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_queue_item`
- The top validator now moves from `sgx_ta_submit_op_request` to
  `sgx_ta_queue_item`.
- The live top-layer execution boundary is now unified too:
  - the submit-op ioctl path builds and applies `sgx_ta_queue_item`
  - `issue_submit()` compatibility flow builds and applies `sgx_ta_queue_item`
  - `transition_submit()` compatibility flow builds and applies `sgx_ta_queue_item`
- The new shared top object now owns:
  - the existing `sgx_ta_submit_op_request`
  - the queue effect summary:
    - `new_read_offset`
    - `new_write_offset`
    - `new_ccb_offset`
    - `outstanding_packets`
    - `wrapped`
    - `pending_seq`
    - `complete_seq`
    - `issue_cookie`
    - `complete_cookie`
    - `epoch`
  - the global ticket summary:
    - `issue_ticket`
    - `running_ticket`
    - `complete_ticket`
- The first local draft picked the issue-side `ccb_offset` from the wrong
  scheduler snapshot; the local rebuild caught that immediately, and the fix
  was to source that field from the issue image instead.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-220428-sgx-ta-queue-item-deploy`
  - final live override:
    - `driver_srcversion=6D6E13A5AC5A07DA2DE5EEC`
    - `override_module_size=473084`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-220620-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=6D6E13A5AC5A07DA2DE5EEC`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-220721-sgx-submit-ta-queue-item-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-220800-sgx-multi-submit-ta-queue-item-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-220620`
- Documented in:
  - `docs/198-sgx-submit-ta-queue-item-object.md`

### SGX submit TA submit-op request object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_submit_op_request`
- The top validator now moves from `sgx_ta_ioctl_request` to
  `sgx_ta_submit_op_request`.
- The live top-layer execution boundary is now unified too:
  - the submit-op ioctl path builds and applies `sgx_ta_submit_op_request`
  - `issue_submit()` compatibility flow builds and applies `sgx_ta_submit_op_request`
  - `transition_submit()` compatibility flow builds and applies `sgx_ta_submit_op_request`
- The new shared top object now owns:
  - the existing `sgx_ta_ioctl_request`
  - the copied public submit-op packet:
    - `op`
    - `op_kind`
  - the final public resolution seam:
    - `resolved_submit_id`
    - `focus_resolved`
    - `use_focus_request`
    - `dependency_fields_present`
  - the top request split:
    - `issue_request`
    - `transition_request`
    - `active`
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-215042-sgx-ta-submit-op-request-deploy`
  - final live override:
    - `driver_srcversion=32C3455CEA624602FC2F106`
    - `override_module_size=472188`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-215228-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=32C3455CEA624602FC2F106`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-215330-sgx-submit-ta-submit-op-request-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-215411-sgx-multi-submit-ta-submit-op-request-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-215228`
- Documented in:
  - `docs/197-sgx-submit-ta-submit-op-request-object.md`

### SGX submit TA ioctl request object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_ioctl_request`
- The top validator now moves from `sgx_ta_request` to `sgx_ta_ioctl_request`.
- The live top-layer execution boundary is now unified too:
  - the submit-op ioctl path builds and applies `sgx_ta_ioctl_request`
  - `issue_submit()` compatibility flow builds and applies `sgx_ta_ioctl_request`
  - `transition_submit()` compatibility flow builds and applies `sgx_ta_ioctl_request`
- The new shared top object now owns:
  - the existing `sgx_ta_request`
  - the public ioctl seam:
    - `ioctl_op`
    - `requested_submit_id`
    - `resolved_submit_id`
    - `dependency_fields_present`
    - `focus_resolved`
  - the copied top request seam state:
    - `op_kind`
    - `owner_state`
    - `packet_count`
    - `active`
- First live draft booted but failed the exact `complete` proof with:
  - `submit_op_ret=-1`
  - `errno=22`
  - saved in `logs/vaio-plan-session/20260513-212617-sgx-plan-session`
- The regression came from validating too deep at the new top seam, so the fix
  kept `sgx_ta_ioctl_request` validation at the ioctl boundary only instead of
  re-validating the deeper built `sgx_ta_request` contract there.
- Guarded redeploy completed successfully in:
  - `logs/vaio-deploy/20260513-213143-sgx-ta-ioctl-request-rerun-deploy`
  - final live override:
    - `driver_srcversion=CAB474CD8A51DA1D0BE821D`
    - `override_module_size=471808`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the fixed module also still passed:
  - `logs/vaio-plan-session/20260513-213332-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=CAB474CD8A51DA1D0BE821D`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-213452-sgx-submit-ta-ioctl-request-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-213532-sgx-multi-submit-ta-ioctl-request-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-213332`
- Documented in:
  - `docs/196-sgx-submit-ta-ioctl-request-object.md`

### SGX submit TA request object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_request`
- The top validator now moves from `sgx_ta_job` to `sgx_ta_request`.
- The live top-layer execution boundary is now unified too:
  - `issue_submit()` builds and applies `sgx_ta_request`
  - `transition_submit()` builds and applies `sgx_ta_request`
- The new shared top object now owns:
  - the existing `sgx_ta_job`
  - the public request boundary:
    - `submit_id`
    - `op_kind`
    - `owner_state`
    - `packet_count`
  - the top execution-effect summary:
    - `issue_active`
    - `transition_active`
    - `refresh_views`
    - `increment_issue_count`
    - `increment_transition_count`
    - `issue_advanced`
    - `running_advanced`
    - `complete_advanced`
    - `active_running_changed`
    - `next_state`
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-203614-sgx-ta-request-deploy`
  - final live override:
    - `driver_srcversion=02F0B8378744F4DA3D89A13`
    - `override_module_size=470992`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-203802-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=02F0B8378744F4DA3D89A13`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-203935-sgx-submit-ta-request-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-204040-sgx-multi-submit-ta-request-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_dependency_active_count=1`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-203802`
- Documented in:
  - `docs/195-sgx-submit-ta-request-object.md`

### SGX submit TA job object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_job`
- The top validator now moves from `sgx_ta_work_queue` to `sgx_ta_job`.
- The live top-layer execution boundary is now unified too:
  - `issue_submit()` builds and applies `sgx_ta_job`
  - `transition_submit()` builds and applies `sgx_ta_job`
- The new shared top object now owns:
  - the existing `sgx_ta_work_queue`
  - the typed submit contract:
    - `submit_id`
    - `context`
    - `cmd_type`
    - `ctrl_flags`
    - `control_object_id`
    - `pds_object_id`
    - `use_object_id`
    - `target_object_id`
    - `status_object_id`
    - `num_ta_status_vals`
    - `num_3d_status_vals`
    - `ta_dependency`
  - the copied dependency-state array
  - the top dependency summary:
    - `active_dependency_count`
    - `running_slot_busy`
    - `can_enter_running`
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-201803-sgx-ta-job-deploy`
  - final live override:
    - `driver_srcversion=A0E6A4B613B1CF22BFAB9DC`
    - `override_module_size=469344`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-202002-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=A0E6A4B613B1CF22BFAB9DC`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-202558-sgx-submit-ta-job-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-202643-sgx-multi-submit-ta-job-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_dependency_active_count=1`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=11`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-202002`
- Documented in:
  - `docs/194-sgx-submit-ta-job-object.md`

### SGX submit TA work queue object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_work_queue`
- The top validator now moves from `sgx_ta_sequence` to `sgx_ta_work_queue`.
- The live top-layer execution boundary is now unified too:
  - `issue_submit()` builds and applies `sgx_ta_work_queue`
  - `transition_submit()` builds and applies `sgx_ta_work_queue`
- The new shared top object now owns:
  - the existing `sgx_ta_sequence`
  - the typed request boundary:
    - `submit_id`
    - `op_kind`
    - `owner_state`
    - `packet_count`
  - the pre-apply scheduler snapshot
  - the pre-apply issue/transition count base
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-195847-sgx-ta-work-queue-deploy`
  - final live override:
    - `driver_srcversion=30EBD26CDB25CFB331717ED`
    - `override_module_size=467988`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-200217-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=30EBD26CDB25CFB331717ED`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-200432-sgx-submit-ta-work-queue-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
    - final cleanup:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-200459-sgx-multi-submit-ta-work-queue-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_dependency_active_count=1`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-200217`
- Documented in:
  - `docs/193-sgx-submit-ta-work-queue-object.md`

### SGX submit TA sequence object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_sequence`
- The top validator now moves from `sgx_ta_scheduler` to `sgx_ta_sequence`.
- The live top-layer sequence execution path is now unified too:
  - `issue_submit()` builds and applies `sgx_ta_sequence`
  - `transition_submit()` builds and applies `sgx_ta_sequence`
- The new shared top object now owns:
  - the existing `sgx_ta_scheduler`
  - the issue/transition count advance image
  - the timestamp-touch image
  - the refresh-views decision
- This batch also fixed one local regression before deployment:
  - the first draft reintroduced a large stack-frame warning in the sequence
    validator
  - the final version replaced the zero-struct validator path with direct
    field checks, so the module stayed build-clean again before live deploy
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-193942-sgx-ta-sequence-deploy`
  - final live override:
    - `driver_srcversion=37BC439A264460517FFE84D`
    - `override_module_size=466448`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-194137-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=37BC439A264460517FFE84D`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-194256-sgx-submit-ta-sequence-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
    - final cleanup:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-194350-sgx-multi-submit-ta-sequence-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_dependency_active_count=1`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-194137`
- Documented in:
  - `docs/192-sgx-submit-ta-sequence-object.md`

### SGX submit TA scheduler object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_scheduler`
- The top validator now moves from `sgx_ta_op` to `sgx_ta_scheduler`.
- The live top-layer scheduler execution path is now unified too:
  - `issue_submit()` builds and applies `sgx_ta_scheduler`
  - `transition_submit()` builds and applies `sgx_ta_scheduler`
- The new shared top object now owns:
  - the existing `sgx_ta_op`
  - the final driver-global issue/running/complete coordination image
  - the active-running owner image
  - the flags that say which coordination edges advanced
- This batch also removes one remaining split in the live apply path:
  - `sgx_ta_commit` no longer writes global issue/running/complete state
    directly into `dev_priv`
  - `sgx_ta_scheduler` now owns that apply-side scheduler image explicitly
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-192012-sgx-ta-scheduler-deploy`
  - final live override:
    - `driver_srcversion=2F3CD7EC0198A4A88B2798E`
    - `override_module_size=465728`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-192157-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=2F3CD7EC0198A4A88B2798E`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-192257-sgx-submit-ta-scheduler-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
    - final cleanup:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-192342-sgx-multi-submit-ta-scheduler-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_dependency_active_count=1`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-192157`
- Documented in:
  - `docs/191-sgx-submit-ta-scheduler-object.md`

### SGX submit TA op object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_op`
- The top validator now moves from `sgx_ta_work_item` to `sgx_ta_op`.
- The live top-layer public request execution path is now unified too:
  - `issue_submit()` builds and applies `sgx_ta_op`
  - `transition_submit()` builds and applies `sgx_ta_op`
  - submit-op ioctl `ISSUE`/`TRANSITION` request validation now routes through
    one shared `sgx_ta_op` request helper
- The new shared top object now owns:
  - the existing `sgx_ta_work_item`
  - the public request kind
  - the public request owner state
  - the public request packet count
  - the top validator boundary for the live submit-op path
- This batch also tightened one real public-boundary mismatch:
  - ioctl `ISSUE` now rejects `owner_state=complete`
  - ioctl `ISSUE` now rejects `packet_count=0`
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-185946-sgx-ta-op-deploy`
  - final live override:
    - `driver_srcversion=F387C89C0831778420098EB`
    - `override_module_size=463956`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-190135-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=F387C89C0831778420098EB`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-190259-sgx-submit-ta-op-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
    - final cleanup:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-190345-sgx-multi-submit-ta-op-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=4`
    - `SUBMIT_B_ID=5`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_dependency_active_count=1`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=6`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-190135`
- Documented in:
  - `docs/190-sgx-submit-ta-op-object.md`

### SGX submit TA work item object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_work_item`
- The top validator now moves from `sgx_ta_event` to `sgx_ta_work_item`.
- The live top-layer request execution path is now unified too:
  - `issue_submit()` builds and applies `sgx_ta_work_item`
  - `transition_submit()` builds and applies `sgx_ta_work_item`
- The new shared top object now owns:
  - the existing `sgx_ta_event`
  - the issue-vs-transition request kind
  - the request packet count
  - the pre-apply validate rule
  - the stale-build disarm rule
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-183718-sgx-ta-work-item-deploy`
  - final live override:
    - `driver_srcversion=2077F2C05141C6DFD29769B`
    - `override_module_size=464604`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-183909-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=2077F2C05141C6DFD29769B`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-184011-sgx-submit-ta-work-item-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
    - final cleanup:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-184054-sgx-multi-submit-ta-work-item-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_dependency_active_count=1`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-183909`
- Documented in:
  - `docs/189-sgx-submit-ta-work-item-object.md`

### SGX submit TA event object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_event`
- The top validator now moves from `sgx_ta_commit` to `sgx_ta_event`.
- The live top-layer scheduler event path is now unified too:
  - `issue_submit()` builds and applies `sgx_ta_event`
  - `transition_submit()` builds and applies `sgx_ta_event`
- The new shared top object now owns:
  - the existing `sgx_ta_commit`
  - the event-side count increment plan
  - the event-side timestamp update plan
  - the post-apply refresh plan
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-181949-sgx-ta-event-deploy`
  - final live override:
    - `driver_srcversion=42F7BCB76CD9DEFD437A03A`
    - `override_module_size=464220`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-182148-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=42F7BCB76CD9DEFD437A03A`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-182251-sgx-submit-ta-event-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
    - final cleanup:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-182331-sgx-multi-submit-ta-event-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_dependency_active_count=1`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-182148`
- Documented in:
  - `docs/188-sgx-submit-ta-event-object.md`

### SGX submit TA commit object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_commit`
- Added one new typed next-state image:
  - `psb_sgx_lab_submit_sgx_ta_commit_state`
- The top validator now moves from `sgx_ta_path` to `sgx_ta_commit`.
- The live top-layer apply path is now unified too:
  - `issue_submit()` builds and applies `sgx_ta_commit`
  - `transition_submit()` builds and applies `sgx_ta_commit`
- The new shared top object now owns:
  - the existing `sgx_ta_path`
  - the queue-header / CCB / sync / owner write plan
  - the typed next-submit-state image for issue
  - the typed next-submit-state image for transition
  - the global issue / running / complete apply plan
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-172835-sgx-ta-commit-deploy`
  - final live override:
    - `driver_srcversion=008618C5501CD0CD7A0CB2B`
    - `override_module_size=464244`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-173037-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=008618C5501CD0CD7A0CB2B`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-173156-sgx-submit-ta-commit-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
    - final cleanup:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-173156-sgx-multi-submit-ta-commit-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_dependency_active_count=1`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-173037`
- Documented in:
  - `docs/187-sgx-submit-ta-commit-object.md`

### SGX submit TA path issue unification

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- No new top object was added in this batch.
- The live issue path is now unified with the existing typed top object:
  - `psb_sgx_lab_submit_sgx_ta_path`
- The new shared top-layer issue helper now routes `issue_submit()` through:
  - `poulsbo_sgx_submit_sgx_ta_path_issue_status_locked()`
- The live-path split is now closed:
  - top validator uses `sgx_ta_path`
  - `transition_submit()` uses `sgx_ta_path`
  - `issue_submit()` now also uses `sgx_ta_path`
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-170524-guarded-deploy`
  - final live override:
    - `driver_srcversion=5B50AE965337A8FBA71E50E`
    - `override_module_size=462900`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-170719-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=5B50AE965337A8FBA71E50E`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-170719-sgx-submit-ta-path-issue-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
    - final cleanup:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-170719-sgx-multi-submit-ta-path-issue-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_dependency_active_count=1`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=4`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-170719`
- Documented in:
  - `docs/186-sgx-submit-ta-path-issue-unification.md`

### SGX submit TA path object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped top object:
  - `psb_sgx_lab_submit_sgx_ta_path`
- The new `sgx_ta_path` object now keeps together:
  - the live `sgx_ta_dispatch` image
  - the typed transition queue-header image
  - the typed transition CCB image
  - the typed transition sync image
  - the typed transition owner image
- The top validator now moves from `sgx_ta_dispatch` to `sgx_ta_path`, and the
  live `transition_submit()` path now consumes the same typed transition image
  instead of rebuilding queue/CCB/sync/owner writeback inline.
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-164652-guarded-deploy`
  - final live override:
    - `driver_srcversion=A84CAC0D63EBA733BE134C6`
    - `override_module_size=462848`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-164848-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=A84CAC0D63EBA733BE134C6`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-165147-sgx-submit-ta-path-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
    - final cleanup:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-165002-sgx-multi-submit-ta-path-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_dependency_active_count=1`
      - `dependency[1]_required_ticket=3`
      - `dependency[1]_observed_complete_ticket=5`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-164848`
- Documented in:
  - `docs/185-sgx-submit-ta-path-object.md`

### SGX submit TA execution object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped execution object:
  - `psb_sgx_lab_submit_sgx_ta_execution`
- The new `sgx_ta_execution` object now keeps together:
  - the live `ccb_kick`
  - the live `sgx_ta_submission`
  - the optional launch-time `ccb_kick`
  - the optional launch-time `sgx_ta_submission`
- The first live draft regressed the old `launch -> issue` boundary again:
  - guarded deploy:
    - `logs/vaio-deploy/20260513-111124-guarded-deploy`
    - live override `srcversion 9574D599C4FD8B682D93F32`
  - exact proof:
    - `logs/vaio-plan-session/20260513-111340-sgx-plan-session`
    - failed at `submit_op_name=issue`
    - `submit_op_ret=-1 errno=22`
- The root cause was launch-side overvalidation:
  - the new wrapper was running the full live-submission coherence helper on
    the launch snapshot
  - that future snapshot intentionally carries launch-time `issue-plan` state
    and cannot be treated like the current live submission image
- The fix kept the higher-level object but narrowed launch-side validation to
  launch-stable fields only:
  - queued TA command wiring
  - queue-header placement
  - copied future `issue-plan` contents
- Final guarded deploy after that fix:
  - `logs/vaio-deploy/20260513-120442-guarded-deploy`
  - final live override:
    - `driver_srcversion=311989DA7E14F05C171ED79`
    - `override_module_size=454160`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the repaired module passed again:
  - `logs/vaio-plan-session/20260513-120641-sgx-plan-session`
  - `selected_phase_profile=complete`
  - `selected_outcome=completed`
  - `final_driver_srcversion=311989DA7E14F05C171ED79`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-120825-sgx-submit-ta-execution-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-120927-sgx-multi-submit-ta-execution-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_dependency_active_count=1`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-120641`
- Documented in:
  - `docs/178-sgx-submit-ta-execution-object.md`

### SGX submit TA submission object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new kernel-internal SGX-shaped execution object:
  - `psb_sgx_lab_submit_sgx_ta_submission`
- The new `sgx_ta_submission` object now wraps:
  - `sgx_queued_ta_command`
  - typed `live`
  - typed `manifest`
  - typed `descriptor`
  - typed `plan`
  - optional typed `issue-plan`
- The control-side image validators can now validate against already-built
  typed images from that submission object, so one checked SGX layer now spans
  the queued TA command and the control-backed memory views that describe the
  same submission.
- The first live validator draft was too strict in two places:
  - it treated `issue-plan` as if it always mirrored current live queue state
  - it treated `plan` as if it always mirrored current live queue state
- Live debugging proved both assumptions wrong:
  - `issue` validation runs on the `launched` state before the issue mutation
  - `plan` is source-switched by stage for `launch`, `dispatch`, and `retired`
- Hardware validation therefore advanced in three steps:
  - first guarded deploy:
    - `logs/vaio-deploy/20260513-075745-guarded-deploy`
    - live override `srcversion 0AD6D850315762A2E4F6E45`
    - exact proof failed at `issue` in
      `logs/vaio-plan-session/20260513-080115-sgx-plan-session`
  - second guarded deploy after relaxing the `issue-plan` cross-check:
    - `logs/vaio-deploy/20260513-080356-guarded-deploy`
    - live override `srcversion 9172ED422A56E8DA614C79D`
    - exact proof still failed at `issue` in
      `logs/vaio-plan-session/20260513-080540-sgx-plan-session`
  - final guarded deploy after relaxing the stage-switched `plan` cross-check:
    - `logs/vaio-deploy/20260513-080856-guarded-deploy`
    - final live override:
      - `driver_srcversion=329853678DD9A992924BB5C`
      - `override_module_size=453508`
      - `overall=remote_usable`
      - `lightdm_active=inactive`
      - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the final module passed again:
  - `logs/vaio-plan-session/20260513-081039-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=329853678DD9A992924BB5C`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-081614-sgx-submit-ta-submission-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
    - final cleanup:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-081700-sgx-multi-submit-ta-submission-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=6`
    - `SUBMIT_B_ID=7`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_dependency_active_count=1`
    - final cleanup returned:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-081039`
- Documented in:
  - `docs/177-sgx-submit-ta-submission-object.md`

### SGX submit TA command object

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped execution object:
  - `psb_sgx_lab_submit_sgx_ta_command`
- The new `sgx_ta_command` object now wraps one `sgx_ta_kick` and one typed
  CCB record image, so the validator consumes one full queued TA command
  object instead of checking the TA-kick command words and the CCB payload as
  separate layers.
- The new object keeps these bindings coherent as one validated snapshot:
  - `service_address`
  - `cache_control`
  - `data0`
  - `data1`
  - `pds_gpu_va32`
  - `use_gpu_va32`
  - `target_gpu_va32`
  - `status_gpu_va32`
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-061050-guarded-deploy`
  - final live override:
    - `driver_srcversion=61850CF09DE3C6531E06127`
    - `override_module_size=451152`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-061236-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=61850CF09DE3C6531E06127`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-061337-sgx-submit-ta-command-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_live_stage_name=retired`
    - `submit_result_stage_name=retired`
    - `submit_manifest_stage_name=retired`
    - `submit_descriptor_stage_name=retired`
    - `submit_plan_stage_name=retired`
    - `submit_issue_plan_stage_name=retired`
    - `submit_issue_plan_dependency_active_count=0`
    - final cleanup:
      - `submit_dependency_active_count=0`
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit validation also still passed:
  - `logs/vaio-ioctl/20260513-061416-sgx-multi-submit-ta-command-live/summary.log`
  - submit A final:
    - `SUBMIT_A_ID=3`
    - `submit_prepare_count=1`
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `SUBMIT_B_ID=4`
    - `submit_prepare_count=1`
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_result_dependency_active_count=1`
    - `submit_manifest_dependency_active_count=1`
    - `submit_descriptor_dependency_active_count=1`
    - `submit_plan_dependency_active_count=1`
    - `submit_dependency_active_count=1`
- The shared current-boundary resolver remains terminal on the new line:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-061236`
- Documented in:
  - `docs/175-sgx-submit-ta-command-object.md`

### Shared SGX submit `cmdta_shared` object and validator-frame cleanup

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added one new explicit kernel-internal SGX-shaped execution layer above the
  existing `sgx_ccb_kick` model:
  - `struct psb_sgx_lab_submit_sgx_cmdta_shared`
- The shared internal flow is now:
  - `psb_sgx_lab_submit_exec_record`
  - `-> psb_sgx_lab_submit_exec_packet`
  - `-> psb_sgx_lab_submit_cmdta_kick`
  - `-> psb_sgx_lab_submit_ccb_kick`
  - `-> psb_sgx_lab_submit_sgx_ccb_kick`
  - `-> psb_sgx_lab_submit_sgx_cmdta_shared`
- The new `sgx_cmdta_shared` object now carries:
  - kick pointer back to the normalized `ccb_kick`
  - control flags
  - dst sync-object count
  - TA and 3D status-value counts
  - first-kick-or-resume state
  - CCB packet offset
  - TA dependency presence
  - TA and 3D TQ sync GPU-VA32 words
  - TA source and destination sync counts
  - 3D source-sync count
  - dependency submit id and ticket snapshot
  - shared pointers to the normalized TA and 3D status-update arrays
- The status-side validator now consumes one explicit live
  `sgx_cmdta_shared` object instead of carrying both a live `sgx_ccb_kick`
  object and raw status-update loops.
- The builder now rebuilds the SGX kick side internally, so the validator no
  longer keeps both SGX layers live on one frame.
- Local validation passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
  - `./scripts/docker-build-vaio-gma500.sh`
  - the earlier `-Wframe-larger-than=1024` warning in
    `poulsbo_sgx_validate_submit_sgx_cmdta_shared_locked()` no longer
    appeared in the local build
- Guarded deploy bundle:
  - [logs/vaio-deploy/20260513-045436-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-045436-guarded-deploy)
- That deploy rebooted the VAIO onto:
  - `driver_srcversion=040B3A07BC156A1AAEE99D6`
  - `override_module_size=449672`
  - `lightdm_active=inactive`
  - `kdump_service_active=active`
- Exact terminal rerun:
  - [logs/vaio-plan-session/20260513-045622-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-045622-sgx-plan-session)
  - `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete`
  - result:
    - `session_outcome=completed`
    - `selected_phase_profile=complete`
    - `final_driver_srcversion=040B3A07BC156A1AAEE99D6`
- Focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-045728-sgx-submit-cmdta-shared-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-045728-sgx-submit-cmdta-shared-live/summary.log)
- Final focused typed state:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - `submit_live_stage_name=retired`
  - `submit_result_stage_name=retired`
  - `submit_manifest_stage_name=retired`
  - `submit_descriptor_stage_name=retired`
  - `submit_plan_stage_name=retired`
  - `submit_issue_plan_stage_name=retired`
  - `submit_issue_plan_stage_count=1`
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-045811-sgx-multi-submit-cmdta-shared-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-045811-sgx-multi-submit-cmdta-shared-live/summary.log)
- Final typed multi-submit state:
  - submit B retained dependency state cleanly through the new shared layer:
    - `submit_result_dependency_active_count=1`
    - `submit_manifest_dependency_active_count=1`
    - `submit_descriptor_dependency_active_count=1`
    - `submit_plan_dependency_active_count=1`
    - `submit_dependency_active_count=1`
    - `dependency[1]_submit_id=3`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
    - `dependency[1]_flags=0xf`
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- The shared current-boundary resolver stays terminal on the new module:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-045622`
  - `source_row_module_srcversion=040B3A07BC156A1AAEE99D6`

Conclusion:

- the shared SGX-shaped submit model now has one more explicit execution-side
  layer above the `sgx_ccb_kick` image
- the status-side validator now consumes that layer directly, and the local
  stack-frame regression is gone again
- the next useful work is the first real SGX execution object above
  `sgx_cmdta_shared`, not another validation-only SGX wrapper

### Shared SGX submit `sgx_ccb_kick` object and dependency-source fix

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
  - `scripts/run-vaio-sgx-multi-submit.sh`
- Added one new explicit kernel-internal SGX-shaped execution layer above the
  existing `ccb_kick` model:
  - `struct psb_sgx_lab_submit_sgx_ccb_kick`
- The shared internal flow is now:
  - `psb_sgx_lab_submit_exec_record`
  - `-> psb_sgx_lab_submit_exec_packet`
  - `-> psb_sgx_lab_submit_cmdta_kick`
  - `-> psb_sgx_lab_submit_ccb_kick`
  - `-> psb_sgx_lab_submit_sgx_ccb_kick`
- The new `sgx_ccb_kick` object now carries:
  - dst sync-object count
  - TA and 3D status-value counts
  - first-kick-or-resume state
  - CCB packet offset
  - TA source and destination sync counts
  - 3D source sync count
  - dependency submit id and ticket snapshot
  - TA and 3D sync GPU-VA32 words
- The first typed multi-submit rerun exposed one real kernel-side regression:
  - the dependent submit failed at `prepare`
  - the failing payload came back as:
    - `submit_op_name=prepare`
    - `submit_op_ret=-1 errno=22`
- Root cause:
  - the first draft materialized the 3D source-sync side of the new
    `sgx_ccb_kick` object before the dependency snapshot source submit id and
    required ticket had been populated by `depend-slot` and `depstate`
- The fix:
  - `poulsbo_sgx_build_submit_sgx_ccb_kick_locked()` now only materializes the
    3D source-sync side when both:
    - `dependency_snapshot_source_submit_id`
    - `dependency_snapshot_required_ticket`
    are present
  - `scripts/run-vaio-sgx-multi-submit.sh` now also prints the captured submit
    payload on failure instead of hiding it behind a silent wrapper error
- Local validation passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
  - `./scripts/docker-build-vaio-gma500.sh`
  - `bash -n scripts/run-vaio-sgx-multi-submit.sh`
- Guarded deploy bundle:
  - [logs/vaio-deploy/20260513-042457-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-042457-guarded-deploy)
- That deploy rebooted the VAIO onto:
  - `driver_srcversion=529917AB9D340930B19FF28`
  - `override_module_size=449016`
  - `lightdm_active=inactive`
  - `kdump_service_active=active`
- Exact terminal rerun:
  - [logs/vaio-plan-session/20260513-042638-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-042638-sgx-plan-session)
  - `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete`
  - result:
    - `session_outcome=completed`
    - `selected_phase_profile=complete`
    - `final_driver_srcversion=529917AB9D340930B19FF28`
- Focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-042800-sgx-submit-sgx-ccb-kick-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-042800-sgx-submit-sgx-ccb-kick-live/summary.log)
- Final focused typed state:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - `submit_live_stage_name=retired`
  - `submit_result_stage_name=retired`
  - `submit_manifest_stage_name=retired`
  - `submit_descriptor_stage_name=retired`
  - `submit_plan_stage_name=retired`
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-042841-sgx-multi-submit-sgx-ccb-kick-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-042841-sgx-multi-submit-sgx-ccb-kick-live/summary.log)
- Final typed multi-submit state:
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_live_stage_name=retired`
    - `submit_result_stage_name=retired`
    - `submit_manifest_stage_name=retired`
    - `submit_descriptor_stage_name=retired`
    - `submit_plan_stage_name=retired`
  - submit B:
    - `submit_prepare_count=1`
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_live_stage_name=retired`
    - `submit_result_stage_name=retired`
    - `submit_manifest_stage_name=retired`
    - `submit_descriptor_stage_name=retired`
    - `submit_plan_stage_name=retired`
    - `submit_live_dependency_active_count=1`
    - `submit_result_dependency_active_count=1`
    - `submit_manifest_dependency_active_count=1`
    - `submit_descriptor_dependency_active_count=1`
    - `submit_plan_dependency_active_count=1`
    - `dependency[1]_submit_id=3`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
    - `dependency[1]_flags=0xf`
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- The shared current-boundary resolver stays terminal on the new module:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-042638`
  - `source_row_module_srcversion=529917AB9D340930B19FF28`

Conclusion:

- the shared SGX-shaped submit model now has one more explicit execution-side
  layer above the raw `ccb_kick` image
- the new layer immediately paid for itself by catching a real dependency
  materialization bug in the typed multi-submit path
- the next useful work is the first real SGX execution object above
  `sgx_ccb_kick`, not another read-side-only expansion

### Shared SGX submit exec-packet structure and full validation

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
  - `scripts/run-vaio-sgx-multi-submit.sh`
- Added one new explicit kernel-internal SGX-shaped execution layer above the
  shared normalized record:
  - `struct psb_sgx_lab_submit_exec_packet`
  - `struct psb_sgx_lab_submit_ccb_command`
  - `struct psb_sgx_lab_submit_ta_shared`
- The new internal flow is now:
  - `psb_sgx_lab_submit_exec_record`
  - `-> psb_sgx_lab_submit_exec_packet`
- That packet now feeds:
  - `queue header`
  - `ccb record`
  - `sync`
  - `owner`
  - `report`
  - `live`
  - `result`
  - `manifest`
  - `descriptor`
  - `plan`
  - `issue-plan`
- The first typed multi-submit rerun exposed two runner-side expectation bugs
  only:
  - `issue-plan dependency_active_count` stays `0` before `launch`
  - on the dependent submit, it becomes `1` at `launch`
  - it is back to `0` once the launched packet is consumed and later retired
- Local validation passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
  - `./scripts/docker-build-vaio-gma500.sh`
  - `bash -n scripts/run-vaio-sgx-multi-submit.sh`
- Guarded deploy bundle:
  - [logs/vaio-deploy/20260513-030640-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-030640-guarded-deploy)
- That deploy rebooted the VAIO onto:
  - `driver_srcversion=A154282637C389A3032C8D9`
  - `override_module_size=447160`
  - `lightdm_active=inactive`
  - `kdump_service_active=active`
- Exact terminal rerun:
  - [logs/vaio-plan-session/20260513-030828-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-030828-sgx-plan-session)
  - `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete`
  - result:
    - `session_outcome=completed`
    - `selected_phase_profile=complete`
    - `final_driver_srcversion=A154282637C389A3032C8D9`
- Focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-030828-sgx-submit-exec-packet-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-030828-sgx-submit-exec-packet-live/summary.log)
- Final focused typed state:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - `submit_live_stage_name=retired`
  - `submit_result_stage_name=retired`
  - `submit_issue_plan_stage_name=retired`
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-031429-sgx-multi-submit-exec-packet-live-typed-rerun2/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-031429-sgx-multi-submit-exec-packet-live-typed-rerun2/summary.log)
- Final typed multi-submit state:
  - `SUBMIT_A_ID=7`
  - `SUBMIT_B_ID=8`
  - submit A:
    - `submit_live_stage_name=retired`
    - `submit_live_stage_count=3`
    - `submit_result_stage_name=retired`
    - `submit_result_stage_count=3`
    - `submit_issue_plan_stage_name=retired`
    - `submit_issue_plan_stage_count=3`
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B:
    - `submit_live_stage_name=retired`
    - `submit_live_stage_count=2`
    - `submit_result_stage_name=retired`
    - `submit_result_stage_count=2`
    - `submit_issue_plan_stage_name=retired`
    - `submit_issue_plan_stage_count=2`
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_live_dependency_active_count=1`
    - `submit_result_dependency_active_count=1`
    - `submit_plan_dependency_active_count=1`
    - `submit_issue_plan_dependency_active_count=0`
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- The shared current-boundary resolver stays terminal on the new module:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-030828`
  - `source_row_module_srcversion=A154282637C389A3032C8D9`

Conclusion:

- the SGX submit model now has one more execution-oriented kernel-internal
  layer above the normalized record
- the fixed queue/CCB/sync/owner blocks and the typed submit views now share
  one SGX-shaped packet image
- the next useful work is to keep pushing that packet model toward the first
  truly execution-oriented SGX kernel object, not to add more read-side view
  plumbing

### Typed SGX submit `live` block and live validation

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
  - `workloads/psb_sgx_lab_query.c`
  - `workloads/psb_sgx_lab_submit_op.c`
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
  - `scripts/run-vaio-sgx-multi-submit.sh`
- Added a first-class typed control-backed submit `live` block:
  - new submit op:
    - `DRM_PSB_SGX_LAB_SUBMIT_OP_LIVE`
  - new query-side `submit_live_*` fields
  - new serialized image:
    - `struct psb_sgx_lab_live_words`
- Wired `live` through:
  - control-layout overlap rejection
  - object access validation
  - refresh preflight
  - backing-image validation
  - typed query readback
  - focused submit-op validation
  - typed multi-submit validation
- Local validation passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
  - `git diff --check -- workloads scripts`
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded deploy bundle:
  - [logs/vaio-deploy/20260513-015019-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-015019-guarded-deploy)
- That deploy rebooted the VAIO onto:
  - `driver_srcversion=864D7160A07B237AB1A9D36`
  - `override_module_size=442596`
  - `lightdm_active=inactive`
  - `kdump_service_active=active`
- Exact terminal rerun:
  - [logs/vaio-plan-session/20260513-015211-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-015211-sgx-plan-session)
  - `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete`
  - result:
    - `session_outcome=completed`
    - `selected_phase_profile=complete`
    - `final_driver_srcversion=864D7160A07B237AB1A9D36`
- Focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-015552-sgx-submit-live-op-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-015552-sgx-submit-live-op-live/summary.log)
- Final focused typed `live` state:
  - `SUBMIT_ID=5`
  - `submit_live_stage_name=retired`
  - `submit_live_stage_count=1`
  - `submit_live_prepare_count=1`
  - `submit_live_arm_count=1`
  - `submit_live_emit_count=1`
  - `submit_live_launch_count=1`
  - `submit_live_dispatch_count=1`
  - `submit_live_retired_count=1`
  - `submit_live_owner_state_name=complete`
  - `submit_live_dependency_active_count=0`
  - `submit_live_queue_packet_count=0`
  - `submit_live_queue_epoch=0`
  - `submit_live_control_gpu_va32=0x807bf000`
  - `submit_live_status_gpu_va32=0x807cf000`
  - `submit_live_pds_gpu_va32=0x807c3000`
  - `submit_live_use_gpu_va32=0x807c7000`
  - `submit_live_target_gpu_va32=0x807cb000`
  - `submit_live_ccb_status_gpu_va32=0x807cf000`
  - `submit_live_config_count=1`
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-015629-sgx-multi-submit-live-op-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-015629-sgx-multi-submit-live-op-live/summary.log)
- Final typed multi-submit state:
  - `SUBMIT_A_ID=6`
  - `SUBMIT_B_ID=7`
  - submit A:
    - `submit_live_stage_name=retired`
    - `submit_live_stage_count=3`
    - `submit_live_prepare_count=1`
    - `submit_live_arm_count=3`
    - `submit_live_emit_count=3`
    - `submit_live_launch_count=3`
    - `submit_live_dispatch_count=3`
    - `submit_live_retired_count=3`
    - `submit_live_owner_state_name=complete`
    - `submit_live_dependency_active_count=0`
    - `submit_live_queue_packet_count=0`
    - `submit_live_queue_epoch=2`
    - `submit_live_control_gpu_va32=0x807bf000`
    - `submit_live_status_gpu_va32=0x807cf000`
    - `submit_live_pds_gpu_va32=0x807c3000`
    - `submit_live_use_gpu_va32=0x807c7000`
    - `submit_live_target_gpu_va32=0x807cb000`
    - `submit_live_ccb_status_gpu_va32=0x807cf000`
    - `submit_live_config_count=1`
  - submit B:
    - `submit_live_stage_name=retired`
    - `submit_live_stage_count=2`
    - `submit_live_prepare_count=1`
    - `submit_live_arm_count=2`
    - `submit_live_emit_count=2`
    - `submit_live_launch_count=2`
    - `submit_live_dispatch_count=2`
    - `submit_live_retired_count=2`
    - `submit_live_owner_state_name=complete`
    - `submit_live_dependency_active_count=1`
    - `submit_live_queue_packet_count=0`
    - `submit_live_queue_epoch=1`
    - `submit_live_control_gpu_va32=0x807d3000`
    - `submit_live_status_gpu_va32=0x807e3000`
    - `submit_live_pds_gpu_va32=0x807d7000`
    - `submit_live_use_gpu_va32=0x807db000`
    - `submit_live_target_gpu_va32=0x807df000`
    - `submit_live_ccb_status_gpu_va32=0x807e3000`
    - `submit_live_config_count=1`
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- The shared current-boundary resolver stays terminal on the new module:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-015211`
  - `source_row_module_srcversion=864D7160A07B237AB1A9D36`

Conclusion:

- the typed SGX lab submit image now has an explicit control-backed `live`
  execution block on top of the shared execution-facing kernel view model
- the new block is clean on exact `complete`, focused typed submit, and typed
  multi-submit validation
- the next useful work is above this validated view set: the first more
  execution-oriented SGX kernel structure, not more read-side block plumbing

### Shared SGX live-view exec-record unification and pre-queue CCB fix

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Extended the shared kernel-internal execution-shaped submit record so it now
  also drives the older fixed live submit blocks:
  - `queue header`
  - `ccb record`
  - `sync`
  - `owner`
  - `report`
- The first guarded candidate exposed a real regression in the legal pre-queue
  `ccb` path:
  - regression bundle:
    [logs/vaio-plan-session/20260513-004821-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-004821-sgx-plan-session)
  - failing live override:
    - `driver_srcversion=A91AC3D851E88D083C04C9F`
  - failing exact phase:
    - `selected_failed_phase=complete`
    - `selected_failed_step=submit_op_ccb_focus`
    - `selected_failed_rc=1`
- Cause:
  - the queue-header builder started assuming queue geometry already existed
    and tried to compute queue progression even before the later `queue`
    configure step
- Fix:
  - `queue header` and `sync` now keep a valid pre-queue fallback and skip
    queue progression until queue geometry exists
- Local validation passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
  - `./scripts/docker-build-vaio-gma500.sh`
- Final direct guarded deploy bundle:
  - [logs/vaio-deploy/20260513-005139-guarded-deploy-direct](/root/Poulsbo/logs/vaio-deploy/20260513-005139-guarded-deploy-direct)
- That deploy rebooted the VAIO onto:
  - `driver_srcversion=21D34B366D429B395AD45DC`
  - `override_module_size=434960`
  - `lightdm_active=inactive`
  - `kdump_service_active=active`
- Exact terminal rerun:
  - [logs/vaio-plan-session/20260513-005336-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-005336-sgx-plan-session)
  - `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete`
  - result:
    - `session_outcome=completed`
    - `selected_phase_profile=complete`
    - `final_driver_srcversion=21D34B366D429B395AD45DC`
- Focused typed submit bundle:
  - [logs/vaio-ioctl/20260513-005719-sgx-submit-exec-record-full-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-005719-sgx-submit-exec-record-full-live/summary.log)
- Final focused typed state:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - `submit_result_stage_name=retired`
  - `submit_result_source_name=retired`
  - `submit_result_owner_state_name=complete`
- Typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-005759-sgx-multi-submit-exec-record-full-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-005759-sgx-multi-submit-exec-record-full-live/summary.log)
- Final typed multi-submit state:
  - `SUBMIT_A_ID=6`
  - `SUBMIT_B_ID=7`
  - submit A:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_result_stage_name=retired`
    - `submit_result_source_name=retired`
    - `submit_result_owner_state_name=complete`
  - submit B:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_result_stage_name=retired`
    - `submit_result_source_name=retired`
    - `submit_result_owner_state_name=complete`
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- The shared current-boundary resolver stays terminal on the new module:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-005336`
  - `source_row_module_srcversion=21D34B366D429B395AD45DC`

Conclusion:

- the full typed SGX lab submit image now shares one execution-facing kernel
  record
- the pre-queue `ccb` path still mattered and is now preserved explicitly
- the final direct-deploy candidate is clean on exact `complete`, focused
  submit, and typed multi-submit validation

### Shared SGX submit exec-record refactor and live validation

- Updated:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added the first shared kernel-internal execution-shaped submit record:
  - `enum psb_sgx_lab_exec_source`
  - `struct psb_sgx_lab_submit_exec_record`
  - `poulsbo_sgx_build_submit_exec_record_locked()`
- Rewired the typed memory-backed submit-view builders to consume that shared
  record:
  - `manifest`
  - `descriptor`
  - `plan`
  - `result`
- Local validation passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded deploy bundle:
  - [logs/vaio-deploy/20260513-002930-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260513-002930-guarded-deploy)
- That deploy rebooted the VAIO onto:
  - `driver_srcversion=59347D903B1ABC5E00105D3`
  - `override_module_size=434080`
  - `lightdm_active=inactive`
  - `kdump_service_active=active`
- Exact `complete` regression bundle:
  - [logs/vaio-plan-session/20260513-003119-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260513-003119-sgx-plan-session)
- That exact rerun stayed clean:
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=59347D903B1ABC5E00105D3`
- Focused typed submit-op bundle:
  - [logs/vaio-ioctl/20260513-003228-sgx-submit-exec-record-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-003228-sgx-submit-exec-record-live/summary.log)
- Final focused typed state:
  - `submit_prepare_count=1`
  - `submit_arm_count=1`
  - `submit_emit_count=1`
  - `submit_launch_count=1`
  - `submit_dispatch_count=1`
  - `submit_retired_count=1`
  - `submit_issue_count=1`
  - `submit_transition_count=2`
  - `submit_result_stage_name=retired`
  - `submit_result_source_name=retired`
  - `submit_result_owner_state_name=complete`
- Typed multi-submit bundle:
  - [logs/vaio-ioctl/20260513-003228-sgx-multi-submit-exec-record-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260513-003228-sgx-multi-submit-exec-record-live/summary.log)
- Final typed multi-submit state:
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_result_stage_name=retired`
    - `submit_result_source_name=retired`
    - `submit_result_owner_state_name=complete`
  - submit B:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_result_stage_name=retired`
    - `submit_result_source_name=retired`
    - `submit_result_owner_state_name=complete`
  - final cleanup:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- The shared current-boundary resolver now also advances the terminal proof to
  the new module:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-003119`
  - `source_row_module_srcversion=59347D903B1ABC5E00105D3`

Conclusion:

- the SGX lab path now has a real kernel-internal execution record shared by
  the typed submit-view builders
- the new structure is live-validated on the VAIO
- the next useful work is above this layer: move that execution record toward a
  more real SGX execution model instead of adding more parallel per-view state

## 2026-05-12

### Fixed-module exact `plan` reruns through terminal `complete`

- Updated:
  - `scripts/lib-vaio-plan-summary.sh`
- Continued exact no-redeploy reruns on fixed override
  `srcversion 6E95EDFFFB5384C8E58F25B`:
  - `emit`
    - [logs/vaio-plan-session/20260512-225457-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-225457-sgx-plan-session)
  - `validate`
    - [logs/vaio-plan-session/20260512-225600-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-225600-sgx-plan-session)
  - `launch`
    - [logs/vaio-plan-session/20260512-225704-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-225704-sgx-plan-session)
  - `issue`
    - [logs/vaio-plan-session/20260512-225809-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-225809-sgx-plan-session)
  - `running`
    - [logs/vaio-plan-session/20260512-225915-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-225915-sgx-plan-session)
  - `complete`
    - [logs/vaio-plan-session/20260512-230021-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-230021-sgx-plan-session)
- Every exact rerun stayed:
  - `session_outcome=completed`
  - `last_overall=remote_usable`
  - `final_driver_srcversion=6E95EDFFFB5384C8E58F25B`
- The exact fixed-module line now clears the entire single-submit ladder:
  - `plan_query_offset`
  - `plan_query_config`
  - `plan_query_snapshot`
  - `prepare_op`
  - `prepare_query`
  - `arm`
  - `emit`
  - `validate`
  - `launch`
  - `issue`
  - `running`
  - `complete`
- That exposed one offline logic bug: after a clean terminal `complete`, the
  shared current-boundary resolver still fell back to `post_descriptor`
  because there was no successor phase.
- Fixed in `scripts/lib-vaio-plan-summary.sh` by treating “no successor after
  a clean last-good phase” as terminal evidence instead of a fallback case.
- Current saved boundary after the fix:
  - `resolved_profile=complete`
  - `resolved_phase=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260512-230021`
- `scripts/report-vaio-plan-status.sh` now reports the same terminal state.

Conclusion:

- the fixed module now survives the full exact single-submit `plan` ladder
- the current `plan` line is no longer blocked inside the exact phase sequence
- the next useful work is beyond exact single-submit phase isolation, not more
  ladder narrowing
### VAIO kdump panic capture, auto-reboot, and end-to-end validation

- Added:
  - `scripts/configure-vaio-panic-capture.sh`
  - `scripts/fetch-vaio-panic-artifacts.sh`
  - `scripts/run-vaio-panic-capture-setup-session.sh`
  - `scripts/run-vaio-panic-test-session.sh`
- Extended `scripts/show-vaio-gma500-state.sh` with:
  - `crashkernel_cmdline_value`
  - `kernel_panic`
  - `kernel_panic_on_oops`
  - `kernel_sysrq`
  - `kdump_*`
  - `panic_latest_*`
- Live setup bundle:
  - [logs/vaio-panic/20260512-221904-panic-capture-setup](/root/Poulsbo/logs/vaio-panic/20260512-221904-panic-capture-setup)
- The setup session:
  - installed `kdump-tools`, `kexec-tools`, and `makedumpfile`
  - persisted `kernel.panic=10`, `kernel.panic_on_oops=1`, and `kernel.sysrq=1`
  - booted with `crashkernel=128M panic=10 panic_on_oops=1`
  - rebooted the VAIO successfully back to `overall=remote_usable`
  - validated `kdump-config status -> current state   : ready to kdump`
  - validated `/sys/kernel/kexec_crash_loaded = 1`
- End-to-end destructive validation bundle:
  - [logs/vaio-panic-test/20260512-222708-panic-test](/root/Poulsbo/logs/vaio-panic-test/20260512-222708-panic-test)
- That panic test:
  - triggered `echo c > /proc/sysrq-trigger`
  - observed the target drop to `overall=ssh_port_down`
  - observed the target auto-reboot back to `overall=remote_usable`
  - preserved the same live override:
    - `driver_srcversion=6E95EDFFFB5384C8E58F25B`
- The first fetch pass exposed a real target-specific detail: Ubuntu kdump on
  this VAIO writes:
  - `/var/crash/202605122227/dump.202605122227`
  - `/var/crash/202605122227/dmesg.202605122227`
  not a literal `vmcore`.
- Widened:
  - `scripts/configure-vaio-panic-capture.sh`
  - `scripts/fetch-vaio-panic-artifacts.sh`
  - `scripts/show-vaio-gma500-state.sh`
  so the collector/fetch/state path now recognizes both classic `vmcore`
  layouts and Ubuntu `dump.<timestamp>` layouts.
- Refresh validation bundle:
  - [logs/vaio-panic/20260512-223405-panic-capture-setup](/root/Poulsbo/logs/vaio-panic/20260512-223405-panic-capture-setup)
- That rerun now resolves:
  - `latest_dump_present=1`
  - `latest_dump_kind=dumpfile`
  - `latest_dump_path=/var/crash/202605122227/dump.202605122227`
  - `latest_dump_dmesg_path=/var/crash/202605122227/dmesg.202605122227`
- The panic-test bundle was backfilled too:
  - [logs/vaio-panic-test/20260512-222708-panic-test/panic-artifacts/crashdump/202605122227](/root/Poulsbo/logs/vaio-panic-test/20260512-222708-panic-test/panic-artifacts/crashdump/202605122227)

Conclusion:

- the VAIO now auto-reboots after panic
- a real panic was captured and recovered on the headless target
- saved crash artifacts are now fetched correctly for offline inspection
- risky SGX work no longer depends on console-only panic forensics

### Fixed-module guarded deploy and exact `plan` reruns through `arm`

- Updated:
  - `scripts/lib-vaio-plan-summary.sh`
  - `scripts/resolve-sgx-submit-phase-profile.sh`
  - `scripts/run-vaio-sgx-plan-session.sh`
- The old live override `0EEEE302E3FDA9F59B7CFB9` is no longer the current
  test module. A guarded deploy session:
  - `PLAN_SESSION_MODE=deploy PLAN_PHASE_PROFILE=plan_query_offset ./scripts/run-vaio-sgx-plan-session.sh`
  - [logs/vaio-plan-session/20260512-213102-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-213102-sgx-plan-session)
  rebooted the VAIO onto:
  - `driver_srcversion=6E95EDFFFB5384C8E58F25B`
  - `override_module_size=429020`
  - `driver_module_loaded=1`
  - `drm_card0_present=1`
  - `lightdm_active=inactive`
  - `xorg_process_count=0`
- Exact no-redeploy reruns on that fixed module all completed cleanly:
  - `plan_query_config`
    - [logs/vaio-plan-session/20260512-213353-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-213353-sgx-plan-session)
  - `plan_query_snapshot`
    - [logs/vaio-plan-session/20260512-213943-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-213943-sgx-plan-session)
  - `prepare_op`
    - [logs/vaio-plan-session/20260512-214214-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-214214-sgx-plan-session)
  - `prepare_query`
    - [logs/vaio-plan-session/20260512-214311-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-214311-sgx-plan-session)
  - `arm`
    - [logs/vaio-plan-session/20260512-214548-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-214548-sgx-plan-session)
- Those exact reruns proved the fixed module now clears:
  - `plan_query_offset`
  - `plan_query_config`
  - `plan_query_snapshot`
  - `prepare_op`
  - `prepare_query`
  - `arm`
- The shared current-boundary resolver had been stale because it only looked
  at remote-usable rows with failures. It now also consumes successful exact
  phase-evidence rows and advances to the next phase in the full ladder.
- The named exact profiles now also cover the post-`prepare` single phases:
  - `arm`
  - `emit`
  - `validate`
  - `launch`
  - `issue`
  - `running`
  - `complete`
- Current saved boundary:
  - `resolved_profile=emit`
  - `resolved_phase=emit`
  - `resolved_reason=latest_phase_evidence_advanced`
  - `source_timestamp=20260512-214548`

Conclusion:

- the old `plan_query` read-side panic line is cleared on the fixed module
- the live `plan` investigation is now through `arm`
- the next exact live question is `emit`

### `plan_query_offset` live narrowing, headless state probe, and unsafe-module guard

- Updated:
  - `scripts/show-vaio-gma500-state.sh`
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
  - `scripts/resolve-sgx-submit-phase-profile.sh`
  - `scripts/lib-vaio-plan-summary.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
  - `scripts/run-vaio-sgx-plan-recovery-session.sh`
  - `scripts/run-vaio-sgx-plan-session.sh`
  - `scripts/report-vaio-plan-status.sh`
- `show-vaio-gma500-state.sh` is now headless-aware and reports:
  - `driver_module_loaded`
  - `drm_card_entries`
  - `drm_connector_entries`
  - `dri_dev_nodes`
  - `drm_card0_present`
  - `lightdm_enabled`
  - `lightdm_active`
  - `system_default_target`
  - `xorg_process_count`
- Live state before the exact rerun on `10.1.11.218`:
  - `driver_srcversion=0EEEE302E3FDA9F59B7CFB9`
  - `driver_module_loaded=1`
  - `drm_card0_present=1`
  - `lightdm_active=inactive`
  - `xorg_process_count=0`
- Split `plan_query_basic` further into:
  - `plan_query_offset`
  - `plan_query_config`
- Live rerun:
  - `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=plan_query_basic ./scripts/run-vaio-sgx-plan-session.sh`
  - bundle:
    - `logs/vaio-plan-session/20260512-210302-sgx-plan-session`
- Saved result:
  - `selected_failed_phase=plan_query_offset`
  - `selected_failed_reason=phase_timeout`
  - `final_driver_srcversion=0EEEE302E3FDA9F59B7CFB9`
- Saved bisect result:
  - `probe_order=plan_query_offset`
  - `first_bad_phase=plan_query_offset`
  - `first_bad_reason=phase_timeout`
  - `authoritative_terminal_bad=1`
- The probe still cleared:
  - `STAGE_OK=sync`
  - `STAGE_OK=owner`
  - `STAGE_OK=report`
  - `STAGE_OK=manifest`
  - `STAGE_OK=descriptor_op`
  - `STAGE_OK=descriptor_query`
  - `STAGE_OK=plan_op`
- The console panic after that rerun hit:
  - `Kernel panic - not syncing: stack-protector: Kernel stack is corrupted in: poulsbo_sgx_fill_submit_query_locked`
- That makes the old loaded `0EEEE302E3FDA9F59B7CFB9` candidate unsafe for
  further recovery-side `plan_query*` reruns.
- The plan report now exposes:
  - `current_recovery_profile=plan_query_offset`
  - `current_recovery_live_module_safe=0`
  - `current_recovery_live_module_warning=known_unsafe_plan_query_live_module`
- The next recommendation therefore changes from `recovery` to `deploy`:
  - `PLAN_SESSION_MODE=deploy PLAN_PHASES=plan_query_offset ./scripts/run-vaio-sgx-plan-session.sh`
  - `PLAN_SESSION_MODE=deploy PLAN_PHASE_PROFILE=plan_query_offset ./scripts/run-vaio-sgx-plan-session.sh`

Conclusion:

- the headless target baseline is now explicit instead of inferred
- the exact live bad phase is now `plan_query_offset`
- the old `0EEEE302E3FDA9F59B7CFB9` module is unsafe for further no-redeploy
  `plan_query*` work
- the next step is a guarded deploy of the fixed local tree, not another
  recovery rerun on the same candidate

### `plan_query_basic` live narrowing and exact profile aliases

- Ran the split `plan_query` no-redeploy rerun on the live candidate with:
  - `PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_query_fetch,plan_query_basic,plan_query_snapshot ./scripts/run-vaio-sgx-plan-session.sh`
- The bounded top-level session is:
  - `logs/vaio-plan-session/20260512-201327-sgx-plan-session`
- The saved result is:
  - `selected_failed_phase=plan_query_basic`
  - `selected_failed_reason=phase_timeout`
  - `final_driver_srcversion=0EEEE302E3FDA9F59B7CFB9`
- The bisect child used one probe:
  - `probe_order=plan_query_basic`
  and the live phase log for that probe still cleared:
  - `TARGET_DRM_NODE_MISSING=/dev/dri/card0`
  - `AUTOLOAD_GMA500_MODULE=gma500_gfx`
  - `TARGET_DRM_NODE_READY=/dev/dri/card0`
  - `STAGE_OK=sync`
  - `STAGE_OK=owner`
  - `STAGE_OK=report`
  - `STAGE_OK=manifest`
  - `STAGE_OK=descriptor_op`
  - `STAGE_OK=descriptor_query`
  - `STAGE_OK=plan_op`
- So the autoload self-heal worked, and the actual live boundary is now one
  step narrower than the earlier coarse `plan_query` result:
  - `plan_query_basic`
- Updated:
  - `scripts/resolve-sgx-submit-phase-profile.sh`
  - `scripts/lib-vaio-plan-summary.sh`
  - `scripts/run-vaio-sgx-plan-session.sh`
- The operator layer now exposes first-class exact profiles for:
  - `plan_query_fetch`
  - `plan_query_basic`
  - `plan_query_snapshot`
- The shared current-boundary resolver now lands on:
  - `resolved_profile=plan_query_basic`
  - `resolved_phases=plan_query_basic`
  - `resolved_phase=plan_query_basic`
  - `resolved_reason=phase_timeout`
  - `source_timestamp=20260512-201327`
  - `source_row_module_srcversion=0EEEE302E3FDA9F59B7CFB9`
- The current-state report now recommends:
  - `PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_query_basic ./scripts/run-vaio-sgx-plan-session.sh`
  - `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=plan_query_basic ./scripts/run-vaio-sgx-plan-session.sh`

Validated result:

- `bash -n` passed for:
  - `scripts/resolve-sgx-submit-phase-profile.sh`
  - `scripts/lib-vaio-plan-summary.sh`
  - `scripts/run-vaio-sgx-plan-session.sh`
- `./scripts/resolve-vaio-plan-current-phase.sh` now reports:
  - `resolved_profile=plan_query_basic`
  - `resolved_phase=plan_query_basic`
  - `resolved_reason=phase_timeout`
- `./scripts/report-vaio-plan-status.sh` now reports:
  - `current_recovery_profile=plan_query_basic`
  - `current_recovery_phase=plan_query_basic`
  - `recommended_next_phase=plan_query_basic`
  - `recommended_next_profile_command=PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=plan_query_basic ./scripts/run-vaio-sgx-plan-session.sh`

Conclusion:

- the live `plan` boundary is now narrowed to `plan_query_basic`
- the autoload self-heal is working as intended
- the next rerun can be repeated exactly with a same-name profile instead of
  falling back to the broader `post_descriptor` alias

### `plan_query` live rerun, autoload gap, and source-module precedence fix

- The booted VAIO came back with the current `plan` override file still
  installed and reporting:
  - `driver_srcversion=0EEEE302E3FDA9F59B7CFB9`
  - `rollback_pending_present=0`
  but the graphics stack was not actually live:
  - `/dev/dri` missing
  - `/sys/class/drm` missing
  - `lsmod | grep gma500` empty
- Manual `modprobe gma500_gfx` restored:
  - `/dev/dri/card0`
  - `/sys/class/drm/card0`
  and `dmesg` showed normal `gma500` init
- The first no-redeploy recovery rerun:
  - `logs/vaio-plan-session/20260512-193915-sgx-plan-session`
  failed before real SGX diagnosis at:
  - `selected_failed_phase=plan_query`
  - `selected_failed_reason=phase_failed`
  - `selected_failed_step=gtt_before`
  - `drm_open_errno=2`
  while the target stayed:
  - `last_overall=remote_usable`
- Updated:
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- The focused submit runner now detects a missing target DRM node, runs:
  - `modprobe gma500_gfx`
  waits for the node to appear, and records:
  - `TARGET_DRM_NODE_MISSING`
  - `AUTOLOAD_GMA500_MODULE`
  - `TARGET_DRM_NODE_READY`
- After manually loading `gma500_gfx`, the next no-redeploy rerun:
  - `logs/vaio-plan-session/20260512-194235-sgx-plan-session`
  still failed on the real live boundary:
  - `selected_failed_phase=plan_query`
  - `selected_failed_reason=phase_timeout`
  - `final_driver_srcversion=0EEEE302E3FDA9F59B7CFB9`
- Its bisect child:
  - `logs/vaio-plan-session/20260512-194235-sgx-plan-session/children/20260512-194247-recovery/children/20260512-194247-bisect`
  saved:
  - `probe_order=plan_query`
  - `first_bad_phase=plan_query`
  - `first_bad_reason=phase_timeout`
  - `authoritative_terminal_bad=1`
- Updated:
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
  - `scripts/resolve-sgx-submit-phase-profile.sh`
  - `scripts/lib-vaio-plan-summary.sh`
- The coarse `plan_query` stop point is now split into:
  - `plan_query_fetch`
  - `plan_query_basic`
  - `plan_query_snapshot`
  and the named profile:
  - `PLAN_PHASE_PROFILE=plan_query`
  now expands to:
  - `PLAN_PHASES=plan_query_fetch,plan_query_basic,plan_query_snapshot`
- Fixed one real summary/report bug in:
  - `scripts/summarize-vaio-plan-sessions.sh`
  where the offline plan summary had been preferring
  `local_module_srcversion` over the actual live
  `final_driver_srcversion`
- The precedence is now:
  - `final_driver_srcversion`
  - `post-state.log driver_srcversion`
  - `pre-state.log driver_srcversion`
  - `source_module_srcversion`
  - `local_module_srcversion`
- So the current saved recovery boundary now resolves as:
  - `resolved_profile=plan_query`
  - `resolved_phases=plan_query_fetch,plan_query_basic,plan_query_snapshot`
  - `resolved_phase=plan_query`
  - `resolved_reason=phase_timeout`
  - `source_timestamp=20260512-194235`
  - `source_row_module_srcversion=0EEEE302E3FDA9F59B7CFB9`

Validated result:

- `bash -n` passed for:
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
  - `scripts/resolve-sgx-submit-phase-profile.sh`
  - `scripts/lib-vaio-plan-summary.sh`
  - `scripts/report-vaio-plan-status.sh`
  - `scripts/summarize-vaio-plan-sessions.sh`
- `./scripts/resolve-vaio-plan-current-phase.sh` now reports:
  - `resolved_profile=plan_query`
  - `resolved_phases=plan_query_fetch,plan_query_basic,plan_query_snapshot`
  - `resolved_reason=phase_timeout`
  - `source_row_module_srcversion=0EEEE302E3FDA9F59B7CFB9`
- `./scripts/report-vaio-plan-status.sh` now reports:
  - `recommended_next_phases=plan_query_fetch,plan_query_basic,plan_query_snapshot`
  - `recommended_next_command=PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_query_fetch,plan_query_basic,plan_query_snapshot ./scripts/run-vaio-sgx-plan-session.sh`

Conclusion:

- the booted-machine DRM autoload gap is now handled operationally
- the real live failure is still inside `plan_query`
- the next rerun is now narrowed to the three `plan_query` subphases
- the saved current-boundary source row now reports the actually loaded module,
  not the local build artifact

### `plan` terminal bad-probe boundary correction

- Fixed one real semantic bug in the offline `plan` boundary logic:
  the saved `20260512-180606` rerun had been treated as if it moved the
  current boundary to `descriptor_query`, but its bisect logs actually show:
  - `probe=1 phase=plan_query`
  - `probe_1_reason=phase_timeout`
  - `probe_1_health=ssh_port_down`
  - `probe=2 phase=descriptor_query`
  - `probe_2_reason=phase_failed`
  - `probe_2_health=ssh_port_down`
- So the later `descriptor_query` probe was not valid new boundary evidence.
  It only ran after the earlier `plan_query` probe had already taken the
  target down.
- Updated:
  - `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
  - `scripts/lib-vaio-plan-summary.sh`
  - `scripts/summarize-vaio-plan-sessions.sh`
- Future bisects now stop once a bad probe leaves the target unhealthy and
  persist `authoritative_*` fields for the terminal bad probe.
- The shared offline summary path now backfills the same rule for older saved
  bundles, so the corrected `180606` row is now:
  - `evidence_last_started=plan_query`
  - `evidence_first_bad=plan_query`
  - `evidence_reason=phase_timeout`
- The shared current-boundary resolver now lands on:
  - `resolved_profile=plan_query`
  - `resolved_phase=plan_query`
  - `resolved_reason=phase_timeout`
  - `source_name=sgx-plan-session`
  - `source_timestamp=20260512-180606`
- The next narrowed rerun is therefore back to:
  - `PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_query ./scripts/run-vaio-sgx-plan-session.sh`
  - `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=plan_query ./scripts/run-vaio-sgx-plan-session.sh`

Validated result:

- `bash -n` passed for:
  - `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
  - `scripts/lib-vaio-plan-summary.sh`
  - `scripts/summarize-vaio-plan-sessions.sh`
- `./scripts/resolve-vaio-plan-current-phase.sh` now reports:
  - `resolved_profile=plan_query`
  - `resolved_phase=plan_query`
  - `source_row_evidence_first_bad=plan_query`
  - `source_row_evidence_reason=phase_timeout`
- `MAX_SESSIONS=6 ./scripts/summarize-vaio-plan-sessions.sh` now shows the
  `180606` row as:
  - `evidence_last_started=plan_query`
  - `evidence_first_bad=plan_query`
  - `evidence_reason=phase_timeout`

### Shared phase-source helper and strict live-evidence selector

- Added one shared boundary/source-row helper:
  - `scripts/lib-vaio-plan-phase-source.sh`
- Moved the duplicated current-boundary source plumbing into that helper for:
  - `scripts/run-vaio-sgx-plan-session.sh`
  - `scripts/run-vaio-sgx-plan-recovery-session.sh`
  - `scripts/run-vaio-recovery-session.sh`
  - `scripts/report-vaio-plan-status.sh`
- The helper now centralizes:
  - last-value reads from resolver/result env files
  - source-row initialization
  - current-resolver assignment
  - environment import/export for child wrappers
  - prefixed source/source-row emission into manifests and results
- Added one stricter shared evidence-row selector in:
  - `scripts/lib-vaio-plan-summary.sh`
  - `vaio_plan_summary_pick_latest_phase_evidence_row()`
- The current-boundary resolver and report now use that stricter selector,
  which only accepts remote-usable `plan_session` rows that carry real
  evidence through at least one of:
  - `evidence_first_bad`
  - `evidence_reason`
  - `phase_source_reason`
- That closes a real live bug: while
  `logs/vaio-plan-session/20260512-180606-sgx-plan-session` was still
  in-flight, the older selector tried to treat that new
  `interrupted_or_incomplete remote_usable` parent as the newest evidence row,
  which broadened the current boundary back toward `post_descriptor` because
  the row had no real `first_bad` / `reason` evidence yet
- While `20260512-180606` was still in flight, the hardened selector kept the
  resolver/report pinned to the stable earlier boundary:
  - `resolved_profile=plan_query`
  - `resolved_reason=phase_timeout`
  - `source_name=sgx-plan-session-plan-op-plan-query-live`
  - `source_timestamp=20260512-154733`
- The same live `180606` rerun still produced one more useful datapoint before
  the machine dropped again. Its bisect child:
  - `logs/vaio-plan-session/20260512-180606-sgx-plan-session/children/20260512-180625-recovery/children/20260512-180625-bisect`
  started:
  - `probe=1 phase=plan_query`
  and the live phase log reached:
  - `STAGE_OK=sync`
  - `STAGE_OK=owner`
  - `STAGE_OK=report`
  - `STAGE_OK=manifest`
  - `STAGE_OK=descriptor_op`
  - `STAGE_OK=descriptor_query`
  - `STAGE_OK=plan_op`
  before the target dropped back to `ssh_port_down`
- Once that same rerun finished writing real bisect evidence, it legitimately
  became the newest phase-evidence source. The current boundary is now:
  - `resolved_profile=post_descriptor`
  - `resolved_phase=descriptor_query`
  - `resolved_reason=phase_failed`
  - `source_name=sgx-plan-session`
  - `source_timestamp=20260512-180606`

Validated result:

- `bash -n` passed for:
  - `scripts/lib-vaio-plan-phase-source.sh`
  - `scripts/lib-vaio-plan-summary.sh`
  - `scripts/run-vaio-sgx-plan-recovery-session.sh`
  - `scripts/run-vaio-recovery-session.sh`
  - `scripts/run-vaio-sgx-plan-session.sh`
  - `scripts/report-vaio-plan-status.sh`
- `./scripts/resolve-vaio-plan-current-phase.sh` first ignored the newer
  in-flight `180606` parent, and after completion now correctly reports:
  - `resolved_profile=post_descriptor`
  - `resolved_phase=descriptor_query`
  - `source_name=sgx-plan-session`
  - `source_row_evidence_first_bad=descriptor_query`
- `./scripts/report-vaio-plan-status.sh` now reports the completed `180606`
  row as the newest real evidence:
  - `latest_remote_usable_phase_evidence_timestamp=20260512-180606`
  - `current_recovery_profile=post_descriptor`
  - `current_recovery_phase=descriptor_query`
  - `recommended_next_phase=descriptor_query`

Conclusion:

- boundary/source-row propagation is now centralized instead of hand-copied
- the current-boundary resolver/report no longer self-poison from newer
  remote-usable in-progress `plan_session` parents without actual evidence
- the active narrowed live question is now `descriptor_query`

### Shared `plan` summary library and boundary parser consolidation

- Added one shared offline helper:
  - `scripts/lib-vaio-plan-summary.sh`
- Moved the duplicated `plan` summary/bisect/current-boundary parsing into that
  library:
  - header-aware TSV column parsing
  - latest-session selection
  - latest bisect summary/result discovery
  - bisect `last_started` / `last_good` extraction
  - named `full` / `post_descriptor` / `plan_query` phase-profile expansion
  - current saved live recovery-boundary resolution
- Switched the following tools onto that one helper instead of keeping local
  copies of the same parsing code:
  - `scripts/resolve-vaio-plan-current-phase.sh`
  - `scripts/report-vaio-plan-status.sh`
  - `scripts/summarize-vaio-plan-sessions.sh`
- The shared resolver still selects the same current saved live boundary:
  - `resolved_profile=plan_query`
  - `resolved_phases=plan_query`
  - `resolved_phase=plan_query`
  - `resolved_reason=phase_timeout`
  - `source_kind=plan_session`
  - `source_name=sgx-plan-session-plan-op-plan-query-live`
  - `source_timestamp=20260512-154733`
- The current-state report still emits the same next-step commands:
  - `PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_query ./scripts/run-vaio-sgx-plan-session.sh`
  - `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=plan_query ./scripts/run-vaio-sgx-plan-session.sh`
- The latest blocked top-level summary rows remain:
  - `requested=auto:profile[plan_query]`
  - `resolved=plan_query:profile[plan_query]`
  - `child=plan_query:request_only[plan_query]`
  - `phase_source=phase_timeout@20260512-154733`

Validated result:

- `bash -n` passed for:
  - `scripts/lib-vaio-plan-summary.sh`
  - `scripts/resolve-vaio-plan-current-phase.sh`
  - `scripts/report-vaio-plan-status.sh`
  - `scripts/summarize-vaio-plan-sessions.sh`
- `./scripts/resolve-vaio-plan-current-phase.sh` now reports the shared source
  identity correctly:
  - `source_kind=plan_session`
  - `source_name=sgx-plan-session-plan-op-plan-query-live`
- `./scripts/report-vaio-plan-status.sh` still reports:
  - `current_recovery_profile=plan_query`
  - `current_recovery_reason=phase_timeout`
  - `recommended_next_phase=plan_query`
- the target is still down:
  - `ping_status=fail`
  - `tcp_22_status=closed`
  - `ssh_pubkey_status=no_route`
  - `overall=ssh_port_down`

Conclusion:

- there is now one source of truth for offline `plan` boundary parsing
- future summary-schema or bisect-evidence changes only need to be implemented
  once instead of being copied across multiple wrappers

### Structured `plan` phase-source columns in summary and report

- The offline `plan` tooling still had one lossy spot:
  - saved bundle results already preserved
    `selected_phase_source_reason/kind/name/timestamp/session_dir`
  - but `scripts/summarize-vaio-plan-sessions.sh` collapsed that back to
    `phase_source=phase_timeout@20260512-154733`
- Updated:
  - `scripts/summarize-vaio-plan-sessions.sh`
  - `scripts/report-vaio-plan-status.sh`
- The summary now keeps the compact `phase_source` display for quick scanning,
  but also exports:
  - `phase_source_reason`
  - `phase_source_kind`
  - `phase_source_name`
  - `phase_source_timestamp`
  - `phase_source_session_dir`
- The report now emits those same structured saved-row fields, for example:
  - `latest_remote_usable_phase_evidence_phase_source_reason=phase_timeout`
  - `latest_remote_usable_phase_evidence_phase_source_kind=plan_session`
  - `latest_remote_usable_phase_evidence_phase_source_name=sgx-plan-session-plan-op-plan-query-live`
  - `latest_remote_usable_phase_evidence_phase_source_timestamp=20260512-154733`
- The canonical blocked summary row remains:
  - `requested=auto:profile[plan_query]`
  - `resolved=plan_query:profile[plan_query]`
  - `child=plan_query:request_only[plan_query]`
  - `phase_source=phase_timeout@20260512-154733`
  but now also carries the same source identity structurally.

Validated result:

- `bash -n scripts/summarize-vaio-plan-sessions.sh`
- `bash -n scripts/report-vaio-plan-status.sh`
- `OUTPUT_FORMAT=tsv MAX_SESSIONS=2 ./scripts/summarize-vaio-plan-sessions.sh`
- `./scripts/report-vaio-plan-status.sh`
- the target is still down:
  - `overall=ssh_port_down`

Conclusion:

- the offline summary, current-state report, and saved bundle results now
  expose the same phase-source provenance model instead of mixing a structured
  resolver with a lossy summary string

### Resolved source row now persists into saved parent bundles

- The report and resolver already knew the full source row behind the current
  `plan_query` boundary, but saved top-level parent bundles still only kept the
  smaller source-identity tuple.
- Updated:
  - `scripts/lib-vaio-plan-summary.sh`
  - `scripts/resolve-vaio-plan-current-phase.sh`
  - `scripts/run-vaio-sgx-plan-session.sh`
  - `scripts/run-vaio-sgx-plan-recovery-session.sh`
  - `scripts/run-vaio-recovery-session.sh`
  - `scripts/report-vaio-plan-status.sh`
- The resolver now emits the source row directly, including:
  - `source_row_requested`
  - `source_row_resolved`
  - `source_row_child`
  - `source_row_phase_source`
  - `source_row_module_srcversion`
  - `source_row_evidence_last_good`
  - `source_row_evidence_first_bad`
  - `source_row_evidence_reason`
- The top-level plan parent now persists those as:
  - `selected_phase_source_row_*`
- The recovery-side wrapper now persists those as:
  - `validation_phase_source_row_*`
- The current-state report now prefers the resolver’s direct source-row payload
  when emitting:
  - `current_recovery_source_row_*`
- Fresh blocked proof:
  - `logs/vaio-plan-session/20260512-173551-sgx-plan-session`
  - top-level result now includes:
    - `selected_phase_source_row_requested=post_descriptor:explicit[plan_op,plan_query]`
    - `selected_phase_source_row_evidence_last_good=plan_op`
    - `selected_phase_source_row_evidence_first_bad=plan_query`
    - `selected_phase_source_row_evidence_reason=phase_timeout`

Validated result:

- `bash -n` passed on the touched scripts
- `./scripts/resolve-vaio-plan-current-phase.sh` now includes `source_row_*`
- `./scripts/report-vaio-plan-status.sh` now includes `current_recovery_source_row_*`
- `MAX_POLLS=1 POLL_INTERVAL_SECS=1 PLAN_SESSION_MODE=recovery ./scripts/run-vaio-sgx-plan-session.sh`
  wrote a fresh blocked parent with populated `selected_phase_source_row_*`

Conclusion:

- the current boundary source row is now persisted into saved parent bundles
  instead of only being rediscoverable through the summary

### Current-state report now dereferences the resolved source row

- The report already surfaced the current boundary source identity:
  - `current_recovery_source_kind=plan_session`
  - `current_recovery_source_name=sgx-plan-session-plan-op-plan-query-live`
  - `current_recovery_source_timestamp=20260512-154733`
- The remaining gap was that operators still had to run the separate summary or
  mentally join fields to see what that source bundle’s own saved row said.
- Updated:
  - `scripts/lib-vaio-plan-summary.sh`
  - `scripts/report-vaio-plan-status.sh`
- Added one shared exact-row lookup helper in the plan-summary library:
  - `vaio_plan_summary_find_row()`
- The report now uses the resolved source identity to emit one more saved-row
  block directly:
  - `current_recovery_source_row_present`
  - `current_recovery_source_row_requested`
  - `current_recovery_source_row_resolved`
  - `current_recovery_source_row_child`
  - `current_recovery_source_row_failed_phase`
  - `current_recovery_source_row_failed_step`
  - `current_recovery_source_row_evidence_last_good`
  - `current_recovery_source_row_evidence_first_bad`
  - `current_recovery_source_row_evidence_reason`
- All emitted session blocks now also carry the summary-row evidence columns:
  - `*_evidence_last_started`
  - `*_evidence_last_good`
  - `*_evidence_first_bad`
  - `*_evidence_reason`

Validated result:

- `bash -n scripts/lib-vaio-plan-summary.sh`
- `bash -n scripts/report-vaio-plan-status.sh`
- `./scripts/report-vaio-plan-status.sh`
- current boundary is still:
  - `plan_query`
  - `phase_timeout`
  - `source=sgx-plan-session-plan-op-plan-query-live@20260512-154733`

Conclusion:

- one report now gives both the current selected boundary and the exact saved
  source row that established it

### SGX `plan` rerun reaches `descriptor` again and wrapper capture goes pipe-free

- The VAIO came back to `overall=remote_usable` on the already-loaded guarded
  deploy candidate:
  - `driver_srcversion=0EEEE302E3FDA9F59B7CFB9`
  - `override_module_size=429756`
- Reran the top-level operator path without redeploy:
  - `PLAN_SESSION_MODE=recovery ./scripts/run-vaio-sgx-plan-session.sh`
  - `logs/vaio-plan-session/20260512-132138-sgx-plan-session`
- The first cumulative bisect probe was still `phase=prepare`, and the live
  submit runner again advanced through:
  - `STAGE_OK=sync`
  - `STAGE_OK=owner`
  - `STAGE_OK=report`
  - `STAGE_OK=manifest`
  - `STAGE_OK=descriptor`
  before the target dropped off the network again.
- That means the local `plan` dword-count fix did not regress the already-good
  prefix; the current failure boundary remains at or after the
  `descriptor -> plan / prepare` edge on this candidate.
- The rerun exposed one new local orchestration bug:
  - the timed focused child died
  - but the parent bisect / investigation / recovery / operator stack stayed
    hung locally because several wrappers were capturing child output with
    `... | tee logfile`
  - timed-out descendants kept the pipe write side open, so `tee` never saw
    EOF and the parent never completed
- Fixed that wrapper class in:
  - `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
  - `scripts/run-vaio-recovery-session.sh`
  - `scripts/run-vaio-sgx-plan-session.sh`
  - `scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`
  - `scripts/run-vaio-guarded-deploy-session.sh`
- New local capture model:
  - run child with stdout/stderr redirected straight to a log file
  - capture the real child rc directly
  - replay the finished log with `cat`
- Cleaned up the already-hung local wrapper processes from the failed rerun.

Validated result:

- `bash -n` passed for all touched wrapper scripts
- the VAIO is currently back to:
  - `ping_status=fail`
  - `tcp_22_status=closed`
  - `ssh_pubkey_status=no_route`
  - `overall=ssh_port_down`

Conclusion:

- the next recovery-driven `plan` rerun should now return a bounded timeout or
  failure result instead of hanging locally after target loss
- the live failure boundary is still at or after `descriptor`, not back at the
  older `kick` rejection

### SGX submit word-block size hardening

- Fixed one concrete kernel-side overrun in
  `drivers/gpu/drm/gma500/psb_drv.c`:
  - `PSB_SGX_LAB_PLAN_DWORDS` had drifted to `42`
  - `struct psb_sgx_lab_plan_words` actually contained `41` `u32` fields
- That meant both:
  - `poulsbo_sgx_write_submit_plan_locked()`
  - `poulsbo_sgx_read_submit_plan_locked()`
  were iterating one dword past the private plan image.
- Fixed the immediate bug by changing `PSB_SGX_LAB_PLAN_DWORDS` to `41`.
- Widened that into a class-level hardening pass for the normalized
  word-backed submit blocks:
  - added compile-time size checks for:
    - queue header
    - CCB record
    - sync
    - owner
    - report
    - manifest
    - descriptor
    - plan
    - dependency
  - added shared word-copy helpers:
    - `poulsbo_sgx_object_write_words_locked()`
    - `poulsbo_sgx_object_read_words_locked()`
  - rewired the loop-based read/write paths to derive their word counts from
    `sizeof(struct) / sizeof(u32)` instead of repeating hand-kept dword loop
    bounds for:
    - queue header
    - CCB record
    - sync
    - owner
    - report
    - descriptor
    - plan
    - dependency

Validated result:

- local kernel-tree diff hygiene passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- local Ubuntu 5.4 module rebuild passed:
  - `./scripts/docker-build-vaio-gma500.sh`
- there is still no new hardware validation result because the VAIO remained
  down throughout this batch:
  - `ping_status=fail`
  - `tcp_22_status=closed`
  - `ssh_pubkey_status=no_route`
  - `overall=ssh_port_down`

Conclusion:

- the concrete `plan` dword-count overrun is fixed locally
- future drift between normalized private word-backed structs and their dword
  block sizes now fails at build time instead of silently overrunning a stale
  loop bound

### Live `plan` rerun now captures the real `kick` failure on a healthy target

- The VAIO stayed at `overall=remote_usable` on override
  `srcversion 2BE09D0DC1CCBDB51D2328E`, size `429820`.
- Fixed one coherent propagation gap across the `plan` investigation stack:
  - `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
    - now captures stderr as well as stdout from the focused submit runner
    - now records `first_bad_failed_step`, `first_bad_failed_rc`,
      `first_bad_failed_log`, and `first_bad_remote_tmpdir`
  - `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
    - now captures stderr as well as stdout from the focused submit runner
    - now records `last_phase_failed_step`, `last_phase_failed_rc`,
      `last_phase_failed_log`, and `last_phase_remote_tmpdir`
  - `scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
    - now propagates the child failed-step fields from both bisect and
      confirm
  - `scripts/run-vaio-recovery-session.sh`
    - now captures stderr from the validation child
    - now propagates `validation_failed_phase`, `validation_failed_step`, and
      `validation_failed_rc`
  - `scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`
    - now preserves the recovery child’s failed-phase and failed-step fields
  - `scripts/run-vaio-sgx-plan-session.sh`
    - now surfaces the selected child’s failed phase, step, and rc directly in
      the top-level parent result
- Reran the operator entrypoint in recovery mode:
  - `PLAN_SESSION_MODE=recovery ./scripts/run-vaio-sgx-plan-session.sh`
  - `logs/vaio-plan-session/20260512-111909-sgx-plan-session`
- Corrected live result:
  - top-level parent:
    - `session_outcome=validation_failed`
    - `selected_failed_phase=sync`
    - `selected_failed_step=submit_op_kick_focus`
    - `selected_failed_rc=1`
  - recovery wrapper:
    - `validation_failed_phase=sync`
    - `validation_failed_step=submit_op_kick_focus`
    - `validation_failed_rc=1`
  - bisect:
    - `probe_order=prepare,report,sync`
    - `last_good_phase=none`
    - `first_bad_phase=sync`
    - `first_bad_failed_step=submit_op_kick_focus`
    - `first_bad_failed_rc=1`
  - confirm:
    - `last_phase=sync`
    - `last_phase_failed_step=submit_op_kick_focus`
    - `last_phase_failed_rc=1`
- The captured failing ioctl is stable:
  - `submit_op_name=kick`
  - `submit_selector=focus`
  - `submit_op_ret=-1 errno=22`
  - `submit_cmd_type_name=ta`
  - `submit_ccb_offset=0x20`
  - `submit_ctrl_flags=0x1`
- Result:
  - the target is no longer blocked on recovery
  - the current `plan` candidate is no longer “prepare/report healthy, sync
    bad”
  - all currently probed prefixes fail at the same bounded live substep:
    `submit_op_kick_focus`
- Documented in:
  - `docs/136-sgx-plan-kick-failure-clean-rerun.md`

### SGX submit `report` struct normalization

- Normalized the memory-backed submit `report` block onto the same structured
  helper model as `manifest`, `descriptor`, and `plan`.
- Added `struct psb_sgx_lab_report_words` plus shared helpers in
  `drivers/gpu/drm/gma500/psb_drv.c`:
  - `poulsbo_sgx_build_submit_report_locked()`
  - `poulsbo_sgx_write_submit_report_locked()`
  - `poulsbo_sgx_read_submit_report_locked()`
- Extended the shared submit-view refresh preflight so it now also builds
  `report` before the first write, not just `manifest`, `descriptor`, and
  `plan`.
- Changed the refresh path to write `report` through the structured helper and
  mirror the structured image back into the in-memory submit snapshot.
- Changed status-side backing-image validation so `report` is now validated as
  one bounded image via `read + build + memcmp`, instead of as an inline
  field-by-field special case.

Validated result:

- local kernel-tree diff hygiene passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- local Ubuntu 5.4 module rebuild passed:
  - `./scripts/docker-build-vaio-gma500.sh`
- there is still no new hardware validation result because the VAIO remained
  down throughout this batch:
  - `ping_status=fail`
  - `tcp_22_status=closed`
  - `ssh_pubkey_status=no_route`
  - `overall=ssh_port_down`

Conclusion:

- all four memory-backed submit views now share the same structured backing
  image model:
  - `report`
  - `manifest`
  - `descriptor`
  - `plan`
- the local `plan` recovery candidate is more internally consistent, so the
  first post-recovery rerun is more likely to stop at one bounded validator or
  readback point instead of through a stale `report`-specific path

### SGX submit fixed-block struct normalization

- Normalized the remaining fixed submit blocks onto the same structured helper
  model already used by the memory-backed submit views.
- Added structured images in `drivers/gpu/drm/gma500/psb_drv.c` for:
  - queue header
  - CCB record
  - sync block
  - owner block
- Added shared build/read/write helpers for those four blocks.
- Changed the control-side validator so queue header and CCB record now use
  `read + build + memcmp` instead of field-by-field scalar comparison.
- Changed the status-side validator so sync and owner now use
  `read + build + memcmp` instead of field-by-field scalar comparison.
- Rewired the fixed-block write sites to use the same structured helpers
  across:
  - direct configure ops
  - `prepare`
  - `issue`
  - `transition`
- Updated the typed submit-query path so live CCB readback now also comes
  through the structured CCB helper.

Validated result:

- local kernel-tree diff hygiene passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- local Ubuntu 5.4 module rebuild passed:
  - `./scripts/docker-build-vaio-gma500.sh`
- there is still no new hardware validation result because the VAIO remained
  down throughout this batch:
  - `ping_status=fail`
  - `tcp_22_status=closed`
  - `ssh_pubkey_status=no_route`
  - `overall=ssh_port_down`

Conclusion:

- the local candidate now has one consistent structured model across both:
  - the fixed queue/status blocks
  - the newer memory-backed submit views
- the first post-recovery `plan` rerun is less likely to depend on ad hoc
  fixed-block write paths that diverge from what the validator and query path
  read back

### SGX submit dependency-block struct normalization

- Normalized dependency-state slots onto the same structured helper model used
  by the other bounded submit blocks.
- Added `struct psb_sgx_lab_dependency_words` in
  `drivers/gpu/drm/gma500/psb_drv.c`.
- Added shared helpers to:
  - build one dependency-slot image from live submit/dependency state
  - read one dependency-slot image from the status object
  - write one dependency-slot image to the status object
  - store the mirrored snapshot into the in-memory submit state
  - load that mirrored snapshot back out for validation
- Changed dependency-block refresh to use the structured helper path.
- Changed status-side dependency validation to compare the live block against
  one structured expected snapshot image instead of field-by-field scalar
  reads.
- Changed dependency-slot clear to use the same structured zero-image write and
  structured snapshot clear path.

Validated result:

- local kernel-tree diff hygiene passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- local Ubuntu 5.4 module rebuild passed:
  - `./scripts/docker-build-vaio-gma500.sh`
- there is still no new hardware validation result because the VAIO remained
  down throughout this batch:
  - `ping_status=fail`
  - `tcp_22_status=closed`
  - `ssh_pubkey_status=no_route`
  - `overall=ssh_port_down`

Conclusion:

- the remaining dependency path no longer has a separate ad hoc write/validate
 /clear representation
- the local candidate now has one structured helper model across the full
  bounded status-side submit image set

## 2026-05-11

### SGX submit `dispatch` state plus focused ticket-baseline fix

- Finished the first persistent post-`issue` execution snapshot on top of the
  existing `prepare + arm + emit + launch` path.
- The typed query and dump helpers now expose `dispatch_*` counters, queue and
  sync state, tickets, object IDs, GPU VAs, dependency-active count, and
  `last_dispatch_jiffies`.
- Fixed one real focused-runner regression in
  `scripts/run-vaio-sgx-ioctl-submit-op.sh`: it had hard-coded global
  issue/running/complete ticket `=1`, which only held on a cold coordination
  state. It now resolves live ticket baselines from the target before launch
  and asserts baseline-relative values instead.
- Captured canonical live bundles in:
  - `logs/vaio-ioctl/20260511-192912-sgx-submit-dispatch-live/summary.log`
  - `logs/vaio-ioctl/20260511-192947-sgx-multi-submit-dispatch-live/summary.log`

Validated result on `10.1.11.218`:

- guarded deploy succeeded and returned the target to `overall=remote_usable`
- live module after deploy:
  - `driver_srcversion=D73A90E161CB5B4680FD013`
  - `override_module_size=406352`
  - `rollback_pending_present=0`
- focused typed submit-op run proved:
  - coordination baseline `issue/running/complete = 6/6/6`
  - final coordination state `7/7/7`
  - final dispatch state:
    - `submit_dispatch_count=1`
    - `submit_dispatch_prepare_count=1`
    - `submit_dispatch_arm_count=1`
    - `submit_dispatch_emit_count=1`
    - `submit_dispatch_launch_count=1`
    - `submit_dispatch_owner_state_name=complete`
    - `submit_dispatch_issued_packets=1`
    - `submit_dispatch_completed_packets=2`
    - `submit_dispatch_queue_packet_count=0`
    - `submit_dispatch_global_issue_ticket=7`
    - `submit_dispatch_global_running_ticket=7`
    - `submit_dispatch_global_complete_ticket=7`
- typed multi-submit run proved:
  - submit A final:
    - `submit_dispatch_count=3`
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B final:
    - `submit_dispatch_count=2`
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_dependency_active_count=1`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
  - final typed cleanup still returned:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`

Conclusion:

- the lab path now has a persistent dispatched execution snapshot after
  `issue`, not just pre-`issue` materialization checkpoints
- the next useful kernel step is now to build the first more SGX-shaped
  execution structure on top of that dispatched image, not more wrapper or
  query growth

### SGX lab `launch` stage plus focused launch-path fix

- Added typed submit-op `launch` to the Ubuntu 5.4 lab tree in
  `psb_drm_lab.h` and `psb_drv.c`.
- Added persistent launched issue-plan state to the submit descriptor and typed
  query path.
- Changed the typed issue path so it consumes one previously launched snapshot
  instead of recomputing directly from arbitrary mutable submit state.
- Extended `psb_sgx_lab_query.c` and `psb_sgx_lab_dump.c` with launched-state
  fields.
- Extended the focused and typed multi-submit runners to drive:
  - `prepare`
  - `arm`
  - `validate`
  - `emit`
  - `validate`
  - `launch`
  - `validate`
  - `issue`
- Fixed one real userspace bug in `psb_sgx_lab_submit_op.c`: `launch` had been
  parsed, but it was missing from the selector-parsing set, so
  `launch focus ...` incorrectly submitted `submit_id=0`.
- Added remote-shell tracing and explicit typed submit-op failure reporting to
  `run-vaio-sgx-ioctl-submit-op.sh` so future focused launch or issue failures
  do not exit silently.
- Captured canonical live bundles in:
  - `logs/vaio-ioctl/20260511-190148-sgx-submit-launch-op-live/summary.log`
  - `logs/vaio-ioctl/20260511-190211-sgx-multi-submit-launch-live/summary.log`

Validated result on `10.1.11.218`:

- guarded deploy succeeded and returned the target to `overall=remote_usable`
- live module after deploy:
  - `driver_srcversion=A6F49F33394FCA892D9A5B2`
  - `override_module_size=404800`
  - `rollback_pending_present=0`
- focused typed submit-op run proved:
  - `submit_launch_valid=1`
  - `submit_launch_consumed=0 -> 1` across `launch -> issue`
  - `submit_launch_count=1`
  - `submit_launch_prepare_count=1`
  - `submit_launch_arm_count=1`
  - `submit_launch_emit_count=1`
  - `submit_launch_owner_state_name=queued`
  - `submit_launch_issued_packets=1`
  - `submit_launch_queue_packet_count=2`
  - `submit_launch_sync_pending_seq=0x42`
  - `submit_launch_owner_issue_cookie=0x102`
  - `submit_launch_global_issue_ticket=1`
- typed multi-submit run proved:
  - submit A final `submit_launch_count=3`
  - submit B final `submit_launch_count=2`
  - submit B launched dependency-bearing state had
    `submit_launch_dependency_active_count=1`
  - both submits still completed cleanly through dependency-gated progression
    and typed cleanup

Conclusion:

- the lab path now has a first-class launched issue-plan checkpoint on top of
  `prepare + arm + emit`
- the next useful kernel step is to build a more SGX-shaped execution
  structure on top of that launched image, not more read-side plumbing

## 2026-05-10

### Repo bootstrap and design baseline

- Wrote the initial reverse-engineering, kernel, Mesa, Xorg, clean-room, and
  testing documents.
- Added baseline tooling for target-state collection, trace directory creation,
  MMIO annotation, and MMIO comparison.
- Added the initial register database workflow and a script that can regenerate
  `data/psb_registers.csv` from upstream `psb_reg.h`.

Validated result:

- the repo has a reproducible documentation and tooling baseline for Poulsbo RE
- upstream `gma500` was mapped as a display-first driver, not a render stack

### Ubuntu 18.04 build environment

- Added Docker-based Ubuntu 18.04 build environments for native and 32-bit
  builds.
- Added local build helpers for workloads, out-of-tree module experiments, and
  Ubuntu kernel-tree `gma500` builds.
- Added the local-build/remote-deploy workflow for the Sony VAIO target.

Validated result:

- Bionic package availability was confirmed
- locally built kernel and userspace artifacts can be produced reproducibly

### Ubuntu 5.4 `gma500` lab instrumentation

- Forked the Ubuntu 5.4 `gma500` tree under `third_party/linux-ubuntu-5.4`.
- Added SGX IRQ and BIF/MMU fault instrumentation in `psb_irq.c` and
  `psb_drv.h`.
- Exported the live patch as
  `patches/ubuntu-5.4/0001-drm-gma500-add-sgx-lab-instrumentation.patch`.

Validated result:

- local builds succeed against the mirrored `5.4.0-150-generic` target headers
- the target can boot the locally built override module

### Deterministic fbdev 2D validation on the VAIO

- Routed the fbdev path to the Poulsbo 2D helpers where available.
- Added deterministic sysfs-driven lab commands on the live PCI device.
- Implemented and validated hardware-backed `fillrect`, `copyarea`, and chunked
  mono `imageblit` paths.
- Added the remote helper `scripts/run-vaio-lab-2d.sh`.

Validated result on `10.1.11.218`:

- `clear` produces `2D fillrect submit` and `TWOD_COMPLETE`
- `fill` produces `2D fillrect submit` and `TWOD_COMPLETE`
- `fill ... xor` produces `rop=1` and `TWOD_COMPLETE`
- `copy` produces `2D copyarea submit` and `TWOD_COMPLETE`
- `mono` produces `2D imageblit submit-chunked` and repeated `TWOD_COMPLETE`
- no SGX/BIF fault growth was observed during those runs

Conclusion:

- the deterministic fbdev-side 2D envelope is real on hardware
- current verified 2D coverage is copy, fill, xor fill, and mono blit

### X11, EGL, DRI2, and DRM probe path

- Added 32-bit userspace probes for X11/EGL, X11/DRI2, and pure DRM.
- Added remote runners for each probe against the live VAIO target.
- Documented the LightDM auth path required to talk to the current `:0` server.

Validated result on `10.1.11.218`:

- X11 and EGL context creation work on the live display
- rendering is software through `llvmpipe`
- X exports the DRI2 extension, but screen 0 reports `dri2_version=0.0` and no
  driver/device strings
- pure DRM probing confirms KMS plus dumb buffers, but no PRIME, no render
  node, no syncobj, and no render-facing uAPI

Conclusion:

- the current open stack is functioning as display/KMS plus software rendering
- the blocker is not cookie handling or `drmAuthMagic`
- the blocker is the absence of an SGX render path and corresponding Mesa work

### Documentation discipline added

- Added the standing documentation rule to `README.md`.
- Added this progress log.
- Added the active work tracker in `docs/12-active-todo.md`.

Validated result:

- the repo now has a fixed place to record progress and remaining work for every
  substantial step

### SGX live snapshot path on the VAIO

- Added a persistent SGX snapshot structure to `dev_priv`.
- Added the PCI-device sysfs view `poulsbo_sgx_state`.
- Wired the snapshot capture into the existing lab command path and SGX IRQ path.
- Updated `scripts/run-vaio-lab-2d.sh` to print SGX state before and after the
  default 2D sequence.

Validated result on `10.1.11.218`:

- the VAIO booted the new locally built module with
  `srcversion 1488E719E92B4497BFCF240`
- `poulsbo_sgx_state` exists on the live PCI device
- the helper captured `SGX_STATE_BEFORE` with `sequence=642`, `irq_count=641`,
  `fault_count=0`, `dir0=0x341da000`, `2d_base=0x80000000`,
  `3d_base=0x30000000`, `2d_blit_status=0x000582`
- the helper captured `SGX_STATE_AFTER` with `sequence=654`, `irq_count=648`,
  `fault_count=0`, `2d_blit_status=0x000590`
- the same run still produced clean `TWOD_COMPLETE` interrupts for `clear`,
  `fill`, `copy`, and chunked `mono` blits

Conclusion:

- the repo now has a real hardware-backed SGX/MMU/BIF snapshot path
- we can compare live SGX register state across known 2D workloads without
  adding a render uAPI yet

### GEM, GTT, and MMU audit plus live range-state path

- Audited the Ubuntu 5.4 `gem.c`, `gtt.c`, and `mmu.c` object path.
- Added live per-range and per-MMU counters through `poulsbo_gtt_state`.
- Added the VAIO helper `scripts/run-vaio-gtt-state.sh`.
- Documented the structural findings in `docs/13-gem-mmu-audit.md`.
- Fixed `psb_gem_create()` so it respects the caller's `align` argument.
- Fixed `psb_mmu_insert_pfn_sequence()` so allocation failures propagate
  through its return value instead of being silently converted to success.

Validated result on `10.1.11.218`:

- the VAIO booted the new locally built module with
  `srcversion FF3F35B2E190C724432A766`
- `poulsbo_gtt_state` exists on the live PCI device
- the initial GTT/MMU snapshot reported
  `flush=2 set_pd_context=2 insert_pfn_sequence=1 insert_pages=2 remove_pages=0`
- the same view reported live `cursor`, `fb`, and `gem` ranges, including a
  live mmap'd GEM object with `in_gart=2`, `mmapping=1`, and `npage=1200`
- the 2D fbdev lab run did not change MMU counters, confirming it stays on the
  stolen-fb path
- after `drm_probe`, the MMU counters changed to
  `flush=4 insert_pages=3 remove_pages=1`

Conclusion:

- the repo now has a hardware-backed view of the actual GEM-to-GTT-to-MMU path
- the stolen-fb 2D path and the dumb-buffer GEM/MMU path are now clearly
  distinguishable on real hardware

### Additional source triage

- Triaged additional Poulsbo-adjacent source leads and wrote the result into
  `docs/14-source-triage.md`.
- Updated `docs/sources.md` with the new external references.
- Identified old Intel/EMGD/OpenEmbedded `pvr/services4` trees as the highest
  value historical source lead so far.

Validated result:

- the repo now distinguishes high-value historical SGX references from lower
  value community context and distractions
- the next archival mining step has a concrete source priority order instead of
  an unstructured link list

### First explicit PF-PD bind path on real hardware

- Added the first explicit SGX-aware bind control in the Ubuntu 5.4 lab tree:
  `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_bind`.
- Added PF-PD bind accounting to `gtt_range` and exposed it through
  `poulsbo_gtt_state`.
- Refactored `gtt.c` pin/unpin handling into internal locked helpers so the
  lab bind path can share the real GEM/GTT/MMU lifetime code.
- Extended `drm_probe.c` with a hold mode and added
  `scripts/run-vaio-pf-bind.sh` to exercise a live dumb GEM object on the
  VAIO.
- Wrote the design and validation note in `docs/15-pf-pd-bind-lab.md`.

Validated result on `10.1.11.218`:

- the VAIO booted the new locally built module with
  `srcversion D18FBB9CEBF42668699D620`
- the new `poulsbo_sgx_bind` sysfs control exists on the live PCI device
- `run-vaio-pf-bind.sh` created a held dumb GEM object as `range id=10` at
  `gpu_va=0x81127000` with `npage=4`
- MMU counters moved in the expected sequence:
  - held dumb buffer: `insert_pages` `2 -> 3`
  - PF bind: `insert_pages` `3 -> 4`
  - PF unbind: `remove_pages` `0 -> 1`
  - probe exit and destroy: `remove_pages` `1 -> 2`
- per-range bind state moved as expected:
  - after bind: `pf_bind_count=1 pf_bind_total=1`
  - after unbind: `pf_bind_count=0 pf_unbind_total=1`
- target `dmesg` logged both
  `lab pf_bind id=10 ...` and `lab pf_unbind id=10 ...`
- `poulsbo_sgx_state` still reported `fault_count=0` and `bif_fault=0x000000`
  after the run

Conclusion:

- the repo now has a hardware-validated explicit SGX-context bind lifetime over
  a real non-stolen GEM object
- the next render step is no longer “invent the first bind path”; it is
  “turn the working bind path into a clearer SGX object model and submission
  interface”

### First kernel-owned SGX lab object lifetime

- Added the SGX lab object control and object-state views:
  - `poulsbo_sgx_object`
  - `poulsbo_sgx_objects`
- Added a kernel-side object wrapper that adopts a live `gtt_range` with a real
  GEM reference.
- Added object-aware bind and unbind commands over the existing PF-PD bind lab
  path.
- Added `sgx_lab_object_id` tracking to `gtt_range` and exposed it through
  `poulsbo_gtt_state`.
- Added `scripts/run-vaio-sgx-object.sh` to validate object adoption, object
  persistence across userspace exit, and final explicit drop.
- Wrote the design and validation note in `docs/16-sgx-lab-objects.md`.

Validated result on `10.1.11.218`:

- the VAIO booted the new locally built module with
  `srcversion 92CA91A4FED5EB2EA73F40D`
- the new `poulsbo_sgx_object` and `poulsbo_sgx_objects` sysfs nodes exist on
  the live PCI device
- `run-vaio-sgx-object.sh` created a held dumb GEM object as `range id=14` at
  `gpu_va=0x80c77000`
- `adopt 14` created `object id=1`
- `pf_bind 1` changed `insert_pages` from `4` to `5`
- `pf_unbind 1` changed `remove_pages` from `1` to `2`
- after userspace exit, `range id=14` still existed and still showed
  `sgx_object_id=1`
- after `drop 1`, `remove_pages` changed from `2` to `3`, the range
  disappeared, and `poulsbo_sgx_objects` returned to zero objects
- target `dmesg` logged adopt, bind, unbind, and drop for the same object
- `poulsbo_sgx_state` after the run still reported `fault_count=0` and
  `bif_fault=0x000000`

Conclusion:

- the repo now has a kernel-owned SGX object lifetime, not just raw range-ID
  commands
- the next design step is no longer “can the kernel keep a render candidate
  alive?”; it is “what SGX-specific roles and submission structures should live
  on top of that object?”

### First SGX role metadata and submit descriptor validation

- Added SGX lab object role metadata:
  - `generic`
  - `pds`
  - `use`
  - `control`
  - `target`
- Added SGX lab object expected-context metadata:
  - `auto`
  - `default`
  - `pf`
- Added the first SGX lab submit control and submit-state views:
  - `poulsbo_sgx_submit`
  - `poulsbo_sgx_submits`
- Added a multi-object submit descriptor with `control`, `pds`, `use`, and
  `target` slots plus validation bookkeeping.
- Added `scripts/run-vaio-sgx-submit.sh` to exercise three live objects and one
  PF-context submit descriptor on the VAIO.
- Wrote the design and validation note in `docs/17-sgx-lab-submit-path.md`.

Validated result on `10.1.11.218`:

- the VAIO booted the new locally built module with
  `srcversion 9A0DCD9E393D70292202009`
- the new `poulsbo_sgx_submit` and `poulsbo_sgx_submits` sysfs nodes exist on
  the live PCI device
- `run-vaio-sgx-submit.sh` created three new live GEM ranges:
  - `range id=10`
  - `range id=11`
  - `range id=12`
- those ranges were adopted as `object id=1`, `object id=2`, and `object id=3`
- the three objects were assigned `pds`, `use`, and `target` roles and `pf`
  expected context
- PF-binding those objects moved MMU `insert_pages` from `5` to `8`
- a PF-context `submit id=1` was created and attached to object IDs `1`, `2`,
  and `3`
- the first validate returned `status=0`
- after all three userspace probes exited, the objects and submit still existed
  and the second validate also returned `status=0`
- after dropping the submit and all three objects, `poulsbo_sgx_submits` and
  `poulsbo_sgx_objects` returned to zero entries and MMU `remove_pages` moved
  from `0` to `6`
- `poulsbo_sgx_state` after the run still reported `fault_count=0` and
  `bif_fault=0x000000`

Conclusion:

- the repo now has the first SGX-oriented multi-object submission descriptor,
  even though it still does not execute SGX 3D work
- the next useful step is to add the smallest SGX-specific packet fields that
  matter for real submission experiments instead of more generic role-only
  scaffolding

## 2026-05-11

### First validate-only SGX kick packet on the VAIO

- Extended the SGX lab submit descriptor with the first SGX-shaped packet
  fields:
  - `cmd_type`
  - `ccb_offset`
  - `ctrl_flags`
  - `num_ta_status_vals`
  - `num_3d_status_vals`
  - `ta_dependency`
  - packet configuration timestamps and counters
- Added packet parsing for:
  - `ta`
  - `transfer`
  - `2d`
  - `power`
  - `cleanup`
  - `getmiscinfo`
  - `process_queues`
- Added the new submit control command:
  - `kick <submit_id> <cmd_type> <ccb_offset> <ctrl_flags> <num_ta_status> <num_3d_status> <ta_dependency>`
- Added validation rules for the current packet model:
  - explicit `cmd_type`
  - current validate support only for `ta`
  - 16-byte `ccb_offset` alignment
  - only the `READY` control flag accepted today
  - capped TA and 3D status counts
- Updated `scripts/run-vaio-sgx-submit.sh` so the helper configures the submit
  packet before validation.
- Wrote the design and validation note in `docs/18-sgx-lab-kick-packet.md`.

Validated result on `10.1.11.218`:

- the VAIO booted the new locally built module with
  `srcversion DB9A06B6BF23EF03033ED3E`
- the override module is loaded from
  `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- the updated helper still created three live GEM ranges:
  - `range id=10`
  - `range id=11`
  - `range id=12`
- those ranges were adopted as `object id=1`, `object id=2`, and `object id=3`
- the helper configured the submit packet as:
  - `cmd_type=ta`
  - `ccb_offset=0x20`
  - `ctrl_flags=0x1`
  - `num_ta_status_vals=1`
  - `num_3d_status_vals=1`
  - `ta_dependency=0`
- `poulsbo_sgx_submits` showed `packet_config_count=1`
- after slot attach, the first validate returned `status=0`
- after all three userspace probes exited, the second validate also returned
  `status=0`
- final teardown returned `poulsbo_sgx_submits` and `poulsbo_sgx_objects` to
  zero entries and moved MMU `remove_pages` from `0` to `6`
- `poulsbo_sgx_state` after the run still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`

Conclusion:

- the repo now has the first validate-only SGX-style kick packet on real
  Poulsbo hardware
- submit validation now depends on SGX-shaped packet state, not only generic
  object slots
- the next useful step is to add the first GPU-address and object-class
  semantics that will matter for a real SGX submission path

### First explicit object-class and GPU-address semantics on the VAIO

- Extended SGX lab objects with:
  - `object_class`
  - `gpu_use`
  - timestamps for the last class and GPU-use update
- Added object control commands:
  - `class <object_id> <generic|packet|code|state|surface>`
  - `gpu_use <object_id> <generic|control_ccb|pds_code|use_code|render_target>`
- Extended submit validation so current `ta` packets now require:
  - `control`, `pds`, `use`, and `target` objects
  - matching role per slot
  - matching object class per slot
  - matching intended GPU-use per slot
  - `ccb_offset + 16` to fit inside the attached control object
- Extended the submit-state view with resolved slot GPU virtual addresses:
  - `control_gpu_va`
  - `pds_gpu_va`
  - `use_gpu_va`
  - `target_gpu_va`
- Updated `scripts/run-vaio-sgx-submit.sh` to:
  - create four held GEM objects
  - assign `control/pds/use/target`
  - assign `packet/code/code/surface`
  - assign `control_ccb/pds_code/use_code/render_target`
  - adopt fresh range IDs by watermark instead of relying on the truncated
    one-page `poulsbo_gtt_state` diff
- Wrote the design and validation note in
  `docs/19-sgx-object-address-semantics.md`.

Validated result on `10.1.11.218`:

- the VAIO booted the new locally built module with
  `srcversion EF61C0BA3E7AC7F9B23DD47`
- the helper adopted four fresh live GEM ranges:
  - `range id=14`
  - `range id=15`
  - `range id=16`
  - `range id=17`
- those ranges became:
  - `object id=1`
  - `object id=2`
  - `object id=3`
  - `object id=4`
- after role/class/gpu-use/context assignment and PF bind:
  - `object 1 -> control / packet / control_ccb / pf`
  - `object 2 -> pds / code / pds_code / pf`
  - `object 3 -> use / code / use_code / pf`
  - `object 4 -> target / surface / render_target / pf`
- after attach and validate, the live submit state resolved:
  - `control_gpu_va=0x81127000`
  - `pds_gpu_va=0x8112b000`
  - `use_gpu_va=0x8112f000`
  - `target_gpu_va=0x81133000`
- both validates returned `status=0`
- after final teardown, `poulsbo_sgx_submits` and `poulsbo_sgx_objects`
  returned to zero entries and MMU `remove_pages` moved from `4` to `12`
- `poulsbo_sgx_state` after the run still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`

Conclusion:

- the repo now has the first explicit SGX object-class model on real hardware
- the repo now has the first intended GPU-address-use model on real hardware
- the kick packet now validates a concrete four-object control/pds/use/target
  topology instead of anonymous attached buffers
- the next useful step is to make the control object less symbolic and define
  the first CCB/control-block semantics inside it

### First non-symbolic SGX control-block model on the VAIO

- Extended the SGX lab submit descriptor with explicit CCB configuration state:
  - `ccb_write_offset`
  - `ccb_read_offset`
  - `ccb_service_address`
  - `ccb_cache_control`
  - `ccb_data0`
  - `ccb_data1`
  - configuration timestamps and counters
- Added control-object page read/write helpers in `psb_drv.c` so the submit
  path can write and read back CCB words through the live GEM-backed pages.
- Added the new submit control command:
  - `ccb <submit_id> <write_offset> <read_offset> <service> <cache> <data0> <data1>`
- Defined the first minimal in-object control layout:
  - header dwords at `0x0` and `0x4` for write/read offsets
  - four command dwords at `ccb_offset` for service, cache, and two payload
    words
- Tightened submit validation so current `ta` packets now also require:
  - configured CCB state
  - 4-byte aligned read and write offsets
  - header and payload words to fit inside the control object
  - live readback from the control object to match the submit snapshot
- Updated `scripts/run-vaio-sgx-submit.sh` to:
  - configure the new CCB state after packet setup
  - keep using range-ID watermark adoption so helper behavior does not depend
    on truncated one-page `poulsbo_gtt_state` output
- Wrote the design and validation note in
  `docs/20-sgx-control-block-model.md`.

Validated result on `10.1.11.218`:

- the VAIO booted the new locally built module with
  `srcversion A9D37B592F6EA8CB2A6BBA1`
- the helper adopted four fresh live GEM ranges:
  - `range id=10`
  - `range id=11`
  - `range id=12`
  - `range id=13`
- those ranges became:
  - `object id=1`
  - `object id=2`
  - `object id=3`
  - `object id=4`
- the helper configured the submit packet as before and then configured:
  - `ccb_write_offset=0x20`
  - `ccb_read_offset=0x0`
  - `ccb_service=0x1000`
  - `ccb_cache=0x0`
  - `ccb_data0=0xc0de0001`
  - `ccb_data1=0xc0de0002`
- the live submit state reported `ccb_config_count=1`
- both validates returned `status=0`, including after all userspace holders had
  exited
- final teardown returned `poulsbo_sgx_submits` and `poulsbo_sgx_objects` to
  zero entries and moved MMU `remove_pages` from `0` to `8`
- `poulsbo_sgx_state` after the run still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`
- target `dmesg` logged:
  - `lab submit ccb submit_id=1 write_offset=0x20 read_offset=0x0 service=0x1000 cache=0x0 data0=0xc0de0001 data1=0xc0de0002`
  - two successful `lab submit validate ... status=0`

Conclusion:

- the repo now has the first non-symbolic SGX control-block model on real
  hardware
- the control object now carries explicit header and payload words rather than
  only role metadata
- current validation depends on readback from live GEM-backed pages, not only
  on cached submit-side bookkeeping
- the next useful step is to decide which sync, status, or broader queue
  semantics should become explicit next before any real SGX execution attempt

### First explicit SGX status backing object on the VAIO

- Extended SGX lab roles and GPU-use semantics with:
  - `status`
  - `status_block`
- Extended the SGX lab submit descriptor with:
  - `status_object_id`
  - `status_ta_offset`
  - `status_3d_offset`
  - `status_stride`
  - `status_value_seed`
  - status configuration timestamps and counters
- Added the new submit control command:
  - `status <submit_id> <ta_offset> <3d_offset> <stride> <seed>`
- Tightened current `ta` validation so the status path now also requires:
  - a fifth `status` object when TA or 3D status counts are non-zero
  - role/class/GPU-use validation for that object
  - aligned offsets and stride
  - non-overlapping TA and 3D status regions
  - live readback of deterministic TA and 3D status words from the attached
    state object
- Updated `scripts/run-vaio-sgx-submit.sh` to:
  - hold five live dumb GEM objects
  - attach `control`, `pds`, `use`, `target`, and `status`
  - configure the new status block after the control block
- Wrote the design and validation note in
  `docs/21-sgx-status-block-model.md`.

Validated result on `10.1.11.218`:

- the VAIO booted the new locally built module with
  `srcversion 3482CBD7D1F81D9D4C6F81A`
- the loaded override module is
  `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- the helper adopted five fresh live GEM ranges:
  - `range id=14`
  - `range id=15`
  - `range id=16`
  - `range id=17`
  - `range id=18`
- those ranges became:
  - `object id=1`
  - `object id=2`
  - `object id=3`
  - `object id=4`
  - `object id=5`
- the helper configured the submit status block as:
  - `status_ta_offset=0x80`
  - `status_3d_offset=0xc0`
  - `status_stride=0x4`
  - `status_seed=0x5a500000`
- the live submit state resolved:
  - `status_object_id=5`
  - `status_gpu_va=0x80c87000`
  - `status_config_count=1`
- both validates returned `status=0`, including after all userspace holders had
  exited
- final teardown returned `poulsbo_sgx_submits` and `poulsbo_sgx_objects` to
  zero entries and moved:
  - MMU `insert_pages` from `8` to `13`
  - MMU `remove_pages` from `1` to `11`
- post-run SGX state still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`
- target `dmesg` logged:
  - `lab submit attach submit_id=1 slot=status object_id=5`
  - `lab submit status submit_id=1 ta_offset=0x80 3d_offset=0xc0 stride=0x4 seed=0x5a500000 ta_count=1 3d_count=1`
  - two successful `lab submit validate ... status=0`

Conclusion:

- the repo now has the first explicit SGX status backing object on real
  hardware
- the current validate-only submit model now covers separate control, code,
  target, and status memory roles
- the next useful step is no longer “make status backing explicit”; it is to
  extend queue progression, synchronization, and broader SGX state semantics

### First explicit SGX queue-progression model on the VAIO

- Extended the SGX lab submit descriptor with:
  - `queue_base_offset`
  - `queue_size`
  - `queue_packet_size`
  - `queue_packet_count`
  - queue configuration timestamps and counters
- Added the new submit control command:
  - `queue <submit_id> <base_offset> <size> <packet_size> <packet_count>`
- Defined the first minimal in-object queue layout:
  - dword `0x8`: `queue_base_offset`
  - dword `0xc`: `queue_size`
  - dword `0x10`: `queue_packet_size`
  - dword `0x14`: `queue_packet_count`
- Tightened current `ta` validation so the queue path now also requires:
  - configured queue state
  - aligned queue base and packet size
  - queue size to be a multiple of packet size
  - `ccb_offset == queue_base_offset + ccb_read_offset`
  - `ccb_write_offset == ccb_read_offset + queue_packet_size * queue_packet_count`
  - live readback of the queue metadata from the attached `control` object
- Updated `scripts/run-vaio-sgx-submit.sh` to:
  - configure the new queue block after the control block
  - print `SUBMITS_AFTER_QUEUE`
- Wrote the design and validation note in
  `docs/22-sgx-queue-model.md`.

Validated result on `10.1.11.218`:

- the VAIO booted the new locally built module with
  `srcversion 45E15DF2729D274214EAFB6`
- the loaded override module is
  `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- the helper adopted five fresh live GEM ranges:
  - `range id=10`
  - `range id=11`
  - `range id=12`
  - `range id=13`
  - `range id=14`
- those ranges became:
  - `object id=1`
  - `object id=2`
  - `object id=3`
  - `object id=4`
  - `object id=5`
- the helper configured the queue block as:
  - `queue_base_offset=0x20`
  - `queue_size=0x40`
  - `queue_packet_size=0x20`
  - `queue_packet_count=1`
- the live submit state reported:
  - `queue_config_count=1`
  - `queue_base_offset=0x20`
  - `queue_size=0x40`
  - `queue_packet_size=0x20`
  - `queue_packet_count=1`
- both validates returned `status=0`, including after all userspace holders had
  exited
- final teardown returned `poulsbo_sgx_submits` and `poulsbo_sgx_objects` to
  zero entries and moved:
  - MMU `insert_pages` from `7` to `12`
  - MMU `remove_pages` from `0` to `10`
- post-run SGX state still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`
- target `dmesg` logged:
  - `lab submit queue submit_id=1 base_offset=0x20 size=0x40 packet_size=0x20 packet_count=1 read_offset=0x0 write_offset=0x20`
  - two successful `lab submit validate ... status=0`

Conclusion:

- the repo now has the first explicit SGX queue-progression model on real
  hardware
- the current validate-only submit model now covers control, queue, packet, and
  status semantics across separate live objects
- the next useful step is to extend this non-wrapping queue model toward
  wrap-around, synchronization, or broader SGX queue ownership semantics

### First explicit SGX wrap-aware queue model on the VAIO

- Extended the SGX lab submit descriptor with:
  - `queue_tail_packet_count`
  - `queue_head_packet_count`
- Added queue progression derivation that:
  - enforces packet-aligned `ccb_read_offset`
  - derives wrapped `ccb_write_offset`
  - splits queue occupancy into tail and head packet counts
- Extended the in-object queue layout in the `control` object with:
  - dword `0x18`: `queue_tail_packet_count`
  - dword `0x1c`: `queue_head_packet_count`
- Tightened current `ta` validation so the queue path now also requires:
  - live readback of the new tail/head packet counts
  - derived write-offset agreement for wrapped or linear progression
  - packet-aligned queue pointer placement within the queue region
- Updated `scripts/run-vaio-sgx-submit.sh` to:
  - default to `QUEUE_MODE=wrap`
  - drive a wrapped queue case with `ccb_offset=0x80`
  - still support `QUEUE_MODE=nowrap` for the earlier linear path
- Wrote the design and validation note in
  `docs/23-sgx-queue-wrap-model.md`.

Validated wrap-mode result on `10.1.11.218`:

- the VAIO booted the new locally built module with
  `srcversion B88E97705620D63C25579DD`
- the loaded override module is
  `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- the helper ran in:
  - `QUEUE_MODE=wrap`
- the helper adopted five fresh live GEM ranges:
  - `range id=10`
  - `range id=11`
  - `range id=12`
  - `range id=13`
  - `range id=14`
- those ranges became:
  - `object id=1`
  - `object id=2`
  - `object id=3`
  - `object id=4`
  - `object id=5`
- the helper configured the queue block as:
  - `queue_base_offset=0x20`
  - `queue_size=0x80`
  - `queue_packet_size=0x20`
  - `queue_packet_count=2`
  - `queue_tail_packet_count=1`
  - `queue_head_packet_count=1`
  - `queue_wrap=1`
- the live submit state also showed:
  - `ccb_read_offset=0x60`
  - `ccb_write_offset=0x20`
  - `ccb_offset=0x80`
- both validates returned `status=0`, including after all userspace holders had
  exited
- final teardown returned `poulsbo_sgx_submits` and `poulsbo_sgx_objects` to
  zero entries and moved:
  - MMU `insert_pages` from `3` to `13`
  - MMU `remove_pages` from `1` to `11`
- a direct post-run SGX state read still reported:
  - `fault_count=0`
  - `bif_fault=0x000000`
- target `dmesg` logged:
  - `lab submit queue submit_id=1 base_offset=0x20 size=0x80 packet_size=0x20 packet_count=2 read_offset=0x60 write_offset=0x20 tail_packets=1 head_packets=1 wrapped=1`
  - two successful `lab submit validate ... status=0`

Validated nowrap regression result on `10.1.11.218`:

- the same helper also ran in:
  - `QUEUE_MODE=nowrap`
  - `HOLD_SECONDS=5`
- the linear path still validated with:
  - `queue_tail_packet_count=1`
  - `queue_head_packet_count=0`
  - `queue_wrap=0`
- both validates again returned `status=0`
- target `dmesg` logged:
  - `lab submit queue submit_id=2 base_offset=0x20 size=0x40 packet_size=0x20 packet_count=1 read_offset=0x0 write_offset=0x20 tail_packets=1 head_packets=0 wrapped=0`

Conclusion:

- the repo now has the first explicit wrap-aware SGX queue model on real
  hardware
- the queue path distinguishes tail and head occupancy without breaking the
  earlier linear case
- the next useful step is to add sync, completion, or queue-ownership semantics
  on top of this queue model rather than more basic queue geometry

### First explicit SGX sync block in the local tree

- Extended the SGX lab submit descriptor with:
  - `sync_offset`
  - `sync_complete_seq`
  - `sync_pending_seq`
  - `sync_flags`
  - `sync_config_count`
  - `last_sync_jiffies`
- Added a validate-only sync block in the existing `status` object with:
  - mirrored `ccb_read_offset`
  - mirrored `ccb_write_offset`
  - `sync_complete_seq`
  - `sync_pending_seq`
  - `sync_flags`
- Added the new submit control command:
  - `sync <submit_id> <offset> <complete_seq> <pending_seq> <flags>`
- Tightened validation so the sync block, if configured, now also requires:
  - a live `status` object
  - non-overlap with TA/3D status words
  - `READY` flag presence
  - wrapped-flag agreement with queue state
  - `pending_seq - complete_seq == queue_packet_count`
  - live readback of mirrored queue offsets and sync words
- Updated `scripts/run-vaio-sgx-submit.sh` to:
  - program the new sync block after `status`
  - default wrap mode to `sync_flags=0x3`
  - default linear mode to `sync_flags=0x1`
  - print `SUBMITS_AFTER_SYNC`
- Wrote the design note in:
  - `docs/24-sgx-sync-block-model.md`

Local validation result on May 11, 2026:

- the updated helper passed:
  - `bash -n scripts/run-vaio-sgx-submit.sh`
- the updated kernel tree passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- the local Ubuntu `5.4.0-150-generic` module build completed successfully
- the locally built module `srcversion` is:
  - `8B1DA24F8B1EFA8C47EFB32`

Target-side status on `10.1.11.218`:

- the new module was deployed
- a reboot was initiated
- after reboot, the target responded to ICMP and had TCP/22 open
- SSH transport/key exchange completed, but even
  `PreferredAuthentications=none` stalled before command execution
- but remote SSH command execution stalled, including direct helper attempts

Conclusion:

- the sync-block milestone is implemented and built locally
- the exported patch set is updated
- hardware validation is currently blocked on recovering normal remote command
  execution on the VAIO

### Canonical target-health classification for the VAIO stall

- Added a reusable target probe:
  - `scripts/check-vaio-target-health.sh`
- The script classifies:
  - ICMP reachability
  - TCP/22 reachability
  - SSH `none` authentication progress
  - SSH public-key command execution progress
  - SCP progress
- It reports one summary state such as:
  - `remote_usable`
  - `host_down`
  - `ssh_port_down`
  - `ssh_not_usable`
  - `ssh_stalled_post_kex`
- Documented the result and recovery path in:
  - `docs/25-vaio-ssh-stall-recovery.md`

Observed result on May 11, 2026:

- `target_host=10.1.11.218`
- `ping_status=ok`
- `tcp_22_status=open`
- `ssh_none_status=timeout_post_kex`
- `ssh_none_kex=1`
- `ssh_pubkey_status=timeout_post_kex`
- `ssh_pubkey_kex=1`
- `scp_status=timeout_post_kex`
- `overall=ssh_stalled_post_kex`

Conclusion:

- the VAIO stall is now classified reproducibly instead of by ad hoc command
  attempts
- the blocker remains target-side recovery, not more remote driver iteration

### Remote recovery, health-classifier fix, and guarded deploy

- Updated `scripts/check-vaio-target-health.sh` so `overall=remote_usable`
  reflects successful SSH command execution instead of failing the target
  purely because ICMP is inconsistent.
- Added:
  - `scripts/deploy-vaio-gma500-guarded.sh`
- Verified on May 11, 2026 that the guarded deploy flow could redeploy and
  reboot the VAIO while confirming the target returned to:
  - `ping_status=ok`
  - `tcp_22_status=open`
  - `ssh_none_status=auth_rejected`
  - `ssh_pubkey_status=ok`
  - `scp_status=ok`
  - `overall=remote_usable`

Conclusion:

- the earlier VAIO SSH stall is now a documented recovery mode, not the current
  blocker
- deploy-and-reboot cycles can now be guarded by an explicit target-health
  check instead of blind retries

### SGX sync block validated on hardware in both queue modes

- Re-ran `scripts/run-vaio-sgx-submit.sh` serially on May 11, 2026 after the
  target recovered.
- Verified wrap mode with:
  - `QUEUE_MODE=wrap`
  - `HOLD_SECONDS=5`
  - `queue_wrap=1`
  - `sync_offset=0x20`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x42`
  - `sync_flags=0x3`
  - `validate_count=2`
  - `last_validate_status=0`
- Verified linear mode with:
  - `QUEUE_MODE=nowrap`
  - `HOLD_SECONDS=5`
  - `queue_wrap=0`
  - `sync_offset=0x20`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x41`
  - `sync_flags=0x1`
  - `validate_count=2`
  - `last_validate_status=0`
- Confirmed the live target remained healthy after the final run:
  - loaded override module:
    `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
  - loaded module `srcversion`:
    `8B1DA24F8B1EFA8C47EFB32`
  - `lightdm=active`
  - `poulsbo_sgx_state`:
    - `fault_count=0`
    - `bif_fault=0x000000`
    - `capture_failed=0`

Conclusion:

- the sync-block milestone is no longer local-only
- both queue modes are now validated on the real VAIO target
- the next useful step is queue ownership or completion semantics, not more
  sync-block bring-up

### SGX submit helper hardening from the rerun failures

- Updated `scripts/run-vaio-sgx-submit.sh` to:
  - stage the remote probe binary under a unique temporary filename
  - drop stale SGX lab submits and objects before each run
  - fall back to a wider range-ID scan when the page-sized
    `poulsbo_gtt_state` dump truncates the visible fresh GEM ranges
- These fixes were driven by three real rerun failures on May 11, 2026:
  - remote binary install collision
  - stale adopted objects from an aborted run
  - truncated GTT-state visibility hiding a fresh fifth GEM range

Conclusion:

- the helper is now robust enough for repeat sync-block validation runs
- the failures were in the lab harness, not the SGX sync-block validation logic

### Explicit SGX owner block with completion cookies

- Extended the SGX lab submit descriptor with:
  - `owner_offset`
  - `owner_state`
  - `owner_issue_cookie`
  - `owner_complete_cookie`
  - `owner_config_count`
  - `last_owner_jiffies`
- Added a new submit control command:
  - `owner <submit_id> <offset> <owner_state> <issue_cookie> <complete_cookie>`
- Added a second validate-only block in the existing `status` object with:
  - one owner-state word
  - one issued-cookie word
  - one completed-cookie word
- Tightened validation so the owner block, if configured, now also requires:
  - a configured sync block
  - a live `status` object
  - non-overlap with the sync block
  - non-overlap with TA/3D status words
  - `issue_cookie >= complete_cookie`
  - `issue_cookie - complete_cookie == sync_pending_seq - sync_complete_seq`
  - `complete` owner state only when the outstanding count is zero
- Updated `scripts/run-vaio-sgx-submit.sh` to:
  - program the new owner block after `sync`
  - default wrap mode to `owner_state=running issue_cookie=0x102 complete_cookie=0x100`
  - default linear mode to `owner_state=running issue_cookie=0x101 complete_cookie=0x100`
  - print `SUBMITS_AFTER_OWNER`
- Wrote the design note in:
  - `docs/26-sgx-owner-block-model.md`

Validation result on May 11, 2026:

- rebuilt local VAIO module `srcversion`:
  - `29FE07980FD575A0704D90D`
- deployed with:
  - `scripts/deploy-vaio-gma500-guarded.sh`
- verified wrap mode with:
  - `owner_offset=0x40`
  - `owner_state=running`
  - `owner_issue_cookie=0x102`
  - `owner_complete_cookie=0x100`
  - `validate_count=2`
  - `last_validate_status=0`
- verified linear mode with:
  - `owner_offset=0x40`
  - `owner_state=running`
  - `owner_issue_cookie=0x101`
  - `owner_complete_cookie=0x100`
  - `validate_count=2`
  - `last_validate_status=0`
- confirmed post-run target health:
  - override module path:
    `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`

Conclusion:

- the submit model now has a separate memory-backed ownership/cookie block in
  addition to the sync block
- queue ownership semantics are now validated on the real VAIO target in both
  queue modes
- the next useful step is transition semantics across owner states, not another
  static block

### Explicit SGX transition model with queued/running/complete progression

- Extended the Ubuntu 5.4 `gma500` lab submit path with:
  - `transition <submit_id> <owner_state> <completed_packets>`
  - `transition_count`
  - `last_transition_jiffies`
- Added a validate-only transition state machine in
  `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c` that now:
  - supports `queued -> running -> complete`
  - supports partial retirement in `running`
  - allows `queue_packet_count=0` only through transition
  - advances queue read/write state, sync completion state, and owner
    completion-cookie state together
- Fixed the 32-bit VAIO build after the first implementation introduced a
  `u64` modulus and hit `__umoddi3`:
  - the live tree now uses `div_u64_rem()` in the queue-advance path
- Updated `scripts/run-vaio-sgx-submit.sh` to:
  - default to `TRANSITION_ENABLE=1`
  - default initial owner state to `queued` when transitions are enabled
  - drive wrap-mode `running 1 -> complete 1`
  - drive nowrap `running 0 -> complete 1`
  - print the submit state after each transition and validation phase
- Wrote the design note in:
  - `docs/27-sgx-transition-model.md`

Validation result on May 11, 2026:

- locally rebuilt VAIO module `srcversion`:
  - `0A250824D4E00B9BBB52C70`
- deployed with:
  - `scripts/deploy-vaio-gma500-guarded.sh`
- verified wrap mode with:
  - `QUEUE_MODE=wrap`
  - initial `owner_state=queued`
  - after `running 1`:
    - `queue_packet_count=1`
    - `sync_complete_seq=0x41`
    - `owner_complete_cookie=0x101`
  - after `complete 1`:
    - `queue_packet_count=0`
    - `sync_complete_seq=0x42`
    - `sync_pending_seq=0x42`
    - `owner_complete_cookie=0x102`
    - `transition_count=2`
    - `validate_count=4`
    - `last_validate_status=0`
- verified nowrap mode with:
  - `QUEUE_MODE=nowrap`
  - initial `owner_state=queued`
  - after `running 0`:
    - `queue_packet_count=1`
    - `sync_complete_seq=0x40`
    - `owner_complete_cookie=0x100`
  - after `complete 1`:
    - `queue_packet_count=0`
    - `sync_complete_seq=0x41`
    - `sync_pending_seq=0x41`
    - `owner_complete_cookie=0x101`
    - `transition_count=2`
    - `validate_count=4`
    - `last_validate_status=0`
- confirmed post-run target health:
  - override module path:
    `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`

Conclusion:

- the submit model now has a real validate-only state transition path instead
  of only static queue, sync, and owner metadata
- queue retirement is now visible through synchronized queue, sync, and owner
  updates on the real VAIO target
- the next useful step is issue-side progression, not more completion-side
  transition scaffolding

### Explicit SGX issue model with write-side queue growth

- Extended the Ubuntu 5.4 `gma500` lab submit path with:
  - `issue <submit_id> <owner_state> <issued_packets>`
  - `issue_count`
  - `last_issue_jiffies`
- Added a validate-only issue-side state machine in
  `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c` that now:
  - grows `queue_packet_count` without touching completion-side state
  - advances `ccb_write_offset`
  - advances `sync_pending_seq`
  - advances `owner_issue_cookie`
  - rewrites the dummy queue packet payload into each newly issued slot
  - keeps owner-state changes narrow: `queued -> queued`, `running -> running`,
    and `complete -> queued` only from an empty queue
- Updated `scripts/run-vaio-sgx-submit.sh` to:
  - default to `ISSUE_ENABLE=1`
  - start from a smaller queued workload
  - validate the initial queue state
  - issue one packet and validate again
  - then retire the queued work through the existing transition model
  - print `SUBMITS_AFTER_ISSUE` and `SUBMITS_AFTER_ISSUE_VALIDATE`
- Wrote the design note in:
  - `docs/28-sgx-issue-model.md`

Validation result on May 11, 2026:

- locally rebuilt VAIO module `srcversion`:
  - `2A252234DCE536B6C9CF9F6`
- deployed with:
  - `scripts/deploy-vaio-gma500-guarded.sh`
- verified wrap mode with:
  - initial queue:
    - `ccb_read_offset=0x60`
    - `ccb_write_offset=0x0`
    - `queue_packet_count=1`
    - `sync_pending_seq=0x41`
    - `owner_issue_cookie=0x101`
  - after `issue queued 1`:
    - `ccb_write_offset=0x20`
    - `queue_packet_count=2`
    - `queue_tail_packet_count=1`
    - `queue_head_packet_count=1`
    - `queue_wrap=1`
    - `sync_pending_seq=0x42`
    - `owner_issue_cookie=0x102`
    - `issue_count=1`
    - `validate_count=2`
    - `last_validate_status=0`
  - after later retirement:
    - `transition_count=2`
    - `validate_count=5`
    - `last_validate_status=0`
- verified nowrap mode with:
  - initial queue:
    - `ccb_read_offset=0x0`
    - `ccb_write_offset=0x20`
    - `queue_packet_count=1`
    - `sync_pending_seq=0x41`
    - `owner_issue_cookie=0x101`
  - after `issue queued 1`:
    - `ccb_write_offset=0x0`
    - `queue_packet_count=2`
    - `queue_tail_packet_count=2`
    - `queue_head_packet_count=0`
    - `queue_wrap=0`
    - `sync_pending_seq=0x42`
    - `owner_issue_cookie=0x102`
    - `issue_count=1`
    - `validate_count=2`
    - `last_validate_status=0`
  - after later retirement:
    - `transition_count=2`
    - `validate_count=5`
    - `last_validate_status=0`
- confirmed post-run target health:
  - override module path:
    `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `submits_shown=0`
  - `objects_shown=0`

Conclusion:

- the submit model now has explicit write-side queue growth in addition to
  explicit completion-side retirement
- queue growth and queue retirement are both hardware-validated on the real
  VAIO target
- the next useful step is repeated queue cycling, not more first-cycle issue
  scaffolding

### Repeated SGX queue cycling with explicit epoch tracking

- Extended the Ubuntu 5.4 `gma500` lab submit path again without adding a new
  command:
  - the existing `issue` path now detects refill from
    `owner_state=complete` with `queue_packet_count=0`
  - the submit descriptor now carries:
    - `queue_epoch`
    - `last_epoch_jiffies`
  - the refill path now:
    - increments `queue_epoch`
    - records `last_epoch_jiffies`
    - rejects `queue_epoch == U32_MAX` with `-EOVERFLOW`
  - `dmesg` now logs `epoch=` and `refill=` on each issue event
- Updated `scripts/run-vaio-sgx-submit.sh` to add a default second cycle with:
  - `REFILL_ENABLE=1`
  - `REFILL_ISSUE_STATE=queued`
  - `REFILL_ISSUED_PACKETS=1`
  - `REFILL_RUNNING_STATE=running`
  - `REFILL_RUNNING_COMPLETED_PACKETS=0`
  - `REFILL_COMPLETE_STATE=complete`
  - `REFILL_COMPLETE_COMPLETED_PACKETS=1`
  - new output sections:
    - `SUBMITS_AFTER_REFILL_ISSUE`
    - `SUBMITS_AFTER_REFILL_ISSUE_VALIDATE`
    - `SUBMITS_AFTER_REFILL_RUNNING`
    - `SUBMITS_AFTER_REFILL_RUNNING_VALIDATE`
    - `SUBMITS_AFTER_REFILL_COMPLETE`
    - `SUBMITS_AFTER_REFILL_COMPLETE_VALIDATE`
- Wrote the design note in:
  - `docs/29-sgx-queue-cycle-model.md`

Validation result on May 11, 2026:

- locally rebuilt VAIO module `srcversion`:
  - `7E750058A1E31F16F36953E`
- deployed with:
  - `scripts/deploy-vaio-gma500-guarded.sh`
- verified wrap-mode repeated cycling with:
  - first cycle refill-free issue:
    - `queue_epoch=0`
    - `issue_count=1`
    - `transition_count=2`
    - `validate_count=4` after first completion
  - second-cycle refill issue:
    - `queue_epoch=1`
    - `sync_pending_seq=0x43`
    - `owner_issue_cookie=0x103`
    - `issue_count=2`
    - `validate_count=5`
  - after second-cycle completion:
    - `queue_packet_count=0`
    - `sync_complete_seq=0x43`
    - `owner_complete_cookie=0x103`
    - `queue_epoch=1`
    - `transition_count=4`
    - `validate_count=7`
    - `last_validate_status=0`
  - final validate after probe exit:
    - `validate_count=8`
    - `last_validate_status=0`
- verified nowrap repeated cycling with:
  - first cycle issue:
    - `queue_epoch=0`
    - `issue_count=1`
  - second-cycle refill issue:
    - `queue_epoch=1`
    - `sync_pending_seq=0x43`
    - `owner_issue_cookie=0x103`
    - `issue_count=2`
    - `validate_count=5`
  - after second-cycle completion:
    - `queue_packet_count=0`
    - `sync_complete_seq=0x43`
    - `owner_complete_cookie=0x103`
    - `queue_epoch=1`
    - `transition_count=4`
    - `validate_count=7`
    - `last_validate_status=0`
  - final validate after probe exit:
    - `validate_count=8`
    - `last_validate_status=0`
- confirmed post-run target health:
  - loaded module `srcversion`:
    - `7E750058A1E31F16F36953E`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `submits_shown=0`
  - `objects_shown=0`

Conclusion:

- repeated `complete -> queued -> running -> complete` cycling on one
  persistent submit object is now hardware-validated
- the current `queue_epoch` model is sufficient for one refill epoch
- the next useful step is deciding whether broader generation semantics are
  needed across many refill epochs or many concurrent persistent submits

### Multi-epoch SGX queue cycling with epoch-coupled sync and owner blocks

- Extended the Ubuntu 5.4 `gma500` lab submit path again without adding a new
  command:
  - the sync block now mirrors `queue_epoch` at `sync_offset + 20`
  - the owner block now mirrors `queue_epoch` at `owner_offset + 12`
  - `validate` now reads back both epoch words and requires them to match the
    live submit `queue_epoch`
  - `sync`, `owner`, `issue`, and `transition` now all write the current epoch
    back into the status object
- Extended `scripts/run-vaio-sgx-submit.sh` with:
  - `REFILL_CYCLES=<n>`
  - numbered multi-cycle output sections when `REFILL_CYCLES > 1`
  - compatibility with the old unnumbered output when `REFILL_CYCLES=1`
- Wrote the design note in:
  - `docs/30-sgx-multi-epoch-cycle-model.md`

Validation result on May 11, 2026:

- locally rebuilt VAIO module `srcversion`:
  - `C94A92B9C66705DD4B23C07`
- deployed with:
  - `scripts/deploy-vaio-gma500-guarded.sh`
- verified wrap-mode multi-epoch cycling with:
  - `REFILL_CYCLES=3`
  - third refill issue:
    - `sync_complete_seq=0x44`
    - `sync_pending_seq=0x45`
    - `owner_issue_cookie=0x105`
    - `owner_complete_cookie=0x104`
    - `queue_epoch=3`
    - `issue_count=4`
    - `transition_count=6`
    - `validate_count=11`
  - third refill complete:
    - `queue_packet_count=0`
    - `sync_complete_seq=0x45`
    - `sync_pending_seq=0x45`
    - `owner_issue_cookie=0x105`
    - `owner_complete_cookie=0x105`
    - `queue_epoch=3`
    - `issue_count=4`
    - `transition_count=8`
    - `validate_count=13`
    - `last_validate_status=0`
  - final validate after probe exit:
    - `validate_count=14`
    - `last_validate_status=0`
- verified nowrap multi-epoch cycling with:
  - `REFILL_CYCLES=3`
  - third refill issue:
    - `sync_complete_seq=0x44`
    - `sync_pending_seq=0x45`
    - `owner_issue_cookie=0x105`
    - `owner_complete_cookie=0x104`
    - `queue_epoch=3`
    - `issue_count=4`
    - `transition_count=6`
    - `validate_count=11`
  - third refill complete:
    - `queue_packet_count=0`
    - `sync_complete_seq=0x45`
    - `sync_pending_seq=0x45`
    - `owner_issue_cookie=0x105`
    - `owner_complete_cookie=0x105`
    - `queue_epoch=3`
    - `issue_count=4`
    - `transition_count=8`
    - `validate_count=13`
    - `last_validate_status=0`
  - final validate after probe exit:
    - `validate_count=14`
    - `last_validate_status=0`
- confirmed `dmesg` epoch progression:
  - initial `sync` and `owner` setup logged `epoch=0`
  - later refill issues and transitions logged `epoch=1`, `epoch=2`, and
    `epoch=3`
- confirmed post-run target health:
  - loaded module `srcversion`:
    - `C94A92B9C66705DD4B23C07`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `submits_shown=0`
  - `objects_shown=0`

Conclusion:

- the status-backed sync and owner blocks are now explicitly epoch-coupled
- one persistent submit object is now hardware-validated across three refill
  epochs in both wrap and nowrap queue modes
- the next useful question is no longer “can multiple epochs work?” but
  whether the current single-submit epoch model is enough once multiple
  persistent submits or more SGX-shaped state are introduced

### Focused submit-state view and concurrent multi-submit isolation

- Extended the Ubuntu 5.4 `gma500` lab submit path with:
  - `focus <submit_id>`
  - `focus clear`
  - new sysfs view:
    - `poulsbo_sgx_submit_state`
  - a shared formatting path so both the global submit list and the focused
    state node print the same submit fields
- Added a new concurrent validator:
  - `scripts/run-vaio-sgx-multi-submit.sh`
  - it creates two submit objects with separate object sets, drives submit A
    and submit B independently, and uses the focused state node to confirm that
    one submit stays unchanged while the other progresses
- Wrote the design note in:
  - `docs/31-sgx-multi-submit-isolation.md`

Validation result on May 11, 2026:

- concurrent two-submit validation used override module `srcversion`:
  - `2716472E418F5F57F7D7D04`
- verified submit A setup state:
  - `QUEUE_MODE=wrap`
  - `queue_epoch=0`
  - `validate_count=1`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x41`
  - `owner_issue_cookie=0x101`
  - `owner_complete_cookie=0x100`
- verified submit B setup state:
  - `QUEUE_MODE=nowrap`
  - `queue_epoch=0`
  - `validate_count=1`
  - `sync_complete_seq=0x40`
  - `sync_pending_seq=0x41`
  - `owner_issue_cookie=0x101`
  - `owner_complete_cookie=0x100`
- drove submit A first with `REFILL_CYCLES=2` and verified final state:
  - `queue_epoch=2`
  - `issue_count=3`
  - `transition_count=6`
  - `validate_count=10`
  - `sync_complete_seq=0x44`
  - `sync_pending_seq=0x44`
  - `owner_issue_cookie=0x104`
  - `owner_complete_cookie=0x104`
- re-read submit B before it ran and confirmed it was still idle:
  - `queue_epoch=0`
  - `issue_count=0`
  - `transition_count=0`
  - `validate_count=1`
- drove submit B with `REFILL_CYCLES=1` and verified final state:
  - `queue_epoch=1`
  - `issue_count=2`
  - `transition_count=4`
  - `validate_count=7`
  - `sync_complete_seq=0x43`
  - `sync_pending_seq=0x43`
  - `owner_issue_cookie=0x103`
  - `owner_complete_cookie=0x103`
- re-read submit A after submit B finished and confirmed it was unchanged:
  - `queue_epoch=2`
  - `issue_count=3`
  - `transition_count=6`
  - `validate_count=10`
- after all userspace holders exited:
  - submit A:
    - `validate_count=11`
    - `last_validate_status=0`
  - submit B:
    - `validate_count=8`
    - `last_validate_status=0`
- confirmed post-run target health:
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `submits_shown=0`
  - `objects_shown=0`

Follow-up cleanup fix on May 11, 2026:

- found that `poulsbo_sgx_submit_state` still pointed at a missing ID after the
  focused submit was dropped
- fixed submit drop and unload paths so focus clears automatically
- validated on override module `srcversion`:
  - `DDC182564CFB437A4C27B84`
- minimal focused-submit lifecycle check:
  - create one submit
  - `focus <submit_id>`
  - confirm `state=present`
  - `drop <submit_id>`
  - confirm `focused_submit=none`

Conclusion:

- the repo now has a stable per-submit inspection path for concurrent work
- two persistent submits are now hardware-validated as isolated state machines
- the next useful question is whether any later render-facing state needs
  explicit global coordination semantics beyond the current per-submit epoch

### Global submit coordination counters and per-submit ticket mirroring

- Extended the Ubuntu 5.4 submit lab path with:
  - new sysfs view:
    - `poulsbo_sgx_coord_state`
  - new per-submit fields:
    - `global_issue_ticket`
    - `global_complete_ticket`
  - sync-block mirroring of the issue ticket
  - owner-block mirroring of the completion ticket
- Extended the concurrent validator:
  - `scripts/run-vaio-sgx-multi-submit.sh`
  - it now dumps `COORD_STATE_AFTER_SETUP`,
    `COORD_STATE_AFTER_SUBMIT_A`, `COORD_STATE_AFTER_SUBMIT_B`, and
    `COORD_STATE_AFTER_DROP`
- Wrote the design note in:
  - `docs/32-sgx-global-submit-coordination.md`

Validation result on May 11, 2026:

- the first hardware pass exposed one real validation bug:
  - the initial completion-ticket rule incorrectly treated any transition as if
    it required a completion ticket, which rejected the intermediate
    `running` state
  - fixed that logic and rebuilt locally
- final validated override module `srcversion`:
  - `1170524E4DF5B69FF27D8E2`
- verified global coordination progression:
  - after setup:
    - `issue_ticket_counter=0`
    - `complete_ticket_counter=0`
    - `last_issue_submit_id=0`
    - `last_complete_submit_id=0`
    - `live_submit_count=2`
  - after submit A:
    - `issue_ticket_counter=3`
    - `complete_ticket_counter=3`
    - `last_issue_submit_id=1`
    - `last_complete_submit_id=1`
  - after submit B:
    - `issue_ticket_counter=5`
    - `complete_ticket_counter=5`
    - `last_issue_submit_id=2`
    - `last_complete_submit_id=2`
  - after drop:
    - `issue_ticket_counter=5`
    - `complete_ticket_counter=5`
    - `focused_submit_valid=0`
    - `live_submit_count=0`
- verified per-submit coordination state:
  - submit B stayed idle while submit A ran:
    - `queue_epoch=0`
    - `issue_count=0`
    - `transition_count=0`
    - `global_issue_ticket=0`
    - `global_complete_ticket=0`
  - submit A final state:
    - `queue_epoch=2`
    - `issue_count=3`
    - `transition_count=6`
    - `global_issue_ticket=3`
    - `global_complete_ticket=3`
    - `validate_count=10`
  - submit B final state:
    - `queue_epoch=1`
    - `issue_count=2`
    - `transition_count=4`
    - `global_issue_ticket=5`
    - `global_complete_ticket=5`
    - `validate_count=7`
  - after userspace exit:
    - submit A:
      - `validate_count=11`
      - `last_validate_status=0`
    - submit B:
      - `validate_count=8`
      - `last_validate_status=0`
- confirmed post-run target health:
  - loaded module `srcversion`:
    - `1170524E4DF5B69FF27D8E2`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `issue_ticket_counter=5`
  - `complete_ticket_counter=5`
  - `submits_shown=0`
  - `focused_submit=none`
  - `objects_shown=0`

Conclusion:

- the repo now has explicit global issue-side and completion-side ordering
  state across persistent submits
- per-submit state remains isolated while the shared counters advance
  monotonically
- the next useful question is whether those two global lanes are enough for
  future fence-like or queue-ownership semantics

### Running-owner arbitration and exclusive active-lane validation

- Extended the Ubuntu 5.4 submit lab path with:
  - a third coordination lane for active `running` ownership
  - new coord-state fields:
    - `running_ticket_counter`
    - `last_running_submit_id`
    - `active_running_valid`
    - `active_running_submit_id`
    - `active_running_ticket`
  - new per-submit field:
    - `global_running_ticket`
  - owner-block mirroring of that running ticket
- Extended the multi-submit helper:
  - `scripts/run-vaio-sgx-multi-submit.sh`
  - it now drives submit A into `running`, attempts a conflicting
    `transition submit_b running 0`, requires that write to fail, then confirms
    submit B remained queued and idle before the normal submit B run
- Wrote the design note in:
  - `docs/33-sgx-running-owner-arbitration.md`

Validation result on May 11, 2026:

- final validated override module `srcversion`:
  - `39837C8555DA32AD66B21C9`
- verified setup coordination state:
  - `issue_ticket_counter=0`
  - `running_ticket_counter=0`
  - `complete_ticket_counter=0`
  - `active_running_valid=0`
  - `live_submit_count=2`
- verified arbitration while submit A was active:
  - `issue_ticket_counter=1`
  - `running_ticket_counter=1`
  - `complete_ticket_counter=0`
  - `last_issue_submit_id=1`
  - `last_running_submit_id=1`
  - `active_running_valid=1`
  - `active_running_submit_id=1`
  - `active_running_ticket=1`
- verified the conflicting submit B transition failed with:
  - `Device or resource busy`
- verified submit B stayed unchanged after that failed attempt:
  - `owner_state=queued`
  - `queue_epoch=0`
  - `issue_count=0`
  - `transition_count=0`
  - `global_issue_ticket=0`
  - `global_running_ticket=0`
  - `global_complete_ticket=0`
  - `validate_count=1`
- verified post-run coordination state:
  - after submit A:
    - `issue_ticket_counter=3`
    - `running_ticket_counter=3`
    - `complete_ticket_counter=3`
    - `last_issue_submit_id=1`
    - `last_running_submit_id=1`
    - `last_complete_submit_id=1`
    - `active_running_valid=0`
  - after submit B:
    - `issue_ticket_counter=5`
    - `running_ticket_counter=5`
    - `complete_ticket_counter=5`
    - `last_issue_submit_id=2`
    - `last_running_submit_id=2`
    - `last_complete_submit_id=2`
    - `active_running_valid=0`
- verified final per-submit state after userspace exit:
  - submit A:
    - `global_issue_ticket=3`
    - `global_running_ticket=3`
    - `global_complete_ticket=3`
    - `validate_count=11`
    - `last_validate_status=0`
  - submit B:
    - `global_issue_ticket=5`
    - `global_running_ticket=5`
    - `global_complete_ticket=5`
    - `validate_count=8`
    - `last_validate_status=0`
- confirmed post-run target health:
  - loaded module `srcversion`:
    - `39837C8555DA32AD66B21C9`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `running_ticket_counter=5`
  - `active_running_valid=0`
  - `submits_shown=0`
  - `focused_submit=none`
  - `objects_shown=0`

Conclusion:

- the repo now has explicit issue, running, and completion coordination lanes
- the running lane is now hardware-validated as exclusive across two persistent
  submits
- the next useful question is whether future dependency or fence-like behavior
  needs more than these three lanes

### Completion-ticket dependency gating through `ta_dependency`

- Extended the Ubuntu 5.4 submit lab path so:
  - `ta_dependency` now acts as a real validate-only completion-ticket gate on
    `queued -> running`
  - unmet dependency now returns `-EAGAIN`
  - active-owner contention still returns `-EBUSY`
- Extended the multi-submit helper:
  - `scripts/run-vaio-sgx-multi-submit.sh`
  - it now sets submit B dependency automatically from
    `SUBMIT_A_REFILL_CYCLES + 1`
  - it first verifies the dependency wait failure before submit A runs
  - it then still verifies running-owner contention while submit A is active
- Wrote the design note in:
  - `docs/34-sgx-completion-dependency-gating.md`

Validation result on May 11, 2026:

- final validated override module `srcversion`:
  - `CDDD68E97136F8541780DA8`
- verified submit B setup state:
  - `ta_dependency=3`
  - `owner_state=queued`
  - `queue_epoch=0`
  - `global_issue_ticket=0`
  - `global_running_ticket=0`
  - `global_complete_ticket=0`
  - `validate_count=1`
- verified dependency gating before submit A ran:
  - submit B `running` attempt failed with:
    - `Resource temporarily unavailable`
  - submit B remained unchanged
  - coord state remained:
    - `issue_ticket_counter=0`
    - `running_ticket_counter=0`
    - `complete_ticket_counter=0`
    - `active_running_valid=0`
- verified running-owner gating still worked while submit A was active:
  - submit B `running` attempt failed with:
    - `Device or resource busy`
  - coord state while submit A was active:
    - `issue_ticket_counter=1`
    - `running_ticket_counter=1`
    - `complete_ticket_counter=0`
    - `active_running_valid=1`
    - `active_running_submit_id=1`
    - `active_running_ticket=1`
- verified post-run state:
  - after submit A:
    - `issue_ticket_counter=3`
    - `running_ticket_counter=3`
    - `complete_ticket_counter=3`
  - after submit B:
    - `issue_ticket_counter=5`
    - `running_ticket_counter=5`
    - `complete_ticket_counter=5`
- verified final per-submit state after userspace exit:
  - submit A:
    - `ta_dependency=0`
    - `global_issue_ticket=3`
    - `global_running_ticket=3`
    - `global_complete_ticket=3`
    - `validate_count=11`
    - `last_validate_status=0`
  - submit B:
    - `ta_dependency=3`
    - `global_issue_ticket=5`
    - `global_running_ticket=5`
    - `global_complete_ticket=5`
    - `validate_count=8`
    - `last_validate_status=0`
- confirmed post-run target health:
  - loaded module `srcversion`:
    - `CDDD68E97136F8541780DA8`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `issue_ticket_counter=5`
  - `running_ticket_counter=5`
  - `complete_ticket_counter=5`
  - `active_running_valid=0`
  - `submits_shown=0`
  - `focused_submit=none`
  - `objects_shown=0`

Conclusion:

- the repo now has both dependency gating and owner-arbitration gating
  validated on the same concurrent submit flow
- `ta_dependency` is no longer just metadata; it now acts as a real wait gate
- the next useful question is whether one global completion-ticket dependency
  lane is enough for future fence-like semantics

### Source-submit dependency binding through `dependency_submit_id`

- Extended the Ubuntu 5.4 submit lab path so:
  - each submit now also carries `dependency_submit_id`
  - `depend <submit_id> <dependency_submit_id>` binds one submit to another
    live submit
  - `queued -> running` now checks the dependency submit's
    `global_complete_ticket` when both `ta_dependency` and
    `dependency_submit_id` are set
  - self-dependency and missing dependency submits are rejected
- Extended the multi-submit helper:
  - `scripts/run-vaio-sgx-multi-submit.sh`
  - it now configures submit B with both:
    - `ta_dependency=<expected completion ticket>`
    - `depend <submit_b_id> <submit_a_id>`
  - it now verifies that submit B reports the concrete dependency source while
    still hitting `-EAGAIN` before submit A completes and `-EBUSY` while
    submit A owns the running lane
- Wrote the design note in:
  - `docs/35-sgx-source-submit-dependency.md`

Validation result on May 11, 2026:

- final validated override module `srcversion`:
  - `05DE875B8F21073BDD04B44`
- verified submit B setup state:
  - `ta_dependency=3`
  - `dependency_submit_id=1`
  - `owner_state=queued`
  - `queue_epoch=0`
  - `global_issue_ticket=0`
  - `global_running_ticket=0`
  - `global_complete_ticket=0`
  - `validate_count=1`
- verified dependency gating before submit A ran:
  - submit B `running` attempt failed with:
    - `Resource temporarily unavailable`
  - submit B remained unchanged
  - `dependency_submit_id` still showed `1`
  - coord state remained:
    - `issue_ticket_counter=0`
    - `running_ticket_counter=0`
    - `complete_ticket_counter=0`
    - `active_running_valid=0`
- verified running-owner gating still worked while submit A was active:
  - submit B `running` attempt failed with:
    - `Device or resource busy`
  - submit B still showed:
    - `dependency_submit_id=1`
    - `owner_state=queued`
    - `global_issue_ticket=0`
    - `global_running_ticket=0`
    - `global_complete_ticket=0`
- verified post-run state:
  - submit A:
    - `ta_dependency=0`
    - `dependency_submit_id=0`
    - `global_issue_ticket=3`
    - `global_running_ticket=3`
    - `global_complete_ticket=3`
    - `validate_count=11`
    - `last_validate_status=0`
  - submit B:
    - `ta_dependency=3`
    - `dependency_submit_id=1`
    - `global_issue_ticket=5`
    - `global_running_ticket=5`
    - `global_complete_ticket=5`
    - `validate_count=8`
    - `last_validate_status=0`
- confirmed post-run target health:
  - loaded module `srcversion`:
    - `05DE875B8F21073BDD04B44`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `issue_ticket_counter=5`
  - `running_ticket_counter=5`
  - `complete_ticket_counter=5`
  - `active_running_valid=0`
  - `submits_shown=0`
  - `focused_submit=none`
  - `objects_shown=0`
  - target health:
    - `overall=remote_usable`

Conclusion:

- the repo now has explicit source-submit dependency binding on top of the
  earlier completion-ticket gate
- dependency waiting and active-owner contention are still validated as
  distinct `-EAGAIN` and `-EBUSY` paths on real hardware
- the next useful question is whether one dependency source plus one completion
  ticket is enough for later fence-like semantics

### Explicit dependency-state block through `depstate`

- Extended the Ubuntu 5.4 submit lab path so:
  - `depstate <submit_id> <offset>` now writes a four-dword dependency
    snapshot into the `status` object
  - the snapshot now carries:
    - source submit ID
    - required completion ticket
    - observed completion ticket
    - dependency flags
  - validation now reads back that block and checks it against the stored
    snapshot fields
- Extended the multi-submit helper:
  - `scripts/run-vaio-sgx-multi-submit.sh`
  - it now refreshes submit B's dependency block:
    - after setup
    - after the expected dependency wait failure
    - after submit A reaches the required completion ticket
  - it validates each refresh on hardware
- Wrote the design note in:
  - `docs/36-sgx-dependency-state-block.md`

Validation result on May 11, 2026:

- final validated override module `srcversion`:
  - `243EBC2DB6AC1F1149CA3AB`
- verified initial submit B dependency snapshot:
  - `ta_dependency=3`
  - `dependency_submit_id=1`
  - `dependency_offset=0x60`
  - `dependency_snapshot_source_submit_id=1`
  - `dependency_required_ticket=3`
  - `dependency_observed_complete_ticket=0`
  - `dependency_flags=0xb`
  - `dependency_config_count=1`
  - `validate_count=2`
  - `last_validate_status=0`
- verified dependency wait before submit A ran:
  - submit B `running` attempt failed with:
    - `Resource temporarily unavailable`
  - refreshed dependency block still showed:
    - `dependency_snapshot_source_submit_id=1`
    - `dependency_required_ticket=3`
    - `dependency_observed_complete_ticket=0`
    - `dependency_flags=0xb`
    - `dependency_config_count=2`
    - `validate_count=3`
    - `last_validate_status=0`
- verified dependency release after submit A completed:
  - refreshed dependency block then showed:
    - `dependency_snapshot_source_submit_id=1`
    - `dependency_required_ticket=3`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0xf`
    - `dependency_config_count=3`
    - `validate_count=4`
    - `last_validate_status=0`
  - submit B still remained queued at that point
- verified post-run state:
  - submit A:
    - `global_issue_ticket=3`
    - `global_running_ticket=3`
    - `global_complete_ticket=3`
    - `validate_count=11`
    - `last_validate_status=0`
  - submit B:
    - `dependency_snapshot_source_submit_id=1`
    - `dependency_required_ticket=3`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0xf`
    - `dependency_config_count=3`
    - `validate_count=11`
    - `last_validate_status=0`
- confirmed post-run target health:
  - loaded module `srcversion`:
    - `243EBC2DB6AC1F1149CA3AB`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `issue_ticket_counter=5`
  - `running_ticket_counter=5`
  - `complete_ticket_counter=5`
  - `active_running_valid=0`
  - `submits_shown=0`
  - `focused_submit=none`
  - `objects_shown=0`
  - target health:
    - `overall=remote_usable`

Conclusion:

- the repo now has an explicit dependency-state block in memory on top of the
  earlier source-submit dependency model
- the dependency block now shows the real unsatisfied-to-satisfied transition
  on hardware through `dependency_flags=0xb -> 0xf`
- the next useful question is whether explicit dependency snapshots are enough
  or whether later fence-like work needs a more live-updated dependency model

### Live dependency-state updates after one `depstate` setup

- Extended the Ubuntu 5.4 submit lab path so:
  - a configured dependency block now refreshes automatically when the source
    submit reaches `owner_state=complete`
  - `dependency_config_count` now stays stable after setup while the dependency
    snapshot can still change
  - `kick` and `depend` now also refuse obvious invalid dependency-block
    mutations earlier when a live dependency block already exists
- Extended the multi-submit helper:
  - `scripts/run-vaio-sgx-multi-submit.sh`
  - it now issues only one explicit `depstate` command for submit B
  - after that it only validates and inspects submit B
  - it proves the dependency block flips without another helper refresh
- Wrote the design note in:
  - `docs/37-sgx-live-dependency-block.md`

Validation result on May 11, 2026:

- final validated override module `srcversion`:
  - `6CA61C95B44598D425816B1`
- verified initial submit B dependency block after setup:
  - `ta_dependency=3`
  - `dependency_submit_id=1`
  - `dependency_offset=0x60`
  - `dependency_snapshot_source_submit_id=1`
  - `dependency_required_ticket=3`
  - `dependency_observed_complete_ticket=0`
  - `dependency_flags=0xb`
  - `dependency_config_count=1`
  - `validate_count=2`
  - `last_validate_status=0`
- verified dependency wait before submit A ran:
  - submit B `running` attempt failed with:
    - `Resource temporarily unavailable`
  - the helper then only ran `validate`
  - submit B still showed:
    - `dependency_snapshot_source_submit_id=1`
    - `dependency_required_ticket=3`
    - `dependency_observed_complete_ticket=0`
    - `dependency_flags=0xb`
    - `dependency_config_count=1`
    - `validate_count=3`
    - `last_validate_status=0`
- verified live dependency release after submit A completed:
  - the helper again only ran `validate`
  - submit B then showed:
    - `dependency_snapshot_source_submit_id=1`
    - `dependency_required_ticket=3`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0xf`
    - `dependency_config_count=1`
    - `validate_count=4`
    - `last_validate_status=0`
  - submit B still remained queued at that point
- verified post-run state:
  - submit A:
    - `global_issue_ticket=3`
    - `global_running_ticket=3`
    - `global_complete_ticket=3`
    - `validate_count=11`
    - `last_validate_status=0`
  - submit B:
    - `dependency_snapshot_source_submit_id=1`
    - `dependency_required_ticket=3`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0xf`
    - `dependency_config_count=1`
    - `validate_count=11`
    - `last_validate_status=0`
- confirmed post-run target health:
  - loaded module `srcversion`:
    - `6CA61C95B44598D425816B1`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `capture_failed=0`
  - `issue_ticket_counter=5`
  - `running_ticket_counter=5`
  - `complete_ticket_counter=5`
  - `active_running_valid=0`
  - `submits_shown=0`
  - `focused_submit=none`
  - `objects_shown=0`
  - target health:
    - `overall=remote_usable`

Conclusion:

- the repo now has a live-updated dependency block on top of the earlier
  explicit dependency snapshot model
- the dependency release is now driven by source-submit completion, not by a
  second helper-issued `depstate`
- the next useful question is whether one live dependency lane is enough for
  later fence-like semantics

### Dependency source-drop state after one `depstate` setup

- Extended the Ubuntu 5.4 submit lab path so:
  - dependency snapshots now carry
    `PSB_SGX_LAB_DEPENDENCY_FLAG_SOURCE_MISSING`
  - `drop <submit_id>` now refreshes dependent dependency blocks after the
    source submit leaves the live submit list
  - validate and dependency-gated `running` transitions now use that
    snapshot-based missing-source state when a dependency block already exists
- Extended the helper coverage:
  - `scripts/run-vaio-sgx-multi-submit.sh`
    now supports `DROP_SOURCE_BEFORE_RUN=1`
  - added
    `scripts/run-vaio-sgx-dependency-drop.sh`
    as the dedicated wrapper for the source-drop case
- Wrote the design note in:
  - `docs/38-sgx-dependency-source-drop-state.md`

Validation result on May 11, 2026:

- final validated override module `srcversion`:
  - `662C941AA2B250ACFFEDB1F`
- validated rerun submit IDs:
  - source submit:
    - `3`
  - dependent submit:
    - `4`
- verified initial dependent submit state after one `depstate` setup:
  - `ta_dependency=1`
  - `dependency_submit_id=3`
  - `dependency_offset=0x60`
  - `dependency_snapshot_source_submit_id=3`
  - `dependency_required_ticket=1`
  - `dependency_observed_complete_ticket=0`
  - `dependency_flags=0x0b`
  - `dependency_config_count=1`
  - `validate_count=2`
  - `last_validate_status=0`
- verified dependency wait before the source submit ran:
  - dependent `running` attempt failed with:
    - `Resource temporarily unavailable`
  - a plain `validate` still showed:
    - `dependency_snapshot_source_submit_id=3`
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=0`
    - `dependency_flags=0x0b`
    - `dependency_config_count=1`
    - `validate_count=3`
    - `last_validate_status=0`
- verified explicit missing-source state after dropping the source submit:
  - the dependent submit then showed:
    - `dependency_snapshot_source_submit_id=3`
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=0`
    - `dependency_flags=0x1b`
    - `dependency_config_count=1`
    - `validate_count=4`
    - `last_validate_status=0`
  - `0x1b` means:
    - ready + source-bound + gated + source-missing
- verified dependency wait after the source drop:
  - dependent `running` attempt again failed with:
    - `Resource temporarily unavailable`
  - a plain `validate` still showed:
    - `dependency_snapshot_source_submit_id=3`
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=0`
    - `dependency_flags=0x1b`
    - `dependency_config_count=1`
    - `validate_count=5`
    - `last_validate_status=0`
- verified final post-exit dependent state:
  - `dependency_snapshot_source_submit_id=3`
  - `dependency_required_ticket=1`
  - `dependency_observed_complete_ticket=0`
  - `dependency_flags=0x1b`
  - `dependency_config_count=1`
  - `validate_count=6`
  - `last_validate_status=0`
- confirmed post-run target health:
  - loaded module `srcversion`:
    - `662C941AA2B250ACFFEDB1F`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `active_running_valid=0`
  - `live_submit_count=0`
  - `submits_shown=0`
  - `objects_shown=0`
  - target health:
    - `overall=remote_usable`

Conclusion:

- the repo now has an explicit missing-source dependency state in addition to
  the earlier waiting and satisfied dependency states
- a dependent submit with a configured dependency block now stays on the
  dependency-gated path after the source submit is dropped, instead of falling
  back to a generic missing-submit failure
- the next useful dependency question is whether a previously satisfied source
  dependency should stay runnable after that source submit is dropped

### Satisfied source-drop state after dependency release

- Extended the helper coverage:
  - `scripts/run-vaio-sgx-multi-submit.sh`
    now supports `DROP_SOURCE_AFTER_RELEASE=1`
  - added
    `scripts/run-vaio-sgx-dependency-satisfied-drop.sh`
    as the dedicated wrapper for the satisfied-then-drop case
- Wrote the design note in:
  - `docs/39-sgx-satisfied-source-drop-state.md`

Validation result on May 11, 2026:

- validated override module `srcversion` remained:
  - `662C941AA2B250ACFFEDB1F`
- validated rerun submit IDs:
  - source submit:
    - `5`
  - dependent submit:
    - `6`
- verified satisfied dependency state before the source drop:
  - `dependency_snapshot_source_submit_id=5`
  - `dependency_required_ticket=1`
  - `dependency_observed_complete_ticket=1`
  - `dependency_flags=0x0f`
  - `dependency_config_count=1`
  - `validate_count=4`
  - `last_validate_status=0`
- verified satisfied-plus-missing state after dropping the source submit:
  - `dependency_snapshot_source_submit_id=5`
  - `dependency_required_ticket=1`
  - `dependency_observed_complete_ticket=1`
  - `dependency_flags=0x1f`
  - `dependency_config_count=1`
  - `validate_count=5`
  - `last_validate_status=0`
  - `0x1f` means:
    - ready + source-bound + gated + satisfied + source-missing
- verified that the dependent submit still ran normally under `0x1f`:
  - after `running`:
    - `owner_state=running`
    - `dependency_flags=0x1f`
    - `global_issue_ticket=2`
    - `global_running_ticket=2`
    - `validate_count=6`
  - after `complete`:
    - `owner_state=complete`
    - `dependency_flags=0x1f`
    - `global_issue_ticket=2`
    - `global_running_ticket=2`
    - `global_complete_ticket=2`
    - `validate_count=7`
  - after probe exit:
    - `owner_state=complete`
    - `dependency_flags=0x1f`
    - `queue_packet_count=0`
    - `validate_count=9`
    - `last_validate_status=0`
- verified coordination state:
  - after the satisfied source drop:
    - `issue_ticket_counter=1`
    - `running_ticket_counter=1`
    - `complete_ticket_counter=1`
    - `last_issue_submit_id=5`
    - `last_running_submit_id=5`
    - `last_complete_submit_id=5`
    - `active_running_valid=0`
    - `live_submit_count=1`
  - after submit B completed:
    - `issue_ticket_counter=2`
    - `running_ticket_counter=2`
    - `complete_ticket_counter=2`
    - `last_issue_submit_id=6`
    - `last_running_submit_id=6`
    - `last_complete_submit_id=6`
    - `active_running_valid=0`
    - `live_submit_count=1`
- confirmed post-run target health:
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `submits_shown=0`
  - `objects_shown=0`
  - target health:
    - `overall=remote_usable`

Conclusion:

- the repo now has a validated `satisfied + source-missing` dependency state in
  addition to the earlier waiting and satisfied states
- a dependent submit remains runnable when its source submit is dropped after
  already satisfying the gate
- the next useful dependency question is whether the observed completion ticket
  stays source-specific once unrelated submits complete after the source is
  gone

### Unrelated submit stability after satisfied source drop

- Extended the helper coverage:
  - `scripts/run-vaio-sgx-multi-submit.sh`
    now supports `RUN_UNRELATED_AFTER_SATISFIED_DROP=1`
  - added
    `scripts/run-vaio-sgx-dependency-unrelated-submit.sh`
    as the dedicated three-submit wrapper
- Hardened the helper path:
  - it now performs a bounded stale submit/object ID sweep before setup
  - it now derives adopted object IDs from the first adopted object plus the
    known sequential adopt order, instead of relying on a full
    `poulsbo_sgx_objects` dump once that view truncates
- Wrote the design note in:
  - `docs/40-sgx-unrelated-submit-stability.md`

Validation result on May 11, 2026:

- validated override module `srcversion` remained:
  - `662C941AA2B250ACFFEDB1F`
- validated rerun submit IDs:
  - source submit:
    - `7`
  - dependent submit:
    - `8`
  - unrelated submit:
    - `9`
- verified satisfied source-drop baseline:
  - after source completion:
    - `dependency_snapshot_source_submit_id=7`
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0x0f`
    - `validate_count=4`
  - after dropping that satisfied source:
    - `dependency_snapshot_source_submit_id=7`
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0x1f`
    - `validate_count=5`
- verified state after the unrelated submit completed:
  - coordination moved to:
    - `issue_ticket_counter=4`
    - `running_ticket_counter=4`
    - `complete_ticket_counter=4`
    - `last_issue_submit_id=9`
    - `last_running_submit_id=9`
    - `last_complete_submit_id=9`
  - the dependent submit still showed:
    - `dependency_snapshot_source_submit_id=7`
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0x1f`
    - `dependency_config_count=1`
    - `validate_count=6`
    - `last_validate_status=0`
- verified that the dependent submit still ran normally after that unrelated
  completion:
  - after `running`:
    - `owner_state=running`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0x1f`
    - `global_issue_ticket=5`
    - `global_running_ticket=5`
  - after `complete`:
    - `owner_state=complete`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0x1f`
    - `global_issue_ticket=5`
    - `global_running_ticket=5`
    - `global_complete_ticket=5`
  - after probe exit:
    - `owner_state=complete`
    - `dependency_observed_complete_ticket=3`
    - `dependency_flags=0x1f`
    - `queue_packet_count=0`
    - `validate_count=10`
    - `last_validate_status=0`
- confirmed post-run target health:
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`
  - `submits_shown=0`
  - `objects_shown=0`
  - target health:
    - `overall=remote_usable`

Conclusion:

- the satisfied-plus-missing dependency state is now proven to stay
  source-specific even after an unrelated submit completes
- the current dependency snapshot behaves like a pinned per-source fence state,
  not like a rolling alias of the global completion counter
- the next useful dependency question is whether one dependency lane is still
  enough once more than one source submit or more than one wait ticket is
  needed

### Multi-source dependency gating

- extended the kernel submit model in `psb_drv.c` with a second dependency lane
  plus the new sysfs commands:
  - `depend2`
  - `depstate2`
- extended `scripts/run-vaio-sgx-multi-submit.sh` with
  `RUN_SECONDARY_DEPENDENCY_SOURCE=1`
- added `scripts/run-vaio-sgx-multi-source-dependency.sh`
- wrote the milestone note:
  - `docs/41-sgx-multi-source-dependency-gating.md`
- built the updated VAIO-target module locally and deployed it with the guarded
  reboot flow
- validated on `10.1.11.218` with override module `srcversion`:
  - `F0B85EC0EA42E7360092BD1`
- validated three-submit topology:
  - source submit A:
    - `id=1`
  - dependent submit B:
    - `id=2`
  - source submit C:
    - `id=3`
- verified initial two-lane state on submit B:
  - primary lane:
    - `dependency_submit_id=1`
    - `dependency_observed_complete_ticket=0`
    - `dependency_flags=0x0b`
  - secondary lane:
    - `dependency2_submit_id=3`
    - `dependency2_observed_complete_ticket=0`
    - `dependency2_flags=0x0b`
  - `dependency_config_count=1`
  - `dependency2_config_count=1`
  - `validate_count=3`
- verified partial release after submit A completed:
  - primary lane advanced to:
    - `dependency_observed_complete_ticket=1`
    - `dependency_flags=0x0f`
  - secondary lane remained gated at:
    - `dependency2_observed_complete_ticket=0`
    - `dependency2_flags=0x0b`
  - submit B still failed `running` with:
    - `Resource temporarily unavailable`
  - post-wait validate still showed:
    - `validate_count=6`
    - `last_validate_status=0`
- verified full release after submit C completed:
  - primary lane stayed released at:
    - `dependency_observed_complete_ticket=1`
    - `dependency_flags=0x0f`
  - secondary lane advanced to:
    - `dependency2_observed_complete_ticket=2`
    - `dependency2_flags=0x0f`
  - submit B validate then showed:
    - `validate_count=7`
    - `last_validate_status=0`
- verified submit B then ran normally only after both lanes released:
  - after `running`:
    - `owner_state=running`
    - `global_issue_ticket=3`
    - `global_running_ticket=3`
    - `validate_count=8`
  - after `complete`:
    - `owner_state=complete`
    - `global_complete_ticket=3`
    - `queue_packet_count=0`
    - `validate_count=9`
  - after probe exit:
    - `dependency_observed_complete_ticket=1`
    - `dependency2_observed_complete_ticket=2`
    - `validate_count=11`
    - `last_validate_status=0`
- verified coordination progression:
  - after submit A:
    - `issue/running/complete = 1/1/1`
  - after submit C:
    - `issue/running/complete = 2/2/2`
  - after submit B:
    - `issue/running/complete = 3/3/3`
  - after cleanup:
    - `live_submit_count=0`
    - `focused_submit_valid=0`
- confirmed post-run target state:
  - `overall=remote_usable`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`

Conclusion:

- one dependent submit now validates against two concrete source-submit lanes
- release of one lane alone is not enough to allow `running`
- the next dependency question is per-lane ticket semantics, not whether the
  model can represent more than one source submit

### Per-lane ticket gating

- extended the kernel dependency model in `psb_drv.c` with a second lane ticket
  field plus the new sysfs command:
  - `depticket2`
- updated the dependency snapshot, validation, and gate logic so lane 2 can use
  its own required completion ticket instead of always inheriting
  `ta_dependency`
- extended `scripts/run-vaio-sgx-multi-submit.sh` with
  `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET`
- added `scripts/run-vaio-sgx-multi-source-ticket-dependency.sh`
- wrote the milestone note:
  - `docs/42-sgx-per-lane-ticket-gating.md`
- built the updated VAIO-target module locally and deployed it with the guarded
  reboot flow
- validated on `10.1.11.218` with override module `srcversion`:
  - `22E5E2738069346A4881DE7`
- validated three-submit topology:
  - source submit A:
    - `id=1`
  - dependent submit B:
    - `id=2`
  - source submit C:
    - `id=3`
- verified submit B's initial split thresholds after `depticket2` plus
  `depstate2`:
  - primary lane:
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=0`
    - `dependency_flags=0x0b`
  - secondary lane:
    - `dependency2_required_ticket=3`
    - `dependency2_observed_complete_ticket=0`
    - `dependency2_flags=0x0b`
  - `dependency_config_count=1`
  - `dependency2_config_count=1`
  - `validate_count=4`
  - `last_validate_status=0`
- verified partial release after submit A completed:
  - primary lane advanced to:
    - `dependency_observed_complete_ticket=1`
    - `dependency_flags=0x0f`
  - secondary lane stayed gated at:
    - `dependency2_observed_complete_ticket=0`
    - `dependency2_required_ticket=3`
    - `dependency2_flags=0x0b`
- verified the stricter lane-2 threshold after submit C completed once:
  - submit B then showed:
    - `dependency2_observed_complete_ticket=2`
    - `dependency2_required_ticket=3`
    - `dependency2_flags=0x0b`
  - submit B still failed `running` with:
    - `Resource temporarily unavailable`
  - post-wait validate still showed:
    - `validate_count=9`
    - `last_validate_status=0`
- verified full release after submit C completed twice:
  - primary lane stayed released at:
    - `dependency_observed_complete_ticket=1`
    - `dependency_flags=0x0f`
  - secondary lane advanced to:
    - `dependency2_observed_complete_ticket=3`
    - `dependency2_required_ticket=3`
    - `dependency2_flags=0x0f`
  - submit B validate then showed:
    - `validate_count=10`
    - `last_validate_status=0`
- verified submit B then ran normally only after both lanes released:
  - after `running`:
    - `owner_state=running`
    - `global_issue_ticket=4`
    - `global_running_ticket=4`
    - `validate_count=11`
  - after `complete`:
    - `owner_state=complete`
    - `global_complete_ticket=4`
    - `queue_packet_count=0`
    - `validate_count=12`
  - after probe exit:
    - `dependency_observed_complete_ticket=1`
    - `dependency2_observed_complete_ticket=3`
    - `dependency_flags=0x0f`
    - `dependency2_flags=0x0f`
    - `validate_count=14`
    - `last_validate_status=0`
- verified coordination progression:
  - after submit A:
    - `issue/running/complete = 1/1/1`
  - after submit C:
    - `issue/running/complete = 3/3/3`
  - after submit B:
    - `issue/running/complete = 4/4/4`
  - after cleanup:
    - `live_submit_count=0`
    - `focused_submit_valid=0`
- confirmed post-run target health:
  - `overall=remote_usable`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`

Conclusion:

- the dependency model now validates per-lane ticket semantics on real hardware
- one lane can be released while another still remains gated on the same
  dependent submit
- the next dependency question is whether two fixed lanes are enough, not
  whether lane-specific thresholds work

### Third dependency lane

- extended the kernel dependency model in `psb_drv.c` to three slots with the
  new sysfs commands:
  - `depend3`
  - `depticket3`
  - `depstate3`
- extended the focused submit state and validation path so lane 3 exposes its
  own source submit, required ticket, observed completion ticket, flags,
  config count, and timestamps
- extended `scripts/run-vaio-sgx-multi-submit.sh` with
  `RUN_TERTIARY_DEPENDENCY_SOURCE=1`
- added `scripts/run-vaio-sgx-third-dependency-lane.sh`
- wrote the milestone note:
  - `docs/43-sgx-third-dependency-lane.md`
- built the updated VAIO-target module locally and deployed it with the guarded
  reboot flow
- validated on `10.1.11.218` with override module `srcversion`:
  - `157E84B220F2E20EEF2CF58`
- validated four-submit topology:
  - source submit A:
    - `id=1`
  - dependent submit B:
    - `id=2`
  - source submit C:
    - `id=3`
  - source submit D:
    - `id=4`
- verified initial three-lane state on submit B:
  - primary lane:
    - `dependency_required_ticket=1`
    - `dependency_observed_complete_ticket=0`
    - `dependency_flags=0x0b`
  - secondary lane:
    - `dependency2_required_ticket=2`
    - `dependency2_observed_complete_ticket=0`
    - `dependency2_flags=0x0b`
  - tertiary lane:
    - `dependency3_required_ticket=3`
    - `dependency3_observed_complete_ticket=0`
    - `dependency3_flags=0x0b`
  - `dependency_config_count=1`
  - `dependency2_config_count=1`
  - `dependency3_config_count=1`
  - `validate_count=6`
  - `last_validate_status=0`
- verified partial release after submit A completed:
  - primary lane advanced to:
    - `dependency_observed_complete_ticket=1`
    - `dependency_flags=0x0f`
  - secondary and tertiary lanes remained gated at:
    - `dependency2_observed_complete_ticket=0`
    - `dependency2_flags=0x0b`
    - `dependency3_observed_complete_ticket=0`
    - `dependency3_flags=0x0b`
- verified partial release after submit C completed:
  - secondary lane advanced to:
    - `dependency2_observed_complete_ticket=2`
    - `dependency2_flags=0x0f`
  - tertiary lane remained gated at:
    - `dependency3_observed_complete_ticket=0`
    - `dependency3_required_ticket=3`
    - `dependency3_flags=0x0b`
  - submit B still failed `running` with:
    - `Resource temporarily unavailable`
  - post-wait validate still showed:
    - `validate_count=11`
    - `last_validate_status=0`
- verified full release after submit D completed:
  - tertiary lane advanced to:
    - `dependency3_observed_complete_ticket=3`
    - `dependency3_required_ticket=3`
    - `dependency3_flags=0x0f`
  - submit B validate then showed:
    - `validate_count=12`
    - `last_validate_status=0`
- verified submit B then ran normally only after all three lanes released:
  - after `running`:
    - `owner_state=running`
    - `global_issue_ticket=4`
    - `global_running_ticket=4`
    - `validate_count=13`
  - after `complete`:
    - `owner_state=complete`
    - `global_complete_ticket=4`
    - `queue_packet_count=0`
    - `validate_count=14`
  - after probe exit:
    - `dependency_observed_complete_ticket=1`
    - `dependency2_observed_complete_ticket=2`
    - `dependency3_observed_complete_ticket=3`
    - `dependency_flags=0x0f`
    - `dependency2_flags=0x0f`
    - `dependency3_flags=0x0f`
    - `validate_count=16`
    - `last_validate_status=0`
- verified coordination progression:
  - after submit A:
    - `issue/running/complete = 1/1/1`
  - after submit C:
    - `issue/running/complete = 2/2/2`
  - after submit D:
    - `issue/running/complete = 3/3/3`
  - after submit B:
    - `issue/running/complete = 4/4/4`
  - after cleanup:
    - `live_submit_count=0`
    - `focused_submit_valid=0`
- confirmed post-run target health:
  - `overall=remote_usable`
  - `lightdm=active`
  - `fault_count=0`
  - `bif_fault=0x000000`

Conclusion:

- the dependency model now validates three concrete source-submit lanes on one
  dependent submit
- the third lane can keep the dependent submit gated after the first two lanes
  are already released
- the next dependency question is whether further growth should remain
  fixed-lane or become a broader dependency array or fence block

### Indexed dependency-slot interface

- exposed the generic dependency-slot helpers already present in `psb_drv.c`
  through new sysfs parser commands:
  - `depend_slot`
  - `depticket_slot`
  - `depstate_slot`
- updated all SGX submit help text blocks in `psb_drv.c` so the indexed
  commands are discoverable on the live PCI sysfs nodes
- extended `scripts/run-vaio-sgx-multi-submit.sh` with
  `USE_INDEXED_DEPENDENCIES=1`
- added `scripts/run-vaio-sgx-indexed-dependency-array.sh`
- fixed one remote-runner bug by exporting the primary dependency ticket into
  the remote shell before using the new indexed commands
- wrote the milestone note:
  - `docs/44-sgx-indexed-dependency-interface.md`
- built the updated VAIO-target module locally and deployed it with the guarded
  reboot flow
- validated on `10.1.11.218` with override module `srcversion`:
  - `381C2A7C9AB93F0C3220005`
- verified the new wrapper reports:
  - `DEPENDENCY_INTERFACE=indexed`
- verified indexed slot setup on submit B:
  - after `depticket_slot 1`:
    - `dependency_submit_id=5`
    - `ta_dependency=1`
    - `dependency_required_ticket=0`
    - `dependency_flags=0x0`
    - `validate_count=2`
  - after `depstate_slot 1`:
    - `dependency_offset=0x60`
    - `dependency_snapshot_source_submit_id=5`
    - `dependency_required_ticket=1`
    - `dependency_flags=0x0b`
    - `validate_count=3`
  - after `depticket_slot 2` plus `depstate_slot 2`:
    - `dependency2_submit_id=7`
    - `dependency2_required_ticket=2`
    - `dependency2_flags=0x0b`
    - `validate_count=5`
  - after `depticket_slot 3` plus `depstate_slot 3`:
    - `dependency3_submit_id=8`
    - `dependency3_required_ticket=3`
    - `dependency3_flags=0x0b`
    - `validate_count=7`
- verified dependency waits still behave correctly through the indexed surface:
  - before submit A:
    - submit B `running` failed with:
      - `Resource temporarily unavailable`
  - after submit A:
    - `dependency_observed_complete_ticket=1`
    - `dependency_flags=0x0f`
    - `dependency2_flags=0x0b`
    - `dependency3_flags=0x0b`
    - `validate_count=9`
    - submit B still failed `running` with:
      - `Resource temporarily unavailable`
  - after submit C:
    - `dependency2_observed_complete_ticket=2`
    - `dependency2_flags=0x0f`
    - `dependency3_flags=0x0b`
    - `validate_count=11`
    - submit B still failed `running` with:
      - `Resource temporarily unavailable`
  - after submit D:
    - `dependency3_observed_complete_ticket=3`
    - `dependency3_flags=0x0f`
    - `validate_count=13`
    - `last_validate_status=0`
- verified submit B then completed cleanly after all three indexed slots
  released:
  - after probe exit:
    - `owner_state=complete`
    - `dependency_observed_complete_ticket=1`
    - `dependency2_observed_complete_ticket=2`
    - `dependency3_observed_complete_ticket=3`
    - `dependency_flags=0x0f`
    - `dependency2_flags=0x0f`
    - `dependency3_flags=0x0f`
    - `global_issue_ticket=4`
    - `global_running_ticket=4`
    - `global_complete_ticket=4`
    - `validate_count=17`
    - `last_validate_status=0`
- verified coordination progression stayed clean:
  - after submit A:
    - `issue/running/complete = 1/1/1`
  - after submit C:
    - `issue/running/complete = 2/2/2`
  - after submit D:
    - `issue/running/complete = 3/3/3`
  - after submit B:
    - `issue/running/complete = 4/4/4`
  - post-run target state:
    - `overall=remote_usable`
    - `fault_count=0`
    - `bif_fault=0x000000`
    - `active_running_valid=0`
    - `live_submit_count=0`

Conclusion:

- the three-lane dependency model is now validated through one indexed slot
  surface, not only through fixed-lane aliases
- the next dependency question is whether three indexed slots are enough or
  whether the lab path should move to a broader dependency array or fence block

### Fourth indexed dependency slot

- widened `PSB_SGX_LAB_DEPENDENCY_SLOTS` from `3` to `4` in `psb_drv.c`
- added fourth-slot submit fields and focused-state formatting for:
  - `dependency4_*`
  - `dependency_config_count_quaternary`
  - `last_dependency_quaternary_jiffies`
  - `last_dependency_block_quaternary_jiffies`
- extended the generic slot accessors, required-ticket logic, and validation
  path so slot `4` participates in the same loop-based machinery as slots
  `1..3`
- extended `scripts/run-vaio-sgx-multi-submit.sh` with:
  - `RUN_QUATERNARY_DEPENDENCY_SOURCE=1`
  - `SUBMIT_E_QUEUE_MODE`
  - `SUBMIT_E_REFILL_CYCLES`
  - `SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET`
  - `SUBMIT_B_DEPENDENCY4_OFFSET`
- required `USE_INDEXED_DEPENDENCIES=1` for the slot-4 path
- added `scripts/run-vaio-sgx-fourth-dependency-slot.sh`
- wrote the milestone note:
  - `docs/45-sgx-fourth-indexed-dependency-slot.md`
- built the updated VAIO-target module locally and deployed it with the guarded
  reboot flow
- validated on `10.1.11.218` with override module `srcversion`:
  - `9CDF8E2749FF727D47C0BA4`
- verified the new wrapper still reports:
  - `DEPENDENCY_INTERFACE=indexed`
- verified fourth-slot setup on submit B:
  - after `depticket_slot 4`:
    - `dependency4_submit_id=5`
    - `dependency4_offset=0x0`
    - `dependency4_required_ticket=0`
    - `dependency4_flags=0x0`
    - `validate_count=8`
  - after `depstate_slot 4`:
    - `dependency4_offset=0x100`
    - `dependency4_snapshot_source_submit_id=5`
    - `dependency4_required_ticket=4`
    - `dependency4_observed_complete_ticket=0`
    - `dependency4_flags=0x0b`
    - `dependency4_config_count=1`
    - `validate_count=9`
- verified slot-4 gating is real after the first three source submits already
  released:
  - after submit D:
    - `dependency_observed_complete_ticket=1`
    - `dependency2_observed_complete_ticket=2`
    - `dependency3_observed_complete_ticket=3`
    - `dependency4_observed_complete_ticket=0`
    - `dependency4_required_ticket=4`
    - `dependency4_flags=0x0b`
  - submit B still failed `running` with:
    - `Resource temporarily unavailable`
- verified full release only after submit E completed:
  - `dependency4_observed_complete_ticket=4`
  - `dependency4_flags=0x0f`
  - `validate_count=17`
  - `last_validate_status=0`
- verified submit B then completed cleanly after all four indexed slots
  released:
  - after probe exit:
    - `dependency_observed_complete_ticket=1`
    - `dependency2_observed_complete_ticket=2`
    - `dependency3_observed_complete_ticket=3`
    - `dependency4_observed_complete_ticket=4`
    - `dependency_flags=0x0f`
    - `dependency2_flags=0x0f`
    - `dependency3_flags=0x0f`
    - `dependency4_flags=0x0f`
    - `global_issue_ticket=5`
    - `global_running_ticket=5`
    - `global_complete_ticket=5`
    - `dependency4_config_count=1`
    - `validate_count=21`
    - `last_validate_status=0`
- verified source submit E also validated cleanly after probe exit:
  - `owner_state=complete`
  - `global_issue_ticket=4`
  - `global_running_ticket=4`
  - `global_complete_ticket=4`
  - `validate_count=5`
  - `last_validate_status=0`
- verified coordination progression stayed clean:
  - after submit A:
    - `issue/running/complete = 1/1/1`
  - after submit C:
    - `issue/running/complete = 2/2/2`
  - after submit D:
    - `issue/running/complete = 3/3/3`
  - after submit E:
    - `issue/running/complete = 4/4/4`
  - after submit B:
    - `issue/running/complete = 5/5/5`
  - post-run target state:
    - `overall=remote_usable`
    - `fault_count=0`
    - `bif_fault=0x000000`
    - `active_running_valid=0`
    - `live_submit_count=0`

Conclusion:

- the indexed dependency interface now scales past the old fixed-lane alias set
- slot `4` is validated on hardware without introducing a `depend4` alias
- the next dependency question is whether four indexed slots are enough or
  whether the lab path should move to a broader dependency array or fence block

### Indexed dependency-slot clear path, local build, and target-side blocker

- Added:
  - `depclear_slot <submit_id> <slot>` to the SGX submit sysfs parser in
    `psb_drv.c`
  - the dedicated wrapper
    `scripts/run-vaio-sgx-dependency-slot-clear.sh`
- Implemented a real indexed-slot clear path that:
  - zeroes the selected dependency block in the status object
  - clears the slot's bound source submit, required/observed ticket state,
    flags, config count, and timestamps
  - clears the slot-specific required-ticket source
  - refreshes the remaining active dependency blocks after the clear
- Extended `scripts/run-vaio-sgx-multi-submit.sh` with:
  - `CLEAR_DEPENDENCY4_AFTER_D=1`
  - `RUN_SUBMIT_E_AFTER_CLEAR=1`
- Local validation passed:
  - `bash -n scripts/run-vaio-sgx-multi-submit.sh`
  - `bash -n scripts/run-vaio-sgx-dependency-slot-clear.sh`
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
  - local Ubuntu 5.4 module build
- Current local module:
  - `build/devnull-vaio-gma500-src/gma500_gfx.ko`
  - `srcversion 2B42B80E462B9A3C0BAAC9B`
- Deployed the new module with the guarded flow, but the VAIO did not return to
  usable SSH after reboot
- Repeated health checks stayed at:
  - `ping_status=ok`
  - `tcp_22_status=closed`
  - `ssh_pubkey_status=connection_refused`
  - `overall=ssh_port_down`

Conclusion:

- the new `depclear_slot` path is implemented locally but not yet
  hardware-validated
- the next required step is VAIO recovery, not more unvalidated remote-driver
  iteration

### VAIO auto-resume helper for blocked validation

- Added:
  - `scripts/wait-vaio-target-ready.sh`
- The helper:
  - polls `scripts/check-vaio-target-health.sh`
  - waits for `overall=remote_usable`
  - optionally runs a follow-up command immediately after recovery
- Intended current use:
  - `./scripts/wait-vaio-target-ready.sh ./scripts/run-vaio-sgx-dependency-slot-clear.sh`
- Local validation passed:
  - `bash -n scripts/wait-vaio-target-ready.sh`
  - `MAX_POLLS=1 POLL_INTERVAL_SECS=1 ./scripts/wait-vaio-target-ready.sh`
    against the current blocked target state

Conclusion:

- while the VAIO remains at `overall=ssh_port_down`, the useful local
  improvement is recovery automation, not more unvalidated kernel changes

### Boot rollback guard for future risky deploys

- Added:
  - `scripts/arm-vaio-gma500-rollback.sh`
  - `scripts/disarm-vaio-gma500-rollback.sh`
- Updated:
  - `scripts/deploy-vaio-gma500-guarded.sh`
- The guarded deploy path now:
  - arms a target-side systemd rollback service before reboot
  - preserves the currently installed override as a rollback backup before
    replacing it
  - leaves that rollback armed until post-boot SSH health returns to
    `overall=remote_usable`
  - extends the post-reboot wait window when the rollback guard is enabled
  - keeps polling through intermediate bad states instead of bailing early on
    `ssh_stalled_post_kex` when an automatic rollback reboot may still recover
    the machine
- Local validation passed:
  - `bash -n scripts/arm-vaio-gma500-rollback.sh`
  - `bash -n scripts/disarm-vaio-gma500-rollback.sh`
  - `bash -n scripts/deploy-vaio-gma500-guarded.sh`

Conclusion:

- the next useful deployment improvement is now staged locally even though the
  current VAIO outage still prevents end-to-end validation
- rollback can now prefer the previous known-good override instead of always
  dropping back to the stock module

### Post-recovery VAIO state inspection helper

- Added:
  - `scripts/show-vaio-gma500-state.sh`
- Updated:
  - `scripts/deploy-vaio-gma500-guarded.sh`
- The helper reports:
  - active `gma500_gfx` modinfo path and `srcversion`
  - override-module presence
  - rollback-service enabled/active state
  - rollback pending-marker presence and fields
  - rollback backup-module presence
- The guarded deploy path now prints that state automatically after a healthy
  post-reboot return
- Local validation passed:
  - `bash -n scripts/show-vaio-gma500-state.sh`
  - `bash -n scripts/deploy-vaio-gma500-guarded.sh`

Conclusion:

- once the VAIO is back, one command now gives the real module plus rollback
  state instead of relying on scattered ad hoc remote checks

### Timestamped VAIO health watcher

- Added:
  - `scripts/watch-vaio-target-health.sh`
- The watcher:
  - polls `scripts/check-vaio-target-health.sh`
  - prints one timestamped compact summary line every poll
  - prints the full probe output on state transitions
  - supports `STOP_ON_OVERALL`, `MAX_POLLS`, `PRINT_FULL_EACH_POLL`, and
    `LOG_FILE`
- Local validation passed:
  - `bash -n scripts/watch-vaio-target-health.sh`
  - `MAX_POLLS=2 POLL_INTERVAL_SECS=1 ./scripts/watch-vaio-target-health.sh`
    against the current blocked target state

Conclusion:

- the next recovery window can now be observed as a real timeline instead of a
  few manual probe snapshots

### One-command VAIO recovery-session wrapper

- Added:
  - `scripts/run-vaio-recovery-session.sh`
- The wrapper:
  - creates a timestamped local session directory under `logs/vaio-recovery/`
  - records a small local session manifest
  - records a structured local session result
  - waits for `overall=remote_usable` through the timestamped watcher
  - captures the remote `gma500` and rollback-guard state
  - runs the pending `depclear_slot` validator
  - writes explicit skipped-state markers when the target never becomes
    remotely usable
- Local validation passed:
  - `bash -n scripts/run-vaio-recovery-session.sh`
  - `MAX_POLLS=1 POLL_INTERVAL_SECS=1 ./scripts/run-vaio-recovery-session.sh`
    against the current blocked target state

Conclusion:

- the next successful recovery window can now be captured as one coherent local
  session bundle instead of several loosely connected commands

### Guarded deploy session wrapper

- Added:
  - `scripts/run-vaio-guarded-deploy-session.sh`
- The wrapper:
  - creates a timestamped local session directory under `logs/vaio-deploy/`
  - records preflight target health
  - captures pre-deploy target state when usable
  - runs the guarded deploy flow
  - captures post-deploy target state on success
  - writes a structured session result file even when preflight is blocked
- Local validation passed:
  - `bash -n scripts/run-vaio-guarded-deploy-session.sh`
  - blocked-target execution against the current `overall=ssh_port_down` state

Conclusion:

- the first risky deploy after recovery can now be captured as one coherent
  local session bundle instead of a loose script transcript

### Cross-session summary helper

- Added:
  - `scripts/summarize-vaio-sessions.sh`
- The helper scans both:
  - `logs/vaio-recovery/`
  - `logs/vaio-deploy/`
- It prints a compact table with:
  - session kind
  - timestamp
  - session name
  - outcome
  - last known target state
  - local module `srcversion`
  - finish time
  - session directory
- Fixed one real bug in the first version:
  - the initial scan depth was too shallow, so it printed only the header
- Local validation passed:
  - `bash -n scripts/summarize-vaio-sessions.sh`
  - `MAX_SESSIONS=10 ./scripts/summarize-vaio-sessions.sh`

Conclusion:

- the saved recovery and deploy history can now be inspected from one command
  instead of opening each session bundle by hand

### Live `depclear_slot` hardware validation on the VAIO

- The VAIO returned to:
  - `overall=remote_usable`
  - `ping_status=ok`
  - `tcp_22_status=open`
  - `ssh_pubkey_status=ok`
- Ran the live recovery bundle:
  - `logs/vaio-recovery/20260511-065408-depclear-live`
- Session result:
  - `session_outcome=completed`
  - `watcher_rc=0`
  - `last_overall=remote_usable`
  - `state_rc=0`
  - `validation_rc=0`
- Validated module state on target:
  - kernel `5.4.0-150-generic`
  - `gma500_gfx` override `srcversion 2B42B80E462B9A3C0BAAC9B`
- Validated the intended indexed slot-clear flow on real hardware:
  - submit B initially carried dependency slots `1..4`
  - after A, C, and D completed, slot `4` remained the only unreleased lane
  - `depclear_slot submit_id=2 slot=4` executed on hardware
  - submit B then ran and completed before submit E
  - submit E ran afterward and completed separately
  - submit B still showed `dependency4_submit_id=0`,
    `dependency4_offset=0x0`, and `dependency4_flags=0x0` after E
- Documented the completed milestone in:
  - `docs/54-sgx-dependency-slot-clear-validation.md`

Conclusion:

- the indexed `depclear_slot` path is no longer “implemented locally but
  pending validation”; it is now a completed real-hardware milestone

### `show-vaio-gma500-state.sh` cleanup

- Fixed one real helper bug:
  - `show-vaio-gma500-state.sh` could print a stray standalone `unknown` line
    when `systemctl` returned a non-zero status with stdout
- The helper now normalizes empty/non-zero `systemctl` queries into the keyed
  `rollback_service_enabled=` and `rollback_service_active=` fields without
  emitting an extra raw line

### Live rollback-guard healthy-boot validation

- Ran the guarded deploy bundle:
  - `logs/vaio-deploy/20260511-065750-rollback-guard-live`
- Session result:
  - `session_outcome=completed`
  - `preflight_overall=remote_usable`
  - `pre_state_rc=0`
  - `deploy_rc=0`
  - `post_state_rc=0`
- Validated healthy-path rollback behavior on real hardware:
  - preflight required `overall=remote_usable`
  - the rollback guard armed before reboot
  - the currently installed override was preserved as
    `/var/lib/poulsbo-rollback/gma500_gfx.previous.ko`
  - the post-reboot wait window expanded to `360` seconds
  - the target returned to `overall=remote_usable`
  - the wrapper cleared the pending rollback marker
  - the target stayed on override `srcversion 2B42B80E462B9A3C0BAAC9B`
  - final post-state showed:
    - `rollback_service_enabled=enabled`
    - `rollback_service_active=inactive`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
- Documented the milestone in:
  - `docs/55-vaio-rollback-guard-healthy-boot-validation.md`

Conclusion:

- the rollback guard is now validated for the normal healthy deploy/reboot
  path; the remaining open question is the failure-triggered automatic restore
  path

### Live rollback-guard auto-restore validation

- Added the deliberate failure-path session wrapper:
  - `scripts/run-vaio-rollback-autorestore-session.sh`
- Used a stripped candidate override to make restore visible without changing
  module identity:
  - source size `366368`
  - candidate size `364748`
  - both with `srcversion 2B42B80E462B9A3C0BAAC9B`
- Simplified the generated rollback service to the form now validated on
  hardware:
  - no explicit `After=multi-user.target`
  - no explicit `Wants=multi-user.target`
  - only `WantedBy=multi-user.target`
- Ran the live auto-restore bundle:
  - `logs/vaio-rollback/20260511-070400-autorestore-live`
- Session result:
  - `session_outcome=completed`
  - `rollback_watch_rc=0`
  - `final_override_size=366368`
  - `final_pending_present=0`
  - `final_backup_present=1`
- Mid-state after the first healthy boot proved the stripped candidate was
  really live on the target:
  - `override_module_size=364748`
  - `rollback_pending_present=1`
  - `rollback_backup_size=366368`
- The rollback-watch timeline then showed the intended second reboot:
  - target stayed `overall=remote_usable` through `07:10:11`
  - target dropped to `overall=ssh_port_down` at `07:10:34` and `07:10:51`
  - target returned to `overall=remote_usable` at `07:11:10`
- The target-side journal confirmed the restore action:
  - service started at `07:08:55`
  - rollback helper restored
    `/var/lib/poulsbo-rollback/gma500_gfx.previous.ko` to
    `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko` at
    `07:09:55`
  - the target then rebooted automatically
- Final post-state after the restore reboot showed:
  - `override_module_size=366368`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

- Added a new memory-backed typed submit `descriptor` block above the existing
  `report + manifest + expanded 8-dword CCB` model.
  - kernel-side:
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
  - userspace and runner-side:
    - `workloads/psb_sgx_lab_submit_op.c`
    - `workloads/psb_sgx_lab_query.c`
    - `workloads/psb_sgx_lab_dump.c`
    - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
    - `scripts/run-vaio-sgx-multi-submit.sh`
  - the descriptor now snapshots:
    - stage and stage count
    - context and command type
    - owner state
    - issued/completed packet counts
    - dependency-active count
    - queue count and epoch
    - queue layout and `ccb/sync/owner/report/manifest` offsets
    - global issue/running/complete tickets
    - report flags
    - status offsets and seed
    - `control/pds/use/target/status` GPU-VA32 words
  - the first focused live rerun exposed a userspace selector bug:
    - `descriptor focus 0x200` returned `errno=2`
    - `submit_flags=0x0`
    - root cause:
      - `workloads/psb_sgx_lab_submit_op.c` did not list
        `DRM_PSB_SGX_LAB_SUBMIT_OP_DESCRIPTOR` in `submit_op_uses_selector()`
    - fix:
      - add the descriptor op to the selector-using table
      - rebuild workloads only
      - keep the already-deployed kernel module
  - final live validation passed on the VAIO:
    - focused typed bundle:
      - `logs/vaio-ioctl/20260511-230546-sgx-submit-descriptor-live/summary.log`
      - `SUBMIT_ID=4`
      - descriptor stage progression:
        - `none -> prepared -> armed -> emitted -> launched -> dispatched -> retired`
      - final counts:
        - `prepare=1`
        - `arm=1`
        - `emit=1`
        - `launch=1`
        - `dispatch=1`
        - `retired=1`
        - `issue=1`
        - `transition=2`
    - typed multi-submit bundle:
      - `logs/vaio-ioctl/20260511-230625-sgx-multi-submit-descriptor-live/summary.log`
      - `SUBMIT_A_ID=6`
      - `SUBMIT_B_ID=7`
      - dependency-gated evidence:
        - `submit_descriptor_dependency_active_count=1`
        - `submit_emit_dependency_active_count=1`
      - final counts:
        - submit A:
          - `arm=3`
          - `emit=3`
          - `launch=3`
          - `dispatch=3`
          - `retired=3`
          - `issue=3`
          - `transition=6`
        - submit B:
          - `arm=2`
          - `emit=2`
          - `launch=2`
          - `dispatch=2`
          - `retired=2`
          - `issue=2`
          - `transition=4`
      - final cleanup:
        - `coord_live_submit_count=0`
        - `gtt_state_range_sgx_object_count=0`
  - current target state after the reruns:
    - `overall=remote_usable`
    - `driver_srcversion=9E540439CC3313278F803B0`
    - `override_module_size=427328`
    - `rollback_pending_present=0`
  - `rollback_service_enabled=enabled`
  - `rollback_service_active=inactive`
- Documented the completed milestone in:
  - `docs/56-vaio-rollback-guard-autorestore-validation.md`

Conclusion:

- the rollback guard is now validated on real hardware in both directions:
  healthy confirmed boot and automatic previous-override restore

### Session summary helper rollback-srcversion follow-up

- Updated `scripts/summarize-vaio-sessions.sh` so session summaries now fall
  back through:
  - `local_module_srcversion`
  - `source_module_srcversion`
  - `final_driver_srcversion`
- This fixes the new `rollback_autorestore` bundles reporting
  `module_srcversion=unknown`
- Updated `docs/53-vaio-session-summary-helper.md` to note that the helper now
  covers rollback-autorestore bundles too

Conclusion:

- saved VAIO session history now reports a correct module `srcversion` across
  recovery, guarded-deploy, and rollback-autorestore session types

### SGX lab ioctl query surface

- Added the first bounded SGX lab ioctl surface in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Defined two read-only lab commands:
  - `DRM_IOCTL_PSB_SGX_LAB_QUERY_SUBMIT`
  - `DRM_IOCTL_PSB_SGX_LAB_QUERY_COORD`
- Kept the scope narrow:
  - query existing SGX lab submit state by explicit submit ID
  - query existing SGX lab submit state by focused-submit lookup
  - query existing global SGX lab coordination state
- Added the first typed userspace query workload in:
  - `workloads/psb_sgx_lab_query.c`
- Added the live target runner in:
  - `scripts/run-vaio-sgx-ioctl-query.sh`
- Fixed two real bring-up issues during userspace validation:
  - added missing `<fcntl.h>` include for `open(O_RDWR)` in
    `workloads/psb_sgx_lab_query.c`
  - added the same wider fallback range-ID adopt scan used by
    `run-vaio-sgx-submit.sh` so the ioctl runner does not fail when the
    one-page `poulsbo_gtt_state` dump truncates fresh GEM ranges
- Built locally and guarded-deployed the updated module:
  - target module `srcversion 0A70DFC7732D064F1658113`
  - override module size `368548`
- Ran live hardware validation through:
  - `logs/vaio-ioctl/20260511-072823-sgx-query-live`
- Verified on hardware:
  - submit query by explicit ID resolved submit `2`
  - submit query by focus resolved the same submit `2`
  - coord ioctl reported the expected live submit/object counts
  - base ioctl state matched the sysfs submit and coord views
  - after one `issue` step, ioctl state matched updated sysfs fields for:
    - `validate_count`
    - `issue_count`
    - `queue_packet_count`
    - `sync_pending_seq`
    - `owner_issue_cookie`
    - `global_issue_ticket`

Conclusion:

- SGX lab submit-state inspection is no longer sysfs-only
- the next interface question is narrower: keep mutation on sysfs or move one
  bounded write-side path into ioctl

### SGX lab submit-op ioctl surface

- Extended the shared lab header in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
- Added the first bounded write-side SGX lab ioctl:
  - `DRM_IOCTL_PSB_SGX_LAB_SUBMIT_OP`
- Kept the scope narrow in
  `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`:
  - `validate`
  - `issue`
  - explicit submit ID or focused-submit lookup
- Added the typed userspace submit-op probe in:
  - `workloads/psb_sgx_lab_submit_op.c`
- Added the live target runner in:
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- Built locally and guarded-deployed the updated module:
  - target module `srcversion 7ADE14FECA75AEE334A3D5C`
  - override module size `368948`
  - rollback backup size `368548`
- Ran live hardware validation through:
  - `logs/vaio-ioctl/20260511-074144-sgx-submit-op-live`
- Verified on hardware:
  - base submit state started at:
    - `validate_count=0`
    - `last_validate_status=-22`
    - `issue_count=0`
  - validate via focused-submit ioctl succeeded with:
    - `submit_op_ret=0`
    - `submit_resolved_id=1`
  - after validate, the live state moved to:
    - `validate_count=1`
    - `last_validate_status=0`
  - issue via explicit submit ID ioctl succeeded with:
    - `submit_op_ret=0`
    - `submit_resolved_id=1`
    - `submit_packet_count=1`
  - after issue, the live state moved to:
    - `queue_packet_count=2`
    - `sync_pending_seq=0x42`
    - `owner_issue_cookie=0x102`
    - `global_issue_ticket=1`
    - `issue_count=1`
  - the typed query ioctl matched the post-issue state, including:
    - `submit_issue_count=1`
    - `submit_queue_packet_count=2`
    - `submit_sync_pending_seq=0x42`
    - `submit_owner_issue_cookie=0x102`
    - `submit_global_issue_ticket=1`
  - the coordination ioctl matched the global state too:
    - `coord_issue_ticket=1`
    - `coord_issue_submit_id=1`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the SGX lab path now has both bounded typed query ioctls and a first bounded
  write-side ioctl for `validate` and `issue`
- the remaining interface question is narrower: whether broader submit mutation
  should stay on sysfs or move into similarly bounded lab ioctls

### SGX lab submit-op transition extension

- Extended the bounded write-side SGX lab submit-op ioctl in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added:
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_TRANSITION`
- Extended the typed userspace tool in:
  - `workloads/psb_sgx_lab_submit_op.c`
- Extended the live runner in:
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- Built locally and guarded-deployed the updated module:
  - target module `srcversion 19A66F470763F1CC9968B27`
  - override module size `369164`
  - rollback backup size `368948`
- Ran live hardware validation through:
  - `logs/vaio-ioctl/20260511-074937-sgx-transition-op-live`
- Verified on hardware:
  - validate via focus still succeeded
  - issue via explicit submit ID still succeeded
  - transition via focus into `running` succeeded with:
    - `submit_op_ret=0`
    - `submit_owner_state_name=running`
    - `submit_transition_count=1`
    - `submit_ccb_offset=0x40`
    - `submit_ccb_read_offset=0x20`
    - `submit_queue_packet_count=1`
    - `submit_global_running_ticket=1`
    - `coord_running_ticket=1`
    - `coord_active_running_valid=1`
  - transition via explicit submit ID into `complete` succeeded with:
    - `submit_op_ret=0`
    - `submit_owner_state_name=complete`
    - `submit_transition_count=2`
    - `submit_queue_packet_count=0`
    - `submit_sync_complete_seq=0x42`
    - `submit_owner_complete_cookie=0x102`
    - `submit_global_complete_ticket=1`
    - `coord_running_ticket=1`
    - `coord_complete_ticket=1`
    - `coord_active_running_valid=0`
  - the kernel log matched the same progression:
    - `lab submit transition ... state=running ... global_running_ticket=1`
    - `lab submit transition ... state=complete ... global_complete_ticket=1`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the bounded SGX lab submit-op ioctl now covers the full validated
  single-submit progression: `validate -> issue -> transition`
- the remaining interface question is no longer whether any write-side ioctl
  should exist; it is only which remaining validated mutations should move off
  sysfs

### SGX lab submit-op lifecycle extension

- Extended the bounded write-side SGX lab submit-op ioctl in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added lifecycle operations:
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_CREATE`
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_FOCUS`
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_FOCUS_CLEAR`
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_DROP`
- Extended the typed userspace tool in:
  - `workloads/psb_sgx_lab_submit_op.c`
- Extended the live runner in:
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- The first live attempt exposed a userspace parser bug:
  - `create` was parsed early but still fell through to `invalid op`
  - the failed development bundle is kept in:
    - `logs/vaio-ioctl/20260511-075831-sgx-lifecycle-op-live`
- Fixed the userspace parser and rebuilt locally:
  - local module build succeeded
  - 32-bit workload rebuild succeeded
- Guarded-deployed the updated module:
  - target module `srcversion 9F374F3350ECBF29F9A0DB0`
  - override module size `370400`
  - rollback backup size `369164`
- Ran final live hardware validation through:
  - `logs/vaio-ioctl/20260511-080143-sgx-lifecycle-op-live-retry`
- Verified on hardware:
  - create through the ioctl succeeded with:
    - `submit_op_ret=0`
    - `submit_resolved_id=1`
    - `submit_context_name=pf`
  - focus through explicit ID succeeded with:
    - `submit_op_ret=0`
    - `submit_selector=1`
  - base state after create and focus showed:
    - `coord_focus_valid=1`
    - `coord_focused_submit_id=1`
    - `coord_live_submit_count=1`
  - validate through focus still succeeded with:
    - `submit_validate_count=1`
  - issue through explicit submit ID still succeeded with:
    - `submit_issue_count=1`
    - `submit_global_issue_ticket=1`
    - `coord_issue_ticket=1`
  - transition through focus into `running` succeeded with:
    - `submit_owner_state_name=running`
    - `submit_transition_count=1`
    - `submit_global_running_ticket=1`
    - `coord_running_ticket=1`
    - `coord_active_running_valid=1`
  - transition through explicit ID into `complete` succeeded with:
    - `submit_owner_state_name=complete`
    - `submit_transition_count=2`
    - `submit_global_complete_ticket=1`
    - `coord_complete_ticket=1`
    - `coord_active_running_valid=0`
  - drop through focus succeeded with:
    - `submit_op_ret=0`
    - `submit_resolved_id=1`
  - post-drop state matched full teardown:
    - `focused_submit_valid=0`
    - `live_submit_count=0`
  - post-drop typed queries failed as expected:
    - `QUERY_FOCUS_AFTER_DROP_RC=3`
    - `QUERY_BY_ID_AFTER_DROP_RC=3`
    - `submit_ioctl_ret=-1 errno=2`
  - the kernel log matched the same lifecycle:
    - `lab submit create submit_id=1 context=pf`
    - `lab submit issue submit_id=1 ... global_issue_ticket=1`
    - `lab submit transition submit_id=1 state=running ... global_running_ticket=1`
    - `lab submit transition submit_id=1 state=complete ... global_complete_ticket=1`
    - `lab submit drop submit_id=1 context=pf`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the bounded SGX lab submit-op ioctl now covers the full validated
  single-submit lifecycle: `create -> focus -> validate -> issue ->
  transition -> drop`
- the remaining sysfs dependence is narrower:
  - object adoption
  - object attachment
  - queue and block setup
  - broader lab-state configuration

### SGX lab object-op ioctl surface

- Extended the shared lab header in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
- Added the bounded SGX lab object-op ioctl in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added object-side operations:
  - `DRM_PSB_SGX_LAB_OBJECT_OP_ADOPT`
  - `DRM_PSB_SGX_LAB_OBJECT_OP_DROP`
  - `DRM_PSB_SGX_LAB_OBJECT_OP_PF_BIND`
  - `DRM_PSB_SGX_LAB_OBJECT_OP_PF_UNBIND`
  - `DRM_PSB_SGX_LAB_OBJECT_OP_GPU_USE`
  - `DRM_PSB_SGX_LAB_OBJECT_OP_CONTEXT`
- Added the typed userspace helper in:
  - `workloads/psb_sgx_lab_object_op.c`
- Added the live target runner in:
  - `scripts/run-vaio-sgx-ioctl-object-op.sh`
- Built locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the updated module:
  - deploy bundle: `logs/vaio-ioctl/20260511-081603-sgx-object-op-live`
  - target module `srcversion 95EEBEFCF0AA1E3328782CE`
  - override module size `371516`
  - rollback backup size `370400`
- Two local wrapper fixes happened before the final clean bundle:
  - the first post-deploy run used a non-executable local script
  - the next run validated the hardware path but ended with a stray trailing
    `EOF` in the local wrapper
- Ran final clean hardware validation through:
  - `logs/vaio-ioctl/20260511-081924-sgx-object-op-live-retry2`
- Verified on hardware:
  - adopt through the ioctl succeeded with:
    - `object_resolved_id=2`
    - `object_range_id=11`
  - the adopted object started in the default state:
    - `gpu_use=generic`
    - `expected_context=auto`
    - `pf_bind_count=0`
  - metadata updates through the ioctl succeeded:
    - `gpu_use=control_ccb`
    - `expected_context=pf`
  - PF bind through the ioctl succeeded with:
    - `pf_bind_count=1`
    - `pf_bind_total=1`
    - `sgx_object_id=2`
    - MMU `flush=7 -> 8`
    - MMU `insert_pages=5 -> 6`
  - PF unbind through the ioctl succeeded with:
    - `pf_bind_count=0`
    - `pf_unbind_total=1`
    - MMU `flush=8 -> 9`
    - MMU `remove_pages=2 -> 3`
  - after userspace exit, the kernel-owned object still existed with:
    - `gpu_use=control_ccb`
    - `expected_context=pf`
    - `pf_bind_total=1`
    - `pf_unbind_total=1`
  - drop through the ioctl succeeded with:
    - `objects_shown=0`
    - MMU `flush=9 -> 10`
    - MMU `remove_pages=3 -> 4`
  - the kernel log matched the same lifecycle:
    - `lab object adopt object_id=2 range_id=11`
    - `lab object gpu_use object_id=2 gpu_use=control_ccb`
    - `lab object context object_id=2 context=pf`
    - `lab pf_bind id=11 ... total=1`
    - `lab pf_unbind id=11 ... total=1`
    - `lab object drop object_id=2 range_id=11`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the bounded SGX lab ioctl surface now covers object-side mutation as well as
  submit-side mutation
- the remaining sysfs dependence is narrower again:
  - object `role` and `class`
  - submit attachment
  - packet, queue, sync, and block setup

### SGX lab object role/class ioctl extension

- Extended the shared lab header in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
- Extended the bounded SGX lab object-op ioctl in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Extended the typed userspace helper in:
  - `workloads/psb_sgx_lab_object_op.c`
- Extended the live target runner in:
  - `scripts/run-vaio-sgx-ioctl-object-op.sh`
- Added object-side operations:
  - `DRM_PSB_SGX_LAB_OBJECT_OP_ROLE`
  - `DRM_PSB_SGX_LAB_OBJECT_OP_CLASS`
- Built locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the updated module and validated through:
  - `logs/vaio-ioctl/20260511-082821-sgx-object-meta-op-live`
  - target module `srcversion 5C538A0E1B6A6AC1CFBF24F`
  - override module size `371980`
  - rollback backup size `371516`
- Verified on hardware:
  - adopt through the ioctl still succeeded with:
    - `object_resolved_id=1`
    - `object_range_id=10`
  - the adopted object started in the default state:
    - `role=generic`
    - `class=generic`
    - `gpu_use=generic`
    - `expected_context=auto`
  - metadata updates through the ioctl succeeded:
    - `role=control`
    - `class=packet`
    - `gpu_use=control_ccb`
    - `expected_context=pf`
  - PF bind and PF unbind still succeeded on the same object:
    - `pf_bind_total=1`
    - `pf_unbind_total=1`
  - after userspace exit, the kernel-owned object still existed with:
    - `role=control`
    - `class=packet`
    - `gpu_use=control_ccb`
    - `expected_context=pf`
  - drop through the ioctl still succeeded with:
    - `objects_shown=0`
  - the kernel log matched the same lifecycle:
    - `lab object role object_id=1 role=control`
    - `lab object class object_id=1 class=packet`
    - `lab object drop object_id=1 range_id=10`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the bounded SGX lab object-op ioctl now covers the full validated object-side
  mutation path:
  - `adopt`
  - `role`
  - `class`
  - `gpu_use`
  - `context`
  - `pf_bind`
  - `pf_unbind`
  - `drop`
- the remaining sysfs dependence narrowed again:
  - submit attachment
  - queue and state-block setup

### SGX lab submit attach/detach ioctl extension

- Extended the shared lab header in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
- Extended the bounded SGX lab submit-op ioctl in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Extended the typed userspace helper in:
  - `workloads/psb_sgx_lab_submit_op.c`
- Extended the live target runner in:
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- Added submit-side operations:
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_ATTACH`
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_DETACH`
- Built locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the updated module and validated through:
  - `logs/vaio-ioctl/20260511-083705-sgx-submit-attach-op-live`
  - target module `srcversion AADEFBD92B8B3E07E7907A0`
  - override module size `372736`
  - rollback backup size `371980`
- Verified on hardware:
  - typed slot attachment succeeded with:
    - `attach 1 control 1`
    - `attach 1 pds 2`
    - `attach 1 use 3`
    - `attach 1 target 4`
    - `attach focus status 5`
  - typed query after attachment showed:
    - `submit_control_object_id=1`
    - `submit_pds_object_id=2`
    - `submit_use_object_id=3`
    - `submit_target_object_id=4`
    - `submit_status_object_id=5`
  - typed detach through focus succeeded with:
    - `submit_slot_name=status`
    - `submit_resolved_id=1`
  - typed query after detach showed:
    - `submit_status_object_id=0`
  - typed reattach by explicit submit ID succeeded with:
    - `submit_slot_name=status`
    - `submit_object_id=5`
  - typed query after reattach showed:
    - `submit_status_object_id=5`
  - the existing validated submit progression still worked on top of the new
    typed attachment path:
    - `validate_count=1`
    - `issue_count=1`
    - `transition_count=2`
    - `global_issue_ticket=1`
    - `global_running_ticket=1`
    - `global_complete_ticket=1`
  - post-drop typed queries still failed as expected:
    - `QUERY_FOCUS_AFTER_DROP_RC=3`
    - `QUERY_BY_ID_AFTER_DROP_RC=3`
  - the kernel log matched the same attachment lifecycle:
    - `lab submit attach submit_id=1 slot=control object_id=1`
    - `lab submit detach submit_id=1 slot=status`
    - `lab submit attach submit_id=1 slot=status object_id=5`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the bounded SGX lab submit-op ioctl now covers the validated submit-side
  mutation path:
  - `create`
  - `focus`
  - `attach`
  - `detach`
  - `validate`
  - `issue`
  - `transition`
  - `drop`
- the remaining sysfs dependence inside the current SGX lab flow is narrower:
  - `kick`
  - `ccb`
  - `queue`
  - `status`
  - `sync`
  - `owner`
  - later dependency and broader block setup

### SGX lab submit kick ioctl extension

- Extended the shared lab header in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
- Extended the bounded SGX lab submit-op ioctl in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Extended the typed userspace helper in:
  - `workloads/psb_sgx_lab_submit_op.c`
- Extended the live target runner in:
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- Added the submit-side operation:
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_KICK`
- Built locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the updated module and validated through:
  - `logs/vaio-ioctl/20260511-084613-sgx-submit-kick-op-live`
  - target module `srcversion C9FFE7995A1210A0D10781A`
  - override module size `373572`
  - rollback backup size `372736`
- Verified on hardware:
  - typed kick through focused-submit lookup succeeded with:
    - `submit_cmd_type_name=ta`
    - `submit_ccb_offset=0x20`
    - `submit_ctrl_flags=0x1`
    - `submit_num_ta_status_vals=1`
    - `submit_num_3d_status_vals=1`
    - `submit_ta_dependency=0`
  - the typed query after kick showed the same live submit fields:
    - `submit_cmd_type_name=ta`
    - `submit_ccb_offset=0x20`
    - `submit_ctrl_flags=0x1`
    - `submit_num_ta_status_vals=1`
    - `submit_num_3d_status_vals=1`
    - `submit_ta_dependency=0`
    - `submit_status_object_id=5`
  - the existing validated submit progression still worked on top of the new
    typed kick path:
    - `validate_count=1`
    - `issue_count=1`
    - `transition_count=2`
    - final `owner_state=complete`
    - final `global_complete_ticket=1`
  - post-drop typed queries still failed as expected:
    - `QUERY_FOCUS_AFTER_DROP_RC=3`
    - `QUERY_BY_ID_AFTER_DROP_RC=3`
  - the kernel log matched the same packet-setup progression:
    - `lab submit kick submit_id=1 cmd=ta ccb_offset=0x20 ctrl_flags=0x1 ta_status=1 3d_status=1 ta_dependency=0`
    - `lab submit validate submit_id=1 context=pf status=0 count=1`
    - `lab submit issue submit_id=1 ... global_issue_ticket=1`
    - `lab submit transition submit_id=1 state=running ... global_running_ticket=1`
    - `lab submit transition submit_id=1 state=complete ... global_complete_ticket=1`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the bounded SGX lab submit-op ioctl now covers the validated submit-side
  mutation path:
  - `create`
  - `focus`
  - `attach`
  - `detach`
  - `kick`
  - `validate`
  - `issue`
  - `transition`
  - `drop`
- the remaining sysfs dependence inside the current SGX lab flow is narrower:
  - `ccb`
  - `queue`
  - `status`
  - `sync`
  - `owner`
  - later dependency and broader block setup

### SGX lab submit ccb ioctl extension

- Extended the shared lab header in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
- Extended the bounded SGX lab submit-op ioctl in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Extended the typed userspace helper in:
  - `workloads/psb_sgx_lab_submit_op.c`
- Extended the live target runner in:
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- Added the submit-side operation:
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_CCB`
- Built locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the updated module and validated through:
  - `logs/vaio-ioctl/20260511-085320-sgx-submit-ccb-op-live`
  - target module `srcversion 27FE36C613637398E2288FE`
  - override module size `374484`
  - rollback backup size `373572`
- Verified on hardware:
  - typed `ccb` through focused-submit lookup succeeded with:
    - `submit_ccb_write_offset=0x20`
    - `submit_ccb_read_offset=0x0`
    - `submit_ccb_service_address=0x1000`
    - `submit_ccb_cache_control=0x0`
    - `submit_ccb_data0=0xc0de0001`
    - `submit_ccb_data1=0xc0de0002`
  - the typed query after `ccb` showed the same live submit fields:
    - `submit_ccb_write_offset=0x20`
    - `submit_ccb_read_offset=0x0`
    - `submit_ccb_service_address=0x1000`
    - `submit_ccb_cache_control=0x0`
    - `submit_ccb_data0=0xc0de0001`
    - `submit_ccb_data1=0xc0de0002`
    - `submit_status_object_id=5`
  - the existing validated submit progression still worked on top of the new
    typed `ccb` path:
    - `validate_count=1`
    - `issue_count=1`
    - `transition_count=2`
    - final `owner_state=complete`
    - final `global_complete_ticket=1`
  - post-drop typed queries still failed as expected:
    - `QUERY_FOCUS_AFTER_DROP_RC=3`
    - `QUERY_BY_ID_AFTER_DROP_RC=3`
  - the kernel log matched the same CCB progression:
    - `lab submit ccb submit_id=1 write_offset=0x20 read_offset=0x0 service=0x1000 cache=0x0 data0=0xc0de0001 data1=0xc0de0002`
    - `lab submit validate submit_id=1 context=pf status=0 count=1`
    - `lab submit issue submit_id=1 ... global_issue_ticket=1`
    - `lab submit transition submit_id=1 state=running ... global_running_ticket=1`
    - `lab submit transition submit_id=1 state=complete ... global_complete_ticket=1`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the bounded SGX lab submit-op ioctl now covers the validated submit-side
  mutation path:
  - `create`
  - `focus`
  - `attach`
  - `detach`
  - `kick`
  - `ccb`
  - `validate`
  - `issue`
  - `transition`
  - `drop`
- the remaining sysfs dependence inside the current SGX lab flow is narrower:
  - `queue`
  - `status`
  - `sync`
  - `owner`
  - later dependency and broader block setup

### SGX lab submit queue ioctl extension

- Extended the shared lab header in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
- Extended the bounded SGX lab submit-op ioctl in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Extended the typed userspace helper in:
  - `workloads/psb_sgx_lab_submit_op.c`
- Extended the live target runner in:
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- Added the submit-side operation:
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_QUEUE`
- Built locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the updated module and validated through:
  - `logs/vaio-ioctl/20260511-090033-sgx-submit-queue-op-live`
  - target module `srcversion 61D418B9197433168D7124F`
  - override module size `375144`
  - rollback backup size `374484`
- Verified on hardware:
  - typed `queue` through focused-submit lookup succeeded with:
    - `submit_queue_base_offset=0x20`
    - `submit_queue_size=0x40`
    - `submit_queue_packet_size=0x20`
    - `submit_queue_packet_count=1`
  - the typed query after `queue` showed the same live submit fields:
    - `submit_queue_base_offset=0x20`
    - `submit_queue_size=0x40`
    - `submit_queue_packet_size=0x20`
    - `submit_queue_packet_count=1`
    - `submit_status_object_id=5`
  - the existing validated submit progression still worked on top of the new
    typed `queue` path:
    - `validate_count=1`
    - `issue_count=1`
    - `transition_count=2`
    - final `owner_state=complete`
    - final `global_complete_ticket=1`
  - post-drop typed queries still failed as expected:
    - `QUERY_FOCUS_AFTER_DROP_RC=3`
    - `QUERY_BY_ID_AFTER_DROP_RC=3`
  - the kernel log matched the same queue progression:
    - `lab submit queue submit_id=1 base_offset=0x20 size=0x40 packet_size=0x20 packet_count=1 read_offset=0x0 write_offset=0x20 tail_packets=1 head_packets=0 wrapped=0`
    - `lab submit validate submit_id=1 context=pf status=0 count=1`
    - `lab submit issue submit_id=1 ... global_issue_ticket=1`
    - `lab submit transition submit_id=1 state=running ... global_running_ticket=1`
    - `lab submit transition submit_id=1 state=complete ... global_complete_ticket=1`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the bounded SGX lab submit-op ioctl now covers the validated submit-side
  mutation path:
  - `create`
  - `focus`
  - `attach`
  - `detach`
  - `kick`
  - `ccb`
  - `queue`
  - `validate`
  - `issue`
  - `transition`
  - `drop`
- the remaining sysfs dependence inside the current SGX lab flow is narrower:
  - `status`
  - `sync`
  - `owner`
  - later dependency and broader block setup

### SGX lab submit status ioctl extension

- Added the next bounded submit-side mutation step in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
  - `workloads/psb_sgx_lab_query.c`
  - `workloads/psb_sgx_lab_submit_op.c`
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- Added the submit-side operation:
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_STATUS`
- Extended the typed submit query surface with:
  - `submit_status_ta_offset`
  - `submit_status_3d_offset`
  - `submit_status_stride`
  - `submit_status_value_seed`
- Built locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the updated module and validated through:
  - `logs/vaio-ioctl/20260511-091155-sgx-submit-status-op-live`
  - target module `srcversion 1A0C93B58FDC70B06D841C6`
  - override module size `375860`
  - rollback backup size `375144`
- Verified on hardware:
  - typed `status` through focused-submit lookup succeeded with:
    - `submit_status_ta_offset=0x80`
    - `submit_status_3d_offset=0xc0`
    - `submit_status_stride=0x4`
    - `submit_status_value_seed=0x5a500000`
  - the typed query after `status` showed the same live submit fields:
    - `submit_status_ta_offset=0x80`
    - `submit_status_3d_offset=0xc0`
    - `submit_status_stride=0x4`
    - `submit_status_value_seed=0x5a500000`
    - `submit_status_object_id=5`
  - the existing validated submit progression still worked on top of the new
    typed `status` path:
    - `validate_count=1`
    - `issue_count=1`
    - `transition_count=2`
    - final `owner_state=complete`
    - final `global_complete_ticket=1`
  - post-drop typed queries still failed as expected:
    - `QUERY_FOCUS_AFTER_DROP_RC=3`
    - `QUERY_BY_ID_AFTER_DROP_RC=3`
  - the kernel log matched the same status progression:
    - `lab submit status submit_id=1 ta_offset=0x80 3d_offset=0xc0 stride=0x4 seed=0x5a500000 ta_count=1 3d_count=1`
    - `lab submit validate submit_id=1 context=pf status=0 count=1`
    - `lab submit issue submit_id=1 ... global_issue_ticket=1`
    - `lab submit transition submit_id=1 state=running ... global_running_ticket=1`
    - `lab submit transition submit_id=1 state=complete ... global_complete_ticket=1`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the bounded SGX lab submit-op ioctl now covers the validated submit-side
  mutation path:
  - `create`
  - `focus`
  - `attach`
  - `detach`
  - `kick`
  - `ccb`
  - `queue`
  - `status`
  - `validate`
  - `issue`
  - `transition`
  - `drop`
- the remaining sysfs dependence inside the current SGX lab flow is narrower:
  - `sync`
  - `owner`
  - later dependency and broader block setup

### SGX lab submit sync ioctl extension

- Added the next bounded submit-side mutation step in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
  - `workloads/psb_sgx_lab_submit_op.c`
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- Added the submit-side operation:
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_SYNC`
- Built locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the updated module and validated through:
  - `logs/vaio-ioctl/20260511-091910-sgx-submit-sync-op-live`
  - target module `srcversion 15BB28412789D8525D2C02A`
  - override module size `376916`
  - rollback backup size `375860`
- Verified on hardware:
  - typed `sync` through focused-submit lookup succeeded with:
    - `submit_sync_offset=0x20`
    - `submit_sync_complete_seq=0x40`
    - `submit_sync_pending_seq=0x41`
    - `submit_sync_flags=0x1`
  - the typed query after `sync` showed the same live submit fields:
    - `submit_sync_offset=0x20`
    - `submit_sync_complete_seq=0x40`
    - `submit_sync_pending_seq=0x41`
    - `submit_sync_flags=0x1`
  - the existing validated submit progression still worked on top of the new
    typed `sync` path:
    - `validate_count=1`
    - `issue_count=1`
    - `transition_count=2`
    - final `owner_state=complete`
    - final `global_complete_ticket=1`
  - post-drop typed queries still failed as expected:
    - `QUERY_FOCUS_AFTER_DROP_RC=3`
    - `QUERY_BY_ID_AFTER_DROP_RC=3`
  - the kernel log matched the same sync progression:
    - `lab submit sync submit_id=1 offset=0x20 read_offset=0x0 write_offset=0x20 complete_seq=0x40 pending_seq=0x41 flags=0x1 wrapped=0 epoch=0 global_issue_ticket=0`
    - `lab submit validate submit_id=1 context=pf status=0 count=1`
    - `lab submit issue submit_id=1 ... global_issue_ticket=1`
    - `lab submit transition submit_id=1 state=running ... global_running_ticket=1`
    - `lab submit transition submit_id=1 state=complete ... global_complete_ticket=1`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the bounded SGX lab submit-op ioctl now covers the validated submit-side
  mutation path:
  - `create`
  - `focus`
  - `attach`
  - `detach`
  - `kick`
  - `ccb`
  - `queue`
  - `status`
  - `sync`
  - `validate`
  - `issue`
  - `transition`
  - `drop`
- the remaining sysfs dependence inside the current SGX lab flow is narrower:
  - `owner`
  - later dependency and broader block setup

### SGX lab submit owner ioctl extension

- Added the next bounded submit-side mutation step in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
  - `workloads/psb_sgx_lab_submit_op.c`
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- Added the submit-side operation:
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_OWNER`
- Built locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the updated module and validated through:
  - `logs/vaio-ioctl/20260511-092344-sgx-submit-owner-op-live`
  - target module `srcversion E8B0AF3BC985C1317C25433`
  - override module size `377912`
  - rollback backup size `376916`
- Verified on hardware:
  - typed `owner` through focused-submit lookup succeeded with:
    - `submit_owner_offset=0x40`
    - `submit_owner_state_name=queued`
    - `submit_owner_issue_cookie=0x101`
    - `submit_owner_complete_cookie=0x100`
  - the typed query after `owner` showed the same live submit fields:
    - `submit_owner_offset=0x40`
    - `submit_owner_state_name=queued`
    - `submit_owner_issue_cookie=0x101`
    - `submit_owner_complete_cookie=0x100`
  - the existing validated submit progression still worked on top of the new
    typed `owner` path:
    - `validate_count=1`
    - `issue_count=1`
    - `transition_count=2`
    - final `owner_state=complete`
    - final `global_complete_ticket=1`
  - post-drop typed queries still failed as expected:
    - `QUERY_FOCUS_AFTER_DROP_RC=3`
    - `QUERY_BY_ID_AFTER_DROP_RC=3`
  - the kernel log matched the same owner progression:
    - `lab submit owner submit_id=1 offset=0x40 state=queued issue_cookie=0x101 complete_cookie=0x100 outstanding=0x1 epoch=0 global_complete_ticket=0 global_running_ticket=0`
    - `lab submit validate submit_id=1 context=pf status=0 count=1`
    - `lab submit issue submit_id=1 ... issue_cookie=0x102`
    - `lab submit transition submit_id=1 state=running ... complete_cookie=0x101`
    - `lab submit transition submit_id=1 state=complete ... complete_cookie=0x102 global_complete_ticket=1`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the bounded SGX lab submit-op ioctl now covers the validated submit-side
  mutation path:
  - `create`
  - `focus`
  - `attach`
  - `detach`
  - `kick`
  - `ccb`
  - `queue`
  - `status`
  - `sync`
  - `owner`
  - `validate`
  - `issue`
  - `transition`
  - `drop`
- that means the current core SGX lab submit setup no longer depends on PCI
  device sysfs for the basic single-submit path
- the remaining sysfs dependence is now in later dependency and broader
  state-block setup only

### SGX lab submit dependency-slot ioctl extension

- Added the next bounded dependency-side submit mutation step in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
  - `workloads/psb_sgx_lab_submit_op.c`
  - `scripts/run-vaio-sgx-multi-submit.sh`
- Added the submit-side operations:
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_DEPEND_SLOT`
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_DEPTICKET_SLOT`
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_DEPSTATE_SLOT`
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_DEPCLEAR_SLOT`
- Built locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the updated module and validated through:
  - `logs/vaio-ioctl/20260511-093732-sgx-dependency-slot-op-live`
  - target module `srcversion D06E0CB8A26EC70E1FAF340`
  - override module size `380244`
  - rollback backup size `377912`
- Verified on hardware:
  - the multi-submit runner completed with:
    - `DEPENDENCY_SETUP=ioctl`
    - `DEPENDENCY_INTERFACE=indexed`
  - slot `4` setup landed before clear:
    - `SUBMIT_B_AFTER_DEPTICKET4_SETUP`
    - `SUBMIT_B_AFTER_DEPSTATE4_SETUP`
    - `dependency4_submit_id=5`
    - `dependency4_offset=0x100`
    - `dependency4_required_ticket=4`
    - `dependency4_flags=0x0b`
  - the typed clear path removed slot `4` live:
    - `SUBMIT_B_AFTER_DEPCLEAR4`
    - `dependency4_submit_id=0`
    - `dependency4_offset=0x0`
    - `dependency4_required_ticket=0`
    - `dependency4_flags=0x0`
    - `dependency4_config_count=0`
  - submit B still completed cleanly after that clear:
    - `validate_count=22`
    - `issue_count=1`
    - `transition_count=2`
    - `owner_state=complete`
    - `global_issue_ticket=4`
    - `global_running_ticket=4`
    - `global_complete_ticket=4`
  - submit E later completed independently and did not repopulate slot `4`:
    - `COORD_STATE_AFTER_SUBMIT_E_POST_CLEAR`
    - final coordination `5/5/5`
    - submit B still showed `dependency4_submit_id=0`
  - final SGX state stayed clean:
    - `fault_count=0`
    - `bif_fault=0x000000`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `driver_srcversion=D06E0CB8A26EC70E1FAF340`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the bounded SGX lab submit-op ioctl surface now also covers:
  - `depend-slot`
  - `depticket-slot`
  - `depstate-slot`
  - `depclear-slot`
- the remaining sysfs dependence is no longer missing dependency-slot mutation
  coverage; it is now mostly higher-level orchestration convenience and later
  dependency or state-block work

### SGX lab multi-submit submit-op validation

- Extended `scripts/run-vaio-sgx-multi-submit.sh` so the real multi-submit
  dependency-slot-clear scenario can run with:
  - `USE_IOCTL_SUBMIT_SETUP=1`
  - `USE_IOCTL_DEPENDENCY_SETUP=1`
- Fixed one real userspace bug in:
  - `workloads/psb_sgx_lab_submit_op.c`
  - the tool now uses raw `ioctl()` for
    `DRM_IOCTL_PSB_SGX_LAB_SUBMIT_OP`
  - the reason is that libdrm's `drmIoctl()` retries `-EAGAIN`, but this lab
    path intentionally uses `-EAGAIN` to report dependency-gated transitions
- Fixed one real runner bug in:
  - `scripts/run-vaio-sgx-multi-submit.sh`
  - the new `USE_IOCTL_SUBMIT_SETUP=1` branch was not saving
    `after_submit_a_create_submits.txt`, which caused the final summary block
    to fail even after a successful target run
- Rebuilt locally:
  - `./scripts/docker-build-workloads-i386.sh`
- Directly smoke-tested the fixed submit-op helper on the live VAIO:
  - copied the rebuilt `psb_sgx_lab_submit_op` binary to
    `/root/poulsbo-workloads/psb_sgx_lab_submit_op`
  - verified `/root/poulsbo-workloads/psb_sgx_lab_submit_op /dev/dri/card0 transition 7 running 0`
    now returns:
    - `submit_op_ret=-1`
    - `errno=11`
    - exit code `1`
- Completed the clean live validation in:
  - `docs/71-sgx-lab-multi-submit-submit-op-validation.md`
  - `logs/vaio-ioctl/20260511-100431-sgx-multi-submit-op-live`
- Verified on hardware:
  - the run completed with:
    - `SUBMIT_SETUP=ioctl`
    - `DEPENDENCY_SETUP=ioctl`
    - `DEPENDENCY_INTERFACE=indexed`
  - the dependency-gated negative path now returns cleanly instead of spinning
  - slot `4` still cleared live on submit B:
    - `SUBMIT_B_AFTER_DEPCLEAR4`
    - `dependency4_submit_id=0`
    - `dependency4_offset=0x0`
    - `dependency4_required_ticket=0`
    - `dependency4_flags=0x0`
    - `dependency4_config_count=0`
  - submit B still completed cleanly after that clear
  - submit E later completed independently and did not repopulate slot `4`
  - the post-clear coordination snapshot showed:
    - `issue_ticket_counter=15`
    - `running_ticket_counter=15`
    - `complete_ticket_counter=15`
    - `live_submit_count=5`
  - final cleanup completed cleanly:
    - `OBJECTS_AFTER_DROP` reported `objects_shown=0 objects_total=0`
    - `COORD_STATE_AFTER_DROP` reported `live_submit_count=0`
    - `focused_submit_valid=0`
  - post-run SGX state stayed clean:
    - `fault_count=0`
    - `bif_fault=0x000000`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `driver_srcversion=D06E0CB8A26EC70E1FAF340`
  - `override_module_size=380244`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the bounded submit-op ioctl path is now validated on real hardware not only
  for the single-submit path but also for the real multi-submit dependency-slot
  clear scenario
- the remaining sysfs dependence is no longer the core multi-submit submit
  lifecycle or indexed dependency-slot mutation path
- the immediate userspace follow-up is to audit the remaining custom lab tools
  for `drmIoctl()` retry behavior before they grow more `-EAGAIN`-style
  semantics

### SGX lab full object plus submit ioctl validation

- Extended `scripts/run-vaio-sgx-multi-submit.sh` so the real multi-submit
  dependency-slot-clear scenario can also run with:
  - `USE_IOCTL_OBJECT_SETUP=1`
  - `USE_IOCTL_SUBMIT_SETUP=1`
  - `USE_IOCTL_DEPENDENCY_SETUP=1`
- Fixed one real userspace consistency issue in:
  - `workloads/psb_sgx_lab_object_op.c`
  - the tool now uses raw `ioctl()` for
    `DRM_IOCTL_PSB_SGX_LAB_OBJECT_OP`
  - this keeps the object-side lab path aligned with the already-fixed
    submit-op tool instead of inheriting libdrm retry behavior
- Fixed two real multi-submit runner issues in:
  - `scripts/run-vaio-sgx-multi-submit.sh`
  - added `REMOTE_SHELL_TRACE=1` so target-side shell failures can be debugged
    with remote `set -x`
  - rejected `object_resolved_id=0` in `object_adopt()` and in the mapped
    object validation pass so fallback range scanning cannot poison the live
    object map with invalid IDs
- Rebuilt locally:
  - `./scripts/docker-build-workloads-i386.sh`
- Completed the clean live validation in:
  - `docs/72-sgx-lab-full-object-submit-ioctl-validation.md`
  - `logs/vaio-ioctl/20260511-101953-sgx-full-object-submit-ioctl-live`
- Verified on hardware:
  - the run completed with:
    - `OBJECT_SETUP=ioctl`
    - `SUBMIT_SETUP=ioctl`
    - `DEPENDENCY_SETUP=ioctl`
    - `DEPENDENCY_INTERFACE=indexed`
  - the full object set for the real multi-submit scenario was adopted and
    configured through the object-op ioctl path
  - slot `4` still cleared live on submit B:
    - `SUBMIT_B_AFTER_DEPCLEAR4`
    - `dependency4_submit_id=0`
    - `dependency4_offset=0x0`
    - `dependency4_required_ticket=0`
    - `dependency4_flags=0x0`
    - `dependency4_config_count=0`
  - submit B still completed cleanly after that clear
  - submit E later completed independently and did not repopulate slot `4`
  - post-clear coordination reached:
    - `issue_ticket_counter=20`
    - `running_ticket_counter=20`
    - `complete_ticket_counter=20`
    - `live_submit_count=5`
  - final cleanup completed cleanly:
    - `OBJECTS_AFTER_DROP` reported `objects_shown=0 objects_total=0`
    - `COORD_STATE_AFTER_DROP` reported `live_submit_count=0`
    - `focused_submit_valid=0`
  - post-run SGX state stayed clean:
    - `fault_count=0`
    - `bif_fault=0x000000`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `driver_srcversion=D06E0CB8A26EC70E1FAF340`
  - `override_module_size=380244`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the real multi-submit dependency-slot-clear scenario is now validated on the
  VAIO with core object setup, submit setup, and dependency-slot mutation all
  on bounded ioctl surfaces
- the remaining sysfs dependence is no longer the core mutation path; it is
  now mainly human-readable text dumps, stale cleanup convenience, range
  discovery convenience, and later lab-only state growth

### SGX lab range query ioctl validation

- Added a bounded live-range query ioctl in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added a new 32-bit helper in:
  - `workloads/psb_sgx_lab_range_query.c`
  - it supports:
    - `index <query_index> [gem]`
    - `id <range_id> [gem]`
  - it prints typed live-range fields including:
    - resolved range ID and index
    - range type
    - `start/end/len`
    - `gpu_va`
    - `offset`
    - `stolen/in_gart/mmapping/npage/roll`
    - SGX object ownership and PF-bind counters
- Updated:
  - `workloads/Makefile`
  - `scripts/run-vaio-sgx-multi-submit.sh`
- The multi-submit runner now supports:
  - `USE_IOCTL_RANGE_DISCOVERY=1`
  - `RANGE_QUERY_SCAN_MAX=<n>`
- In that mode it:
  - stages the new range-query helper to the VAIO
  - enumerates live GEM ranges by typed ioctl index queries
  - compares before/during GEM lists to discover new candidate range IDs
  - no longer depends on scraping the page-sized `poulsbo_gtt_state` sysfs
    dump in the validated hot path
- Rebuilt locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the new module and validated on hardware in:
  - `docs/73-sgx-lab-range-query-ioctl.md`
  - `logs/vaio-ioctl/20260511-103211-sgx-range-query-live`
- Verified on the VAIO:
  - the run completed with:
    - `OBJECT_SETUP=ioctl`
    - `SUBMIT_SETUP=ioctl`
    - `RANGE_DISCOVERY=ioctl`
    - `DEPENDENCY_SETUP=ioctl`
    - `DEPENDENCY_INTERFACE=indexed`
  - the typed range path discovered:
    - `RANGE_IDS=10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34`
  - the same five-submit object map still resolved cleanly:
    - `OBJECT_IDS_A=1 2 3 4 5`
    - `OBJECT_IDS_B=6 7 8 9 10`
    - `OBJECT_IDS_C=11 12 13 14 15`
    - `OBJECT_IDS_D=16 17 18 19 20`
    - `OBJECT_IDS_E=21 22 23 24 25`
  - slot `4` still cleared live on submit B:
    - `dependency4_submit_id=0`
    - `dependency4_offset=0x0`
    - `dependency4_required_ticket=0`
    - `dependency4_flags=0x0`
    - `dependency4_config_count=0`
  - submit E still completed later without repopulating slot `4`
  - final cleanup still completed cleanly:
    - `OBJECTS_AFTER_DROP` reported `objects_shown=0 objects_total=0`
    - `COORD_STATE_AFTER_DROP` reported `live_submit_count=0`
    - `focused_submit_valid=0`
  - post-run SGX state stayed clean:
    - `fault_count=0`
    - `bif_fault=0x000000`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `driver_srcversion=8D7D3CAE32683F70FE739B7`
  - `override_module_size=381292`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the real multi-submit dependency-slot-clear scenario is now validated on the
  VAIO with typed range discovery, typed object setup, typed submit setup, and
  typed indexed dependency-slot mutation all on bounded ioctl surfaces
- the remaining sysfs dependence is narrower again:
  - human-readable text dumps
  - stale cleanup convenience
  - later lab-only state growth that still lacks a bounded ioctl surface

### SGX lab query tools raw-ioctl validation

- Updated:
  - `workloads/psb_sgx_lab_query.c`
  - `workloads/psb_sgx_lab_range_query.c`
- Both custom typed query helpers now use raw `ioctl()` instead of libdrm's
  `drmIoctl()`
- Added direct live validator:
  - `scripts/run-vaio-sgx-ioctl-range-query.sh`
- Rebuilt locally:
  - `./scripts/docker-build-workloads-i386.sh`
- Completed live validations in:
  - `docs/74-sgx-lab-query-tools-raw-ioctl-validation.md`
  - `logs/vaio-ioctl/20260511-110011-sgx-query-raw-ioctl-live`
  - `logs/vaio-ioctl/20260511-110639-sgx-range-query-direct-live-clean`
- Verified on hardware:
  - the raw submit/coord query rerun still succeeded with:
    - `coord_ioctl_ret=0 errno=0`
    - `submit_ioctl_ret=0 errno=0`
    - `submit_resolved_id=6`
    - `submit_context_name=pf`
    - `submit_cmd_type_name=ta`
    - `submit_owner_state_name=queued`
  - after one issue step, the same helper still matched the advanced state:
    - `submit_validate_count=2`
    - `submit_issue_count=1`
    - `submit_global_issue_ticket=6`
    - `coord_issue_ticket=6`
    - `coord_issue_submit_id=6`
  - the direct range-query runner enumerated live GEM ranges by typed index
    query and then re-queried the first one by explicit ID
  - the clean range-query run reported:
    - `RANGE_QUERY_COUNT=7`
    - `range_resolved_id=6`
    - `range_resolved_id=7`
    - `range_resolved_id=8`
    - `range_resolved_id=9`
    - `range_resolved_id=43`
    - `range_resolved_id=44`
    - `range_resolved_id=45`
    - `range_matching_count=7`
  - both bundles finished with clean target state:
    - `overall=remote_usable`
    - `driver_srcversion=8D7D3CAE32683F70FE739B7`
    - `override_module_size=381292`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
- Fixed one real validator bug during this work:
  - the first draft of `run-vaio-sgx-ioctl-range-query.sh` used an unbound
    remote `hold_seconds` variable in its wait loop
  - reran the validation after fixing that and kept the clean rerun as the
    canonical bundle

Conclusion:

- the custom SGX lab query-tool retry audit is now closed for:
  - `psb_sgx_lab_query`
  - `psb_sgx_lab_range_query`
- the remaining interface work is no longer “should these helpers stay on
  libdrm?”; it is later read-side growth and any richer lab-only error
  semantics that future helpers may need

### SGX lab SGX-state query ioctl validation

- Added a bounded typed SGX snapshot query in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added a new 32-bit userspace helper:
  - `workloads/psb_sgx_lab_sgx_state_query.c`
  - modes:
    - `capture`
    - `cached`
- Added a direct live validator:
  - `scripts/run-vaio-sgx-ioctl-sgx-state.sh`
- Rebuilt locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the new module and validated on hardware in:
  - `docs/75-sgx-lab-sgx-state-query-ioctl.md`
  - `logs/vaio-ioctl/20260511-112522-sgx-state-query-live`
- Verified on the VAIO:
  - guarded deploy returned cleanly with:
    - `driver_srcversion=85342D8F2786B99DF520799`
    - `override_module_size=382696`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
  - the new query helper succeeded before the 2D sequence with:
    - `sgx_state_reason=ioctl-sgx-state`
    - `sgx_state_sequence=1`
    - `sgx_state_irq_count=0`
    - `sgx_state_fault_count=0`
    - `sgx_state_twod_blit_status=0x0`
  - the cached query returned the same ioctl-triggered snapshot unchanged
  - after a sysfs read, the cached query matched the sysfs-triggered snapshot:
    - `sgx_state_reason=poulsbo_sgx_state`
    - `sgx_state_sequence=2`
  - after the validated 2D lab sequence, the ioctl-captured snapshot showed:
    - `sgx_state_reason=ioctl-sgx-state`
    - `sgx_state_sequence=14`
    - `sgx_state_irq_count=7`
    - `sgx_state_fault_count=0`
    - `sgx_state_twod_blit_status=0xe`
  - after the sysfs post-read, the cached query again matched the sysfs
    snapshot:
    - `sgx_state_reason=poulsbo_sgx_state`
    - `sgx_state_sequence=15`
    - `sgx_state_irq_count=7`
    - `sgx_state_fault_count=0`
    - `sgx_state_twod_blit_status=0xe`
  - the saved kernel log tail showed the expected real 2D activity:
    - `2D fillrect submit`
    - `2D copyarea submit`
    - `2D imageblit submit-chunked`
    - repeated `SGX irq ... [TWOD_COMPLETE]`
- Final saved target state remained clean:
  - `overall=remote_usable`
  - `driver_srcversion=85342D8F2786B99DF520799`
  - `override_module_size=382696`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

Conclusion:

- the SGX snapshot path is no longer sysfs-only
- the remaining read-side sysfs dependence is narrower again:
  - human-readable GTT dumps
  - stale cleanup convenience
  - later state growth that still lacks a bounded typed query

### SGX lab GTT-state query ioctl validation

- Added a bounded typed GTT/MMU snapshot query in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added a new 32-bit userspace helper:
  - `workloads/psb_sgx_lab_gtt_state_query.c`
- Added a direct live validator:
  - `scripts/run-vaio-sgx-ioctl-gtt-state.sh`
- Rebuilt locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the new module and validated on hardware in:
  - `docs/76-sgx-lab-gtt-state-query-ioctl.md`
  - `logs/vaio-ioctl/20260511-110215-gtt-state-query-live`
- Verified on the VAIO:
  - guarded deploy returned cleanly with:
    - `driver_srcversion=8CB35627334C3560115D1F6`
    - `override_module_size=383644`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
  - `IOCTL_GTT_STATE_BEFORE` succeeded with:
    - `gtt_state_mmu_flush_count=2`
    - `gtt_state_mmu_insert_pages_count=2`
    - `gtt_state_mmu_remove_pages_count=0`
    - `gtt_state_range_total_count=7`
    - `gtt_state_range_gem_count=4`
    - `gtt_state_range_mmapping_count=2`
  - while three held dumb GEM ranges remained live:
    - `gtt_state_mmu_flush_count=5`
    - `gtt_state_mmu_insert_pages_count=5`
    - `gtt_state_mmu_remove_pages_count=0`
    - `gtt_state_range_total_count=10`
    - `gtt_state_range_gem_count=7`
    - `gtt_state_range_mmapping_count=5`
  - after those ranges were released:
    - `gtt_state_mmu_flush_count=8`
    - `gtt_state_mmu_insert_pages_count=5`
    - `gtt_state_mmu_remove_pages_count=3`
    - `gtt_state_range_total_count=7`
    - `gtt_state_range_gem_count=4`
    - `gtt_state_range_mmapping_count=2`
  - the stable layout fields stayed unchanged across all three phases:
    - `gtt_state_gtt_start=0x94280000`
    - `gtt_state_gatt_start=0x80000000`
    - `gtt_state_mmu_gatt_start=0xe0000000`
    - `gtt_state_gatt_free_offset=0xe0800000`
  - the typed ioctl view matched the sysfs MMU summary at each phase
  - final saved target state remained clean:
    - `overall=remote_usable`
    - `driver_srcversion=8CB35627334C3560115D1F6`
    - `override_module_size=383644`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
    - `fault_count=0`
    - `bif_fault=0x000000`

Conclusion:

- the global GTT/MMU state path is no longer sysfs-only
- the remaining read-side sysfs dependence is narrower again:
  - human-readable text dumps
  - stale cleanup convenience
  - later state growth that still lacks a bounded typed query

### SGX lab cleanup-op ioctl validation

- Added typed cleanup ops in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Extended the userspace helpers:
  - `workloads/psb_sgx_lab_submit_op.c`
  - `workloads/psb_sgx_lab_object_op.c`
- Added a direct live validator:
  - `scripts/run-vaio-sgx-ioctl-cleanup.sh`
- Updated the ioctl-based runners to prefer typed cleanup first:
  - `scripts/run-vaio-sgx-ioctl-object-op.sh`
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
  - `scripts/run-vaio-sgx-multi-submit.sh`
- Rebuilt locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the new module and validated on hardware in:
  - `docs/77-sgx-lab-cleanup-op-ioctl.md`
  - `logs/vaio-ioctl/20260511-111529-sgx-cleanup-op-live`
- Verified on the VAIO:
  - guarded deploy returned cleanly with:
    - `driver_srcversion=C142911E7A8FF2E7821CAE0`
    - `override_module_size=384424`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
  - the validator seeded stale state with:
    - `RANGE_ID_A=5`
    - `RANGE_ID_B=7`
    - two adopted objects
    - two submits
    - one focused submit
  - pre-cleanup state showed:
    - `focused_submit_valid=1`
    - `focused_submit_id=1`
    - `live_submit_count=2`
    - `objects_shown=2`
    - one PF-bound object
  - typed submit cleanup succeeded with:
    - `submit_op_name=cleanup`
    - `submit_op_ret=0 errno=0`
    - `submit_result_count=2`
  - after typed submit cleanup:
    - `focused_submit_valid=0`
    - `focused_submit_id=0`
    - `live_submit_count=0`
    - `submits_shown=0`
  - typed object cleanup succeeded with:
    - `object_op_name=cleanup`
    - `object_op_ret=0 errno=0`
    - `object_result_count=2`
  - after typed object cleanup:
    - `objects_shown=0`
    - both source ranges reported `range_sgx_object_id=0`
    - the PF-bound source range also reported:
      - `range_pf_bind_count=0`
      - `range_pf_bind_total=1`
      - `range_pf_unbind_total=1`
  - final saved target state remained clean:
    - `overall=remote_usable`
    - `driver_srcversion=C142911E7A8FF2E7821CAE0`
    - `override_module_size=384424`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
    - `fault_count=0`
    - `bif_fault=0x000000`
  - did one integrated runner sanity rerun in:
    - `logs/vaio-ioctl/20260511-111946-sgx-multi-submit-cleanup-rerun`
  - that rerun proved the updated helper path actually took:
    - `STALE_SUBMIT_CLEANUP=ioctl`
    - `STALE_OBJECT_CLEANUP=ioctl`
  - and still finished with:
    - `overall=remote_usable`

Conclusion:

- the ioctl-based stale cleanup path is no longer dependent on sysfs
  scrape-and-drop loops
- the remaining sysfs dependence is narrower again:
  - human-readable inspection dumps
  - legacy compatibility fallback paths in older runners
  - later dependency or state-block growth that still lacks a bounded ioctl

### SGX lab object-query ioctl validation

- Added a bounded typed object query in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added a new 32-bit userspace helper:
  - `workloads/psb_sgx_lab_object_query.c`
- Added a direct live validator:
  - `scripts/run-vaio-sgx-ioctl-object-query.sh`
- Rebuilt locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the new module and validated on hardware in:
  - `docs/78-sgx-lab-object-query-ioctl.md`
  - `logs/vaio-ioctl/20260511-112759-sgx-object-query-live`
- Verified on the VAIO:
  - guarded deploy returned cleanly with:
    - `driver_srcversion=176E7F4A4C9DCF459B10C8E`
    - `override_module_size=385256`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
  - the validator seeded:
    - `RANGE_ID_A=5`
    - `RANGE_ID_B=7`
    - object `1` on range `5`
    - object `2` on range `7`
  - object `1` was then configured to:
    - `role=control`
    - `class=packet`
    - `gpu_use=control_ccb`
    - `expected_context=pf`
    - `pf_bind_count=1`
  - the typed query matched the live sysfs object dump:
    - `OBJECT_QUERY_ID_A_DURING`:
      - `object_resolved_id=1`
      - `object_resolved_index=0`
      - `object_total_count=2`
      - `object_role_name=control`
      - `object_class_name=packet`
      - `object_gpu_use_name=control_ccb`
      - `object_expected_context_name=pf`
      - `object_pf_bind_count=1`
    - `OBJECT_QUERY_ID_B_DURING`:
      - `object_resolved_id=2`
      - `object_resolved_index=1`
      - `object_total_count=2`
  - the index-based queries also worked:
    - `OBJECT_QUERY_INDEX_0_DURING` resolved object `1`
    - `OBJECT_QUERY_INDEX_1_DURING` resolved object `2`
  - after the userspace holders exited:
    - `OBJECT_QUERY_ID_A_AFTER_PROBES` still succeeded
    - `OBJECT_QUERY_ID_B_AFTER_PROBES` still succeeded
  - after typed cleanup:
    - `object_result_count=2`
    - `objects_shown=0`
    - `OBJECT_QUERY_ID_A_AFTER_CLEANUP` returned `errno=2`
    - `OBJECT_QUERY_ID_B_AFTER_CLEANUP` returned `errno=2`
  - final saved target state remained clean:
    - `overall=remote_usable`
    - `driver_srcversion=176E7F4A4C9DCF459B10C8E`
    - `override_module_size=385256`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
    - `fault_count=0`
    - `bif_fault=0x000000`

Conclusion:

- the live SGX lab object inspection path is no longer sysfs-only
- the remaining sysfs dependence is narrower again:
  - human-readable submit and coordination text dumps
  - any later object or submit list enumeration growth that still lacks a
    bounded typed query

### SGX lab submit-query live-index ioctl validation

- Extended the bounded typed submit query in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Added a new submit-query selector flag and typed fields:
  - `DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_INDEX`
  - `query_index`
  - `resolved_index`
  - `total_submit_count`
- Extended the 32-bit query helper:
  - `workloads/psb_sgx_lab_query.c`
- Added a direct live validator:
  - `scripts/run-vaio-sgx-ioctl-submit-query.sh`
- Rebuilt locally:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded-deployed the new module and validated on hardware in:
  - `docs/79-sgx-lab-submit-query-index-ioctl.md`
  - `logs/vaio-ioctl/20260511-113812-sgx-submit-query-live`
- Verified on the VAIO:
  - guarded deploy returned cleanly with:
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
  - the validator seeded:
    - `RANGE_IDS=7 9 15 16 17`
    - `OBJECT_IDS=6 7 8 9 10`
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
  - submit A was advanced one issue step and submit B was left queued:
    - submit A:
      - `submit_resolved_index=0`
      - `submit_total_count=2`
      - `submit_issue_count=1`
      - `submit_queue_packet_count=2`
      - `submit_sync_pending_seq=0x42`
      - `submit_owner_issue_cookie=0x102`
    - submit B:
      - `submit_resolved_index=1`
      - `submit_total_count=2`
      - `submit_issue_count=0`
      - `submit_queue_packet_count=1`
      - `submit_sync_pending_seq=0x41`
      - `submit_owner_issue_cookie=0x101`
  - the new index-based queries matched those same live submits:
    - `SUBMIT_QUERY_INDEX_A`:
      - `submit_query_index=0`
      - `submit_resolved_id=3`
      - `submit_issue_count=1`
    - `SUBMIT_QUERY_INDEX_B`:
      - `submit_query_index=1`
      - `submit_resolved_id=4`
      - `submit_issue_count=0`
  - the focus query still matched submit B:
    - `submit_resolved_id=4`
    - `submit_resolved_index=1`
  - after the userspace holders exited:
    - both explicit-ID queries still succeeded
    - both index-based queries still succeeded
  - after typed cleanup:
    - `submit_result_count=2`
    - `object_result_count=5`
    - explicit-ID, index, and focus submit queries all failed cleanly with
      `submit_ioctl_ret=-1 errno=2`
  - final saved target state remained clean:
    - `overall=remote_usable`
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

Conclusion:

- the live SGX lab submit inspection path is no longer limited to explicit ID
  and focused-submit lookup
- bounded typed submit lookup now covers live submit ordering as well
- the remaining sysfs dependence is narrower again:
  - human-readable submit and coordination text dumps
  - any later submit-list or state-block growth that still lacks a bounded
    typed query

### SGX lab typed no-sysfs state-dump validation

- Extended the 32-bit query helper:
  - `workloads/psb_sgx_lab_query.c`
  - added `coord` mode so typed coordination readout no longer requires a
    submit selector
- Added a direct live validator:
  - `scripts/run-vaio-sgx-ioctl-state-dump.sh`
- Rebuilt locally:
  - `./scripts/docker-build-workloads-i386.sh`
- Validated on hardware in:
  - `docs/80-sgx-lab-typed-state-dump-validation.md`
  - `logs/vaio-ioctl/20260511-114905-sgx-state-dump-live`
- Verified on the VAIO:
  - no kernel deploy was needed because the lab ABI was unchanged
  - the validator used only:
    - `psb_sgx_lab_query`
    - `psb_sgx_lab_object_query`
    - `psb_sgx_lab_sgx_state_query`
    - `psb_sgx_lab_gtt_state_query`
    - `psb_sgx_lab_object_op`
    - `psb_sgx_lab_submit_op`
  - it seeded:
    - `RANGE_IDS=7 9 20 21 22`
    - `OBJECT_IDS=11 12 13 14 15`
    - `SUBMIT_A_ID=5`
    - `SUBMIT_B_ID=6`
  - the typed coord dump reported:
    - `coord_focus_valid=1`
    - `coord_focused_submit_id=6`
    - `coord_live_submit_count=2`
    - `coord_live_object_count=5`
    - `coord_issue_ticket=3`
    - `coord_issue_submit_id=5`
  - the typed global dumps matched the staged state:
    - `gtt_state_range_sgx_object_count=5`
    - `gtt_state_range_pf_bound_count=5`
    - `sgx_state_reason=ioctl-sgx-state`
    - `sgx_state_fault_count=0`
    - `sgx_state_bif_fault=0x0`
  - the typed object and submit index dumps also matched:
    - object `11` at index `0`:
      - `role=control`
      - `class=packet`
      - `gpu_use=control_ccb`
    - object `15` at index `4`:
      - `role=status`
      - `class=state`
      - `gpu_use=status_block`
    - submit `5` at index `0`:
      - `issue_count=1`
      - `queue_packet_count=2`
      - `sync_pending_seq=0x42`
    - submit `6` at index `1`:
      - `issue_count=0`
      - `queue_packet_count=1`
      - `sync_pending_seq=0x41`
  - after all userspace holders exited:
    - the typed coord dump still reported:
      - `coord_live_submit_count=2`
      - `coord_live_object_count=5`
    - typed index queries still resolved object `11` and submit `5`
  - after typed cleanup:
    - `submit_result_count=2`
    - `object_result_count=5`
    - `coord_live_submit_count=0`
    - `coord_live_object_count=0`
    - `gtt_state_range_sgx_object_count=0`
    - `gtt_state_range_pf_bound_count=0`
  - final saved target state remained clean:
    - `overall=remote_usable`
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

Conclusion:

- live SGX lab inspection no longer needs sysfs for one end-to-end
  human-readable state dump
- the remaining sysfs dependence is narrower again:
  - legacy compatibility fallback paths
  - any later state-block growth that still lacks a bounded typed query

### SGX lab standalone dump-tool validation

- Added a new 32-bit helper:
  - `workloads/psb_sgx_lab_dump.c`
- Updated the live no-sysfs validator:
  - `scripts/run-vaio-sgx-ioctl-state-dump.sh`
- Rebuilt locally:
  - `./scripts/docker-build-workloads-i386.sh`
- Validated on hardware in:
  - `docs/81-sgx-lab-dump-tool-validation.md`
  - `logs/vaio-ioctl/20260511-114905-sgx-state-dump-live`
- Verified on the VAIO:
  - no kernel deploy was needed because the SGX lab ABI was unchanged
  - the runner now stages and executes:
    - `psb_sgx_lab_dump`
  - the standalone dump tool reported, during the staged state:
    - `coord_live_submit_count=2`
    - `coord_live_object_count=5`
    - `gtt_state_range_sgx_object_count=5`
    - `sgx_state_fault_count=0`
    - `object[0]_role_name=control`
    - `submit[0]_issue_count=1`
  - after typed cleanup the same dump tool reported:
    - `coord_live_submit_count=0`
    - `coord_live_object_count=0`
    - `gtt_state_range_sgx_object_count=0`
  - final saved target state remained clean:
    - `overall=remote_usable`
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

Conclusion:

- the typed no-sysfs SGX lab dump path now has a standalone userspace tool
- human-readable live inspection no longer depends on shell-side composition
  alone

### SGX lab dump JSON validation

- Extended the standalone dump helper:
  - `workloads/psb_sgx_lab_dump.c`
  - added:
    - `json`
    - `capture-json`
- Updated the live validator:
  - `scripts/run-vaio-sgx-ioctl-state-dump.sh`
  - it now validates both text and JSON dump output during and after cleanup
- Rebuilt locally:
  - `./scripts/docker-build-workloads-i386.sh`
- Validated on hardware in:
  - `docs/82-sgx-lab-dump-json-validation.md`
  - `logs/vaio-ioctl/20260511-120359-sgx-dump-json-live`
- Verified on the VAIO:
  - no kernel deploy was needed because the SGX lab ABI was unchanged
  - the target stayed on:
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
  - the runner staged:
    - `RANGE_IDS=7 9 30 31 32`
    - `OBJECT_IDS=21 22 23 24 25`
    - `SUBMIT_A_ID=9`
    - `SUBMIT_B_ID=10`
  - the new JSON dump matched the staged live state:
    - `"live_submit_count": 2`
    - `"live_object_count": 5`
    - `"range_sgx_object_count": 5`
    - `"fault_count": 0`
    - `"role_name": "control"`
    - `"issue_count": 1`
  - after typed cleanup the JSON dump matched the cleared state:
    - `"live_submit_count": 0`
    - `"live_object_count": 0`
    - `"range_sgx_object_count": 0`
  - final saved target state remained clean:
    - `overall=remote_usable`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

Conclusion:

- the standalone no-sysfs SGX lab dump path now has both:
  - human-readable text output
  - structured JSON output
- later validation can now consume the same typed live SGX lab snapshot
  without parsing sysfs text or shell-composed dumps

### SGX lab cleanup no-sysfs validation

- Reworked the focused cleanup validator:
  - `scripts/run-vaio-sgx-ioctl-cleanup.sh`
- It now stages and uses only typed lab helpers for verification:
  - `psb_sgx_lab_query`
  - `psb_sgx_lab_object_query`
  - `psb_sgx_lab_range_query`
  - `psb_sgx_lab_dump`
  - plus the existing object-op and submit-op mutation helpers
- It no longer reads:
  - `poulsbo_sgx_coord_state`
  - `poulsbo_sgx_submits`
  - `poulsbo_sgx_objects`
  - `poulsbo_gtt_state`
- Validated on hardware in:
  - `docs/83-sgx-lab-cleanup-nosysfs-validation.md`
  - `logs/vaio-ioctl/20260511-122500-sgx-cleanup-nosysfs-live`
- Verified on the VAIO:
  - no kernel deploy was needed because the SGX lab ABI was unchanged
  - the target stayed on:
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
  - before cleanup the typed path showed:
    - `coord_focus_valid=1`
    - `coord_focused_submit_id=13`
    - `coord_live_submit_count=2`
    - `coord_live_object_count=2`
    - submit indices `0/1` resolved submits `13/14`
    - the dump snapshot showed:
      - `gtt_state_range_sgx_object_count=2`
      - `gtt_state_range_pf_bound_count=1`
  - after typed submit cleanup:
    - `coord_focus_valid=0`
    - `coord_live_submit_count=0`
    - focused-submit query failed with `submit_ioctl_ret=-1 errno=2`
    - submit index `0` query failed with `submit_ioctl_ret=-1 errno=2`
  - after typed object cleanup:
    - object-ID queries for objects `28/29` failed with
      `object_ioctl_ret=-1 errno=2`
    - the dump snapshot showed:
      - `coord_live_object_count=0`
      - `gtt_state_range_sgx_object_count=0`
      - `gtt_state_range_pf_bound_count=0`
    - final typed range-ID checks still showed:
      - `range_sgx_object_id=0`
      - `range_pf_bind_count=0`
      - nonzero lifetime bind/unbind totals on the previously PF-bound range
  - final saved target state remained clean:
    - `overall=remote_usable`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

Conclusion:

- the focused cleanup validator no longer depends on sysfs text reads for its
  verification path
- the remaining sysfs dependence is now further concentrated in:
  - legacy compatibility fallbacks
  - older non-ioctl wrappers
  - any later state growth that still lacks a bounded typed read path

### SGX lab object-op no-sysfs validation

- Reworked the focused object-op validator:
  - `scripts/run-vaio-sgx-ioctl-object-op.sh`
- It now stages and uses only typed lab helpers for verification:
  - `psb_sgx_lab_object_query`
  - `psb_sgx_lab_range_query`
  - `psb_sgx_lab_gtt_state_query`
  - plus the existing object-op mutation helper
- It no longer reads:
  - `poulsbo_sgx_objects`
  - `poulsbo_gtt_state`
- Validated on hardware in:
  - `docs/84-sgx-lab-object-op-nosysfs-validation.md`
  - `logs/vaio-ioctl/20260511-123200-sgx-object-op-nosysfs-live`
- Verified on the VAIO:
  - no kernel deploy was needed because the SGX lab ABI was unchanged
  - the target stayed on:
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
  - typed baseline showed:
    - `range_sgx_object_id=0`
    - `range_pf_bind_count=0`
    - `gtt_state_range_sgx_object_count=0`
    - `gtt_state_range_pf_bound_count=0`
  - after typed adopt of object `30` on range `7`:
    - object query showed generic role/class/gpu_use and auto context
    - range query showed `range_sgx_object_id=30`
    - GTT state showed `gtt_state_range_sgx_object_count=1`
  - after typed metadata updates:
    - object query showed:
      - `object_role_name=control`
      - `object_class_name=packet`
      - `object_gpu_use_name=control_ccb`
      - `object_expected_context_name=pf`
  - after typed PF bind:
    - `object_pf_bind_count=1`
    - `range_pf_bind_count=1`
    - `gtt_state_range_pf_bound_count=1`
  - after typed PF unbind:
    - `object_pf_bind_count=0`
    - `range_pf_bind_count=0`
    - `gtt_state_range_pf_bound_count=0`
  - after userspace close, typed object-ID query still resolved object `30`
  - after typed drop:
    - object-ID query failed with `object_ioctl_ret=-1 errno=2`
    - range query showed `range_sgx_object_id=0`
    - GTT state showed `gtt_state_range_sgx_object_count=0`
  - final saved target state remained clean:
    - `overall=remote_usable`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

Conclusion:

- the focused object-op validator no longer depends on sysfs text reads for
  its verification path
- the remaining sysfs dependence is now further concentrated in:
  - legacy compatibility fallbacks
  - older non-ioctl wrappers
  - any later state growth that still lacks a bounded typed read path

### SGX lab submit-op no-sysfs validation

- Reworked the focused submit-op validator:
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- It now stages and verifies through typed lab helpers only:
  - `workloads/drm_probe.c`
  - `workloads/psb_sgx_lab_query.c`
  - `workloads/psb_sgx_lab_submit_op.c`
  - `workloads/psb_sgx_lab_object_op.c`
  - `workloads/psb_sgx_lab_object_query.c`
  - `workloads/psb_sgx_lab_range_query.c`
  - `workloads/psb_sgx_lab_gtt_state_query.c`
- It no longer reads:
  - `poulsbo_sgx_object`
  - `poulsbo_sgx_objects`
  - `poulsbo_sgx_submit`
  - `poulsbo_sgx_submits`
  - `poulsbo_sgx_submit_state`
  - `poulsbo_sgx_coord_state`
  - `poulsbo_gtt_state`
- Validated on hardware in:
  - `docs/85-sgx-lab-submit-op-nosysfs-validation.md`
  - `logs/vaio-ioctl/20260511-123800-sgx-submit-op-nosysfs-live`
- Verified on the VAIO:
  - no kernel deploy was needed because the SGX lab ABI was unchanged
  - the target stayed on:
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
  - the runner staged:
    - `SUBMIT_ID=16`
    - `RANGE_IDS=7 9 45 46 47`
    - `OBJECT_IDS=36 37 38 39 40`
  - typed baseline showed:
    - `gtt_state_range_sgx_object_count=0`
    - `gtt_state_range_pf_bound_count=0`
  - after typed object setup:
    - control object `36` resolved as:
      - `object_role_name=control`
      - `object_class_name=packet`
      - `object_gpu_use_name=control_ccb`
      - `object_expected_context_name=pf`
      - `object_pf_bind_count=1`
    - status object `40` resolved as:
      - `object_role_name=status`
      - `object_class_name=state`
      - `object_gpu_use_name=status_block`
      - `object_expected_context_name=pf`
      - `object_pf_bind_count=1`
    - typed GTT state showed:
      - `gtt_state_range_sgx_object_count=5`
      - `gtt_state_range_pf_bound_count=5`
  - after typed attach:
    - submit query showed:
      - `submit_control_object_id=36`
      - `submit_pds_object_id=37`
      - `submit_use_object_id=38`
      - `submit_target_object_id=39`
      - `submit_status_object_id=40`
      - `submit_owner_state_name=queued`
  - after typed progression:
    - `validate` produced:
      - `submit_validate_count=1`
    - `issue` produced:
      - `coord_issue_submit_id=16`
      - `submit_issue_count=1`
      - `submit_global_issue_ticket=7`
    - `transition running` produced:
      - `coord_running_submit_id=16`
      - `submit_owner_state_name=running`
      - `submit_transition_count=1`
      - `submit_global_running_ticket=2`
    - `transition complete` produced:
      - `coord_complete_submit_id=16`
      - `submit_owner_state_name=complete`
      - `submit_transition_count=2`
      - `submit_global_complete_ticket=2`
  - after typed submit drop:
    - focused-submit query failed with `submit_ioctl_ret=-1 errno=2`
    - explicit submit-ID query failed with `submit_ioctl_ret=-1 errno=2`
    - `coord_live_submit_count=0`
    - `coord_live_object_count=5`
  - after typed object cleanup:
    - `object_result_count=5`
    - `gtt_state_range_sgx_object_count=0`
    - `gtt_state_range_pf_bound_count=0`
    - object-ID query for object `36` failed with
      `object_ioctl_ret=-1 errno=2`
  - final saved target state remained clean:
    - `overall=remote_usable`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

Conclusion:

- the focused submit-op validator no longer depends on sysfs text reads for
  either setup or verification
- the remaining sysfs dependence is now further concentrated in:
  - legacy compatibility fallbacks
  - older non-ioctl wrappers
  - any later dependency or state-block growth that still lacks a bounded
    typed read path

### SGX lab multi-submit no-sysfs validation

- Extended the higher-level multi-submit runner:
  - `scripts/run-vaio-sgx-multi-submit.sh`
- Added the dedicated wrapper:
  - `scripts/run-vaio-sgx-multi-submit-nosysfs.sh`
- Added a strict typed verification mode:
  - `USE_IOCTL_VERIFICATION=1`
- That mode now requires:
  - `USE_IOCTL_OBJECT_SETUP=1`
  - `USE_IOCTL_SUBMIT_SETUP=1`
  - `USE_IOCTL_DEPENDENCY_SETUP=1`
  - `USE_IOCTL_RANGE_DISCOVERY=1`
- In that mode, the runner stages and verifies through typed helpers only:
  - `workloads/psb_sgx_lab_query.c`
  - `workloads/psb_sgx_lab_gtt_state_query.c`
  - `workloads/psb_sgx_lab_dump.c`
  - `workloads/psb_sgx_lab_range_query.c`
  - `workloads/psb_sgx_lab_object_op.c`
  - `workloads/psb_sgx_lab_submit_op.c`
- It no longer needs:
  - `poulsbo_sgx_object`
  - `poulsbo_sgx_objects`
  - `poulsbo_sgx_submit`
  - `poulsbo_sgx_submits`
  - `poulsbo_sgx_submit_state`
  - `poulsbo_sgx_coord_state`
  - `poulsbo_gtt_state`
  for verification when all typed setup paths are enabled
- Validated on hardware in:
  - `docs/86-sgx-lab-multi-submit-nosysfs-validation.md`
  - `logs/vaio-ioctl/20260511-124033-sgx-multi-submit-nosysfs-live`
- Verified on the VAIO:
  - no kernel deploy was needed because the SGX lab ABI was unchanged
  - the target stayed on:
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
  - the saved run reported:
    - `SUBMIT_SETUP=ioctl`
    - `OBJECT_SETUP=ioctl`
    - `RANGE_DISCOVERY=ioctl`
    - `DEPENDENCY_SETUP=ioctl`
    - `VERIFICATION_MODE=ioctl`
    - `DEPENDENCY_INTERFACE=indexed`
  - the full four-lane scenario staged:
    - `coord_live_submit_count=5`
    - `coord_live_object_count=25`
    - `gtt_state_range_sgx_object_count=25`
    - `gtt_state_range_pf_bound_count=25`
  - submit B still cleared lane 4 live:
    - `dependency[4]_submit_id=0`
    - `dependency[4]_flags=0x0`
  - submit E still completed independently after that clear
  - the same typed run still resolved all five submits after userspace exit:
    - submit A: `validate_count=5 issue_count=1 transition_count=2`
    - submit B: `validate_count=22 issue_count=1 transition_count=2`
    - submit C: `validate_count=5 issue_count=1 transition_count=2`
    - submit D: `validate_count=5 issue_count=1 transition_count=2`
    - submit E: `validate_count=5 issue_count=1 transition_count=2`
  - final typed cleanup returned to zero:
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
    - `gtt_state_range_pf_bound_count=0`
  - final saved target state remained clean:
    - `overall=remote_usable`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

Conclusion:

- the higher-level multi-submit runner no longer depends on sysfs text reads
  for its verification path when the typed object, submit, dependency, and
  range setup paths are enabled together
- the remaining sysfs dependence is now further concentrated in:
  - legacy compatibility fallbacks in the same runner
  - older non-ioctl wrappers
  - any later dependency or state-block growth that still lacks a bounded
    typed read path

### SGX dependency-wrapper typed defaults

- Added the generic typed multi-submit wrapper:
  - `scripts/run-vaio-sgx-multi-submit-typed.sh`
- Moved the dependency-oriented scenario wrappers onto it:
  - `scripts/run-vaio-sgx-dependency-slot-clear.sh`
  - `scripts/run-vaio-sgx-fourth-dependency-slot.sh`
  - `scripts/run-vaio-sgx-indexed-dependency-array.sh`
  - `scripts/run-vaio-sgx-third-dependency-lane.sh`
  - `scripts/run-vaio-sgx-multi-source-dependency.sh`
  - `scripts/run-vaio-sgx-multi-source-ticket-dependency.sh`
  - `scripts/run-vaio-sgx-dependency-drop.sh`
  - `scripts/run-vaio-sgx-dependency-satisfied-drop.sh`
  - `scripts/run-vaio-sgx-dependency-unrelated-submit.sh`
- Kept the scenario-specific wrappers responsible only for:
  - source-count and lane-count shape
  - ticket defaults
  - clear/drop behavior
- The new generic wrapper now only forces the already-validated typed path:
  - `USE_IOCTL_OBJECT_SETUP=1`
  - `USE_IOCTL_SUBMIT_SETUP=1`
  - `USE_IOCTL_DEPENDENCY_SETUP=1`
  - `USE_IOCTL_RANGE_DISCOVERY=1`
  - `USE_IOCTL_VERIFICATION=1`
- The first live rerun failed locally with `Permission denied` because the new
  wrapper file had been added without the executable bit.
- Fixed that with:
  - `chmod +x scripts/run-vaio-sgx-multi-submit-typed.sh`
- Validated on hardware in:
  - `docs/87-sgx-dependency-wrapper-typed-defaults.md`
  - `logs/vaio-ioctl/20260511-124744-sgx-dependency-slot-clear-typed-wrapper-live-retry`
- Verified on the VAIO:
  - the wrapper-level scenario stayed on the typed path:
    - `SUBMIT_SETUP=ioctl`
    - `OBJECT_SETUP=ioctl`
    - `RANGE_DISCOVERY=ioctl`
    - `DEPENDENCY_SETUP=ioctl`
    - `VERIFICATION_MODE=ioctl`
    - `DEPENDENCY_INTERFACE=indexed`
  - the wrapper-level staged state was still correct:
    - `coord_live_submit_count=5`
    - `coord_live_object_count=25`
    - `gtt_state_range_sgx_object_count=25`
    - `gtt_state_range_pf_bound_count=25`
  - submit B lane 4 was observed before and after clear:
    - before clear:
      - `dependency[4]_submit_id=31`
      - `dependency[4]_required_ticket=4`
      - `dependency[4]_flags=0xb`
    - after clear:
      - `dependency[4]_submit_id=0`
      - `dependency[4]_required_ticket=0`
      - `dependency[4]_flags=0x0`
  - final typed cleanup returned to zero:
    - `coord_live_submit_count=0`
    - `coord_live_object_count=0`
    - `gtt_state_range_sgx_object_count=0`
    - `gtt_state_range_pf_bound_count=0`
  - final saved target state remained clean:
    - `overall=remote_usable`
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

Conclusion:

- the dependency-specific wrapper layer no longer defaults callers back onto
  the older direct multi-submit path
- the remaining sysfs dependence is now further concentrated in:
  - legacy compatibility fallbacks in the same runner
  - older direct sysfs-oriented bring-up helpers outside the typed wrapper path
  - any later dependency or state-block growth that still lacks a bounded
    typed read path

### SGX focused-wrapper typed defaults

- Added focused typed wrappers:
  - `scripts/run-vaio-sgx-object-typed.sh`
  - `scripts/run-vaio-sgx-submit-typed.sh`
- Changed the older focused helper names so they now default onto those typed
  wrappers:
  - `scripts/run-vaio-sgx-object.sh`
  - `scripts/run-vaio-sgx-submit.sh`
- Kept the older sysfs-heavy script bodies available through explicit opt-in:
  - `SGX_OBJECT_DRIVER_MODE=legacy`
  - `SGX_SUBMIT_DRIVER_MODE=legacy`
- The focused typed wrappers map the old probe-binary variables:
  - `LOCAL_BIN`
  - `TARGET_BIN`
  - `TARGET_STAGE`
  onto the already-validated typed validators:
  - `scripts/run-vaio-sgx-ioctl-object-op.sh`
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- Validated on hardware in:
  - `docs/88-sgx-focused-wrapper-typed-defaults.md`
  - `logs/vaio-ioctl/20260511-125525-sgx-object-wrapper-typed-default-live-rerun`
  - `logs/vaio-ioctl/20260511-125341-sgx-submit-wrapper-typed-default-live`
- Verified on the VAIO:
  - the object wrapper now ran the typed object path:
    - `REMOTE_OP_BIN=/root/poulsbo-workloads/psb_sgx_lab_object_op`
    - `RANGE_ID=7`
    - `OBJECT_ID=123`
    - metadata resolved as:
      - `object_role_name=control`
      - `object_class_name=packet`
      - `object_gpu_use_name=control_ccb`
      - `object_expected_context_name=pf`
    - PF bind/unbind still behaved correctly:
      - `object_pf_bind_count=1`
      - `range_pf_bind_count=1`
      - `gtt_state_range_pf_bound_count=1`
      - then back to `0`
    - final drop returned:
      - `object_ioctl_ret=-1 errno=2`
      - `gtt_state_range_sgx_object_count=0`
  - the submit wrapper now ran the typed submit path:
    - `REMOTE_SUBMIT_OP_BIN=/root/poulsbo-workloads/psb_sgx_lab_submit_op`
    - `SUBMIT_ID=32`
    - `RANGE_IDS=7 9 125 126 127`
    - `OBJECT_IDS=117 118 119 120 121`
    - post-setup:
      - `gtt_state_range_sgx_object_count=5`
      - `gtt_state_range_pf_bound_count=5`
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
    - final drop and cleanup returned:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
      - `gtt_state_range_pf_bound_count=0`
  - final saved target state remained clean:
    - `overall=remote_usable`
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

Conclusion:

- the older focused helper names no longer default callers back onto the
  sysfs-heavy bring-up path
- the remaining direct sysfs use for those focused helpers is now explicit and
  opt-in through the `legacy` mode variables

### SGX multi-submit wrapper mode gate

- Added the dispatcher:
  - `scripts/run-vaio-sgx-multi-submit-default.sh`
- Added explicit multi-submit wrapper modes:
  - `SGX_MULTI_SUBMIT_DRIVER_MODE=typed`
  - `SGX_MULTI_SUBMIT_DRIVER_MODE=legacy`
- Kept the default as:
  - `typed`
- Updated the dependency-oriented wrappers to call the dispatcher instead of
  calling `run-vaio-sgx-multi-submit-typed.sh` directly:
  - `scripts/run-vaio-sgx-dependency-slot-clear.sh`
  - `scripts/run-vaio-sgx-fourth-dependency-slot.sh`
  - `scripts/run-vaio-sgx-indexed-dependency-array.sh`
  - `scripts/run-vaio-sgx-third-dependency-lane.sh`
  - `scripts/run-vaio-sgx-multi-source-dependency.sh`
  - `scripts/run-vaio-sgx-multi-source-ticket-dependency.sh`
  - `scripts/run-vaio-sgx-dependency-drop.sh`
  - `scripts/run-vaio-sgx-dependency-satisfied-drop.sh`
  - `scripts/run-vaio-sgx-dependency-unrelated-submit.sh`
- Validated on hardware in:
  - `docs/89-sgx-multi-submit-wrapper-mode-gate.md`
  - `logs/vaio-ioctl/20260511-130053-sgx-indexed-wrapper-default-live`
- Verified on the VAIO:
  - the indexed wrapper stayed on the typed path:
    - `SUBMIT_SETUP=ioctl`
    - `OBJECT_SETUP=ioctl`
    - `RANGE_DISCOVERY=ioctl`
    - `DEPENDENCY_SETUP=ioctl`
    - `VERIFICATION_MODE=ioctl`
    - `DEPENDENCY_INTERFACE=indexed`
  - the live staged state was correct:
    - `coord_live_submit_count=4`
    - `coord_live_object_count=20`
    - `gtt_state_range_sgx_object_count=20`
    - `gtt_state_range_pf_bound_count=20`
  - the dependent submit showed the expected indexed bindings:
    - `dependency[1]_submit_id=33`
    - `dependency[2]_submit_id=35`
    - `dependency[3]_submit_id=36`
  - the same wrapper run completed and cleaned up through the typed path:
    - terminal submit counts:
      - `submit_validate_count=17`
      - `submit_issue_count=1`
      - `submit_transition_count=2`
    - final global state:
      - `coord_live_submit_count=0`
      - `coord_live_object_count=0`
      - `gtt_state_range_sgx_object_count=0`
      - `gtt_state_range_pf_bound_count=0`
  - final saved target state remained clean:
    - `overall=remote_usable`
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

Conclusion:

- the dependency-specific multi-submit wrappers now behave like the focused
  helper layer:
  - typed by default
  - explicit `legacy` escape hatch
- the remaining sysfs-heavy multi-submit behavior is now explicit and opt-in
  instead of being the silent alternate path behind those wrappers

### SGX legacy mode-gate validation

- Validated the explicit legacy escape hatches:
  - `SGX_OBJECT_DRIVER_MODE=legacy`
  - `SGX_SUBMIT_DRIVER_MODE=legacy`
  - `SGX_MULTI_SUBMIT_DRIVER_MODE=legacy`
- During validation, the focused legacy submit path exposed a real bug in
  `scripts/run-vaio-sgx-submit.sh`:
  - the fallback scan only searched up to
    `max_before_range_id + 128`
  - that was no longer enough once live GEM range IDs on the VAIO advanced
    further
- Fixed that legacy submit path by adding:
  - `RANGE_SCAN_MAX`
  - current default: `512`
- Also added basic validation:
  - `RANGE_SCAN_MAX` must be at least `1`
- Applied the same symmetry fix to the direct legacy multi-submit path in
  `scripts/run-vaio-sgx-multi-submit.sh`, which had still been hard-coded to
  search only up to `max_before_range_id + 256`
- Validated on hardware in:
  - `docs/90-sgx-legacy-mode-gate-validation.md`
  - `logs/vaio-legacy/20260511-130407-sgx-object-wrapper-legacy-live`
  - `logs/vaio-legacy/20260511-130601-sgx-submit-wrapper-legacy-live-rerun`
  - `logs/vaio-legacy/20260511-130640-sgx-indexed-wrapper-legacy-live`
  - `logs/vaio-legacy/20260511-131035-sgx-indexed-wrapper-legacy-live-rerun`
- Verified on the VAIO:
  - focused object legacy stayed on the old sysfs-heavy path and still
    completed cleanly:
    - `RANGE_ID=153`
    - `OBJECT_ID=144`
    - clean `GTT_STATE_AFTER_DROP`
    - clean `OBJECTS_AFTER_DROP`
  - focused submit legacy failed before the fix with:
    - `Expected 5 fresh SGX-adoptable GEM ranges, found 2`
  - focused submit legacy succeeded after the `RANGE_SCAN_MAX` fix:
    - `RANGE_IDS=159 160 161 162 163`
    - `OBJECT_IDS=147 148 149 150 151`
    - `SUBMIT_ID=37`
    - clean `SUBMITS_AFTER_DROP`
    - clean `OBJECTS_AFTER_DROP`
  - indexed multi-submit legacy stayed on the old path and still completed:
    - `DEPENDENCY_INTERFACE=indexed`
    - `SUBMIT_A_ID=38`
    - `SUBMIT_B_ID=39`
    - `SUBMIT_C_ID=40`
    - `SUBMIT_D_ID=41`
    - clean `SUBMITS_AFTER_DROP`
    - clean `OBJECTS_AFTER_DROP`
  - indexed multi-submit legacy also completed cleanly again after the
    multi-submit-side `RANGE_SCAN_MAX` symmetry fix:
    - `SUBMIT_A_ID=42`
    - `SUBMIT_B_ID=43`
    - `SUBMIT_C_ID=44`
    - `SUBMIT_D_ID=45`
    - clean `SUBMITS_AFTER_DROP`
    - clean `OBJECTS_AFTER_DROP`
  - final saved target state remained clean:
    - `overall=remote_usable`
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

Conclusion:

- the explicit legacy modes are usable on the VAIO after the wrapper refactors
- the focused legacy submit path needed one real maintenance fix, which is now
  in place
- the repo now has validated behavior for:
  - typed-by-default wrappers
  - explicit legacy focused object path
  - explicit legacy focused submit path
  - explicit legacy indexed multi-submit path

### SGX typed-vs-legacy compare sessions

- Added direct compare wrappers for the three wrapper families:
  - `scripts/run-vaio-sgx-object-mode-compare.sh`
  - `scripts/run-vaio-sgx-submit-mode-compare.sh`
  - `scripts/run-vaio-sgx-multi-submit-mode-compare.sh`
- Each compare wrapper now:
  - checks target health first
  - captures pre-state
  - runs the default typed path
  - rechecks health
  - runs the explicit legacy path
  - rechecks health again
  - captures post-state
  - saves `mode-summary.log` and `session-result.env`
- Validated on hardware in:
  - `docs/91-sgx-mode-compare-session-validation.md`
  - `logs/vaio-compare/20260511-132342-sgx-object-mode-compare`
  - `logs/vaio-compare/20260511-132458-sgx-submit-mode-compare`
  - `logs/vaio-compare/20260511-132628-sgx-multi-submit-mode-compare`
- Verified on the VAIO:
  - focused object compare completed cleanly:
    - typed:
      - `typed_range_id=7`
      - `typed_object_id=204`
      - `typed_object_role=control`
      - `typed_object_class=packet`
      - `typed_object_gpu_use=control_ccb`
      - `typed_object_context=pf`
      - final typed cleanup:
        - `typed_range_sgx_object_count=0`
        - `typed_range_pf_bound_count=0`
    - legacy:
      - `legacy_range_id=217`
      - `legacy_object_id=205`
      - clean `GTT_STATE_AFTER_DROP`
      - clean `OBJECTS_AFTER_DROP`
  - focused submit compare completed cleanly:
    - typed:
      - `typed_submit_id=48`
      - `typed_issue_submit_id=48`
      - `typed_running_submit_id=48`
      - `typed_complete_submit_id=48`
      - `typed_validate_count=1`
      - `typed_issue_count=1`
      - `typed_transition_count=2`
      - `typed_owner_state=complete`
      - final typed cleanup:
        - `typed_live_submit_count=0`
        - `typed_range_sgx_object_count=0`
        - `typed_range_pf_bound_count=0`
    - legacy:
      - `legacy_submit_id=49`
      - `legacy_queue_mode=wrap`
      - clean `SUBMITS_AFTER_DROP`
      - clean `OBJECTS_AFTER_DROP`
  - multi-submit compare completed cleanly:
    - typed:
      - `typed_submit_setup=ioctl`
      - `typed_object_setup=ioctl`
      - `typed_range_discovery=ioctl`
      - `typed_dependency_setup=ioctl`
      - `typed_verification_mode=ioctl`
      - `typed_submit_a_id=50`
      - `typed_submit_b_id=51`
      - `typed_submit_validate_count=11`
      - `typed_submit_issue_count=2`
      - `typed_submit_transition_count=4`
      - `typed_submit_owner_state=complete`
      - final typed cleanup:
        - `typed_live_submit_count=0`
        - `typed_live_object_count=0`
        - `typed_range_sgx_object_count=0`
        - `typed_range_pf_bound_count=0`
    - legacy:
      - `legacy_submit_a_id=52`
      - `legacy_submit_b_id=53`
      - `legacy_dependency_interface=named`
      - clean `SUBMITS_AFTER_DROP`
      - clean `OBJECTS_AFTER_DROP`
      - clean `COORD_STATE_AFTER_DROP`
  - final target state remained clean after all compare sessions:
    - `overall=remote_usable`
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

### VAIO session summary ordering fix

- The new compare bundles exposed one real helper bug in
  `scripts/summarize-vaio-sessions.sh`:
  - it sorted by full path instead of the session timestamp
  - that buried fresh `logs/vaio-compare/` runs behind older directory
    prefixes
- Fixed it by sorting candidates on the timestamp prefix from the session
  directory name instead of on the raw path
- Revalidated with:
  - `MAX_SESSIONS=8 ./scripts/summarize-vaio-sessions.sh`
- Verified the newest compare bundles now surface first:
  - `20260511-132628-sgx-multi-submit-mode-compare`
  - `20260511-132458-sgx-submit-mode-compare`
  - `20260511-132342-sgx-object-mode-compare`

### Compare-session metadata persistence

- Extended the three compare wrappers so they now record:
  - `final_driver_srcversion`
  - `final_override_module_size`
  in `session-result.env`
- Also improved `scripts/summarize-vaio-sessions.sh` further:
  - identical typed and legacy health now collapses to one
    `last_state=remote_usable`
  - module srcversion now falls back to compare-session `post-state.log` or
    `pre-state.log` when older bundles do not yet have structured result
    metadata
- Revalidated on hardware with:
  - `logs/vaio-compare/20260511-133519-sgx-object-mode-compare`
- Verified:
  - `session-result.env` now carries:
    - `final_driver_srcversion=D43951B174F84BE870230AB`
    - `final_override_module_size=385384`
  - `MAX_SESSIONS=4 ./scripts/summarize-vaio-sessions.sh` now shows the fresh
    compare bundles with:
    - `last_state=remote_usable`
    - `module_srcversion=D43951B174F84BE870230AB`

### Dependency-wrapper typed-vs-legacy compare session

- Added a direct compare wrapper for the dependency alias layer:
  - `scripts/run-vaio-sgx-dependency-slot-clear-mode-compare.sh`
- This compare session drives the default wrapper path through:
  - `scripts/run-vaio-sgx-dependency-slot-clear.sh`
  and the explicit legacy path through:
  - `SGX_MULTI_SUBMIT_DRIVER_MODE=legacy`
- Documented it in:
  - `docs/92-sgx-dependency-wrapper-mode-compare-validation.md`
- Validated on hardware in:
  - `logs/vaio-compare/20260511-134016-sgx-dependency-slot-clear-mode-compare`
- Verified on the VAIO:
  - session metadata:
    - `session_kind=sgx_dependency_slot_clear_mode_compare`
    - `session_outcome=completed`
    - `typed_health_overall=remote_usable`
    - `legacy_health_overall=remote_usable`
    - `last_overall=remote_usable`
  - typed/default dependency wrapper path completed cleanly:
    - `typed_submit_setup=ioctl`
    - `typed_object_setup=ioctl`
    - `typed_range_discovery=ioctl`
    - `typed_dependency_setup=ioctl`
    - `typed_verification_mode=ioctl`
    - `typed_dependency_interface=indexed`
    - `typed_submit_a_id=54`
    - `typed_submit_b_id=55`
    - `typed_submit_c_id=56`
    - `typed_submit_d_id=57`
    - `typed_submit_e_id=58`
    - post-clear state:
      - `typed_dependency4_submit_id_after_clear=0`
      - `typed_dependency4_flags_after_clear=0x0`
      - `typed_submit_b_owner_state_after_post_clear=complete`
      - `typed_submit_b_validate_count_after_post_clear=21`
      - `typed_submit_b_issue_count_after_post_clear=1`
      - `typed_submit_b_transition_count_after_post_clear=2`
    - final typed cleanup:
      - `typed_live_submit_count_after_drop=0`
      - `typed_live_object_count_after_drop=0`
      - `typed_range_sgx_object_count_after_drop=0`
      - `typed_range_pf_bound_count_after_drop=0`
  - explicit legacy dependency wrapper path completed cleanly:
    - `legacy_submit_setup=sysfs`
    - `legacy_object_setup=sysfs`
    - `legacy_range_discovery=sysfs`
    - `legacy_dependency_setup=sysfs`
    - `legacy_verification_mode=sysfs`
    - `legacy_dependency_interface=indexed`
    - `legacy_submit_a_id=59`
    - `legacy_submit_b_id=60`
    - `legacy_submit_c_id=61`
    - `legacy_submit_d_id=62`
    - `legacy_submit_e_id=63`
    - cleanup markers:
      - `legacy_has_submits_after_drop=SUBMITS_AFTER_DROP`
      - `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`
      - `legacy_has_coord_after_drop=COORD_STATE_AFTER_DROP`
  - the session summary helper now surfaces the dependency compare bundle as a
    real compare session with:
    - `last_state=remote_usable`
    - `module_srcversion=D43951B174F84BE870230AB`
  - final target state remained clean:
    - `overall=remote_usable`
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

### Indexed dependency-array typed-vs-legacy compare session

- Extended the generic multi-submit compare harness so thin scenario wrappers
  can override:
  - `SESSION_KIND`
- Added one thin compare wrapper for the indexed dependency-array scenario:
  - `scripts/run-vaio-sgx-indexed-dependency-array-mode-compare.sh`
- It reuses:
  - `scripts/run-vaio-sgx-multi-submit-mode-compare.sh`
  with:
  - `MULTI_SUBMIT_SCRIPT=scripts/run-vaio-sgx-indexed-dependency-array.sh`
- Documented it in:
  - `docs/93-sgx-indexed-dependency-array-mode-compare-validation.md`
- Validated on hardware in:
  - `logs/vaio-compare/20260511-134740-sgx-indexed-dependency-array-mode-compare`
- Verified on the VAIO:
  - session metadata:
    - `session_kind=sgx_indexed_dependency_array_mode_compare`
    - `session_outcome=completed`
    - `typed_health_overall=remote_usable`
    - `legacy_health_overall=remote_usable`
    - `last_overall=remote_usable`
  - typed/default indexed wrapper path completed cleanly:
    - `typed_submit_setup=ioctl`
    - `typed_object_setup=ioctl`
    - `typed_range_discovery=ioctl`
    - `typed_dependency_setup=ioctl`
    - `typed_verification_mode=ioctl`
    - `typed_dependency_interface=indexed`
    - `typed_submit_a_id=64`
    - `typed_submit_b_id=65`
    - `typed_submit_c_id=66`
    - `typed_submit_d_id=67`
    - terminal typed state:
      - `typed_submit_validate_count=5`
      - `typed_submit_issue_count=1`
      - `typed_submit_transition_count=2`
      - `typed_submit_owner_state=complete`
    - final typed cleanup:
      - `typed_live_submit_count=0`
      - `typed_live_object_count=0`
      - `typed_range_sgx_object_count=0`
      - `typed_range_pf_bound_count=0`
  - explicit legacy indexed wrapper path completed cleanly:
    - `legacy_dependency_interface=indexed`
    - `legacy_submit_a_id=68`
    - `legacy_submit_b_id=69`
    - `legacy_submit_c_id=70`
    - `legacy_submit_d_id=71`
    - cleanup markers:
      - `legacy_has_submits_after_drop=SUBMITS_AFTER_DROP`
      - `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`
      - `legacy_has_coord_after_drop=COORD_STATE_AFTER_DROP`
  - `scripts/summarize-vaio-sessions.sh` now surfaces the bundle with the
    distinct kind:
    - `sgx_indexed_dependency_array_mode_compare`
  - final target state remained clean:
    - `overall=remote_usable`
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

### Relative dependency-ticket fix plus multi-source ticket compare

- Fixed a real runner bug in `scripts/run-vaio-sgx-multi-submit.sh`:
  - dependency-ticket values for later lanes were being treated as absolute
    global completion-ticket thresholds
  - that made `scripts/run-vaio-sgx-multi-source-ticket-dependency.sh`
    silently wrong once the driver's persistent completion counter moved far
    above the old absolute `3` threshold
- Added support for relative dependency-ticket syntax:
  - `+<n>`
  - resolved on the target as:
    - `coord_complete_ticket_baseline + n`
- Made the baseline resolver handle both live coord formats:
  - typed query:
    - `coord_complete_ticket=...`
  - legacy sysfs:
    - `complete_ticket_counter=...`
- Updated the dedicated wrapper:
  - `scripts/run-vaio-sgx-multi-source-ticket-dependency.sh`
  - lane 2 default is now `+3` instead of absolute `3`
- Added the compare wrapper:
  - `scripts/run-vaio-sgx-multi-source-ticket-dependency-mode-compare.sh`
- Documented it in:
  - `docs/94-sgx-relative-dependency-ticket-fix.md`

- Direct scenario revalidation on the VAIO:
  - `scripts/run-vaio-sgx-multi-source-ticket-dependency.sh`
  - returned `RC=0`
  - live target log showed the resolved relative lane-2 ticket:
    - `lab submit depticket_slot submit_id=82 slot=2 required_ticket=75`

- Typed-versus-legacy compare validation on hardware:
  - `logs/vaio-compare/20260511-140046-sgx-multi-source-ticket-dependency-mode-compare`
  - saved session result:
    - `session_kind=sgx_multi_source_ticket_dependency_mode_compare`
    - `session_outcome=completed`
    - `typed_health_overall=remote_usable`
    - `legacy_health_overall=remote_usable`
    - `last_overall=remote_usable`
  - typed/default path:
    - `typed_submit_setup=ioctl`
    - `typed_object_setup=ioctl`
    - `typed_range_discovery=ioctl`
    - `typed_dependency_setup=ioctl`
    - `typed_verification_mode=ioctl`
    - `typed_dependency_interface=named`
    - `typed_submit_a_id=90`
    - `typed_submit_b_id=91`
    - `typed_submit_c_id=92`
    - terminal typed state:
      - `typed_submit_validate_count=8`
      - `typed_submit_issue_count=2`
      - `typed_submit_transition_count=4`
      - `typed_submit_owner_state=complete`
      - `typed_live_submit_count=0`
      - `typed_live_object_count=0`
      - `typed_range_sgx_object_count=0`
      - `typed_range_pf_bound_count=0`
    - typed relative-ticket evidence:
      - `COORD_COMPLETE_TICKET_BASELINE=84`
      - `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET=87`
      - `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET_RELATIVE=3`
    - typed scenario still hit the expected gating checkpoints:
      - `SUBMIT_B_AFTER_AUTO_DEPSTATE_AFTER_C_FIRST_COMPLETE`
      - `SUBMIT_B_DEPENDENCY_WAIT_AFTER_C_FIRST_COMPLETE`
  - explicit legacy path:
    - `legacy_dependency_interface=named`
    - `legacy_submit_a_id=93`
    - `legacy_submit_b_id=94`
    - `legacy_submit_c_id=95`
    - legacy relative-ticket evidence:
      - `COORD_COMPLETE_TICKET_BASELINE=88`
      - `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET=91`
      - `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET_RELATIVE=3`
    - legacy cleanup markers:
      - `legacy_has_submits_after_drop=SUBMITS_AFTER_DROP`
      - `legacy_has_objects_after_drop=OBJECTS_AFTER_DROP`
      - `legacy_has_coord_after_drop=COORD_STATE_AFTER_DROP`
  - final target state remained clean:
    - `overall=remote_usable`
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

### Compare reset helper plus legacy range-baseline fix

- Added:
  - `scripts/run-vaio-sgx-lab-reset.sh`
- Updated compare wrappers to call it before the typed branch and again
  between the typed and legacy branches:
  - `scripts/run-vaio-sgx-object-mode-compare.sh`
  - `scripts/run-vaio-sgx-submit-mode-compare.sh`
  - `scripts/run-vaio-sgx-multi-submit-mode-compare.sh`
  - `scripts/run-vaio-sgx-dependency-slot-clear-mode-compare.sh`
- Updated `scripts/run-vaio-sgx-multi-submit.sh` so the before/during live GEM
  baseline now prefers the typed range-query helper when that path is
  available, even if the scenario is otherwise running in explicit legacy mode
- Root causes addressed:
  - warm-target compare sessions could hand off from typed to legacy before
    remote holder teardown had fully settled
  - the direct legacy multi-submit path could still derive
    `MAX_BEFORE_RANGE_ID` from the truncated `poulsbo_gtt_state` dump
- Documented in:
  - `docs/95-sgx-compare-reset-and-legacy-range-baseline-fix.md`

- Local validation:
  - `bash -n` passed on:
    - `scripts/run-vaio-sgx-lab-reset.sh`
    - `scripts/run-vaio-sgx-multi-submit.sh`
    - `scripts/run-vaio-sgx-dependency-slot-clear-mode-compare.sh`
    - `scripts/run-vaio-sgx-multi-submit-mode-compare.sh`
    - `scripts/run-vaio-sgx-object-mode-compare.sh`
    - `scripts/run-vaio-sgx-submit-mode-compare.sh`
  - direct reset-helper smoke check:
    - `./scripts/run-vaio-sgx-lab-reset.sh`
    - `RC=0`
    - first attempt cleared:
      - `object_result_count=2`
      - `coord_live_submit_count=0`
      - `coord_live_object_count=0`
      - `gtt_state_range_sgx_object_count=0`
      - `gtt_state_range_pf_bound_count=0`

- Hardware validation on the VAIO:
  - bundle:
    - `logs/vaio-compare/20260511-141658-sgx-dependency-slot-clear-mode-compare`
  - saved result:
    - `session_outcome=completed`
    - `pre_reset_rc=0`
    - `typed_rc=0`
    - `typed_health_overall=remote_usable`
    - `inter_reset_rc=0`
    - `legacy_rc=0`
    - `legacy_health_overall=remote_usable`
    - `last_overall=remote_usable`
  - pre-reset evidence:
    - `object_result_count=2`
    - all typed coord and GTT live counters converged to zero
  - typed/default dependency-slot-clear branch completed cleanly:
    - `typed_submit_a_id=116`
    - `typed_submit_b_id=117`
    - `typed_submit_c_id=118`
    - `typed_submit_d_id=119`
    - `typed_submit_e_id=120`
    - `typed_range_ids=685..709`
    - `typed_dependency4_submit_id_after_clear=0`
    - `typed_dependency4_flags_after_clear=0x0`
  - typed relative-ticket evidence:
    - `COORD_COMPLETE_TICKET_BASELINE=112`
    - `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET=114`
    - `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET_RELATIVE=2`
    - `SUBMIT_B_DEPENDENCY3_COMPLETE_TICKET=115`
    - `SUBMIT_B_DEPENDENCY3_COMPLETE_TICKET_RELATIVE=3`
    - `SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET=116`
    - `SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET_RELATIVE=4`
  - explicit legacy branch now also completed cleanly on the same warm target:
    - `legacy_submit_setup=sysfs`
    - `legacy_object_setup=sysfs`
    - `legacy_range_discovery=sysfs`
    - `legacy_dependency_setup=sysfs`
    - `legacy_verification_mode=sysfs`
    - `legacy_dependency_interface=indexed`
    - `legacy_submit_a_id=121`
    - `legacy_submit_b_id=122`
    - `legacy_submit_c_id=123`
    - `legacy_submit_d_id=124`
    - `legacy_submit_e_id=125`
    - `legacy_range_ids=710..734`
  - legacy relative-ticket evidence:
    - `COORD_COMPLETE_TICKET_BASELINE=117`
    - `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET=119`
    - `SUBMIT_B_DEPENDENCY2_COMPLETE_TICKET_RELATIVE=2`
    - `SUBMIT_B_DEPENDENCY3_COMPLETE_TICKET=120`
    - `SUBMIT_B_DEPENDENCY3_COMPLETE_TICKET_RELATIVE=3`
    - `SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET=121`
    - `SUBMIT_B_DEPENDENCY4_COMPLETE_TICKET_RELATIVE=4`
- final target state stayed clean:
  - `overall=remote_usable`
  - `driver_srcversion=D43951B174F84BE870230AB`
  - `override_module_size=385384`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

- Documented in:
  - `docs/108-sgx-lab-submit-prepare-op-ioctl.md`

- Submit prepare materialization batch:
  - kernel changes:
    - added typed submit-op `prepare`
    - added `prepare_count` and `last_prepare_jiffies` to the submit query
      surface
    - added one kernel-side materialization pass that writes:
      - queue header words
      - CCB words
      - packet image
      - status values
      - sync block
      - owner block
      - dependency blocks
    - and then re-validates the prepared image
  - userspace changes:
    - `workloads/psb_sgx_lab_submit_op.c` now supports `prepare`
    - `workloads/psb_sgx_lab_query.c` now prints prepare counters
    - `workloads/psb_sgx_lab_dump.c` now includes prepare count in text and
      JSON output
    - `scripts/run-vaio-sgx-ioctl-submit-op.sh` now calls `prepare focus`
    - `scripts/run-vaio-sgx-multi-submit.sh` now inserts one typed `prepare`
      call per submit during typed setup
  - local checks passed:
    - `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
    - `bash -n scripts/run-vaio-sgx-multi-submit.sh`
    - `./scripts/docker-build-workloads-i386.sh`
    - `./scripts/docker-build-vaio-gma500.sh`
    - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
  - guarded deploy moved the VAIO from:
    - `driver_srcversion=933E555C1A18400A2CF86AE`
    - `override_module_size=387664`
    to:
    - `driver_srcversion=AE18527FD5F6C10BE6C85A5`
    - `override_module_size=390608`
  - canonical live bundle:
    - `logs/vaio-ioctl/20260511-172038-sgx-submit-prepare-live`
  - focused single-submit result:
    - typed `prepare focus` executed once
    - post-prepare state:
      - `submit_prepare_count=1`
      - `submit_last_prepare_jiffies=4294940548`
    - final state:
      - `submit_validate_count=1`
      - `submit_issue_count=1`
      - `submit_transition_count=2`
      - `submit_owner_state_name=complete`
  - typed multi-submit result:
    - canonical rerun used live submits:
      - `submit_id=5`
      - `submit_id=6`
    - both submits were prepared exactly once before progression
    - final live state:
      - submit A:
        - `submit_prepare_count=1`
        - `submit_validate_count=11`
        - `submit_issue_count=3`
        - `submit_transition_count=6`
      - submit B:
        - `submit_prepare_count=1`
        - `submit_validate_count=11`
        - `submit_issue_count=2`
        - `submit_transition_count=4`
    - final typed cleanup:
      - `coord_live_submit_count=0`
      - `coord_live_object_count=0`
      - `gtt_state_range_sgx_object_count=0`
      - `gtt_state_range_pf_bound_count=0`
  - target stayed clean:
    - `overall=remote_usable`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

### Range-query submit-match selection on shared GPU VAs

- Extended the bounded `QUERY_RANGE` GPU-VA path so it can select one concrete
  submit-reference ordinal directly through `gpuva-submit`.
- Added typed result fields for selected submit resolution:
  - `sgx_resolved_submit_found`
  - `sgx_resolved_submit_id`
  - `sgx_resolved_submit_slot`
  - `sgx_resolved_submit_match_index`
- Fixed two real kernel bugs in the range-query path:
  - submit-list scanning now holds `sgx_lab_submit_mutex`
  - the ioctl exit path no longer unlocks `sgx_lab_submit_mutex` without
    locking it first
- Added the userspace `gpuva-submit` mode to
  `workloads/psb_sgx_lab_range_query.c`.
- Extended `scripts/run-vaio-sgx-ioctl-range-query.sh` for the shared-control
  two-submit case and added the dedicated wrapper
  `scripts/run-vaio-sgx-ioctl-range-query-submit-match.sh`.
- Documented the milestone in
  `docs/107-sgx-lab-range-query-submit-match-validation.md`.

Validated result on `10.1.11.218`:

- the VAIO booted the new locally built override module with
  `srcversion 933E555C1A18400A2CF86AE`
- the canonical live bundle is
  `logs/vaio-ioctl/20260511-165702-sgx-range-query-submit-match-live`
- on shared control-object GPU VA `0x807c0000`:
  - plain `gpuva` reported `submit_ref_count=2`
  - `gpuva-submit 0` resolved submit `1`
  - `gpuva-submit 1` resolved submit `2`
  - after primary submit drop:
    - plain `gpuva` reported `submit_ref_count=1`
    - `gpuva-submit 0` resolved submit `2`
    - `gpuva-submit 1` no longer resolved a selected submit
  - after final submit drop:
    - plain `gpuva` reported `submit_ref_count=0`
- final target state stayed clean:
  - `overall=remote_usable`
  - `override_module_size=387664`
  - `rollback_pending_present=0`

Conclusion:

- the bounded range-query family can now disambiguate concrete shared-object
  submit owners by ordinal from one GPU VA, not just count them or name the
  first one
- both read-side GPU-VA query families now cover shared-object multi-submit
  selection without falling back to sysfs text dumps

- 2026-05-11:
  - Added a canonical wrapper-level enumeration flow for the already-validated
    direct GPU-VA submit-match path:
    - `ENUMERATE_MATCHES=1`
    - `scripts/run-vaio-sgx-ioctl-submit-query-gpuva-list.sh`
  - Userspace/script changes:
    - `scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
    - `scripts/run-vaio-sgx-ioctl-submit-query-gpuva-list.sh`
    - the base validator can now record:
      - full match list while both submits exist
      - full match list after userspace holder exit
      - contracted match list after the primary submit drops
  - Local checks passed:
    - `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
    - `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva-list.sh`
  - Canonical live bundle:
    - `logs/vaio-ioctl/20260511-164757-sgx-submit-query-gpuva-list-live`
    - `session_kind=sgx_submit_query_gpuva_list_validation`
    - `session_outcome=completed`
  - Canonical live evidence:
    - staged submits:
      - `SUBMIT_ID=3`
      - `SECOND_SUBMIT_ID=4`
    - while both submits exist:
      - `SUBMIT_QUERY_GPUVA_LIST_LIVE_0`
        - `submit_resolved_id=3`
        - `submit_matching_submit_count=2`
        - `submit_resolved_match_index=0`
      - `SUBMIT_QUERY_GPUVA_LIST_LIVE_1`
        - `submit_resolved_id=4`
        - `submit_matching_submit_count=2`
        - `submit_resolved_match_index=1`
    - after userspace holder exit:
      - `SUBMIT_QUERY_GPUVA_LIST_POST_PROBE_EXIT_0`
        - `submit_resolved_id=3`
      - `SUBMIT_QUERY_GPUVA_LIST_POST_PROBE_EXIT_1`
        - `submit_resolved_id=4`
    - after primary-submit drop:
      - `SUBMIT_QUERY_GPUVA_LIST_AFTER_PRIMARY_DROP_0`
        - `submit_resolved_id=4`
        - `submit_matching_submit_count=1`
        - `submit_resolved_match_index=0`
  - Documentation synced:
    - `docs/106-sgx-lab-submit-query-gpuva-list-validation.md`
    - `README.md`
    - `docs/10-ubuntu-18.04-target.md`
    - `docs/12-active-todo.md`
  - Final target state stayed clean:
    - `overall=remote_usable`
    - `driver_srcversion=0F9AE3730BF455D57D74E17`
    - `override_module_size=387528`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

- 2026-05-11:
  - Extended the direct GPU-VA submit-query path so one typed lookup can now
    select a concrete matching submit ordinal while more than one live submit
    still references the same VA:
    - `DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_GPU_VA_MATCH_INDEX`
    - `query_match_index`
    - `resolved_match_index`
  - Kernel-side changes:
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
    - the GPU-VA submit resolver now:
      - counts all matches through `matching_submit_count`
      - resolves a specific match ordinal when the new flag is present
      - returns `-ENOENT` when that requested ordinal no longer exists
  - Userspace changes:
    - `workloads/psb_sgx_lab_query.c`
    - `scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
    - `scripts/run-vaio-sgx-ioctl-submit-query-gpuva-match.sh`
    - the query helper now supports:
      - `gpuva-match <gpu_va> <match_index>`
    - it now prints:
      - `submit_query_match_index`
      - `submit_resolved_match_index`
    - the base validator now proves explicit selection of submit A and submit
      B before any drop
    - the dedicated wrapper fixes that scenario as the canonical live path
  - Local checks passed:
    - `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
    - `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva-match.sh`
    - `./scripts/docker-build-workloads-i386.sh`
    - `./scripts/docker-build-vaio-gma500.sh`
    - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
  - Guarded deploy succeeded on the VAIO:
    - `driver_srcversion=0F9AE3730BF455D57D74E17`
    - `override_module_size=387528`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
    - `rollback_backup_size=387336`
  - Canonical live bundle:
    - `logs/vaio-ioctl/20260511-163739-sgx-submit-query-gpuva-match-live`
    - `session_kind=sgx_submit_query_gpuva_match_validation`
    - `session_outcome=completed`
  - Canonical live evidence on GPU VA `0x807c0000`:
    - staged ranges:
      - `RANGE_IDS=7 9 10 11 12`
    - staged objects:
      - `OBJECT_IDS=1 2 3 4 5`
    - staged submits:
      - `SUBMIT_ID=1`
      - `SECOND_SUBMIT_ID=2`
    - while both submits exist:
      - plain `gpuva` resolved the first match:
        - `submit_resolved_id=1`
        - `submit_matching_submit_count=2`
        - `submit_resolved_match_index=0`
      - `gpuva-match 0` resolved submit A:
        - `submit_query_match_index=0`
        - `submit_resolved_id=1`
        - `submit_matching_submit_count=2`
        - `submit_resolved_match_index=0`
      - `gpuva-match 1` resolved submit B:
        - `submit_query_match_index=1`
        - `submit_resolved_id=2`
        - `submit_matching_submit_count=2`
        - `submit_resolved_match_index=1`
    - after userspace holder exit:
      - `gpuva-match 1` still resolved submit B with:
        - `submit_matching_submit_count=2`
        - `submit_resolved_match_index=1`
    - after typed submit-A drop:
      - plain `gpuva` and `gpuva-match 0` both resolved submit B with:
        - `submit_resolved_id=2`
        - `submit_matching_submit_count=1`
        - `submit_resolved_match_index=0`
      - `gpuva-match 1` failed cleanly:
        - `SUBMIT_QUERY_GPUVA_MATCH1_AFTER_PRIMARY_DROP_RC=3`
        - `submit_ioctl_ret=-1 errno=2`
    - after typed submit-B drop:
      - plain `gpuva` failed cleanly too:
        - `SUBMIT_QUERY_GPUVA_AFTER_DROP_RC=3`
        - `submit_ioctl_ret=-1 errno=2`
    - after typed object cleanup:
      - `object_result_count=5`
  - Documentation synced:
    - `docs/105-sgx-lab-submit-query-gpuva-match-validation.md`
    - `README.md`
    - `docs/10-ubuntu-18.04-target.md`
    - `docs/12-active-todo.md`
    - `workloads/README.md`
  - Final target state stayed clean:
    - `overall=remote_usable`
    - `driver_srcversion=0F9AE3730BF455D57D74E17`
    - `override_module_size=387528`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

- 2026-05-11:
  - Extended the direct GPU-VA submit-query path so one typed lookup now also
    reports how many live submits currently match that same VA:
    - `matching_submit_count`
  - Kernel-side changes:
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
    - the GPU-VA submit resolver now keeps the first matching submit as the
      resolved result and counts later matches on the same VA
    - explicit-ID, focused-submit, and submit-index queries now return
      `matching_submit_count=1` on success
  - Userspace changes:
    - `workloads/psb_sgx_lab_query.c`
    - `scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
    - `scripts/run-vaio-sgx-ioctl-submit-query-gpuva-multi-ref.sh`
    - the query helper now prints:
      - `submit_matching_submit_count`
    - the base validator now supports `SHARED_CONTROL_SUBMITS=2`
    - the dedicated wrapper now locks the canonical shared-control two-submit
      handoff path onto that same validator
  - Local checks passed:
    - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
    - `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
    - `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva-multi-ref.sh`
    - `./scripts/docker-build-workloads-i386.sh`
    - `./scripts/docker-build-vaio-gma500.sh`
  - Guarded deploy succeeded on the VAIO:
    - `driver_srcversion=11920D5153C981A7E8A264A`
    - `override_module_size=387336`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
    - `rollback_backup_size=387144`
  - Canonical live bundle:
    - `logs/vaio-ioctl/20260511-162454-sgx-submit-query-gpuva-multi-ref-live`
    - `session_kind=sgx_submit_query_gpuva_multi_ref_validation`
    - `session_outcome=completed`
  - Canonical live evidence on GPU VA `0x807c0000`:
    - staged ranges:
      - `RANGE_IDS=7 9 10 11 12`
    - staged objects:
      - `OBJECT_IDS=1 2 3 4 5`
    - staged submits:
      - `SUBMIT_ID=1`
      - `SECOND_SUBMIT_ID=2`
    - while both submits exist:
      - `submit_resolved_id=1`
      - `submit_resolved_slot_name=control`
      - `submit_resolved_object_id=1`
      - `submit_resolved_range_id=7`
      - `submit_resolved_range_type_name=gem`
      - `submit_resolved_object_role_name=control`
      - `submit_resolved_object_class_name=packet`
      - `submit_resolved_object_gpu_use_name=control_ccb`
      - `submit_resolved_object_expected_context_name=pf`
      - `submit_query_gpu_va_offset=0x00001000`
      - `submit_matching_submit_count=2`
    - after userspace holder exit:
      - submit A still resolved first
      - `submit_matching_submit_count=2`
    - after typed submit-A drop:
      - the same GPU VA resolved submit B instead:
        - `submit_resolved_id=2`
        - `submit_matching_submit_count=1`
    - after typed submit-B drop:
      - `SUBMIT_QUERY_GPUVA_AFTER_DROP_RC=3`
      - `submit_ioctl_ret=-1 errno=2`
    - after typed object cleanup:
      - `object_result_count=5`
  - Documentation synced:
    - `docs/104-sgx-lab-submit-query-gpuva-multi-ref-validation.md`
    - `README.md`
    - `docs/10-ubuntu-18.04-target.md`
    - `docs/12-active-todo.md`
    - `workloads/README.md`
  - Final target state stayed clean:
    - `overall=remote_usable`
    - `driver_srcversion=11920D5153C981A7E8A264A`
    - `override_module_size=387336`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

- 2026-05-11:
  - Extended the direct GPU-VA submit-query path so one typed lookup now also
    returns the matched object’s semantic metadata:
    - `resolved_object_role`
    - `resolved_object_class`
    - `resolved_object_gpu_use`
    - `resolved_object_expected_context`
  - Kernel-side changes:
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
    - once `QUERY_SUBMIT gpuva` resolves the object ID, the submit-query fill
      path now also copies that object’s semantic metadata into the typed
      result
  - Userspace changes:
    - `workloads/psb_sgx_lab_query.c`
    - `scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
    - the query helper now prints resolved object role/class/gpu_use/context
    - the focused validator now also asserts:
      - `control`
      - `packet`
      - `control_ccb`
      - `pf`
      on the canonical control-object GPU VA before and after holder exit
  - Local checks passed:
    - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
    - `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
    - `./scripts/docker-build-workloads-i386.sh`
    - `./scripts/docker-build-vaio-gma500.sh`
  - Guarded deploy succeeded on the VAIO:
    - `driver_srcversion=19426C7ED791DCAAD913C11`
    - `override_module_size=387144`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
    - `rollback_backup_size=387080`
  - Canonical live bundle:
    - `logs/vaio-ioctl/20260511-161543-sgx-submit-query-gpuva-object-live`
    - `session_kind=sgx_submit_query_gpuva_object_validation`
    - `session_outcome=completed`
  - Canonical live evidence on GPU VA `0x807c0000`:
    - staged ranges:
      - `RANGE_IDS=7 9 10 11 12`
    - staged objects:
      - `OBJECT_IDS=1 2 3 4 5`
    - staged submit:
      - `SUBMIT_ID=1`
    - while the submit exists:
      - `submit_resolved_id=1`
      - `submit_resolved_slot_name=control`
      - `submit_resolved_object_id=1`
      - `submit_resolved_range_id=7`
      - `submit_resolved_range_type_name=gem`
      - `submit_resolved_object_role_name=control`
      - `submit_resolved_object_class_name=packet`
      - `submit_resolved_object_gpu_use_name=control_ccb`
      - `submit_resolved_object_expected_context_name=pf`
      - `submit_query_gpu_va_offset=0x00001000`
    - after userspace holder exit:
      - the same `submit / slot / object / range / role / class / gpu_use /
        context / offset` tuple still resolved
    - after typed submit drop:
      - `SUBMIT_QUERY_GPUVA_AFTER_DROP_RC=3`
      - `submit_ioctl_ret=-1 errno=2`
    - after typed object cleanup:
      - `object_result_count=5`
  - Documentation synced:
    - `docs/103-sgx-lab-submit-query-gpuva-object-validation.md`
    - `README.md`
    - `docs/10-ubuntu-18.04-target.md`
    - `docs/12-active-todo.md`
    - `workloads/README.md`
  - Final target state stayed clean:
    - `overall=remote_usable`
    - `driver_srcversion=19426C7ED791DCAAD913C11`
    - `override_module_size=387144`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

- 2026-05-11:
  - Extended the direct GPU-VA submit-query path so one typed lookup now also
    returns the backing range identity:
    - `resolved_range_id`
    - `resolved_range_type`
  - Kernel-side changes:
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
    - the internal GPU-VA submit resolver now also returns:
      - `gtt_range.debug_id`
      - lab-visible range type via `poulsbo_sgx_lab_range_type()`
  - Userspace changes:
    - `workloads/psb_sgx_lab_query.c`
    - `scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
    - the query helper now prints:
      - `submit_resolved_range_id`
      - `submit_resolved_range_type`
      - `submit_resolved_range_type_name`
    - the focused validator now also asserts the resolved backing range ID and
      type on the canonical control-object GPU VA
  - Local checks passed:
    - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
    - `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
    - `./scripts/docker-build-workloads-i386.sh`
    - `./scripts/docker-build-vaio-gma500.sh`
  - Guarded deploy succeeded on the VAIO:
    - `driver_srcversion=9864A6D5176BBB851C0B51E`
    - `override_module_size=387080`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
    - `rollback_backup_size=386888`
  - Canonical live bundle:
    - `logs/vaio-ioctl/20260511-160747-sgx-submit-query-gpuva-range-live`
    - `session_kind=sgx_submit_query_gpuva_range_validation`
    - `session_outcome=completed`
  - Canonical live evidence on GPU VA `0x807c0000`:
    - staged ranges:
      - `RANGE_IDS=7 9 10 11 12`
    - staged objects:
      - `OBJECT_IDS=1 2 3 4 5`
    - staged submit:
      - `SUBMIT_ID=1`
    - while the submit exists:
      - `submit_resolved_id=1`
      - `submit_resolved_slot_name=control`
      - `submit_resolved_object_id=1`
      - `submit_resolved_range_id=7`
      - `submit_resolved_range_type_name=gem`
      - `submit_query_gpu_va_offset=0x00001000`
    - after userspace holder exit:
      - the same `submit / slot / object / range / type / offset` tuple still
        resolved
    - after typed submit drop:
      - `SUBMIT_QUERY_GPUVA_AFTER_DROP_RC=3`
      - `submit_ioctl_ret=-1 errno=2`
    - after typed object cleanup:
      - `object_result_count=5`
  - Documentation synced:
    - `docs/102-sgx-lab-submit-query-gpuva-range-validation.md`
    - `README.md`
    - `docs/10-ubuntu-18.04-target.md`
    - `docs/12-active-todo.md`
    - `workloads/README.md`
  - Final target state stayed clean:
    - `overall=remote_usable`
    - `driver_srcversion=9864A6D5176BBB851C0B51E`
    - `override_module_size=387080`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

- 2026-05-11:
  - Extended the bounded submit-query path so one typed SGX lab lookup can now
    resolve a live submit directly from GPU VA:
    - `DRM_PSB_SGX_LAB_QUERY_SUBMIT_USE_GPU_VA`
    - `query_gpu_va`
    - `resolved_slot`
    - `resolved_object_id`
    - `query_gpu_va_offset`
  - Kernel-side changes:
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
    - `QUERY_SUBMIT` selectors are now exclusive across:
      - explicit submit ID
      - focused submit
      - live submit index
      - GPU VA
    - the submit-query path now scans the live submit slots and resolves the
      first attached object range that contains the requested GPU VA
  - Userspace changes:
    - `workloads/psb_sgx_lab_query.c`
    - `scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
    - the query helper now supports:
      - `gpuva <gpu_va>`
    - the focused validator now stages five objects, attaches them to one
      PF-context submit, resolves the control-object GPU VA, proves direct
      submit lookup on that VA, proves that readback survives holder exit, and
      then proves `ENOENT` after typed submit drop
  - Local checks passed:
    - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
    - `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
    - `./scripts/docker-build-workloads-i386.sh`
    - `./scripts/docker-build-vaio-gma500.sh`
  - Guarded deploy succeeded on the VAIO:
    - `driver_srcversion=CA6AF1E225E6F5B90E66C7D`
    - `override_module_size=386888`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
    - `rollback_backup_size=386328`
  - Canonical live bundle:
    - `logs/vaio-ioctl/20260511-155105-sgx-submit-query-gpuva-live`
    - `session_kind=sgx_submit_query_gpuva_validation`
    - `session_outcome=completed`
  - Canonical live evidence on GPU VA `0x807c0000`:
    - staged ranges:
      - `RANGE_IDS=5 7 9 40 41`
    - staged objects:
      - `OBJECT_IDS=31 32 33 34 35`
    - staged submit:
      - `SUBMIT_ID=7`
    - while the submit exists:
      - `submit_resolved_id=7`
      - `submit_resolved_slot_name=control`
      - `submit_resolved_object_id=31`
      - `submit_query_gpu_va_offset=0x00001000`
      - `submit_validate_count=0`
      - `submit_issue_count=0`
      - `submit_transition_count=0`
    - after userspace holder exit:
      - the same `submit / slot / object / offset` tuple still resolved
    - after typed submit drop:
      - `coord_live_submit_count=0`
      - `submit_ioctl_ret=-1 errno=2`
    - after typed object cleanup:
      - `object_result_count=5`
  - Documentation synced:
    - `docs/101-sgx-lab-submit-query-gpuva-validation.md`
    - `README.md`
    - `docs/10-ubuntu-18.04-target.md`
    - `docs/12-active-todo.md`
    - `workloads/README.md`
  - Final target state stayed clean:
    - `overall=remote_usable`
    - `driver_srcversion=CA6AF1E225E6F5B90E66C7D`
    - `override_module_size=386888`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

- 2026-05-11:
  - Extended the typed GPU-VA range-query path so one live lookup now also
    reports SGX submit-reference state:
    - `sgx_submit_ref_count`
    - `sgx_first_submit_id`
    - `sgx_first_submit_slot`
  - Kernel-side changes:
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
    - `QUERY_RANGE` now scans the live SGX submit list for the adopted object
      behind the resolved range
    - the range-query lock order is now:
      - `sgx_lab_submit_mutex`
      - `sgx_lab_object_mutex`
      - `gtt_mutex`
  - Userspace changes:
    - `workloads/psb_sgx_lab_range_query.c`
    - `scripts/run-vaio-sgx-ioctl-range-query.sh`
    - the runner now stages `psb_sgx_lab_submit_op`, runs typed cleanup,
      creates one live submit, attaches the adopted object in `control`,
      validates the new submit-reference fields, drops the submit, and then
      drops the object
  - Local checks passed:
    - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
    - `bash -n scripts/run-vaio-sgx-ioctl-range-query.sh`
    - `./scripts/docker-build-workloads-i386.sh`
    - `./scripts/docker-build-vaio-gma500.sh`
  - Guarded deploy succeeded on the VAIO:
    - `driver_srcversion=AEB8DB73A3D6849EE3A59FA`
    - `override_module_size=386328`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
    - `rollback_backup_size=385984`
  - Canonical live bundle:
    - `logs/vaio-ioctl/20260511-152849-sgx-range-query-gpuva-submit-ref-live`
    - `session_kind=sgx_range_query_gpuva_submit_ref_validation`
    - `session_outcome=completed`
  - Canonical live evidence on GPU VA `0x807c0000`:
    - baseline fallback:
      - `BASELINE_RANGE_IDS=7 8 9 13 14 15`
      - `FRESH_RANGE_ID=7`
      - `FRESH_RANGE_IS_NEW=0`
    - before adopt:
      - `range_sgx_submit_ref_count=0`
      - `range_sgx_first_submit_slot_name=none`
    - after typed object metadata setup:
      - `range_sgx_object_role_name=control`
      - `range_sgx_object_class_name=packet`
      - `range_sgx_object_gpu_use_name=control_ccb`
      - `range_sgx_object_expected_context_name=pf`
      - `range_sgx_submit_ref_count=0`
    - after typed submit create plus `control` attach:
      - `submit_resolved_id=2`
      - `range_sgx_submit_ref_count=1`
      - `range_sgx_first_submit_id=2`
      - `range_sgx_first_submit_slot_name=control`
    - after userspace holder exit:
      - `range_sgx_submit_ref_count=1`
      - `range_sgx_first_submit_id=2`
      - `range_sgx_first_submit_slot_name=control`
    - after typed submit drop:
      - `range_sgx_submit_ref_count=0`
      - `range_sgx_first_submit_slot_name=none`
      - object semantics still `control / packet / control_ccb / pf`
    - after typed object drop:
      - `range_sgx_object_id=0`
      - `range_sgx_object_role_name=generic`
      - `range_sgx_submit_ref_count=0`
      - `range_sgx_first_submit_slot_name=none`
  - Documentation synced:
    - `docs/100-sgx-lab-range-query-gpuva-submit-ref-validation.md`
    - `README.md`
    - `docs/10-ubuntu-18.04-target.md`
    - `docs/12-active-todo.md`
    - `workloads/README.md`
  - Final target state stayed clean:
    - `overall=remote_usable`
    - `driver_srcversion=AEB8DB73A3D6849EE3A59FA`
    - `override_module_size=386328`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

- Documented in:
  - `docs/98-sgx-lab-range-query-gpuva-validation.md`

- Extended the bounded range-query path from `index` / `id` lookup to direct
  GPU-VA lookup:
  - kernel-side:
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
  - userspace-side:
    - `workloads/psb_sgx_lab_range_query.c`
    - `scripts/run-vaio-sgx-ioctl-range-query.sh`
  - added:
    - `DRM_PSB_SGX_LAB_QUERY_RANGE_USE_GPU_VA`
    - `query_gpu_va`
    - `query_gpu_va_offset`
    - userspace `gpuva <gpu_va> [gem]` mode
  - local validation:
    - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
    - `bash -n scripts/run-vaio-sgx-ioctl-range-query.sh`
    - `./scripts/docker-build-workloads-i386.sh`
    - `./scripts/docker-build-vaio-gma500.sh`
  - guarded deploy brought the VAIO to:
    - `driver_srcversion=C69057A8C6926335A87C2A9`
    - `override_module_size=385832`
  - canonical live bundle:
    - `logs/vaio-ioctl/20260511-150817-sgx-range-query-gpuva-live`
  - saved result:
    - `session_kind=sgx_range_query_gpuva_live`
    - `session_outcome=completed`
  - canonical live evidence:
    - `BASELINE_RANGE_IDS=5 7 8 9 16 17 18`
    - `FRESH_RANGE_ID=5`
    - `FRESH_RANGE_IS_NEW=0`
    - `range_query_gpu_va=0x807c0000`
    - `range_query_gpu_va_offset=0x00001000`
    - `range_resolved_id=5`
    - before adopt:
      - `range_sgx_object_id=0`
    - after typed object adopt:
      - `object_resolved_id=2`
      - `range_sgx_object_id=2`
    - after userspace holder exit:
      - `range_sgx_object_id=2`
    - after typed object drop:
      - `RANGE_QUERY_BY_GPUVA_AFTER_DROP_RC=0`
      - `range_sgx_object_id=0`
  - final target state stayed clean:
    - `overall=remote_usable`
    - `driver_srcversion=C69057A8C6926335A87C2A9`
    - `override_module_size=385832`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

- Documented in:
  - `docs/99-sgx-lab-range-query-gpuva-metadata-validation.md`

- Extended the bounded GPU-VA range-query path so one lookup also returns SGX
  object semantics:
  - kernel-side:
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
    - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
  - userspace-side:
    - `workloads/psb_sgx_lab_range_query.c`
    - `scripts/run-vaio-sgx-ioctl-range-query.sh`
  - added typed range-query fields:
    - `sgx_object_role`
    - `sgx_object_class`
    - `sgx_object_gpu_use`
    - `sgx_object_expected_context`
  - local validation:
    - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
    - `bash -n scripts/run-vaio-sgx-ioctl-range-query.sh`
    - `./scripts/docker-build-workloads-i386.sh`
    - `./scripts/docker-build-vaio-gma500.sh`
  - guarded deploy brought the VAIO to:
    - `driver_srcversion=1DAAA2E3085BB97D83AA3D7`
    - `override_module_size=385984`
  - canonical live bundle:
    - `logs/vaio-ioctl/20260511-151741-sgx-range-query-gpuva-metadata-live`
  - saved result:
    - `session_kind=sgx_range_query_gpuva_metadata_live`
    - `session_outcome=completed`
  - canonical live evidence:
    - `BASELINE_RANGE_IDS=6 7 8 9 10 11 12`
    - `FRESH_RANGE_ID=6`
    - `FRESH_RANGE_IS_NEW=0`
    - `range_query_gpu_va=0x807c0000`
    - `range_query_gpu_va_offset=0x00001000`
    - before adopt:
      - `range_sgx_object_id=0`
      - `range_sgx_object_role_name=generic`
      - `range_sgx_object_class_name=generic`
      - `range_sgx_object_gpu_use_name=generic`
      - `range_sgx_object_expected_context_name=auto`
    - after adopt plus metadata setup:
      - `object_resolved_id=1`
      - `range_sgx_object_id=1`
      - `range_sgx_object_role_name=control`
      - `range_sgx_object_class_name=packet`
      - `range_sgx_object_gpu_use_name=control_ccb`
      - `range_sgx_object_expected_context_name=pf`
    - after userspace holder exit:
      - `range_sgx_object_id=1`
      - `range_sgx_object_role_name=control`
      - `range_sgx_object_class_name=packet`
      - `range_sgx_object_gpu_use_name=control_ccb`
      - `range_sgx_object_expected_context_name=pf`
    - after drop:
      - `RANGE_QUERY_BY_GPUVA_AFTER_DROP_RC=0`
      - `range_sgx_object_id=0`
      - `range_sgx_object_role_name=generic`
      - `range_sgx_object_class_name=generic`
      - `range_sgx_object_gpu_use_name=generic`
      - `range_sgx_object_expected_context_name=auto`
  - final target state stayed clean:
    - `overall=remote_usable`
    - `driver_srcversion=1DAAA2E3085BB97D83AA3D7`
    - `override_module_size=385984`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

- Documented in:
  - `docs/97-sgx-generic-multi-submit-compare-reset-follow-up.md`

- Generic multi-submit compare follow-up after the warm-target reset and
  focused legacy submit range-query fixes:
  - completed live bundle:
    - `logs/vaio-compare/20260511-145551-sgx-multi-submit-mode-compare`
  - saved result:
    - `session_outcome=completed`
    - `pre_reset_rc=0`
    - `typed_rc=0`
    - `inter_reset_rc=0`
    - `legacy_rc=0`
    - `legacy_health_overall=remote_usable`
    - `last_overall=remote_usable`
  - typed/default branch:
    - `typed_submit_setup=ioctl`
    - `typed_object_setup=ioctl`
    - `typed_range_discovery=ioctl`
    - `typed_dependency_setup=ioctl`
    - `typed_verification_mode=ioctl`
    - `typed_dependency_interface=named`
    - `typed_submit_a_id=133`
    - `typed_submit_b_id=134`
    - `typed_range_ids=802 803 804 805 806 807 808 810 809 811`
    - `typed_submit_validate_count=11`
    - `typed_submit_issue_count=2`
    - `typed_submit_transition_count=4`
    - `typed_submit_owner_state=complete`
  - explicit legacy branch:
    - `legacy_dependency_interface=named`
    - `legacy_submit_a_id=135`
    - `legacy_submit_b_id=136`
    - `legacy_range_ids=812 813 814 815 816 817 818 819 820 821`
  - final target state stayed clean:
    - `overall=remote_usable`
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

- Added and hardware-validated a control-side execution-shaped submit
  `manifest` block:
  - kernel-side:
    - [psb_drm_lab.h](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h)
    - [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
  - userspace and runner coverage:
    - [psb_sgx_lab_submit_op.c](/root/Poulsbo/workloads/psb_sgx_lab_submit_op.c)
    - [psb_sgx_lab_query.c](/root/Poulsbo/workloads/psb_sgx_lab_query.c)
    - [psb_sgx_lab_dump.c](/root/Poulsbo/workloads/psb_sgx_lab_dump.c)
    - [run-vaio-sgx-ioctl-submit-op.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-op.sh)
    - [run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh)
  - local checks passed:
    - `./scripts/docker-build-workloads-i386.sh`
    - `./scripts/docker-build-vaio-gma500.sh`
    - `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
    - `bash -n scripts/run-vaio-sgx-multi-submit.sh`
    - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
  - guarded deploy:
    - [logs/vaio-deploy/20260511-222440-guarded-deploy](/root/Poulsbo/logs/vaio-deploy/20260511-222440-guarded-deploy)
    - live override moved from:
      - `srcversion 111E4E9C4E0999DA366432F`
      - `override_module_size=418808`
    - to:
      - `srcversion 9E29F79593770BEEC5AEFD2`
      - `override_module_size=423020`
  - canonical live bundles:
    - focused typed submit-op:
      - [logs/vaio-ioctl/20260511-222845-sgx-submit-manifest-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-222845-sgx-submit-manifest-live/summary.log)
    - typed multi-submit:
      - [logs/vaio-ioctl/20260511-222923-sgx-multi-submit-manifest-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-222923-sgx-multi-submit-manifest-live/summary.log)
  - focused live evidence:
    - `submit_manifest_offset=0x180`
    - manifest stage progression:
      - `none`
      - `prepared`
      - `armed`
      - `emitted`
      - `launched`
      - `dispatched`
      - `retired`
    - live queue and owner transitions:
      - after `issue`:
        - `submit_manifest_owner_state_name=queued`
        - `submit_manifest_queue_packet_count=2`
      - after `transition running`:
        - `submit_manifest_owner_state_name=running`
        - `submit_manifest_queue_packet_count=1`
      - after `transition complete`:
        - `submit_manifest_owner_state_name=complete`
        - `submit_manifest_queue_packet_count=0`
    - the control-side manifest GPU-VA32 words matched the expanded CCB:
      - `submit_manifest_pds_gpu_va32=0x80c6f000`
      - `submit_manifest_use_gpu_va32=0x81123000`
      - `submit_manifest_target_gpu_va32=0x81127000`
      - `submit_manifest_status_gpu_va32=0x8112b000`
    - final focused progression counts:
      - `submit_prepare_count=1`
      - `submit_arm_count=1`
      - `submit_emit_count=1`
      - `submit_launch_count=1`
      - `submit_dispatch_count=1`
      - `submit_retired_count=1`
      - `submit_issue_count=1`
      - `submit_transition_count=2`
  - typed multi-submit live evidence:
    - dependency-gated prepared manifest state was visible before issue:
      - `submit_manifest_stage_name=prepared`
      - `submit_manifest_stage_count=1`
      - `submit_manifest_owner_state_name=queued`
      - `submit_manifest_queue_packet_count=1`
      - `submit_manifest_dependency_active_count=1`
    - live typed setup reached:
      - `coord_live_submit_count=2`
      - `coord_live_object_count=10`
      - `gtt_state_range_sgx_object_count=10`
      - `gtt_state_range_pf_bound_count=10`
    - submit A final counts:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - final `submit_manifest_stage_name=retired`
      - final `submit_manifest_stage_count=3`
    - submit B final counts:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - final `submit_manifest_stage_name=retired`
      - final `submit_manifest_stage_count=2`
      - final `submit_manifest_dependency_active_count=1`
    - final typed cleanup returned:
      - `coord_live_submit_count=0`
      - `coord_live_object_count=0`
      - `gtt_state_range_sgx_object_count=0`
      - `gtt_state_range_pf_bound_count=0`
  - final target state stayed clean:
    - `overall=remote_usable`
    - `driver_srcversion=9E29F79593770BEEC5AEFD2`
    - `override_module_size=423020`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`
  - documented in:
    - [docs/118-sgx-lab-submit-manifest-op-ioctl.md](/root/Poulsbo/docs/118-sgx-lab-submit-manifest-op-ioctl.md)

## 2026-05-11 - SGX-shaped 8-dword CCB record validated on hardware

- expanded the current fixed CCB payload into a more SGX-shaped
  non-executing 8-dword record:
  - service address
  - cache control
  - data0
  - data1
  - `pds_gpu_va32`
  - `use_gpu_va32`
  - `target_gpu_va32`
  - `status_gpu_va32`
- kernel-side materialization and validation changed in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
- typed read-side helpers now expose the memory-backed CCB GPU-VA32 words in:
  - `workloads/psb_sgx_lab_query.c`
  - `workloads/psb_sgx_lab_dump.c`
- the focused submit runner now asserts that the memory-backed CCB record
  matches the attached object GPU VAs in:
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- local checks passed:
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
  - `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
  - `bash -n scripts/run-vaio-sgx-multi-submit.sh`
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- guarded deploy:
  - `logs/vaio-deploy/20260511-214857-guarded-deploy`
  - live override:
    - `srcversion 111E4E9C4E0999DA366432F`
    - `override_module_size=418808`
- canonical live validation:
  - focused typed submit-op:
    - `logs/vaio-ioctl/20260511-215431-sgx-submit-ccb-record-live/summary.log`
    - proved:
      - `submit_ccb_pds_gpu_va32=0x81123000`
      - `submit_ccb_use_gpu_va32=0x81127000`
      - `submit_ccb_target_gpu_va32=0x8112b000`
      - `submit_ccb_status_gpu_va32=0x8112f000`
      - matching attached:
        - `submit_pds_gpu_va=0x81123000`
        - `submit_use_gpu_va=0x81127000`
        - `submit_target_gpu_va=0x8112b000`
        - `submit_status_gpu_va=0x8112f000`
      - clean progression:
        - `submit_prepare_count=1`
        - `submit_validate_count=1`
        - `submit_issue_count=1`
        - `submit_transition_count=2`
  - typed multi-submit:
    - `logs/vaio-ioctl/20260511-215509-sgx-multi-submit-ccb-record-live/summary.log`
    - proved the same expanded record across both live submits:
      - submit A final CCB GPU-VA32 words:
        - `0x8112b000`
        - `0x8112f000`
        - `0x81133000`
        - `0x81137000`
      - submit B final CCB GPU-VA32 words:
        - `0x8113f000`
        - `0x81143000`
        - `0x81147000`
        - `0x8114b000`
      - final progression counts:
        - submit A:
          - `submit_emit_count=3`
          - `submit_launch_count=3`
          - `submit_issue_count=3`
          - `submit_transition_count=6`
        - submit B:
          - `submit_emit_count=2`
          - `submit_launch_count=2`
          - `submit_issue_count=2`
          - `submit_transition_count=4`
          - `submit_emit_dependency_active_count=1`
      - final typed cleanup returned:
        - `coord_live_submit_count=0`
        - `gtt_state_range_sgx_object_count=0`
- documented in:
  - `docs/117-sgx-lab-ccb-record-expansion.md`

- 2026-05-11: closed the typed submit `report` batch and validated it on the
  VAIO
  - kernel-side:
    - added the bounded typed `report` submit-op surface and memory-backed
      report block in:
      - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
      - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
    - fixed the refill-cycle report-stage resolver so a newer
      `prepare/arm/emit/launch` cycle is no longer masked by an older
      persisted `retired` snapshot
  - userspace and runner-side:
    - `workloads/psb_sgx_lab_submit_op.c` now resolves `report focus ...`
      through the generic selector helper
    - `scripts/run-vaio-sgx-ioctl-submit-op.sh` now checks the actual live
      report-flag transitions after `issue` and both `transition` steps
    - `scripts/run-vaio-sgx-multi-submit.sh` now extracts snapshot values with
      one `awk` pass instead of `tr | awk`, avoiding false `141` exits under
      `pipefail`
  - guarded deploy:
    - `logs/vaio-deploy/20260511-212444-guarded-deploy`
    - previous module:
      - `srcversion 093DDDD6F956A0E7E690BBE`
      - `override_module_size=416544`
    - current module:
      - `srcversion E774FBBBD966270FEA6C67B`
      - `override_module_size=416736`
  - focused validation:
    - `logs/vaio-ioctl/20260511-212632-sgx-submit-report-op-live/summary.log`
    - proved report-stage progression:
      - `none`
      - `prepared`
      - `armed`
      - `emitted`
      - `launched`
      - `dispatched`
      - `retired`
    - proved report-flag progression:
      - `0x0`
      - `0x1`
      - `0x3`
      - `0x7`
      - `0x1c`
      - `0x10`
      - `0x30`
    - final focused counts:
      - `submit_prepare_count=1`
      - `submit_arm_count=1`
      - `submit_emit_count=1`
      - `submit_launch_count=1`
      - `submit_issue_count=1`
      - `submit_transition_count=2`
  - typed multi-submit validation:
    - `logs/vaio-ioctl/20260511-212931-sgx-multi-submit-report-live/summary.log`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_report_stage_name=retired`
      - `submit_report_stage_count=3`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_report_dependency_active_count=1` while the dependency lane
        was active
      - `submit_report_stage_name=retired`
      - `submit_report_stage_count=2`
    - refill cycles now re-expose the current live stage instead of sticking
      on the older retired snapshot
  - documented in:
    - `docs/116-sgx-lab-submit-report-op-ioctl.md`

## 2026-05-11 - deadlock-fix candidate validated on hardware

- redeployed the kernel deadlock-fix candidate with the guarded session path:
  - [logs/vaio-deploy/20260511-203613-deadlock-fix-redeploy](/root/Poulsbo/logs/vaio-deploy/20260511-203613-deadlock-fix-redeploy)
  - post-reboot loaded override:
    - `driver_srcversion=BB080942FE121033007E2EC`
    - `override_module_size=408344`
- validated the focused typed submit path on the candidate override:
  - [logs/vaio-ioctl/20260511-203809-sgx-submit-op-deadlock-fix-candidate/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-203809-sgx-submit-op-deadlock-fix-candidate/summary.log)
  - key result:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_retired_owner_state_name=complete`
    - final zero cleanup:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- the first typed/default multi-submit run on the candidate exposed a stale
  runner-side refill-cycle assertion against the persisted retired snapshot:
  - [logs/vaio-ioctl/20260511-203848-sgx-multi-submit-default-deadlock-fix-candidate/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-203848-sgx-multi-submit-default-deadlock-fix-candidate/summary.log)
- fixed the runner coherently in
  [scripts/run-vaio-sgx-multi-submit.sh](/root/Poulsbo/scripts/run-vaio-sgx-multi-submit.sh):
  - added `require_snapshot_value`
  - added `require_snapshot_int_ge`
  - added `assert_retired_persisted_snapshot_file`
  - switched typed refill-cycle retired checks from exact
    `completed_packets=1` assumptions to persisted-snapshot checks
- reran the safe typed/default multi-submit wrapper successfully on the same
  candidate override:
  - [logs/vaio-ioctl/20260511-204244-sgx-multi-submit-default-deadlock-fix-candidate-rerun/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-204244-sgx-multi-submit-default-deadlock-fix-candidate-rerun/summary.log)
  - key result:
    - submit A final execution-shaped counts:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
    - submit B final execution-shaped counts:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
    - final cleanup:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- documented the full close-out in:
  - [docs/115-sgx-deadlock-fix-candidate-validation.md](/root/Poulsbo/docs/115-sgx-deadlock-fix-candidate-validation.md)
- final target state is clean:
  - `overall=remote_usable`
  - `driver_srcversion=BB080942FE121033007E2EC`
  - `override_module_size=408344`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`

- 2026-05-11: traced the typed submit-cleanup hang to a real kernel
  self-deadlock in `poulsbo_sgx_submits_show()`
  - remote evidence on the VAIO:
    - one D-state `cat /sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`
    - two D-state `psb_sgx_lab_submit_op /dev/dri/card0 cleanup` tasks
    - cleanup stacks blocked in `poulsbo_sgx_drop_all_submits_count`
  - fixed locally in
    [psb_drv.c](/root/Poulsbo/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c)
    by removing the duplicate `sgx_lab_submit_mutex` lock in
    `poulsbo_sgx_submits_show()`
  - also added target-side `flock(1)` serialization to the main mutation/reset
    runners:
    - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
    - `scripts/run-vaio-sgx-multi-submit.sh`
    - `scripts/run-vaio-sgx-ioctl-object-op.sh`
    - `scripts/run-vaio-sgx-ioctl-cleanup.sh`
    - `scripts/run-vaio-sgx-lab-reset.sh`
  - local verification passed:
    - shell syntax checks
    - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
    - local VAIO-target rebuild
  - guarded deploy outcome:
    - previous loaded override was `srcversion 801B0633670F92DA0659014`
    - candidate local module was `srcversion BB080942FE121033007E2EC`
    - reboot did not return to remote usability
    - final observed state became `overall=ssh_port_down`
    - rollback did not recover it remotely
  - documented in:
    - [docs/113-sgx-submit-cleanup-deadlock-fix.md](/root/Poulsbo/docs/113-sgx-submit-cleanup-deadlock-fix.md)
  - the target later recovered on the restored previous override:
    - `driver_srcversion=801B0633670F92DA0659014`
    - `override_module_size=408352`
    - `rollback_pending_present=0`
  - focused typed submit validation was then rerun sequentially and saved at:
    - [logs/vaio-ioctl/20260511-201657-sgx-submit-op-retired-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-201657-sgx-submit-op-retired-live/summary.log)
    - key result:
      - `SUBMIT_ID=2`
      - `submit_prepare_count=1`
      - `submit_arm_count=1`
      - `submit_emit_count=1`
      - `submit_launch_count=1`
      - `submit_dispatch_count=1`
      - `submit_retired_count=1`
      - `submit_retired_owner_state_name=complete`
      - `submit_retired_completed_packets=2`
      - final zero cleanup:
        - `coord_live_submit_count=0`
        - `gtt_state_range_sgx_object_count=0`
  - one direct rerun mistake re-triggered the old sysfs deadlock on the
    restored previous override:
    - the raw legacy `run-vaio-sgx-multi-submit.sh` path still read
      `poulsbo_sgx_submits`
    - that produced another D-state
      `cat /sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`
    - a plain reboot was issued to clear it
    - the next multi-submit rerun should use
      `run-vaio-sgx-multi-submit-default.sh` or the typed no-sysfs wrapper,
      not the direct legacy entrypoint
  - after that reboot, the typed/default multi-submit wrapper completed cleanly
    on the restored previous override and was saved at:
    - [logs/vaio-ioctl/20260511-203149-sgx-multi-submit-default-retired-live/summary.log](/root/Poulsbo/logs/vaio-ioctl/20260511-203149-sgx-multi-submit-default-retired-live/summary.log)
    - the wrapper exited successfully with:
      - `STALE_SUBMIT_CLEANUP=ioctl`
      - `STALE_OBJECT_CLEANUP=ioctl`
    - final target state stayed clean:
      - `overall=remote_usable`
      - `driver_srcversion=801B0633670F92DA0659014`
      - `rollback_pending_present=0`
- 2026-05-11: added an explicit guardrail to the raw multi-submit
  implementation entrypoint
  - `scripts/run-vaio-sgx-multi-submit.sh` now refuses direct legacy/sysfs
    execution unless `ALLOW_DIRECT_LEGACY_MULTI_SUBMIT=1`
  - `scripts/run-vaio-sgx-multi-submit-default.sh` now sets that opt-in only
    for the explicit `SGX_MULTI_SUBMIT_DRIVER_MODE=legacy` branch
  - local verification passed:
    - `bash -n scripts/run-vaio-sgx-multi-submit.sh`
    - `bash -n scripts/run-vaio-sgx-multi-submit-default.sh`
    - direct invocation now exits immediately with the expected guard message
  - documented in:
    - [docs/114-sgx-multi-submit-legacy-guardrail.md](/root/Poulsbo/docs/114-sgx-multi-submit-legacy-guardrail.md)

### SGX submit emit stage on real hardware

- Extended the typed SGX lab submit-op path with the new execution-shaped
  `emit` stage.
- Added kernel-side emitted-state snapshot fields and made `issue` require and
  consume a valid emitted record instead of progressing directly from armed
  state.
- Updated the typed query and dump helpers to expose emitted-state counters,
  timestamps, object IDs, GPU VAs, queue state, sync state, owner state, and
  dependency-active count.
- Updated the focused typed submit runner and typed multi-submit runner so
  every issue step now uses:
  - `prepare`
  - `arm`
  - `validate`
  - `emit`
  - `validate`
  - `issue`
- Captured fresh canonical validation bundles in:
  - `logs/vaio-ioctl/20260511-181000-sgx-submit-emit-op-live`
  - `logs/vaio-ioctl/20260511-181100-sgx-multi-submit-emit-live`

Validated result on `10.1.11.218`:

- the VAIO remained healthy on override module
  `srcversion 333CBAAFFFDE848128DC9EC`
- focused typed submit path proved:
  - pre-emit state stayed:
    - `submit_emit_valid=0`
    - `submit_emit_count=0`
  - post-emit state moved to:
    - `submit_emit_valid=1`
    - `submit_emit_count=1`
    - `submit_emit_cmd_type_name=ta`
    - `submit_emit_owner_state_name=queued`
    - `submit_emit_control_object_id=1`
    - `submit_emit_status_object_id=5`
    - `submit_last_emit_jiffies=4294937765`
  - post-issue state returned to:
    - `submit_emit_valid=0`
  - final state still completed cleanly:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- typed multi-submit path proved:
  - typed setup remained enabled:
    - `SUBMIT_SETUP=ioctl`
    - `OBJECT_SETUP=ioctl`
    - `RANGE_DISCOVERY=ioctl`
    - `DEPENDENCY_SETUP=ioctl`
    - `VERIFICATION_MODE=ioctl`
  - submit A finished with:
    - `submit_prepare_count=1`
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
  - submit B finished with:
    - `submit_prepare_count=1`
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
  - while live, submit B's emitted state also showed:
    - `submit_emit_dependency_active_count=1`
  - final typed cleanup returned:
    - `coord_live_submit_count=0`
    - `coord_live_object_count=0`
    - `gtt_state_range_sgx_object_count=0`
    - `gtt_state_range_pf_bound_count=0`
- kernel log evidence showed the emitted stage directly:
  - `lab submit emit submit_id=1 arm_count=1 packet_count=1 global_issue_ticket=0 dependency_active_count=0`
  - submit A refill cycles:
    - `lab submit emit submit_id=2 arm_count=1 ... dependency_active_count=0`
    - `lab submit emit submit_id=2 arm_count=2 ... dependency_active_count=0`
    - `lab submit emit submit_id=2 arm_count=3 ... dependency_active_count=0`
  - dependency-gated submit B:
    - `lab submit emit submit_id=3 arm_count=1 ... dependency_active_count=1`
    - `lab submit emit submit_id=3 arm_count=2 ... dependency_active_count=1`

Conclusion:

- the lab submit path now has a three-stage execution-shaped contract before
  issue:
  - materialize through `prepare`
  - commit one issue-side snapshot through `arm`
  - freeze one emitted execution record through `emit`
- `issue` now consumes only the emitted record and invalidates it afterward
- the next kernel batch should build the first real execution-oriented SGX
  structure on top of `prepare + arm + emit`, not another read-side helper

### Submit Arm State Validation

- Added a bounded typed `arm` submit-op ioctl on top of the existing prepared
  submit image.
- Extended the submit query and dump paths with arm-state counters and the
  armed queue, sync, owner, and issue-side snapshot fields.
- Changed `issue` so it now requires and consumes a fresh armed snapshot.
- Changed all submit-shaping mutations plus `prepare` to disarm stale armed
  state before further progression.
- Changed both the focused typed submit-op runner and the typed multi-submit
  runner to use:
  - `prepare`
  - `arm`
  - `validate`
  - `issue`
- Completed one guarded deploy and two canonical live reruns saved as:
  - `logs/vaio-ioctl/20260511-175155-sgx-submit-arm-op-live`
  - `logs/vaio-ioctl/20260511-175233-sgx-multi-submit-arm-live`

Validated result on `10.1.11.218`:

- guarded deploy brought the VAIO to:
  - `driver_srcversion=270B70E360AEFF73D0C52F3`
  - `override_module_size=396320`
  - `rollback_pending_present=0`
  - `rollback_backup_present=1`
- focused typed submit-op path proved:
  - after prepare:
    - `submit_prepare_count=1`
    - `submit_arm_valid=0`
  - after arm:
    - `submit_arm_valid=1`
    - `submit_arm_count=1`
    - `submit_arm_prepare_count=1`
    - `submit_arm_queue_packet_count=1`
    - `submit_arm_owner_state_name=queued`
  - after issue:
    - `submit_arm_valid=0`
  - final state:
    - `submit_validate_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_owner_state_name=complete`
- typed multi-submit path proved:
  - every live typed submit reached:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
  - dependency-gated and later submits still completed cleanly:
    - submit A:
      - `submit_issue_count=1`
      - `submit_transition_count=2`
    - submit B:
      - `submit_validate_count=23`
      - `submit_issue_count=1`
      - `submit_transition_count=2`
    - submit C:
      - `submit_issue_count=1`
      - `submit_transition_count=2`
    - submit D:
      - `submit_issue_count=1`
      - `submit_transition_count=2`
    - submit E:
      - `submit_issue_count=1`
      - `submit_transition_count=2`
  - all post-issue or post-transition queries showed:
    - `submit_arm_valid=0`
  - final cleanup returned:
    - `coord_live_submit_count=0`
    - `coord_live_object_count=0`
    - `gtt_state_range_sgx_object_count=0`
    - `gtt_state_range_pf_bound_count=0`

Conclusion:

- the lab submit path now behaves like an execution-oriented state machine:
  - materialize through `prepare`
  - commit one issue-side snapshot through `arm`
  - consume that exact armed image through `issue`
- the next kernel batch should build on that state machine instead of adding
  more read-side helpers

- Documented in:
  - `docs/96-sgx-focused-compare-reset-and-range-query-fix.md`

- Focused compare follow-up after the warm-target reset helper:
  - the focused object compare wrapper now also completes cleanly through the
    reset-assisted typed-versus-legacy path:
    - `logs/vaio-compare/20260511-142409-sgx-object-mode-compare`
    - `session_outcome=completed`
    - `pre_reset_rc=0`
    - `typed_rc=0`
    - `inter_reset_rc=0`
    - `legacy_rc=0`
    - typed/default evidence:
      - `typed_range_id=7`
      - `typed_object_id=641`
      - `typed_object_role=control`
      - `typed_object_class=packet`
      - `typed_object_gpu_use=control_ccb`
      - `typed_object_context=pf`
    - explicit legacy evidence:
      - `legacy_range_id=736`
      - `legacy_object_id=642`
  - the focused submit compare wrapper had a real warm-target legacy regression
    after the reset-hook work:
    - failing bundles:
      - `logs/vaio-compare/20260511-142528-sgx-submit-mode-compare`
      - `logs/vaio-compare/20260511-142805-sgx-submit-mode-compare`
      - `logs/vaio-compare/20260511-143134-sgx-submit-mode-compare`
      - `logs/vaio-compare/20260511-143948-sgx-submit-mode-compare`
      - `logs/vaio-compare/20260511-144519-sgx-submit-mode-compare`
    - fixes:
      - `scripts/run-vaio-sgx-submit.sh` now deploys and uses the typed
        range-query helper for warm-target before/during GEM-range baselines in
        its explicit legacy branch
      - `scripts/run-vaio-sgx-submit.sh` now parses `range_type_name=...`
        instead of the nonexistent `range_name=...`
      - `scripts/run-vaio-sgx-submit.sh` and
        `scripts/run-vaio-sgx-lab-reset.sh` now parse
        `range_ioctl_ret=0 errno=0` by extracting only the first token after
        the prefix
      - `scripts/run-vaio-sgx-lab-reset.sh` now verifies mmapping-GEM baseline
        convergence through:
        - `range_query_live_gem_count`
        - `baseline_mmapping_gem_max`
    - local reset-helper check after the parser fix:
      - `RC=0`
      - `submit_result_count=0`
      - `object_result_count=2`
      - `range_query_live_gem_count=2`
      - `baseline_mmapping_gem_max=2`
    - completed submit compare bundle:
      - `logs/vaio-compare/20260511-144832-sgx-submit-mode-compare`
      - `session_outcome=completed`
      - `pre_reset_rc=0`
      - `typed_rc=0`
      - `inter_reset_rc=0`
      - `legacy_rc=0`
      - typed/default branch:
        - `typed_submit_id=131`
        - `typed_range_ids=7 9 792 793 794`
        - `typed_object_ids=672 673 674 675 676`
        - `typed_validate_count=1`
        - `typed_issue_count=1`
        - `typed_transition_count=2`
        - `typed_owner_state=complete`
      - explicit legacy branch:
        - `legacy_queue_mode=wrap`
        - `legacy_submit_id=132`
        - `legacy_range_ids=797 798 799 800 801`
        - `legacy_object_ids=677 678 679 680 681`
  - final target state stayed clean:
    - `overall=remote_usable`
    - `driver_srcversion=D43951B174F84BE870230AB`
    - `override_module_size=385384`
    - `rollback_pending_present=0`
    - `rollback_backup_present=1`

### Submit `plan` block landed, locally verified, first hardware run blocked

- Added a new memory-backed typed submit `plan` block above the existing
  `report + manifest + descriptor + expanded 8-dword CCB` model.
- Kernel-side changes landed in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Userspace and runner-side changes landed in:
  - `workloads/psb_sgx_lab_submit_op.c`
  - `workloads/psb_sgx_lab_query.c`
  - `workloads/psb_sgx_lab_dump.c`
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
  - `scripts/run-vaio-sgx-multi-submit.sh`
- The new block snapshots:
  - stage and stage count
  - selected source:
    - `live`
    - `launch`
    - `dispatch`
    - `retired`
  - context, command type, and owner state
  - issued/completed packets, dependency-active count, refill marker
  - queue packet count, queue epoch, queue layout, and CCB offsets
  - sync and owner offsets plus sync or owner values
  - global issue/running/complete tickets
  - linked `report`, `manifest`, and `descriptor` offsets
  - CCB words and `control/pds/use/target/status` GPU-VA32 words
- The typed query and dump paths now export full `submit_plan_*` state.
- The focused and typed multi-submit runners now configure and assert the
  `plan` block through progression.
- Local verification passed:
  - `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
  - `bash -n scripts/run-vaio-sgx-multi-submit.sh`
  - `./scripts/docker-build-workloads-i386.sh`
  - `./scripts/docker-build-vaio-gma500.sh`
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
- The guarded deploy completed successfully and the VAIO came back with:
  - `driver_srcversion=3D21987EBD495B4E78FBBA7`
  - `override_module_size=433996`
  - `rollback_pending_present=0`
- The first focused validation attempt then drove the machine to persistent
  `overall=ssh_port_down` and it did not recover across six bounded health
  polls on May 11, 2026.
- So this batch is currently:
  - implemented
  - locally verified
  - deployed once
  - not yet hardware-validated end to end
- Documented in:
  - `docs/120-sgx-lab-submit-plan-op-ioctl.md`

### Submit `plan` crash-isolation harness added after the first target outage

- Added `STOP_AFTER_STAGE=<stage>` support to:
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
- The focused typed submit-op runner now accepts these stop points:
  - `sync`
  - `owner`
  - `report`
  - `manifest`
  - `descriptor`
  - `plan`
  - `prepare`
  - `arm`
  - `emit`
  - `validate`
  - `launch`
  - `issue`
  - `running`
  - `complete`
- The runner now prints:
  - `STAGE_OK=<stage>`
  - `STOP_AFTER_STAGE_REACHED=<stage>`
- Added a dedicated recovery wrapper:
  - `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
- That wrapper:
  - reruns the focused typed submit path once per stage
  - records one per-phase log and one per-phase health probe
  - stops on the first failing stage or first unhealthy target
  - saves one timestamped session bundle under `logs/vaio-ioctl/`
- This was a local-only follow-up because the VAIO was still down after the
  first full `plan` attempt, so there is no new hardware validation result for
  this harness yet.
- The intended first post-recovery command is:
  - `./scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
- Documented in:
  - `docs/121-sgx-submit-plan-crash-isolation-harness.md`

### Added a dedicated recovery-session wrapper for the blocked submit `plan` path

- Generalized `scripts/run-vaio-recovery-session.sh` so it can record any
  post-recovery validator, not only the earlier `depclear_slot` path.
- New generic recovery-session overrides:
  - `SESSION_KIND`
  - `VALIDATION_LABEL`
  - `VALIDATION_LOG_BASENAME`
  - `VALIDATION_SCRIPT`
- Added a dedicated typed wrapper:
  - `scripts/run-vaio-sgx-plan-recovery-session.sh`
- That wrapper now packages the intended first post-recovery flow:
  - wait for `overall=remote_usable`
  - capture remote module or rollback state
  - run the configured submit-op isolation validator
  - save one structured bundle under `logs/vaio-recovery/`
- Fixed one real issue during local validation:
  - `scripts/run-vaio-sgx-submit-op-phase-matrix.sh` was missing its
    executable bit
- A bounded blocked-target validation now passes:
  - `MAX_POLLS=1 POLL_INTERVAL_SECS=1 ./scripts/run-vaio-sgx-plan-recovery-session.sh`
- The canonical blocked bundle is:
  - `logs/vaio-recovery/20260511-235723-sgx-plan-recovery`
- That session now summarizes cleanly as:
  - `session_outcome=blocked_before_remote_usable`
  - `last_state=ssh_port_down`
- Documented in:
  - `docs/122-sgx-plan-recovery-session-wrapper.md`

### Added a prefix-bisect harness for the blocked submit `plan` path

- Added:
  - `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
- This new wrapper uses the same ordered submit-op stages as the phase matrix,
  but probes cumulative prefixes through a binary search so the first bad stage
  can be found with fewer reruns during the next recovery window.
- The result bundle now records:
  - `probe_count`
  - `probe_order`
  - `last_good_phase`
  - `first_bad_phase`
  - `first_bad_reason`
- The dedicated recovery-session wrapper now defaults to this bisect path:
  - `scripts/run-vaio-sgx-plan-recovery-session.sh`
- The older exhaustive path remains available by overriding
  `VALIDATION_SCRIPT=./scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
- A bounded blocked-target validation now passes directly for the bisect path:
  - `./scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
- The canonical blocked bundle is:
  - `logs/vaio-ioctl/20260511-235723-sgx-submit-op-phase-bisect`
- That session now summarizes cleanly as:
  - `session_outcome=blocked_preflight`
  - `last_state=ssh_port_down`
- Documented in:
  - `docs/123-sgx-submit-plan-bisect-harness.md`

### Added a composed investigation harness for the blocked submit `plan` path

- Added:
  - `scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
- This wrapper now runs the direct prefix bisect first and only launches a
  narrowed confirmation pass when the bisect identifies a bad cumulative
  prefix.
- The parent result bundle records:
  - `bisect_outcome`
  - `bisect_last_good_phase`
  - `bisect_first_bad_phase`
  - `bisect_first_bad_reason`
  - `confirm_phases`
  - `confirm_outcome`
- The dedicated recovery-session wrapper now defaults to this investigation
  path:
  - `scripts/run-vaio-sgx-plan-recovery-session.sh`
- Direct bisect and exhaustive matrix remain available as explicit fallbacks.
- A bounded blocked-target validation now passes directly for the investigation
  path:
  - `./scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
- The canonical blocked investigation bundle is:
  - `logs/vaio-ioctl/20260512-000138-sgx-submit-op-phase-investigation`
- The updated recovery-session wrapper also now has a matching blocked bundle:
  - `logs/vaio-recovery/20260512-000138-sgx-plan-recovery`
- Documented in:
  - `docs/124-sgx-submit-plan-investigation-harness.md`

### Added a guarded-deploy parent session for the blocked submit `plan` path

- Added:
  - `scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`
- This parent wrapper now runs:
  - `scripts/run-vaio-guarded-deploy-session.sh`
  - then, only if deploy succeeds,
    `scripts/run-vaio-sgx-plan-recovery-session.sh`
- The parent result bundle records:
  - `deploy_outcome`
  - `deploy_session_dir`
  - `recovery_outcome`
  - `recovery_session_dir`
  - `last_overall`
  - `final_driver_srcversion`
- Updated `scripts/summarize-vaio-sessions.sh` so `logs/vaio-plan/` sessions
  show as `kind=plan_deploy`.
- A bounded blocked-target validation now passes:
  - `./scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`
- The canonical blocked parent bundle is:
  - `logs/vaio-plan/20260512-000636-sgx-plan-guarded-deploy`
- That parent session now summarizes cleanly as:
  - `session_outcome=deploy_failed_or_blocked`
  - `deploy_outcome=blocked_preflight`
  - `recovery_outcome=skipped`
  - `last_state=ssh_port_down`
- Documented in:
  - `docs/125-sgx-plan-guarded-deploy-session-wrapper.md`
  - `docs/126-sgx-plan-investigation-parent-session-validation.md`

### Added one top-level session selector for the blocked submit `plan` path

- Added:
  - `scripts/run-vaio-sgx-plan-session.sh`
- This wrapper probes current target health first and then selects:
  - `recovery` when the target is not `overall=remote_usable`
  - `deploy` when the target is already `overall=remote_usable`
- It records:
  - `plan_session_mode`
  - `preflight_overall`
  - `selected_mode`
  - `selected_script`
  - `selected_session_dir`
  - `selected_outcome`
  - `last_overall`
  - `final_driver_srcversion`
- Updated `scripts/summarize-vaio-sessions.sh` so `logs/vaio-plan-session/`
  sessions show as `kind=plan_session`.
- Documented in:
  - `docs/127-sgx-plan-session-entrypoint.md`

### Made recent `plan` session wrappers collision-safe

- Added a shared helper:
  - `scripts/allocate-session-dir.sh`
- Updated these wrappers to allocate session directories atomically and accept
  `SESSION_TIMESTAMP` for deterministic testing:
  - `scripts/run-vaio-recovery-session.sh`
  - `scripts/run-vaio-guarded-deploy-session.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
  - `scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`
  - `scripts/run-vaio-sgx-plan-session.sh`
- This fixes the real collision bug where two top-level `plan` session runs
  started in the same second could overwrite each other’s parent bundle.
- A deterministic collision test now passes with:
  - `SESSION_TIMESTAMP=20260512-001500`
- The canonical top-level bundles are:
  - `logs/vaio-plan-session/20260512-001500-sgx-plan-session`
  - `logs/vaio-plan-session/20260512-001500-sgx-plan-session-1`
- Both now complete cleanly instead of colliding and overwriting one another.

### Submit control-layout hardening while the `plan` target is still blocked

- Hardened the kernel-side control object layout rules in
  `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`.
- Added one shared control-layout validator for:
  - queue
  - CCB
  - manifest
  - descriptor
  - plan
- That shared path now rejects:
  - overlap with the reserved control header
  - illegal queue/CCB placement
  - queue overlap with manifest/descriptor/plan
  - CCB overlap with manifest/descriptor/plan
  - manifest overlap with descriptor/plan
  - descriptor overlap with plan
- Wired that validator into:
  - the central submit validator
  - `refresh_submit_views_locked()`
  - the direct control-side mutating setup paths:
    - `kick`
    - `ccb`
    - `queue`
    - `manifest`
    - `descriptor`
    - `plan`
- Fixed one real layout hole in the process:
  - `configure_submit_manifest()` previously did not reject overlap with an
    already-configured descriptor block
- Fixed one real code-quality regression before closing the batch:
  - the first version used whole-submit stack copies in several setters, which
    inflated kernel stack frames past 1 KB
  - that was replaced with small temporary field overrides plus restore
- Local verification passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
  - `./scripts/docker-build-vaio-gma500.sh`
- There is still no new hardware result for this batch because the VAIO
  remained down throughout:
  - `ping_status=fail`
  - `tcp_22_status=closed`
  - `ssh_pubkey_status=no_route`
  - `overall=ssh_port_down`
- Documented in:
  - `docs/128-sgx-submit-control-layout-hardening.md`

### Submit status-layout hardening while the `plan` target is still blocked

- Hardened the kernel-side status object layout rules in
  `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`.
- Added one shared status-layout validator for:
  - TA status
  - 3D status
  - sync
  - owner
  - report
  - dependency-state slots
- That shared path now rejects:
  - TA / 3D status overlap
  - sync overlap with TA or 3D status
  - owner overlap with sync or status
  - report overlap with sync, owner, or status
  - dependency-state overlap with sync, owner, report, or status
  - dependency-state overlap with other dependency-state slots
- Wired that validator into:
  - the central submit validator
  - `refresh_submit_report_locked()`
  - dependency refresh
  - the direct status-side mutating setup paths:
    - `status`
    - `sync`
    - `owner`
    - `report`
    - dependency-state slot configuration
- Fixed one real compile-order bug before closing the batch:
  - the first version called the dependency-slot pointer helpers before their
    definitions
  - added forward declarations so the kernel tree builds cleanly again
- Local verification passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
  - `./scripts/docker-build-vaio-gma500.sh`
- There is still no new hardware result for this batch because the VAIO
  remained down throughout:
  - `ping_status=fail`
  - `tcp_22_status=closed`
  - `ssh_pubkey_status=no_route`
  - `overall=ssh_port_down`
- Documented in:
  - `docs/129-sgx-submit-status-layout-hardening.md`

### Submit validator stack split while the `plan` target is still blocked

- Split the central submit validator in
  `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c` so the
  control-side and status-side backing-image checks no longer live on the same
  stack frame as the full semantic validator.
- Added dedicated helper validators for:
  - control-side backing images
  - status-side backing images
- Changed the central path to rely on the already-shared layout validators
  instead of carrying duplicated inline overlap checks for the same control and
  status regions.
- That preserves the same validation coverage for:
  - queue and CCB readback
  - manifest image
  - descriptor image
  - plan image
  - sync block
  - owner block
  - report block
  - dependency blocks
  - TA/3D status values
- Local verification passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
  - `./scripts/docker-build-vaio-gma500.sh`
- The earlier large-frame warning in `poulsbo_sgx_validate_submit_locked()`
  is now gone from the local module build.
- There is still no new hardware result for this batch because the VAIO
  remained down throughout:
  - `ping_status=fail`
  - `tcp_22_status=closed`
  - `ssh_pubkey_status=no_route`
  - `overall=ssh_port_down`
- Documented in:
  - `docs/130-sgx-submit-validator-stack-split.md`

### Submit refresh preflight while the `plan` target is still blocked

- Hardened the submit-view refresh pipeline in
  `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`.
- Added one shared preflight pass for `refresh_submit_views_locked()` that:
  - finds and reuses the live control object once
  - finds and reuses the live status object once
  - validates the full control layout once
  - validates the full status layout once
  - prebuilds the memory-backed:
    - manifest
    - descriptor
    - plan
- Changed the per-view refresh helpers so they now consume the preflighted
  objects and prebuilt images instead of each repeating lookup/build/access
  work locally.
- Result:
  - later refresh failures now stop before the first write instead of after a
    partial `report -> manifest -> descriptor -> plan` update sequence
- Local verification passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500`
  - `./scripts/docker-build-vaio-gma500.sh`
- There is still no new hardware result for this batch because the VAIO
  remained down throughout:
  - `ping_status=fail`
  - `tcp_22_status=closed`
  - `ssh_pubkey_status=no_route`
  - `overall=ssh_port_down`
- Documented in:
  - `docs/131-sgx-submit-refresh-preflight.md`

### Live `plan` rerun now isolates `sync` cleanly on a healthy target

- The VAIO came back to `overall=remote_usable` on override
  `srcversion 2BE09D0DC1CCBDB51D2328E`, size `429820`.
- Ran the top-level operator entrypoint in recovery mode on the already-loaded
  candidate:
  - `PLAN_SESSION_MODE=recovery ./scripts/run-vaio-sgx-plan-session.sh`
- The first live recovery-mode bundle already isolated the first bad
  cumulative phase to `sync`, but the focused submit-op runner still leaked
  local heredoc expansion noise:
  - `logs/vaio-plan-session/20260512-104039-sgx-plan-session`
- Fixed the remaining runner-side issues in one batch:
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
    - escaped the remote helper's `stage_name` references so the focused
      submit-op wrapper no longer emits heredoc `unbound variable` noise
  - `scripts/run-vaio-recovery-session.sh`
    - now records `final_driver_srcversion`
  - `scripts/run-vaio-guarded-deploy-session.sh`
    - now records `last_overall` and `final_driver_srcversion`
  - `scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`
    - now prefers child `last_overall` instead of only reusing
      `preflight_overall`
  - `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
    - now records `preflight_overall` and `last_overall`
  - `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
    - now records `preflight_overall` and `last_overall`
- Reran the same top-level recovery-mode session cleanly:
  - `logs/vaio-plan-session/20260512-104449-sgx-plan-session`
- The clean child results are now:
  - bisect:
    - `session_outcome=bisect_identified_bad_prefix`
    - `probe_order=prepare,report,sync`
    - `first_bad_phase=sync`
    - `preflight_overall=remote_usable`
    - `last_overall=remote_usable`
  - confirm:
    - `session_outcome=phase_failed`
    - `last_phase=sync`
    - `last_phase_rc=1`
    - `last_phase_health=remote_usable`
    - `preflight_overall=remote_usable`
    - `last_overall=remote_usable`
  - top-level parent:
    - `session_outcome=validation_failed`
    - `preflight_overall=remote_usable`
    - `last_overall=remote_usable`
    - `final_driver_srcversion=2BE09D0DC1CCBDB51D2328E`
- Result at that point:
  - the current blocker was no longer a generic boot or recovery failure
  - the parent phase label was stably `sync`
  - the target remained remotely usable throughout the failure
- Later correction:
  - this runner batch predated stderr capture in the bisect and confirm phase
    logs, so the repo now treats it as an incomplete diagnosis
  - the corrected live result is recorded in
    `docs/136-sgx-plan-kick-failure-clean-rerun.md`
- Documented in:
  - `docs/135-sgx-plan-sync-failure-clean-rerun.md`

### Post-`descriptor` crash and timeout-bounded operator hardening

- The earlier live `kick` diagnosis on override
  `2BE09D0DC1CCBDB51D2328E` was real, but it is no longer the active first
  blocker on the `plan` line.
- Found the kernel-side cause of that older failure:
  - `poulsbo_sgx_submit_has_status_views()` treated raw TA/3D status counts as
    a real status-backed view
  - the focused submit runner configures `kick` before the later `status` op
    defines backing status-memory layout
  - refresh preflight therefore validated status layout too early and kicked
    back `-EINVAL`
- Fixed that by changing `poulsbo_sgx_submit_has_status_views()` so refresh
  preflight only treats real status-backed configs as status views:
  - `status_config_count`
  - `sync_config_count`
  - `owner_config_count`
  - `report_config_count`
  - dependency-state config counts
- Rebuilt locally and guarded-deployed the new candidate.
- The guarded deploy succeeded and booted the VAIO on:
  - `srcversion 0EEEE302E3FDA9F59B7CFB9`
  - `override_module_size=429756`
- The live recovery investigation then advanced materially further than before:
  - `STAGE_OK=sync`
  - `STAGE_OK=owner`
  - `STAGE_OK=report`
  - `STAGE_OK=manifest`
  - `STAGE_OK=descriptor`
- After `descriptor`, the target dropped off the network before the old
  wrapper stack emitted a finished failure record.
- That exposed a second real runner-side bug:
  - bisect / matrix / investigation sessions could hang indefinitely when a
    target died mid-phase
- Fixed the operator stack in one coherent batch:
  - `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
  - `scripts/run-vaio-recovery-session.sh`
  - `scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`
  - `scripts/run-vaio-sgx-plan-session.sh`
- New behavior:
  - per-phase submit runs are bounded by `PHASE_TIMEOUT_SECS`
  - per-phase health probes are bounded by `HEALTH_TIMEOUT_SECS`
  - state capture is bounded by `STATE_TIMEOUT_SECS`
  - recovery validation children are bounded by
    `VALIDATION_TIMEOUT_SECS`
  - timeout/unhealthy reasons now propagate through:
    - bisect
    - matrix
    - investigation
    - recovery
    - top-level plan session
- Local stub validation now proves:
  - bisect writes `first_bad_reason=phase_timeout`
  - investigation writes `confirm_last_phase_reason=phase_timeout`
  - recovery writes `validation_failed_reason=phase_timeout`
- The VAIO remained down after the post-`descriptor` live crash:
  - `ping_status=fail`
  - `tcp_22_status=closed`
  - `ssh_pubkey_status=no_route`
  - `overall=ssh_port_down`
- Next real target step is now:
  - recover the VAIO on console
  - rerun `./scripts/run-vaio-sgx-plan-session.sh`
  - get the first bounded answer at or after `plan`
- Documented in:
  - `docs/137-sgx-plan-timeout-hardening-and-post-descriptor-crash.md`

### Session result finalization and summary hardening

- The operator stack still had one real observability gap:
  interrupted parent sessions could leave only `session.env`, which made
  `scripts/summarize-vaio-sessions.sh` report `outcome=no_result`.
- Fixed that across the recent plan/recovery/deploy/rollback parents:
  - `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
  - `scripts/run-vaio-recovery-session.sh`
  - `scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`
  - `scripts/run-vaio-sgx-plan-session.sh`
  - `scripts/run-vaio-guarded-deploy-session.sh`
  - `scripts/run-vaio-rollback-autorestore-session.sh`
- New behavior:
  - interrupted wrappers now write fallback `session-result.env`
  - fallback result files preserve best-known:
    - `session_outcome`
    - `last_overall`
    - `final_driver_srcversion`
    - failed phase / step metadata
    - `fallback_exit_rc`
  - remaining timeout-sensitive local `tee` sites were removed
  - rollback autorestore sessions now use collision-safe
    `allocate-session-dir.sh`
- `scripts/summarize-vaio-sessions.sh` now degrades missing-result bundles to:
  - `session_outcome=interrupted_or_incomplete`
  - inferred `last_state` from saved state/health logs when possible
  - inferred `finished_at` from directory mtime when needed
- Local validation passed:
  - `bash -n` on the touched wrappers and summary helper
  - no remaining targeted timeout-sensitive `| tee` matches
  - synthetic top-level `plan` parent failure wrote fallback result with
    `session_outcome=interrupted`
  - synthetic rollback collision run produced:
    - `.../20260512-140000-autorestore`
    - `.../20260512-140000-autorestore-1`
  - existing interrupted bundles now summarize cleanly instead of as
    `no_result`
- Documented in:
  - `docs/140-sgx-session-result-finalization-and-summary-hardening.md`

### Post-descriptor `plan` subphase isolation

- The current live `plan` question was still too coarse:
  recovery only proved the newer candidate survived through `descriptor`
  inside the first cumulative `prepare` prefix.
- Fixed that by splitting the focused runner’s post-`descriptor` boundary into:
  - `descriptor_op`
  - `descriptor_query`
  - `plan_op`
  - `plan_query`
  - `prepare_op`
  - `prepare_query`
- Kept backward compatibility:
  - `descriptor` still stops after `descriptor_query`
  - `plan` still stops after `plan_query`
  - `prepare` still stops after `prepare_query`
- The same change now drives the default phase order in:
  - `scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
  - `scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
- The focused runner now also records explicit failed-step names in that zone:
  - `submit_op_descriptor_focus`
  - `query_by_id_after_descriptor`
  - `submit_op_plan_focus`
  - `query_by_id_after_plan`
  - `submit_op_prepare_focus`
  - `query_by_id_after_prepare`
- Local verification passed:
  - `bash -n scripts/run-vaio-sgx-ioctl-submit-op.sh`
  - `bash -n scripts/run-vaio-sgx-submit-op-phase-matrix.sh`
  - `bash -n scripts/run-vaio-sgx-submit-op-phase-bisect.sh`
  - `bash -n scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
- No new hardware rerun was attempted in this batch.
- Documented in:
  - `docs/141-sgx-plan-post-descriptor-subphase-isolation.md`

### `plan` phase-profile operator defaults

- The finer post-`descriptor` split still relied on ad hoc `PHASES=...`
  overrides at the operator layer.
- Added shared resolver:
  - `scripts/resolve-sgx-submit-phase-profile.sh`
- New named profiles:
  - `full`
  - `post_descriptor`
- The generic phase tools now support either:
  - `PHASE_PROFILE=<name>`
  - `PHASES=<csv>` overriding it
- The plan-specific wrappers now choose better defaults:
  - `run-vaio-sgx-plan-recovery-session.sh`
    - default `PHASE_PROFILE=post_descriptor`
  - `run-vaio-sgx-plan-guarded-deploy-session.sh`
    - default recovery-side `PHASE_PROFILE=full`
  - `run-vaio-sgx-plan-session.sh`
    - new `PLAN_PHASE_PROFILE=auto|full|post_descriptor`
    - `auto -> post_descriptor` for recovery
    - `auto -> full` for deploy
- The top-level parent result now also records:
  - `selected_phase_profile`
- Local verification passed:
  - `bash -n` on touched wrappers and phase tools
  - phase-profile helper output for `full`
  - phase-profile helper output for `post_descriptor`
  - blocked direct investigation recorded:
    - `phase_profile=post_descriptor`
    - `phase_selection=profile`
    - `phases=descriptor_query,plan_op,plan_query,prepare_op,prepare_query`
  - bounded blocked top-level recovery run recorded:
    - `selected_mode=recovery`
    - `selected_phase_profile=post_descriptor`
- No new hardware rerun was attempted in this batch.
- Documented in:
  - `docs/142-sgx-plan-phase-profile-operator-defaults.md`

### Phase-profile result propagation and summary

- The new phase profiles still lived too low in the session tree:
  child manifests knew them, but parent result files and the summary view did
  not.
- Fixed that in one coherent batch:
  - phase tools now persist:
    - `phase_profile`
    - `phase_selection`
    - `phases`
  - recovery wrapper now persists:
    - `validation_phase_profile`
    - `validation_phase_selection`
    - `validation_phases`
  - guarded deploy parent now persists:
    - `recovery_phase_profile`
    - `recovery_phase_selection`
    - `recovery_phases`
  - top-level plan parent now persists:
    - `selected_phase_profile`
    - `selected_phase_selection`
    - `selected_phases`
- `scripts/summarize-vaio-sessions.sh` now shows a `phase_profile` column.
- Local blocked validation confirmed:
  - direct investigation result carries `phase_profile=post_descriptor`
  - bounded top-level recovery result carries
    `selected_phase_profile=post_descriptor`
  - bounded top-level recovery result now also carries
    `selected_phases=descriptor_query,plan_op,plan_query,prepare_op,prepare_query`
  - child recovery result now carries
    `validation_phases=descriptor_query,plan_op,plan_query,prepare_op,prepare_query`
  - session summary now shows a visible `phase_profile` column
- No new hardware rerun was attempted in this batch.
- Documented in:
  - `docs/143-sgx-phase-profile-result-propagation-and-summary.md`

### Plan phase-selection semantics

- The earlier phase-profile propagation still blurred the operator-facing
  request with the child recovery wrapper's internal `request_only` state.
- Fixed that in one coherent batch:
  - `run-vaio-sgx-plan-recovery-session.sh` now treats
    `PLAN_PHASE_PROFILE` / `PLAN_PHASES` as the preferred interface and
    forwards `PHASE_SELECTION=profile|explicit`
  - `run-vaio-recovery-session.sh` now persists:
    - `validation_phase_profile_request`
    - `validation_phase_selection_request`
    - `validation_phases_request`
  - `run-vaio-sgx-plan-guarded-deploy-session.sh` now persists:
    - `recovery_requested_phase_profile`
    - `recovery_requested_phase_selection`
    - `recovery_requested_phases`
  - `run-vaio-sgx-plan-session.sh` now preserves:
    - `selected_phase_profile`
    - `selected_phase_selection`
    - `selected_phases`
    - `selected_child_phase_profile`
    - `selected_child_phase_selection`
    - `selected_child_phases`
  - `scripts/summarize-vaio-sessions.sh` now renders the phase column as
    `profile:selection`
- Local blocked validation confirmed:
  - top-level alias rerun:
    - `PLAN_SESSION_MODE=recovery`
    - `PLAN_PHASES=plan_op,plan_query`
  - parent result now carries:
    - `selected_phase_profile=post_descriptor`
    - `selected_phase_selection=explicit`
    - `selected_phases=plan_op,plan_query`
    - `selected_child_phase_selection=request_only`
  - child recovery result now carries:
    - `validation_phase_selection_request=explicit`
    - `validation_phase_selection=request_only`
  - session summary now shows:
    - `post_descriptor:explicit`
- No new hardware rerun was attempted in this batch.
- Documented in:
  - `docs/144-sgx-plan-phase-selection-semantics.md`

### `plan` diagnosis summary script

- The generic session summary was still too broad for the current `plan`
  investigation work because the relevant evidence lived in different result
  formats across:
  - top-level `plan` sessions
  - guarded deploy parents
  - recovery wrappers
  - direct submit-op phase investigations
- Added a dedicated offline view:
  - `scripts/summarize-vaio-plan-sessions.sh`
- It flattens each saved `plan`-related bundle into one row with:
  - `kind`
  - `timestamp`
  - `outcome`
  - `last_state`
  - `requested`
  - `child`
  - `failed_phase`
  - `failed_step`
  - `module_srcversion`
- Local verification confirmed the table now shows both:
  - the latest blocked explicit-window session
    - `requested=post_descriptor:explicit[plan_op,plan_query]`
    - `child=post_descriptor:request_only[plan_op,plan_query]`
  - the older live confirmed-bad session
    - `failed_phase=sync`
    - `failed_step=submit_op_kick_focus`
- No new hardware rerun was attempted in this batch.
- Documented in:
  - `docs/145-sgx-plan-diagnosis-summary-script.md`

### `plan` status report and narrowed live `plan_query` rerun

- The new diagnosis summary still required manual picking of the current
  reference bundles before each next rerun.
- Added one current-state wrapper on top of it:
  - `scripts/report-vaio-plan-status.sh`
- That report now surfaces, in one place:
  - current target health
  - latest explicit-window blocked session
  - latest confirmed live failure
  - latest remote-usable interrupted session
  - latest guarded-deploy candidate
- During this batch the VAIO came back to `overall=remote_usable`, and the new
  report identified the current loaded candidate and the right next narrowed
  rerun window.
- I then ran:
  - `PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_op,plan_query ./scripts/run-vaio-sgx-plan-session.sh`
- The live result moved the boundary forward:
  - `plan_op` completed cleanly
  - `plan_query` reached `descriptor_query`
  - then the VAIO dropped off the network again
- So the current live question is no longer `plan_op`; it is now inside
  `plan_query`.
- Current preferred next live command after recovery:
  - `PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_query ./scripts/run-vaio-sgx-plan-session.sh`
- Documented in:
  - `docs/146-sgx-plan-status-report-and-live-plan-query-rerun.md`

### `plan` status report phase-evidence selection fix

- The first reporter draft still chose the newest parent row with
  `outcome=interrupted` and `last_state=remote_usable`, which regressed the
  recommendation to the older `descriptor_query` path even after a newer live
  `plan_op,plan_query` rerun already existed.
- Fixed `scripts/report-vaio-plan-status.sh` in two parts:
  - corrected the `latest_interrupted_remote_usable_row` versus
    `latest_remote_usable_interrupted_row` unbound-variable bug
  - widened the recommendation source from "latest interrupted parent" to the
    newest remote-usable phase-evidence `plan_session`, including
    `validation_failed` parents with child bisect state
- The report now extracts from the newest bisect child:
  - `last_good_phase=plan_op`
  - `first_bad_phase=plan_query`
  - `first_bad_reason=phase_timeout`
- That means the current recommended narrow rerun is now emitted directly as:
  - `PLAN_SESSION_MODE=recovery PLAN_PHASES=plan_query ./scripts/run-vaio-sgx-plan-session.sh`
- Documented in:
  - `docs/146-sgx-plan-status-report-and-live-plan-query-rerun.md`

### `plan` evidence columns and `plan_query` phase profile

- Extended `scripts/summarize-vaio-plan-sessions.sh` so each row now carries the
  newest bisect evidence it can find under that bundle:
  - `evidence_last_started`
  - `evidence_last_good`
  - `evidence_first_bad`
  - `evidence_reason`
- On the current live narrowed rerun that means the summary now shows, directly
  on the `20260512-154733` row:
  - `evidence_last_good=plan_op`
  - `evidence_first_bad=plan_query`
  - `evidence_reason=phase_timeout`
- Added one named single-phase resolver profile in
  `scripts/resolve-sgx-submit-phase-profile.sh`:
  - `plan_query -> plan_query`
- Updated the top-level plan operator validator so
  `PLAN_PHASE_PROFILE=plan_query` is accepted as a first-class alias beside the
  older explicit `PLAN_PHASES=plan_query` form.
- Updated `scripts/report-vaio-plan-status.sh` so the current-state view now
  also emits:
  - `recommended_next_phase_profile=plan_query`
  - `recommended_next_profile_command=PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=plan_query ./scripts/run-vaio-sgx-plan-session.sh`

### Shared `plan` current-boundary resolver and recovery auto-default

- Added `scripts/resolve-vaio-plan-current-phase.sh` so the operator stack has
  one shared place that resolves the current saved live boundary from the
  newest remote-usable phase-evidence bundle.
- On the current saved evidence it resolves:
  - `resolved_profile=plan_query`
  - `resolved_phases=plan_query`
  - `resolved_phase=plan_query`
  - `resolved_reason=phase_timeout`
  - `source_timestamp=20260512-154733`
- Wired that helper into:
  - `scripts/run-vaio-sgx-plan-recovery-session.sh`
  - `scripts/run-vaio-sgx-plan-session.sh`
  - `scripts/report-vaio-plan-status.sh`
- Recovery-mode `auto` now follows the same current boundary as the status
  report instead of defaulting back to the broader `post_descriptor` window.
- A blocked top-level operator rerun now confirms that directly in saved parent
  state:
  - `selected_phase_profile=plan_query`
  - `selected_phase_selection=profile`
  - `selected_phases=plan_query`
  - `selected_child_phase_profile=plan_query`
  - `selected_child_phase_selection=request_only`
  - `selected_child_phases=plan_query`
- Documented in:
  - `docs/147-sgx-plan-current-boundary-resolver-and-auto-recovery-default.md`

### `plan` request/resolved/source provenance

- The current-boundary resolver was working, but saved parent bundles still
  blurred together:
  - what the operator requested
  - what the wrapper actually resolved to
  - which saved live bundle supplied that resolution
- Updated:
  - `scripts/run-vaio-sgx-plan-session.sh`
  - `scripts/run-vaio-sgx-plan-recovery-session.sh`
  - `scripts/run-vaio-recovery-session.sh`
  - `scripts/summarize-vaio-plan-sessions.sh`
  - `scripts/report-vaio-plan-status.sh`
- The summary table now exposes:
  - `requested`
  - `resolved`
  - `child`
  - `phase_source`
- The canonical blocked auto-resolution row is now:
  - `requested=auto:profile[plan_query]`
  - `resolved=plan_query:profile[plan_query]`
  - `child=plan_query:request_only[plan_query]`
  - `phase_source=phase_timeout@20260512-154733`
- The status report now surfaces the same provenance directly:
  - `current_recovery_profile=plan_query`
  - `current_recovery_reason=phase_timeout`
  - `current_recovery_source_timestamp=20260512-154733`
- Documented in:
  - `docs/148-sgx-plan-request-resolved-source-provenance.md`

### SGX lab submit `result` block and fixed-module validation

- Added one new memory-backed typed submit `result` block through:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
  - `workloads/psb_sgx_lab_submit_op.c`
  - `workloads/psb_sgx_lab_query.c`
  - `workloads/psb_sgx_lab_dump.c`
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
  - `scripts/run-vaio-sgx-multi-submit.sh`
- The typed submit-op surface now supports:
  - `result <submit-selector> <offset>`
- The memory-backed `result` block now carries:
  - stage and stage count
  - selected source `live|dispatch|retired`
  - context, command type, and owner state
  - issued/completed packet counts
  - dependency-active count
  - queue packet count and queue epoch
  - sync, owner, and global ticket fields
  - linked `report/manifest/descriptor/plan` offsets
  - control/status object IDs and GPU-VA32 words
  - config count and `last_result_jiffies`
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260512-234450-guarded-deploy`
  - live override:
    - `driver_srcversion=EAD330CBA159A4483448EBF`
    - `override_module_size=434112`
    - `last_overall=remote_usable`
- Focused direct hardware validation then passed and is now captured in:
  - `logs/vaio-ioctl/20260513-000332-sgx-submit-result-live/summary.log`
  - final focused `result` state proved:
    - `submit_result_offset=0x180`
    - `submit_result_stage_name=retired`
    - `submit_result_source_name=retired`
    - `submit_result_owner_state_name=complete`
    - `submit_result_issued_packets=1`
    - `submit_result_completed_packets=2`
    - `submit_result_queue_packet_count=0`
    - `submit_result_queue_epoch=0`
    - `submit_result_control_gpu_va32=0x807bf000`
    - `submit_result_status_gpu_va32=0x807cf000`
    - `submit_result_report_offset=0x120`
    - `submit_result_manifest_offset=0x180`
    - `submit_result_descriptor_offset=0x200`
    - `submit_result_plan_offset=0x300`
    - `submit_result_config_count=1`
  - focused progression still completed cleanly:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
- Typed multi-submit hardware validation also passed and is now captured in:
  - `logs/vaio-ioctl/20260513-000411-sgx-multi-submit-result-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=23`
    - `SUBMIT_B_ID=24`
    - submit A final:
      - `submit_result_stage_name=retired`
      - `submit_result_stage_count=3`
      - `submit_result_source_name=retired`
      - `submit_result_owner_state_name=complete`
      - `submit_result_dependency_active_count=0`
      - `submit_result_queue_epoch=2`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_result_stage_name=retired`
      - `submit_result_stage_count=2`
      - `submit_result_source_name=retired`
      - `submit_result_owner_state_name=complete`
      - `submit_result_dependency_active_count=1`
      - `submit_result_queue_epoch=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
    - final cleanup returned:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- The exact no-redeploy `plan` regression line still passed on that same
  override:
  - `logs/vaio-plan-session/20260512-235844-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=EAD330CBA159A4483448EBF`
- The shared current-boundary resolver now therefore holds:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260512-235844`
- Documented in:
  - `docs/160-sgx-plan-fixed-module-through-complete.md`
  - `docs/161-sgx-lab-submit-result-op-ioctl.md`

### SGX submit view-record collection

- Added one shared collected-record layer for the typed submit live views in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- New shared pieces:
  - `struct psb_sgx_lab_submit_view_records`
  - `poulsbo_sgx_collect_submit_view_records_locked()`
- The typed submit image now collects:
  - `live`
  - `plan`
  - `result`
  once per pass, then reuses those records across refresh and validation
- Rewired the fixed and memory-backed submit builders to consume the collected
  records:
  - `queue header`
  - `ccb record`
  - `sync`
  - `owner`
  - `report`
  - `manifest`
  - `descriptor`
  - `plan`
  - `result`
- `poulsbo_sgx_preflight_submit_views_locked()` now collects the typed
  view-record set once whenever any typed memory-backed submit view is present
- Split the image validation back into smaller per-image helpers so the
  shared wrappers stay used and the validator path remains warning-clean
- Local verification passed:
  - `git -C third_party/linux-ubuntu-5.4 diff --check -- drivers/gpu/drm/gma500/psb_drv.c`
  - `./scripts/docker-build-vaio-gma500.sh`
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-012143-guarded-deploy`
  - final live override:
    - `driver_srcversion=7E90BF7EA4740AF68A6B588`
    - `override_module_size=438196`
    - `last_overall=remote_usable`
- Exact no-redeploy `plan` proof still passed on that same module:
  - `logs/vaio-plan-session/20260513-012335-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=7E90BF7EA4740AF68A6B588`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-012435-sgx-submit-view-records-live/summary.log`
  - final focused state:
    - `SUBMIT_ID=4`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_result_stage_name=retired`
    - `submit_result_source_name=retired`
    - `submit_result_owner_state_name=complete`
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-012435-sgx-multi-submit-view-records-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=2`
    - `SUBMIT_B_ID=3`
    - submit A final:
      - `submit_prepare_count=1`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_result_stage_name=retired`
      - `submit_result_source_name=retired`
      - `submit_result_owner_state_name=complete`
    - submit B final:
      - `submit_prepare_count=1`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_result_stage_name=retired`
      - `submit_result_source_name=retired`
      - `submit_result_owner_state_name=complete`
    - final cleanup returned:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-012335`
- Documented in:
  - `docs/164-sgx-submit-view-record-collection.md`

### SGX submit issue-plan block

- Added one new typed submit configure op:
  - `DRM_PSB_SGX_LAB_SUBMIT_OP_ISSUE_PLAN`
- Added one new memory-backed control-side submit block:
  - `issue-plan`
- Kernel-side changes in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drm_lab.h`
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- Userspace and focused / multi-submit validation changes in:
  - `workloads/psb_sgx_lab_submit_op.c`
  - `workloads/psb_sgx_lab_query.c`
  - `scripts/run-vaio-sgx-ioctl-submit-op.sh`
  - `scripts/run-vaio-sgx-multi-submit.sh`
- The new `issue-plan` block now snapshots launch-time control-side state:
  - stage and stage count
  - valid state
  - owner state
  - issued packet count
  - refill-from-complete state
  - queue and queue-epoch state
  - sync, owner-cookie, and ticket state
  - CCB words
  - control/status object ids
  - control/status/PDS/USE/target GPU-VA32 words
- The top-level exact no-redeploy `plan` proof on the new module also now
  passes cleanly:
  - `logs/vaio-plan-session/20260513-024621-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=228151D0E1E224804A6E1E6`
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-023337-guarded-deploy`
  - final live override:
    - `driver_srcversion=228151D0E1E224804A6E1E6`
    - `override_module_size=446824`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-023956-sgx-submit-issue-plan-live/summary.log`
  - final focused proof:
    - `submit_issue_plan_stage_name=retired`
    - `submit_issue_plan_stage_count=1`
    - `submit_issue_plan_valid=0`
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-024024-sgx-multi-submit-issue-plan-live/summary.log`
  - final typed run proved:
    - submit A final:
      - `submit_issue_plan_stage_name=retired`
      - `submit_issue_plan_stage_count=3`
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
    - submit B final:
      - `submit_issue_plan_stage_name=retired`
      - `submit_issue_plan_stage_count=2`
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
    - final cleanup returned:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-024621`
- Documented in:
  - `docs/166-sgx-lab-submit-issue-plan-op-ioctl.md`

### SGX submit CMDTA kick object

- Added one new kernel-internal TA-specific execution object:
  - `psb_sgx_lab_submit_cmdta_kick`
- Added one shared GPU-VA32 offset helper:
  - `poulsbo_sgx_gpu_va32_add_offset()`
- Kernel-side changes in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- The new `cmdta_kick` object now wraps one `exec_packet` and derives:
  - queue GPU VA
  - CCB GPU VA
  - sync GPU VA
  - owner GPU VA
  - TA status GPU VA
  - 3D status GPU VA
- The typed `issue-plan` builder now uses a launch-source `cmdta_kick`
  directly, and the central validator now prebuilds:
  - one live-source `cmdta_kick`
  - one launch-source `cmdta_kick` when a predicted post-issue plan exists
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-032841-guarded-deploy`
  - final live override:
    - `driver_srcversion=C1CC8033D0035B46726BAB0`
    - `override_module_size=447532`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-033102-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=C1CC8033D0035B46726BAB0`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-033202-sgx-submit-cmdta-kick-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_live_stage=6`
    - `submit_result_stage=6`
    - `submit_manifest_stage=6`
    - `submit_descriptor_stage=6`
    - `submit_plan_stage=6`
    - `submit_issue_plan_stage_name=retired`
    - `submit_issue_plan_stage_count=1`
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-033241-sgx-multi-submit-cmdta-kick-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_issue_plan_stage_name=retired`
      - `submit_issue_plan_stage_count=3`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_issue_plan_stage_name=retired`
      - `submit_issue_plan_stage_count=2`
    - the dependent submit still showed the intended live path in dmesg:
      - `lab submit emit submit_id=4 ... dependency_active_count=1`
      - `lab submit launch submit_id=4 ... dependency_active_count=1`
      - `lab submit issue submit_id=4 ... refill=1`
    - final cleanup returned:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-033102`
- Documented in:
  - `docs/168-sgx-submit-cmdta-kick-object.md`

### SGX submit CCB kick object

- Added one new kernel-internal kick-shaped execution object:
  - `psb_sgx_lab_submit_ccb_kick`
- Added one shared status-update helper structure:
  - `psb_sgx_lab_submit_status_update`
- Kernel-side changes in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- The new `ccb_kick` object now wraps one `cmdta_kick` and derives:
  - the CCB packet offset
  - the TA dependency bit
  - the TA status update set
  - the 3D status update set
  - the exact expected status values and GPU VA32 addresses per update slot
- The status-side validator now uses one live-source `ccb_kick` instead of
  rebuilding TA and 3D status checks from raw submit offsets inline, and the
  central validator also prebuilds the launch-source `ccb_kick` when a
  predicted post-issue plan exists
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-034651-guarded-deploy`
  - final live override:
    - `driver_srcversion=8B053004C83E74C5824A815`
    - `override_module_size=448296`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-034951-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=8B053004C83E74C5824A815`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-035055-sgx-submit-ccb-kick-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_live_stage=6`
    - `submit_result_stage=6`
    - `submit_manifest_stage=6`
    - `submit_descriptor_stage=6`
    - `submit_plan_stage=6`
    - `submit_issue_plan_stage_name=retired`
    - `submit_issue_plan_stage_count=1`
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-035151-sgx-multi-submit-ccb-kick-live/summary.log`
  - the dependent submit still proved the intended normalized dependency path:
    - `submit_result_dependency_active_count=1`
    - `submit_manifest_dependency_active_count=1`
    - `submit_descriptor_dependency_active_count=1`
    - `submit_plan_dependency_active_count=1`
    - `submit_dependency_active_count=1`
    - `dependency[1]_submit_id=3`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
    - `dependency[1]_flags=0xf`
    - final cleanup returned:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-034951`
- Documented in:
  - `docs/169-sgx-submit-ccb-kick-object.md`

### SGX submit device sync objects

- Normalized the shared SGX sync side around explicit internal helpers:
  - `psb_sgx_lab_submit_sgx_device_sync_object`
  - `psb_sgx_lab_submit_sgx_hwdevice_sync_list`
  - updated `psb_sgx_lab_submit_sgx_cmdta_shared`
- Kernel-side changes in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- The shared builder now derives separate read/write pending values and
  complete GPU VA32 addresses for:
  - `ta_tq_sync`
  - `threed_tq_sync`
  - `ta3d_dependency_sync`
  - `hwdevice_sync_list`
- The validator now checks those explicit sync objects directly, and the
  temporary `ccb_kick` scratch object moved off stack so the old
  `-Wframe-larger-than=1024` warning is gone again
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-052151-guarded-deploy`
  - final live override:
    - `driver_srcversion=41BF53B53E121CEEB0F3594`
    - `override_module_size=449960`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-052334-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=41BF53B53E121CEEB0F3594`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-052451-sgx-submit-device-sync-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_live_stage_name=retired`
    - `submit_result_stage_name=retired`
    - `submit_manifest_stage_name=retired`
    - `submit_descriptor_stage_name=retired`
    - `submit_plan_stage_name=retired`
    - `submit_issue_plan_stage_name=retired`
    - `submit_issue_plan_dependency_active_count=0`
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-052529-sgx-multi-submit-device-sync-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_issue_plan_stage_name=retired`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_result_dependency_active_count=1`
      - `submit_manifest_dependency_active_count=1`
      - `submit_descriptor_dependency_active_count=1`
      - `submit_plan_dependency_active_count=1`
      - `submit_dependency_active_count=1`
      - `submit_issue_plan_stage_name=retired`
    - final cleanup returned:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-052334`
- Documented in:
  - `docs/172-sgx-submit-device-sync-objects.md`

### SGX submit CMDTA object

- Added one new kernel-internal SGX-shaped execution object:
  - `psb_sgx_lab_submit_sgx_cmdta`
- Kernel-side changes in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- The new `sgx_cmdta` object embeds the current shared layer and materializes
  the bounded sync-object arrays the current lab model really supports:
  - `ta_dst_syncs`
  - `threed_src_syncs`
- The submit validator now consumes `sgx_cmdta` instead of validating only the
  lower shared layer
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-053804-guarded-deploy`
  - final live override:
    - `driver_srcversion=2DA0FEA89EBE73F96BE4053`
    - `override_module_size=450568`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-053956-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=2DA0FEA89EBE73F96BE4053`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-054056-sgx-submit-cmdta-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_live_stage_name=retired`
    - `submit_result_stage_name=retired`
    - `submit_manifest_stage_name=retired`
    - `submit_descriptor_stage_name=retired`
    - `submit_plan_stage_name=retired`
    - `submit_issue_plan_stage_name=retired`
    - `submit_issue_plan_dependency_active_count=0`
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-054133-sgx-multi-submit-cmdta-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_issue_plan_stage_name=retired`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_result_dependency_active_count=1`
      - `submit_manifest_dependency_active_count=1`
      - `submit_descriptor_dependency_active_count=1`
      - `submit_plan_dependency_active_count=1`
      - `submit_dependency_active_count=1`
      - `submit_issue_plan_stage_name=retired`
    - final cleanup returned:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-053956`
- Documented in:
  - `docs/173-sgx-submit-cmdta-object.md`

### SGX submit queued TA command object

- Added one new kernel-internal SGX-shaped execution object:
  - `psb_sgx_lab_submit_sgx_queued_ta_command`
- Kernel-side changes in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- The new `sgx_queued_ta_command` object now wraps:
  - the validated `sgx_ta_command`
  - queue-header placement words
  - optional typed launch-time `issue-plan` state
- The submit validator now consumes `sgx_queued_ta_command` instead of
  stopping at `sgx_ta_command`, so one checked object now covers the queued TA
  command words, its typed CCB payload, queue placement, and optional
  `issue-plan`
- The first guarded deploy at
  - `logs/vaio-deploy/20260513-062738-guarded-deploy`
  missed the default rollback-confirmation window, but later journal evidence
  showed the candidate boot did reach Wi-Fi and SSH before the rollback reboot
- The successful rerun used a longer rollback grace and completed in:
  - `logs/vaio-deploy/20260513-070613-guarded-deploy`
  - final live override:
    - `driver_srcversion=2C7400755C1D450933F7344`
    - `override_module_size=451936`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-070759-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=2C7400755C1D450933F7344`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-070912-sgx-submit-queued-ta-command-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_live_stage_name=retired`
    - `submit_result_stage_name=retired`
    - `submit_manifest_stage_name=retired`
    - `submit_descriptor_stage_name=retired`
    - `submit_plan_stage_name=retired`
    - `submit_issue_plan_stage_name=retired`
    - `submit_issue_plan_dependency_active_count=0`
    - `submit_dependency_active_count=0`
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-070951-sgx-multi-submit-queued-ta-command-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_result_dependency_active_count=1`
      - `submit_manifest_dependency_active_count=1`
      - `submit_descriptor_dependency_active_count=1`
      - `submit_plan_dependency_active_count=1`
      - `submit_dependency_active_count=1`
    - final cleanup returned:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-070759`
- Documented in:
  - `docs/176-sgx-submit-queued-ta-command-object.md`

### SGX submit TA kick object

- Added one new kernel-internal SGX-shaped execution object:
  - `psb_sgx_lab_submit_sgx_ta_kick`
- Added one SGX-style command wrapper object:
  - `psb_sgx_lab_submit_sgx_command`
- Kernel-side changes in:
  - `third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500/psb_drv.c`
- The new `sgx_ta_kick` object now wraps one `sgx_cmdta` and materializes:
  - SGX command words:
    - `service_address`
    - `cache_control`
    - `data0`
    - `data1`
  - live GPU bindings:
    - `control_gpu_va32`
    - `ccb_gpu_va32`
    - `cmdta_gpu_va32`
    - `queue_gpu_va32`
    - `sync_gpu_va32`
    - `owner_gpu_va32`
    - `ta_status_gpu_va32`
    - `status_3d_gpu_va32`
- The submit validator now consumes `sgx_ta_kick` instead of validating only
  the lower `sgx_cmdta` layer, and the launch-time `issue-plan` branch now
  also rebuilds through the new TA-kick object
- Guarded deploy completed successfully in:
  - `logs/vaio-deploy/20260513-055407-guarded-deploy`
  - final live override:
    - `driver_srcversion=B0D06254613C58FF8D2BBF3`
    - `override_module_size=450652`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- The exact no-redeploy `plan` proof on the new module also still passed:
  - `logs/vaio-plan-session/20260513-055558-sgx-plan-session`
  - `session_outcome=completed`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=B0D06254613C58FF8D2BBF3`
- Focused direct hardware validation passed and is now saved in:
  - `logs/vaio-ioctl/20260513-055708-sgx-submit-ta-kick-live/summary.log`
  - final focused proof:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_live_stage_name=retired`
    - `submit_result_stage_name=retired`
    - `submit_manifest_stage_name=retired`
    - `submit_descriptor_stage_name=retired`
    - `submit_plan_stage_name=retired`
    - `submit_issue_plan_stage_name=retired`
    - `submit_issue_plan_dependency_active_count=0`
    - `submit_dependency_active_count=0`
    - `coord_live_submit_count=0`
    - `gtt_state_range_sgx_object_count=0`
- Typed multi-submit hardware validation also passed and is now saved in:
  - `logs/vaio-ioctl/20260513-055746-sgx-multi-submit-ta-kick-live/summary.log`
  - final typed run proved:
    - `SUBMIT_A_ID=3`
    - `SUBMIT_B_ID=4`
    - submit A final:
      - `submit_arm_count=3`
      - `submit_emit_count=3`
      - `submit_launch_count=3`
      - `submit_dispatch_count=3`
      - `submit_retired_count=3`
      - `submit_issue_count=3`
      - `submit_transition_count=6`
      - `submit_issue_plan_stage_name=retired`
      - `submit_dependency_active_count=0`
    - submit B final:
      - `submit_arm_count=2`
      - `submit_emit_count=2`
      - `submit_launch_count=2`
      - `submit_dispatch_count=2`
      - `submit_retired_count=2`
      - `submit_issue_count=2`
      - `submit_transition_count=4`
      - `submit_result_dependency_active_count=1`
      - `submit_manifest_dependency_active_count=1`
      - `submit_descriptor_dependency_active_count=1`
      - `submit_plan_dependency_active_count=1`
      - `submit_dependency_active_count=1`
      - `submit_issue_plan_stage_name=retired`
    - final cleanup returned:
      - `coord_live_submit_count=0`
      - `gtt_state_range_sgx_object_count=0`
- The shared current-boundary resolver therefore still holds terminal:
  - `resolved_profile=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-055558`
- Documented in:
  - `docs/174-sgx-submit-ta-kick-object.md`

2026-05-13: added `psb_sgx_lab_submit_sgx_ta_timeline` above
`psb_sgx_lab_submit_sgx_ta_execution`, split the duplicated TA command /
queued-command / issue-plan validation into shared helpers, and validated the
repaired runtime object on hardware. First draft:

- deploy:
  - `logs/vaio-deploy/20260513-123521-guarded-deploy`
  - live override `srcversion 82C2FB699E0C6937AB04CC2`
- exact `complete` still passed:
  - `logs/vaio-plan-session/20260513-123726-sgx-plan-session`
- focused typed submit still passed:
  - `logs/vaio-ioctl/20260513-123846-sgx-submit-ta-timeline-live/summary.log`
- typed multi-submit failed at `SUBMIT_A_AFTER_RUNNING_VALIDATE` with
  `submit_op_ret=-1 errno=22` in:
  - `logs/vaio-ioctl/20260513-123933-sgx-multi-submit-ta-timeline-live/summary.log`

Repaired draft:

- deploy:
  - `logs/vaio-deploy/20260513-124147-guarded-deploy`
  - final live override:
    - `driver_srcversion=A1A66310C19EE4235C25ED2`
    - `override_module_size=455932`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- exact proof back to green:
  - `logs/vaio-plan-session/20260513-124338-sgx-plan-session`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=A1A66310C19EE4235C25ED2`
- focused typed submit passed:
  - `logs/vaio-ioctl/20260513-124444-sgx-submit-ta-timeline-live/summary.log`
  - final focused state:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- typed multi-submit passed on the repaired validator:
  - `logs/vaio-ioctl/20260513-124523-sgx-multi-submit-ta-timeline-live/summary.log`
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A final:
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=1`
- documented in:
  - `docs/179-sgx-submit-ta-timeline-object.md`

2026-05-13: added `psb_sgx_lab_submit_sgx_ta_runtime` above
`psb_sgx_lab_submit_sgx_ta_timeline`, then fixed the first live validator draft
after it wrongly rejected unresolved dependencies during `prepare`.

First draft:

- deploy:
  - `logs/vaio-deploy/20260513-125935-guarded-deploy`
  - live override `srcversion BC32B83F0BFBD10FB1D0645`
- exact `complete` still passed:
  - `logs/vaio-plan-session/20260513-130122-sgx-plan-session`
- focused typed submit still passed:
  - `logs/vaio-ioctl/20260513-130233-sgx-submit-ta-runtime-live/summary.log`
- typed multi-submit failed at `submit_prepare 6` with `errno=22`:
  - `logs/vaio-ioctl/20260513-130314-sgx-multi-submit-ta-runtime-live/summary.log`
- traced failure bundle:
  - `logs/vaio-ioctl/20260513-130855-sgx-multi-submit-ta-runtime-trace/summary.log`

Repaired draft:

- deploy:
  - `logs/vaio-deploy/20260513-131259-guarded-deploy`
  - final live override:
    - `driver_srcversion=440FDB3F77708F25340DA25`
    - `override_module_size=456232`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- exact proof back to green:
  - `logs/vaio-plan-session/20260513-131445-sgx-plan-session`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=440FDB3F77708F25340DA25`
- focused typed submit passed:
  - `logs/vaio-ioctl/20260513-131554-sgx-submit-ta-runtime-live/summary.log`
  - final focused state:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- typed multi-submit passed on the repaired validator:
  - `logs/vaio-ioctl/20260513-131635-sgx-multi-submit-ta-runtime-live/summary.log`
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_dependency_active_count=1`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
- documented in:
  - `docs/180-sgx-submit-ta-runtime-object.md`

2026-05-13: added `psb_sgx_lab_submit_sgx_ta_gate` above
`psb_sgx_lab_submit_sgx_ta_runtime`, then switched the live
`transition -> running` path over to the same typed gate object that the
validator now checks.

- deploy:
  - `logs/vaio-deploy/20260513-151922-guarded-deploy`
  - final live override:
    - `driver_srcversion=643122821ACD1D37A506EB0`
    - `override_module_size=458160`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- exact proof back to green:
  - `logs/vaio-plan-session/20260513-152113-sgx-plan-session`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=643122821ACD1D37A506EB0`
- focused typed submit passed:
  - `logs/vaio-ioctl/20260513-152220-sgx-submit-ta-gate-live/summary.log`
  - final focused state:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- typed multi-submit passed on the new gate object:
  - `logs/vaio-ioctl/20260513-152220-sgx-multi-submit-ta-gate-live/summary.log`
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_dependency_active_count=1`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
- documented in:
  - `docs/181-sgx-submit-ta-gate-object.md`

2026-05-13: added `psb_sgx_lab_submit_sgx_ta_control` above
`psb_sgx_lab_submit_sgx_ta_gate`, and moved both launch-time issue readiness
and `transition -> running` admission onto that shared typed control object.

- deploy:
  - `logs/vaio-deploy/20260513-153601-guarded-deploy`
  - final live override:
    - `driver_srcversion=B77A6FEA8B9671946B32272`
    - `override_module_size=459916`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- exact proof back to green:
  - `logs/vaio-plan-session/20260513-153751-sgx-plan-session`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=B77A6FEA8B9671946B32272`
- focused typed submit passed:
  - `logs/vaio-ioctl/20260513-153922-sgx-submit-ta-control-live/summary.log`
  - final focused state:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- typed multi-submit passed on the new control object:
  - `logs/vaio-ioctl/20260513-154028-sgx-multi-submit-ta-control-live/summary.log`
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_dependency_active_count=1`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
- documented in:
  - `docs/182-sgx-submit-ta-control-object.md`

2026-05-13: added `psb_sgx_lab_submit_sgx_ta_schedule` above
`psb_sgx_lab_submit_sgx_ta_control`, and moved queue/ticket/sync progression
for live owner-state transitions onto that shared typed schedule object.

- deploy:
  - `logs/vaio-deploy/20260513-160724-guarded-deploy`
  - final live override:
    - `driver_srcversion=801A7DA2CBDDDCBEBC3AFEF`
    - `override_module_size=461036`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- exact proof back to green:
  - `logs/vaio-plan-session/20260513-160909-sgx-plan-session`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=801A7DA2CBDDDCBEBC3AFEF`
- focused typed submit passed:
  - `logs/vaio-ioctl/20260513-161024-sgx-submit-ta-schedule-live/summary.log`
  - final focused state:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- typed multi-submit passed on the new schedule object:
  - `logs/vaio-ioctl/20260513-161102-sgx-multi-submit-ta-schedule-live/summary.log`
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_dependency_active_count=1`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
- documented in:
  - `docs/183-sgx-submit-ta-schedule-object.md`

2026-05-13: added `psb_sgx_lab_submit_sgx_ta_dispatch` above
`psb_sgx_lab_submit_sgx_ta_schedule`, and moved the launch-side issue
writeback image onto that shared typed dispatch object.

- deploy:
  - `logs/vaio-deploy/20260513-162106-guarded-deploy`
  - final live override:
    - `driver_srcversion=AA6BEDA312078692D47D0A8`
    - `override_module_size=462560`
    - `overall=remote_usable`
    - `lightdm_active=inactive`
    - `kdump_service_active=active`
- exact proof back to green:
  - `logs/vaio-plan-session/20260513-162255-sgx-plan-session`
  - `selected_phase_profile=complete`
  - `final_driver_srcversion=AA6BEDA312078692D47D0A8`
- focused typed submit passed:
  - `logs/vaio-ioctl/20260513-162355-sgx-submit-ta-dispatch-live/summary.log`
  - final focused state:
    - `submit_prepare_count=1`
    - `submit_arm_count=1`
    - `submit_emit_count=1`
    - `submit_launch_count=1`
    - `submit_dispatch_count=1`
    - `submit_retired_count=1`
    - `submit_issue_count=1`
    - `submit_transition_count=2`
    - `submit_dependency_active_count=0`
- typed multi-submit passed on the new dispatch object:
  - `logs/vaio-ioctl/20260513-162434-sgx-multi-submit-ta-dispatch-live/summary.log`
  - `SUBMIT_A_ID=3`
  - `SUBMIT_B_ID=4`
  - submit A final:
    - `submit_arm_count=3`
    - `submit_emit_count=3`
    - `submit_launch_count=3`
    - `submit_dispatch_count=3`
    - `submit_retired_count=3`
    - `submit_issue_count=3`
    - `submit_transition_count=6`
    - `submit_dependency_active_count=0`
  - submit B final:
    - `submit_arm_count=2`
    - `submit_emit_count=2`
    - `submit_launch_count=2`
    - `submit_dispatch_count=2`
    - `submit_retired_count=2`
    - `submit_issue_count=2`
    - `submit_transition_count=4`
    - `submit_dependency_active_count=1`
    - `dependency[1]_required_ticket=3`
    - `dependency[1]_observed_complete_ticket=5`
- documented in:
  - `docs/184-sgx-submit-ta-dispatch-object.md`
