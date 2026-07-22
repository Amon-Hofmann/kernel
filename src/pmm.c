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

#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))
#define ALIGN_UP(x, a)   (((x) + (a) - 1) & ~((a) - 1))
#define BITMAP_MAX_INDEX (MAX_FRAMES / 32)

static uint32_t bitmap[BITMAP_MAX_INDEX]; /* 128 KiB */

static void KERNEL_UNUSED KERNEL_COLD print_bitmap(
    KERNEL_UNUSED uintptr_t addr_start, KERNEL_UNUSED uintptr_t addr_stop) {
    for (uint16_t addr = 0; addr < BITMAP_MAX_INDEX; addr++) {
    }
}
static void KERNEL_UNUSED KERNEL_COLD print_bitmap_idx(uint16_t idx_start,
                                                       uint16_t idx_stop) {
    for (uint16_t index = idx_start;
         index < idx_stop && index < BITMAP_MAX_INDEX; index++) {
        serial_printf("%X: %lb\n", index, (unsigned long)bitmap[index]);
    }
}

KERNEL_UNUSED KERNEL_COLD static void test_alignment(void) {
    serial_printf("aligning onto 8");
    for (uint8_t n = 0; n < UINT8_MAX; n++) {
        serial_printf("n: %b    aligned_up: %b\n", n, ALIGN_UP(n, 8));
    }
    for (uint8_t n = 0; n < UINT8_MAX; n++) {
        // serial_printf("n: %x    aligned_down: %x\n", n, ALIGN_UP(n, 8));
    }
}

KERNEL_UNUSED static void mark_used(uintptr_t addr, uintptr_t len) {
    KERNEL_UNUSED uintptr_t end_addr = addr + len;
    KERNEL_UNUSED uintptr_t end_frame = ALIGN_UP(addr + len, FRAME_SIZE);
    KERNEL_UNUSED uintptr_t start_frame = ALIGN_DOWN(addr, FRAME_SIZE);
    KERNEL_UNUSED uint16_t idx_start = 0;
    serial_printf("addr:        %lX\n", (unsigned long)addr);
    serial_printf("end addr:    %lX\n", (unsigned long)end_addr);
    serial_printf("end frame:   %lX\n", (unsigned long)end_frame);
    serial_printf("start frame: %lX\n", (unsigned long)start_frame);

    /*
        for(uint16_t index=0; ;){

        }

        for (uint16_t N = addr; N <= ALIGN_UP(addr + len, FRAME_SIZE); N++) {
            serial_printf( "N: %X; addr: %lX; align_up(addr + len, frame_size):
       %lX\n", N, (unsigned long)addr, (unsigned long)ALIGN_UP(addr + len,
       FRAME_SIZE));  // DEBUG bitmap[N / 32] |= (1u << (N % 32));
        }
        */
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
    // mark_used(10, FRAME_SIZE);
    // print_bitmap_idx(0, 100);
    test_alignment();
}

uintptr_t pmm_alloc_frame(void);
void pmm_free_frame(uintptr_t addr);
