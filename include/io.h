/* io.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef IO_H
#define IO_H
#include <stdint.h>

void port_io_write_byte(uint8_t value, uint16_t port);

uint8_t port_io_read_byte(uint16_t port);

#endif  // IO_H
