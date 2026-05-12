# VAIO SSH Stall Recovery

This note records the current recovery procedure for the Sony VAIO VGN-P11Z
target when it reaches a state where the network is up but normal SSH command
execution does not complete.

## Observed Failure Mode

Observed on May 11, 2026 after deploying the sync-block module candidate:

- target replies to ICMP at `10.1.11.218`
- TCP port `22` accepts connections
- OpenSSH transport setup and key exchange complete
- both `ssh` and `scp` then stall before command or file-transfer execution
- forcing `PreferredAuthentications=none` still stalls

That means the target is not dead on the network, but it is not remotely usable
enough for continued validation.

The current classification helper is:

- `scripts/check-vaio-target-health.sh`

Latest observed output:

```text
target_host=10.1.11.218
target_user=root
timeout_secs=15
ping_status=ok
tcp_22_status=open
ssh_none_status=timeout_post_kex
ssh_none_kex=1
ssh_pubkey_status=timeout_post_kex
ssh_pubkey_kex=1
scp_status=timeout_post_kex
overall=ssh_stalled_post_kex
```

## Practical Reading

This is not a normal “host down” case.

Current best inference:

- the box is partially booted
- `sshd` is reachable
- userauth or session setup on the target is wedged badly enough that remote
  command execution is not viable

At that point, the right next step is console recovery, not more remote retry
loops.

## Current Recovered State

Later on May 11, 2026, the target recovered enough for normal remote use again.
The canonical good-state output from `scripts/check-vaio-target-health.sh` is:

```text
target_host=10.1.11.218
target_user=root
timeout_secs=15
ping_status=ok
tcp_22_status=open
ssh_none_status=auth_rejected
ssh_none_kex=1
ssh_pubkey_status=ok
ssh_pubkey_kex=1
scp_status=ok
overall=remote_usable
```

The health script was also corrected so `overall=remote_usable` reflects
successful SSH command execution rather than failing the target only because
ICMP is inconsistent.

The guarded reboot/deploy helper is now:

- `scripts/deploy-vaio-gma500-guarded.sh`

Use it when a step includes rebooting the VAIO and you want the post-reboot
health state checked automatically.

## Console Recovery

From the VAIO console:

1. Check whether the machine reached a usable multi-user state.
2. If the override module is suspect, remove it and regenerate module deps:

```bash
rm -f /lib/modules/5.4.0-150-generic/updates/poulsbo/gma500_gfx.ko
depmod -a 5.4.0-150-generic
```

3. Reboot:

```bash
reboot
```

4. After reboot, verify from the console:

```bash
modinfo -n gma500_gfx
systemctl status ssh
systemctl status lightdm
```

If remote access is still unavailable, inspect:

```bash
journalctl -b -p warning..alert
journalctl -b -u ssh
dmesg | tail -n 200
```

## Expected Good Recovery State

The target is considered recovered enough for continued work when:

- `ssh root@10.1.11.218 'echo ok'` completes normally
- `scp` completes normally
- `modinfo -n gma500_gfx` reports either the distro module or a deliberate
  override
- `lightdm` is active again if X-side checks are needed

## Next Action After Recovery

Once SSH works again:

1. Confirm whether the target is running the distro module or the override.
2. If needed, redeploy a known-good local build.
3. Resume:
   - `scripts/run-vaio-sgx-submit.sh`
   - first in `QUEUE_MODE=wrap`
   - then in `QUEUE_MODE=nowrap`
4. Re-check:
   - `last_validate_status`
   - `fault_count`
   - `bif_fault`
