#include <limine.h>
#include <stddef.h>
#include "draw.h"
#include "font.h"

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

// Define the global framebuffer variables
struct limine_framebuffer *framebuffer = NULL;
uint32_t *fb_ptr = NULL;
int fb_width = 0;
int fb_height = 0;
int fb_pitch = 0;

void init_framebuffer(void) {
    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }
    
    framebuffer = framebuffer_request.response->framebuffers[0];
    fb_ptr = framebuffer->address;
    fb_width = framebuffer->width;
    fb_height = framebuffer->height;
    fb_pitch = framebuffer->pitch / 4;
}

static int string_length(const char *str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

void draw_char(int x, int y, const uint8_t *char_bitmap, uint32_t color) {
    if (!char_bitmap) return;
    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t byte = char_bitmap[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            if (byte & (0x80 >> col)) {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < fb_width && py >= 0) {
                    fb_ptr[py * fb_pitch + px] = color;
                }
            }
        }
    }
}

void draw_char_with_bg(int x, int y, const uint8_t *char_bitmap, uint32_t fg_color, uint32_t bg_color) {
    if (!char_bitmap) return;
    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t byte = char_bitmap[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            int px = x + col;
            int py = y + row;
            if (px >= 0 && px < fb_width && py >= 0) {
                uint32_t color = (byte & (0x80 >> col)) ? fg_color : bg_color;
                fb_ptr[py * fb_pitch + px] = color;
            }
        }
    }
}

void draw_ascii_char(int x, int y, char c, uint32_t color) {
    if (c >= 0 && c < 128 && font_table[c]) {
        draw_char(x, y, font_table[c], color);
    }
}

void draw_ascii_char_with_bg(int x, int y, char c, uint32_t fg_color, uint32_t bg_color) {
    if (c >= 0 && c < 128 && font_table[c]) {
        draw_char_with_bg(x, y, font_table[c], fg_color, bg_color);
    } else {
        for (int row = 0; row < FONT_HEIGHT; row++) {
            for (int col = 0; col < FONT_WIDTH; col++) {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < fb_width && py >= 0) {
                    fb_ptr[py * fb_pitch + px] = bg_color;
                }
            }
        }
    }
}

void draw_string(int x, int y, const char *str, uint32_t color) {
    int cx = x;
    for (int i = 0; str[i]; i++) {
        draw_ascii_char(cx, y, str[i], color);
        cx += FONT_WIDTH;
    }
}

void draw_string_with_bg(int x, int y, const char *str, uint32_t fg_color, uint32_t bg_color) {
    int cx = x;
    for (int i = 0; str[i]; i++) {
        draw_ascii_char_with_bg(cx, y, str[i], fg_color, bg_color);
        cx += FONT_WIDTH;
    }
}

void draw_string_centered(int y, const char *str, uint32_t color) {
    int len = string_length(str);
    int str_width = len * FONT_WIDTH;
    int cx = (fb_width - str_width) / 2;
    draw_string(cx, y, str, color);
}

void draw_string_centered_with_bg(int y, const char *str, uint32_t fg_color, uint32_t bg_color) {
    int len = string_length(str);
    int str_width = len * FONT_WIDTH;
    int cx = (fb_width - str_width) / 2;
    draw_string_with_bg(cx, y, str, fg_color, bg_color);
}

void draw_string_center_screen(const char *str, uint32_t color) {
    int len = string_length(str);
    int str_width = len * FONT_WIDTH;
    int cx = (fb_width - str_width) / 2;
    int cy = (fb_height - FONT_HEIGHT) / 2;
    draw_string(cx, cy, str, color);
}

void draw_string_center_screen_with_bg(const char *str, uint32_t fg_color, uint32_t bg_color) {
    int len = string_length(str);
    int str_width = len * FONT_WIDTH;
    int cx = (fb_width - str_width) / 2;
    int cy = (fb_height - FONT_HEIGHT) / 2;
    draw_string_with_bg(cx, cy, str, fg_color, bg_color);
}

uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b) {
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}