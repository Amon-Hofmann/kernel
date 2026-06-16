/* kernel.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <gdt.h>
#include <idt.h>
#include <kernel.h>
#include <serial.h>
#include <vgaterm.h>

void kernel_main(void) {
    gdt_install();
    idt_install();
    serial_init();

    serial_writestring("Hello from serial\n");

    terminal_initialize();
    terminal_writestring("Hello, Kernel!");
    terminal_writestring("\n");
    for (uint16_t idx = 0; idx < 20; idx++) {
        terminal_writestring("Tach auch, ich bins. Der ERWIN!");
    }

}
