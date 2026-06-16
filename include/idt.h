/* idt.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef IDT_H
#define IDT_H

#include <isr.h>
#include <stdint.h>

#define IDT_ENTRY_LEN (256)

typedef struct idt_entry {
    uint16_t offset_low;  /* handler address bits 15:0  */
    uint16_t selector;    /* code segment selector       */
    uint8_t zero;         /* always 0                    */
    uint8_t type_attr;    /* P, DPL, type                */
    uint16_t offset_high; /* handler address bits 31:16  */
} __attribute__((packed)) idt_entry_t;

typedef struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

void idt_set_gate(uint8_t vector, uint32_t handler, uint16_t selector,
                  uint8_t type_attr);

void idt_install(void);

#endif  // IDT_H
