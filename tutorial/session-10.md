# Session 10 — Paging

## Goal

The hardware MMU translates every memory access through a two-level table before
it reaches physical memory. In this session you will build that table, identity-map
the first 4 MiB so the running kernel survives the transition, and enable paging
with one bit in CR0. You will also give the page-fault handler real teeth: CR2
and the error code decoded, printed, and halted — a concrete safety net for
every memory bug from here on.

---

## Why paging

Segmentation (session 3) gave you one flat region per privilege level. It cannot
express "this 4 KiB block is read-only," "that block is off-limits to user
mode," or "process A and process B each see their own private 4 GiB space."
Paging solves all three:

- Each 4 KiB page carries independent permission bits (present, writable,
  user-accessible).
- Each process gets its own page directory — load a different physical address
  into CR3 and the CPU sees a completely different virtual address space.
- The kernel can sit in the upper portion of every process's space, visible only
  from ring 0.

Segmentation is still active (i386 cannot disable it), but the flat GDT you
installed in session 3 (base = 0, limit = 4 GiB) reduces it to a no-op: every
linear address equals its virtual address. Paging works entirely in terms of
linear addresses.

---

## Address translation

Every load and store goes through a two-level lookup before reaching physical
memory:

```
Virtual address (32 bits)
 ┌────────────┬────────────┬──────────────┐
 │  dir index │  tbl index │ page offset  │
 │  [31:22]   │  [21:12]   │   [11:0]     │
 │  10 bits   │  10 bits   │   12 bits    │
 └────────────┴────────────┴──────────────┘
      │              │              │
      │              │              └─► byte within the 4 KiB page
      │              │
      ▼              ▼
  CR3 → page directory          page table
  (1024 × PDE)                  (1024 × PTE)
  PDE[dir index] ──────────►  PTE[tbl index]
                                     │
                                     ▼
                              physical page base
                              + page offset
                              = physical address
```

With 10 bits per level and a 12-bit offset:
- 2¹⁰ = 1024 entries per table, each entry 4 bytes → each table is exactly 4 KiB.
- 2¹² = 4096 bytes per page = 4 KiB.
- Each PDE covers 1024 × 4 KiB = 4 MiB of virtual address space.
- Full virtual space: 1024 PDEs × 4 MiB = 4 GiB.

Both the page directory and every page table must be **4 KiB-aligned** in
physical memory.

---

## Page directory entry (PDE)

| Bits  | Field | Meaning                                                              |
|-------|-------|----------------------------------------------------------------------|
| 31–12 | Addr  | Physical address of the page table (upper 20 bits; low 12 assumed 0)|
| 11–9  | AVL   | Available for OS use                                                 |
| 8     | —     | Ignored for PDEs (no G semantics at this level)                      |
| 7     | PS    | Page size: 0 = 4 KiB pages (normal); 1 = 4 MiB page (PSE, leave 0) |
| 6     | —     | Reserved, must be 0 when PS=0                                        |
| 5     | A     | Accessed — set by hardware on first use of this PDE                  |
| 4     | PCD   | Page cache disable                                                   |
| 3     | PWT   | Page write-through                                                   |
| 2     | U/S   | 0 = supervisor only; 1 = user-accessible                             |
| 1     | R/W   | 0 = read-only; 1 = read/write                                        |
| 0     | P     | Present — if 0, any access through this entry raises #PF             |

For kernel page tables: set P=1, R/W=1, U/S=0.

---

## Page table entry (PTE)

| Bits  | Field | Meaning                                                              |
|-------|-------|----------------------------------------------------------------------|
| 31–12 | Addr  | Physical address of the 4 KiB page                                   |
| 11–9  | AVL   | Available for OS use                                                 |
| 8     | G     | Global — TLB entry survives CR3 reload (requires CR4.PGE; leave 0)   |
| 7     | PAT   | Page attribute table index (leave 0)                                 |
| 6     | D     | Dirty — set by hardware on first write to this page                  |
| 5     | A     | Accessed — set by hardware on first read or write                    |
| 4     | PCD   | Page cache disable                                                   |
| 3     | PWT   | Page write-through                                                   |
| 2     | U/S   | 0 = supervisor only; 1 = user-accessible                             |
| 1     | R/W   | 0 = read-only; 1 = read/write                                        |
| 0     | P     | Present — if 0, access raises #PF                                    |

A PTE with P=0 — all other bits can carry OS-defined data (e.g., "swapped to
disk at block N"). The hardware ignores every other field when P=0.

---

## CR3 and CR0

**CR3** holds the physical address of the current page directory. Load it with:

```c
__asm__ volatile("mov %0, %%cr3" :: "r"(phys_addr_of_page_directory) : "memory");
```

Reloading CR3 flushes the entire TLB (except global entries). The lower 12 bits
of CR3 are control flags (PWT/PCD for the directory itself); set them to 0.

**CR0 bit 31 (PG)** enables paging. Bit 0 (PE, protected mode) must already be
set — it has been since session 3. Enable paging with a read-modify-write:

```c
uint32_t cr0;
__asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
cr0 |= 0x80000000;
__asm__ volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
```

The instruction immediately following `mov %eax, %cr0` executes with paging
active. The EIP must be covered by a valid PTE **before** this instruction
runs — if not, the CPU tries to fetch the next opcode, cannot translate the
address, and takes a page fault before any handler is reachable.

---

## Identity mapping

The simplest safe mapping is *identity*: virtual address N maps to physical
address N. After enabling paging nothing changes from the CPU's perspective;
physical and virtual addresses remain identical.

To cover the first 4 MiB you need exactly two structures:

1. **One page table** (`page_table_0[1024]`): PTE[i] = `(i * 4096) | P | R/W`
   for i = 0 to 1023.
2. **One PDE**: `page_directory[0]` points to `page_table_0`, with P | R/W set.

This covers virtual addresses `0x00000000`–`0x003FFFFF`, which includes:
- The BIOS/VGA region (0x00000–0xFFFFF)
- The kernel image (loaded at 1 MiB = 0x100000 by the linker script)
- The PMM bitmap and any data structures you have so far

4 MiB is enough for an initial flat kernel. Sessions 11 and 12 extend the
mappings as the kernel grows into virtual memory.

---

## The TLB

The hardware caches recent translations in the **Translation Lookaside Buffer
(TLB)**. When you modify a PTE or PDE, the old cached translation for the
affected address is stale and must be invalidated, or the CPU will use the old
mapping:

```c
__asm__ volatile("invlpg (%0)" :: "r"(virt_addr) : "memory");
```

`invlpg` invalidates exactly one page's TLB entry. Reloading CR3 flushes
everything. Use `invlpg` for single-page updates; CR3 reload for wholesale
replacements (e.g., address space switches).

---

## Page fault (#PF, vector 14)

A page fault fires when the MMU cannot complete a translation:
- PDE.P = 0 or PTE.P = 0 (page not present)
- Write to a page with R/W = 0
- User-mode access to a page with U/S = 0

**CR2** contains the linear (virtual) address that caused the fault. Read it
**before** doing anything else — any subsequent faulting memory access overwrites
CR2 with the new faulting address.

**Error code** (pushed by the CPU like other exception error codes; available
at `frame->error_code` in your existing handler):

| Bit | Name | 0                 | 1                                              |
|-----|------|-------------------|------------------------------------------------|
| 0   | P    | Not-present fault | Protection violation                           |
| 1   | W/R  | Fault on a read   | Fault on a write                               |
| 2   | U/S  | Supervisor mode   | User mode                                      |
| 3   | RSVD | —                 | Reserved bit set in a PDE/PTE (ignore for now) |

An error code of `0x00` (all zero) means: not-present, read, supervisor — the
most common case when a NULL pointer is dereferenced in kernel mode.

---

## What you need to implement

### New files

**`include/paging.h`**:

```c
#include <stdint.h>

#define PAGE_PRESENT  (1u << 0)
#define PAGE_WRITABLE (1u << 1)
#define PAGE_USER     (1u << 2)

void paging_init(void);
void map_page(uint32_t virt, uint32_t phys, uint32_t flags);
```

**`src/paging.c`** — three things to implement:

- **`static uint32_t page_directory[1024]`** and **`static uint32_t page_table_0[1024]`**
  — both declared at file scope with `__attribute__((aligned(4096)))`. They must
  not be stack-allocated; their addresses must be valid physical addresses (since
  you are identity-mapped, the C pointer value equals the physical address).

- **`void paging_init(void)`** — three steps in order:
  1. Fill `page_table_0`: each entry maps one 4 KiB frame starting at 0, with
     P and R/W set. Do not use `pmm_alloc_frame` here — the static array is
     already in place.
  2. Install `page_directory[0]` pointing to `page_table_0`, with P and R/W.
     Leave all other PDE entries zero (not present).
  3. Load `page_directory`'s address into CR3, then set CR0.PG as shown above.
     After this returns, paging is active.

- **`void map_page(uint32_t virt, uint32_t phys, uint32_t flags)`** — installs
  a mapping for a single 4 KiB page. Extract the directory index (bits [31:22])
  and table index (bits [21:12]) from `virt`. If the PDE for that directory index
  is not present, allocate a new page table frame with `pmm_alloc_frame`, zero
  it, and install the PDE. Write `(phys & ~0xFFFu) | flags | PAGE_PRESENT` into
  the correct PTE, then call `invlpg` on `virt`.

### Extending the page-fault handler

The IDT already has `isr14` wired up from session 4, and `exception_handler` in
`isr.c` already prints the vector and error code. Add a special case for vector
14 **before** the generic print:

```c
if (frame->vector == 14) {
    uint32_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    serial_printf("PAGE FAULT at 0x%lX\n", (unsigned long)cr2);
    serial_printf("  error: %s %s in %s mode\n",
        frame->error_code & 1 ? "protection violation" : "not-present",
        frame->error_code & 2 ? "write"                : "read",
        frame->error_code & 4 ? "user"                 : "supervisor");
    /* fall through to generic halt */
}
```

### Modified files

**`src/kernel.c`** — call `paging_init()` after `pmm_init(mbi)`. Then verify
paging is working by deliberately triggering a page fault with an unmapped
address:

```c
paging_init();

/* Trigger #PF to verify the handler works — remove before moving on */
volatile uint32_t *bad = (volatile uint32_t *)0xDEADB000;
(void)*bad;
```

Remove the deliberate fault once the handler output is confirmed.

### Build and test

```
make clean && make iso && make run
```

With the deliberate fault in place, expected serial output (before halt):

```
PAGE FAULT at 0xDEADB000
  error: not-present read in supervisor mode
```

Without the deliberate fault, the kernel should continue running normally — the
tick loop still prints, serial still works. That confirms the identity map is
correct: nothing that was working before paging broke after enabling it.

---

## Concept check

1. Why must the kernel be identity-mapped **before** the instruction that sets
   CR0.PG executes? Describe exactly what the CPU does on the very next
   instruction fetch if the identity map is not yet in place.

2. There are 1024 PDE entries, each covering 4 MiB. What is the maximum virtual
   address space, and what is the maximum physical address space on a plain
   i386 (without PAE)?

3. A page fault fires with error code `0x7` (binary `0b111`). Decode each bit
   and describe what the faulting access was and which rule it violated.

---

## Mutation exercise

**Part A:** After `paging_init` returns, clear the P bit of one PTE that the
kernel actively uses — for example, `page_table_0[1]` (the page at 0x1000).
Flush its TLB entry with `invlpg(0x1000)`. Then access address 0x1000. Confirm
the page-fault handler fires and reports P=0 (not-present). Note the error code
value. Restore when done.

**Part B:** After paging is enabled, mark the identity-mapped VGA buffer page
read-only by clearing R/W in its PTE (the VGA buffer is at 0xB8000 — which PTE
index does that fall in?). Flush the TLB entry. Then let `terminal_writestring`
write to the VGA buffer. Confirm the page fault fires. Which bits of the error
code tell you this is a protection violation on a write, not a not-present
fault? Restore when done.

When satisfied, run `/review`.
