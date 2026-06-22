/* isr.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <isr.h>
#include <pic.h>

inline void sti(void) {
    __asm__ volatile("sti");
}

inline void cli(void) {
    __asm__ volatile("cli");
}

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

void exception_handler(struct interrupt_frame *frame) {
    serial_printf("EXCEPTION: %s (#%x)\n", itr_names[frame->vector],
                  frame->vector);
    serial_printf("EIP: %X CS: %X EFLAGS: %X ERR: %X\n", frame->eip, frame->cs,
                  frame->eflags, frame->error_code);
    __asm__ volatile("cli");
    __asm__ volatile("1: hlt");
    __asm__ volatile("jmp 1b");
}

static const char *irq_names[32] = {"System Timer (PIT)",
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
void irq_handler(struct interrupt_frame *frame) {
    serial_printf("IRQ: %s (#%x)\n", irq_names[frame->error_code],
                  frame->error_code);
    serial_printf("EIP: %X CS: %X EFLAGS: %X ERR: %X\n", frame->eip, frame->cs,
                  frame->eflags, frame->error_code);
    pic_send_eoi(frame->error_code);
}
