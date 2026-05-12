# Fixed-Module `plan` Reruns Through `arm`

## Summary

After the console panic on the older live override `srcversion
0EEEE302E3FDA9F59B7CFB9`, I switched the `plan` operator path to a guarded
deploy of the fixed local tree and reran the narrowed exact phases on the new
module.

The new live override is:

- `driver_srcversion=6E95EDFFFB5384C8E58F25B`
- `override_module_size=429020`
- `driver_module_loaded=1`
- `drm_card0_present=1`
- `lightdm_active=inactive`
- `xorg_process_count=0`

The guarded deploy bundle is:

- [logs/vaio-plan-session/20260512-213102-sgx-plan-session](/root/Poulsbo/logs/vaio-plan-session/20260512-213102-sgx-plan-session)

That session proved:

- deploy completed cleanly on the fixed module
- `plan_query_offset` is healthy on `6E95EDFFFB5384C8E58F25B`

## Exact Reruns

Exact recovery reruns on the new module all completed cleanly:

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

Each saved bisect child recorded:

- `session_outcome=all_prefixes_healthy`
- `first_bad_phase=none`
- `last_overall=remote_usable`

The `arm` phase log also proved the entire ladder below it is now clean on the
fixed module:

- `STAGE_OK=plan_query_fetch`
- `STAGE_OK=plan_query_offset`
- `STAGE_OK=plan_query_config`
- `STAGE_OK=plan_query_snapshot`
- `STAGE_OK=prepare_op`
- `STAGE_OK=prepare_query`
- `STAGE_OK=arm`

## Operator Follow-Up

The shared current-boundary resolver had been stale because it only considered
remote-usable rows with failures. It now also consumes successful exact
phase-evidence rows and advances to the next phase in the full ladder.

I also extended the named exact profiles beyond `plan_query*`:

- `prepare_op`
- `prepare_query`
- `arm`
- `emit`
- `validate`
- `launch`
- `issue`
- `running`
- `complete`

The current saved recovery boundary is now:

- `resolved_profile=emit`
- `resolved_phases=emit`
- `resolved_phase=emit`
- `resolved_reason=latest_phase_evidence_advanced`
- `source_timestamp=20260512-214548`

The next exact live command is therefore:

```bash
PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=emit ./scripts/run-vaio-sgx-plan-session.sh
```
