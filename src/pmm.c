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

// addresses from 0 to (FRAME_SIZE * MAX_FRAMES) = 2^32

#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))
#define ALIGN_UP(x, a)   (((x) + (a) - 1) & ~((a) - 1))
#define BITMAP_LEN       ((MAX_FRAMES) / 32)  // 2^15
#define BITMAP_MAX_INDEX (BITMAP_LEN - 1)

// indices from 0 to BITMAP_LEN - 1 = 2^15

static uint32_t bitmap[BITMAP_LEN]; /* 128 KiB */

KERNEL_UNUSED static uintptr_t frame_to_addr(uint16_t frame) {
    return ((uintptr_t)frame) << 12;
}
KERNEL_UNUSED static uint16_t addr_to_frame(uintptr_t addr) {
    return (uint16_t)(addr >> 12);
}

static void print_bitmap(uint16_t frame_start, uint16_t frame_stop) {
    serial_writestring("Address     Frame    Bitmap Value \n");
    for (uint16_t frame = frame_start; frame < frame_stop && frame < BITMAP_LEN;
         frame++) {
        serial_printf("%lX  %u:    %lb\n", (long unsigned)frame_to_addr(frame),
                      frame, (unsigned long)bitmap[frame]);
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
    if (addr > MAX_FRAMES * FRAME_SIZE) {
        serial_writestring("addr to big\n");
    }
    if (addr + len > MAX_FRAMES * FRAME_SIZE) {
        serial_writestring("addr + len to big\n");
    }
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

KERNEL_UNUSED static bool is_frame_in_use_idx(uint16_t index) {
    return bitmap[index / 32] & (1u << (index % 32));  // non-zero → in use
}

KERNEL_UNUSED static bool is_frame_in_use_(uintptr_t addr) {
    uint16_t index = ALIGN_UP(addr, FRAME_SIZE) / FRAME_SIZE;
    return bitmap[index / 32] & (1u << (index % 32));  // non-zero → in use
}

void pmm_init(KERNEL_UNUSED multiboot_info_t *mbi) {
    mark_used(10, 50 * FRAME_SIZE + 10);
    print_bitmap(0, 10);
    serial_writestring("\n");

    mark_free(10, 50 * FRAME_SIZE + 10);
    print_bitmap(0, 10);
    serial_writestring("\n");

    mark_used(MAX_FRAMES * FRAME_SIZE - FRAME_SIZE, 5);
    serial_printf("is %sin use\n",
                  is_frame_in_use_idx(BITMAP_LEN) ? "" : "not ");

    serial_writestring("max - 10\n");
    print_bitmap(BITMAP_LEN - 10, BITMAP_LEN);
    serial_writestring("done\n");

#ifdef DEBUG
    test_alignment();
#endif  // DEBUG
}

uintptr_t pmm_alloc_frame(void);
void pmm_free_frame(uintptr_t addr);
