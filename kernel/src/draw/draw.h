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

typedef struct {
    uint16_t type;        // "BM" 
    uint32_t size;        // File size
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;      // Offset to pixel data
} __attribute__((packed)) bmp_header_t;

typedef struct {
    uint32_t size;        // Header size
    int32_t width;        // Image width
    int32_t height;       // Image height
    uint16_t planes;      // Must be 1
    uint16_t bits;        // Bits per pixel
    uint32_t compression; // Compression type
    uint32_t imagesize;   // Image size
    int32_t xresolution;  // Pixels per meter
    int32_t yresolution;  // Pixels per meter
    uint32_t ncolours;    // Number of colours
    uint32_t importantcolours; // Important colours
} __attribute__((packed)) bmp_info_t;

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

void set_cursor_pos(uint16_t x, uint16_t y);
void move_cursor_by_chars(int chars);
void move_cursor_by_lines(int lines);
void cursor_newline(void);
uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b);
uint16_t get_cursor_x(void);
uint16_t get_cursor_y(void);
uint16_t move_cursor_right(uint16_t amount);
uint16_t move_cursor_up(uint16_t amount);
uint16_t move_cursor_down(uint16_t amount);
uint16_t move_cursor_left(uint16_t amount);
void move_cursor_to(uint16_t x, uint16_t y);
int get_screen_width(void);
int get_screen_height(void);

#endif // DRAW_H