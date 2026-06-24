/* isr.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef ISR_H
#define ISR_H
#include <kernel_common.h>
#include <stdint.h>

struct interrupt_frame {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pusha order */
    uint32_t vector, error_code;
    uint32_t eip, cs, eflags;
};

void exception_handler(struct interrupt_frame *frame) _NONNULL _COLD _NORETURN;

void irq_handler(struct interrupt_frame *frame) _HOT _NONNULL;
// interrupt_frame->error_code is here the irq line (0-15)

#endif  // ISR_H
