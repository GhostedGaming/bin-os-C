#include "../../serial/serial.h"
#include "../../io.h"
#include "../../draw/draw.h"
#include "../keyboard/keyboard.h"
#include "../interrupts/idt.h"
#include "stdbool.h"
#include "keyboard.h"

// Keyboard state variables
static bool caps_lock_on = false;
static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;

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

// Scancode to character mapping (US QWERTY layout)
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
    // 0x60-0x7F (fill rest with UNKNOWN)
    [0x60 ... 0x7F] = UNKNOWN
};

// Shifted character mappings
static const char shift_map[128] = {
    ['1'] = '!', ['2'] = '@', ['3'] = '#', ['4'] = '$', ['5'] = '%',
    ['6'] = '^', ['7'] = '&', ['8'] = '*', ['9'] = '(', ['0'] = ')',
    ['-'] = '_', ['='] = '+', ['['] = '{', [']'] = '}', ['\\'] = '|',
    [';'] = ':', ['\''] = '"', ['`'] = '~', [','] = '<', ['.'] = '>',
    ['/'] = '?'
};

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
    
    serial_printf("Keyboard initialized\r\n");
    
    init_keyboard_irq();
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
            serial_printf("[ESC]\r\n");
            break;
            
        case F1: case F2: case F3: case F4: case F5: case F6:
        case F7: case F8: case F9: case F10: case F11: case F12:
            serial_printf("[F%d]\r\n", (int)(F1 - key + 1));
            break;
            
        case HOME:
            serial_printf("[HOME]\r\n");
            break;
            
        case END:
            serial_printf("[END]\r\n");
            break;
            
        case PGUP:
            serial_printf("[PAGE UP]\r\n");
            break;
            
        case PGDOWN:
            serial_printf("[PAGE DOWN]\r\n");
            break;
            
        case UP:
            serial_printf("[UP ARROW]\r\n");
            break;
            
        case DOWN:
            serial_printf("[DOWN ARROW]\r\n");
            break;
            
        case LEFT:
            serial_printf("[LEFT ARROW]\r\n");
            break;
            
        case RIGHT:
            serial_printf("[RIGHT ARROW]\r\n");
            break;
            
        case INSERT:
            serial_printf("[INSERT]\r\n");
            break;
            
        case DELETE:
            serial_printf("[DELETE]\r\n");
            break;
            
        case '\b':
            serial_printf("[BACKSPACE]\r\n");
            break;
            
        case '\t':
            serial_printf("[TAB]\r\n");
            break;
            
        case '\n':
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
                        serial_printf("[CTRL+%c]\r\n", c);
                    } else if (alt_pressed) {
                        serial_printf("[ALT+%c]\r\n", c);
                    } else {
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