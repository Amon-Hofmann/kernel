# Session 06 — PIT & Timer Interrupts

## Goal

Program the 8253/8254 Programmable Interval Timer to fire IRQ0 at a
configurable frequency. Maintain a global tick counter incremented on every
interrupt. Implement a `ksleep_ms(uint32_t ms)` busy-wait built on top of that
counter. By the end of this session the kernel prints `tick N` to serial at a
steady rate you control.

---

## The 8253/8254 PIT

The Programmable Interval Timer is a chip with three independent countdown
channels. Channel 0 is wired to IRQ0 and is the standard source of periodic
timer interrupts on PC hardware.

### The input clock

The PIT is driven by a fixed 14.31818 MHz crystal divided by 12, giving an
input frequency of:

```
f_in = 14.31818 MHz / 12 = 1.193182 MHz
```

This is a hardware constant — your kernel cannot change it. What you can
change is the **divisor**: a 16-bit value you write to the PIT that determines
how many input clock cycles elapse between interrupts.

```
f_irq = f_in / divisor
```

| Divisor | Frequency | Period |
|---------|-----------|--------|
| 65536 (0) | ~18.2 Hz | ~54.9 ms |
| 1193    | ~1000 Hz  | ~1 ms  |
| 11932   | ~100 Hz   | ~10 ms |

A divisor of 0 is treated as 65536 by the hardware — it is the power-on
default and what BIOS/SeaBIOS leaves configured, which is why you have already
been seeing ~18 Hz ticks in session 5.

### Port map

| Port  | Direction | Function |
|-------|-----------|---------|
| `0x40` | write | Channel 0 data (divisor low byte then high byte) |
| `0x41` | write | Channel 1 data (unused) |
| `0x42` | write | Channel 2 data (PC speaker) |
| `0x43` | write | Mode/Command register |

### Programming channel 0

To configure the PIT write to the mode/command register (`0x43`) first, then
send the divisor to the data port (`0x40`) as two bytes: low byte first, then
high byte.

The mode/command byte for channel 0, lobyte/hibyte access, mode 2 (rate
generator) is `0x36`:

| Bits | Value | Meaning |
|------|-------|---------|
| 7:6  | `00`  | Select channel 0 |
| 5:4  | `11`  | Access mode: lobyte/hibyte |
| 3:1  | `010` | Operating mode 2: rate generator |
| 0    | `0`   | Binary (not BCD) counting |

**Mode 2 (rate generator)** reloads the countdown register automatically on
each expiry and asserts IRQ0 for one clock cycle. This is the correct mode for
a periodic timer — the counter reloads itself without any software intervention.

The initialisation sequence for 100 Hz:

```c
#define PIT_CMD_PORT  0x43
#define PIT_CH0_PORT  0x40
#define PIT_MODE_RATE 0x36
#define PIT_DIVISOR   11932   /* 1193182 / 100 ≈ 11931.82, round to 11932 */

port_io_write_byte(PIT_MODE_RATE, PIT_CMD_PORT);
port_io_write_byte(PIT_DIVISOR & 0xFF,        PIT_CH0_PORT);  /* low byte  */
port_io_write_byte((PIT_DIVISOR >> 8) & 0xFF, PIT_CH0_PORT);  /* high byte */
```

---

## The tick counter

Every time IRQ0 fires, the CPU dispatches to your `irq_handler` via the stub
and IDT gate installed in sessions 4 and 5. Inside the handler you need to
increment a counter.

This counter is shared between the IRQ handler (which runs at interrupt context)
and the main kernel loop (which reads it to implement sleep). The compiler must
not cache a stale value in a register, and it must not reorder reads across
other code. The correct declaration is:

```c
static volatile uint32_t pit_ticks = 0;
```

`volatile` forces the compiler to re-read the variable from memory on every
access and to not move reads/writes across sequence points. Without it the
compiler may hoist the read out of a spin-wait loop, producing an infinite loop
that never re-reads the (changing) counter.

---

## `ksleep_ms`

With a known tick frequency you can spin-wait for a duration:

```c
void ksleep_ms(uint32_t ms) {
    uint32_t ticks = (PIT_HZ * ms) / 1000;
    uint32_t start = pit_ticks;
    while ((pit_ticks - start) < ticks) {
        __asm__ volatile("hlt");
    }
}
```

The `hlt` inside the loop is not strictly necessary for correctness, but it is
good practice: it tells the CPU to sleep until the next interrupt fires rather
than burning cycles in a tight spin. Because the loop exits on an interrupt and
the increment happens inside the IRQ handler, `hlt` wakes on exactly the event
you are waiting for.

The subtraction `pit_ticks - start` is deliberate unsigned arithmetic: it
handles the case where `pit_ticks` wraps around `UINT32_MAX` during the wait.
As long as the sleep duration is shorter than ~49 days at 1000 Hz, the wrap
is transparent.

---

## What you need to implement

### New files

**`include/pit.h`** — declare:
```c
void pit_init(uint32_t hz);
void ksleep_ms(uint32_t ms);
uint32_t pit_get_ticks(void);
```

**`src/pit.c`** — implement:
- Port constants (`PIT_CMD_PORT`, `PIT_CH0_PORT`)
- `pit_init(uint32_t hz)`: compute the divisor from the requested frequency,
  write `0x36` to the command port, then the divisor low/high bytes to channel 0
- A `static volatile uint32_t pit_ticks` counter
- `pit_get_ticks(void)`: return the current tick count
- `ksleep_ms(uint32_t ms)`: busy-wait as described above

### Modify existing files

**`src/isr.c`** — in `irq_handler`, add a case for IRQ line 0 that calls a
function (or directly increments) the tick counter. The tick increment must
happen before `pic_send_eoi` — incrementing after EOI creates a window where
a re-entrant interrupt could read a stale value.

**`src/kernel.c`** — after `sti()`:
1. Call `pit_init(100)` to configure 100 Hz
2. Use `ksleep_ms` and `pit_get_ticks` to print `tick N` to serial every second

### Build and test

```
make clean && make iso && make run
```

You should see lines like:

```
tick 0
tick 100
tick 200
```

appearing on serial at roughly one-second intervals. Changing `pit_init`'s
argument to 1000 should give the same visible output but with the underlying
counter incrementing 10× faster.

---

## Concept check

Answer these before running `/review`:

1. Why must `pit_ticks` be declared `volatile`? What specific misbehaviour
   would you observe if it were not, and which compiler optimisation causes it?

2. The PIT input clock is 1.193182 MHz. You request 100 Hz. The true divisor
   is 11931.82 — a non-integer. What happens to the timer accuracy, and is
   there anything you can do about it?

3. `ksleep_ms` uses `pit_ticks - start` rather than `pit_ticks >= start +
   ticks`. Why does the subtraction form handle counter wrap-around correctly
   while the comparison form does not?

---

## Mutation exercise

**Part A:** Change `pit_init` to use mode 0 (interrupt on terminal count,
command byte `0x30`) instead of mode 2. Observe what happens to the tick rate
on serial and explain why.

**Part B:** Remove the `volatile` qualifier from `pit_ticks`. Compile with
`-O2` instead of `-Og` (edit the Makefile temporarily). Run and observe whether
`ksleep_ms` still terminates. Restore both changes when done.

When you are satisfied with the whole session, run `/review`.
