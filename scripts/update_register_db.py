#!/usr/bin/env python3
"""Generate or refresh the PSB register database from a psb_reg.h header."""

from __future__ import annotations

import argparse
import csv
import re
import sys
from pathlib import Path


DEFINE_RE = re.compile(r"^#define\s+(PSB_CR_[A-Z0-9_]+)\s+(0x[0-9A-Fa-f]+)\b")


def infer_block(name: str) -> str:
    if "BIF" in name:
        return "bif"
    if "_2D_" in name or name.endswith("_2D") or "TWOD" in name:
        return "2d"
    if "PDS" in name:
        return "pds"
    if "USE" in name:
        return "use"
    if "EVENT" in name or "KICK" in name:
        return "event"
    return "core"


def load_existing(path: Path) -> dict[tuple[str, str], dict[str, str]]:
    if not path.exists():
        return {}

    existing: dict[tuple[str, str], dict[str, str]] = {}
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            existing[(row["address"], row["name"])] = row
    return existing


def parse_header(path: Path) -> list[dict[str, str]]:
    registers: list[dict[str, str]] = []
    seen: set[tuple[str, str]] = set()

    for line in path.read_text(encoding="utf-8").splitlines():
        match = DEFINE_RE.match(line.strip())
        if not match:
            continue
        name, address = match.groups()
        key = (address.lower(), name)
        if key in seen:
            continue
        seen.add(key)
        registers.append(
            {
                "address": f"0x{int(address, 16):04X}",
                "name": name,
                "block": infer_block(name),
                "notes": "",
            }
        )
    return sorted(registers, key=lambda row: (int(row["address"], 16), row["name"]))


def merge(existing: dict[tuple[str, str], dict[str, str]], parsed: list[dict[str, str]]) -> list[dict[str, str]]:
    merged: list[dict[str, str]] = []
    for row in parsed:
        key = (row["address"], row["name"])
        prior = existing.get(key)
        if prior is not None:
            row["block"] = prior.get("block", row["block"]) or row["block"]
            row["notes"] = prior.get("notes", "")
        merged.append(row)
    return merged


def write_csv(path: Path, rows: list[dict[str, str]]) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=["address", "name", "block", "notes"])
        writer.writeheader()
        writer.writerows(rows)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Refresh data/psb_registers.csv from a psb_reg.h header."
    )
    parser.add_argument("header", type=Path, help="Path to psb_reg.h")
    parser.add_argument(
        "output",
        type=Path,
        nargs="?",
        default=Path("data/psb_registers.csv"),
        help="Output CSV path",
    )
    args = parser.parse_args()

    if not args.header.exists():
        print(f"header not found: {args.header}", file=sys.stderr)
        return 1

    existing = load_existing(args.output)
    rows = merge(existing, parse_header(args.header))
    write_csv(args.output, rows)
    print(f"Wrote {len(rows)} registers to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
