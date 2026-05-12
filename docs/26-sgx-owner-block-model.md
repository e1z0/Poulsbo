# SGX Owner Block Model

This note records the first explicit queue-owner and completion-cookie block
added on top of the already validated sync block in the Ubuntu 5.4 Poulsbo lab
driver.

## Why This Step Exists

The earlier sync block made queue progress visible with:

- mirrored `ccb_read_offset`
- mirrored `ccb_write_offset`
- `sync_complete_seq`
- `sync_pending_seq`
- `sync_flags`

That was enough to validate explicit outstanding work counts, but it still had
no separate memory-backed notion of queue ownership or completion cookies.

This step adds a second validate-only block so the lab submit model can now
carry:

- who conceptually owns the queued work
- an issued-completion cookie pair independent of the sync sequence words

without pretending the driver has real SGX execution or fences yet.

## Current Kernel Surface

The existing submit control remains:

- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

The submit path now adds:

- a new submit command:
  - `owner <submit_id> <offset> <owner_state> <issue_cookie> <complete_cookie>`

The current owner-state names are:

- `queued`
- `running`
- `complete`

The owner block lives in the existing `status` object. The current layout is:

- dword `+0x0`: `owner_state`
- dword `+0x4`: `owner_issue_cookie`
- dword `+0x8`: `owner_complete_cookie`

## Current Validation Rules

The owner block is still validate-only. It does not execute SGX work.

If configured, validation now requires:

- a configured sync block
- a live `status` object
- a non-overlapping owner block region inside the `status` object
- no overlap with TA/3D status words
- `issue_cookie >= complete_cookie`
- `issue_cookie - complete_cookie == sync_pending_seq - sync_complete_seq`
- `complete` owner state only when the outstanding count is zero
- live readback of the owner state and both cookie words

The original owner-block-only helper validation used `running`, because that
milestone still described a static outstanding queue rather than an explicit
transition model.

## Helper Changes

The VAIO helper:

- `scripts/run-vaio-sgx-submit.sh`

now programs the owner block after `sync`.

Current static-owner defaults, still available with `TRANSITION_ENABLE=0`:

- `QUEUE_MODE=wrap`
  - `owner_offset=0x40`
  - `owner_state=running`
  - `owner_issue_cookie=0x102`
  - `owner_complete_cookie=0x100`
- `QUEUE_MODE=nowrap`
  - `owner_offset=0x40`
  - `owner_state=running`
  - `owner_issue_cookie=0x101`
  - `owner_complete_cookie=0x100`

Since the later transition milestone in `docs/27-sgx-transition-model.md`, the
same helper now defaults to `TRANSITION_ENABLE=1`, which starts from
`owner_state=queued` before it drives explicit `queued -> running -> complete`
progression.

The live submit view now also exposes:

- `owner_offset`
- `owner_state`
- `owner_issue_cookie`
- `owner_complete_cookie`
- `owner_config_count`
- `last_owner_jiffies`

## Current Status

Validated on May 11, 2026 on `10.1.11.218` with locally built module:

- `/lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko`
- `srcversion 29FE07980FD575A0704D90D`

Wrap-mode validation:

- `QUEUE_MODE=wrap`
- `owner_offset=0x40`
- `owner_state=running`
- `owner_issue_cookie=0x102`
- `owner_complete_cookie=0x100`
- `validate_count=2`
- `last_validate_status=0`

Target `dmesg` logged:

- `lab submit owner submit_id=1 offset=0x40 state=running issue_cookie=0x102 complete_cookie=0x100 outstanding=0x2`
- two successful `lab submit validate ... status=0`

Nowrap validation:

- `QUEUE_MODE=nowrap`
- `owner_offset=0x40`
- `owner_state=running`
- `owner_issue_cookie=0x101`
- `owner_complete_cookie=0x100`
- `validate_count=2`
- `last_validate_status=0`

Target `dmesg` logged:

- `lab submit owner submit_id=2 offset=0x40 state=running issue_cookie=0x101 complete_cookie=0x100 outstanding=0x1`
- two successful `lab submit validate ... status=0`

Post-run target state:

- `lightdm=active`
- `fault_count=0`
- `bif_fault=0x000000`
- `capture_failed=0`

## Next Step

The next useful step is not another fixed write-only state block. It is to turn
the current owner/cookie model into explicit transition semantics:

- `queued -> running -> complete`
- queue-owner handoff
- completion advancement

still without claiming real SGX execution until hardware evidence supports it.
