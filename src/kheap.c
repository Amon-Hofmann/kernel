/* kheap.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <kheap.h>
#include <paging.h>
#include <pmm.h>
#include <serial.h>
#include <stdbool.h>

#define KHEAP_VIRT_START (0x00400000u)
#define KHEAP_MAX_SIZE   (0x00400000u)
#define KHEAP_MAGIC      (0xDEADC0DEu)
#define KHEAP_INIT_PAGES (4u)
#define PAGE_SIZE        (4096u)

static uintptr_t heap_end = KHEAP_VIRT_START;

KERNEL_UNUSED static heap_block_t *heap_start = NULL;

#define HEAP_LIST_INIT \
    { .head = NULL, .tail = NULL, .len = 0 }

/* insert node at the end of list */
KERNEL_UNUSED KERNEL_NONNULL static void heap_list_insert_tail(heap_dll_t *list,
                                                               heap_block_t *node) {
    if (!list->head) {  // list empty
        list->head = node;
        list->tail = node;
        node->prev = NULL;
    } else {
        list->tail->next = node;
        node->prev = list->tail;
    }
    list->tail = node;
    node->next = NULL;
    list->len++;
}

/* insert node immediately after existing */
KERNEL_UNUSED KERNEL_NONNULL static bool heap_list_insert_after(heap_dll_t *list,
                                                                heap_block_t *existing,
                                                                heap_block_t *node) {
    if (!list->head) {  // list empty
        serial_printf("called %s on an empty list\n", __FUNCTION__);
        __asm__ volatile("hlt");
        __builtin_unreachable();
    }
    if (existing == list->tail) {
        heap_list_insert_tail(list, node);
        return true;
    }
    if (existing == list->head) {
        node->next = existing->next;
        node->prev = existing;
        existing->next = node;
        node->next->prev = node;
        list->len++;
        return true;
    } else {
        heap_block_t *ptr = list->head;
        while (ptr->next && ptr != existing) {
            ptr = ptr->next;
        }
        if (ptr == existing) {  // found
            node->next = existing->next;
            existing->next->prev = node;
            node->prev = existing;
            existing->next = node;
            list->len++;
            return true;
        } else {  // not in list
            return false;
        }
    }
}

/* unlink node from list (node is not freed) */
KERNEL_UNUSED KERNEL_NONNULL static void heap_list_remove(heap_dll_t *list,
                                                          heap_block_t *node) {
    if (!list->head) {  // list empty
        serial_printf("called %s on an empty list\n", __FUNCTION__);
        __asm__ volatile("hlt");
        __builtin_unreachable();
    }
    if (list->tail == node && list->head == node){
        node->next = NULL;
        node->prev = NULL;
        list->tail = NULL;
        list->head = NULL;
        list->len--;
        return;
    }
    if (node == list->tail) {
        list->tail = list->tail->prev;
        list->tail->next = NULL;
        node->prev = NULL;
        node->next = NULL;
        list->len--;
        return;
    }
    if (node == list->head) {
        list->head = list->head->next;
        list->head->prev = NULL;
        node->next = NULL;
    } else {
        heap_block_t *ptr = list->head;
        while (ptr->next && ptr != node) {
            ptr = ptr->next;
        }
        if (ptr == node) {  // found
            node->prev->next = node->next;
            node->next->prev = node->prev;
            node->prev = NULL;
            node->next = NULL;
        } else {  // not in list
            return;
        }
    }
    list->len--;
}

KERNEL_UNUSED static void heap_extend(size_t n_pages) {
    if (heap_end + n_pages * PAGE_SIZE > KHEAP_VIRT_START + KHEAP_MAX_SIZE) {
        serial_printf("Kernel Out Of Memory!\n");
        __asm__ volatile("hlt");
    }
    uintptr_t page = 0;

    for (size_t N = 0; N < n_pages; N++) {
        page = pmm_alloc_frame();
        map_page(heap_end, page, PAGE_PRESENT | PAGE_WRITABLE);
        heap_end += PAGE_SIZE;
    }
}

void kheap_init(void) {
    // set heap_start
}

void *kmalloc(size_t size);

void kfree(void *ptr);
