#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "font.h"
#include <limine.h>

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
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

uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b) {
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

void draw_char(uint32_t *framebuffer, int fb_width, int fb_pitch, 
               int x, int y, const uint8_t *char_bitmap, uint32_t color) {
    if (char_bitmap == NULL) return;
    
    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t byte = char_bitmap[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            if (byte & (0x80 >> col)) {
                int pixel_x = x + col;
                int pixel_y = y + row;
                
                if (pixel_x >= 0 && pixel_x < fb_width && 
                    pixel_y >= 0 && pixel_y < (fb_pitch * 4 / sizeof(uint32_t))) {
                    framebuffer[pixel_y * fb_pitch + pixel_x] = color;
                }
            }
        }
    }
}

// New function to draw character with background
void draw_char_with_bg(uint32_t *framebuffer, int fb_width, int fb_pitch, 
                       int x, int y, const uint8_t *char_bitmap, 
                       uint32_t fg_color, uint32_t bg_color) {
    if (char_bitmap == NULL) return;
    
    // First, draw the background rectangle
    for (int row = 0; row < FONT_HEIGHT; row++) {
        for (int col = 0; col < FONT_WIDTH; col++) {
            int pixel_x = x + col;
            int pixel_y = y + row;
            
            if (pixel_x >= 0 && pixel_x < fb_width && 
                pixel_y >= 0 && pixel_y < (fb_pitch * 4 / sizeof(uint32_t))) {
                framebuffer[pixel_y * fb_pitch + pixel_x] = bg_color;
            }
        }
    }
    
    // Then draw the character on top
    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t byte = char_bitmap[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            if (byte & (0x80 >> col)) {
                int pixel_x = x + col;
                int pixel_y = y + row;
                
                if (pixel_x >= 0 && pixel_x < fb_width && 
                    pixel_y >= 0 && pixel_y < (fb_pitch * 4 / sizeof(uint32_t))) {
                    framebuffer[pixel_y * fb_pitch + pixel_x] = fg_color;
                }
            }
        }
    }
}

void draw_ascii_char(uint32_t *framebuffer, int fb_width, int fb_pitch,
                     int x, int y, char c, uint32_t color) {
    if (c >= 0 && c < 128 && font_table[c] != NULL) {
        draw_char(framebuffer, fb_width, fb_pitch, x, y, font_table[c], color);
    }
}

// New function to draw ASCII character with background
void draw_ascii_char_with_bg(uint32_t *framebuffer, int fb_width, int fb_pitch,
                              int x, int y, char c, uint32_t fg_color, uint32_t bg_color) {
    if (c >= 0 && c < 128 && font_table[c] != NULL) {
        draw_char_with_bg(framebuffer, fb_width, fb_pitch, x, y, font_table[c], fg_color, bg_color);
    }
}

void draw_string(uint32_t *framebuffer, int fb_width, int fb_pitch,
                 int x, int y, const char *str, uint32_t color) {
    int current_x = x;
    for (int i = 0; str[i] != '\0'; i++) {
        draw_ascii_char(framebuffer, fb_width, fb_pitch, current_x, y, str[i], color);
        current_x += FONT_WIDTH + 1;
    }
}

// New function to draw string with background
void draw_string_with_bg(uint32_t *framebuffer, int fb_width, int fb_pitch,
                         int x, int y, const char *str, uint32_t fg_color, uint32_t bg_color) {
    int current_x = x;
    for (int i = 0; str[i] != '\0'; i++) {
        draw_ascii_char_with_bg(framebuffer, fb_width, fb_pitch, current_x, y, str[i], fg_color, bg_color);
        current_x += FONT_WIDTH + 1;
    }
}

// Helper function to calculate string length
int string_length(const char *str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// Function to draw centered string (no background)
void draw_string_centered(uint32_t *framebuffer, int fb_width, int fb_height, int fb_pitch,
                         int y, const char *str, uint32_t color) {
    int str_len = string_length(str);
    int str_width = str_len * (FONT_WIDTH + 1) - 1; // -1 because last char doesn't need spacing
    int center_x = (fb_width - str_width) / 2;
    draw_string(framebuffer, fb_width, fb_pitch, center_x, y, str, color);
}

// Function to draw centered string with background
void draw_string_centered_with_bg(uint32_t *framebuffer, int fb_width, int fb_height, int fb_pitch,
                                  int y, const char *str, uint32_t fg_color, uint32_t bg_color) {
    int str_len = string_length(str);
    int str_width = str_len * (FONT_WIDTH + 1) - 1; // -1 because last char doesn't need spacing
    int center_x = (fb_width - str_width) / 2;
    draw_string_with_bg(framebuffer, fb_width, fb_pitch, center_x, y, str, fg_color, bg_color);
}

// Function to draw string centered both horizontally and vertically
void draw_string_center_screen(uint32_t *framebuffer, int fb_width, int fb_height, int fb_pitch,
                              const char *str, uint32_t color) {
    int str_len = string_length(str);
    int str_width = str_len * (FONT_WIDTH + 1) - 1;
    int center_x = (fb_width - str_width) / 2;
    int center_y = (fb_height - FONT_HEIGHT) / 2;
    draw_string(framebuffer, fb_width, fb_pitch, center_x, center_y, str, color);
}

// Function to draw string centered both horizontally and vertically with background
void draw_string_center_screen_with_bg(uint32_t *framebuffer, int fb_width, int fb_height, int fb_pitch,
                                      const char *str, uint32_t fg_color, uint32_t bg_color) {
    int str_len = string_length(str);
    int str_width = str_len * (FONT_WIDTH + 1) - 1;
    int center_x = (fb_width - str_width) / 2;
    int center_y = (fb_height - FONT_HEIGHT) / 2;
    draw_string_with_bg(framebuffer, fb_width, fb_pitch, center_x, center_y, str, fg_color, bg_color);
}

void kmain(void) {
    uint32_t background_color = rgb_to_color(0, 0, 0);
    uint32_t text_color = rgb_to_color(255, 255, 255);
    uint32_t highlight_color = rgb_to_color(0, 100, 200); // Blue highlight
    uint32_t yellow_highlight = rgb_to_color(255, 255, 0); // Yellow highlight
    uint32_t green_highlight = rgb_to_color(0, 150, 0); // Green highlight
    uint32_t red_highlight = rgb_to_color(200, 0, 0); // Red highlight
    
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
    int pitch = framebuffer->pitch / 4;

    // Fill background
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            fb_ptr[y * pitch + x] = background_color;
        }
    }

    draw_string_center_screen_with_bg(fb_ptr, width, height, pitch, "HELLO WORLD!", text_color, highlight_color);

    draw_string_centered_with_bg(fb_ptr, width, height, pitch, 50, "YELLOW HIGHLIGHT", rgb_to_color(0, 0, 0), yellow_highlight);
    draw_string_centered_with_bg(fb_ptr, width, height, pitch, 100, "GREEN HIGHLIGHT", text_color, green_highlight);
    draw_string_centered_with_bg(fb_ptr, width, height, pitch, 150, "RED HIGHLIGHT", text_color, red_highlight);

    draw_string_centered(fb_ptr, width, height, pitch, 200, "Centered Normal Text", text_color);
    draw_string_centered_with_bg(fb_ptr, width, height, pitch, 250, "Centered Highlighted", text_color, highlight_color);

    draw_string_with_bg(fb_ptr, width, pitch, 50, 320, ":;\".<>", text_color, yellow_highlight);
    draw_string_with_bg(fb_ptr, width, pitch, 50, 370, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", text_color, green_highlight);
    draw_string_with_bg(fb_ptr, width, pitch, 50, 420, "'HELLO WORLD'", rgb_to_color(0, 0, 0), rgb_to_color(255, 255, 255));

    draw_string_centered(fb_ptr, width, height, pitch, height - 50, "Centered at bottom", text_color);

    hcf();
}