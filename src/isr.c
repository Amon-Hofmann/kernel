/* isr.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <isr.h>
#include <kernel_common.h>
#include <pic.h>
#include <serial.h>
#include <stdbool.h>

static const char *const itr_names[32] = {"Divide Error",
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

_COLD _NORETURN void exception_handler(struct interrupt_frame *frame) {
    serial_printf("EXCEPTION: %s (#%lx)\n", itr_names[frame->vector],
                  (unsigned long)frame->vector);
    serial_printf("EIP: %lX CS: %lX EFLAGS: %lX ERR: %lX\n",
                  (unsigned long)frame->eip, (unsigned long)frame->cs,
                  (unsigned long)frame->eflags,
                  (unsigned long)frame->error_code);
    __asm__ volatile("cli");
    __asm__ volatile("1: hlt");
    __asm__ volatile("jmp 1b");
    __builtin_unreachable();
}

static const char *const irq_names[16] = {"System Timer (PIT)",
                                          "Keyboard",
                                          "-- (cascade to slave) --",
                                          "Serial port (COM2 / COM4)",
                                          "Serial port (COM1 / COM3)",
                                          "Sound card / LPT2",
                                          "Floppy disk controller",
                                          "Parallel port (LPT1)",
                                          "Real-time clock (RTC)",
                                          "Available",
                                          "Available",
                                          "Available",
                                          "PS/2 mouse",
                                          "FPU / coprocessor",
                                          "Primary ATA",
                                          "Secondary ATA"};
_HOT void irq_handler(struct interrupt_frame *frame) {
    serial_printf("IRQ: %s (#%lx)\n", irq_names[frame->error_code],
                  (unsigned long)frame->error_code);
    serial_printf("EIP: %lX CS: %lX EFLAGS: %lX ERR: %lX\n",
                  (unsigned long)frame->eip, (unsigned long)frame->cs,
                  (unsigned long)frame->eflags,
                  (unsigned long)frame->error_code);
    pic_send_eoi(frame->error_code);
}
