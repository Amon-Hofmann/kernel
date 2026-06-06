/* string.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n)
    __attribute__((__nonnull__(1, 2)));

void *memmove(void *dest, const void *src, size_t n)
    __attribute__((__nonnull__(1, 2)));

void *memset(void *s, int c, size_t n) __attribute__((__nonnull__(1)));

#endif  // STRING_H
