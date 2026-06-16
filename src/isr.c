/* isr.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <isr.h>

static const char *itr_names[32] = {"Divide Error",
                                    "Debug",
                                    "NMI",
                                    "Breakpoint",
                                    "Overflow",
                                    "Bound Range Exceeded",
                                    "Invalid Opcode",
                                    "Device Not Available",
                                    "Double Fault",
                                    "Coprocessor Segment Overrun",
                                    "Invalid TSS",
                                    "Segment not Present",
                                    "Stack-Segment Fault",
                                    "General Protection Fault",
                                    "Page Fault",
                                    "Reserved",
                                    "x87 FPU Error",
                                    "Alignment Check",
                                    "Machine Check",
                                    "Reserved",
                                    "Reserved",
                                    "Reserved",
                                    "Reserved",
                                    "Reserved",
                                    "Reserved",
                                    "Reserved",
                                    "Reserved",
                                    "Reserved",
                                    "Reserved",
                                    "Reserved",
                                    "Reserved",
                                    "Reserved"};

void exception_handler(struct interrupt_frame *frame __attribute__((unused))) {
    serial_printf("EXCEPTION: %s (#%x)\n", itr_names[frame->vector],
                  frame->vector);
    serial_printf("EIP: %X CS: %X EFLAGS: %X ERR: %X\n", frame->eip, frame->cs,
                  frame->eflags, frame->error_code);
    __asm__ volatile("cli");
    __asm__ volatile("1: hlt");
    __asm__ volatile("jmp 1b");
}
