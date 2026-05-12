# VAIO State Inspection Helper

This note records a small post-recovery helper added while the VAIO is still
blocked in `overall=ssh_port_down`.

## Why This Exists

Once the target comes back, the next few seconds of SSH access matter. We need
one command that shows both:

- which `gma500_gfx` module path/srcversion is active
- what state the rollback guard is in

That is more useful than re-running several ad hoc remote commands after every
recovery or guarded deploy.

## Added Script

- [scripts/show-vaio-gma500-state.sh](/root/Poulsbo/scripts/show-vaio-gma500-state.sh)

It reports:

- running kernel
- `gma500_gfx` modinfo path and `srcversion`
- whether the override module exists under
  `/lib/modules/<kernel>/updates/poulsbo/gma500_gfx.ko`
- whether `poulsbo-module-rollback.service` is enabled and active
- whether `/var/lib/poulsbo-rollback/pending.env` exists
- pending rollback target fields when that marker exists
- whether `/var/lib/poulsbo-rollback/gma500_gfx.previous.ko` exists

## Guarded Deploy Integration

[scripts/deploy-vaio-gma500-guarded.sh](/root/Poulsbo/scripts/deploy-vaio-gma500-guarded.sh)
now runs this helper automatically after post-boot health returns to
`overall=remote_usable`.

That means the successful deploy path now ends with one concrete remote-state
dump instead of only printing the health probe.

## Local Validation

Local checks passed:

- `bash -n scripts/show-vaio-gma500-state.sh`
- `bash -n scripts/deploy-vaio-gma500-guarded.sh`

The helper was not target-validated in this step because the VAIO still
remains at:

- `ping_status=ok`
- `tcp_22_status=closed`
- `ssh_pubkey_status=connection_refused`
- `overall=ssh_port_down`

## Intended Use

Manual use after recovery:

```bash
./scripts/show-vaio-gma500-state.sh
```

The next successful guarded deploy will also print it automatically.
