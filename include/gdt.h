/* gdt.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef GDT_H
#define GDT_H

#include <kernel_common.h>
#include <stdint.h>

typedef struct gdt_entry {
    uint16_t limit_low;  /* limit bits 15:0  */
    uint16_t base_low;   /* base  bits 15:0  */
    uint8_t base_mid;    /* base  bits 23:16 */
    uint8_t access;      /* see access byte below */
    uint8_t granularity; /* flags[7:4] | limit[19:16][3:0] */
    uint8_t base_high;   /* base  bits 31:24 */
} __PACKED gdt_entry_t;

typedef struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __PACKED gdt_ptr_t;

void gdt_install(void);

#endif  // GDT_H
