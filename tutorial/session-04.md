# Session 04 — IDT & CPU Exceptions

## Goal

Install an Interrupt Descriptor Table with handlers for all 32 CPU exception
vectors. When the CPU faults — divide by zero, invalid opcode, page fault —
your handler fires, prints the exception name and a register dump to serial,
and halts. No more silent triple faults.

By the end of this session a deliberate divide-by-zero in `kernel_main` should
produce readable output on the serial terminal instead of resetting the machine.

---

## Interrupts, exceptions, faults, traps, aborts

These terms have precise meanings on x86:

**Interrupt** — an asynchronous event from hardware (keyboard, timer, disk).
The CPU finishes the current instruction, then jumps to the handler. The
interrupted code is resumed afterwards.

**Exception** — a synchronous event caused by the currently executing
instruction. Subdivided into:

- **Fault** — the instruction that caused it can be restarted after the
  handler fixes the condition. Example: page fault (handler maps the page,
  returns, instruction reruns). The saved `%eip` points to the *faulting*
  instruction.
- **Trap** — the instruction has completed; the saved `%eip` points to the
  *next* instruction. Example: breakpoint (`int 3`), overflow (`into`).
- **Abort** — unrecoverable. The handler cannot return and the CPU state is
  undefined. Example: double fault, machine check.

For this session all 32 exception handlers will simply print and halt. The
fault/trap/abort distinction matters more once you implement paging and
signals.

---

## The IDT

The **Interrupt Descriptor Table** is an array of up to 256 gate descriptors.
Each entry describes how the CPU should handle one interrupt or exception
vector. The CPU locates it via the **IDTR** register, loaded with `lidt`
(analogous to `lgdt` for the GDT).

The first 32 vectors (0–31) are reserved by Intel for CPU exceptions. Vectors
32–255 are available for hardware IRQs and software interrupts.

### Gate descriptor format (8 bytes)

```
 63      48 47 46:45 44 43:40 39:32
 +---------+--+-----+--+-----+------+
 | offset  | P| DPL |0 |type |      |
 | [31:16] |  |     |  |     |  0   |
 +---------+--+-----+--+-----+------+

 31              16 15               0
 +----------------+-----------------+
 |   segment      |    offset       |
 |   selector     |    [15:0]       |
 +----------------+-----------------+
```

As a C struct:

```c
struct idt_entry {
    uint16_t offset_low;   /* handler address bits 15:0  */
    uint16_t selector;     /* code segment selector       */
    uint8_t  zero;         /* always 0                    */
    uint8_t  type_attr;    /* P, DPL, type                */
    uint16_t offset_high;  /* handler address bits 31:16  */
} __attribute__((packed));
```

The **selector** must be the kernel code segment selector (`0x0008`). When an
exception fires, the CPU uses this selector to load `%cs` before jumping to
the handler.

### The type_attr byte

| Bit | Name | Meaning |
|-----|------|---------|
| 7   | P    | Present — must be 1 for the gate to be valid |
| 6:5 | DPL  | Descriptor Privilege Level for software `int` |
| 4   | 0    | Always 0 for interrupt/trap gates |
| 3:0 | Type | Gate type (see below) |

**Gate types** relevant to us:

| Type field | Name           | Hex  |
|------------|----------------|------|
| `1110`     | Interrupt gate | `0xE`|
| `1111`     | Trap gate      | `0xF`|

For a present interrupt gate at ring 0: `type_attr = 0x8E` (P=1, DPL=0, type=0xE).

### Interrupt gate vs. trap gate

The only difference is what happens to the **IF (Interrupt Flag)** in `%eflags`:

- **Interrupt gate** — CPU clears IF on entry. Hardware interrupts are
  disabled for the duration of the handler. Prevents nested interrupts.
- **Trap gate** — CPU leaves IF unchanged. Hardware interrupts can occur
  during the handler.

For exception handlers: use interrupt gates. You do not want further
interrupts while handling a fault.

---

## What the CPU does on an exception

When an exception fires, the CPU (in ring 0):

1. Pushes `%eflags`, `%cs`, `%eip` onto the current stack — the **interrupt
   frame**
2. For exceptions that have an error code: pushes the **error code** on top
3. Looks up the gate descriptor for the vector in the IDT
4. Loads `%cs` with the gate's selector, jumps to the gate's offset

The stack at handler entry (with error code) looks like:

```
 esp+12  eflags
 esp+8   cs
 esp+4   eip         ← instruction that faulted (or next, for traps)
 esp+0   error_code
```

Without an error code, `eip` is at `esp+0` on entry.

`iret` restores `%eip`, `%cs`, and `%eflags` from the stack in one
instruction — the inverse of what the CPU pushed. A plain `ret` would only
pop `%eip` and leave `%cs` and `%eflags` corrupt.

### Which exceptions push an error code?

| Vector | Name                        | Error code? |
|--------|-----------------------------|-------------|
| 0      | Divide Error                | No          |
| 1      | Debug                       | No          |
| 2      | NMI                         | No          |
| 3      | Breakpoint                  | No          |
| 4      | Overflow                    | No          |
| 5      | Bound Range Exceeded        | No          |
| 6      | Invalid Opcode              | No          |
| 7      | Device Not Available        | No          |
| 8      | Double Fault                | Yes (always 0) |
| 9      | Coprocessor Segment Overrun | No          |
| 10     | Invalid TSS                 | Yes         |
| 11     | Segment Not Present         | Yes         |
| 12     | Stack-Segment Fault         | Yes         |
| 13     | General Protection Fault    | Yes         |
| 14     | Page Fault                  | Yes         |
| 15     | Reserved                    | No          |
| 16     | x87 FPU Error               | No          |
| 17     | Alignment Check             | Yes         |
| 18     | Machine Check               | No          |
| 19–31  | Reserved                    | No          |

---

## ISR stubs and the common handler pattern

Each IDT entry points to a unique stub function. You cannot point all 32
entries at the same C function because:

1. Some exceptions push an error code, others do not — the stack layout
   differs. You must normalise it before calling common C code.
2. You need to know which vector fired. The CPU does not pass it as an
   argument.

The solution: write 32 small assembly stubs, one per vector. Each stub:
- If the exception does **not** push an error code: push a dummy value (e.g.
  `0`) so the stack layout is uniform
- Push the vector number
- Jump to a shared `isr_common` routine

`isr_common` then:
- Saves all general-purpose registers (`pusha`)
- Saves the data segment register
- Loads the kernel data segment into `%ds`, `%es`, `%fs`, `%gs`
- Calls your C exception handler
- Restores everything
- Executes `iret`

This pattern keeps the assembly minimal and the logic in C.

---

## What you need to implement

### New files

**`include/idt.h`** — declare:
- `struct idt_entry` (8 bytes, packed)
- `struct idt_ptr` (6 bytes, packed): `uint16_t limit`, `uint32_t base`
- `void idt_install(void)`

**`src/idt.c`** — implement:
- A static array of 256 `struct idt_entry`
- A static `struct idt_ptr`
- `idt_set_gate(uint8_t vector, uint32_t handler, uint16_t selector, uint8_t type_attr)`
- `idt_install`: calls `idt_set_gate` for all 32 exception vectors, then
  calls `idt_flush(&idt_ptr)`

**`as/isr_stubs.s`** — implement 32 ISR stubs following the pattern above,
plus the `isr_common` routine. The common routine must save state, call the C
handler, restore state, and `iret`.

Define a macro for the stub body to avoid 32 copies of near-identical code:

```asm
.macro ISR_NOERR n
.global isr\n
isr\n:
    push $0        # dummy error code
    push $\n       # vector number
    jmp isr_common
.endm

.macro ISR_ERR n
.global isr\n
isr\n:
    push $\n       # CPU already pushed error code
    jmp isr_common
.endm
```

**`include/isr.h`** — declare:

```c
struct interrupt_frame {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pusha order */
    uint32_t vector, error_code;
    uint32_t eip, cs, eflags;
};

void exception_handler(struct interrupt_frame *frame);
```

**`src/isr.c`** — implement `exception_handler`:
- Print the exception name (use a lookup table of the 32 names) to serial
- Print `eip`, `cs`, `eflags`, `error_code` from the frame
- Halt with `cli` + `hlt` in an infinite loop (do not return from an
  exception in this session)

### Modify existing files

**`src/kernel.c`** — call `idt_install()` after `gdt_install()`.

**`as/isr_stubs.s`** — declare `isr_common` as calling `exception_handler`
via a C-callable interface. Remember to pass a pointer to the frame, not the
frame itself.

### Build and test

```
make clean && make iso && make run
```

The kernel should boot normally and print the serial hello. No exceptions
should fire during normal boot.

To test the handler, temporarily add to `kernel_main` after `idt_install`:

```c
__asm__ volatile("div %0" : : "r"((uint32_t)0));
```

The serial terminal should print something like:

```
EXCEPTION: Divide Error (#0)
EIP=0x001001xx CS=0x0008 EFLAGS=0x00000246 ERR=0x00000000
```

Then the kernel halts. Remove the test line before committing.

Reference: i386 Programmer's Reference Manual, Chapter 9 (Exceptions and
Interrupts).

---

## Concept check

Answer these before running `/review`:

1. Why do some ISR stubs push a dummy error code before pushing the vector
   number, while others do not? What breaks if you skip the dummy push for a
   no-error-code exception?

2. What is the difference between an interrupt gate and a trap gate in terms
   of what happens to the IF flag, and which should you use for exception
   handlers?

3. Why does returning from an interrupt handler require `iret` rather than
   `ret`?

---

## Mutation exercise

Do this after the divide-by-zero test passes.

**Part A:** Trigger the divide-by-zero intentionally from `kernel_main` and
confirm the handler fires and prints the exception name and register state.
Document what you observe — specifically what `eip` points to and what the
error code is.

**Part B:** Change the gate type for exception 0 from interrupt gate (`0x8E`)
to trap gate (`0x8F`) and trigger the divide-by-zero again. The exception
should still fire. Check whether `eflags` differs between the two runs.
Explain what the IF bit change means in practice.

Restore the gate type to interrupt gate when done.

When you are satisfied with the whole session, run `/review`.
