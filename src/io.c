/* io.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <io.h>
#include <stdint.h>

inline void port_io_write_byte(uint8_t value, uint16_t port) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

inline uint8_t port_io_read_byte(uint16_t port) {
    uint8_t volatile ret = 0;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
