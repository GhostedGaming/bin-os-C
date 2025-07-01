#include "../../serial/serial.h"
#include "../../io.h"
#include "../../draw/draw.h"
#include "../../font.h"
#include "../keyboard/keyboard.h"
#include "../interrupts/idt.h"
#include "stdbool.h"
#include "keyboard.h"

// Keyboard state variables
static bool caps_lock_on = false;
static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;

// Terminal cursor position
static uint16_t term_cursor_x = 0;
static uint16_t term_cursor_y = 0;

// Define colors
#define COLOR_WHITE     rgb_to_color(255, 255, 255)
#define COLOR_BLACK     rgb_to_color(0, 0, 0)
#define COLOR_GREEN     rgb_to_color(0, 255, 0)
#define COLOR_RED       rgb_to_color(255, 0, 0)
#define COLOR_BLUE      rgb_to_color(0, 0, 255)
#define COLOR_YELLOW    rgb_to_color(0, 255, 255)

// Terminal dimensions
static int term_width_chars = 0;
static int term_height_chars = 0;

// Special key codes
#define UNKNOWN 0xFFFFFFFF
#define ESC     0xFFFFFFFF - 1
#define CTRL    0xFFFFFFFF - 2
#define LSHIFT  0xFFFFFFFF - 3
#define RSHIFT  0xFFFFFFFF - 4
#define ALT     0xFFFFFFFF - 5
#define CAPS    0xFFFFFFFF - 6
#define F1      0xFFFFFFFF - 7
#define F2      0xFFFFFFFF - 8
#define F3      0xFFFFFFFF - 9
#define F4      0xFFFFFFFF - 10
#define F5      0xFFFFFFFF - 11
#define F6      0xFFFFFFFF - 12
#define F7      0xFFFFFFFF - 13
#define F8      0xFFFFFFFF - 14
#define F9      0xFFFFFFFF - 15
#define F10     0xFFFFFFFF - 16
#define F11     0xFFFFFFFF - 17
#define F12     0xFFFFFFFF - 18
#define HOME    0xFFFFFFFF - 19
#define END     0xFFFFFFFF - 20
#define PGUP    0xFFFFFFFF - 21
#define PGDOWN  0xFFFFFFFF - 22
#define UP      0xFFFFFFFF - 23
#define DOWN    0xFFFFFFFF - 24
#define LEFT    0xFFFFFFFF - 25
#define RIGHT   0xFFFFFFFF - 26
#define INSERT  0xFFFFFFFF - 27
#define DELETE  0xFFFFFFFF - 28

static const uint32_t scancode_map[128] = {
    // 0x00-0x0F
    UNKNOWN, ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    // 0x10-0x1F
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', CTRL, 'a', 's',
    // 0x20-0x2F
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', LSHIFT, '\\', 'z', 'x', 'c', 'v',
    // 0x30-0x3F
    'b', 'n', 'm', ',', '.', '/', RSHIFT, '*', ALT, ' ', CAPS, F1, F2, F3, F4, F5,
    // 0x40-0x4F
    F6, F7, F8, F9, F10, UNKNOWN, UNKNOWN, HOME, UP, PGUP, '-', LEFT, UNKNOWN, RIGHT, '+', END,
    // 0x50-0x5F
    DOWN, PGDOWN, INSERT, DELETE, UNKNOWN, UNKNOWN, UNKNOWN, F11, F12, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
    [0x60 ... 0x7F] = UNKNOWN
};

static const char shift_map[128] = {
    ['1'] = '!', ['2'] = '@', ['3'] = '#', ['4'] = '$', ['5'] = '%',
    ['6'] = '^', ['7'] = '&', ['8'] = '*', ['9'] = '(', ['0'] = ')',
    ['-'] = '_', ['='] = '+', ['['] = '{', [']'] = '}', ['\\'] = '|',
    [';'] = ':', ['\''] = '"', ['`'] = '~', [','] = '<', ['.'] = '>',
    ['/'] = '?'
};

static void scroll_screen_up(void) {
    if (term_cursor_y >= term_height_chars - 1) {
        clear_screen(COLOR_BLACK);
        term_cursor_x = 0;
        term_cursor_y = 0;
    }
}

static void advance_cursor(void) {
    term_cursor_x++;
    if (term_cursor_x >= term_width_chars) {
        term_cursor_x = 0;
        term_cursor_y++;
        if (term_cursor_y >= term_height_chars) {
            scroll_screen_up();
        }
    }
}

static void handle_newline(void) {
    term_cursor_x = 0;
    term_cursor_y++;
    if (term_cursor_y >= term_height_chars) {
        scroll_screen_up();
    }
}

static void handle_backspace(void) {
    if (term_cursor_x > 0) {
        term_cursor_x--;
        // Clear the character at the current position
        int pixel_x = term_cursor_x * FONT_WIDTH;
        int pixel_y = term_cursor_y * FONT_HEIGHT;
        draw_ascii_char_with_bg(pixel_x, pixel_y, ' ', COLOR_WHITE, COLOR_BLACK);
    } else if (term_cursor_y > 0) {
        // Move to end of previous line
        term_cursor_y--;
        term_cursor_x = term_width_chars - 1;
        // Clear the character at the current position
        int pixel_x = term_cursor_x * FONT_WIDTH;
        int pixel_y = term_cursor_y * FONT_HEIGHT;
        draw_ascii_char_with_bg(pixel_x, pixel_y, ' ', COLOR_WHITE, COLOR_BLACK);
    }
}

static void handle_tab(void) {
    int spaces = 4 - (term_cursor_x % 4);
    for (int i = 0; i < spaces; i++) {
        if (term_cursor_x < term_width_chars) {
            int pixel_x = term_cursor_x * FONT_WIDTH;
            int pixel_y = term_cursor_y * FONT_HEIGHT;
            draw_ascii_char(pixel_x, pixel_y, ' ', COLOR_WHITE);
            advance_cursor();
        }
    }
}

static void draw_terminal_char(char c, uint32_t color) {
    int pixel_x = term_cursor_x * FONT_WIDTH;
    int pixel_y = term_cursor_y * FONT_HEIGHT;
    draw_ascii_char(pixel_x, pixel_y, c, color);
    advance_cursor();
}

// Function to get character based on current keyboard state
static char get_character(uint32_t key) {
    if (key >= 256) return 0; // Not a printable character
    
    char c = (char)key;
    bool should_uppercase = false;
    
    // Determine if we should use uppercase
    if (c >= 'a' && c <= 'z') {
        should_uppercase = caps_lock_on ^ shift_pressed;
        if (should_uppercase) {
            c = c - 'a' + 'A';
        }
    } else if (shift_pressed && shift_map[c]) {
        c = shift_map[c];
    }
    
    return c;
}

void init_keyboard() {
    // Initialize keyboard state
    caps_lock_on = false;
    shift_pressed = false;
    ctrl_pressed = false;
    alt_pressed = false;
    
    // Initialize terminal dimensions
    term_width_chars = get_screen_width() / FONT_WIDTH;
    term_height_chars = get_screen_height() / FONT_HEIGHT;
    term_cursor_x = 0;
    term_cursor_y = 0;
    
    // Clear keyboard buffer
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }
    
    // Wait for keyboard to be ready
    while (inb(0x64) & 0x02);
    
    // Enable keyboard
    outb(0x64, 0xAE);
    
    // Wait for command to be processed
    while (inb(0x64) & 0x02);
    
    serial_printf("Keyboard initialized - Terminal size: %dx%d chars\r\n", 
                  term_width_chars, term_height_chars);
    
    enable_keyboard_irq();
}

void keyboard_handler(struct interrupt_registers *regs) {
    // Check if keyboard controller has data ready
    uint8_t status = inb(0x64);
    if (!(status & 0x01)) {
        return; // No data available
    }
    
    // Read scancode
    uint8_t scancode_raw = inb(0x60);
    uint8_t scancode = scancode_raw & 0x7F;
    bool key_pressed = !(scancode_raw & 0x80);
    
    // Bounds check
    if (scancode >= 128) {
        return;
    }
    
    uint32_t key = scancode_map[scancode];
    
    // Handle modifier keys
    switch (key) {
        case LSHIFT:
        case RSHIFT:
            shift_pressed = key_pressed;
            return;
            
        case CTRL:
            ctrl_pressed = key_pressed;
            return;
            
        case ALT:
            alt_pressed = key_pressed;
            return;
            
        case CAPS:
            if (key_pressed) {
                caps_lock_on = !caps_lock_on;
                serial_printf("[CAPS LOCK %s]\r\n", caps_lock_on ? "ON" : "OFF");
            }
            return;
    }
    
    // Only process key presses (not releases) for printable characters
    if (!key_pressed) {
        return;
    }
    
    // Handle special keys
    switch (key) {
        case ESC:
            // Draw ESC indicator
            draw_string(term_cursor_x * FONT_WIDTH, term_cursor_y * FONT_HEIGHT, "[ESC]", COLOR_YELLOW);
            advance_cursor();
            advance_cursor();
            advance_cursor();
            advance_cursor();
            advance_cursor();
            serial_printf("[ESC]\r\n");
            break;
            
        case F1: case F2: case F3: case F4: case F5: case F6:
        case F7: case F8: case F9: case F10: case F11: case F12: {
            int f_num = (int)(F1 - key + 1);
            char f_str[10];
            // Simple integer to string conversion
            if (f_num < 10) {
                f_str[0] = '[';
                f_str[1] = 'F';
                f_str[2] = '0' + f_num;
                f_str[3] = ']';
                f_str[4] = '\0';
            } else {
                f_str[0] = '[';
                f_str[1] = 'F';
                f_str[2] = '1';
                f_str[3] = '0' + (f_num - 10);
                f_str[4] = ']';
                f_str[5] = '\0';
            }
            draw_string(term_cursor_x * FONT_WIDTH, term_cursor_y * FONT_HEIGHT, f_str, COLOR_YELLOW);
            // Advance cursor for each character
            for (int i = 0; f_str[i]; i++) advance_cursor();
            serial_printf("[F%d]\r\n", f_num);
            break;
        }
            
        case HOME:
            term_cursor_x = 0;
            serial_printf("[HOME]\r\n");
            break;
            
        case END:
            term_cursor_x = term_width_chars - 1;
            serial_printf("[END]\r\n");
            break;
            
        case UP:
            if (term_cursor_y > 0) term_cursor_y--;
            serial_printf("[UP ARROW]\r\n");
            break;
            
        case DOWN:
            if (term_cursor_y < term_height_chars - 1) term_cursor_y++;
            serial_printf("[DOWN ARROW]\r\n");
            break;
            
        case LEFT:
            if (term_cursor_x > 0) {
                term_cursor_x--;
            } else if (term_cursor_y > 0) {
                term_cursor_y--;
                term_cursor_x = term_width_chars - 1;
            }
            serial_printf("[LEFT ARROW]\r\n");
            break;
            
        case RIGHT:
            if (term_cursor_x < term_width_chars - 1) {
                term_cursor_x++;
            } else if (term_cursor_y < term_height_chars - 1) {
                term_cursor_y++;
                term_cursor_x = 0;
            }
            serial_printf("[RIGHT ARROW]\r\n");
            break;
            
        case '\b':
            handle_backspace();
            serial_printf("[BACKSPACE]\r\n");
            break;
            
        case '\t':
            handle_tab();
            serial_printf("[TAB]\r\n");
            break;
            
        case '\n':
            handle_newline();
            serial_printf("\r\n");
            break;
            
        case UNKNOWN:
            serial_printf("[UNKNOWN KEY: scancode %d]\r\n", scancode);
            break;
            
        default:
            // Handle printable characters
            if (key < 256) {
                char c = get_character(key);
                if (c) {
                    // Handle control key combinations
                    if (ctrl_pressed) {
                        // Draw control combination indicator
                        char ctrl_str[10] = "[CTRL+";
                        ctrl_str[6] = c;
                        ctrl_str[7] = ']';
                        ctrl_str[8] = '\0';
                        draw_string(term_cursor_x * FONT_WIDTH, term_cursor_y * FONT_HEIGHT, ctrl_str, COLOR_RED);
                        for (int i = 0; ctrl_str[i]; i++) advance_cursor();
                        serial_printf("[CTRL+%c]\r\n", c);
                    } else if (alt_pressed) {
                        // Draw alt combination indicator
                        char alt_str[10] = "[ALT+";
                        alt_str[5] = c;
                        alt_str[6] = ']';
                        alt_str[7] = '\0';
                        draw_string(term_cursor_x * FONT_WIDTH, term_cursor_y * FONT_HEIGHT, alt_str, COLOR_BLUE);
                        for (int i = 0; alt_str[i]; i++) advance_cursor();
                        serial_printf("[ALT+%c]\r\n", c);
                    } else {
                        // Draw normal character
                        draw_terminal_char(c, COLOR_WHITE);
                        serial_printf("%c", c);
                    }
                }
            }
            break;
    }
}

// Utility functions for keyboard state
bool is_shift_pressed(void) {
    return shift_pressed;
}

bool is_ctrl_pressed(void) {
    return ctrl_pressed;
}

bool is_alt_pressed(void) {
    return alt_pressed;
}

bool is_caps_lock_on(void) {
    return caps_lock_on;
}

// Additional utility functions for terminal
void get_terminal_cursor_position(uint16_t *x, uint16_t *y) {
    *x = term_cursor_x;
    *y = term_cursor_y;
}

void set_terminal_cursor_position(uint16_t x, uint16_t y) {
    if (x < term_width_chars && y < term_height_chars) {
        term_cursor_x = x;
        term_cursor_y = y;
    }
}

void clear_terminal(void) {
    clear_screen(COLOR_BLACK);
    term_cursor_x = 0;
    term_cursor_y = 0;
}

void enable_keyboard_irq(void) {
    // Enable IRQ1 (keyboard) in PIC
    uint8_t mask = inb(0x21);
    mask &= ~(1 << 1); // Clear bit 1 to enable IRQ1
    outb(0x21, mask);
    
    serial_printf("Keyboard: IRQ1 enabled in PIC\r\n");
}