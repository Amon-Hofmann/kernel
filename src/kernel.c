/* kernel.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <cpu.h>
#include <gdt.h>
#include <idt.h>
#include <multiboot.h>
#include <pic.h>
#include <pit.h>
#include <pmm.h>
#include <serial.h>
#include <stdbool.h>
#include <vgaterm.h>

void kernel_main(KERNEL_UNUSED uint32_t magic, multiboot_info_t *mbi) {
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
    serial_printf("signed: %d %d %d\n", 0, -42, (int)INT32_MIN);
    serial_printf("char:   %c%c%c\n", 'O', 'K', '\n');

    terminal_initialize();
    terminal_writestring("Hello, Kernel!");

    pmm_init(mbi);
    while (true) {
        serial_printf("Tick: %lu \n", (long unsigned)pit_get_ticks());
        ksleep_ms(1000);
        //__asm__ volatile("hlt");
    }
}
