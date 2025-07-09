#include <stddef.h>
#include "shell.h"
#include "../serial/serial.h"
#include "../draw/draw.h"
#include "../memory.h"

int strcmp(const char *str1, const char *str2);
int strlen(const char *str);
char *strncpy(char *dest, const char *src, int n);
char *strcpy(char *dest, const char *src);
void *memset(void *s, int c, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);
void parse_args(const char *command, char args[][32], int *argc);

// Add forward declaration for shell_print_welcome
void shell_print_welcome(void);

// Command function declarations
void cmd_help(int argc, char args[][32]);
void cmd_hello(int argc, char args[][32]);
void cmd_clear(int argc, char args[][32]);
void cmd_echo(int argc, char args[][32]);
void cmd_history(int argc, char args[][32]);
void cmd_uptime(int argc, char args[][32]);
void cmd_exit(int argc, char args[][32]);

static char command_buffer[256];
static char input_buffer[256];
static char history[10][256];
static int history_index = 0;
static int history_current = 0;
static int input_index = 0;
static int cursor_pos = 0;

static int shell_x = 0;
static int shell_y = 0;
static const int line_height = 16;
static const int char_width = 8;
static const uint32_t text_color = 0xFFFFFFFF;
static const uint32_t bg_color = 0xFF000000;
static const uint32_t cursor_color = 0xFF00FF00;
static const uint32_t error_color = 0xFFFF0000;
static const uint32_t success_color = 0xFF00FF00;
static int cursor_blink_counter = 0;
static int cursor_visible = 1;

void shell_init(void) {
    shell_x = 0;
    shell_y = 0;
    input_index = 0;
    cursor_pos = 0;
    history_index = 0;
    history_current = 0;
    input_buffer[0] = '\0';
    command_buffer[0] = '\0';
    memset(history, 0, sizeof(history));
    clear_screen(bg_color);
    shell_print_welcome();
    shell_print_prompt();
}

void shell_print_welcome(void) {
    shell_print("SimpleOS Shell v1.0\n");
    shell_print("Type 'help' for available commands.\n\n");
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
        int cursor_x = text_x + (cursor_pos * char_width);
        draw_rect(cursor_x, shell_y, char_width, line_height, cursor_color);
        
        if (cursor_pos < input_index && input_buffer[cursor_pos] != '\0') {
            char cursor_char[2] = {input_buffer[cursor_pos], '\0'};
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

void shell_print_color(const char *text, uint32_t color) {
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
                draw_ascii_char(shell_x, shell_y, ' ', color);
                shell_x += char_width;
                if (shell_x >= get_screen_width() - char_width) {
                    shell_newline();
                    break;
                }
            }
        } else {
            draw_ascii_char(shell_x, shell_y, *text, color);
            shell_x += char_width;
            
            if (shell_x >= get_screen_width() - char_width) {
                shell_newline();
            }
        }
        text++;
    }
}

void shell_print(const char *text) {
    shell_print_color(text, text_color);
}

void shell_error(const char *text) {
    shell_print_color("Error: ", error_color);
    shell_print(text);
    shell_print("\n");
}

void shell_success(const char *text) {
    shell_print_color(text, success_color);
    shell_print("\n");
}

void shell_backspace(void) {
    if (cursor_pos > 0) {
        for (int i = cursor_pos - 1; i < input_index; i++) {
            input_buffer[i] = input_buffer[i + 1];
        }
        cursor_pos--;
        input_index--;
        input_buffer[input_index] = '\0';
        shell_draw_input_line();
    }
}

void shell_delete(void) {
    if (cursor_pos < input_index) {
        for (int i = cursor_pos; i < input_index; i++) {
            input_buffer[i] = input_buffer[i + 1];
        }
        input_index--;
        input_buffer[input_index] = '\0';
        shell_draw_input_line();
    }
}

void shell_move_cursor_left(void) {
    if (cursor_pos > 0) {
        cursor_pos--;
        shell_draw_input_line();
    }
}

void shell_move_cursor_right(void) {
    if (cursor_pos < input_index) {
        cursor_pos++;
        shell_draw_input_line();
    }
}

void shell_move_cursor_home(void) {
    cursor_pos = 0;
    shell_draw_input_line();
}

void shell_move_cursor_end(void) {
    cursor_pos = input_index;
    shell_draw_input_line();
}

void shell_clear_input(void) {
    input_index = 0;
    cursor_pos = 0;
    input_buffer[0] = '\0';
    shell_draw_input_line();
}

void shell_add_to_history(const char *command) {
    if (strlen(command) == 0) return;
    
    strcpy(history[history_index], command);
    history_index = (history_index + 1) % 10;
    history_current = history_index;
}

void shell_history_up(void) {
    int prev = (history_current - 1 + 10) % 10;
    if (strlen(history[prev]) > 0) {
        history_current = prev;
        strcpy(input_buffer, history[history_current]);
        input_index = strlen(input_buffer);
        cursor_pos = input_index;
        shell_draw_input_line();
    }
}

void shell_history_down(void) {
    int next = (history_current + 1) % 10;
    if (next != history_index) {
        history_current = next;
        strcpy(input_buffer, history[history_current]);
        input_index = strlen(input_buffer);
        cursor_pos = input_index;
        shell_draw_input_line();
    } else {
        shell_clear_input();
    }
}

char *input(char received) {
    if (received == '\n' || received == '\r') {
        input_buffer[input_index] = '\0';
        
        strncpy(command_buffer, input_buffer, sizeof(command_buffer) - 1);
        command_buffer[sizeof(command_buffer) - 1] = '\0';
        
        if (strlen(command_buffer) > 0) {
            shell_add_to_history(command_buffer);
        }
        
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
    else if (received == 4) {
        shell_delete();
        return NULL;
    }
    else if (received == 1) {
        shell_move_cursor_home();
        return NULL;
    }
    else if (received == 5) {
        shell_move_cursor_end();
        return NULL;
    }
    else if (received == 12) {
        clear_screen(bg_color);
        shell_x = 0;
        shell_y = 0;
        shell_print_prompt();
        shell_draw_input_line();
        return NULL;
    }
    else if (received == 27) {
        return NULL;
    }
    else if (received >= 32 && received <= 126 && input_index < (int)(sizeof(input_buffer) - 1)) {
        for (int i = input_index; i > cursor_pos; i--) {
            input_buffer[i] = input_buffer[i - 1];
        }
        input_buffer[cursor_pos] = received;
        cursor_pos++;
        input_index++;
        input_buffer[input_index] = '\0';
        shell_draw_input_line();
        return NULL;
    }
    return NULL;
}

void shell_cancel_input(void) {
    shell_clear_input();
    shell_newline();
    shell_print("^C\n");
    shell_print_prompt();
    shell_draw_input_line();
}

void parse_args(const char *command, char args[][32], int *argc) {
    *argc = 0;
    int i = 0, j = 0;
    
    while (command[i] && *argc < 8) {
        while (command[i] == ' ') i++;
        if (!command[i]) break;
        
        j = 0;
        while (command[i] && command[i] != ' ' && j < 31) {
            args[*argc][j++] = command[i++];
        }
        args[*argc][j] = '\0';
        (*argc)++;
    }
}

void cmd_help(int argc __attribute__((unused)), char args[][32] __attribute__((unused))) {
    shell_print("Available commands:\n");
    shell_print("  hello [name]  - Display greeting\n");
    shell_print("  help          - Show this help message\n");
    shell_print("  clear         - Clear the screen\n");
    shell_print("  echo <text>   - Echo text to screen\n");
    shell_print("  history       - Show command history\n");
    shell_print("  uptime        - Show system uptime\n");
    shell_print("  exit          - Exit the shell\n");
    shell_print("\nNavigation:\n");
    shell_print("  Ctrl+C        - Cancel current input\n");
    shell_print("  Ctrl+L        - Clear screen\n");
    shell_print("  Ctrl+A        - Move to beginning of line\n");
    shell_print("  Ctrl+E        - Move to end of line\n");
}

void cmd_hello(int argc, char args[][32]) {
    if (argc > 1) {
        shell_print("Hello, ");
        shell_print(args[1]);
        shell_print("!\n");
    } else {
        shell_print("Hello world!\n");
    }
}

void cmd_clear(int argc __attribute__((unused)), char args[][32] __attribute__((unused))) {
    clear_screen(bg_color);
    shell_x = 0;
    shell_y = 0;
}

void cmd_echo(int argc, char args[][32]) {
    if (argc < 2) {
        shell_error("echo: missing argument");
        return;
    }
    
    for (int i = 1; i < argc; i++) {
        shell_print(args[i]);
        if (i < argc - 1) shell_print(" ");
    }
    shell_print("\n");
}

void cmd_history(int argc __attribute__((unused)), char args[][32] __attribute__((unused))) {
    shell_print("Command history:\n");
    for (int i = 0; i < 10; i++) {
        int idx = (history_index + i) % 10;
        if (strlen(history[idx]) > 0) {
            shell_print("  ");
            shell_print(history[idx]);
            shell_print("\n");
        }
    }
}

void cmd_uptime(int argc __attribute__((unused)), char args[][32] __attribute__((unused))) {
    shell_print("System uptime: ");
    shell_print("Unknown");
    shell_print("\n");
}

void cmd_exit(int argc __attribute__((unused)), char args[][32] __attribute__((unused))) {
    shell_success("Goodbye!");
}

void parse_command(void) {
    if (command_buffer[0] == '\0') {
        shell_print_prompt();
        shell_draw_input_line();
        return;
    }
    
    char args[8][32];
    int argc;
    parse_args(command_buffer, args, &argc);
    
    if (argc == 0) {
        shell_print_prompt();
        shell_draw_input_line();
        return;
    }
    
    if (strcmp(args[0], "hello") == 0) {
        cmd_hello(argc, args);
    } else if (strcmp(args[0], "help") == 0) {
        cmd_help(argc, args);
    } else if (strcmp(args[0], "clear") == 0) {
        cmd_clear(argc, args);
    } else if (strcmp(args[0], "echo") == 0) {
        cmd_echo(argc, args);
    } else if (strcmp(args[0], "history") == 0) {
        cmd_history(argc, args);
    } else if (strcmp(args[0], "uptime") == 0) {
        cmd_uptime(argc, args);
    } else if (strcmp(args[0], "exit") == 0) {
        cmd_exit(argc, args);
        return;
    } else {
        shell_error("Unknown command");
        shell_print("Type 'help' for available commands.\n");
    }
    
    command_buffer[0] = '\0';
    shell_print_prompt();
    shell_draw_input_line();
}