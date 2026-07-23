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
#define BITMAP_MAX_INDEX ((MAX_FRAMES) / 32)

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
    serial_writestring("aligning onto 8\n");
    for (uint8_t n = 0; n < UINT8_MAX; n++) {
        serial_printf("n: %b    aligned_up: %b\n", n, ALIGN_UP(n, 8));
    }
    for (uint8_t n = 0; n < UINT8_MAX; n++) {
        // serial_printf("n: %x    aligned_down: %x\n", n, ALIGN_UP(n, 8));
    }
}

static void mark_used(uintptr_t addr, uintptr_t len) {
    uintptr_t end_addr = addr + len;
    uintptr_t start_frame = ALIGN_DOWN(addr, FRAME_SIZE) / FRAME_SIZE;
    uintptr_t stop_frame = ALIGN_UP(end_addr, FRAME_SIZE) / FRAME_SIZE;
#ifdef DEBUG
    serial_printf("addr:        %lX\n", (unsigned long)addr);
    serial_printf("end addr:    %lX\n", (unsigned long)end_addr);
    serial_printf("start frame: %lX\n", (unsigned long)start_frame);
    serial_printf("stop_frame:  %lX\n", (unsigned long)stop_frame);
#endif  // DEBUG

    for (uint16_t N = start_frame; N < stop_frame; N++) {
        bitmap[N / 32] |= (1u << (N % 32));
    }
}

KERNEL_UNUSED static void mark_free(uintptr_t addr, uintptr_t len) {
    uintptr_t end_addr = addr + len;
    uintptr_t start_frame = ALIGN_UP(addr, FRAME_SIZE) / FRAME_SIZE;
    uintptr_t stop_frame = ALIGN_DOWN(end_addr, FRAME_SIZE) / FRAME_SIZE;
    for (uint16_t N = start_frame; N < stop_frame; N++) {
        bitmap[N / 32] &= ~(1u << (N % 32));
    }
}

KERNEL_UNUSED static bool test_frame(uint16_t N) {
    return bitmap[N / 32] & (1u << (N % 32));  // non-zero → in use
}

void pmm_init(KERNEL_UNUSED multiboot_info_t *mbi) {
    mark_used(0, 3 * FRAME_SIZE + 10);
    print_bitmap_idx(0, 10);

    mark_free(0, 3 * FRAME_SIZE + 10);
    print_bitmap_idx(0, 10);

    serial_writestring("printing bitmap\n");
    print_bitmap_idx(BITMAP_MAX_INDEX - 10, BITMAP_MAX_INDEX);
    serial_writestring("done..\n");

#ifdef DEBUG
    test_alignment();
#endif  // DEBUG
}

uintptr_t pmm_alloc_frame(void);
void pmm_free_frame(uintptr_t addr);
