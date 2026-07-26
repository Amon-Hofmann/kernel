/* paging.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <kernel_common.h>
#include <paging.h>

#define PAGE_DIR_LEN     (1024)
#define PAGE_TABLE_0_LEN (1024)

#define PAGE_SIZE (4096)

#define PTE_FIELD_AVL                (11)
#define PTE_FIELD_GLOBAL             (8)
#define PTE_FIELD_PAT                (7)
#define PTE_FIELD_DIRTY              (6)
#define PTE_FIELD_ACCESSED           (5)
#define PTE_FIELD_PAGE_CACHE_DISABLE (4)
#define PTE_FIELD_PAGE_WRITE_THROUGH (3)
#define PTE_FIELD_USER_SUPER         (2)
#define PTE_FIELD_READ_WRITE         (1)
#define PTE_FIELD_PRESENT            (0)

#define PDE_FIELD_ACCESSED           (5)
#define PDE_FIELD_PAGE_CACHE_DISABLE (4)
#define PDE_FIELD_PAGE_WRITE_THROUGH (3)
#define PDE_FIELD_USER_SUPER         (2)
#define PDE_FIELD_READ_WRITE         (1)
#define PDE_FIELD_PRESENT            (0)

#define CR0_BIT_PAGING    (31)
#define CR0_BIT_PROTECTED (0)

KERNEL_ALIGN_PAGE KERNEL_UNUSED static uint32_t page_directory[1024];
KERNEL_ALIGN_PAGE KERNEL_UNUSED static uint32_t page_table_0[1024];

void paging_init(void) {
    // set identity map for pages 0x00000 - 0x00400
    for (uint32_t p = 0; p < PAGE_TABLE_0_LEN; p++) {
        page_table_0[p] =
            0 | (p * PAGE_SIZE) | (1u << PTE_FIELD_PRESENT) | (1u << PTE_FIELD_READ_WRITE);
    }
    // install page directory
    page_directory[0] =
        (uint32_t)page_table_0 | (1u << PDE_FIELD_PRESENT) | (1u << PDE_FIELD_READ_WRITE);

    // load page directory address into cr3 and set paging bit in cr0
    uint32_t cr0;
    __asm__ volatile("mov %0, %%cr3" ::"r"(page_directory) : "memory");
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1u << CR0_BIT_PAGING);
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0) : "memory");
    // paging active ...
}

void map_page(uint32_t virt, uint32_t phys, uint32_t flags);
