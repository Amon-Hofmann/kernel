# Session 05 — PIC & IRQ Routing

## Goal

Remap the 8259A Programmable Interrupt Controller so hardware IRQs land on
vectors 32–47 instead of colliding with your CPU exception vectors. Install a
generic IRQ handler, acknowledge interrupts with EOI, and control which lines
are allowed to fire via the mask register.

By the end of this session you can mask all hardware interrupts, confirm
nothing fires, then unmask IRQ0 and watch ticks arrive on serial — without
having written a single line of PIT code. The default BIOS/SeaBIOS PIT
configuration already ticks at ~18.2 Hz; you are just choosing whether to let
it through.

---

## The 8259A PIC

The IBM PC and its successors use two cascaded 8259A Programmable Interrupt
Controllers to multiplex 15 hardware interrupt lines onto the CPU's single
`INTR` pin.

### Master/slave cascade

- **Master PIC** — command port `0x20`, data port `0x21`. Handles IRQ0–IRQ7.
- **Slave PIC** — command port `0xA0`, data port `0xA1`. Handles IRQ8–IRQ15.

The slave's output is wired into the master's **IRQ2** input. IRQ2 itself is
never used by a device — it exists purely to cascade the slave's signal into
the master. This means any IRQ8–15 event physically interrupts the master via
line 2, and the master then interrupts the CPU.

| IRQ | Typical device          |
|-----|--------------------------|
| 0   | System timer (PIT)       |
| 1   | Keyboard                 |
| 2   | — (cascade to slave) —   |
| 3   | Serial port (COM2/COM4)  |
| 4   | Serial port (COM1/COM3)  |
| 5   | Sound card / LPT2        |
| 6   | Floppy disk controller   |
| 7   | Parallel port (LPT1)     |
| 8   | Real-time clock (RTC)    |
| 9–11| Available                |
| 12  | PS/2 mouse                |
| 13  | FPU / coprocessor         |
| 14  | Primary ATA               |
| 15  | Secondary ATA              |

### Why the default mapping collides with CPU exceptions

On power-up, the PICs are configured (by firmware convention, not hardware
default) to deliver IRQ0–7 as interrupt vectors `0x08`–`0x0F` and IRQ8–15 as
`0x70`–`0x77`. Vectors `0x08`–`0x0F` (8–15) are squarely inside the range
Intel reserves for CPU exceptions (0–31) — exactly the vectors you wired up
in Session 4. A timer tick at vector 8 would dispatch to your **Double
Fault** handler.

You must reprogram both PICs to use a vector range outside 0–31 before it is
safe to enable interrupts. The conventional choice is `0x20`–`0x2F` (32–47):
master gets 32–39 (IRQ0–7), slave gets 40–47 (IRQ8–15).

### Remapping: the ICW sequence

Reprogramming a PIC means sending it four **Initialization Command Words**,
in order, alternating between its command port and data port:

| Step | Port    | Value (typical) | Meaning |
|------|---------|------------------|---------|
| ICW1 | command | `0x11`           | Begin init; expect ICW4; cascade mode |
| ICW2 | data    | vector offset    | `0x20` for master, `0x28` for slave |
| ICW3 | data    | cascade wiring   | Master: `0x04` (slave on IRQ2). Slave: `0x02` (its identity on the master's line 2) |
| ICW4 | data    | `0x01`           | 8086/88 mode |

Both PICs must receive their four words before either is usable again. After
this sequence, IRQ0 generates vector `0x20`, IRQ1 generates `0x21`, ... IRQ15
generates `0x2F`.

### Masking IRQ lines (OCW1)

Each PIC has an 8-bit **Interrupt Mask Register (IMR)**, written via its data
port at any time after initialization (this is "OCW1", though it is just a
normal byte write). A `1` bit masks (disables) that line; `0` unmasks it.

| PIC    | Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
|--------|-------|-------|-------|-------|-------|-------|-------|-------|
| Master | IRQ7  | IRQ6  | IRQ5  | IRQ4  | IRQ3  | IRQ2  | IRQ1  | IRQ0  |
| Slave  | IRQ15 | IRQ14 | IRQ13 | IRQ12 | IRQ11 | IRQ10 | IRQ9  | IRQ8  |

To mask or unmask a single line you must read the current IMR, flip one bit,
and write it back — writing a fresh byte would clobber every other line's
state.

### End of Interrupt (EOI)

After your handler has serviced an IRQ, you must tell the PIC so by writing
`0x20` (the EOI command) to its **command port**. Until you do, the PIC
believes that IRQ — and, depending on priority, every lower-priority IRQ on
that controller — is still being serviced and will not deliver another
interrupt for it.

If the IRQ came from the slave (IRQ8–15), you must send EOI to **both** the
slave and the master, since the event physically passed through both
controllers via the cascade.

Forgetting EOI does not crash anything immediately — it just looks like that
device stopped generating interrupts.

---

## IRQ stubs and the shared handler

This mirrors the ISR stub pattern from Session 4, but simpler: no IRQ pushes
an error code, and there is no fault/trap distinction to worry about — these
are always asynchronous interrupt gates.

```asm
.macro IRQ_STUB vector, irq
.global irq\irq
irq\irq:
    push $\irq      # which IRQ line (0-15), not the raw vector
    push $\vector   # the IDT vector (32-47)
    jmp irq_common
.endm
```

`irq_common` saves state the same way `isr_common` does, calls a C dispatcher
with a pointer to the frame, sends EOI for the right controller(s), restores
state, and `iret`s.

You need the **IRQ line number** (0–15) inside the handler to know which bit
to EOI and which device fired — the IDT vector alone (32–47) is recoverable
from it by a fixed offset, but keeping both on the frame avoids recomputing
it in C.

---

## What you need to implement

### New files

**`include/pic.h`** — declare:
```c
void pic_remap(void);
void pic_send_eoi(uint8_t irq);
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);
```

**`src/pic.c`** — implement the above:
- Port constants for master/slave command and data ports
- `pic_remap`: the full ICW1–ICW4 sequence for both controllers
- `pic_send_eoi`: write `0x20` to the master's command port; if `irq >= 8`,
  also write it to the slave's command port
- `pic_set_mask` / `pic_clear_mask`: read the correct PIC's IMR, set or clear
  the bit for `irq % 8`, write it back

### Modify existing files

**`as/isr_stubs.s`** — add the `IRQ_STUB` macro, invoke it for IRQ 0–15
(vectors 32–47), and add `irq_common` (parallel to `isr_common`).

**`include/isr.h`** — extend `struct interrupt_frame` if needed, or add a
parallel struct for IRQs; declare `void irq_handler(struct interrupt_frame
*frame);`

**`src/isr.c`** — implement `irq_handler`:
- Print which IRQ fired to serial
- Call `pic_send_eoi` with the IRQ number
- Return normally (do **not** halt — IRQs are routine, not faults)

**`src/idt.c`** — in `idt_install`, add gates for vectors 32–47 pointing at
`irq0`...`irq15`, using interrupt gates as before.

**`src/kernel.c`** — after `idt_install()`:
1. Call `pic_remap()`
2. Mask all 16 IRQ lines
3. Unmask only IRQ0
4. `sti`

### Build and test

```
make clean && make iso && make run
```

With only IRQ0 unmasked, you should see periodic "IRQ0 fired" (or similar)
lines on serial — the default PIT tick, already running at roughly 18.2 Hz
before your kernel ever touched it. Mask IRQ0 too and confirm the ticks stop.

Reference: i386 Programmer's Reference Manual is silent on the 8259A (it is
a separate chip, not part of the CPU) — for register-level detail consult the
8259A datasheet directly.

---

## Concept check

Answer these before running `/review`:

1. Why must the PIC be remapped to vectors 32+ before you call `sti`, given
   what vectors 0–31 are already used for?

2. IRQ2 is wired to the master PIC like any other line, but no device uses
   it. Why does servicing an IRQ8–15 event require sending EOI to *both*
   PICs?

3. You unmask IRQ0 but forget to call `pic_send_eoi` in the handler. What do
   you observe, and why does the PIC behave that way?

---

## Mutation exercise

**Part A:** Mask all 16 IRQ lines after `pic_remap()`. Confirm on serial that
no interrupts fire, even though the PIT is still physically ticking.

**Part B:** Unmask only IRQ0 and confirm ticks resume. Then mask IRQ0 again
and unmask IRQ1 instead. Generate a keyboard interrupt (press a key in the
QEMU window) and confirm it is IRQ1 that fires, not IRQ0.

Restore IRQ0 as the only unmasked line when done.

When you are satisfied with the whole session, run `/review`.
