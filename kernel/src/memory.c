#include <stdint.h>
#include <stddef.h>
#include <limine.h>
#include "memory.h"

// Fixed: proper __attribute__ syntax
__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

// Simple allocator state
static uint8_t *heap_start = NULL;
static uint8_t *heap_current = NULL;
static size_t heap_size = 0;
static int allocator_initialized = 0;

uint64_t detect_total_memory(void) {
    if (memmap_request.response == NULL) {
        return 0; // Failed
    }
    
    struct limine_memmap_response *memmap = memmap_request.response;
    uint64_t total_usable = 0;
    uint64_t total_physical = 0;
    
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        total_physical += entry->length;
        
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            total_usable += entry->length;
        }
    }
    
    return total_usable;
}

void init_memory_allocator(void) {
    if (memmap_request.response == NULL) {
        return;
    }
    
    // Find largest usable memory region for heap
    struct limine_memmap_response *memmap = memmap_request.response;
    size_t largest_size = 0;
    uint64_t best_base = 0;
    
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length > largest_size) {
            largest_size = entry->length;
            best_base = entry->base;
        }
    }
    
    if (largest_size > 0) {
        heap_start = (uint8_t*)best_base;
        heap_current = heap_start;
        heap_size = largest_size;
        allocator_initialized = 1;
    }
}

// Simple bump allocator (better than your original)
void *allocate_memory(size_t size) {
    if (!allocator_initialized) {
        init_memory_allocator();
    }
    
    if (!allocator_initialized || heap_current == NULL) {
        return NULL;
    }

    size_t aligned_size = (size + 7) & ~7;
    
    if ((heap_current + aligned_size) > (heap_start + heap_size)) {
        return NULL;
    }
    
    void *result = heap_current;
    heap_current += aligned_size;
    
    return result;
}

void free_memory(void *ptr) {
    (void)ptr;
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *restrict pdest = (uint8_t *restrict)dest;
    const uint8_t *restrict psrc = (const uint8_t *restrict)src;
    
    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }
    
    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;
    
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    
    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;
    
    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }
    
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }
    
    return 0;
}