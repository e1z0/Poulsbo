#!/usr/bin/env python3
"""Compare two MMIO logs and summarize register access overlap."""

from __future__ import annotations

import argparse
import csv
import re
from collections import Counter
from pathlib import Path


HEX_RE = re.compile(r"0x[0-9a-fA-F]+")


def load_registers(path: Path) -> dict[int, dict[str, str]]:
    if not path.exists():
        return {}
    rows: dict[int, dict[str, str]] = {}
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            rows[int(row["address"], 16)] = row
    return rows


def extract_addresses(path: Path) -> list[int]:
    addresses: list[int] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        tokens = HEX_RE.findall(line)
        if not tokens:
            continue
        addresses.append(int(tokens[0], 16))
    return addresses


def label(addr: int, regs: dict[int, dict[str, str]]) -> str:
    row = regs.get(addr)
    if row is None:
        return f"0x{addr:08X}"
    return f"0x{addr:08X} {row['name']} [{row['block']}]"


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare two MMIO/register logs.")
    parser.add_argument("left", type=Path, help="First log")
    parser.add_argument("right", type=Path, help="Second log")
    parser.add_argument(
        "--registers",
        type=Path,
        default=Path("data/psb_registers.csv"),
        help="Optional register database",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=25,
        help="Maximum rows per section",
    )
    args = parser.parse_args()

    regs = load_registers(args.registers)
    left = extract_addresses(args.left)
    right = extract_addresses(args.right)
    left_counts = Counter(left)
    right_counts = Counter(right)

    left_only = sorted(set(left_counts) - set(right_counts))
    right_only = sorted(set(right_counts) - set(left_counts))
    common = sorted(set(left_counts) & set(right_counts))
    common_ranked = sorted(
        common,
        key=lambda addr: abs(left_counts[addr] - right_counts[addr]),
        reverse=True,
    )

    print(f"left:  {args.left} ({len(left)} accesses, {len(left_counts)} unique)")
    print(f"right: {args.right} ({len(right)} accesses, {len(right_counts)} unique)")
    print()

    print("common registers with largest count deltas:")
    for addr in common_ranked[: args.limit]:
        print(f"  {label(addr, regs)} | left={left_counts[addr]} right={right_counts[addr]}")
    print()

    print("left-only registers:")
    for addr in left_only[: args.limit]:
        print(f"  {label(addr, regs)} | left={left_counts[addr]}")
    print()

    print("right-only registers:")
    for addr in right_only[: args.limit]:
        print(f"  {label(addr, regs)} | right={right_counts[addr]}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
