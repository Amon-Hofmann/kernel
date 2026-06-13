# Session 3.1 — Serial Output: UART & Port I/O

## Goal

A working serial driver for COM1. By the end of this session `make run` should
print output in your terminal via QEMU's `-serial stdio`. You will use this
instead of the VGA stress loop for verifying the kernel boots — it is faster
under KVM and gives you a debug channel for every session that follows.

---

## Port I/O vs memory-mapped I/O

You already know MMIO: the VGA controller maps its buffer at physical address
`0xB8000`. A plain `mov` instruction writes to it.

The UART uses a different mechanism: **port I/O**. The x86 architecture has a
separate 16-bit I/O address space — 65536 ports — accessed with two dedicated
instructions:

```asm
in  %dx, %al    /* read one byte from port in %dx into %al */
out %al, %dx    /* write one byte from %al to port in %dx  */
```

In C, GCC exposes these as `__asm__` inline or you can wrap them in small
inline functions. There is no pointer arithmetic — you address hardware by
port number, not by memory address.

Why does the UART use port I/O rather than MMIO? Historical convention: the
original IBM PC mapped the UART into the I/O space. The VGA was added later
and its large framebuffer needed a memory address range. Both mechanisms
coexist on the same bus; the CPU chooses which address space to use based on
whether the access is a `mov` (memory) or `in`/`out` (I/O).

**The KVM advantage:** MMIO accesses to unmapped ranges (like VGA) require a
VM exit so QEMU can emulate the device. Port I/O also requires a VM exit in
general, but KVM can batch or fast-path many UART writes. More importantly,
serial output through `-serial stdio` in QEMU avoids the VGA emulation path
entirely — characters go straight to your terminal.

---

## The 16550 UART

The PC serial port standard is the **16550 UART** (Universal Asynchronous
Receiver/Transmitter). COM1 is at base I/O port `0x3F8`. Each register is
one byte wide, accessed at `base + offset`:

| Offset | DLAB | Register | Purpose |
|--------|------|----------|---------|
| +0     | 0    | RBR / THR | Receive Buffer / Transmit Holding |
| +1     | 0    | IER       | Interrupt Enable |
| +0     | 1    | DLL       | Baud Rate Divisor Low |
| +1     | 1    | DLH       | Baud Rate Divisor High |
| +2     | —    | FCR / IIR | FIFO Control / Interrupt ID |
| +3     | —    | LCR       | Line Control |
| +4     | —    | MCR       | Modem Control |
| +5     | —    | LSR       | Line Status |

**DLAB** (Divisor Latch Access Bit) is bit 7 of LCR. When DLAB=1, offsets +0
and +1 address the baud rate divisor instead of the data registers.

### Baud rate

The UART's internal clock runs at 115200 Hz. The divisor sets the baud rate:

```
divisor = 115200 / baud_rate
The implementation went with the full 115200 baud
```

For 38400 baud: divisor = 3 (low byte = 3, high byte = 0).

### Line Control Register (LCR, offset +3)

| Bits | Name | Meaning |
|------|------|---------|
| 1:0  | WLS  | Word length: 11 = 8 bits |
| 2    | STB  | Stop bits: 0 = 1 stop bit |
| 3    | PEN  | Parity enable: 0 = no parity |
| 7    | DLAB | Divisor latch access |

For 8N1 (8 data bits, no parity, 1 stop bit): LCR = `0x03`.

### Line Status Register (LSR, offset +5)

| Bit | Name | Meaning |
|-----|------|---------|
| 5   | THRE | Transmit Holding Register Empty — safe to write next byte |

Before writing a byte you must wait until THRE = 1, otherwise you overwrite a
byte that has not been sent yet.

---

## Initialisation sequence

To bring the UART up from reset:

1. Set DLAB=1 (LCR bit 7) to access the divisor
2. Write baud rate divisor low byte to offset +0
3. Write baud rate divisor high byte to offset +1
4. Clear DLAB, set 8N1: write `0x03` to LCR (offset +3)
5. Enable and reset FIFOs: write `0xC7` to FCR (offset +2)
6. Enable DTR and RTS: write `0x03` to MCR (offset +4)
7. Disable all interrupts: write `0x00` to IER (offset +1)

Step 7 last — after DLAB is cleared so offset +1 addresses IER again.

---

## What you need to implement

### New files

**`include/serial.h`** — declare:
- `#define SERIAL_COM1_BASE 0x3F8`
- `void serial_init(void)`
- `void serial_putchar(char c)`
- `void serial_writestring(const char *s)`

**`src/serial.c`** — implement:

Two helper inline functions first — one to write a byte to a port, one to
read a byte from a port. Use `__asm__ volatile` with the `out` / `in`
instructions. Look at `gdt_flush.s` for AT&T syntax reference; inline asm
form is `__asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port))` for
output and `__asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port))` for
input. Make them `static inline` — they are implementation details.

`serial_init`: follow the initialisation sequence above. Use `SERIAL_COM1_BASE`
as the base and write each register at `base + offset`.

`serial_putchar`: poll LSR bit 5 (THRE) in a loop until it is set, then write
the character to THR (offset +0).

`serial_writestring`: call `serial_putchar` for each character until the null
terminator.

### Modify existing files

**`src/kernel.c`** — add `serial_init()` after `gdt_install()`, then add a
`serial_writestring("Hello from serial!\n")` call. Remove or replace the
2000-iteration ERWIN loop with something that does not hammer the VGA buffer.

### Build and test

```
make clean && make iso && make run
```

`Hello from serial!` should appear in your terminal (stdout of QEMU, via
`-serial stdio`). The VGA window should show `Hello, Kernel!` as before.

---

## Concept check

Answer these before running `/review`:

1. What is the mechanical difference between a port I/O access and a
   memory-mapped I/O access at the CPU instruction level? Why does the CPU
   know which address space to use?

2. What does polling the THRE bit before each write prevent, and what would
   happen if you skipped the check and wrote bytes as fast as possible?

When you are done, run `/review`.
