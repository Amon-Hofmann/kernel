/* kernel.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <cpu.h>
#include <gdt.h>
#include <idt.h>
#include <kernel.h>
#include <pic.h>
#include <serial.h>
#include <stdbool.h>
#include <vgaterm.h>

void kernel_main(void) {
    gdt_install();
    idt_install();

    pic_remap();
    pic_irq_mask_all();
    pic_clear_mask(0x0);

    serial_init();
    sti();

    serial_writestring("Hello from serial\n");

    terminal_initialize();
    terminal_writestring("Hello, Kernel!");
    terminal_writestring("\n");
    for (uint16_t idx = 0; idx < 200; idx++) {
        terminal_writestring("Tach auch, ich bins. Der ERWIN!");
    }
    while (true) {
        __asm__ volatile("hlt");
    }
}
