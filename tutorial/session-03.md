# Session 03 — GDT: Segments & Privilege Rings

## Goal

Replace the GDT that GRUB installed with one you own and understand. Build a
five-entry GDT — null, kernel code, kernel data, user code, user data — load it
with `lgdt`, and flush the segment registers. The kernel continues to run
exactly as before, but now on a descriptor table you control.

---

## Why you need your own GDT

When GRUB hands off to `_start`, the CPU is in protected mode and a GDT is
already loaded. You did not install it. You do not know its layout. You cannot
make assumptions about which selectors it provides, what privilege levels it
assigns, or how much memory each segment covers.

Installing your own GDT is not optional: every kernel operation that depends
on segment-level protection, privilege transitions, or the segment descriptor
cache must start from a known GDT. Do it once, early in `kernel_main`, before
anything else.

---

## Segmentation in protected mode

In real mode the CPU computes physical addresses as `segment * 16 + offset`.
Protected mode replaces that flat multiply with a **descriptor table lookup**.

When you write `mov %eax, (%ebx)`, the CPU does not use `%ebx` as a physical
address directly. Instead it:

1. Takes the **segment register** that applies to the access (for data: `%ds`
   by default)
2. Reads that register's 16-bit **selector** value
3. Looks up the **descriptor** in the GDT at the index the selector specifies
4. Reads the **base address** and **limit** from the descriptor
5. Adds the base to the offset (`%ebx`) to form the linear address
6. Checks that the offset is within the limit

A descriptor fully specifies a segment: where it starts in memory (base), how
large it is (limit), what type it is (code or data), and who is allowed to use
it (privilege level).

For now you will use a **flat memory model**: base = 0, limit = 4 GiB for all
segments. Segmentation then adds zero to every address, which means effective
addresses equal physical addresses. This simplifies everything while satisfying
the CPU's requirement that a GDT is present.

---

## Segment descriptor format

Each GDT entry is exactly **8 bytes**. The fields are split across the bytes
in a non-obvious way — a historical artifact of the 286-to-386 transition.

```
 63      56 55    52 51   48 47      40 39      32
 +---------+--------+-------+---------+---------+
 | base    | flags  | limit | access  | base    |
 | [31:24] |        |[19:16]|  byte   | [23:16] |
 +---------+--------+-------+---------+---------+

 31                 16 15                  0
 +--------------------+--------------------+
 |    base [15:0]     |    limit [15:0]    |
 +--------------------+--------------------+
```

As a C struct this is:

```c
struct gdt_entry {
    uint16_t limit_low;   /* limit bits 15:0  */
    uint16_t base_low;    /* base  bits 15:0  */
    uint8_t  base_mid;    /* base  bits 23:16 */
    uint8_t  access;      /* see access byte below */
    uint8_t  granularity; /* flags[7:4] | limit[19:16][3:0] */
    uint8_t  base_high;   /* base  bits 31:24 */
} __attribute__((packed));
```

`__attribute__((packed))` prevents GCC from inserting alignment padding.
Without it the struct would be 12 bytes instead of 8 and the CPU would read
garbage.

### The access byte (offset 5)

| Bit | Name | Meaning |
|-----|------|---------|
| 7   | P    | Present — must be 1 for the segment to be valid |
| 6:5 | DPL  | Descriptor Privilege Level — 00=ring 0, 11=ring 3 |
| 4   | S    | 1 = code/data segment; 0 = system segment (gates, TSS, LDT) |
| 3   | E    | Executable — 1 = code segment, 0 = data segment |
| 2   | C/D  | Code: Conforming; Data: Direction (0 = grows up) |
| 1   | R/W  | Code: Readable; Data: Writable |
| 0   | A    | Accessed — the CPU sets this bit on first access; set to 0 |

### The granularity byte (offset 6)

The upper nibble carries flags; the lower nibble carries limit bits 19:16.

| Bit | Name | Meaning |
|-----|------|---------|
| 7   | G    | Granularity — 0 = limit in bytes, 1 = limit in 4 KiB pages |
| 6   | D/B  | Default operation size — 1 = 32-bit segment, 0 = 16-bit |
| 5   | L    | 64-bit code segment — always 0 for 32-bit segments |
| 4   | AVL  | Available — ignored by hardware, can be 0 |
| 3:0 | —    | Limit bits 19:16 |

To cover 4 GiB with G=1: limit = 0xFFFFF (20 bits all ones). With G=1, the
CPU multiplies the limit by 4096, giving a byte limit of
`(0xFFFFF + 1) × 4096 = 4 GiB`.

For a 32-bit flat segment the granularity byte is `0xCF`:
- upper nibble = `1100` (G=1, D/B=1, L=0, AVL=0)
- lower nibble = `1111` (limit[19:16] = 0xF)

---

## The null descriptor

The very first entry in the GDT — index 0, selector `0x0000` — must be all
zeros. This is the **null descriptor**. It is mandatory: the x86 specification
requires it.

If code ever loads a segment register with selector 0 and then uses it to
access memory, the CPU raises a General Protection Fault (#GP). This is by
design — selector 0 is a sentinel for "no segment loaded." Uninitialized
segment registers often hold 0; the null descriptor turns those into
catchable faults rather than silent memory corruption.

---

## Rings and privilege levels

The i386 supports four privilege levels, called **rings**, numbered 0 to 3.
Ring 0 is the most privileged; ring 3 is the least.

- **Ring 0 (kernel mode):** Full hardware access. Can execute privileged
  instructions (`lgdt`, `lidt`, `cli`, `sti`, port I/O). The kernel runs here.
- **Ring 3 (user mode):** Restricted. Privileged instructions cause a #GP.
  User programs run here.
- **Rings 1 and 2:** Historically intended for device drivers; almost never
  used on x86. You will use only rings 0 and 3.

The DPL field in a segment descriptor specifies the minimum privilege required
to use that segment. A ring-3 process trying to load a ring-0 segment selector
into a segment register gets a #GP.

---

## Segment selectors

A segment register holds a 16-bit **selector**:

```
 15                  3    2   1:0
 +--------------------+----+-----+
 |    GDT index       | TI | RPL |
 +--------------------+----+-----+
```

| Field | Bits | Meaning |
|-------|------|---------|
| Index | 15:3 | Index into the GDT (or LDT if TI=1) |
| TI    | 2    | Table Indicator — 0 = GDT, 1 = LDT |
| RPL   | 1:0  | Requested Privilege Level |

Examples for a flat kernel with five descriptors starting at index 0:

| Segment     | Index | TI | RPL | Selector |
|-------------|-------|----|-----|---------|
| Null        | 0     | 0  | 0   | `0x0000` |
| Kernel code | 1     | 0  | 0   | `0x0008` |
| Kernel data | 2     | 0  | 0   | `0x0010` |
| User code   | 3     | 0  | 3   | `0x001B` |
| User data   | 4     | 0  | 3   | `0x0023` |

User selectors have RPL = 3 set in their low two bits.

---

## The segment descriptor cache

Each segment register has a hidden **descriptor cache** (also called the
shadow register). When you load a selector into a segment register, the CPU
looks up the descriptor in the GDT and copies the base, limit, and access
rights into this cache. Subsequent memory accesses use the cached values —
not the GDT — for speed.

This is why you cannot just call `lgdt` and continue. After `lgdt` updates
the GDTR register to point to your new table, the descriptor caches still
hold the old values. `cs` especially will still have the old kernel-code
descriptor cached.

A **far jump** forces the CPU to reload the `cs` cache from your new GDT.
A far jump specifies both a segment selector and an offset: `ljmp $selector, $label`.
The CPU fetches the descriptor for `$selector` from the new GDT, reloads the
`cs` cache, and resumes execution at `$label`.

The other segment registers (`ds`, `es`, `fs`, `gs`, `ss`) are flushed by
simply loading them with `mov`.

---

## What you need to implement

### New files

**`include/gdt.h`** — declare:
- `struct gdt_entry` (8 bytes, packed) with the five fields in order
- `struct gdt_ptr` (6 bytes, packed): a `uint16_t limit` and `uint32_t base` —
  this is the operand that `lgdt` takes
- `void gdt_install(void)` — sets up the GDT and loads it

**`src/gdt.c`** — implement:
- A static array of five `struct gdt_entry` — the GDT itself
- A static `struct gdt_ptr` pointing at the array, with `limit = sizeof(gdt) - 1`
- A helper `gdt_set_gate(size_t index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)` that fills in one entry. 
  `flags` is the 4-bit flag nibble (G, D/B, L, AVL) only — the function extracts `limit[19:16]` from `limit` itself and combines them: `granularity_byte = (flags << 4) | ((limit >> 16) & 0x0F)`
- `gdt_install`: calls `gdt_set_gate` five times (null, kernel code, kernel data,
  user code, user data), then calls `gdt_flush(&gdt_ptr)`

**`as/gdt_flush.s`** — implement `gdt_flush`:
- Takes a pointer to `struct gdt_ptr` as its first argument (`4(%esp)`)
- Loads it with `lgdt`
- Does a far jump to the kernel code selector to flush `cs`
- Loads all other segment registers (`ds`, `es`, `fs`, `gs`, `ss`) with the kernel data selector
- Returns

Segment values for a 32-bit flat model (base=0, limit=4GiB):
- Kernel code access byte: P=1, DPL=0, S=1, E=1, C=0, R=1, A=0
- Kernel data access byte: P=1, DPL=0, S=1, E=0, D=0, W=1, A=0
- User code access byte: same as kernel code but DPL=3
- User data access byte: same as kernel data but DPL=3
- Granularity byte for all: G=1, D/B=1, L=0, AVL=0, limit[19:16]=0xF → `0xCF`
- Null descriptor: all zero

Work out the numeric values for each access byte from the bit table above.

### Modify existing files

**`src/kernel.c`** — add `gdt_install()` as the very first call in `kernel_main`,
before `terminal_initialize()`.

**`makefile`** — check that `as/gdt_flush.s` will be compiled and linked.
The existing makefile may already pick up new `.s` files automatically — verify
this before adding manual rules.

### Build and test

```
make clean && make iso && make run
```

The kernel should behave identically to before: `Hello, Kernel!` appears and
the terminal scrolls normally. The GDT change is invisible to the user but
observable with GDB.

To verify the GDT is loaded correctly:

```
make debug
(gdb) break gdt_install
(gdb) continue
(gdb) finish                  # run gdt_install to completion
(gdb) info registers          # check cs, ds, ss
(gdb) x/5xg &gdt              # dump 5 giant (8-byte) words from gdt array
```

The `cs` register should be `0x0008`. The data segment registers should be
`0x0010`. The five 64-bit words in the GDT should match what you expect for
null, kernel code, kernel data, user code, user data.

Reference: i386 Programmer's Reference Manual, Chapter 3 (Protected Mode
Memory Management), sections 3.1–3.4.

---

## Concept check

Answer these before running `/review`:

1. After you call `lgdt`, why must you do a far jump before using the new
   code segment descriptor? What specific hardware mechanism makes this
   necessary?

2. What is the null descriptor and what happens if code loads a segment
   register with selector `0x0000` and then accesses memory through it?

3. A segment selector has an RPL field and a segment descriptor has a DPL
   field. The executing code also has a CPL (stored in `cs` bits 1:0).
   What is the difference between RPL and CPL, and which one does the CPU
   check when deciding whether to allow access to a segment?

---

## Mutation exercise

Do this after the GDT loads cleanly and you have verified it in GDB.

Add a sixth descriptor — call it whatever you like — with:
- base = 0
- limit = 0 (only one page with G=1 — or zero bytes with G=0; your choice)
- access = kernel data access byte

In `kernel_main`, after `gdt_install`, attempt to write to an address beyond
the limit of that segment by temporarily loading it into `ds` and accessing
memory. Observe what happens. Then answer:

- Which exception fires? (Consult Chapter 9 of the i386 manual for the
  exception list.)
- At what point does the fault occur — on the `mov` that loads the selector,
  or on the subsequent memory access?
- Why does the CPU detect the violation at that point rather than earlier or
  later?

Restore the GDT to its original five descriptors when you are done.

When you are satisfied with the whole session, run `/review`.
