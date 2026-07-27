# Session 11 — Kernel Heap

## Goal

Sessions 9 and 10 gave you two primitives: `pmm_alloc_frame()` allocates a
4 KiB physical frame, and `map_page()` wires a virtual address to a physical
frame. Neither can satisfy "give me 24 bytes for a struct." In this session you
build the allocator that bridges the gap: `kmalloc(size)` returns a pointer to
`size` usable bytes, and `kfree(ptr)` returns it to a free pool.

---

## Why not use `pmm_alloc_frame` directly?

`pmm_alloc_frame` hands out whole 4 KiB pages. Every allocation of a
`uint32_t` would waste 4092 bytes. Worse, you cannot partially free a frame —
the PMM has no concept of "the first 24 bytes of frame N are used; the rest
are free."

A heap allocator sits one layer above the PMM. It requests whole frames from
the PMM when it needs more virtual address space, then parcels that space out
in sub-page units. It also tracks which ranges are free so they can be reused.

---

## Virtual heap region

Pick a fixed virtual address range for the heap. The identity map covers
`0x00000000`–`0x003FFFFF` (PDE[0], session 10). Place the heap immediately
above it so there is no overlap:

```c
#define KHEAP_VIRT_START 0x00400000u   /* 4 MiB, first address above identity map */
#define KHEAP_MAX_SIZE   0x00400000u   /* cap at 4 MiB of heap virtual space */
```

At startup the heap has zero mapped pages. As it needs more space it extends
itself by calling `pmm_alloc_frame()` and `map_page()` to wire new physical
frames into the virtual range starting at `KHEAP_VIRT_START`.

```
Virtual address space
────────────────────────────────────────────────────────────
0x00000000  ─────── identity-mapped (session 10) ───────────
            page_directory[0] → page_table_0
0x003FFFFF  ────────────────────────────────────────────────
0x00400000  ─────── KHEAP_VIRT_START ───────────────────────
            page_directory[1] → heap page table (new PDE)
            PTE[0] → physical frame A   ← heap_start
            PTE[1] → physical frame B
            PTE[2] → physical frame C
            PTE[3] → physical frame D   ← after 4 init pages
            PTE[4..] not yet mapped
0x00800000  ─────── KHEAP_VIRT_START + KHEAP_MAX_SIZE ──────
```

The PMM may hand out frames A–D from anywhere in physical memory — they need
not be contiguous physically. `map_page` makes them contiguous in virtual
space, which is all the allocator cares about.

---

## Block header

Every allocation is preceded in memory by a **block header**. The headers form
a doubly-linked list ordered by ascending virtual address:

```c
#define KHEAP_MAGIC 0xDEADC0DEu

typedef struct heap_block {
    uint32_t           magic;  /* KHEAP_MAGIC — detects header corruption   */
    bool               used;   /* true = allocated, false = free             */
    size_t             size;   /* usable bytes immediately after this header */
    struct heap_block *next;   /* next block (higher address), or NULL       */
    struct heap_block *prev;   /* previous block (lower address), or NULL    */
} heap_block_t;
```

**Verify the size before using it.** The compiler inserts padding between
`bool used` and `size_t size` to satisfy alignment requirements. On i386 this
struct is **20 bytes**, not the 17 a naive field count suggests. Confirm:

```c
serial_printf("sizeof(heap_block_t) = %u\n", (unsigned)sizeof(heap_block_t));
```

The pointer returned to the caller is `block + 1` — one struct-stride past the
header, i.e., `(uint8_t *)block + sizeof(heap_block_t)`. With
`sizeof(heap_block_t) == 20` and all usable sizes rounded to multiples of 4,
every returned pointer is 4-byte aligned, satisfying the i386 ABI requirement
for all standard C types.

Round every requested size before storing it in `block->size`:

```c
size = (size + 3u) & ~3u;
```

### Memory layout

After `kheap_init` maps 4 pages (16 KiB) and `kmalloc(64)` is called once:

```
0x00400000 (heap_start)
┌─────────────────────────────────────────┐ ← block A (used)
│ heap_block_t  (20 B)                    │
│   magic = 0xDEADC0DE                    │
│   used  = true                          │
│   size  = 64                            │
│   prev  = NULL                          │
│   next  ─────────────────────────────►  │
├─────────────────────────────────────────┤ 0x00400014  ← returned to caller
│ usable region  (64 B)                   │
└─────────────────────────────────────────┘
0x00400054
┌─────────────────────────────────────────┐ ← block B (free)
│ heap_block_t  (20 B)                    │
│   magic = 0xDEADC0DE                    │
│   used  = false                         │
│   size  = 16364 - 64 - 20 = 16280      │   (16 KiB minus A's header+usable and B's header)
│   prev  ◄─────────────────────────────  │ (back to A)
│   next  = NULL                          │
├─────────────────────────────────────────┤ 0x00400068  ← returned next time, if used
│ free region  (16280 B)                  │
└─────────────────────────────────────────┘
0x00404000 (heap_end, 4 pages mapped)
```

Total accounting: 20 + 64 + 20 + 16280 = 16384 = 4 × 4096 ✓

---

## Splitting a free block

When `kmalloc` finds a free block large enough for the request, it should
**split** the block into two if the leftover portion is large enough to be
useful. The split threshold: the leftover must be at least
`sizeof(heap_block_t) + 4` bytes — a full header plus a minimum usable region.
Below that threshold, give the entire block to the caller (slightly wasteful but
avoids orphaned slivers that can never satisfy a request).

Before split (free block, `size = 200`, requesting `size = 64`):

```
┌────────────────────────────────┐
│ header (size=200, used=false)  │
├────────────────────────────────┤
│     200 bytes free             │
└────────────────────────────────┘
```

After split (leftover = 200 − 64 − 20 = 116 bytes, above threshold):

```
┌────────────────────────────────┐
│ header (size=64, used=true)    │
├────────────────────────────────┤
│     64 bytes  ← returned       │
└────────────────────────────────┘
┌────────────────────────────────┐
│ header (size=116, used=false)  │
├────────────────────────────────┤
│     116 bytes free             │
└────────────────────────────────┘
```

Update `next` and `prev` links in all affected blocks when inserting the new
free block into the list.

---

## Coalescing on free

Freeing a block without merging adjacent free blocks leads to fragmentation:
many small free gaps that cannot satisfy large requests even though the total
free bytes are sufficient. When `kfree` marks a block free, merge it with any
immediately adjacent free neighbours:

1. **Merge with next:** if `block->next != NULL && !block->next->used`, absorb
   `block->next` into `block`. The new `size` is
   `block->size + sizeof(heap_block_t) + block->next->size`. Update `next` to
   skip the merged block.

2. **Merge with prev:** if `block->prev != NULL && !block->prev->used`, absorb
   `block` into `block->prev`. The `prev` block's `size` grows by
   `sizeof(heap_block_t) + block->size`. Update `prev->next` to skip `block`.

Always merge with `next` first. If you merge with `prev` first and then check
`next`, you may dereference a pointer into a block that has already been
absorbed.

---

## Heap extension

When `kmalloc` scans the entire list and finds no block large enough, call
`heap_extend` to grow the heap:

1. Check that `heap_end + n_pages * PAGE_SIZE <= KHEAP_VIRT_START + KHEAP_MAX_SIZE`.
   If not, the heap is exhausted — print a message and halt.
2. For each of the `n_pages` pages:
   - `page = pmm_alloc_frame()`
   - `map_page(heap_end, page, PAGE_PRESENT | PAGE_WRITABLE)`
   - `heap_end += PAGE_SIZE`
3. Place a new free block at the start of the newly mapped region. Its `size`
   is `n_pages * PAGE_SIZE - sizeof(heap_block_t)`. Link it to the tail of the
   block list.
4. If the tail block was already free before extension, coalesce rather than
   creating a new header — this avoids a permanent gap at every extension
   boundary.

---

## What you need to implement

### New files

**`include/kheap.h`**:

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KHEAP_VIRT_START 0x00400000u
#define KHEAP_MAX_SIZE   0x00400000u
#define KHEAP_MAGIC      0xDEADC0DEu
#define KHEAP_INIT_PAGES 4u

typedef struct heap_block {
    uint32_t           magic;
    bool               used;
    size_t             size;
    struct heap_block *next;
    struct heap_block *prev;
} heap_block_t;

void  kheap_init(void);
void *kmalloc(size_t size);
void  kfree(void *ptr);
```

**`src/kheap.c`** — implement the following:

- **`static uintptr_t heap_end`** — tracks how far the virtual heap has been
  mapped. Initialised to `KHEAP_VIRT_START`, advanced by `PAGE_SIZE` per frame
  added by `heap_extend`.

- **`static heap_block_t *heap_head`** — pointer to the first block in the
  list. `NULL` before `kheap_init` is called.

- **`static void heap_extend(size_t n_pages)`** — grows the heap by mapping
  `n_pages` new physical frames at `heap_end`. After mapping, place (or
  coalesce) a free block spanning the new space. See the extension rules above.

- **`void kheap_init(void)`** — set `heap_end = KHEAP_VIRT_START`, set
  `heap_head = NULL`, call `heap_extend(KHEAP_INIT_PAGES)`.

- **`void *kmalloc(size_t size)`** — round `size` up to the next multiple of
  4. Walk from `heap_head`, looking for the first free block with
  `block->size >= size`. If found, apply the split logic and mark the block
  used. Return `(void *)(block + 1)`. If the walk reaches the end with no
  suitable block, call `heap_extend(1)` and retry. If the heap is exhausted,
  print a message to serial and halt.

- **`void kfree(void *ptr)`** — recover the header:
  `heap_block_t *block = (heap_block_t *)ptr - 1`. Verify
  `block->magic == KHEAP_MAGIC` and `block->used == true` — halt with a serial
  message if either check fails (double-free or bad pointer). Mark the block
  free. Coalesce with `next`, then with `prev`.

### Modified files

**`src/kernel.c`** — call `kheap_init()` after `paging_init()`, then run:

```c
kheap_init();

uint32_t *a = kmalloc(sizeof(uint32_t) * 4);   /* 16 bytes */
uint32_t *b = kmalloc(100);
char     *s = kmalloc(32);

serial_printf("a=0x%lX  b=0x%lX  s=0x%lX\n",
              (unsigned long)a, (unsigned long)b, (unsigned long)s);

kfree(b);
uint32_t *c = kmalloc(80);   /* should reuse b's slot */
serial_printf("c=0x%lX  (expect ~= b)\n", (unsigned long)c);

kfree(a);
kfree(c);
kfree(s);
serial_writestring("heap test done\n");
```

### Build and test

```
make clean && make iso && make run
```

Expected: `a`, `b`, `s` are non-zero pointers inside
`[KHEAP_VIRT_START, KHEAP_VIRT_START + KHEAP_MAX_SIZE)`. `c` should land at
or near `b`'s address — the freed slot is reused. The kernel continues
running after the test (no page fault, no halt).

---

## Concept check

1. The struct `heap_block_t` has fields of sizes 4, 1, 4, 4, 4 bytes. A naive
   sum gives 17 bytes, yet `sizeof(heap_block_t)` returns 20 on i386. Explain
   exactly where the 3 bytes of padding are inserted by the compiler, and why
   they are necessary.

2. In `kfree`, the header is recovered with `(heap_block_t *)ptr - 1`. Because
   `ptr` is cast to `heap_block_t *` before the subtraction, C pointer
   arithmetic moves back by `sizeof(heap_block_t)` bytes, not 1 byte.
   If `sizeof(heap_block_t) == 20` and `kmalloc(32)` returned `0x00400014`,
   what numerical address does `(heap_block_t *)0x00400014 - 1` evaluate to?
   Confirm it matches the header address from the layout diagram above.

3. Suppose coalescing is removed from `kfree`. Describe a concrete allocation
   sequence — specific sizes, in order — that leaves the heap unable to satisfy
   a `kmalloc(64)` even though the total free bytes in the heap exceed 64.

---

## Mutation exercise

**Part A:** Remove the split logic from `kmalloc` — when a free block is large
enough, give the entire block to the caller without splitting a remainder. Keep
`KHEAP_INIT_PAGES = 4` (16 KiB initial heap). Allocate 8-byte blocks in a loop
until `kmalloc` extends the heap. Count how many allocations fit in the initial
16 KiB with and without splitting. Explain the difference in terms of internal
fragmentation.

**Part B:** Comment out the coalescing step in `kfree`. Allocate four 64-byte
blocks (`a`, `b`, `c`, `d`). Free `a` and `b` (which are adjacent in the heap).
Then call `kmalloc(100)`. Observe that it cannot satisfy the request from the
freed space even though `a` and `b` together provide 128 bytes of usable
space plus a recoverable header. Restore coalescing and verify `kmalloc(100)`
now reuses that space without extending the heap.

When satisfied, run `/review`.
