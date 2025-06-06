#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

#include "font.h"
#include "draw/draw.h"

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

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

static void hcf(void) {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#elif defined (__loongarch64)
        asm ("idle 0");
#endif
    }
}

void* allocate_memory(size_t size) {
    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_request.response->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= size) {
            return (void*)entry->base;
        }
    }
    return NULL;
}

void kmain(void) {
    uint32_t background_color = rgb_to_color(0, 0, 0);
    uint32_t text_color = rgb_to_color(0, 0, 0);
    uint32_t highlight_color = rgb_to_color(185, 185, 189); // Grey highlight
    
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }
    
    if (framebuffer_request.response == NULL
        || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    allocate_memory(1024 * 1024);
    
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    uint32_t *fb_ptr = framebuffer->address;
    int width = framebuffer->width;
    int height = framebuffer->height;
    int pitch = framebuffer->pitch / 4;

    // Fill background
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            fb_ptr[y * pitch + x] = background_color;
        }
    }

    draw_string_center_screen_with_bg(fb_ptr, width, height, pitch, "Hello world!", text_color, highlight_color);
    while (1) {
        asm volatile ("hlt");
    }
}