# Session 1.1 — GAS Assembly Primer

## What this is

Not a build session. No new kernel code.

This is a reference you read after `boot.s` is working, to understand *why*
every line is written the way it is. The directives in `boot.s` — `.set`,
`.section`, `.align`, `.long`, `.skip`, `.global`, `.type`, `.size` — are not
instructions for the CPU. They are instructions for the **assembler and linker**.
That distinction is the key insight of this session.

---

## What an assembler does

The assembler's job is to turn a `.s` file into a **relocatable object file**
(`.o`). It does two things:

1. Translates mnemonics (`mov`, `call`, `hlt`) into their binary encodings
2. Records symbols and their addresses so the **linker** can connect them later

The `.o` file is not yet an executable. Addresses are not final — the linker
assigns those when it combines all `.o` files according to the linker script.
The assembler just produces a table of symbols and a bag of bytes with
placeholder addresses (called *relocation records*).

You can inspect any `.o` file with:
```
~/cross/opt/bin/i686-elf-objdump -d lib/boot.o      # disassemble
~/cross/opt/bin/i686-elf-objdump -t lib/boot.o      # symbol table
~/cross/opt/bin/i686-elf-readelf -S lib/boot.o      # section headers
```

---

## Directives vs. instructions

A **CPU instruction** becomes machine code: `hlt` becomes the byte `0xF4`,
`mov $0x10000, %esp` becomes a 5-byte encoding.

A **directive** (anything starting with `.`) produces no machine code. It
controls the assembler itself: which section to write into, how to align the
output, what symbols to export, and what metadata to embed.

---

## `.set` — named constants

```asm
.set MAGIC, 0x1BADB002
```

Defines `MAGIC` as an assembler-time constant. Wherever `MAGIC` appears
afterwards, the assembler substitutes the value. It is the assembly equivalent
of `#define` in C, but evaluated at assemble time, not preprocessor time.
`MAGIC` does not appear in the `.o` — only its substituted value does.

---

## `.section` — selecting the output section

```asm
.section .multiboot
.section .bss
.section .text
```

ELF object files are divided into **sections**. Each section has a name, a
type, and flags (executable? writable? allocated in memory?).

`.section name` switches the assembler's write pointer to that section.
Everything that follows — until the next `.section` directive — is placed there.

Common sections:
| Section   | Contents                        | In binary? | Writable? |
|-----------|---------------------------------|------------|-----------|
| `.text`   | Executable code                 | Yes        | No        |
| `.rodata` | Read-only constants             | Yes        | No        |
| `.data`   | Initialised read/write data     | Yes        | Yes       |
| `.bss`    | Zero-initialised data           | No *       | Yes       |

\* `.bss` takes **no space in the binary**. The linker records how many bytes to
reserve, and the loader zeroes that region at startup. This is why the stack
lives in `.bss` — a 16 KiB stack does not add 16 KiB to `kernel.elf`.

---

## `.align` / `.balign` — alignment padding

```asm
.align 4
```

Advances the location counter to the next multiple of 4 (or whatever power of
2 you specify), inserting zero bytes as padding if needed.

The Multiboot spec requires the header to be **4-byte aligned** within the
file. If the preceding section ended on an odd address, `.align 4` pads it to
the next 4-byte boundary before emitting the header words. Without it, GRUB
might not find the header even though it is within the first 8 KiB.

`.balign N` is the same but takes the byte count directly (`.balign 4` = align
to a 4-byte boundary). `.align N` on x86 GAS also means align to 2^N... wait,
no — on x86 GAS, `.align N` aligns to an N-byte boundary (not 2^N). This is a
platform-specific quirk; on ARM `.align 3` means align to 2^3 = 8 bytes. On
x86 GAS, `.balign 4` and `.align 4` both mean the same thing.

---

## `.long`, `.word`, `.byte` — emitting raw values

```asm
.long 0x1BADB002
```

Emits a 32-bit (4-byte) integer at the current location. No instruction — just
raw bytes written into the section.

| Directive | Size    |
|-----------|---------|
| `.byte`   | 1 byte  |
| `.word`   | 2 bytes |
| `.long`   | 4 bytes |
| `.quad`   | 8 bytes |

This is how the Multiboot header is written: three `.long` values placed in
`.section .multiboot`, at a 4-byte aligned address, within the first 8 KiB of
the binary. GRUB scans those bytes looking for the magic value.

---

## `.skip` — reserving space in BSS

```asm
.skip 16384
```

Reserves N bytes at the current location, filled with zeros. In `.bss` this
means: "tell the linker there are 16384 bytes here, but don't write them to
the binary." At runtime, the loader (or the kernel itself, later) zeroes this
region.

This is the correct way to declare the stack. A `16384`-byte `.skip` in `.bss`
adds zero bytes to `kernel.elf` but reserves the virtual address range.

---

## `.global` — exporting a symbol

```asm
.global _start
```

Marks `_start` as a **global symbol** — visible to the linker when it combines
object files. Without `.global`, the symbol is local to the `.o` file and the
linker cannot reference it from outside.

The linker script says `ENTRY(_start)`, which tells the linker "put the address
of `_start` in the ELF entry point field." GRUB reads that field to know where
to jump. For this chain to work, `_start` must be global.

---

## `.type` — ELF symbol type

```asm
.type _start, @function
```

Annotates the symbol `_start` in the ELF symbol table as a function (as opposed
to `@object` for data, or `@notype`).

This does **not** affect execution. It is metadata for tools:
- `objdump -t` will show `F` next to function symbols
- GDB uses it to infer stack frame layout
- Some linkers use it for PLT/GOT generation (not relevant here yet)

It is good practice to annotate, but the kernel will boot without it.

---

## `.size` — ELF symbol size, and the location counter `.`

```asm
.size _start, . - _start
```

This is the line you noticed. Break it down:

**`.`** — the **location counter**. At any point during assembly, `.` holds the
address of the *current output position* — where the next byte would be
written. It changes as the assembler processes each instruction and directive.

**`. - _start`** — at the point this directive is reached, `.` holds the address
*just past the last byte of `_start`'s body*. `_start` holds the address of
the first byte. Subtracting gives the number of bytes in the function.

**`.size _start, <expr>`** — records that size in the ELF symbol table.

Again, this does not change execution. But it matters for:
- `objdump -d` — knows where the function ends, formats disassembly correctly
- GDB — `info symbol <addr>` can tell you which function an address falls in
- Profilers, sanitizers, and other tools that walk ELF symbol tables

The idiom `. - label` ("current position minus the label's address") appears
constantly in assembly. You will use it again when computing sizes of data
tables, section lengths, and offsets.

---

## Local labels and back/forward references

```asm
1:  jmp 1b
```

`1:` defines a **local label** — a label with no name, just a number. Local
labels can be reused: you can have many `1:` labels in the same file.

`1b` means "the nearest `1:` label **b**ehind (before) the current position."
`1f` means "the nearest `1:` label **f**orward (after) the current position."

This idiom avoids cluttering the symbol table with throwaway loop labels. The
`jmp 1b` in `_start` is a tight infinite loop: "jump back to the most recent
`1:` label," which is one instruction behind. This is the conventional
assembly idiom for `while(1) {}`.

---

## Reading your binary

Three commands you should run on `out/kernel.elf` right now:

**Disassemble:**
```
~/cross/opt/bin/i686-elf-objdump -d out/kernel.elf
```
Shows each section's machine code decoded back to mnemonics. Verify `_start`
is at `0x100000` (1 MiB), the Multiboot header bytes appear before it, and the
`call` to `kernel_main` is there.

**Symbol table:**
```
~/cross/opt/bin/i686-elf-objdump -t out/kernel.elf
```
Lists every symbol: address, section, type (`F` for function), size, name.
Check that `_start` is marked `F`, its size is correct, and `stack_top` /
`stack_bottom` are present.

**Section headers:**
```
~/cross/opt/bin/i686-elf-readelf -S out/kernel.elf
```
Shows each section's virtual address, file offset, and size. Notice that `.bss`
has a non-zero size but zero file size — the stack exists in virtual memory but
not in the binary.

---

## There is nothing to implement

This session has no build task. Run the three commands above, read the output,
and make sure you can map each line back to something in `boot.s` or
`kernel.c`. That is the exercise.

When the output makes sense to you, you are done. Move on to the Session 1
concept check and mutation exercise.
