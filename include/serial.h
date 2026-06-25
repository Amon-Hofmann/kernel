/* serial.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef SERIAL_H
#define SERIAL_H

#include <kernel_common.h>

void serial_init(void);
void serial_putchar(char c) KERNEL_HOT;
void serial_writestring(const char *s) KERNEL_NONNULL;
void serial_printf(const char *format, ...)
    __attribute__((format(printf, 1, 2))) KERNEL_NONNULL;

#endif  // SERIAL_H
