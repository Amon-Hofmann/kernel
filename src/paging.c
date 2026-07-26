/* paging.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <kernel_common.h>
#include <paging.h>
#include <pmm.h>
#include <serial.h>
#include <string.h>
#include <vgaterm.h>

#define PAGE_DIR_LEN   (1024)
#define PAGE_TABLE_LEN (1024)

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

#define CR0_BIT_PAGING        (31)
#define CR0_BIT_WRITE_PROTECT (16)

KERNEL_ALIGN_PAGE KERNEL_UNUSED static uint32_t page_directory[1024] = {0};
KERNEL_ALIGN_PAGE KERNEL_UNUSED static uint32_t page_table_0[1024] = {0};

KERNEL_INLINE void activate_paging(void) {
    uint32_t cr0;
    __asm__ volatile("mov %0, %%cr3" ::"r"(page_directory) : "memory");
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1u << CR0_BIT_PAGING);
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0) : "memory");
}

KERNEL_INLINE void invalidate_page(uint32_t virt_addr) {
    // invalidates the pages Translation Lookaside Buffer (cache)
    __asm__ volatile("invlpg (%0)" ::"r"(virt_addr) : "memory");
}

KERNEL_UNUSED KERNEL_INLINE void enforce_write_protection(void) {
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1u << CR0_BIT_WRITE_PROTECT);
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0) : "memory");
}

KERNEL_COLD void paging_init(void) {
    // set identity map for pages 0x00000 - 0x00400
    for (uint32_t p = 0; p < PAGE_TABLE_LEN; p++) {
        page_table_0[p] = 0 | (p * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;
    }
    // install page directory entry 0
    page_directory[0] =
        (uint32_t)page_table_0 | (1u << PDE_FIELD_PRESENT) | (1u << PDE_FIELD_READ_WRITE);

    // load page directory address into cr3 and set paging bit in cr0
    activate_paging();
    // paging active ...
}

KERNEL_INLINE uint32_t *get_addr_from_pde(uint32_t pde) {
    return (uint32_t *)(pde & 0xFFFFF000u);
}

KERNEL_UNUSED KERNEL_INLINE uint32_t *get_addr_from_pte(uint32_t pte) {
    return (uint32_t *)(pte & 0xFFFFF000u);
}

void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint16_t dir_index = (uint16_t)((virt & 0xFFC00000u) >> 22);  // 10 bits 31-22
    uint16_t tbl_index = (uint16_t)((virt & 0x003FF000u) >> 12);  // 10 bits 21-12
    uint32_t page_table_addr = 0;

    if (!(page_directory[dir_index] & PAGE_PRESENT)) {
        // page dir entry is not present ->
        // install page directory entry (allocate page table entry)
        page_table_addr = pmm_alloc_frame();
        memset((void *)page_table_addr, 0, PAGE_SIZE);
        page_directory[dir_index] =
            0 | (page_table_addr & ~0xFFFu) | PAGE_PRESENT | flags;
        uint32_t *page_table = (uint32_t *)page_table_addr;
        page_table[tbl_index] = 0 | (phys & ~0xFFFu) | PAGE_PRESENT | flags;

    } else {
        // page dir entry is already present
        page_table_addr = (uint32_t)get_addr_from_pde(page_directory[dir_index]);
        uint32_t *page_table = (uint32_t *)page_table_addr;
        if (!(page_table[tbl_index] & PAGE_PRESENT)) {
            //  page table entry is not present
            //  install page table entry
            page_table[tbl_index] = 0 | (phys & ~0xFFFu) | PAGE_PRESENT | flags;

        } else {
            //  page table entry is already present -> already mapped -> bad
            serial_printf("Virtual Address %lX already mapped!\n", (unsigned long)virt);
            serial_writestring("halting...\n");
            __asm__ volatile("hlt");
        }
    }
    invalidate_page(virt);
}
