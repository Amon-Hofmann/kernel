/* serial.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef SERIAL_H
#define SERIAL_H

#include <kernel_common.h>

void serial_init(void);
void serial_putchar(char c) _HOT;
void serial_writestring(const char *s) _NONNULL;
void serial_printf(const char *format, ...)
    __attribute__((format(printf, 1, 2))) _NONNULL;

#endif  // SERIAL_H
