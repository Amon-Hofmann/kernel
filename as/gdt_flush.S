#include <selectors.h>

.section .text
.global gdt_flush
.type gdt_flush, @function

gdt_flush:
    movl 4(%esp), %eax
    lgdt (%eax)
    ljmp $KERNEL_CODE_SELECTOR, $1f

1:  mov $KERNEL_DATA_SELECTOR, %eax 
    mov %eax, %ds
    mov %eax, %es
    mov %eax, %fs
    mov %eax, %gs
    mov %eax, %ss
    ret
.size gdt_flush, . - gdt_flush
