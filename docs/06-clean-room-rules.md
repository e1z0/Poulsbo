# Clean-Room Rules

## Purpose

If this project becomes useful, the fastest way to destroy it is to contaminate it with proprietary code or NDA-derived details.

## Allowed Inputs

- public kernel source
- public Mesa source
- public documentation
- self-produced hardware traces
- self-produced behavioral notes
- public patents and public manuals

## Disallowed Inputs

- proprietary driver source
- leaked source
- private SDK headers or manuals not intended for public release
- notes copied from confidential documents
- decompiled code pasted into the repository

## Team Separation Rule

If more than one person works on this:

- tracer role: captures behavior and writes neutral behavioral notes
- implementer role: writes code from neutral notes and public information

Do not mix raw proprietary material into implementation discussions.

## Acceptable Trace Notes

Good note:

- "writing `0x20000000` to `PSB_CR_PDS_EXEC_BASE` precedes successful clear jobs"

Bad note:

- "the vendor function `FooSubmitKick()` does X, Y, Z"

Behavior is acceptable. Proprietary expression is not.

## Repository Rule

Never commit:

- proprietary binaries unless their license explicitly allows redistribution
- copied disassembly derived from proprietary objects without review
- screenshots or logs that expose private information from a third-party system
