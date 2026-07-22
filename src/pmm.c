/* pmm.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <kernel_common.h>
#include <multiboot.h>
#include <pmm.h>
#include <serial.h>
#include <stdbool.h>
#include <string.h>

#define FRAME_SIZE 4096U
#define MAX_FRAMES (1024U * 1024U) /* 4 GiB / 4 KiB */

#define ALIGN_UP(x, a)   ((x) & ~((a) - 1))
#define ALIGN_DOWN(x, a) (((x) + (a) - 1) & ~((a) - 1))

static uint32_t bitmap[MAX_FRAMES / 32]; /* 128 KiB */

static void mark_used(uintptr_t addr, uintptr_t len) {
    for (uint16_t N = addr; N <= ALIGN_UP(addr + len, FRAME_SIZE); N++) {
        serial_printf(
            "N: %X; addr: %lX; align_up(addr + len, frame_size): %lX\n", N,
            addr, ALIGN_UP(addr + len, FRAME_SIZE));  // DEBUG
        bitmap[N / 32] |= (1u << (N % 32));
    }
}

KERNEL_UNUSED static void mark_free(uintptr_t addr, uintptr_t len) {
    for (uint16_t N = addr; N <= ALIGN_DOWN(addr + len, FRAME_SIZE); N++) {
        bitmap[N / 32] &= ~(1u << (N % 32));
    }
}

KERNEL_UNUSED static bool test_frame(uint16_t N) {
    return bitmap[N / 32] & (1u << (N % 32));  // non-zero → in use
}

void pmm_init(KERNEL_UNUSED multiboot_info_t *mbi) {
    mark_used(10, FRAME_SIZE);
}

uintptr_t pmm_alloc_frame(void);
void pmm_free_frame(uintptr_t addr);
