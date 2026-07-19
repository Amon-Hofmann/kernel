/* kernel.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <cpu.h>
#include <gdt.h>
#include <idt.h>
#include <pic.h>
#include <pit.h>
#include <serial.h>
#include <stdbool.h>
#include <vgaterm.h>

void kernel_main(void) {
    gdt_install();
    idt_install();

    pic_remap();
    pic_irq_mask_all();
    pic_clear_mask(0x0);  // timer
    pic_clear_mask(0x1);  // keyboard

    serial_init();
    sti();

    pit_init(100);

    serial_writestring("Hello from serial\n");

    terminal_initialize();
    terminal_writestring("Hello, Kernel!");
    terminal_writestring("\n");
    for (uint16_t idx = 0; idx < 200; idx++) {
        terminal_writestring("Tach auch, ich bins. Der ERWIN!");
    }
    while (true) {
        serial_printf("Tick: %lu \n", (long unsigned)pit_get_ticks());
        ksleep_ms(1000);
        //__asm__ volatile("hlt");
    }
}
