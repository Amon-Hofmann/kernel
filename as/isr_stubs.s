
.macro ISR_NOERR n
.global isr\n
.type isr\n, @function
isr\n:
    push $0        # dummy error code
    push $\n       # vector number
    jmp isr_common
.size isr\n, . - isr\n
.endm

.macro ISR_ERR n
.global isr\n
.type isr\n, @function
isr\n:
    push $\n       # CPU already pushed error code
    jmp isr_common
.size isr\n, . - isr\n
.endm

.set KERNEL_DATA_SELECTOR, 0x0010
.section .text

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR 8
ISR_NOERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

.global isr_common
.type isr_common, @function

isr_common:
    pusha
    mov $KERNEL_DATA_SELECTOR, %eax 
    mov %eax, %ds
    mov %eax, %es
    mov %eax, %fs
    mov %eax, %gs
    push %esp
    call exception_handler
    add $4, %esp
    popa
    add $8, %esp
    iret
.size isr_common, . - isr_common


.macro IRQ_STUB vector, irq
.global irq\irq
.type isr\irq, @function

irq\irq:
    push $\irq      # which IRQ line (0-15), not the raw vector
    push $\vector   # the IDT vector (32-47)
    jmp irq_common
.size irq\irq, . - irq\irq
.endm

.section .text
IRQ_STUB 32, 0
IRQ_STUB 33, 1
IRQ_STUB 34, 2
IRQ_STUB 35, 3
IRQ_STUB 36, 4
IRQ_STUB 37, 5
IRQ_STUB 38, 6
IRQ_STUB 39, 7
IRQ_STUB 40, 8
IRQ_STUB 41, 9
IRQ_STUB 42, 10
IRQ_STUB 43, 11
IRQ_STUB 44, 12
IRQ_STUB 45, 13
IRQ_STUB 46, 14
IRQ_STUB 47, 15

.global irq_common
.type irq_common, @function

irq_common:
    pusha
    mov $KERNEL_DATA_SELECTOR, %eax 
    mov %eax, %ds
    mov %eax, %es
    mov %eax, %fs
    mov %eax, %gs
    push %esp
    call irq_handler
    add $4, %esp
    popa
    add $8, %esp
    iret
.size irq_common, . - irq_common
