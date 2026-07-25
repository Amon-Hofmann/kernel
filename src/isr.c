/* isr.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <io.h>
#include <isr.h>
#include <kernel_common.h>
#include <keyboard.h>
#include <pic.h>
#include <pit.h>
#include <serial.h>

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

KERNEL_COLD KERNEL_NORETURN void exception_handler(struct interrupt_frame *frame) {
    serial_printf("EXCEPTION: %s (#%lx)\n", itr_names[frame->vector],
                  (unsigned long)frame->vector);
    serial_printf("EIP: %lX CS: %lX EFLAGS: %lX ERR: %lX\n", (unsigned long)frame->eip,
                  (unsigned long)frame->cs, (unsigned long)frame->eflags,
                  (unsigned long)frame->error_code);
    __asm__ volatile("cli\n\t1: hlt\n\tjmp 1b");
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

KERNEL_HOT void irq_handler(struct interrupt_frame *frame) {
    switch (frame->error_code) {
        case 1:
            keyboard_handler(
                port_io_read_byte(0x60));  // drain i8042 output buffer so IRQ1 deasserts
            break;
        case 0:
            pit_inc_ticks();
            break;
        default:
            serial_printf("IRQ: %s (#%lx)\n", irq_names[frame->error_code],
                          (unsigned long)frame->error_code);
            serial_printf("EIP: %lX CS: %lX EFLAGS: %lX ERR: %lX\n",
                          (unsigned long)frame->eip, (unsigned long)frame->cs,
                          (unsigned long)frame->eflags, (unsigned long)frame->error_code);
            break;
    }
    pic_send_eoi(frame->error_code);
}
