/* Multiboot header constants */
.set MAGIC,    0x1BADB002
.set FLAGS,    0
.set CHECKSUM, -(MAGIC + FLAGS)

/* Multiboot header — must appear in the first 8 KiB of the kernel image */
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* Stack space in BSS (zero-initialized, no space in binary) */
.section .bss
.align 16
stack_bottom:
.skip 16384         /* 16 KiB */
stack_top:

.section .text
.global _start
.type _start, @function
_start:
    mov $stack_top, %esp    /* point stack register at top (stack grows down) */
    cli                     /* disable interrupts */
    hlt                     /* halt the CPU */
1:  jmp 1b                  /* if somehow resumed, loop forever */
.size _start, . - _start
