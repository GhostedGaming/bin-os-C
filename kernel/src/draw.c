#include "draw.h"
#include "font.h"

static int string_length(const char *str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

void draw_char(uint32_t *framebuffer, int fb_width, int fb_pitch,
                                int x, int y, const uint8_t *char_bitmap, uint32_t color) {
    if (!char_bitmap) return;

    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t byte = char_bitmap[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            if (byte & (0x80 >> col)) {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < fb_width && py >= 0) {
                    framebuffer[py * fb_pitch + px] = color;
                }
            }
        }
    }
}

void draw_char_with_bg(uint32_t *framebuffer, int fb_width, int fb_pitch,
                                int x, int y, const uint8_t *char_bitmap,
                                uint32_t fg_color, uint32_t bg_color) {
    if (!char_bitmap) return;

    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t byte = char_bitmap[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            int px = x + col;
            int py = y + row;
            if (px >= 0 && px < fb_width && py >= 0) {
                uint32_t color = (byte & (0x80 >> col)) ? fg_color : bg_color;
                framebuffer[py * fb_pitch + px] = color;
            }
        }
    }
}

void draw_ascii_char(uint32_t *framebuffer, int fb_width, int fb_pitch,
                                int x, int y, char c, uint32_t color) {
    if (c >= 0 && c < 128 && font_table[c]) {
        draw_char(framebuffer, fb_width, fb_pitch, x, y, font_table[c], color);
    }
    // For space character or unsupported character, don't draw anything (transparent)
}

void draw_ascii_char_with_bg(uint32_t *framebuffer, int fb_width, int fb_pitch,
                                int x, int y, char c, uint32_t fg_color, uint32_t bg_color) {
    if (c >= 0 && c < 128 && font_table[c]) {
        draw_char_with_bg(framebuffer, fb_width, fb_pitch, x, y, font_table[c], fg_color, bg_color);
    } else {
        // Fill space character or unsupported character with background color
        for (int row = 0; row < FONT_HEIGHT; row++) {
            for (int col = 0; col < FONT_WIDTH; col++) {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < fb_width && py >= 0) {
                    framebuffer[py * fb_pitch + px] = bg_color;
                }
            }
        }
    }
}

void draw_string(uint32_t *framebuffer, int fb_width, int fb_pitch,
                                int x, int y, const char *str, uint32_t color) {
    int cx = x;
    for (int i = 0; str[i]; i++) {
        draw_ascii_char(framebuffer, fb_width, fb_pitch, cx, y, str[i], color);
        cx += FONT_WIDTH;
    }
}

void draw_string_with_bg(uint32_t *framebuffer, int fb_width, int fb_pitch,
                                int x, int y, const char *str, uint32_t fg_color, uint32_t bg_color) {
    int cx = x;
    for (int i = 0; str[i]; i++) {
        draw_ascii_char_with_bg(framebuffer, fb_width, fb_pitch, cx, y, str[i], fg_color, bg_color);
        cx += FONT_WIDTH;
    }
}

void draw_string_centered(uint32_t *framebuffer, int fb_width, int fb_height, int fb_pitch,
                                int y, const char *str, uint32_t color) {
    int len = string_length(str);
    int str_width = len * FONT_WIDTH;
    int cx = (fb_width - str_width) / 2;
    draw_string(framebuffer, fb_width, fb_pitch, cx, y, str, color);
}

void draw_string_centered_with_bg(uint32_t *framebuffer, int fb_width, int fb_height, int fb_pitch,
                                int y, const char *str, uint32_t fg_color, uint32_t bg_color) {
    int len = string_length(str);
    int str_width = len * FONT_WIDTH;
    int cx = (fb_width - str_width) / 2;
    draw_string_with_bg(framebuffer, fb_width, fb_pitch, cx, y, str, fg_color, bg_color);
}

void draw_string_center_screen(uint32_t *framebuffer, int fb_width, int fb_height, int fb_pitch,
                                const char *str, uint32_t color) {
    int len = string_length(str);
    int str_width = len * FONT_WIDTH;
    int cx = (fb_width - str_width) / 2;
    int cy = (fb_height - FONT_HEIGHT) / 2;
    draw_string(framebuffer, fb_width, fb_pitch, cx, cy, str, color);
}

void draw_string_center_screen_with_bg(uint32_t *framebuffer, int fb_width, int fb_height, int fb_pitch,
                                const char *str, uint32_t fg_color, uint32_t bg_color) {
    int len = string_length(str);
    int str_width = len * FONT_WIDTH;
    int cx = (fb_width - str_width) / 2;
    int cy = (fb_height - FONT_HEIGHT) / 2;
    draw_string_with_bg(framebuffer, fb_width, fb_pitch, cx, cy, str, fg_color, bg_color);
}