/* cpu.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef CPU_H
#define CPU_H

#include <kernel_common.h>

_INLINE void sti(void) {
    __asm__ volatile("sti");
}

_INLINE void cli(void) {
    __asm__ volatile("cli");
}

#endif  // CPU_H
