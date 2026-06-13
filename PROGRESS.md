# Kernel Tutorial — Progress

## How to use this file
- Mark sessions `[x]` when complete (all three parts: build, concept check, mutation exercise).
- Git tag format: `session-NN-complete` (e.g. `git tag session-00-complete`)
- Notes field: anything worth remembering — tricky parts, detours, time taken.

---

## Phase 0 — Toolchain & Environment

| # | Session | Status | Tag | Notes |
|---|---------|--------|-----|-------|
| 00 | Toolchain: build i686-elf-gcc, install QEMU | [x] | session-00-complete | i686-elf-gcc 14.2.0 + binutils 2.42 built from source to ~/cross/opt; qemu-system-i386 8.2.2; smoke test boots and halts cleanly |

---

## Phase 1 — First Boot

| # | Session | Status | Tag | Notes |
|---|---------|--------|-----|-------|
| 1.1 | GAS assembly primer (interlude) | [x] | session-01.1-complete | Reference session; objdump/readelf inspection; no build artifact |
| 01 | Multiboot & the boot handoff | [x] | session-01-complete | boot.s + kernel_main; stack overflow = memory corruption (no hw boundary); Multiboot checksum verified as magic+flags+checksum==0 mod 2^32; -ffreestanding implies -fno-builtin |
| 02 | VGA text mode | [x] | session-02-complete | volatile uint16_t* VGA buffer; attribute byte = bg<<4|fg; scroll via memmove+direct uint16_t clear; memcpy/memmove/memset in string.c; -fno-tree-loop-distribute-patterns insight |
| 03 | GDT: segments & privilege rings | [x] | session-03-complete |
| 3.1 | Serial output: UART & port I/O | [x] | session-03.1-complete | 16550 UART on COM1 (0x3F8); polled THRE; port I/O via inline asm outb/inb; -nostdinc added to CFLAGS; KVM enforces segment limits, TCG does not |
| 04 | IDT & CPU exceptions | [ ] | | |

---

## Phase 2 — Hardware I/O

| # | Session | Status | Tag | Notes |
|---|---------|--------|-----|-------|
| 05 | PIC & IRQ routing | [ ] | | |
| 06 | PIT & timer interrupts | [ ] | | |
| 07 | PS/2 keyboard | [ ] | | |
| 08 | Serial / UART debug output | [ ] | | |

---

## Phase 3 — Memory Management

| # | Session | Status | Tag | Notes |
|---|---------|--------|-----|-------|
| 09 | Physical memory manager | [ ] | | |
| 10 | Paging | [ ] | | |
| 11 | Kernel heap (kmalloc/kfree) | [ ] | | |
| 12 | Higher-half kernel (optional) | [ ] | | |

---

## Phase 4 — Multitasking

| # | Session | Status | Tag | Notes |
|---|---------|--------|-----|-------|
| 13 | Processes & context switching | [ ] | | |
| 14 | Scheduler | [ ] | | |
| 15 | System calls | [ ] | | |

---

## Phase 5 — Storage

| # | Session | Status | Tag | Notes |
|---|---------|--------|-----|-------|
| 16 | ATA/IDE driver | [ ] | | |
| 17 | VFS layer | [ ] | | |
| 18 | Simple filesystem | [ ] | | |
