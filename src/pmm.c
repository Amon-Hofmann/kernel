/* pmm.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <multiboot.h>
#include <pmm.h>
#include <serial.h>
#include <string.h>
#include <stdbool.h>

#define FRAME_SIZE     4096U
#define MAX_FRAMES     (1024U * 1024U)   /* 4 GiB / 4 KiB */

static uint32_t bitmap[MAX_FRAMES / 32]; /* 128 KiB */

static void mark_used(uint16_t N){
    bitmap[N / 32] |=  (1u << (N % 32));
}

static void mark_free(uint16_t N){
    bitmap[N / 32] &= ~(1u << (N % 32));
}

static bool test_frame(uint16_t N){
    return bitmap[N / 32] &   (1u << (N % 32));    /* non-zero → in use */
}

void pmm_init(multiboot_info_t *mbi){
    /* 1. Everything used */
    memset(bitmap, 0xFF, sizeof(bitmap));

    /* 2. Mark usable regions free */
    if (!(mbi->flags & (1 << 6))) { /* no mmap — panic */ return; }
    uint32_t off = 0;
    while (off < mbi->mmap_length) {
        multiboot_mmap_entry_t *e =
            (multiboot_mmap_entry_t *)(uintptr_t)(mbi->mmap_addr + off);
        if (e->type == MULTIBOOT_MEMORY_AVAILABLE
                && e->addr < 0x100000000ULL) {
            uintptr_t base = (uintptr_t)e->addr;
            uintptr_t len  = (e->addr + e->len > 0x100000000ULL)
                             ? (uintptr_t)(0x100000000ULL - e->addr)
                             : (uintptr_t)e->len;
            mark_free(base, len);
        }
        off += e->size + sizeof(e->size);
    }

    /* 3. Re-mark reserved regions */
    extern uint32_t kernel_phys_start, kernel_phys_end;
    mark_used(0, 0x100000);   /* first 1 MiB */
    mark_used((uintptr_t)&kernel_phys_start,
              (uintptr_t)&kernel_phys_end - (uintptr_t)&kernel_phys_start);
}
uintptr_t pmm_alloc_frame(void);
void pmm_free_frame(uintptr_t addr);
