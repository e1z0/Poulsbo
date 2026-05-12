# First Hardware Session

This is the shortest useful plan for the first real Poulsbo lab session.

## Goal

Leave the session with one reproducible Linux baseline and one structured directory for proprietary-stack RE captures.

## Linux Baseline

On the Linux target:

```bash
cd /path/to/Poulsbo
./scripts/collect_target_state.sh
./scripts/make_trace_dir.sh linux-baseline
```

Archive at least:

- the generated target capture directory
- `dmesg` after a cold boot
- connector behavior with the monitor actually attached
- any `drm_info`, `glxinfo -B`, and `eglinfo` output the machine can provide

## Reference Stack Setup

Prepare a separate offline image for the proprietary stack.

Requirements:

- snapshot-capable environment
- one tiny GLES2 workload binary
- a way to copy logs and screenshots off the machine cleanly

Do not use the machine as a daily desktop. Keep it as a lab image.

## First Workloads

Use the tiny workloads in [workloads/](/root/Poulsbo/workloads) or an equivalent vendor-compatible build.

Recommended order:

1. clear-only
2. solid triangle
3. textured quad

Stop after each stage and archive traces before moving on.

## Trace Folder Discipline

For every session:

```bash
./scripts/make_trace_dir.sh clear-color-pass
```

Then place artifacts into:

- `raw/` for untouched dumps
- `annotated/` for decoded logs
- `screenshots/` for visible output
- `notes/` for the narrative summary
- `artifacts/` for binaries, build logs, and support files

Use [notes/trace-session-template.md](/root/Poulsbo/notes/trace-session-template.md) for session notes.

## Minimum Data To Collect

For the first clear-color workload, try to obtain:

- exact workload source or binary hash
- screenshot of the expected frame
- one MMIO or register access trace
- one kernel or system log around execution
- notes about hangs, resets, and repeatability

## Immediate Follow-Up

Back in this repo:

1. add the raw trace under a new `traces/` session directory
2. run `python3 scripts/annotate_mmio.py ...`
3. diff runs with `python3 scripts/compare_mmio_logs.py ...`
4. promote newly discovered registers with `python3 scripts/update_register_db.py ...`

That is enough to start writing the first kernel instrumentation patch set.
