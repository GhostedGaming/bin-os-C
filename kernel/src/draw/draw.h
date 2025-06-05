#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

// Declarations only — no logic or framebuffer variables
void draw_char(uint32_t *framebuffer, int fb_width, int fb_pitch,
                    int x, int y, const uint8_t *char_bitmap, uint32_t color);

void draw_char_with_bg(uint32_t *framebuffer, int fb_width, int fb_pitch,
                    int x, int y, const uint8_t *char_bitmap, uint32_t fg_color, uint32_t bg_color);

void draw_ascii_char(uint32_t *framebuffer, int fb_width, int fb_pitch,
                    int x, int y, char c, uint32_t color);

void draw_ascii_char_with_bg(uint32_t *framebuffer, int fb_width, int fb_pitch,
                    int x, int y, char c, uint32_t fg_color, uint32_t bg_color);

void draw_string(uint32_t *framebuffer, int fb_width, int fb_pitch,
                    int x, int y, const char *str, uint32_t color);

void draw_string_with_bg(uint32_t *framebuffer, int fb_width, int fb_pitch,
                    int x, int y, const char *str, uint32_t fg_color, uint32_t bg_color);

void draw_string_centered(uint32_t *framebuffer, int fb_width, int fb_height, int fb_pitch,
                    int y, const char *str, uint32_t color);

void draw_string_centered_with_bg(uint32_t *framebuffer, int fb_width, int fb_height, int fb_pitch,
                    int y, const char *str, uint32_t fg_color, uint32_t bg_color);

void draw_string_center_screen(uint32_t *framebuffer, int fb_width, int fb_height, int fb_pitch,
                    const char *str, uint32_t color);

void draw_string_center_screen_with_bg(uint32_t *framebuffer, int fb_width, int fb_height, int fb_pitch,
                    const char *str, uint32_t fg_color, uint32_t bg_color);

uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b);

#endif // DRAW_H
