# Kernel Codebase Quality Rating
> Generated 2026-06-22 (re-rated after working down through serial TU)

## Summary

| TU | Score | Verdict |
|----|:-----:|---------|
| `boot.S` | 8 | Clean Multiboot setup, correct post-return loop |
| `gdt_flush.S` | 9 | Selector constants from shared header; nothing to improve |
| `idt_flush.S` | 9 | Three meaningful lines, no issues |
| `isr_stubs.S` | 8 | Typo fixed; shared header; isr_common/irq_common duplication remains |
| `selectors.h` | 8 | Clean, minimal, correct; serves C and assembly equally |
| `cpu.h` | 8 | Correct home for sti/cli; proper static inline idiom |
| `gdt.h` / `gdt.c` | 8 | All previous issues resolved |
| `idt.h` / `idt.c` | 7 | `idt_set_gate` not static in .c; dead constant; forward decl param shadow |
| `io.h` | 8 | volatile removed; static inline correct; dead io.c deleted |
| `isr.h` / `isr.c` | 7 | Halt loop split across 3 asm blocks; `__COLD`/`__HOT` missing from declarations |
| `pic.h` / `pic.c` | 8 | send_eoi inlined; constants shared; naming fixed; mask functions normalised |
| `serial.h` / `serial.c` | 7 | `volatile val` still redundant; `long_` tracked but never used |
| `string.h` / `string.c` | 7 | Missing `returns_nonnull`, `warn_unused_result`; C89 declarations in memmove |
| `vgaterm.h` / `vgaterm.c` | 6 | `vga` not static; volatile cast-away; `const` on value params |
| `kernel_common.h` / `.c` | 6 | `__` prefix reserved; `.c` file pointless; missing `__RETURNS_NONNULL` |
| `kernel.h` | 2 | Empty file, no reason to exist |

---

## Detailed feedback (score < 8)

### `idt.h` / `idt.c` — 7

- `idt_set_gate` declaration is commented out in `idt.h` but the definition in
  `idt.c` is not `static`. Either make it `static void idt_set_gate(...)` in
  `idt.c` and remove the commented declaration, or leave it public and
  uncomment the declaration. The current state is inconsistent.
- `IDT_TRAP_GATE (0x8F)` is defined in `idt.c` and never used. Delete it.
- `void idt_flush(idt_ptr_t *idt_ptr)` forward declaration: parameter name
  `idt_ptr` shadows the static global `idt_ptr`. Strip the parameter name:
  `void idt_flush(idt_ptr_t *);`

---

### `isr.h` / `isr.c` — 7

- The halt sequence in `exception_handler` is split across three separate
  `__asm__ volatile` blocks (`cli`, `hlt`, `jmp 1b`) followed by `while
  (true) {}`. Merge into one: `__asm__ volatile("cli\n\t1: hlt\n\tjmp 1b")`.
  The `while(true){}` is dead code added to satisfy GCC's `noreturn` check —
  a single asm block makes it unnecessary since GCC sees the `jmp 1b` as a
  non-returning infinite loop when `__attribute__((noreturn))` is present.
- `__COLD` is on the definition of `exception_handler` in `isr.c` but not on
  the declaration in `isr.h`. GCC uses the declaration for call-site
  optimizations (branch prediction hints). Add `__COLD` to the declaration too.
- `irq_handler` is on the hot path (called every hardware interrupt). Add
  `__HOT` to both the declaration in `isr.h` and the definition in `isr.c`.
- `itr_names` and `irq_names` should be `const char * const []` — the pointers
  themselves are fixed entries in a lookup table that never get reassigned.
  Currently `const char *` only protects the pointed-to strings, not the array
  elements. Use `static const char * const itr_names[32] = {...}`.

---

### `serial.h` / `serial.c` — 7

- `uint8_t volatile val = 0` in `serial_putchar`: `volatile` on the local
  variable is redundant. The value is produced by a `volatile` asm output
  constraint inside the inlined `port_io_read_byte`. The compiler cannot
  optimize away the port read regardless of whether `val` is volatile. Remove
  the `volatile`.
- `bool long_ __UNUSED = false` in `serial_printf`: the `l` length prefix is
  parsed and tracked but the switch never reads `long_` — `va_arg(args,
  uint32_t)` is called regardless. On i386 `unsigned long` and `uint32_t` are
  both 32-bit so no runtime bug exists, but the tracking is dead code silenced
  with `__UNUSED`. Either implement it (read `unsigned long` when `long_` is
  true) or remove it entirely and rely on the call-site casts already in place
  in `isr.c`.

---

### `string.h` / `string.c` — 7

- `memcpy`, `memmove`, `memset` always return their first argument and never
  return NULL. Add `__attribute__((returns_nonnull))` to all three
  declarations in `string.h`. Consider adding a `__RETURNS_NONNULL` macro to
  `kernel_common.h` for this.
- Add `__WUNUSED` to `memcpy` and `memmove` declarations. Silently discarding
  their return value is almost always a bug.
- `memmove`: `char *temp; const char *s;` are declared uninitialized at the
  top of the function and assigned below. Write `char *temp = dest; const char
  *s = src;` where first used. This is C99 — declarations belong at the point
  of first use, not at the top of the block.

---

### `vgaterm.h` / `vgaterm.c` — 6

- `volatile uint16_t *vga` has external linkage (no `static`). Any other TU
  can reach in and modify or overwrite this pointer. Make it
  `static volatile uint16_t *vga`.
- `memmove((void *)vga, ...)` casts away `volatile`. The memmove internals use
  `char *` so the writes happen in practice, but the cast is UB. For VGA
  scrolling, a direct volatile word-copy loop over the pointer is correct by
  construction and avoids the cast.
- `terminal_set_attribute(const char fg, const char bg)` and
  `terminal_putchar(const char c)`: `const` on by-value parameters is
  meaningless to the caller. Drop `const` from the header declarations (keep it
  in the `.c` body if you want the local guarantee).
- `terminal_writestring` is missing `__NONNULL`.
- `TERMINAL_FG_*` / `TERMINAL_BG_*` constants are bare integers. An
  `enum vga_color` would give type safety at `terminal_set_attribute` call
  sites.

---

### `kernel_common.h` / `kernel_common.c` — 6

- All macro names use the double-underscore prefix (`__INLINE`, `__NONNULL`,
  etc.). Double-underscore prefixes are reserved by the C standard for the
  implementation. Rename to a project prefix: `KERN_INLINE`, `KERN_NONNULL`,
  `KERN_NORETURN`, `KERN_PACKED`, `KERN_UNUSED`, `KERN_COLD`, `KERN_HOT`,
  `KERN_WUNUSED`. This is a breaking rename across the codebase but the right
  call.
- `kernel_common.c` contains only `#include <kernel_common.h>`. It compiles a
  header that defines only macros — nothing to compile. Delete it.
- Add `__RETURNS_NONNULL` / `KERN_RETURNS_NONNULL` for
  `__attribute__((returns_nonnull))`. It is needed by `string.h` and will be
  needed by future allocators.

---

### `kernel.h` — 2

Empty header with only include guards. `kernel_main` is called from assembly
and needs no C declaration. No consumer depends on this file for anything.
Delete it.
