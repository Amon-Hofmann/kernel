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

#define FRAME_SIZE       ((uintptr_t)4096)
#define MAX_FRAMES       ((uintptr_t)(1024U * 1024U)) /* 4 GiB / 4 KiB */
#define MAX_ALLOWED_ADDR (UINTPTR_MAX)

// addresses from 0 to (FRAME_SIZE * MAX_FRAMES) = 2^32

#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))
#define ALIGN_UP(x, a)                                                              \
    (((x) & ((uintptr_t)(a) - 1U))                                                  \
         ? ((UINTPTR_MAX - (x) >= ((uintptr_t)(a) - ((x) & ((uintptr_t)(a) - 1U)))) \
                ? ((x) + ((uintptr_t)(a) - ((x) & ((uintptr_t)(a) - 1U))))          \
                : UINTPTR_MAX)                                                      \
         : (x))

#define BITMAP_LEN       ((MAX_FRAMES) >> 5)  // 2^15
#define BITMAP_MAX_INDEX (BITMAP_LEN - 1)

// indices from 0 to BITMAP_LEN - 1 = 2^15

#define PRINT_IN_USE(x)                                           \
    (serial_printf("Frame %lu is %sin use\n", (unsigned long)(x), \
                   is_frame_in_use(x) ? "" : "not "))

static uint32_t bitmap[BITMAP_LEN]; /* 128 KiB */

extern uint32_t kernel_phys_start;
extern uint32_t kernel_phys_end;

KERNEL_UNUSED KERNEL_INLINE uintptr_t frame_to_addr(uintptr_t frame) {
    return ((uintptr_t)frame) << 12;
}
KERNEL_UNUSED KERNEL_INLINE uint16_t frame_to_index(uintptr_t frame) {
    return frame >> 5;
}
KERNEL_UNUSED KERNEL_INLINE uintptr_t index_to_frame(uint16_t index) {
    return ((uintptr_t)index) << 5;
}
KERNEL_UNUSED KERNEL_INLINE uintptr_t index_to_addr(uint16_t index) {
    return ((uintptr_t)index) << 17;
}
// addr needs to be page-aligned
KERNEL_UNUSED KERNEL_INLINE uintptr_t addr_to_frame(uintptr_t addr) {
    return addr >> 12;
}
// addr needs to be a page-aligned
KERNEL_UNUSED KERNEL_INLINE uint16_t addr_to_index(uintptr_t addr) {
    return (uint16_t)(addr >> 17);
}

static void print_bitmap(uint16_t frame_start, uint16_t frame_stop) {
    for (uint16_t frame = frame_start; frame < frame_stop && frame < BITMAP_LEN;
         frame++) {
        if (frame == frame_start) {
            serial_writestring("Address     Frame    Bitmap Value \n");
        }
        serial_printf("%lX  %u:    %lb\n", (unsigned long)frame_to_addr(frame),
                      (unsigned)frame, (unsigned long)bitmap[frame_to_index(frame)]);
    }
}

KERNEL_UNUSED KERNEL_COLD static void test_alignment(void) {
    serial_writestring("aligning onto 8\n");
    for (uint8_t n = 0; n < UINT8_MAX; n++) {
        serial_printf("n: %b    aligned_up: %lb\n", n, (unsigned long)ALIGN_UP(n, 8));
    }
    for (uint8_t n = 0; n < UINT8_MAX; n++) {
        serial_printf("n: %x    aligned_down: %x\n", n, ALIGN_DOWN(n, 8));
    }
}

static void mark_used(uintptr_t addr, uintptr_t len) {
    if (addr > MAX_ALLOWED_ADDR) {
        serial_writestring("addr to big\n");
        return;
    }
    if (UINTPTR_MAX - len < addr) {
        serial_writestring("addr + len to big\n");
        return;
    }
    uintptr_t end_addr = addr + len;
    uintptr_t start_frame = addr_to_frame(ALIGN_DOWN(addr, FRAME_SIZE));

    uintptr_t aligned_end = ALIGN_UP(end_addr, FRAME_SIZE);
    uintptr_t stop_frame =
        (aligned_end == UINTPTR_MAX) ? MAX_FRAMES : addr_to_frame(aligned_end);

#ifdef DEBUG
    serial_printf("addr:        %lX\n", (unsigned long)addr);
    serial_printf("end addr:    %lX\n", (unsigned long)end_addr);
    serial_printf("start frame: %lX\n", (unsigned long)start_frame);
    serial_printf("stop_frame:  %lX\n", (unsigned long)stop_frame);
#endif  // DEBUG

    for (uintptr_t frame = start_frame; frame < stop_frame; frame++) {
#ifdef DEBUG
        serial_printf("marking frame %lu used\n", (unsigned long)frame);
#endif  // DEBUG
        bitmap[frame_to_index(frame)] |= (1u << (frame % 32));
    }
}

static void mark_free(uintptr_t addr, uintptr_t len) {
    uintptr_t end_addr = addr + len;
    uintptr_t start_frame = addr_to_frame(ALIGN_UP(addr, FRAME_SIZE));
    uintptr_t stop_frame = addr_to_frame(ALIGN_DOWN(end_addr, FRAME_SIZE));
    for (uintptr_t frame = start_frame; frame < stop_frame; frame++) {
#ifdef DEBUG
        serial_printf("marking frame %lu free\n", (unsigned long)frame);
#endif  // DEBUG
        bitmap[frame_to_index(frame)] &= ~(1u << (frame % 32));
    }
}

KERNEL_UNUSED static bool is_frame_in_use(uintptr_t frame) {
    return bitmap[frame_to_index(frame)] & (1u << (frame % 32));  // non-zero → in use
}

KERNEL_UNUSED static void test_frames(void) {
    mark_used(0, 5);
    PRINT_IN_USE(0);

    mark_used(10, 50 * FRAME_SIZE + 10);
    PRINT_IN_USE(0);
    // print_bitmap(0, 10);
    serial_writestring("\n");

    mark_free(10, 50 * FRAME_SIZE + 10);
    PRINT_IN_USE(0);
    PRINT_IN_USE(49);
    PRINT_IN_USE(50);
    // print_bitmap(0, 10);
    serial_writestring("\n");

    serial_writestring("max - frame_size\n");
    mark_used(MAX_ALLOWED_ADDR - 10, 9);
    PRINT_IN_USE(addr_to_frame(MAX_ALLOWED_ADDR));
    print_bitmap(addr_to_frame(ALIGN_DOWN(MAX_ALLOWED_ADDR - 10, FRAME_SIZE)),
                 addr_to_frame(MAX_ALLOWED_ADDR));
    serial_writestring("done\n");
}

void pmm_init(KERNEL_UNUSED multiboot_info_t *mbi) {
    // mark all frames used
    memset(bitmap, 0xFF, sizeof(bitmap));

    // mark frames free marked as available in multiboot header
    if (mbi->flags & (1 << 6)) {
        multiboot_mmap_entry_t *mmap_start = (multiboot_mmap_entry_t *)mbi->mmap_addr;
        multiboot_mmap_entry_t *entry = mmap_start;
        uint32_t offset = 0;
        uintptr_t base = 0;
        uintptr_t len = 0;

        while (offset < mbi->mmap_length) {
            if (entry->addr > (uint64_t)UINT32_MAX) {
                entry = (multiboot_mmap_entry_t *)((
                    uintptr_t)(entry + entry->size +
                               sizeof(entry->size)));  // cast to non-pointer type for
                                                       // arithmetic
                offset += ((uintptr_t)(entry + entry->size + sizeof(entry->size)));
                continue;
            }
            if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {  // available ram
                base = entry->addr;
                len = entry->addr + entry->len > (uint64_t)UINT32_MAX
                          ? UINT32_MAX - (uintptr_t)base
                          : (uintptr_t)entry->len;
                mark_free(base, len);
            }
            entry = (multiboot_mmap_entry_t *)((
                uintptr_t)(entry + entry->size +
                           sizeof(
                               entry->size)));  // cast to non-pointer type for arithmetic
            offset += ((uintptr_t)(entry + entry->size + sizeof(entry->size)));
        }
    } else {
        serial_writestring("multiboot mmap header not valid!");
    }

    // remark used where we know better than multiboot
    // first 1MiB
    mark_used(0, 1024 * 1024 - 1);

    uintptr_t ks = (uintptr_t)&kernel_phys_start;
    uintptr_t ke = (uintptr_t)&kernel_phys_end;

    // kernel .text, .rodata, .data and .bss
    mark_used(ks, ke - ks);
    print_bitmap(addr_to_frame(ALIGN_DOWN(1024 * 1024 - 1 - 3 * FRAME_SIZE, FRAME_SIZE)),
                 addr_to_frame(ALIGN_UP(1024 * 1024 - 1, FRAME_SIZE)));
}

uintptr_t pmm_alloc_frame(void) {
    for (size_t i = 0; i < MAX_FRAMES / 32; i++) {
        if (bitmap[i] == 0xFFFFFFFF) continue;
        uint32_t bit = __builtin_ctz(~bitmap[i]);
        bitmap[i] |= (1u << bit);
        return (uintptr_t)((i * 32 + bit) * FRAME_SIZE);
    }
    return 0; /* out of memory */
}
void pmm_free_frame(uintptr_t addr);
