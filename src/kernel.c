/* kernel.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <kernel.h>
#include <stdint.h>
#include <vgaterm.h>

void kernel_main(void) {
    terminal_initialize();
    terminal_writestring("Hello, Kernel!");
    terminal_writestring("\n");
    for (uint16_t idx = 0; idx < 2000; idx++) {
        terminal_writestring("Tach auch, ich bins. Der ERWIN!");
    }
}
