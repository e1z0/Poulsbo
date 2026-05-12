# VAIO Kdump Panic Capture And Reboot Validation

## Goal

Set up the VAIO so risky SGX validation can survive a kernel panic, reboot
automatically, and leave one saved crash artifact bundle for offline
inspection.

## New Scripts

- `scripts/configure-vaio-panic-capture.sh`
  - installs `kdump-tools`, `kexec-tools`, and `makedumpfile`
  - persists `kernel.panic=10`, `kernel.panic_on_oops=1`, and `kernel.sysrq=1`
  - adds `crashkernel=128M panic=10 panic_on_oops=1`
  - installs `poulsbo-panic-collect.service`
- `scripts/fetch-vaio-panic-artifacts.sh`
  - fetches saved panic state plus the newest crash dump directory
- `scripts/run-vaio-panic-capture-setup-session.sh`
  - one-command provisioning and optional reboot wrapper
- `scripts/run-vaio-panic-test-session.sh`
  - intentional panic, reboot wait, and artifact fetch wrapper

## Setup Validation

Canonical setup bundle:

- [logs/vaio-panic/20260512-221904-panic-capture-setup](/root/Poulsbo/logs/vaio-panic/20260512-221904-panic-capture-setup)

That session proved:

- `packages_installed=1`
- `sysctl_changed=1`
- `grub_changed=1`
- `kdump_defaults_changed=1`
- `reboot_required=1`
- post-reboot `overall=remote_usable`
- post-reboot kernel command line contains:
  - `crashkernel=128M`
  - `panic=10`
  - `panic_on_oops=1`

Live post-setup state:

- `kernel_panic=10`
- `kernel_panic_on_oops=1`
- `kernel_sysrq=1`
- `kdump_service_enabled=enabled`
- `kdump_service_active=active`
- `kdump-config status -> current state   : ready to kdump`
- `/sys/kernel/kexec_crash_loaded = 1`

## End-To-End Panic Test

Canonical destructive validation bundle:

- [logs/vaio-panic-test/20260512-222708-panic-test](/root/Poulsbo/logs/vaio-panic-test/20260512-222708-panic-test)

That session intentionally triggered:

```bash
sysctl -w kernel.sysrq=1 >/dev/null
echo c > /proc/sysrq-trigger
```

Observed behavior:

- target dropped to `overall=ssh_port_down`
- target rebooted automatically
- target returned to `overall=remote_usable`
- the graphics override stayed loaded after reboot:
  - `driver_srcversion=6E95EDFFFB5384C8E58F25B`

## Important Ubuntu Kdump Detail

The first collector pass assumed a classic `vmcore` layout. That was wrong for
this target.

Ubuntu kdump here writes:

- `/var/crash/202605122227/dump.202605122227`
- `/var/crash/202605122227/dmesg.202605122227`

Actual saved sizes from the validated panic:

- `dump.202605122227 = 27153539` bytes
- `dmesg.202605122227 = 101171` bytes

The previous-boot dump-kernel log also confirms the kdump kernel command line
included:

- `systemd.unit=kdump-tools-dump.service`
- `elfcorehdr=392564K`

## Collector Widening

The collector, fetcher, and state view were widened to recognize both:

- classic `vmcore`
- Ubuntu `dump.<timestamp>`

Updated files:

- `scripts/configure-vaio-panic-capture.sh`
- `scripts/fetch-vaio-panic-artifacts.sh`
- `scripts/show-vaio-gma500-state.sh`

Refresh validation bundle:

- [logs/vaio-panic/20260512-223405-panic-capture-setup](/root/Poulsbo/logs/vaio-panic/20260512-223405-panic-capture-setup)

That rerun now resolves:

- `latest_dump_present=1`
- `latest_dump_kind=dumpfile`
- `latest_dump_path=/var/crash/202605122227/dump.202605122227`
- `latest_dump_dmesg_path=/var/crash/202605122227/dmesg.202605122227`

The original panic-test bundle was backfilled too:

- [logs/vaio-panic-test/20260512-222708-panic-test/panic-artifacts/crashdump/202605122227](/root/Poulsbo/logs/vaio-panic-test/20260512-222708-panic-test/panic-artifacts/crashdump/202605122227)

## Current Operator Commands

Refresh provisioning after panic-capture script changes:

```bash
./scripts/run-vaio-panic-capture-setup-session.sh
```

Run a deliberate end-to-end panic/reboot validation:

```bash
./scripts/run-vaio-panic-test-session.sh
```

Fetch the latest saved crash artifacts without another panic:

```bash
DEST_DIR=/tmp/vaio-panic ./scripts/fetch-vaio-panic-artifacts.sh
```
