/* isr.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef ISR_H
#define ISR_H
#include <serial.h>
#include <stdint.h>

struct interrupt_frame {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pusha order */
    uint32_t vector, error_code;
    uint32_t eip, cs, eflags;
};

void exception_handler(struct interrupt_frame *frame)
    __attribute__((__nonnull__));

#endif  // ISR_H
