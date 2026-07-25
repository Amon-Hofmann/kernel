/* paging.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_PRESENT  (1u << 0)
#define PAGE_WRITABLE (1u << 1)
#define PAGE_USER     (1u << 2)

void paging_init(void);
void map_page(uint32_t virt, uint32_t phys, uint32_t flags);

#endif  // PAGING_H
