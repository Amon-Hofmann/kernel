/* gdt.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <gdt.h>
#include <stddef.h>

#define N_GDT_ENTRIES (5)

#define GDT_FLAT_MEMORY_BASE               (0x0)
#define GDT_FLAT_MEMORY_LIMIT              (0x000FFFFF)
#define GDT_FLAT_MEMORY_ACCESS_KERNEL_CODE (0x9A)
#define GDT_FLAT_MEMORY_ACCESS_KERNEL_DATA (0x92)
#define GDT_FLAT_MEMORY_ACCESS_USER_CODE   (0xFA)
#define GDT_FLAT_MEMORY_ACCESS_USER_DATA   (0xF2)
#define GDT_FLAT_MEMORY_FLAGS              (0x0C)

void gdt_flush(gdt_ptr_t *_gdt);

static gdt_entry_t gdt[N_GDT_ENTRIES];

static gdt_ptr_t gdt_ptr = {.limit = sizeof(gdt) - 1, .base = (uint32_t)gdt};

static void gdt_set_gate(size_t index, uint32_t base, uint32_t limit,
                         uint8_t access, uint8_t flags) {
    gdt[index].limit_low = (uint16_t)(limit & 0x0000FFFF);
    gdt[index].base_low = (uint16_t)(base & 0x0000FFFF);
    gdt[index].base_mid = (uint8_t)((base & 0x00FF0000) >> 16);
    gdt[index].access = access;
    gdt[index].granularity =
        ((flags & 0x0F) << 4) | ((uint8_t)((limit & 0x000F0000) >> 16));
    gdt[index].base_high = (uint8_t)((base & 0xFF000000) >> 24);
}

void gdt_install(void) {
    _Static_assert(N_GDT_ENTRIES == 5,
                   "Update gdt_install if you change 'N_GDT_ENTRIES'");
    gdt_set_gate(0, 0, 0, 0, 0);  // null

    gdt_set_gate(1, GDT_FLAT_MEMORY_BASE, GDT_FLAT_MEMORY_LIMIT,
                 GDT_FLAT_MEMORY_ACCESS_KERNEL_CODE,
                 GDT_FLAT_MEMORY_FLAGS);  // kernel code

    gdt_set_gate(2, GDT_FLAT_MEMORY_BASE, GDT_FLAT_MEMORY_LIMIT,
                 GDT_FLAT_MEMORY_ACCESS_KERNEL_DATA,
                 GDT_FLAT_MEMORY_FLAGS);  // kernel data

    gdt_set_gate(3, GDT_FLAT_MEMORY_BASE, GDT_FLAT_MEMORY_LIMIT,
                 GDT_FLAT_MEMORY_ACCESS_USER_CODE,
                 GDT_FLAT_MEMORY_FLAGS);  // user code

    gdt_set_gate(4, GDT_FLAT_MEMORY_BASE, GDT_FLAT_MEMORY_LIMIT,
                 GDT_FLAT_MEMORY_ACCESS_USER_DATA,
                 GDT_FLAT_MEMORY_FLAGS);  // user data

    gdt_flush(&gdt_ptr);
}
