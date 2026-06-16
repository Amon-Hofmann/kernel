/* idt.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <idt.h>
#include <stddef.h>

#define KERNEL_CODE_SELECTOR (0x0008)
#define IDT_INTERRUPT_GATE   (0x8E)
#define IDT_TRAP_GATE        (0x8F)

static idt_entry_t idt_entries[IDT_ENTRY_LEN];

static idt_ptr_t idt_ptr = {.limit = sizeof(idt_entries) - 1,
                            .base = (uint32_t)idt_entries};

void idt_set_gate(uint8_t vector, uint32_t handler, uint16_t selector,
                  uint8_t type_attr) {
    idt_entries[vector].offset_low = (handler & 0x0000FFFF);
    idt_entries[vector].selector = selector;
    idt_entries[vector].zero = 0;
    idt_entries[vector].type_attr = type_attr;
    idt_entries[vector].offset_high = (handler & 0xFFFF0000) >> 16;
}

void idt_flush(idt_ptr_t *idt_ptr);

void isr0(void);
void isr1(void);
void isr2(void);
void isr3(void);
void isr4(void);
void isr5(void);
void isr6(void);
void isr7(void);
void isr8(void);
void isr9(void);
void isr10(void);
void isr11(void);
void isr12(void);
void isr13(void);
void isr14(void);
void isr15(void);
void isr16(void);
void isr17(void);
void isr18(void);
void isr19(void);
void isr20(void);
void isr21(void);
void isr22(void);
void isr23(void);
void isr24(void);
void isr25(void);
void isr26(void);
void isr27(void);
void isr28(void);
void isr29(void);
void isr30(void);
void isr31(void);

void idt_install(void) {
    idt_set_gate(0, (uint32_t)isr0, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(1, (uint32_t)isr1, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(2, (uint32_t)isr2, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(3, (uint32_t)isr3, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(4, (uint32_t)isr4, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(5, (uint32_t)isr5, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(6, (uint32_t)isr6, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(7, (uint32_t)isr7, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(8, (uint32_t)isr8, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(9, (uint32_t)isr9, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(10, (uint32_t)isr10, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(11, (uint32_t)isr11, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(12, (uint32_t)isr12, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(13, (uint32_t)isr13, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(14, (uint32_t)isr14, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(15, (uint32_t)isr15, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(16, (uint32_t)isr16, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(17, (uint32_t)isr17, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(18, (uint32_t)isr18, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(19, (uint32_t)isr19, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(20, (uint32_t)isr20, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(21, (uint32_t)isr21, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(22, (uint32_t)isr22, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(23, (uint32_t)isr23, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(24, (uint32_t)isr24, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(25, (uint32_t)isr25, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(26, (uint32_t)isr26, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(27, (uint32_t)isr27, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(28, (uint32_t)isr28, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(29, (uint32_t)isr29, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(30, (uint32_t)isr30, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_set_gate(31, (uint32_t)isr31, KERNEL_CODE_SELECTOR, IDT_INTERRUPT_GATE);
    idt_flush(&idt_ptr);
}
