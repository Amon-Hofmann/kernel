/* pic.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef PIC_H
#define PIC_H
#include <io.h>
#include <kernel_common.h>
#include <stdint.h>

#define PIC_MASTER_COMMAND_PORT (0x20)
#define PIC_MASTER_DATA_PORT    (0x21)

#define PIC_SLAVE_COMMAND_PORT (0xA0)
#define PIC_SLAVE_DATA_PORT    (0xA1)

#define PIC_N_IRQ_LINES    (0x10)
#define PIC_N_MASTER_LINES (PIC_N_IRQ_LINES / 2)

void pic_remap(void);
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);
void pic_irq_mask_all(void);

_INLINE void pic_send_eoi(uint8_t irq) {
    port_io_write_byte(0x20, PIC_MASTER_COMMAND_PORT);
    if (irq >= PIC_N_MASTER_LINES) {
        port_io_write_byte(0x20, PIC_SLAVE_COMMAND_PORT);
    }
}

#endif  // PIC_H
