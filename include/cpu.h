/* cpu.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef CPU_H
#define CPU_H

#include <kernel_common.h>

KERNEL_INLINE void sti(void) {
    __asm__ volatile("sti");
}

KERNEL_INLINE void cli(void) {
    __asm__ volatile("cli");
}

#endif  // CPU_H
