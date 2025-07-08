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
uint16_t cursor_position_x = 0;  // Changed to uint16_t for better range
uint16_t cursor_position_y = 0;

// Forward declaration for hcf function
void hcf(void);

uint16_t move_cursor_right(uint16_t amount) {
    // Add bounds checking to prevent cursor from going off screen
    if (cursor_position_x + amount < fb_width) {
        cursor_position_x += amount;  // Fixed: was missing assignment operator
    } else {
        cursor_position_x = fb_width - 1;  // Clamp to screen edge
    }
    return cursor_position_x;
}

uint16_t move_cursor_up(uint16_t amount) {
    if (cursor_position_y >= amount) {
        cursor_position_y -= amount;  // Move up means subtract from y
    } else {
        cursor_position_y = 0;  // Clamp to top of screen
    }
    return cursor_position_y;
}

uint16_t move_cursor_down(uint16_t amount) {
    if (cursor_position_y + amount < fb_height) {
        cursor_position_y += amount;
    } else {
        cursor_position_y = fb_height - 1;  // Clamp to bottom of screen
    }
    return cursor_position_y;
}

uint16_t move_cursor_left(uint16_t amount) {
    if (cursor_position_x >= amount) {
        cursor_position_x -= amount;
    } else {
        cursor_position_x = 0;  // Clamp to left edge
    }
    return cursor_position_x;
}

void move_cursor_to(uint16_t x, uint16_t y) {
    if (x < fb_width) {
        cursor_position_x = x;
    }
    if (y < fb_height) {
        cursor_position_y = y;
    }
}

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

// Fixed: Added missing x parameter to match header declaration
void draw_char(int x, int y, const uint8_t *char_bitmap, uint32_t color) {
    if (!char_bitmap) return;
    
    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t byte = char_bitmap[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            if (byte & (0x80 >> col)) {
                int px = x + col;
                int py = y + row;
                // Add bounds checking for py as well
                if (px >= 0 && px < fb_width && py >= 0 && py < fb_height) {
                    fb_ptr[py * fb_pitch + px] = color;
                }
            }
        }
    }
}

// Fixed: Added missing x parameter to match header declaration
void draw_char_with_bg(int x, int y, const uint8_t *char_bitmap, uint32_t fg_color, uint32_t bg_color) {
    if (!char_bitmap) return;
    
    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t byte = char_bitmap[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            int px = x + col;
            int py = y + row;
            if (px >= 0 && px < fb_width && py >= 0 && py < fb_height) {
                uint32_t color = (byte & (0x80 >> col)) ? fg_color : bg_color;
                fb_ptr[py * fb_pitch + px] = color;
            }
        }
    }
}

void draw_ascii_char(int x, int y, char c, uint32_t color) {
    if (c >= 32 && c <= 126) {
        draw_char(x, y, font8x16[(unsigned char)c - 32], color);
    }
}

void draw_ascii_char_with_bg(int x, int y, char c, uint32_t fg_color, uint32_t bg_color) {
    if (c >= 32 && c <= 126) {
        draw_char_with_bg(x, y, font8x16[(unsigned char)c - 32], fg_color, bg_color);
    } else {
        // Draw background for invalid characters
        for (int row = 0; row < FONT_HEIGHT; row++) {
            for (int col = 0; col < FONT_WIDTH; col++) {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < fb_width && py >= 0 && py < fb_height) {
                    fb_ptr[py * fb_pitch + px] = bg_color;
                }
            }
        }
    }
}

void draw_string(int x, int y, const char *str, uint32_t color) {
    int current_x = x;
    
    for (int i = 0; str[i]; i++) {
        draw_ascii_char(current_x, y, str[i], color);
        current_x += FONT_WIDTH;
    }
}

void draw_string_with_bg(int x, int y, const char *str, uint32_t fg_color, uint32_t bg_color) {
    int current_x = x;
    
    for (int i = 0; str[i]; i++) {
        draw_ascii_char_with_bg(current_x, y, str[i], fg_color, bg_color);
        current_x += FONT_WIDTH;
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

void clear_screen(uint32_t color) {
    for (int y = 0; y < fb_height; y++) {
        for (int x = 0; x < fb_width; x++) {
            fb_ptr[y * fb_pitch + x] = color;
        }
    }
}

void draw_rect(int x, int y, int width, int height, uint32_t color) {
    for (int py = y; py < y + height && py < fb_height; py++) {
        for (int px = x; px < x + width && px < fb_width; px++) {
            if (px >= 0 && py >= 0) {
                fb_ptr[py * fb_pitch + px] = color;
            }
        }
    }
}

void remove_rect(int x, int y, int width, int height, uint32_t bg_color) {
    for (int py = y; py < y + height && py < fb_height; py++) {
        for (int px = x; px < x + width && px < fb_width; px++) {
            if (px >= 0 && py >= 0) {
                fb_ptr[py * fb_pitch + px] = bg_color;
            }
        }
    }
}

uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b) {
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

// Additional utility functions
uint16_t get_cursor_x(void) {
    return cursor_position_x;
}

uint16_t get_cursor_y(void) {
    return cursor_position_y;
}

int get_screen_width(void) {
    return fb_width;
}

int get_screen_height(void) {
    return fb_height;
}