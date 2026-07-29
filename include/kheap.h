/* kheap.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef KHEAP_H
#define KHEAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct heap_block {
    uint32_t magic;
    bool used;
    size_t size;
    struct heap_block *next;
    struct heap_block *prev;
} heap_block_t;

typedef struct heap_dll {
    heap_block_t *head;
    heap_block_t *tail;
    uint32_t len;
} heap_dll_t;

void kheap_init(void);

void *kmalloc(size_t size);

void kfree(void *ptr);

#endif  // KHEAP_H
