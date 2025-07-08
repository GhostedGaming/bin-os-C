#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>
#include <limine.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

extern struct {
    uint64_t x;
    uint64_t y;
} cursor;

// External declarations for global framebuffer variables
extern struct limine_framebuffer *framebuffer;
extern uint32_t *fb_ptr;
extern int fb_width;
extern int fb_height;
extern int fb_pitch;

// Utility functions
void hcf(void);
void init_framebuffer(void);
int get_screen_width(void);
int get_screen_height(void);

// Drawing functions - now use global framebuffer automatically
void draw_char(int x, int y, const uint8_t *char_bitmap, uint32_t color);
void draw_char_with_bg(int x, int y, const uint8_t *char_bitmap, uint32_t fg_color, uint32_t bg_color);
void draw_ascii_char(int x, int y, char c, uint32_t color);
void draw_ascii_char_with_bg(int x, int y, char c, uint32_t fg_color, uint32_t bg_color);
void draw_string(int x, int y, const char *str, uint32_t color);
void draw_string_with_bg(int x, int y, const char *str, uint32_t fg_color, uint32_t bg_color);
void draw_string_centered(int y, const char *str, uint32_t color);
void draw_string_centered_with_bg(int y, const char *str, uint32_t fg_color, uint32_t bg_color);
void draw_string_center_screen(const char *str, uint32_t color);
void draw_string_center_screen_with_bg(const char *str, uint32_t fg_color, uint32_t bg_color);
void clear_screen(uint32_t color);
void draw_rect(int x, int y, int width, int height, uint32_t color);
void remove_rect(int x, int y, int width, int height, uint32_t bg_color);

uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b);

#endif // DRAW_H