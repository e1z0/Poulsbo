# Active TODO

This file tracks the current prioritized work. Completed work should move to
`docs/11-progress-log.md`. New work should be added here before or alongside
implementation if it materially changes scope.

## Current State

- old confirmed live blocker on override `srcversion 2BE09D0DC1CCBDB51D2328E`
  remains historical context only:
  - `selected_failed_phase=sync`
  - `selected_failed_step=submit_op_kick_focus`
  - `selected_failed_rc=1`
- old later candidate `srcversion 0EEEE302E3FDA9F59B7CFB9` also remains as
  historical context only:
  - it required autoloading `gma500_gfx` on a headless boot
  - it later timed out at `plan_query_offset`
  - it then panicked in `poulsbo_sgx_fill_submit_query_locked`
  - it is unsafe for further recovery-side `plan_query*` reruns
- the current live override is now the latest guarded-deploy validated module:
  - `srcversion 228151D0E1E224804A6E1E6`
  - `override_module_size=446824`
  - `driver_module_loaded=1`
  - `drm_card0_present=1`
  - `lightdm_active=inactive`
  - `xorg_process_count=0`
- the VAIO is now also panic-safe for risky SGX work:
  - `crashkernel=128M`
  - `kernel_panic=10`
  - `kernel_panic_on_oops=1`
  - `kernel_sysrq=1`
  - `kdump_service_enabled=enabled`
  - `kdump_service_active=active`
  - `kexec_crash_loaded=1`
  - `kdump-config status -> current state   : ready to kdump`
  - setup bundle:
    - `logs/vaio-panic/20260512-221904-panic-capture-setup`
  - destructive validation bundle:
    - `logs/vaio-panic-test/20260512-222708-panic-test`
  - refreshed collector bundle:
    - `logs/vaio-panic/20260512-223405-panic-capture-setup`
  - latest captured dump:
    - `/var/crash/202605122227/dump.202605122227`
    - `/var/crash/202605122227/dmesg.202605122227`
  - fetched local crashdump:
    - `logs/vaio-panic-test/20260512-222708-panic-test/panic-artifacts/crashdump/202605122227`
- guarded deploy bundle:
  - `logs/vaio-deploy/20260513-023337-guarded-deploy`
- exact live reruns on the fixed module now all complete cleanly:
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
- canonical bundles for that fixed-module line:
  - `logs/vaio-plan-session/20260513-024621-sgx-plan-session`
- the newest typed submit `issue-plan` block batch is now hardware-validated:
  - exact proof:
    - `logs/vaio-plan-session/20260513-024621-sgx-plan-session`
  - focused bundle:
    - `logs/vaio-ioctl/20260513-023956-sgx-submit-issue-plan-live/summary.log`
  - typed multi-submit bundle:
    - `logs/vaio-ioctl/20260513-024024-sgx-multi-submit-issue-plan-live/summary.log`
- the shared current-boundary resolver now advances from successful exact
  evidence rows, so the current saved recovery boundary is:
  - `resolved_profile=complete`
  - `resolved_phase=complete`
  - `resolved_phases=complete`
  - `resolved_reason=latest_phase_evidence_terminal`
  - `source_timestamp=20260513-024621`
  - `source_row_module_srcversion=228151D0E1E224804A6E1E6`
- the summary/report layer now prefers the live
  `final_driver_srcversion` over `local_module_srcversion`, so the current
  saved source row reports the actually loaded candidate instead of the local
  build artifact
- exact operator profiles are now first-class through the current boundary:
  - `plan_query_fetch`
  - `plan_query_basic`
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
- local kernel follow-up around the live `plan` candidate: the submit memory
  model is now hardened on both sides, so overlapping control-side
  `queue + ccb + manifest + descriptor + plan` regions and overlapping
  status-side `status + sync + owner + report + dependency` regions are
  rejected before any refresh write, and the central submit validator has now
  been split so those backing-image checks no longer trigger the earlier
  large-frame warning; the submit-view refresh path now also preflights
  control/status layout plus `report/manifest/descriptor/plan` construction
  before the first write, and the `report` block itself now uses the same
  structured build/read/write helper path as the other memory-backed submit
  views; the remaining fixed blocks `queue header + ccb record + sync +
  owner` now also use structured helper paths across configure, prepare,
  issue, transition, validation, and query readback; dependency-state slots
  now also use the same structured helper pattern across refresh, validation,
  and clear; the latest kernel tree is build-clean locally; see
  [docs/128-sgx-submit-control-layout-hardening.md](/root/Poulsbo/docs/128-sgx-submit-control-layout-hardening.md)
  and
  [docs/129-sgx-submit-status-layout-hardening.md](/root/Poulsbo/docs/129-sgx-submit-status-layout-hardening.md)
  and
  [docs/130-sgx-submit-validator-stack-split.md](/root/Poulsbo/docs/130-sgx-submit-validator-stack-split.md)
  and
  [docs/131-sgx-submit-refresh-preflight.md](/root/Poulsbo/docs/131-sgx-submit-refresh-preflight.md)
  and
  [docs/132-sgx-submit-report-struct-normalization.md](/root/Poulsbo/docs/132-sgx-submit-report-struct-normalization.md)
  and
  [docs/133-sgx-submit-fixed-block-struct-normalization.md](/root/Poulsbo/docs/133-sgx-submit-fixed-block-struct-normalization.md)
  and
  [docs/134-sgx-submit-dependency-block-struct-normalization.md](/root/Poulsbo/docs/134-sgx-submit-dependency-block-struct-normalization.md)
  and
  [docs/138-sgx-submit-word-block-size-hardening.md](/root/Poulsbo/docs/138-sgx-submit-word-block-size-hardening.md)
  and
  [docs/139-sgx-plan-pipefree-timeout-capture.md](/root/Poulsbo/docs/139-sgx-plan-pipefree-timeout-capture.md)
- the shared kernel-internal execution record now sits under the typed submit
  memory model and is already consumed by:
  - `queue header`
  - `ccb record`
  - `sync`
  - `owner`
  - `report`
  - `live`
  - `manifest`
  - `descriptor`
  - `plan`
  - `result`
- first post-recovery validation path: do not rerun the full focused submit
  batch first; use the composed investigation harness in
  [docs/124-sgx-submit-plan-investigation-harness.md](/root/Poulsbo/docs/124-sgx-submit-plan-investigation-harness.md)
  via `./scripts/run-vaio-sgx-submit-op-phase-investigation.sh`
- preferred one-command recovery entrypoint:
  `./scripts/run-vaio-sgx-plan-recovery-session.sh`
- preferred one-command full-cycle rerun once the target is back:
  `./scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`
- preferred operator entrypoint regardless of current target state:
  `./scripts/run-vaio-sgx-plan-session.sh`
- current default phase-profile policy:
  - recovery / no redeploy:
    - `current -> complete` from the newest remote-usable successful exact phase
      evidence
  - fresh deploy:
    - `full`
- current operator/session observability:
  - parent results and session summary now carry the resolved phase profile
    directly, so blocked `plan` runs no longer require opening child manifests
    to tell whether they used `full`, `post_descriptor`, or the current
    `plan_query` boundary
  - the offline resolver / report / summary path now also shares one parsing
    helper:
    - `./scripts/lib-vaio-plan-summary.sh`
  - the boundary/source-row field set now also shares one dedicated helper:
    - `./scripts/lib-vaio-plan-phase-source.sh`
  - the current-boundary resolver and report now ignore newer remote-usable
    in-progress `plan_session` parents unless they carry actual evidence, so
    an in-flight rerun cannot temporarily broaden the saved boundary away from
    the last stable proof while it is still incomplete
- preferred exact live rerun on the current fixed line:
  `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete ./scripts/run-vaio-sgx-plan-session.sh`
- current target state:
  - `ping_status=ok`
  - `tcp_22_status=open`
  - `ssh_pubkey_status=ok`
  - `overall=remote_usable`
- real hardware target: Sony VAIO VGN-P11Z at `10.1.11.218`
- target kernel: `5.4.0-150-generic` on `i686`
- build rule: build locally in this repo, deploy to the target, never build on
  the target
- last confirmed booted override on the VAIO:
  `srcversion 864D7160A07B237AB1A9D36`
- current execution-shaped submit memory model:
  `queue + ccb + sync + owner + report + live + result + manifest + descriptor + plan + issue-plan`
- current shared execution-shaped kernel record:
  `psb_sgx_lab_submit_exec_record`

## Current Priority

- keep the old `0EEEE302E3FDA9F59B7CFB9` candidate blocked for recovery-side
  `plan_query*` work after the `poulsbo_sgx_fill_submit_query_locked` panic
- keep the current guarded-deploy override `228151D0E1E224804A6E1E6` as the
  active headless, panic-safe validation base
- treat the exact single-submit ladder as closed for the current module:
  - the shared resolver now lands on terminal `complete`
  - use `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete ./scripts/run-vaio-sgx-plan-session.sh`
    as the exact regression check after risky kernel changes
- use the new typed `issue-plan`-block validation bundles as the active typed-submit
  validation surface:
  - focused proof:
    `logs/vaio-ioctl/20260513-023956-sgx-submit-issue-plan-live/summary.log`
  - typed multi-submit proof:
    `logs/vaio-ioctl/20260513-024024-sgx-multi-submit-issue-plan-live/summary.log`
- keep the new phase-profile defaults unless a session is intentionally trying
  to re-check earlier stages:
  - `PLAN_PHASE_PROFILE=auto`
  - recovery resolves to `current -> complete`
  - deploy resolves to `full`
- prefer the plan-specific aliases at the operator layer instead of the
  generic `PHASES` / `PHASE_PROFILE` names:
  - `PLAN_PHASE_PROFILE=<name>`
  - `PLAN_PHASES=<csv>`
- use the explicit recovery-side alias when retrying the current exact live
  question on the fixed module:
  - `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete ./scripts/run-vaio-sgx-plan-session.sh`
- next kernel-side goal:
  - keep moving the shared `psb_sgx_lab_submit_exec_record` toward the first
    more execution-oriented SGX kernel structure above the now-validated typed
    `queue + ccb + sync + owner + report + live + result + manifest +
    descriptor + plan + issue-plan` image instead of rebuilding more read-side block
    plumbing
- read the saved parent result with the newer semantics:
  - `selected_phase_selection=explicit` is the operator choice
  - `selected_child_phase_selection=request_only` is the child recovery
    wrapper's blocked-target state
- use the new diagnosis summary before deciding which saved bundle is the
  current reference:
  - `MAX_SESSIONS=10 ./scripts/summarize-vaio-plan-sessions.sh`
- use the current-state reporter before the next live rerun:
  - `./scripts/report-vaio-plan-status.sh`
- that report now prefers the newest remote-usable successful exact phase
  evidence too, so the current exact regression should come back as:
  - `PLAN_PHASE_PROFILE=complete`
- recovery-mode `auto` now also resolves through the same shared current-phase
  helper, so the default no-redeploy operator path should land on the same
  exact `complete` boundary as the status report
- the saved parent bundles preserve request-versus-resolution provenance, and
  the current exact source row now points at the newest clean `complete` rerun:
  - `source_kind=plan_session`
  - `source_name=sgx-plan-session`
  - `source_timestamp=20260513-005336`
  - `source_row_requested=complete:profile[complete]`
  - `source_row_evidence_last_good=complete`
- use the current live fixed module for continued no-redeploy exact reruns:
  - `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete ./scripts/run-vaio-sgx-plan-session.sh`
- keep the layout-hardened, validator-split, refresh-preflighted kernel
  candidate as the active diagnosis base so remaining failures are less likely
  to be silent submit-block self-corruption
- keep pushing the new shared `psb_sgx_lab_submit_exec_record` toward a more
  real SGX execution model instead of rebuilding more parallel per-view state
- current validated 2D path: fbdev `fillrect`, `copyarea`, `xor fill`, and
  chunked mono `imageblit`
- current X stack state: `modesetting` plus software `llvmpipe`
- current render state: KMS and dumb buffers only, no SGX render uAPI
- current binding observability: live SGX state, live GTT/MMU range state, and
  explicit PF-PD bind / unbind over real GEM objects
- current validated target state: the VAIO is back at `overall=remote_usable`,
  running override module `srcversion 21D34B366D429B395AD45DC`; the indexed
  `depclear_slot` path is now hardware-validated through the submit-op ioctl
  path as well as the older sysfs path, the rollback guard is now validated in
  both the healthy-confirmed and automatic previous-override restore paths; the
  bounded SGX lab query, range-query, SGX-state query, object-op, and
  GTT-state query, submit-op ioctl surfaces are now hardware-validated across
  both the object path and the submit path; the current execution-shaped SGX
  lab control-side memory model now includes the validated `report`,
  `manifest`, and `descriptor` blocks plus the expanded 8-dword CCB record;
  the four-lane dependency-slot-clear scenario is now also hardware-validated with
  `USE_IOCTL_RANGE_DISCOVERY=1`, `USE_IOCTL_OBJECT_SETUP=1`,
  `USE_IOCTL_SUBMIT_SETUP=1`, and `USE_IOCTL_DEPENDENCY_SETUP=1`, so the
  multi-submit range-discovery hot path, object lifecycle and configuration
  path, submit lifecycle and setup path, and indexed dependency mutation path
  no longer depend on sysfs writes; the remaining custom typed query helpers
  now also use raw `ioctl()` and are directly validated on the VAIO, and the
  SGX snapshot path plus the global GTT/MMU state path no longer depend
  entirely on `poulsbo_sgx_state` or `poulsbo_gtt_state`; the ioctl-based
  stale cleanup path for submits, focused-submit state, and adopted objects is
  now also hardware-validated and the ioctl-based runners now prefer that path
  before falling back to older sysfs drop loops; the object-side read path is
  now also hardware-validated on a bounded object-query ioctl path; the
  submit-side read path is now also hardware-validated on a bounded
  submit-query-by-index ioctl path; the same typed query helpers now also
  support a fully no-sysfs SGX lab state-dump flow validated on the VAIO, and
  the same path now has a standalone `psb_sgx_lab_dump` userspace tool also
  validated on hardware, including both human-readable text output and
  structured JSON output; the focused cleanup validator now also uses only
  typed coord, submit, object, range, and dump reads for verification instead
  of reading `poulsbo_sgx_coord_state`, `poulsbo_sgx_submits`,
  `poulsbo_sgx_objects`, or `poulsbo_gtt_state`; the focused object-op
  validator now also uses only typed object, range, and global GTT/MMU reads
  instead of `poulsbo_sgx_objects` or `poulsbo_gtt_state`; the focused
  submit-op validator now also uses only typed object, range, submit, coord,
  and global GTT/MMU reads instead of `poulsbo_sgx_object`,
  `poulsbo_sgx_objects`, `poulsbo_sgx_submit`, `poulsbo_sgx_submits`,
  `poulsbo_sgx_submit_state`, `poulsbo_sgx_coord_state`, or
  `poulsbo_gtt_state`; the higher-level multi-submit runner now also has a
  hardware-validated `USE_IOCTL_VERIFICATION=1` mode, and the full four-lane
  dependency-slot-clear scenario is now validated there with typed object,
  submit, dependency, range, coord, GTT, and dump helpers instead of the old
  SGX and GTT sysfs text nodes; the dependency-specific wrapper layer now also
  defaults to that same typed path through `run-vaio-sgx-multi-submit-default.sh`,
  so the slot-clear, indexed-array, third-lane, multi-source, and dependency-
  drop wrappers no longer route callers back onto the older direct multi-submit
  entrypoint by default, and now keep that path only behind
  `SGX_MULTI_SUBMIT_DRIVER_MODE=legacy`; the older focused helper names `run-vaio-sgx-object.sh` and
  `run-vaio-sgx-submit.sh` now also default onto typed wrappers, with the
  older sysfs-heavy script bodies kept only behind
  `SGX_OBJECT_DRIVER_MODE=legacy` and `SGX_SUBMIT_DRIVER_MODE=legacy`; those
  explicit legacy modes are now also hardware-validated, including the fixed
  `RANGE_SCAN_MAX` path in the focused legacy submit helper and the same
  symmetric `RANGE_SCAN_MAX` path in the direct legacy multi-submit helper;
  the repo now also has hardware-validated typed-versus-legacy compare
  harnesses for the focused object, focused submit, generic multi-submit, and
  dependency-slot-clear, indexed-dependency-array, and multi-source-ticket
  wrapper families, and
  `scripts/summarize-vaio-sessions.sh` now sorts session bundles by timestamp
  so those fresh compare runs surface first; the typed GPU-VA range-query path
  now also reports SGX object semantics plus live submit-reference count, first
  submit ID, and first submit slot, and that full object-plus-submit
  transition is now hardware-validated through typed adopt, attach, holder
  exit, submit drop, and object drop on the VAIO; the bounded submit-query
  path now also resolves the same GPU VA directly back to a live submit, its
  matching slot, its matching object, its backing range ID and range type, and
  the matched object’s role/class/gpu_use/context plus the in-range offset
  used for that match, and that read-side path is now hardware-validated
  before holder exit, after holder exit, and after typed submit drop; the same
  direct path now also reports how many live submits currently match that same
  GPU VA, and the shared-object two-submit handoff case is now
  hardware-validated through first-submit resolution, first-submit drop,
  second-submit resolution, and final `ENOENT`, with direct
  `submit_matching_submit_count` progression `2 -> 1 -> ENOENT`; the same
  direct path can now also select a concrete GPU-VA match ordinal through
  `gpuva-match`, with live hardware validation for:
  - `match 0 -> submit A`
  - `match 1 -> submit B`
  - after submit-A drop: `match 0 -> submit B`
  - after submit-A drop: `match 1 -> ENOENT`
  - the repo now also has a canonical wrapper-level list mode for that same
    path, with hardware validation for:
    - `list[0] -> submit A`
    - `list[1] -> submit B`
    - after holder exit: same two-entry list
    - after submit-A drop: one-entry list for submit B
  - the typed submit-op path now also has a hardware-validated `prepare`
    materialization stage, and both the focused submit-op runner and the typed
    multi-submit runner now use it before the first progression step, so the
    lab path can validate one coherent control/status image in backing memory
    before `issue` or `transition`
  - the same typed submit-op path now also has a hardware-validated `arm`
    progression stage, and both the focused submit-op runner and the typed
    multi-submit runner now use:
    - `prepare`
    - `arm`
    - `validate`
    - `issue`
    so the lab path can prove one committed issue-side snapshot instead of
    allowing issue directly from any mutable prepared state
  - the multi-submit runner now also supports `+<n>` relative dependency-ticket
    syntax resolved against the current coordination baseline, so multi-source
    ticket scenarios no longer silently break once the persistent global
    completion counter grows well past the old absolute thresholds
  - the compare wrappers now also have a typed reset hook through
    `scripts/run-vaio-sgx-lab-reset.sh`, and the dependency-slot-clear compare
    wrapper is now revalidated with:
    - `pre_reset_rc=0`
    - `inter_reset_rc=0`
    - `legacy_rc=0`
  - the focused object and focused submit compare wrappers are now also
    revalidated through that same reset-assisted path, and the focused legacy
  submit helper plus the reset helper now use the typed range-query helper
  and the fixed `range_type_name` / `range_ioctl_ret` parsers for warm-target
  GEM-baseline discovery and convergence checks
  - the direct legacy multi-submit path now also uses the typed range-query
    helper for before/during live GEM baselines when that query path is
    available, so warm-target legacy compare sessions no longer depend on the
    truncated `poulsbo_gtt_state` text dump to identify fresh high range IDs
  - the execution-shaped typed submit path now also has a hardware-validated
    control-side `manifest` block in backing memory, so the live focused and
    typed multi-submit runners both validate:
    - stage and stage-count progression
    - queue and owner transitions
    - dependency-active state
    - GPU-VA32 words that match the expanded CCB record
    through the same bounded ioctl path as the rest of the current lab submit
    model
- current execution-shaped submit state: the bounded typed submit path now also
  has hardware-validated `emit`, `launch`, persistent post-`issue`
  `dispatch`, persistent `retired`, and memory-backed typed `report` and
  `manifest` blocks on the VAIO
  - focused path:
    - `submit_emit_count=1`
    - `submit_emit_valid=1 -> 0` across `emit -> issue`
    - `submit_launch_count=1`
    - `submit_launch_valid=1`
    - `submit_launch_consumed=0 -> 1` across `launch -> issue`
  - typed multi-submit path:
    - submit A final `submit_emit_count=3`
    - submit A final `submit_launch_count=3`
    - submit B final `submit_emit_count=2`
    - submit B final `submit_launch_count=2`
    - submit B emitted state proves `submit_emit_dependency_active_count=1`
    - submit B launched state proves `submit_launch_dependency_active_count=1`
    - submit A final `submit_dispatch_count=3`
    - submit A final `submit_issue_count=3`
    - submit A final `submit_transition_count=6`
    - submit B final `submit_dispatch_count=2`
    - submit B final `submit_dispatch_dependency_active_count=1`
    - submit B final `submit_issue_count=2`
    - submit B final `submit_transition_count=4`
  - focused path now also proves one persistent dispatch image after
    `issue -> running -> complete`:
    - coordination baseline `issue/running/complete = 6/6/6`
    - final coordination state `7/7/7`
    - `submit_dispatch_count=1`
    - `submit_dispatch_owner_state_name=complete`
    - `submit_dispatch_completed_packets=2`
    - `submit_dispatch_global_issue_ticket=7`
    - `submit_dispatch_global_running_ticket=7`
    - `submit_dispatch_global_complete_ticket=7`
  - the current typed progression contract is now:
    - `prepare`
    - `arm`
    - `validate`
    - `emit`
    - `validate`
    - `launch`
    - `validate`
    - `issue`
    - `transition running/complete` while preserving one persistent dispatched
      execution snapshot
  - the current report path is now also validated on real hardware:
    - focused report-stage progression:
      - `none`
      - `prepared`
      - `armed`
      - `emitted`
      - `launched`
      - `dispatched`
      - `retired`
    - focused report-flag progression:
      - `0x0`
      - `0x1`
      - `0x3`
      - `0x7`
      - `0x1c`
      - `0x10`
      - `0x30`
    - typed multi-submit refill cycles now re-expose the current live
      `prepared/armed/emitted/launched` report stage instead of sticking on an
      older persisted `retired` snapshot
  - the current memory-backed CCB record is now a validated SGX-shaped
    non-executing 8-dword record:
    - service address
    - cache control
    - data0
    - data1
    - `pds_gpu_va32`
    - `use_gpu_va32`
    - `target_gpu_va32`
    - `status_gpu_va32`
  - focused and typed multi-submit validation now both prove that those four
    GPU-VA32 words match the live attached object GPU VAs before issue-side
    progression
  - final typed cleanup after the canonical reruns still returns:
    - `coord_live_submit_count=0`
    - `coord_live_object_count=0`
    - `gtt_state_range_sgx_object_count=0`
    - `gtt_state_range_pf_bound_count=0`
- current GPU-VA read-side correlation state: the direct submit-query and
  direct range-query families now both support shared-object multi-submit
  inspection on real hardware
  - submit-query side:
    - matching-submit count
    - explicit `gpuva-match <gpu_va> <match_index>`
    - wrapper-level match-list enumeration
  - range-query side:
    - submit-reference count
    - first matching submit ID and slot
    - explicit `gpuva-submit <gpu_va> <submit_match_index>`
  - both sides are hardware-validated across:
    - two live submits sharing one control-object GPU VA
    - userspace holder exit
    - primary submit drop
    - final submit drop
- current object state: kernel-owned SGX lab objects can adopt live GEM ranges
  and outlive userspace close until explicit drop; they now also carry
  explicit `role`, `object_class`, intended `gpu_use`, and expected-context
  metadata, and all of those bounded object-side mutations are now
  hardware-validated through the object-op ioctl surface, including inside the
  real multi-submit dependency-slot-clear scenario
- current submit state: PF-context SGX lab submit descriptors now carry a
  validate-only TA-style kick packet and can validate a five-object
  `control/pds/use/target/status` topology with resolved GPU VAs after
  userspace exit; the `control` object now carries a minimal CCB/control-block
  model plus explicit wrap-aware queue semantics with tail/head packet split
  readback, and the `status` object now carries explicit TA/3D status backing
  with readback validation; the local tree now also has a validate-only sync
  block in the `status` object, and that path is now validated on the VAIO in
  both wrap and nowrap queue modes; the same `status` object now also carries a
  separate validate-only owner/completion block validated in both queue modes;
  the submit path now also validates explicit `queued -> running -> complete`
  transitions in both queue modes with queue, sync, and owner state advancement;
  it now also validates explicit issue-side queue growth in both queue modes
  with `write_offset`, `sync_pending_seq`, and `owner_issue_cookie` advancement;
  it now also validates one repeated refill cycle on the same submit object
  with `complete -> queued -> running -> complete` progression and explicit
  `queue_epoch` tracking; the sync and owner blocks now also mirror
  `queue_epoch` in memory and the submit path is validated across three refill
  epochs on one persistent submit object with `REFILL_CYCLES=3`; two
  persistent submits are now also validated concurrently with focused
  per-submit inspection and isolated epoch/sync/owner progression; that same
  path now also carries explicit driver-global issue, running, and completion
  counters with per-submit `global_issue_ticket` /
  `global_running_ticket` / `global_complete_ticket` mirroring through the
  sync and owner blocks, including real `-EBUSY` arbitration when one submit
  tries to enter `running` while another submit already owns that lane; the
  same flow now also uses `ta_dependency` as a real completion-ticket wait gate
  with `-EAGAIN` before the prerequisite completion ticket exists; it now also
  carries explicit `dependency_submit_id` binding so one submit can wait on a
  concrete prerequisite submit instead of only on an anonymous global ticket;
  the same path now also snapshots dependency state into the `status` object
  through `depstate`, including explicit source/required/observed ticket fields
  and dependency flags; that dependency block is now also live-updated on
  source-submit completion after one setup-time `depstate` command; it now
  also carries an explicit missing-source state when the bound source submit is
  dropped before satisfying the dependency gate, and a validated
  satisfied-plus-missing state when that source is dropped only after
  satisfying the gate; that pinned source-specific state is now also validated
  against later unrelated submit completions; it now also validates two
  concrete source-submit dependency lanes on one dependent submit through the
  new `dependency2_*` state, and it now also validates a third lane through
  `dependency3_*`, with submit B staying blocked until all three source
  submits release their lane-specific completion-ticket thresholds; the same
  three-lane path is now also validated through indexed `depend_slot`,
  `depticket_slot`, and `depstate_slot` commands instead of depending only on
  bespoke per-lane verbs; that same indexed path now also validates slot `4`
  against a fifth submit, with submit B staying gated until all four concrete
  source submits release their slot-specific completion-ticket thresholds; the
  same submit path now also has hardware-validated typed slot `attach` /
  `detach`, typed `kick`, typed `ccb`, typed `queue`, typed `status`, typed
  `sync`, typed `owner`, typed indexed dependency-slot setup, and typed
  dependency-slot clear through the submit-op ioctl surface; the same
  multi-submit scenario is now also validated with object setup on the
  object-op ioctl path instead of direct sysfs writes

## Highest Priority

- [ ] Start kernel-side SGX render groundwork in `gma500`
  - Focus files: `psb_drv.c`, `psb_device.c`, `psb_irq.c`, `gem.c`, `mmu.c`
  - Goal: expose enough controlled kernel-side structure to study render
    submission, memory layout, reset, and fault handling
  - Current baseline to build on:
    - `prepare`
    - `arm`
    - `emit`
    - `launch`
    - `issue`

- [ ] Decide whether remaining SGX lab control and inspection should remain
  sysfs-based
  - The bounded query-side ioctl surface is now validated on hardware
  - The bounded range-query ioctl surface now covers typed live `gtt_range`
    lookup by index, explicit ID, or GPU VA, including GEM-only filtered
    enumeration, typed in-range offset reporting, and direct SGX object
    role/class/gpu_use/context readout when the range is adopted
  - The same bounded range-query path now also covers direct shared-object
    submit inspection on one GPU VA, including:
    - total submit-reference count
    - first matching submit ID and slot
    - explicit submit-match ordinal selection through `gpuva-submit`
  - The bounded SGX-state query ioctl surface now covers typed live SGX
    snapshot readout with either cached-read or fresh-capture mode
  - The bounded GTT-state query ioctl surface now covers typed global GTT/MMU
    layout, MMU counters, and live range totals
  - The bounded object-query ioctl surface now covers typed live SGX lab object
    lookup by explicit object ID or by live object index
  - The bounded submit-query ioctl surface now also covers typed live SGX lab
    submit lookup by explicit submit ID, by focused submit, by live submit
    index, or directly by GPU VA with resolved slot, object, backing range,
    object semantic metadata, matching-submit count, and in-range offset
    reporting
  - The same bounded typed query helpers now also cover a no-sysfs coord plus
    submit/object/state dump flow for live SGX lab inspection
  - The standalone `psb_sgx_lab_dump` tool now also covers one live typed
    snapshot without shell-side composition in both:
    - human-readable text form
    - structured JSON form
  - The focused cleanup validator now also proves stale-submit clearing,
    focused-submit clearing, object cleanup, and PF-unbind cleanup without
    reading the old SGX or GTT sysfs text nodes
  - The focused object-op validator now also proves adopt, metadata mutation,
    PF bind/unbind, object survival after userspace close, and final drop
    without reading the old object or GTT sysfs text nodes
  - The focused submit-op validator now also proves five-object setup, typed
    attach, typed kick/queue/block programming, typed progression, submit
    drop, and final object cleanup without reading the old SGX or GTT sysfs
    text nodes
  - The higher-level multi-submit runner now also proves the full four-lane
    dependency-slot-clear scenario without reading the old SGX or GTT sysfs
    text nodes when `USE_IOCTL_VERIFICATION=1` is paired with the typed
    object, submit, dependency, and range setup paths
  - The bounded object-side ioctl surface now covers `adopt`, `role`, `class`,
    `gpu_use`, `context`, `pf_bind`, `pf_unbind`, `drop`, and `cleanup`
  - The bounded submit-side ioctl surface now covers `create`, `focus`,
    `focus-clear`, `cleanup`, `attach`, `detach`, `kick`, `ccb`, `queue`,
    `status`, `sync`, `owner`, `prepare`, `arm`, `emit`, `launch`,
    `validate`, `issue`, `transition`, and `drop`
  - The same bounded submit-side path now also has a hardware-validated
    materialization stage through `prepare`, so focused and multi-submit typed
    runners can prove one coherent backing control/status image before the
    first progression step
  - The same bounded submit-side path now also has a hardware-validated
    launched issue-plan stage through `launch`, so focused and multi-submit
    typed runners can prove one exact issue-side snapshot before `issue`
  - The same bounded submit-side path now also covers `depend-slot`,
    `depticket-slot`, `depstate-slot`, and `depclear-slot`, and the
    four-lane dependency-slot-clear scenario is now validated with
    `RANGE_DISCOVERY=ioctl`, `OBJECT_SETUP=ioctl`, `SUBMIT_SETUP=ioctl`, and
    `DEPENDENCY_SETUP=ioctl`; the same scenario is now also validated with
    `VERIFICATION_MODE=ioctl`
  - Decide whether the remaining sysfs dependence should stay in:
    - legacy compatibility fallback paths
    - older direct sysfs-oriented bring-up helpers kept only as explicit
      `legacy` paths for comparison
    - later dependency/state-block expansion
  - Keep the interface clearly lab-only until real SGX execution evidence
    justifies broader uAPI claims

- [ ] Decide whether the validated global coordination model is enough
  - Decide whether the current per-submit `queue_epoch` plus driver-global
    issue, running, and completion tickets remains sufficient now that
    multiple persistent submits are validated at once
  - Decide whether later sync, fence, dependency, or queue-ownership state
    needs its own additional generation words or more than the current
    issue/running/complete plus source-submit dependency and dependency-block
    model

- [ ] Use the new SGX snapshot path to answer the next kernel questions
  - Compare snapshots around future MMU, reset, and submission experiments
  - Identify which SGX/BIF registers actually move once work exceeds the current
    2D envelope
  - Keep state-capture scope practical; avoid dumping large register ranges that
    do not change or do not inform the render path

- [ ] Turn the working SGX lab object into the first SGX-aware render object
  - Add explicit object roles such as general, PDS, USE, or other SGX-specific
    categories only when hardware evidence justifies them
  - Decide which mappings remain implicit and which become explicit per context
  - Keep stolen-fb display paths separate from future render-object semantics

- [ ] Mine the highest-value historical SGX source trees
  - Prioritize the Intel/EMGD/OpenEmbedded `pvr/services4` trees captured in
    `docs/14-source-triage.md`
  - Use TI SGX trees as secondary architectural reference
  - Extract concepts and structure, not wholesale code drops

- [ ] Define the first render-uAPI shape
  - Start with a lab-only internal interface or debug ioctl design note
  - Do not pretend to finalize a public ABI before hardware evidence exists

## Near-Term Kernel Tasks

- [ ] Turn the GEM/MMU audit plus SGX lab object path into one concrete object model
  proposal
  - Use `docs/13-gem-mmu-audit.md`, `docs/15-pf-pd-bind-lab.md`,
    `docs/16-sgx-lab-objects.md`, and `docs/17-sgx-lab-submit-path.md` as the
    baseline
- [ ] Decide whether the next lab interface should be a debug ioctl, a debugfs
  hook, or an extension of the current sysfs controls
- [ ] Decide whether the SGX lab object control should remain sysfs-based or
  move to a debug ioctl / debugfs interface before broader submission
  experiments
- [ ] Define the first SGX object metadata that matters for submission
  - next candidate fields: control-block ownership, sync/status backing type,
    or broader SGX state-block classes beyond the current packet/code/surface split
- [ ] Define the first SGX submit metadata that matters for submission
  - Build the next step on top of the now-validated `prepare` image, not on
    pre-materialization incremental setup alone
  - next candidate fields: queue ownership semantics beyond the current
    tail/head split, or broader synchronization/dependency semantics beyond
    the current fixed CCB, queue, and status words
  - next dependency candidates: more than one wait ticket, more than one
    dependency source/context, or a broader fence block beyond the current
    `ta_dependency` plus `dependency_submit_id` plus live-updated dependency
    state blocks; the next immediate follow-up is whether the new indexed
    four-slot surface should stop at four slots or move to a broader
    dependency array or fence block
- [ ] Extend the first CCB/control-block model
  - decide which header or queue semantics beyond the current
    `write_offset/read_offset/base/size/packet_size/packet_count/tail/head`
    model should become explicit next
  - decide which additional SGX-shaped header semantics beyond the current
    validated 8-dword non-executing record should become explicit before any
    real submit attempt
  - keep non-TA command types as metadata until there is hardware evidence for
    broader validation rules
- [ ] Define the first sync and status backing semantics that matter for
  submission beyond the current status object
  - decide whether later sync or fence state belongs in the existing `status`
    object, a second state object, or a later dedicated queue/state class
  - decide whether TA and 3D status ownership should remain packed into one
    object or split further once hardware evidence justifies it
  - decide how many repeated issue-side and completion-side cycles should
    compose, and whether the current global issue/running/complete lanes plus
    four source-submit dependency lanes with lane-specific completion
    thresholds are enough before any real SGX execution attempt
- [ ] Map the reset and interrupt paths needed for eventual SGX job submission
- [ ] Keep the current broad-risk IRQ mask changes out of the live target until
  each bit is justified by hardware evidence

## Userspace And Validation Tasks

- [ ] Keep the 32-bit probe set building in Docker and runnable on the VAIO
- [ ] Add a probe only if it answers a new kernel or render question
- [ ] Use the new GPU-VA range-query submit-ref path in the next submission
  experiments
  - Prefer one GPU VA over broad text dumps when the real question is "which
    adopted object and which live submit slot currently owns this address?"
  - Prefer the new `gpuva-submit` selector when the real question is "which of
    several live submits currently owns match `n` on this shared object VA?"
  - Keep that path read-only; do not turn it into a broader render control ABI
- [ ] Decide whether a more SGX-specific holder or trace workload is needed for
  future object-role and submit experiments
- [ ] Decide whether a target-side helper should evolve from dumb GEM holders to
  a more SGX-shaped object mix for future submit experiments
- [ ] Audit the remaining userspace lab tools for `drmIoctl()` retry semantics
  - `psb_sgx_lab_submit_op.c` now uses raw `ioctl()` because dependency-gated
    `-EAGAIN` is a real lab result, not a transient retry condition
  - `psb_sgx_lab_object_op.c` now also uses raw `ioctl()` so the object-side
    lab path is not coupled to libdrm retry behavior either
  - `psb_sgx_lab_query.c` now also uses raw `ioctl()` so the typed
    submit/coord query path stays aligned with the current lab-only contract
  - `psb_sgx_lab_range_query.c` now also uses raw `ioctl()` so the typed live
    range-query path is not coupled to libdrm retry behavior either
  - `psb_sgx_lab_sgx_state_query.c` already uses raw `ioctl()` for the typed
    SGX snapshot query path
  - `psb_sgx_lab_gtt_state_query.c` uses raw `ioctl()` for the typed global
    GTT/MMU state query path
  - keep checking future helpers and any richer query-side error semantics
    before defaulting back to libdrm wrappers
- [ ] Keep using the guarded deploy and health-probe flow for rebooted VAIO runs
  - Prefer `scripts/deploy-vaio-gma500-guarded.sh` when reboot is part of the
    step
  - Treat `scripts/check-vaio-target-health.sh` as the canonical target-state
    probe before and after remote validation
  - Prefer `scripts/run-vaio-rollback-autorestore-session.sh` for any future
    deliberate rollback-path regression test
- [ ] Keep the typed-versus-legacy compare harnesses current as wrapper
  defaults change
  - focused object:
    - `scripts/run-vaio-sgx-object-mode-compare.sh`
  - focused submit:
    - `scripts/run-vaio-sgx-submit-mode-compare.sh`
  - multi-submit:
    - `scripts/run-vaio-sgx-multi-submit-mode-compare.sh`
  - dependency-slot-clear wrapper:
    - `scripts/run-vaio-sgx-dependency-slot-clear-mode-compare.sh`
  - indexed dependency-array wrapper:
    - `scripts/run-vaio-sgx-indexed-dependency-array-mode-compare.sh`
  - multi-source ticket wrapper:
    - `scripts/run-vaio-sgx-multi-source-ticket-dependency-mode-compare.sh`
  - when wrapper dispatch changes, prefer saving one compare bundle instead of
    proving only the typed side or only the legacy side in isolation
  - when warm-target compare branches hand off from typed to legacy, keep the
    typed reset helper in that path:
    - `scripts/run-vaio-sgx-lab-reset.sh`
    - compare bundles should keep `pre-reset.log` and `inter-reset.log`
    - if a legacy compare path regresses, check those reset logs before
      assuming the scenario logic itself changed
    - the focused legacy submit path now also depends on the typed range-query
      helper for warm-target fresh-range discovery, so if it regresses again,
      check the typed range-query output before trusting the old sysfs GTT dump
  - keep the compare-session metadata stable too:
    - `final_driver_srcversion`
    - `final_override_module_size`
    - if the remote state helper output changes, keep
      `scripts/summarize-vaio-sessions.sh` able to resolve module identity for
      both new and old compare bundles
  - the generic multi-submit wrapper family is now also revalidated through
    the same reset-assisted path; the next compare-side follow-up is only to
    rerun a given wrapper family again if its dispatch or warm-target setup
    changes in the future
- [ ] Avoid further Xorg-side debugging that assumes the render path already
  exists; current evidence says it does not

## Immediate Next Steps

- [ ] Use the now-clean exact single-submit `plan` ladder as the baseline for
  the next kernel-side SGX change
  - the fixed module now clears terminal `complete`
  - the shared typed submit image now also collects `live + plan + result`
    execution records once per pass on override
    `7E90BF7EA4740AF68A6B588`
  - the next useful work is the first more execution-oriented SGX kernel
    structure above that collected view model, not another no-code rerun of
    the same ladder

- [ ] If a no-code regression retest of the fixed module is needed, keep the
  terminal exact rerun as the canonical one:
  - `PLAN_SESSION_MODE=recovery PLAN_PHASE_PROFILE=complete ./scripts/run-vaio-sgx-plan-session.sh`

- [ ] Keep the new panic-capture path enabled for future panic-prone SGX work
  - after collector-side changes, rerun:
    - `./scripts/run-vaio-panic-capture-setup-session.sh`
  - when validating the panic path itself, use:
    - `./scripts/run-vaio-panic-test-session.sh`
  - remember that the canonical Ubuntu crash layout on this target is:
    - `crashdump/<timestamp>/dump.<timestamp>`
    - `crashdump/<timestamp>/dmesg.<timestamp>`

- [ ] Keep the target explicitly headless during current SGX work
  - `lightdm_active=inactive`
  - do not assume Xorg is needed for `/dev/dri/card0` bring-up
  - do not treat display-manager autostart as part of panic recovery

- [ ] Only use the full-cycle deploy parent if the next SGX change actually
  changes the module image
  - `./scripts/run-vaio-sgx-plan-guarded-deploy-session.sh`

- [ ] Keep using the hardened parent/session wrappers and session summary
  while investigating crashes
  - interrupted runs now leave fallback `session-result.env`
  - older incomplete bundles now summarize as
    `interrupted_or_incomplete`

## Documentation Tasks

- [ ] Update `docs/10-ubuntu-18.04-target.md` whenever a target-side validation
  result changes
- [ ] Append every substantial result to `docs/11-progress-log.md`
- [ ] Keep this file current after each meaningful implementation or hardware
  test step

## Deferred Work

- [ ] Mesa Gallium scaffolding
  - Blocked on kernel-side render-path understanding

- [ ] Xorg acceleration bring-up through `glamor`
  - Blocked on a usable hardware GL/EGL stack

- [ ] Packaging or installer work
  - Not useful until the render path is real

## Explicit Non-Goals Right Now

- [ ] Do not spend time on Wayland tuning
- [ ] Do not chase Vulkan
- [ ] Do not treat software-rendered Xorg fixes as progress toward SGX
  acceleration
