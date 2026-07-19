# Session 07 — PS/2 Keyboard

## Goal

Build a keyboard driver. IRQ1 is already routed to your `irq_handler` and port
`0x60` is already read to deassert the interrupt line. In this session you will
capture that scancode byte, translate it from Set 1 to ASCII, track Shift and
Caps Lock state, and echo each keypress to the VGA terminal.

---

## The PS/2 controller

The i8042 PS/2 controller bridges the keyboard to the CPU. It exposes two I/O
ports:

| Port | Direction | Function |
|------|-----------|---------|
| `0x60` | R/W | Data register — read scancodes; write device commands |
| `0x64` | R   | Status register — buffer state flags |
| `0x64` | W   | Command register — send commands to the controller |

### Status register (port 0x64)

| Bit | Name | Meaning |
|-----|------|---------|
| 0   | OBF  | Output Buffer Full — data is waiting in `0x60` to be read |
| 1   | IBF  | Input Buffer Full — controller busy; do not write yet |
| 6   | TO   | Timeout error |
| 7   | PERR | Parity error |

In an interrupt-driven driver you do not need to poll OBF before reading — the
interrupt guarantees the buffer is full. The OBF check matters for polled
drivers and for controller commands where you write a byte and then read a
response. For now, IRQ1 fires and you read.

What you must always do inside the IRQ1 handler is **read port `0x60` exactly
once** before sending EOI. Your `irq_handler` already does this to deassert the
IRQ line; you will capture the return value instead of discarding it.

---

## Set 1 scancodes

The PS/2 keyboard sends **scancodes**, not ASCII. Scancode Set 1 is the
power-on default on PC hardware.

### Make and break codes

Every key generates two events:

- **Make code**: sent when the key is **pressed**. Values range `0x01`–`0x58`
  for standard keys.
- **Break code**: sent when the key is **released**. The break code is the make
  code with bit 7 set: `break = make | 0x80`.

Separating them in code:

```c
uint8_t make     = scancode & 0x7F;
bool    is_break = scancode & 0x80;
```

Because the make code is always a 7-bit value (0–127), a lookup table of 128
entries indexed directly by `make` covers all standard keys.

### Key scancode reference (Set 1)

| Hex       | Unshifted    | Shifted      | Notes              |
|-----------|--------------|--------------|--------------------|
| 0x01      | —            | —            | Escape             |
| 0x02–0x0B | `1`–`0    `      | `!`–`    )`      | Number row         |
| 0x0C      | `-`            | `_`            |                    |
| 0x0D      | `=`            | `+`            |                    |
| 0x0E      | `\b`           | `\b`           | Backspace          |
| 0x0F      | `\t`           | `\t`           | Tab                |
| 0x10–0x19 | `qwertyuiop`   | `QWERTYUIOP`   |                    |
| 0x1A      | `[`            | `{`            |                    |
| 0x1B      | `]`            | `}`            |                    |
| 0x1C      | `\n`           | `\n`           | Enter              |
| 0x1D      | —            | —            | Left Ctrl          |
| 0x1E–0x26 | `asdfghjkl`    | `ASDFGHJKL  `  |                    |
| 0x27      | `;`            | `:`            |                    |
| 0x28      | `'`            | `"`            |                    |
| 0x29      | `` ` ``          | `~`            |                    |
| 0x2A      | —            | —            | Left Shift         |
| 0x2B      | `\`            | `\|`           |                    |
| 0x2C–0x32 | `zxcvbnm`      | `ZXCVBNM`      |                    |
| 0x33      | `,`            | `<`            |                    |
| 0x34      | `.`            | `>`            |                    |
| 0x35      | `/`            | `?`            |                    |
| 0x36      | —            | —            | Right Shift        |
| 0x37      | `*`            | `*`            | Numpad multiply    |
| 0x38      | —            | —            | Left Alt           |
| 0x39      | ` `            | ` `            | Space              |
| 0x3A      | —            | —            | Caps Lock (toggle) |
| 0x3B–0x44 | —            | —            | F1–F10             |

---

## Scancode-to-ASCII tables

Represent the translation as two arrays of 128 `char` entries, indexed by the
7-bit make code. Non-printable entries are `0`; the handler skips them.

```c
static const char lower[128] = {
/*        +0    +1    +2    +3    +4    +5    +6    +7    +8    +9    +A    +B    +C    +D    +E    +F   */
/* 0x00 */  0,    0,  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',  '-',  '=', '\b', '\t',
/* 0x10 */ 'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  'o',  'p',  '[',  ']', '\n',   0,  'a',  's',
/* 0x20 */ 'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';', '\'',  '`',   0,  '\\', 'z',  'x',  'c',  'v',
/* 0x30 */ 'b',  'n',  'm',  ',',  '.',  '/',   0,   '*',   0,   ' ',   0,    0,    0,    0,    0,    0,
/* 0x40 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x50 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x60 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x70 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};

static const char upper[128] = {
/*        +0    +1    +2    +3    +4    +5    +6    +7    +8    +9    +A    +B    +C    +D    +E    +F   */
/* 0x00 */  0,    0,  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',  '_',  '+', '\b', '\t',
/* 0x10 */ 'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  'O',  'P',  '{',  '}', '\n',   0,  'A',  'S',
/* 0x20 */ 'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':', '"',  '~',   0,   '|', 'Z',  'X',  'C',  'V',
/* 0x30 */ 'B',  'N',  'M',  '<',  '>',  '?',   0,   '*',   0,   ' ',   0,    0,    0,    0,    0,    0,
/* 0x40 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x50 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x60 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x70 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
```

---

## Shift and Caps Lock

Track modifier state with two static flags:

```c
static bool shift_held  = false;
static bool caps_active = false;
```

Modifier make/break codes:

| Key         | Make   | Break  |
|-------------|--------|--------|
| Left Shift  | `0x2A` | `0xAA` |
| Right Shift | `0x36` | `0xB6` |
| Caps Lock   | `0x3A` | `0xBA` |

Shift is a **held** modifier: set `shift_held = true` on the make event, clear
it on the break. Caps Lock is a **toggle**: flip `caps_active` on the make
event only; ignore the break.

### Selecting the right character

```c
bool use_upper = shift_held ^ (caps_active && lower[make] >= 'a'
                                           && lower[make] <= 'z');
char ch = use_upper ? upper[make] : lower[make];
```

Why XOR: Caps Lock only applies to alphabetic keys and only inverts the base
state. With Caps Lock on and Shift held, letters revert to lowercase (the two
states cancel), but punctuation remains shifted — matching real keyboard
behaviour.

---

## What you need to implement

### New files

**`include/keyboard.h`** — declare:
```c
void keyboard_handler(uint8_t scancode);
```

**`src/keyboard.c`** — implement:
- The `lower` and `upper` tables as shown above (`static const char [128]`)
- `static bool shift_held` and `static bool caps_active` (include `<stdbool.h>`)
- `keyboard_handler(uint8_t scancode)`:
  1. Extract make code (`scancode & 0x7F`) and break flag (`scancode & 0x80`)
  2. On Left Shift (0x2A) or Right Shift (0x36): update `shift_held`
  3. On Caps Lock (0x3A) make: toggle `caps_active`
  4. For all other make codes: compute `use_upper`, look up the character;
     if non-zero, call `terminal_putchar(ch)`

### Modify existing files

**`src/isr.c`** — in `irq_handler`, for IRQ line 1: capture the scancode from
`port_io_read_byte(0x60)` and pass it to `keyboard_handler`. Add `#include
<keyboard.h>`.

**`src/kernel.c`** — unmask IRQ1 with `pic_clear_mask(1)` alongside the
existing `pic_clear_mask(0)`. The keyboard handler fires from the interrupt; no
polling loop is needed in `kernel_main`.

### Build and test

```
make clean && make iso && make run
```

Click into the QEMU window and press keys. Each keypress should produce the
corresponding ASCII character on the VGA terminal. Verify:
- Letter keys print lowercase; Shift produces uppercase
- Number row and punctuation produce correct shifted variants
- Caps Lock toggles letter case; Shift + Caps Lock restores lowercase letters
- Space, Backspace, and Enter produce visible output (space/cursor move,
  backspace can be ignored by the terminal for now — it just needs not to crash)

---

## Concept check

Answer these before running `/review`:

1. Your `irq_handler` reads port `0x60` before calling `pic_send_eoi`. Why
   must the read happen before the EOI, not after? What would you observe if
   you reversed the order?

2. The make code for the `A` key is `0x1E`. What byte does the keyboard send
   when `A` is released, and how does your driver distinguish that from a
   keypress?

3. Explain the expression `shift_held ^ (caps_active && ...)`. Why is XOR the
   right operator here rather than OR? Give a concrete example with Caps Lock
   on and Shift held to show why OR would give the wrong result.

---

## Mutation exercise

**Part A:** In `irq_handler`, comment out the `port_io_read_byte(0x60)` call
(move the call to `keyboard_handler` so you still get the scancode, but stop
draining the buffer before EOI). Run the kernel and press a single key.
Observe how many characters appear and explain why.

**Part B:** Swap the `lower` and `upper` table references in the `use_upper`
selection — pass `upper[make]` when `!use_upper` and vice versa. Press several
keys with and without Shift and verify the output matches your prediction.
Restore when done.

When you are satisfied with the whole session, run `/review`.
