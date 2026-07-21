/* pmm.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef PMM_H
#define PMM_H

#include <multiboot.h>
#include <stdint.h>

void pmm_init(multiboot_info_t *mbi);
uintptr_t pmm_alloc_frame(void);
void pmm_free_frame(uintptr_t addr);

#endif  // PMM_H
