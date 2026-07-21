/* multiboot.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <kernel_common.h>
#include <stdint.h>

typedef struct {
    uint32_t size; /* byte length of this entry, not counting this field */
    uint64_t addr; /* base physical address of the region */
    uint64_t len;  /* length in bytes */
    uint32_t type; /* 1 = usable RAM, anything else = reserved */
} KERNEL_PACKED multiboot_mmap_entry_t;

typedef struct {
    uint32_t flags;        // @ offset 0
    uint32_t _[6];         // 4
    uint16_t syms;         // 28
    uint16_t __[7];        // 30
    uint32_t mmap_length;  // 44
    uint32_t mmap_addr;    // 48
} KERNEL_PACKED multiboot_info_t;

#define MULTIBOOT_MEMORY_AVAILABLE 1

#endif  // MULTIBOOT_H
