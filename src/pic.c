/* pic.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <io.h>
#include <pic.h>
#include <stdbool.h>

_INLINE void pic_init_chip(bool master) {
    // master / slave -> 0x04=master, 0x20=slave
    uint16_t cmd_port = 0, dat_port = 0;
    uint8_t line = 0;
    uint8_t vector = 0;
    if (master) {
        cmd_port = PIC_MASTER_COMMAND_PORT;
        dat_port = PIC_MASTER_DATA_PORT;
        line = 0x04;
        vector = 0x20;
    } else {
        cmd_port = PIC_SLAVE_COMMAND_PORT;
        dat_port = PIC_SLAVE_DATA_PORT;
        line = 0x02;
        vector = 0x28;
    }
    port_io_write_byte(0x11, cmd_port);

    port_io_write_byte(vector, dat_port);
    port_io_write_byte(line, dat_port);
    port_io_write_byte(0x01, dat_port);
}

void pic_remap(void) {
    pic_init_chip(true);
    pic_init_chip(false);
}

void pic_irq_mask_all(void) {
    port_io_write_byte(0xFF, PIC_MASTER_DATA_PORT);
    port_io_write_byte(0xFF, PIC_SLAVE_DATA_PORT);
}

void pic_set_mask(uint8_t irq) {
    uint16_t port = 0;
    if (irq < PIC_N_MASTER_LINES) {
        port = PIC_MASTER_DATA_PORT;
    } else {
        port = PIC_SLAVE_DATA_PORT;
    }
    uint8_t mask = port_io_read_byte(port);
    mask |= (1 << (irq % 8));
    port_io_write_byte(mask, port);
}

void pic_clear_mask(uint8_t irq) {
    uint16_t port = 0;
    if (irq < PIC_N_MASTER_LINES) {
        port = PIC_MASTER_DATA_PORT;
    } else {
        port = PIC_SLAVE_DATA_PORT;
    }
    uint8_t mask = port_io_read_byte(port);
    mask &= ~(1 << (irq % 8));
    port_io_write_byte(mask, port);
}
