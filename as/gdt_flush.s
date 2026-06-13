.set KERNEL_DATA_SELECTOR, 0x0010
.set KERNEL_CODE_SELECTOR, 0x0008

.section .text
.global gdt_flush
.type gdt_flush, @function

gdt_flush:
    movl 4(%esp), %eax
    lgdt (%eax)
    ljmp $KERNEL_CODE_SELECTOR, $1f

1:  mov $KERNEL_DATA_SELECTOR, %eax 
    mov %eax, %ds
    mov $KERNEL_DATA_SELECTOR, %eax 
    mov %eax, %es
    mov $KERNEL_DATA_SELECTOR, %eax 
    mov %eax, %fs
    mov $KERNEL_DATA_SELECTOR, %eax 
    mov %eax, %gs
    mov $KERNEL_DATA_SELECTOR, %eax 
    mov %eax, %ss
    ret
.size gdt_flush, . - gdt_flush
