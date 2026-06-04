# Session 01 — Multiboot & the Boot Handoff

## Goal

A kernel that boots via GRUB/Multiboot and halts cleanly.
No output yet — just proof that the machine reaches *your* code.

By the end of this session `make run` should launch QEMU, GRUB should load your
kernel, and `_start` should execute. You will step through it in GDB and see
each instruction fire.

---

## What happens before your code runs

Understanding the full chain is not optional — you will need to debug it later.

### 1. Power-on

The CPU starts in **real mode**: 16-bit, 1 MiB address space, no memory
protection. The instruction pointer is set to `0xFFFF0` — the last 16 bytes of
the first megabyte — where the BIOS ROM lives.

### 2. BIOS

The BIOS runs POST (Power-On Self Test), initialises hardware (PIC, PIT, memory
controller), builds tables (interrupt vector table at address 0, BIOS data area
at 0x400), then searches for a bootable device. When it finds one it loads the
first sector (512 bytes) into address `0x7C00` and jumps there.

### 3. GRUB (first-stage → second-stage → kernel loader)

GRUB is a multi-stage bootloader. Its first stage fits in the 512-byte MBR.
That stage knows just enough to find and load GRUB's second stage from disk.
The second stage does the heavy lifting:

- Switches the CPU from real mode to **protected mode** (32-bit, full address
  space, segmentation active)
- Parses the kernel ELF binary
- Checks for a valid **Multiboot header** inside the first 8 KiB of the kernel
- Sets up CPU registers with information your kernel will need (memory map,
  framebuffer info, etc.)
- Jumps to `_start`

When GRUB jumps to `_start`, the machine is in this state:
- 32-bit protected mode
- A20 line enabled (full address space accessible)
- `%eax` = `0x2BADB002` (Multiboot magic — proof GRUB loaded you)
- `%ebx` = physical address of the **Multiboot information structure**
- Interrupts **disabled**
- No valid stack yet — you must set one up immediately
- GDT: GRUB loaded one, but you cannot rely on its layout
- Paging: **off**

---

## The Multiboot specification

Multiboot is a contract between bootloader and kernel. Any bootloader that
implements the spec can boot any kernel that implements the spec.

Your kernel signals compliance by embedding a **Multiboot header** within its
first 8 KiB. The header is exactly 12 bytes:

| Offset | Field    | Value                        | Why                                      |
|--------|----------|------------------------------|------------------------------------------|
| 0      | magic    | `0x1BADB002`                 | Fixed identifier the bootloader scans for|
| 4      | flags    | `0` (for now)                | Feature flags (modules, memory map, etc.)|
| 8      | checksum | `-(magic + flags)` mod 2^32  | Detects corruption; must sum to zero     |

The checksum constraint: `magic + flags + checksum == 0 (mod 2^32)`. If the sum
is wrong, GRUB rejects the kernel with "not a multiboot kernel."

**The header must be 4-byte aligned** and appear in the first 8 KiB. This is
why it goes in its own `.multiboot` section and the linker script places that
section first.

---

## AT&T assembly syntax orientation

GAS (GNU Assembler) uses AT&T syntax. If you have seen Intel syntax before, the
main differences are:

| Concept            | AT&T syntax          | Intel syntax       |
|--------------------|----------------------|--------------------|
| Operand order      | `src, dst`           | `dst, src`         |
| Register prefix    | `%eax`               | `eax`              |
| Immediate prefix   | `$42`                | `42`               |
| Memory access      | `(%eax)`             | `[eax]`            |
| Size suffix        | `movl`, `movw`, `movb` | `mov dword`, ...  |

The operand order is the most common stumbling block. `mov $stack_top, %esp`
means "move the value `stack_top` into `%esp`" — destination is on the right.

---

## The stack

The i386 stack grows **downward**: each push decrements `%esp` by 4, then
writes the value. So the "top" of the stack in memory is the *lowest* address
currently in use.

You allocate stack space as a block of bytes in `.bss`. You point `%esp` at the
*end* of that block (highest address), not the start. The stack then grows
toward the start.

Minimum safe size for early kernel work: 16 KiB. Smaller stacks will cause
silent corruption as soon as you make a few nested function calls.

`.bss` is the right section because:
- It is zero-initialised (by convention; the loader zeroes it)
- It takes no space in the binary (just a size annotation)
- The linker script reserves the virtual address range

---

## The linker script

Your compiler and assembler produce relocatable object files — addresses are
not yet fixed. The linker combines them and assigns final addresses according to
the linker script.

Key concepts you need for `cfg/kernel.ld`:

**`ENTRY(_start)`** — tells the linker which symbol is the program entry point.
This is embedded in the ELF header; GRUB reads it to know where to jump.

**`. = 1M`** — set the location counter to 1 MiB (0x100000). This is where
GRUB loads your kernel. Addresses below 1 MiB are occupied:
- 0x00000–0x004FF: real-mode interrupt vector table + BIOS data area
- 0x07C00–0x07DFF: bootloader (MBR)
- 0x0A000–0x0BFFF: VGA memory (we'll use this in Session 2)
- 0x0F000–0x0FFFF: BIOS ROM shadow

**Standard sections:**
- `.text` — executable code
- `.rodata` — read-only data (string literals, const tables)
- `.data` — initialised read/write data
- `.bss` — zero-initialised data (stack goes here)

**`BLOCK(4K) : ALIGN(4K)`** — align each section to a 4 KiB page boundary.
Not strictly required now, but essential once paging is enabled (Session 10).

---

## Compile flags for a freestanding kernel

| Flag | What it does | Why you need it |
|------|-------------|-----------------|
| `-ffreestanding` | Disables hosted-environment assumptions | Without it GCC assumes `main()` exists, a C library is present, and standard startup code runs. None of that is true. |
| `-nostdlib` | Do not link the standard C library | libc assumes a kernel/OS beneath it. We are the OS. |
| `-lgcc` | Link GCC's internal runtime library | Provides helper functions the compiler emits for things like 64-bit arithmetic on 32-bit hardware (`__udivdi3`, etc.). Unlike libc, this is genuinely freestanding. |
| `-O2` | Optimise | Not strictly required, but `-ffreestanding` without optimisation can produce code that depends on stack frames in ways that break early boot. |

---

## What you need to implement

### Files to create / modify

**`as/boot.s`** — already exists from the smoke test, but you should understand
and own every line. Rewrite it from scratch (or edit in place) so that it has:

1. The three Multiboot header constants defined with `.set`
2. A `.multiboot` section containing the three header words, 4-byte aligned
3. A `.bss` section with a 16 KiB stack, labelled `stack_bottom:` and
   `stack_top:`
4. A `.text` section with `_start:` that:
   - Sets `%esp` to `stack_top`
   - Calls `kernel_main`
   - Disables interrupts (`cli`) after the call returns (it shouldn't, but be
     safe)
   - Halts (`hlt`)
   - Loops forever with `1: jmp 1b` in case the CPU resumes from halt
   - Has correct `.global _start`, `.type _start, @function`, and
     `.size _start, . - _start` directives

**`src/kernel.c`** — create this file. It should contain:

```
void kernel_main(void) {
    /* Session 2 will add output here */
    for (;;) {}
}
```

That's all. One function, infinite loop. The kernel just needs to not crash.

**`cfg/kernel.ld`** — already exists. Review it and make sure you understand
every line. No changes needed yet.

**`makefile`** — already updated. No changes needed.

**`src/main.c`** — this file is a leftover from before. Delete it or replace
its contents with something that won't conflict. The entry point is `_start` in
assembly, not a C `main()`.

### Build and test

```
make clean && make all
```
Should produce `out/kernel.elf` with no warnings.

```
file out/kernel.elf
```
Should say: `ELF 32-bit LSB executable, Intel 80386`

```
make iso && make run
```
QEMU should launch. You will see the GRUB menu briefly, then a blank screen
(no output yet). QEMU should stay running — not reboot, not exit.

### Step through in GDB

Open two terminals.

Terminal 1 — launch QEMU paused, waiting for debugger:
```
qemu-system-i386 -cdrom out/kernel.iso -serial stdio -no-reboot -s -S
```
`-s` opens a GDB server on port 1234. `-S` pauses the CPU at startup.

Terminal 2 — attach GDB:
```
gdb out/kernel.elf
(gdb) target remote :1234
(gdb) break _start
(gdb) continue
(gdb) stepi       # step one instruction at a time
```

Watch `%esp` change when you hit the `mov $stack_top, %esp` instruction.
Watch the call to `kernel_main` happen. Understand what each `stepi` does.

---

## Concept check

Answer these *before* running `/review`. Write the answers in your head or on
paper — the point is that you know, not that you typed them.

1. The stack grows downward. You have a 16 KiB block starting at address X.
   You point `%esp` at `X + 16384`. A function pushes five 4-byte values onto
   the stack. What address does `%esp` now hold? What would happen if your
   stack were only 4 bytes?

2. If the Multiboot checksum field were `0` instead of `-(magic + flags)`,
   what would GRUB do, and why?

3. What does `-ffreestanding` change about how GCC behaves? Name at least two
   concrete effects.

---

## Mutation exercise

Do this *after* the kernel boots successfully and you have stepped through it
in GDB.

**Change the stack size to 8 bytes** (`.skip 8` instead of `.skip 16384`).
Rebuild and boot. Observe what happens. Then restore it to 16384 and explain:

- What went wrong?
- At what point did things fail, and how did you observe it?
- Why does 8 bytes not work even though `_start` itself doesn't push anything?

When you think you are done with the whole session, run `/review`.
