/* serial.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

void serial_init(void);
void serial_putchar(char c);
void serial_writestring(const char *s) __attribute__((__nonnull__(1)));

#endif  // SERIAL_H
