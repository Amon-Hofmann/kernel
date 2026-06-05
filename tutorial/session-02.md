# Session 02 — VGA Text Mode

## Goal

Print `Hello, Kernel!` to the screen.

By the end of this session `make run` should show text in the QEMU window.
You will implement a small terminal driver: character output, color control,
column/row tracking, and line wrapping. No scrolling required yet — that is the
mutation exercise.

---

## Memory-mapped I/O

Most hardware communication happens through one of two mechanisms:

1. **Port I/O** — the CPU has dedicated `in`/`out` instructions that talk to
   hardware registers via a separate 16-bit I/O address space. You will use
   this for the PIC, PIT, and keyboard in later sessions.

2. **Memory-mapped I/O (MMIO)** — the hardware maps its registers or buffers
   into the normal memory address space. The CPU writes to an ordinary address,
   the memory controller routes it to the device instead of RAM.

VGA text mode uses MMIO. The VGA controller maps its **video buffer** starting
at physical address `0xB8000`. Writing bytes to that address range causes the
VGA hardware to display them on screen — no system call, no driver interface,
no setup required. The address has been fixed by the IBM PC standard since 1981
and GRUB leaves it intact.

**Why does this work without paging?** Paging is off. In the absence of a page
directory, the CPU uses physical addresses directly. Physical address `0xB8000`
is the VGA buffer. A `mov` instruction targeting that address writes to the
screen. It is that simple.

---

## VGA text mode geometry

The standard VGA text mode is **80 columns × 25 rows**. Every character cell
occupies exactly **2 bytes** in the buffer:

```
Byte offset 0:  character code (ASCII)
Byte offset 1:  attribute byte
```

The buffer is laid out row by row, left to right. Cell at column `c`, row `r`
is at byte offset:

```
offset = (r * 80 + c) * 2
```

Or as a 16-bit word index:

```
index = r * 80 + c
```

The buffer is most naturally accessed as an array of `uint16_t`:

```c
volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
vga[r * 80 + c] = entry;
```

The `volatile` qualifier is critical — without it the compiler may reorder or
eliminate writes it considers "redundant" because it cannot see any read of
the same memory. The VGA controller reads the buffer; the compiler cannot know
that. `volatile` tells it: every write to this pointer must actually happen, in
order.

---

## The attribute byte

Each cell's second byte controls its appearance:

```
Bit 7:    blink (or bright background, depending on mode)
Bits 6–4: background color (3 bits → 8 colors)
Bits 3–0: foreground color (4 bits → 16 colors)
```

The standard 4-bit VGA color palette:

| Value | Color         | Value | Color          |
|-------|---------------|-------|----------------|
| 0     | Black         | 8     | Dark grey      |
| 1     | Blue          | 9     | Bright blue    |
| 2     | Green         | 10    | Bright green   |
| 3     | Cyan          | 11    | Bright cyan    |
| 4     | Red           | 12    | Bright red     |
| 5     | Magenta       | 13    | Bright magenta |
| 6     | Brown         | 14    | Yellow         |
| 7     | Light grey    | 15    | White          |

A cell entry combines attribute and character into one 16-bit value:

```c
uint16_t entry = (uint16_t)attribute << 8 | (uint16_t)character;
```

And the attribute combines foreground and background:

```c
uint8_t attribute = (background << 4) | foreground;
```

---

## What you need to implement

### Files to create

**`include/vgaterm.h`** — declare the public interface:
- A color enum or `#define` constants for the 16 VGA colors
- `void terminal_initialize(void)`
- `void terminal_putchar(char c)`
- `void terminal_writestring(const char *str)`

**`src/vgaterm.c`** — implement the terminal driver. Internal state it needs:
- Current row and column (start at 0, 0)
- Current color attribute (your choice of default; light grey on black is
  conventional)
- Pointer to the VGA buffer (`volatile uint16_t *` at `0xB8000`)

`terminal_initialize` must:
- Set row and column to 0
- Set a default color
- Clear the entire screen (write spaces with the default attribute to all
  80×25 cells)

`terminal_putchar` must:
- Write the character + attribute to `vga[row * 80 + col]`
- Advance the column
- Wrap to the next row when column reaches 80
- When row reaches 25: for now, just reset row to 0 (wrap around). The
  mutation exercise will ask you to scroll instead.

`terminal_writestring` must:
- Call `terminal_putchar` for each character until the null terminator

**`include/string.h`** — declare `size_t strlen(const char *str)`

**`src/string.c`** — implement `strlen`. No C library in freestanding mode —
you must provide it yourself.

**`src/kernel.c`** — update `kernel_main` to:
```c
terminal_initialize();
terminal_writestring("Hello, Kernel!");
```

### Types you will need

`size_t` is typically defined as `unsigned int` or `unsigned long`. In a
freestanding environment you can either define it yourself or include
`<stddef.h>`, which GCC provides even in freestanding mode and defines
`size_t`, `NULL`, and `ptrdiff_t` without pulling in any libc.

Similarly, `uint8_t` and `uint16_t` are available from `<stdint.h>`, which
GCC also provides in freestanding mode.

### Build and test

```
make clean && make iso && make run
```

You should see `Hello, Kernel!` in the top-left of the QEMU window in light
grey on black (or whatever color you chose).

If you see garbage, a blank screen, or a triple fault, the most common causes
are:
- Missing `volatile` on the VGA pointer — writes optimised away
- Wrong buffer address
- Off-by-one in the index calculation
- `strlen` returning wrong length

---

## Concept check

Answer these before running `/review`:

1. Why is `0xB8000` a valid address to write to without any memory setup —
   doesn't paging need to be configured first?

2. What does the `volatile` keyword do here, and what would go wrong without
   it? (Think about what the compiler is allowed to assume about memory it
   never reads back.)

3. You implemented your own `strlen` because the C standard library is not
   available. But GCC itself sometimes *generates* calls to `memcpy`, `memset`,
   or `strlen` as compiler optimisations — even in freestanding mode. What flag
   or pragma prevents this, and why is it a problem if those calls go
   unresolved?

---

## Mutation exercise

Do both parts after `Hello, Kernel!` appears on screen.

**Part A — change background color:**
Change the default terminal color so the entire cleared screen has a **red
background** with **white foreground**. Rebuild and confirm in QEMU. You should
only need to change one value — the attribute byte — not the display logic.

**Part B — implement scrolling:**
Right now when row reaches 25 you wrap back to row 0, overwriting the top.
Replace that with real scrolling:
- Copy rows 1–24 up to rows 0–23 (move each cell's 16-bit value one row up)
- Clear row 24 (write spaces with the current attribute)
- Keep row at 24 (do not reset to 0)

Test it by writing enough text to fill more than 25 rows. The terminal should
scroll smoothly rather than jumping back to the top.

When you are done with both parts, run `/review`.
