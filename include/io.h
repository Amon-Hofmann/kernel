/* io.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef IO_H
#define IO_H
#include <kernel_common.h>
#include <stdint.h>

KERNEL_INLINE void port_io_write_byte(uint8_t value, uint16_t port) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

KERNEL_INLINE uint8_t port_io_read_byte(uint16_t port) {
    uint8_t ret = 0;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#endif  // IO_H
