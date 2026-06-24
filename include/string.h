/* string.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef STRING_H
#define STRING_H

#include <kernel_common.h>
#include <stddef.h>

void *memcpy(void *restrict dest, const void *restrict src,
             size_t n) _WUNUSED _RET_NONNULL _NONNULL;

void *memmove(void *dest, const void *src, size_t n) _RET_NONNULL _NONNULL;

void *memset(void *s, int c, size_t n) _RET_NONNULL _NONNULL;

#endif  // STRING_H
