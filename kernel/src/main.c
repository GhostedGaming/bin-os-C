#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "font.h"
#include <limine.h>

// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.
__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, *and* they should be accessed at least
// once or marked as used with the "used" attribute as done here.
__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.
__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.
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

// Halt and catch fire function.
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

uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b) {
    // Convert RGB values to a 32-bit color value.
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

// Function to draw a character at specified position
void draw_char(uint32_t *framebuffer, int fb_width, int fb_pitch, 
               int x, int y, const uint8_t *char_bitmap, uint32_t color) {
    if (char_bitmap == NULL) return; // Skip if character not available
    
    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t byte = char_bitmap[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            if (byte & (0x80 >> col)) { // Check if bit is set (starting from MSB)
                int pixel_x = x + col;
                int pixel_y = y + row;
                
                // Bounds checking
                if (pixel_x >= 0 && pixel_x < fb_width && 
                    pixel_y >= 0 && pixel_y < (fb_pitch * 4 / sizeof(uint32_t))) {
                    framebuffer[pixel_y * fb_pitch + pixel_x] = color;
                }
            }
        }
    }
}

// Function to draw a character by ASCII value
void draw_ascii_char(uint32_t *framebuffer, int fb_width, int fb_pitch,
                     int x, int y, char c, uint32_t color) {
    if (c >= 0 && c < 128 && font_table[c] != NULL) {
        draw_char(framebuffer, fb_width, fb_pitch, x, y, font_table[c], color);
    }
}

// Function to draw a string
void draw_string(uint32_t *framebuffer, int fb_width, int fb_pitch,
                 int x, int y, const char *str, uint32_t color) {
    int current_x = x;
    for (int i = 0; str[i] != '\0'; i++) {
        draw_ascii_char(framebuffer, fb_width, fb_pitch, current_x, y, str[i], color);
        current_x += FONT_WIDTH + 1; // Move to next character position (with 1 pixel spacing)
    }
}

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
void kmain(void) {
    uint32_t background_color = rgb_to_color(0, 0, 0);
    uint32_t text_color = rgb_to_color(255, 255, 255);
    
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }
    
    if (framebuffer_request.response == NULL
        || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }
    
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    uint32_t *fb_ptr = framebuffer->address;
    int width = framebuffer->width;
    int height = framebuffer->height;
    int pitch = framebuffer->pitch / 4; // convert from bytes to 32-bit pixels
    
    // Fill background with blue color
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            fb_ptr[y * pitch + x] = background_color;
        }
    }
    
    // Draw individual letters
    draw_ascii_char(fb_ptr, width, pitch, 50, 50, ':', text_color);
    draw_ascii_char(fb_ptr, width, pitch, 59, 50, ';', text_color);
    draw_ascii_char(fb_ptr, width, pitch, 68, 50, ';', text_color);
    draw_ascii_char(fb_ptr, width, pitch, 77, 50, ':', text_color);
    draw_ascii_char(fb_ptr, width, pitch, 86, 50, ';', text_color);
    draw_ascii_char(fb_ptr, width, pitch, 86, 50, ';', text_color);
    
    // Or draw a complete string
    draw_string(fb_ptr, width, pitch, 50, 100, "HELLO", text_color);
    draw_string(fb_ptr, width, pitch, 50, 150, "ABCDEFGHIJKLMNOPQRSTUVWXYZ;:", text_color);
    draw_string(fb_ptr, width, pitch, 50, 200, "''", text_color);

    hcf();
}