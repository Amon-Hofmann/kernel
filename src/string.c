/* string.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <string.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    char *to = dest;
    const char *from = src;

    for (size_t index = 0; index < n; index++) {
        *to++ = *from++;
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    char *temp;
    const char *s;

    temp = dest;
    s = src;
    if (dest >= src) {
        temp += n;
        s += n;
        while (n--) {
            *--temp = *--s;
        }
    } else {
        while (n--) {
            *temp++ = *s++;
        }
    }
    return dest;
}

void *memset(void *s, int c, size_t n) {
    char *temp = s;
    while (n--) {
        *temp++ = c;
    }
    return s;
}
