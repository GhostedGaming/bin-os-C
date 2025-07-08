#include <stddef.h>
#include "shell.h"
#include "../serial/serial.h"
#include "../draw/draw.h"

// Function declarations for custom string functions
int strcmp(const char *str1, const char *str2);
int strlen(const char *str);
char *strncpy(char *dest, const char *src, int n);

static char command_buffer[256];
static char input_buffer[256];
static int input_index = 0;

static int shell_x = 0;
static int shell_y = 0;
static const int line_height = 16;
static const int char_width = 8;
static const uint32_t text_color = 0xFFFFFFFF;
static const uint32_t bg_color = 0xFF000000;
static const uint32_t cursor_color = 0xFF00FF00;
static int cursor_blink_counter = 0;
static int cursor_visible = 1;

void shell_init(void) {
    shell_x = 0;
    shell_y = 0;
    input_index = 0;
    input_buffer[0] = '\0';
    command_buffer[0] = '\0';
    clear_screen(bg_color);
    shell_print_prompt();
}

void shell_print_prompt(void) {
    draw_string(shell_x, shell_y, "$ ", text_color);
    shell_x += 2 * char_width;
}

void shell_draw_input_line(void) {
    int prompt_x = shell_x - (2 * char_width);
    
    draw_rect(prompt_x, shell_y, get_screen_width() - prompt_x, line_height, bg_color);
    draw_string(prompt_x, shell_y, "$ ", text_color);
    
    int text_x = prompt_x + (2 * char_width);
    draw_string(text_x, shell_y, input_buffer, text_color);
    
    if (cursor_visible) {
        int cursor_x = text_x + (input_index * char_width);
        draw_rect(cursor_x, shell_y, char_width, line_height, cursor_color);
        
        if (input_index < strlen(input_buffer) && input_buffer[input_index] != '\0') {
            char cursor_char[2] = {input_buffer[input_index], '\0'};
            draw_string(cursor_x, shell_y, cursor_char, bg_color);
        }
    }
}

void shell_update_cursor(void) {
    cursor_blink_counter++;
    if (cursor_blink_counter >= 30) {
        cursor_visible = !cursor_visible;
        cursor_blink_counter = 0;
        shell_draw_input_line();
    }
}

void shell_newline(void) {
    shell_y += line_height;
    shell_x = 0;

    if (shell_y >= get_screen_height() - line_height) {
        shell_scroll_up();
    }
}

void shell_scroll_up(void) {
    uint32_t *fb = (uint32_t*)fb_ptr;
    int width = get_screen_width();
    int height = get_screen_height();
    int pitch = fb_pitch / sizeof(uint32_t);
    
    for (int y = 0; y < height - line_height; y++) {
        for (int x = 0; x < width; x++) {
            fb[y * pitch + x] = fb[(y + line_height) * pitch + x];
        }
    }
    
    for (int y = height - line_height; y < height; y++) {
        for (int x = 0; x < width; x++) {
            fb[y * pitch + x] = bg_color;
        }
    }
    
    shell_y -= line_height;
}

void shell_print(const char *text) {
    if (!text) return;
    
    while (*text) {
        if (*text == '\n') {
            shell_newline();
        } else if (*text == '\r') {
            shell_x = 0;
        } else if (*text == '\t') {
            int tab_size = 4;
            int spaces_needed = tab_size - (shell_x / char_width) % tab_size;
            for (int i = 0; i < spaces_needed; i++) {
                draw_ascii_char(shell_x, shell_y, ' ', text_color);
                shell_x += char_width;
                if (shell_x >= get_screen_width() - char_width) {
                    shell_newline();
                    break;
                }
            }
        } else {
            draw_ascii_char(shell_x, shell_y, *text, text_color);
            shell_x += char_width;
            
            if (shell_x >= get_screen_width() - char_width) {
                shell_newline();
            }
        }
        text++;
    }
}

void shell_backspace(void) {
    if (input_index > 0) {
        input_index--;
        input_buffer[input_index] = '\0';
        shell_draw_input_line();
    }
}

void shell_clear_input(void) {
    input_index = 0;
    input_buffer[0] = '\0';
    shell_draw_input_line();
}

char *input(char received) {
    if (received == '\n' || received == '\r') {
        input_buffer[input_index] = '\0';
        
        strncpy(command_buffer, input_buffer, sizeof(command_buffer) - 1);
        command_buffer[sizeof(command_buffer) - 1] = '\0';
        
        shell_newline();
        shell_clear_input();
        
        return command_buffer;
    }
    else if (received == '\b' || received == 127) {
        shell_backspace();
        return NULL;
    }
    else if (received == 3) {
        shell_cancel_input();
        return NULL;
    }
    else if (received >= 32 && received <= 126 && input_index < (int)(sizeof(input_buffer) - 1)) {
        input_buffer[input_index] = received;
        input_index++;
        input_buffer[input_index] = '\0';
        shell_draw_input_line();
        return NULL;
    }
    return NULL;
}

int strcmp(const char *str1, const char *str2) {
    if (!str1 || !str2) return str1 ? 1 : (str2 ? -1 : 0);
    
    while (*str1 && *str2 && *str1 == *str2) {
        str1++;
        str2++;
    }
    return (unsigned char)*str1 - (unsigned char)*str2;
}

int strlen(const char *str) {
    if (!str) return 0;
    
    int len = 0;
    while (str[len]) len++;
    return len;
}

char *strncpy(char *dest, const char *src, int n) {
    if (!dest || !src) return dest;
    
    int i;
    for (i = 0; i < n && src[i]; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

void shell_cancel_input(void) {
    shell_clear_input();
    shell_newline();
    shell_print("^C\n");
    shell_print_prompt();
    shell_draw_input_line();
}

void parse_command(void) {
    if (command_buffer[0] == '\0') {
        shell_print_prompt();
        shell_draw_input_line();
        return;
    }
    
    if (strcmp(command_buffer, "hello") == 0) {
        shell_print("Hello world!\n");
    } else if (strcmp(command_buffer, "help") == 0) {
        shell_print("Available commands:\n");
        shell_print("  hello  - Display greeting\n");
        shell_print("  help   - Show this help message\n");
        shell_print("  clear  - Clear the screen\n");
        shell_print("  exit   - Exit the shell\n");
    } else if (strcmp(command_buffer, "clear") == 0) {
        clear_screen(bg_color);
        shell_x = 0;
        shell_y = 0;
    } else if (strcmp(command_buffer, "exit") == 0) {
        shell_print("Goodbye!\n");
        return;
    } else {
        shell_print("Unknown command: ");
        shell_print(command_buffer);
        shell_print("\nType 'help' for available commands.\n");
    }
    
    command_buffer[0] = '\0';
    shell_print_prompt();
    shell_draw_input_line();
}