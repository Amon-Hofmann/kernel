# Kernel Codebase Quality Rating
> Generated 2026-06-25

## Summary

| TU | Score | Verdict |
|----|:-----:|---------|
| `boot.S` | 8 | Clean Multiboot setup, correct post-return loop |
| `gdt_flush.S` | 9 | Selector constants from shared header; nothing to improve |
| `idt_flush.S` | 9 | Three meaningful lines, no issues |
| `isr_stubs.S` | 8 | Clean; isr_common/irq_common duplication is the one remaining issue |
| `selectors.h` | 8 | Clean, minimal, correct |
| `cpu.h` | 8 | Correct home for sti/cli; proper KERNEL_INLINE idiom |
| `gdt.h` / `gdt.c` | 8 | All previous issues resolved |
| `idt.h` / `idt.c` | 8 | Dead constant commented, not deleted |
| `io.h` | 8 | Correct static inline; no issues |
| `isr.h` / `isr.c` | 8 | All previous issues resolved |
| `pic.h` / `pic.c` | 8 | send_eoi inlined; naming fixed; mask functions clean |
| `serial.h` / `serial.c` | 8 | long_ now implemented; volatile removed; where const correct |
| `string.h` / `string.c` | 8 | Attributes correct; memmove declarations cleaned up |
| `vgaterm.h` / `vgaterm.c` | 8 | Volatile loop correct; enum complete; type-safe attributes |
| `kernel_common.h` / `.c` | 7 | Prefix fixed; one stray `_KERNEL_RET_NONNULL` with leading underscore; `.c` still exists |
| `kernel.h` | 2 | Empty file, no reason to exist |

---

## Detailed feedback (score < 8)

### `kernel_common.h` / `kernel_common.c` — 7

- `_KERNEL_RET_NONNULL` on line 18 has a leading underscore, inconsistent with
  all other macros in the same file which use the `KERNEL_` prefix. Rename it
  to `KERNEL_RET_NONNULL`.
- `kernel_common.c` contains only `#include <kernel_common.h>`. The header
  defines only macros — there is nothing to compile. It is a ghost TU in the
  build that serves no purpose. Delete it, and remove it from whatever wildcard
  picks it up in the makefile (the `$(wildcard src/*.c)` will stop finding it
  once the file is gone).

---

### `kernel.h` — 2

Empty header with only include guards. `kernel_main` is called from assembly
and needs no C declaration. No consumer depends on this file. Delete it.

---

## Notes on the codebase overall

The trajectory from session 1 to now is visible in the scores. The majority of
TUs are at 8 — the threshold where issues are minor and stylistic rather than
structural. The two remaining sub-8 items (`kernel_common.h` and `kernel.h`)
are cleanup tasks, not design problems. The codebase is in a sound state for
continuing to the next session.

One thing worth noting positively: the discipline of moving constants to shared
headers (selectors.h), using the preprocessor for `.S` files, and correctly
handling volatile MMIO through a direct word-copy loop rather than casting —
these are non-obvious decisions that were made correctly.
