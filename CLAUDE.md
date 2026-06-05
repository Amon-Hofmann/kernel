# Kernel Tutorial Project

## What this is

A from-scratch i386 kernel tutorial, built and documented session by session.
The goal is deep understanding — every line of assembly and every hardware detail
is explained. See `tutorial-plan.md` for the full session breakdown.

## Key files

| File | Purpose |
|------|---------|
| `tutorial-plan.md` | Master plan: all sessions, topics, concept checks, mutation exercises |
| `PROGRESS.md` | Session completion tracker — update this and create a git tag at the end of each session |
| `i386.pdf` | Intel i386 Programmer's Reference Manual — the authoritative hardware reference |
| `makefile` | Starting point; needs kernel-specific adaptation in Session 0 |

## Skills

- `/review` — assesses completed work; pass/fail only, no hints
- `/hint` — Socratic hints when stuck; one at a time, never gives the answer

## Conventions

- **Architecture:** i386 (32-bit x86)
- **Language:** pure C + AT&T assembly (GAS, `.s` files)
- **Cross-compiler:** `i686-elf-gcc` — not yet built; Session 0 covers this
- **Emulator:** QEMU (`qemu-system-i386`) — not yet installed
- **Bootloader:** GRUB / Multiboot (`grub-mkrescue` available)
- **Directory layout:** `src/` C sources, `as/` assembly, `include/` headers,
  `cfg/` linker script + clang-format, `lib/` objects (gitignored), `out/` output (gitignored)

## Progress tracking

- Mark sessions complete in `PROGRESS.md`
- Git tag each completed session: `git tag session-NN-complete`
- The code at each tag is the working kernel for that session

## Tutorial authoring rules

- Any new session or change to session scope must be reflected in **both**
  `tutorial-plan.md` (master plan) **and** the corresponding `tutorial/session-NN.md` file.
- These two sources must stay in sync — `tutorial-plan.md` is the outline,
  `tutorial/` contains the full student-facing content.
- Sub-sessions (e.g. `session-01.1`) get their own file in `tutorial/` and a
  matching entry in `tutorial-plan.md` under the parent session.

## Current status

Session 0 complete. Session 1 build checkpoint done; concept check and mutation
exercise pending. Session 1.1 (GAS assembly primer) added as a prerequisite interlude.
