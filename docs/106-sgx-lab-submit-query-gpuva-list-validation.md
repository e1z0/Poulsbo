# SGX Lab Submit Query GPU-VA List Validation

Date: 2026-05-11
Target: Sony VAIO VGN-P11Z (`10.1.11.218`)
Module: `gma500_gfx`, `srcversion 0F9AE3730BF455D57D74E17`

## Goal

The direct GPU-VA submit-query path can now disambiguate one concrete match by
ordinal through `gpuva-match`, but the repo still lacked a canonical helper
that enumerates the whole live match set for one shared GPU VA in one run.

This follow-up adds that wrapper-level enumeration flow on top of the already
validated typed query surface, without changing the kernel UAPI again.

## Changes

Validation runners:

- [run-vaio-sgx-ioctl-submit-query-gpuva.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh)
- [run-vaio-sgx-ioctl-submit-query-gpuva-list.sh](/root/Poulsbo/scripts/run-vaio-sgx-ioctl-submit-query-gpuva-list.sh)

The base runner now supports:

- `ENUMERATE_MATCHES=1`

In the shared-control two-submit scenario, that mode now records the full match
set through repeated typed `gpuva-match` queries at three points:

- while both submits are alive
- after userspace holder exit
- after the primary submit drops

The new wrapper fixes that mode on as the canonical live list flow:

- `SHARED_CONTROL_SUBMITS=2`
- `ENUMERATE_MATCHES=1`

## Validation

Local checks:

- `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva.sh`
- `bash -n scripts/run-vaio-sgx-ioctl-submit-query-gpuva-list.sh`

Canonical live bundle:

- [logs/vaio-ioctl/20260511-164757-sgx-submit-query-gpuva-list-live](/root/Poulsbo/logs/vaio-ioctl/20260511-164757-sgx-submit-query-gpuva-list-live)

Saved result:

- `session_kind=sgx_submit_query_gpuva_list_validation`
- `session_outcome=completed`
- `finished_at=2026-05-11T16:48:37+03:00`

Target state from that bundle:

- `overall=remote_usable`
- `driver_srcversion=0F9AE3730BF455D57D74E17`
- `override_module_size=387528`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

Canonical live evidence on the shared control-object GPU VA:

- live staged submits:
  - `SUBMIT_ID=3`
  - `SECOND_SUBMIT_ID=4`

Wrapper-level enumeration while both submits exist:

- `SUBMIT_QUERY_GPUVA_LIST_LIVE_0`
  - `submit_resolved_id=3`
  - `submit_matching_submit_count=2`
  - `submit_resolved_match_index=0`
- `SUBMIT_QUERY_GPUVA_LIST_LIVE_1`
  - `submit_resolved_id=4`
  - `submit_matching_submit_count=2`
  - `submit_resolved_match_index=1`

The same enumeration after userspace holder exit:

- `SUBMIT_QUERY_GPUVA_LIST_POST_PROBE_EXIT_0`
  - `submit_resolved_id=3`
  - `submit_matching_submit_count=2`
  - `submit_resolved_match_index=0`
- `SUBMIT_QUERY_GPUVA_LIST_POST_PROBE_EXIT_1`
  - `submit_resolved_id=4`
  - `submit_matching_submit_count=2`
  - `submit_resolved_match_index=1`

After the primary submit drops:

- `SUBMIT_QUERY_GPUVA_LIST_AFTER_PRIMARY_DROP_0`
  - `submit_resolved_id=4`
  - `submit_matching_submit_count=1`
  - `submit_resolved_match_index=0`

That matches the already-validated lower-level behavior:

- while both submits exist:
  - `gpuva` resolves submit `3`
  - `gpuva-match 0` resolves submit `3`
  - `gpuva-match 1` resolves submit `4`
- after primary submit drop:
  - `gpuva` resolves submit `4`
  - `gpuva-match 0` resolves submit `4`
  - later cleanup returns to `ENOENT`

## Result

The repo now has a canonical typed enumeration flow for one shared GPU VA:

- full live match list while both submits exist
- stable full list after userspace holder exit
- stable reduced list after the primary submit drops

That closes the last practical wrapper-level gap in the current direct
GPU-VA-to-submit inspection flow without adding more kernel-side API surface.
