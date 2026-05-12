#!/usr/bin/env python3
"""Append human-readable register names to simple MMIO logs."""

from __future__ import annotations

import argparse
import csv
import re
import sys
from pathlib import Path


HEX_RE = re.compile(r"0x[0-9a-fA-F]+")
ADDR_EQ_RE = re.compile(r"\baddr\s*=\s*(0x[0-9a-fA-F]+)", re.IGNORECASE)


def load_registers(path: Path) -> dict[int, dict[str, str]]:
    regs: dict[int, dict[str, str]] = {}
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            regs[int(row["address"], 16)] = row
    return regs


def annotate_line(line: str, regs: dict[int, dict[str, str]]) -> str:
    token: str | None = None
    match = ADDR_EQ_RE.search(line)
    if match:
        token = match.group(1)
    else:
        matches = HEX_RE.findall(line)
        if matches:
            token = matches[0]

    if token is not None:
        value = int(token, 16)
        reg = regs.get(value)
        if reg:
            note = f"  # {reg['name']} [{reg['block']}]"
            if reg["notes"]:
                note += f" - {reg['notes']}"
            return line.rstrip("\n") + note + "\n"
    return line


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Annotate MMIO logs with register names from data/psb_registers.csv."
    )
    parser.add_argument("input", type=Path, help="Path to the raw MMIO log")
    parser.add_argument(
        "--registers",
        type=Path,
        default=Path("data/psb_registers.csv"),
        help="CSV file containing register metadata",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="Optional output path; defaults to stdout",
    )
    args = parser.parse_args()

    regs = load_registers(args.registers)
    lines = [annotate_line(line, regs) for line in args.input.read_text(encoding="utf-8").splitlines(True)]

    if args.output:
        args.output.write_text("".join(lines), encoding="utf-8")
    else:
        sys.stdout.write("".join(lines))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
