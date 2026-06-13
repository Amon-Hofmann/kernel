# i386 Kernel Tutorial — Master Plan

## Goals

Build a working i386 kernel from scratch in pure C + AT&T assembly (GAS, `.s` files).
Each session is completable in ~4 hours. Early sessions are quicker and focus on
establishing fundamentals; mid and end sessions of each phase go deeper and build
directly on earlier work. Sessions must be comprehensive — no hand-waving over
"just trust this works."

**Assembly flavour:** AT&T syntax / GAS (`.s` files). Advantages over NASM:
- GAS ships with binutils — no extra tool to install.
- Same syntax as GCC inline asm (`asm volatile(...)`), so it stays consistent
  throughout the tutorial.
- Tighter GCC toolchain integration (CRT objects, linker scripts).
Downside: operand order (src, dst) is the opposite of Intel syntax, which trips
people up initially — we will address this explicitly in the first asm session.

---

## Toolchain status

- `i686-elf-gcc`: **not found** on this system (was at `/home/amon/build-i686-elf/`
  in the old project, that path no longer exists).
- Must be rebuilt or an alternative sourced before session 1 can compile anything.
- Options: build from source (the canonical route), use a pre-built package
  (`i686-elf-gcc` from AUR, Nixpkgs, or crosstool-ng), or use a Docker image.
- **Action item for Session 0:** resolve toolchain first, document the chosen path.

---

## Reference material

- `i386.pdf` in `/home/aj/Projekte/C/Kerneldev/` — the Intel i386 Programmer's
  Reference Manual. Pull specific sections on demand:
  - GDT / segments → Ch. 3
  - Paging → Ch. 5
  - IDT / exceptions / interrupts → Ch. 5–6
  - I/O ports → Ch. 8
- OSDev wiki (https://wiki.osdev.org) — cross-reference as needed.
- Existing os-dev project at `~/Projekte/Kern/os-dev` — reference only;
  tutorial starts from scratch.

---

## Session structure (every session)

Each session has three mandatory parts:

1. **Build** — implement exactly one concept; the kernel must produce a new,
   visible, testable artifact by the end (different output, new behavior in QEMU).
2. **Concept check** — 2–3 questions about the *why*, not the *what*.
   A correct answer proves understanding, not just copy-paste.
3. **Mutation exercise** — one deliberate small change that only works if the
   concept was truly understood (e.g., intentionally break a GDT entry and
   observe the triple fault; then fix it).

---

## Phase 0 — Toolchain & Environment (Session 0)

**Goal:** working cross-compiler, assembler, linker, emulator, debugger.
No kernel code yet — just prove the chain works end to end.

| Step | Task |
|------|------|
| 0.1 | Decide on cross-compiler delivery (build from source vs. package) |
| 0.2 | Install/build `i686-elf-gcc`, `i686-elf-as`, `i686-elf-ld` |
| 0.3 | Install QEMU (`qemu-system-i386`), GDB, GRUB (`grub-mkrescue`) |
| 0.4 | Write a trivial "does it assemble and link?" smoke test |
| 0.5 | Boot a minimal binary in QEMU, confirm no crash (blank screen is fine) |

**Why a cross-compiler?** The host GCC targets the host OS (Linux, ELF with glibc
assumptions). We need a bare-metal i686 target with no OS assumptions — that is
what `i686-elf-gcc` provides.

---

## Phase 1 — First Boot (Sessions 1–4)

### Session 1.1 — GAS Assembly Primer (~1h, interlude)

**What this is:** A reference session — no new kernel code. Covers the GAS
directives and notation used in `boot.s` that are not obvious without a formal
assembly background. Read this after the Session 1 build works, before moving
to Session 2.

**Topics covered:**
- What an assembler does (mnemonics → machine code + relocation records)
- Directives vs. instructions
- `.set` — named constants at assemble time
- `.section` — switching between ELF sections
- `.align` / `.balign` — alignment padding and why it matters
- `.long`, `.word`, `.byte` — emitting raw values into the binary
- `.skip` — reserving N bytes (BSS idiom)
- `.global` — exporting a symbol so the linker can see it
- `.type sym, @function` / `.type sym, @object` — ELF symbol type metadata
- `.size sym, . - sym` — the location counter `.`, what `. - sym` computes, and
  why size annotations matter for debuggers and `objdump`
- Local labels (`1:`, `2:`) and the `1b` / `1f` back/forward reference syntax
- Reading your binary: `objdump -d`, `objdump -t`, `readelf -S`

---

### Session 1 — Multiboot & the Boot Handoff (~3h)

**What you build:** A kernel that boots via GRUB/Multiboot and halts cleanly.
No output yet — just proof the machine reaches our code.

**Topics covered:**
- What BIOS actually does before GRUB runs
- What Multiboot is and why we use it (alternative: writing our own bootloader —
  briefly discussed so the student knows what they are skipping)
- The Multiboot header: magic number, flags, checksum — what each field does and
  why the values are what they are
- Writing `boot.s` from scratch, line by line:
  - `.section .multiboot` and why alignment matters
  - `.section .bss` for the stack — `stack_bottom:`, `.skip 16384`, `stack_top:`
  - `.section .text`, `_start:`, `mov $stack_top, %esp`
  - AT&T syntax orientation (operand order, register prefixes, sigils)
  - `call kernel_main`, `cli`, `hlt`, the `1: jmp 1b` loop and why it's there
  - `.size _start, . - _start` — what this does and why
- The linker script: why we need one, what `.text`, `.rodata`, `.data`, `.bss`
  mean, why the kernel must be linked at 1MB
- The Makefile: cross-compile flags, `-ffreestanding`, `-nostdlib`, CRT objects
  (`crti.o`, `crtbegin.o`, `crtend.o`, `crtn.o`) and their role
- Booting the result in QEMU, attaching GDB, stepping through `_start`

**Concept check:**
1. Why does the stack grow downward, and why do we point `esp` at `stack_top`
   rather than `stack_bottom`?
2. What would happen if the Multiboot checksum were wrong?
3. Why is `-ffreestanding` required, and what does it change about GCC's behavior?

**Mutation exercise:** Change the stack size to 4 bytes. Boot in QEMU and observe
the crash. Restore it. Explain what went wrong.

---

### Session 2 — VGA Text Mode (~3h)

**What you build:** `terminal_initialize()`, `terminal_writestring()` — the kernel
prints "Hello, Kernel!" to the screen.

**Topics covered:**
- Memory-mapped I/O: why writing to 0xB8000 makes text appear
- VGA text mode cell format: bits [15:8] = attribute byte, [7:0] = character
- The attribute byte: foreground color (bits [3:0]), background (bits [6:4]),
  blink (bit 7)
- Implementing a terminal: cursor tracking (row, column), color state
- `terminal_putchar`, wrapping at 80 columns, scrolling at 25 rows
- Why `strlen` must be implemented by us (no stdlib in freestanding)
- Writing and linking `vgaterm.c` and `string.c`

**Concept check:**
1. Why is 0xB8000 a valid address to write to without any setup — doesn't paging
   need to be configured first?
2. What does the VGA hardware do with the data we write to 0xB8000?
3. Why do we need our own `strlen` — couldn't the compiler provide it?

**Mutation exercise:** Change the background color of the entire terminal to red
by modifying only the attribute byte logic. Then add a function that clears only
a single row.

---

### Session 3 — GDT: Segments & Privilege Rings (~4h)

**What you build:** A GDT with null, kernel code, kernel data, user code, user
data segments. Load it with `lgdt`. Kernel still runs as before but now on a
properly configured GDT instead of the one GRUB left behind.

**Topics covered:**
- What segmentation is: base, limit, access byte, granularity
- Why protected mode uses a descriptor table instead of raw base+limit registers
- Segment descriptor format (8 bytes) dissected bit by bit — reference i386 manual Ch. 3
- The null descriptor and why it must be first
- Privilege levels (rings 0–3) — what they mean for memory and instructions
- The segment selector: index, TI bit, RPL field
- Implementing `gdt.c`: the descriptor struct, `gdt_flush` in asm (`lgdt`, far jump)
- The far jump after `lgdt` and why it is necessary (flushing the segment
  descriptor cache)
- `cs`, `ds`, `ss`, `es`, `fs`, `gs` — which get set when and how

**Concept check:**
1. Why do we need a far jump after loading the GDT — what would go wrong without it?
2. What is the purpose of the null descriptor?
3. What is the difference between the segment selector's RPL and the CPL?

**Mutation exercise:** Add a descriptor with a limit of 0 (covering no memory).
Try to use it and observe the fault. Explain which exception fires and why.

---

### Session 3.1 — Serial Output: UART & Port I/O (~1.5h)

**What you build:** A minimal serial driver for COM1. `serial_init` configures the
UART, `serial_putchar` writes one character, `serial_writestring` writes a string.
Output appears in the QEMU terminal via `-serial stdio`. Use it to replace the
VGA stress loop with a serial hello.

**Topics covered:**
- Port I/O vs MMIO: `in`/`out` instructions vs memory-mapped registers
- The 16550 UART: COM1 base address (0x3F8), register map
- Initialisation sequence: baud rate divisor (DLAB), line control, FIFO, modem control
- Polling the THRE bit before each write
- Why serial is faster than VGA under KVM (no MMIO VM exits)

**Concept check:**
1. What is the difference between port I/O and MMIO? Why does VGA use one and
   UART use the other?
2. What does polling THRE before each write prevent?

**No mutation exercise** — serial output is a utility session. Verify with `make run`
that text appears in the terminal.

---

### Session 4 — IDT & CPU Exceptions (~4h)

**What you build:** An IDT with handlers for all 32 CPU exception vectors.
A divide-by-zero or page fault now prints `EXCEPTION #0: Divide By Zero` instead
of silently triple-faulting.

**Topics covered:**
- What an interrupt is vs. an exception vs. a fault vs. a trap vs. an abort
- The IDT: structure mirrors GDT but entries are gate descriptors
- Gate types: interrupt gate vs. trap gate (IF flag behavior difference)
- The interrupt frame pushed by the CPU onto the stack — what's in it and in what order
- Error codes: which exceptions push them, why, what they mean
- Writing ISR stubs in asm: why we need assembly trampolines, the common handler
  pattern, the `iret` instruction
- Implementing `idt.c`: `idt_set_gate`, `idt_flush` (using `lidt`)
- A generic `exception_handler(struct interrupt_frame *)` in C
- Printing the register dump on exception (sets up for debugging everything that follows)

**Concept check:**
1. Why do some ISR stubs push a dummy error code onto the stack before jumping to
   the common handler?
2. What is the difference between an interrupt gate and a trap gate in terms of
   what happens to IF?
3. Why does `iret` rather than `ret` return from an interrupt handler?

**Mutation exercise:** Intentionally trigger a divide-by-zero from `kernel_main`.
Confirm the handler fires and prints. Then change the gate type of exception 0
from interrupt gate to trap gate and observe whether IF behavior changes.

---

## Phase 2 — Hardware I/O (Sessions 5–8)

### Session 5 — PIC & IRQ Routing (~3h)

**What you build:** PIC remapping so hardware IRQs don't collide with CPU
exceptions. A spurious IRQ handler. The kernel no longer crashes on timer ticks.

**Topics covered:**
- The 8259A PIC: master/slave cascade, IRQ0–IRQ15
- Why the default IRQ mapping (0x08–0x0F / 0x70–0x77) collides with CPU exceptions
  in protected mode
- Remapping to 0x20–0x2F via ICW1–ICW4 initialization sequence
- The EOI (End of Interrupt) command — what happens if you forget it
- `inb` / `outb` port I/O in asm vs. inline asm
- Masking/unmasking IRQ lines

**Concept check / mutation:** mask all IRQs, confirm timer stops; unmask only
IRQ0 and confirm the timer fires.

---

### Session 6 — PIT & Timer Interrupts (~3.5h)

**What you build:** A global tick counter incremented on every IRQ0. A
`ksleep(ms)` busy-wait. The kernel prints "tick N" visibly.

**Topics covered:**
- The 8253/8254 PIT: channels, modes, divisor calculation from 1.193182 MHz base
- Programming channel 0 for periodic mode
- Writing the IRQ0 handler in C (called via the IDT stub from session 4)
- Volatile and why the tick counter must be `volatile`
- Busy-wait vs. proper sleep (foreshadow scheduling)

---

### Session 7 — PS/2 Keyboard (~4h)

**What you build:** A keyboard driver. Keypresses print the corresponding
character to the terminal.

**Topics covered:**
- PS/2 controller (0x60/0x64 ports), status register, data register
- Scancodes: Set 1, make vs. break codes
- A scancode-to-ASCII translation table
- Shift, Caps Lock state
- Interrupt-driven input (IRQ1) vs. polling

---

### Session 8 — Serial / UART Debug Output (~3h)

**What you build:** `kprintf` output mirrored to COM1. QEMU shows it in the
terminal so you can debug without a graphical window.

**Topics covered:**
- 16550 UART registers, baud rate divisor
- QEMU `-serial stdio` flag
- Why serial is invaluable for debugging the rest of the tutorial
- Optionally: a minimal `kprintf` with `%s`, `%d`, `%x`, `%c`

---

## Phase 3 — Memory Management (Sessions 9–12)

### Session 9 — Physical Memory Manager (~4h)

**What you build:** A bitmap allocator. `pmm_alloc_frame()` returns a free 4 KiB
page frame; `pmm_free_frame()` returns it.

**Topics covered:**
- The Multiboot memory map (type 1 = usable, type 2 = reserved, etc.)
- Page frames: why 4 KiB is the unit
- Bitmap allocator: one bit per frame, `uint32_t` array arithmetic
- What memory regions to mark reserved at startup (kernel image, VGA, BIOS)
- Testing: allocate 100 frames, free 50, reallocate — verify addresses

---

### Session 10 — Paging (~4h)

**What you build:** Paging enabled. The kernel runs with a page directory and
page tables. Identity-mapped for now.

**Topics covered:**
- Page directory / page table structure (10/10/12 bit split for i386)
- CR3, CR0 bit 31 (PE already set, now PG)
- Page directory entry format, page table entry format — each field explained
- Identity mapping the first 4 MiB
- Enabling paging in asm
- Page fault (#PF) error code dissection (P, W/R, U/S bits) — i386 manual Ch. 5
- Writing a #PF handler that prints the faulting address from CR2

---

### Session 11 — Kernel Heap (~4h)

**What you build:** `kmalloc(size)` / `kfree(ptr)`. Dynamic allocation works
inside the kernel.

**Topics covered:**
- Virtual vs. physical addresses — why we need a mapping layer
- `kvmalloc`: mapping physical frames into virtual address space
- A slab or simple linked-list allocator over the virtual heap region
- Alignment requirements
- Testing: allocate structs, free them, check for leaks

---

### Session 12 — Higher-Half Kernel (~4h, optional / advanced)

**What you build:** The kernel linked at 0xC0100000 but loaded at 0x00100000
by GRUB. Identity + higher-half mapping coexist temporarily; identity map is
unmapped after entry.

**Topics covered:**
- Why higher-half is considered best practice (leaves low memory for user space)
- Two-stage mapping in `boot.s` using a pre-paging identity map trick
- Adjusting the linker script for the higher-half split
- The temporary stack and page directory before paging, swapped after

---

## Phase 4 — Multitasking (Sessions 13–15)

### Session 13 — Processes & Context Switching (~4h)

**What you build:** Two tasks that alternate, each printing to a different
column. Context switch is timer-driven.

**Topics covered:**
- What a process is: an execution context = registers + stack + address space
- `struct task`: pid, esp, page directory, state
- Saving/restoring all registers (`pusha`/`popa`, segment registers, eflags)
- The context switch function in asm — why it cannot be pure C
- Stack layout during a switch

---

### Session 14 — Scheduler (~4h)

**What you build:** A round-robin scheduler. `task_create(fn)` adds a runnable
task; the scheduler preempts on each timer tick.

**Topics covered:**
- Run queue (circular linked list or array)
- Preemption point: inside the timer IRQ handler
- `task_yield()` for voluntary preemption
- Idle task
- Foreshadow: priority, sleep queue

---

### Session 15 — System Calls (~4h)

**What you build:** `int 0x80` dispatch table. A user-mode task (ring 3) calls
`sys_write` and `sys_exit`.

**Topics covered:**
- Ring 3 vs. ring 0: what changes (CS/SS selectors, stack switch via TSS)
- The TSS: what it is, what fields matter for a basic setup
- `int 0x80` gate as a system gate (DPL=3)
- The syscall dispatch table in C
- Returning to user mode the first time (`iret` with user-mode frame)

---

## Phase 5 — Storage (Sessions 16–18)

### Session 16 — ATA/IDE Driver (~4h)

**What you build:** Read a sector from a virtual disk image in QEMU using PIO mode.

**Topics covered:**
- ATA bus registers (0x1F0–0x1F7), status bits (BSY, DRQ, ERR)
- 28-bit LBA addressing
- The `insl` instruction for bulk port reads
- Setting up a disk image in QEMU (`-hda disk.img`)

---

### Session 17 — VFS Layer (~4h)

**What you build:** A virtual filesystem abstraction: `vfs_open`, `vfs_read`,
`vfs_write`, `vfs_close` backed by a driver table.

**Topics covered:**
- Why a VFS: multiple backing filesystems, uniform interface
- `struct vfs_node`, `struct vfs_ops` (vtable pattern in C)
- Mounting a filesystem at a path
- The devfs shortcut: `/dev/null`, `/dev/zero` as trivial implementations

---

### Session 18 — Simple Filesystem (~4h)

**What you build:** A minimal filesystem (FAT16 or a custom flat format) readable
by the kernel and writable by a host-side tool.

**Topics covered:**
- On-disk layout: superblock, FAT / allocation bitmap, data region
- Directory entries
- Mounting and reading a file
- Generating a test image on the host with a Python script

---

## Build progression (what the kernel can do after each phase)

| After phase | Capability |
|-------------|------------|
| 0 | Cross-compiler works, QEMU boots a blank binary |
| 1 | Boots, prints text, handles CPU exceptions gracefully |
| 2 | Responds to keyboard, keeps time, debuggable via serial |
| 3 | Dynamic memory allocation, paging, optional higher-half |
| 4 | Multiple tasks, preemptive scheduling, user/kernel boundary |
| 5 | Reads/writes files on a virtual disk |

---

## Open decisions / action items

- [ ] Resolve toolchain: build `i686-elf-gcc` from source, find a package, or use
      a container. Decide before Session 0.
- [ ] Confirm QEMU version available: `qemu-system-i386 --version`
- [ ] Confirm `grub-mkrescue` available (package: `grub` or `grub2`)
- [ ] Decide on Session 12 (higher-half): include or defer?
- [ ] Decide on Phase 5 filesystem format: FAT16 (real-world, complex) or custom
      flat format (simpler, better for teaching)?
