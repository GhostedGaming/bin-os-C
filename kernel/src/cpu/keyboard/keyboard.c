#include <stddef.h>
#include "../../serial/serial.h"
#include "../../io.h"
#include "../../draw/draw.h"
#include "../../font.h"
#include "../keyboard/keyboard.h"
#include "../interrupts/idt.h"
#include "stdbool.h"
#include "keyboard.h"
#include "../../pc_speaker/speaker.h"
#include "../../shell/shell.h"

#define KB_DATA_PORT    0x60
#define KB_STATUS_PORT  0x64
#define KB_COMMAND_PORT 0x64

#define KB_STATUS_OUTPUT_FULL   0x01
#define KB_STATUS_INPUT_FULL    0x02
#define KB_ENABLE_KEYBOARD      0xAE

#define COLOR_WHITE     rgb_to_color(255, 255, 255)
#define COLOR_BLACK     rgb_to_color(0, 0, 0)
#define COLOR_GREEN     rgb_to_color(0, 255, 0)
#define COLOR_RED       rgb_to_color(255, 0, 0)
#define COLOR_BLUE      rgb_to_color(0, 0, 255)
#define COLOR_YELLOW    rgb_to_color(255, 255, 0)

typedef enum {
    KEY_UNKNOWN = 0,
    KEY_ESC = 1,
    KEY_CTRL = 2,
    KEY_LSHIFT = 3,
    KEY_RSHIFT = 4,
    KEY_ALT = 5,
    KEY_CAPS = 6,
    KEY_F1 = 128,    // Start function keys at 128 to avoid conflicts with ASCII
    KEY_F2 = 129,
    KEY_F3 = 130,
    KEY_F4 = 131,
    KEY_F5 = 132,
    KEY_F6 = 133,
    KEY_F7 = 134,
    KEY_F8 = 135,
    KEY_F9 = 136,
    KEY_F10 = 137,
    KEY_F11 = 138,
    KEY_F12 = 139,
    KEY_HOME = 140,
    KEY_END = 141,
    KEY_PGUP = 142,
    KEY_PGDOWN = 143,
    KEY_UP = 144,
    KEY_DOWN = 145,
    KEY_LEFT = 146,
    KEY_RIGHT = 147,
    KEY_INSERT = 148,
    KEY_DELETE = 149,
    KEY_BACKSPACE = '\b',    // 8
    KEY_TAB = '\t',          // 9 - now no conflict
    KEY_ENTER = '\n',        // 10
    KEY_SPACE = ' '          // 32
} special_key_t;

static bool caps_lock_on = false;
static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static bool extended_scancode = false;

static const uint8_t scancode_map[128] = {
    0, KEY_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', KEY_BACKSPACE, KEY_TAB,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', KEY_ENTER, KEY_CTRL, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', KEY_LSHIFT, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', KEY_RSHIFT, '*', KEY_ALT, KEY_SPACE, KEY_CAPS, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, 0, 0, KEY_HOME, KEY_UP, KEY_PGUP, '-', KEY_LEFT, 0, KEY_RIGHT, '+', KEY_END,
    KEY_DOWN, KEY_PGDOWN, KEY_INSERT, KEY_DELETE, 0, 0, 0, KEY_F11, KEY_F12
};

static const uint8_t extended_scancode_map[128] = {
    [0x48] = KEY_UP,
    [0x50] = KEY_DOWN,
    [0x4B] = KEY_LEFT,
    [0x4D] = KEY_RIGHT,
    [0x47] = KEY_HOME,
    [0x4F] = KEY_END,
    [0x49] = KEY_PGUP,
    [0x51] = KEY_PGDOWN,
    [0x52] = KEY_INSERT,
    [0x53] = KEY_DELETE,
    [0x1C] = KEY_ENTER,
    [0x1D] = KEY_CTRL,
    [0x38] = KEY_ALT
};

static const char shift_map[128] = {
    ['1'] = '!', ['2'] = '@', ['3'] = '#', ['4'] = '$', ['5'] = '%',
    ['6'] = '^', ['7'] = '&', ['8'] = '*', ['9'] = '(', ['0'] = ')',
    ['-'] = '_', ['='] = '+', ['['] = '{', [']'] = '}', ['\\'] = '|',
    [';'] = ':', ['\''] = '"', ['`'] = '~', [','] = '<', ['.'] = '>',
    ['/'] = '?'
};

static void keyboard_wait_input(void) {
    while (inb(KB_STATUS_PORT) & KB_STATUS_INPUT_FULL);
}

// Remove unused function warning by commenting out or removing
// static void keyboard_wait_output(void) {
//     while (!(inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT_FULL));
// }

static char get_character(uint8_t key) {
    if (key == 0 || key >= 128) return 0;
    
    char c = (char)key;
    
    if (c >= 'a' && c <= 'z') {
        bool should_uppercase = caps_lock_on ^ shift_pressed;
        if (should_uppercase) {
            c = c - 'a' + 'A';
        }
    } else if (shift_pressed && shift_map[(unsigned char)c]) {  // Cast to unsigned char to fix warning
        c = shift_map[(unsigned char)c];                        // Cast to unsigned char to fix warning
    }
    
    return c;
}

static void handle_special_key(uint8_t key, bool pressed) {
    if (!pressed) return;
    
    switch (key) {
        case KEY_ESC:
            serial_printf("[ESC]\r\n");
            break;
            
        case KEY_F1: case KEY_F2: case KEY_F3: case KEY_F4: case KEY_F5: case KEY_F6:
        case KEY_F7: case KEY_F8: case KEY_F9: case KEY_F10: case KEY_F11: case KEY_F12: {
            int f_num = key - KEY_F1 + 1;
            serial_printf("[F%d]\r\n", f_num);
            break;
        }
            
        case KEY_HOME:
            serial_printf("[HOME]\r\n");
            break;
            
        case KEY_END:
            serial_printf("[END]\r\n");
            break;
            
        case KEY_UP:
            serial_printf("[UP]\r\n");
            break;
            
        case KEY_DOWN:
            serial_printf("[DOWN]\r\n");
            break;
            
        case KEY_LEFT:
            serial_printf("[LEFT]\r\n");
            break;
            
        case KEY_RIGHT:
            serial_printf("[RIGHT]\r\n");
            break;
            
        case KEY_INSERT:
            serial_printf("[INSERT]\r\n");
            break;
            
        case KEY_DELETE:
            serial_printf("[DELETE]\r\n");
            break;
            
        case KEY_PGUP:
            serial_printf("[PGUP]\r\n");
            break;
            
        case KEY_PGDOWN:
            serial_printf("[PGDOWN]\r\n");
            break;
            
        case KEY_TAB:
            serial_printf("[TAB]\r\n");
            break;
            
        default:
            break;
    }
}

static void handle_modifier_key(uint8_t key, bool pressed) {
    switch (key) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            shift_pressed = pressed;
            break;
            
        case KEY_CTRL:
            ctrl_pressed = pressed;
            break;
            
        case KEY_ALT:
            alt_pressed = pressed;
            break;
            
        case KEY_CAPS:
            if (pressed) {
                caps_lock_on = !caps_lock_on;
                serial_printf("[CAPS LOCK %s]\r\n", caps_lock_on ? "ON" : "OFF");
            }
            break;
    }
}

static void handle_printable_key(uint8_t key, bool pressed) {
    if (!pressed) return;
    
    char c = get_character(key);
    if (c == 0) return;
    
    if (ctrl_pressed) {
        serial_printf("[CTRL+%c]\r\n", c);
        if (c == 'c' || c == 'C') {
            shell_cancel_input();
        }
    } else if (alt_pressed) {
        serial_printf("[ALT+%c]\r\n", c);
    } else {
        char *command = input(c);
        if (command) {
            parse_command();
        }
        serial_printf("%c", c);
    }
}

void init_keyboard(void) {
    caps_lock_on = false;
    shift_pressed = false;
    ctrl_pressed = false;
    alt_pressed = false;
    extended_scancode = false;
    
    while (inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT_FULL) {
        inb(KB_DATA_PORT);
    }
    
    keyboard_wait_input();
    outb(KB_COMMAND_PORT, KB_ENABLE_KEYBOARD);
    keyboard_wait_input();
    
    serial_printf("Keyboard initialized\r\n");
    enable_keyboard_irq();
}

void keyboard_handler(struct interrupt_registers *regs) {
    (void)regs;  // Suppress unused parameter warning
    
    if (!(inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT_FULL)) {
        return;
    }
    
    uint8_t scancode_raw = inb(KB_DATA_PORT);
    
    if (scancode_raw == 0xE0) {
        extended_scancode = true;
        return;
    }
    
    uint8_t scancode = scancode_raw & 0x7F;
    bool key_pressed = !(scancode_raw & 0x80);
    
    if (scancode >= 128) {
        extended_scancode = false;
        return;
    }
    
    uint8_t key;
    if (extended_scancode) {
        key = extended_scancode_map[scancode];
        extended_scancode = false;
    } else {
        key = scancode_map[scancode];
    }
    
    if (key == 0) return;
    
    if (key == KEY_LSHIFT || key == KEY_RSHIFT || key == KEY_CTRL || key == KEY_ALT || key == KEY_CAPS) {
        handle_modifier_key(key, key_pressed);
    } else if (key == KEY_BACKSPACE) {
        if (key_pressed) {
            char *result = input('\b');
            if (result) {
                parse_command();
            }
            serial_printf("[BACKSPACE]\r\n");
        }
    } else if (key == KEY_ENTER) {
        if (key_pressed) {
            char *command = input('\n');
            if (command) {
                parse_command();
            }
            serial_printf("\r\n");
        }
    } else if (key >= KEY_F1 && key <= KEY_DELETE) {
        handle_special_key(key, key_pressed);
    } else if (key >= 32 && key <= 126) {
        handle_printable_key(key, key_pressed);
    }
}

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

void enable_keyboard_irq(void) {
    uint8_t mask = inb(0x21);
    mask &= ~(1 << 1);
    outb(0x21, mask);
    serial_printf("Keyboard: IRQ1 enabled in PIC\r\n");
}