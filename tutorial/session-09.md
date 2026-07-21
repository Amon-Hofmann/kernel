# Session 09 — Physical Memory Manager

## Goal

GRUB maps the machine's physical memory layout at boot time and passes it as a
structured block in the Multiboot info record. In this session you will read that
map, build a bitmap that tracks which 4 KiB frames are free, and expose
`pmm_alloc_frame` / `pmm_free_frame` — the foundation every later memory
subsystem (paging, kernel heap) depends on.

---

## The Multiboot memory map

When GRUB calls `_start`, two registers hold values you need to preserve:

| Register | Value |
|----------|-------|
| `%eax` | Multiboot magic (`0x2BADB002`) — confirms a Multiboot boot |
| `%ebx` | Physical address of a `multiboot_info_t` structure |

`boot.S` currently discards both. Push them before `call kernel_main` so they
become its arguments (i386 cdecl pushes right-to-left — second argument first):

```asm
push %ebx        /* arg 2: mbi */
push %eax        /* arg 1: magic */
call kernel_main
```

`kernel_main` then becomes:

```c
void kernel_main(uint32_t magic, multiboot_info_t *mbi)
```

### The info structure

The `multiboot_info_t` struct has many fields. The ones relevant to this session:

| Field       | Offset | Type     | Meaning |
|-------------|--------|----------|---------|
| `flags`       | 0      | `uint32_t` | Bitmask of which fields below are valid |
| `mmap_length` | 44     | `uint32_t` | Byte length of the memory map array |
| `mmap_addr`   | 48     | `uint32_t` | Physical address of the first entry |

Bit 6 of `flags` must be set before `mmap_addr` and `mmap_length` are valid.
Always check it:

```c
if (!(mbi->flags & (1 << 6))) { /* no mmap — panic */ }
```

### Memory map entries

The memory map is an array of variable-length entries packed end to end:

```c
typedef struct {
    uint32_t size;    /* byte length of this entry, not counting this field */
    uint64_t addr;    /* base physical address of the region */
    uint64_t len;     /* length in bytes */
    uint32_t type;    /* 1 = usable RAM, anything else = reserved */
} __attribute__((packed)) multiboot_mmap_entry_t;

#define MULTIBOOT_MEMORY_AVAILABLE 1
```

`size` does not include the 4 bytes occupied by the `size` field itself, so
each entry is `size + sizeof(entry->size)` bytes wide. Traverse the array with
a byte offset:

```c
uint32_t off = 0;
while (off < mbi->mmap_length) {
    multiboot_mmap_entry_t *e =
        (multiboot_mmap_entry_t *)(uintptr_t)(mbi->mmap_addr + off);
    /* use e */
    off += e->size + sizeof(e->size);
}
```

### 64-bit addresses in a 32-bit kernel

`addr` and `len` are `uint64_t`. On i386, GCC handles 64-bit arithmetic
correctly, but the physical address space is only 32 bits wide (4 GiB maximum).
Skip any entry whose base address is at or above 4 GiB, and cap any entry that
extends past 4 GiB:

```c
if (e->addr >= 0x100000000ULL) continue;
uintptr_t base = (uintptr_t)e->addr;
uintptr_t len  = (e->addr + e->len > 0x100000000ULL)
                 ? (uintptr_t)(0x100000000ULL - e->addr)
                 : (uintptr_t)e->len;
```

---

## Page frames

The hardware paging unit (session 10) maps memory in 4 KiB units — a
*page frame* is one 4 KiB-aligned block of physical memory. Allocating anything
smaller at the physical level is meaningless: the MMU cannot track it.

With 32-bit addresses there are at most 4 GiB / 4 KiB = 1 048 576 frames.

---

## Bitmap representation

A bitmap assigns one bit per frame:

- `0` → frame is free
- `1` → frame is in use

```c
#define FRAME_SIZE     4096U
#define MAX_FRAMES     (1024U * 1024U)   /* 4 GiB / 4 KiB */

static uint32_t bitmap[MAX_FRAMES / 32]; /* 128 KiB */
```

Frame N maps to word `N / 32`, bit `N % 32`:

```
Physical Memory                    Bitmap (uint32_t[])

─────── 0x000000 ──┐              ┌─ bitmap[0] ─────────────────────────────┐
Frame  0   (4 KiB) │              │ bit  31   30  ...   1    0              │
─────── 0x001000   ├─────────────►│      ↕    ↕         ↕    ↕              │
Frame  1   (4 KiB) │              │ fr   31   30  ...   1    0              │
─────── 0x002000   │              └─────────────────────────────────────────┘
...                │
─────── 0x01F000   │              ┌─ bitmap[1] ─────────────────────────────┐
Frame 31   (4 KiB) ┘              │ bit  31   30  ...   1    0              │
─────── 0x020000 ──┐              │      ↕    ↕         ↕    ↕              │
Frame 32   (4 KiB) ├─────────────►│ fr   63   62  ...  33   32              │
─────── 0x021000   │              └─────────────────────────────────────────┘
...                │
─────── 0x03F000   │              ┌─ bitmap[W] ─────────────────────────────┐
Frame 63   (4 KiB) ┘              │ bit   B                    0            │
─────── 0x040000                  │       ↕                    ↕            │
                                  │ frame W·32+B           W·32+0           │
                                  └─────────────────────────────────────────┘

Frame N  →  word = N / 32,   bit = N % 32
Addr  A  →  frame = A >> 12
Frame N  →  addr  = N << 12
```

```c
/* mark frame N used */
bitmap[N / 32] |=  (1u << (N % 32));

/* mark frame N free */
bitmap[N / 32] &= ~(1u << (N % 32));

/* test frame N */
bitmap[N / 32] &   (1u << (N % 32));    /* non-zero → in use */
```

### Allocation

Scan for the first word that is not all-ones, then find its lowest zero bit with
`__builtin_ctz` (count trailing zeros):

```c
uintptr_t pmm_alloc_frame(void) {
    for (size_t i = 0; i < MAX_FRAMES / 32; i++) {
        if (bitmap[i] == 0xFFFFFFFF) continue;
        uint32_t bit = __builtin_ctz(~bitmap[i]);
        bitmap[i] |= (1u << bit);
        return (uintptr_t)((i * 32 + bit) * FRAME_SIZE);
    }
    return 0;   /* out of memory */
}
```

`~bitmap[i]` inverts the bitmap so that set bits mean free frames.
`__builtin_ctz(~bitmap[i])` gives the position of the lowest free frame in that
word. The frame's physical address is its index multiplied by `FRAME_SIZE`.

---

## Initialisation strategy

Start with everything marked used (`memset(bitmap, 0xFF, sizeof(bitmap))`), then:

1. Walk the Multiboot memory map. For each entry with `type == MULTIBOOT_MEMORY_AVAILABLE`, mark those frames free.
2. Re-mark regions that must stay reserved regardless of what the memory map says:
   - **First 1 MiB** (0–0xFFFFF) — contains the real-mode interrupt vector table, BIOS data area, VGA buffer (0xB8000–0xBFFFF), and BIOS code. Handing any of it to the allocator would corrupt hardware state.
   - **The kernel image** — code and data currently executing. If the allocator gave out a kernel frame, the next write to that page would overwrite your own code or data.

Starting from "all used" is the safe choice: any gap in the memory map (a region
not listed at all) stays reserved. Starting from "all free" would require knowing
and enumerating every reserved region — which you cannot do exhaustively.

### Rounding conservatively

When marking a region **free**, round *inward* (only mark frames entirely within
the region):

```
free_start = ALIGN_UP(addr, FRAME_SIZE)
free_end   = ALIGN_DOWN(addr + len, FRAME_SIZE)
```

When marking a region **used**, round *outward* (mark partial frames at both
ends as used too):

```
used_start = ALIGN_DOWN(addr, FRAME_SIZE)
used_end   = ALIGN_UP(addr + len, FRAME_SIZE)
```

---

## Linker symbols for the kernel image

The linker can expose the start and end addresses of the kernel image as
symbols. In `cfg/kernel.ld`, add two symbol assignments around all output
sections:

```
kernel_phys_start = .;

.text BLOCK(4K) : ALIGN(4K) { ... }
...
.bss BLOCK(4K) : ALIGN(4K) { ... }

kernel_phys_end = .;
```

These are not variables — they are symbols whose *address* is the value. In C,
take their address to get the physical boundary:

```c
extern uint32_t kernel_phys_start;
extern uint32_t kernel_phys_end;

uintptr_t ks = (uintptr_t)&kernel_phys_start;
uintptr_t ke = (uintptr_t)&kernel_phys_end;
```

---

## What you need to implement

### New files

**`include/multiboot.h`** — Multiboot structures. Define
`multiboot_mmap_entry_t` as shown above plus a `multiboot_info_t` covering at
least `flags`, the 16-byte `syms` placeholder at offset 28, `mmap_length`, and
`mmap_addr`. Both structs must carry `__attribute__((packed))`.

**`include/pmm.h`** — PMM API:

```c
#include <multiboot.h>
#include <stdint.h>

void      pmm_init(multiboot_info_t *mbi);
uintptr_t pmm_alloc_frame(void);   /* returns 0 if no frame is available */
void      pmm_free_frame(uintptr_t addr);
```

**`src/pmm.c`** — bitmap allocator. You will need the following functions:

- **`static void mark_used(uintptr_t addr, uintptr_t len)`** — converts an
  address range to a frame range (rounding outward), then sets the corresponding
  bits in the bitmap. Define `ALIGN_UP` / `ALIGN_DOWN` macros locally.

- **`static void mark_free(uintptr_t addr, uintptr_t len)`** — same, but clears
  bits and rounds inward so only fully-covered frames are freed.

- **`void pmm_init(multiboot_info_t *mbi)`** — check bit 6 of `mbi->flags`,
  then follow the three-step strategy above: memset the bitmap to `0xFF`, walk
  the mmap entries using `mbi->mmap_addr` and `mbi->mmap_length` (applying the
  64-bit address handling described earlier), then re-mark the first 1 MiB and
  the kernel image as used. Use `extern` declarations to access the linker
  symbols.

- **`uintptr_t pmm_alloc_frame(void)`** — scan `bitmap[]` for the first word
  that is not `0xFFFFFFFF`, extract the lowest zero bit with `__builtin_ctz`,
  mark it used, and return the corresponding physical address. Return `0` if no
  frame is free.

- **`void pmm_free_frame(uintptr_t addr)`** — compute the frame number from the
  address and clear its bit in the bitmap.

### Modified files

**`as/boot.S`** — push `%ebx` then `%eax` before `call kernel_main`.

**`cfg/kernel.ld`** — add `kernel_phys_start` and `kernel_phys_end` symbol
assignments around the output sections.

**`src/kernel.c`** — update the `kernel_main` signature, add `#include
<multiboot.h>` and `#include <pmm.h>`, call `pmm_init(mbi)` before anything
else, then test allocations:

```c
pmm_init(mbi);
for (int i = 0; i < 5; i++) {
    uintptr_t f = pmm_alloc_frame();
    serial_printf("frame %d: 0x%lX\n", i, (unsigned long)f);
}
```

### Build and test

```
make clean && make iso && make run
```

All five addresses must be:
- Non-zero
- Divisible by 4096 (`addr & 0xFFF == 0`)
- At or above 0x100000 (not in the first 1 MiB)
- Outside the kernel image range `[kernel_phys_start, kernel_phys_end)`

---

## Concept check

Answer these before running `/review`:

1. The initialisation strategy starts by marking everything used, then marks
   usable regions free, then re-marks critical regions used. Why not do the
   apparently simpler reverse: start with everything free and only mark reserved
   regions used?

2. Frame number 1000 is in which `bitmap` word, which bit within that word, and
   at what physical address?

3. `addr` and `len` in the mmap entry are `uint64_t`. What goes wrong if you
   silently cast `addr` to `uint32_t` before checking whether to skip the entry,
   when `addr` is, say, `0x100010000` (just above 4 GiB)?

---

## Mutation exercise

**Part A:** In `pmm_init`, comment out the step that re-marks the kernel image
as used after the memory map walk. Allocate 64 frames in a loop. Observe whether
any returned address falls within `[kernel_phys_start, kernel_phys_end)`. Explain
what would happen at runtime if a future subsystem wrote to that frame. Restore
when done.

**Part B:** Comment out the `mark_used(0, 0x100000)` call (the first-1-MiB
reservation). Allocate frames until you get one below 0x100000. Print the
address and identify which hardware region it lies in (IVT, BDA, VGA buffer,
etc.). Explain why handing that frame to any allocator is a problem. Restore
when done.

When you are satisfied with the whole session, run `/review`.
