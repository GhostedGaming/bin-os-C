#include <stddef.h>
#include "shell.h"
#include "../serial/serial.h"
#include "../draw/draw.h"
#include "../memory.h"

static void parse_args(const char *command, char args[][32], int *argc);
static void shell_print_welcome(void);
void shell_scroll_up(void);
static void shell_add_to_history(const char *command);

void cmd_help(void);
void cmd_hello(int argc, char args[][32]);
void cmd_clear(void);
void cmd_echo(int argc, char args[][32]);
void cmd_history(void);
void cmd_uptime(void);
void cmd_exit(void);

#define MAX_INPUT_LENGTH 256
#define MAX_HISTORY_ENTRIES 10
#define MAX_ARGS 8
#define MAX_ARG_LENGTH 32

static char command_buffer[MAX_INPUT_LENGTH];
static char input_buffer[MAX_INPUT_LENGTH];
static char history[MAX_HISTORY_ENTRIES][MAX_INPUT_LENGTH];
static int history_index = 0;
static int history_current = 0;
static int input_index = 0;
static int cursor_pos = 0;  // Text cursor position within input buffer
static int cursor_blink_counter = 0;
static int cursor_visible = 0;

static const int line_height = 16;
static const int char_width = 8;
static const uint32_t text_color = 0xFFFFFFFF;
static const uint32_t bg_color = 0xFF000000;
static const uint32_t cursor_color = 0xFF00FF00;
static const uint32_t error_color = 0xFFFF0000;
static const uint32_t success_color = 0xFF00FF00;

void shell_init(void) {
    input_index = cursor_pos = 0;
    history_index = history_current = cursor_blink_counter = 0;
    input_buffer[0] = command_buffer[0] = '\0';
    memset(history, 0, sizeof(history));
    clear_screen(bg_color);
    move_cursor_to(0, 0);
    shell_print_welcome();
    shell_print_prompt();
}

static void shell_print_welcome(void) {
    shell_print("Binbows Installer v1.2\nType 'help' for available commands.\n\n");
}

void shell_print_prompt(void) {
    draw_string(get_cursor_x(), get_cursor_y(), "$ ", text_color);
    move_cursor_right(2 * char_width);
}

void shell_draw_input_line(void) {
    int prompt_x = get_cursor_x() - (2 * char_width);
    int prompt_y = get_cursor_y();
    
    // Clear the line
    draw_rect(prompt_x, prompt_y, get_screen_width() - prompt_x, line_height, bg_color);
    
    // Draw prompt
    draw_string(prompt_x, prompt_y, "$ ", text_color);
    
    // Draw input text
    int text_x = prompt_x + (2 * char_width);
    draw_string(text_x, prompt_y, input_buffer, text_color);
    
    // Draw cursor
    if (cursor_visible) {
        int cursor_x = text_x + (cursor_pos * char_width);
        draw_rect(cursor_x, prompt_y, char_width, line_height, cursor_color);
        if (cursor_pos < input_index && input_buffer[cursor_pos]) {
            char cursor_char[2] = {input_buffer[cursor_pos], '\0'};
            draw_string(cursor_x, prompt_y, cursor_char, bg_color);
        }
    }
}

void shell_update_cursor(void) {
    if (++cursor_blink_counter >= 30) {
        cursor_visible = !cursor_visible;
        cursor_blink_counter = 0;
        shell_draw_input_line();
    }
}

void shell_newline(void) {
    move_cursor_to(0, get_cursor_y() + line_height);
    if (get_cursor_y() >= get_screen_height() - line_height) {
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
    
    move_cursor_up(line_height);
}

void shell_print_color(const char *text, uint32_t color) {
    if (!text) return;
    
    while (*text) {
        if (*text == '\n') {
            shell_newline();
        } else if (*text == '\r') {
            move_cursor_to(0, get_cursor_y());
        } else if (*text == '\t') {
            int spaces = 4 - (get_cursor_x() / char_width) % 4;
            for (int i = 0; i < spaces; i++) {
                draw_ascii_char(get_cursor_x(), get_cursor_y(), ' ', color);
                move_cursor_right(char_width);
                if (get_cursor_x() >= get_screen_width() - char_width) {
                    shell_newline();
                    break;
                }
            }
        } else {
            draw_ascii_char(get_cursor_x(), get_cursor_y(), *text, color);
            move_cursor_right(char_width);
            if (get_cursor_x() >= get_screen_width() - char_width) {
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
    shell_print(": ");
    shell_print(command_buffer);
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
    input_index = cursor_pos = 0;
    input_buffer[0] = '\0';
    shell_draw_input_line();
}

static void shell_add_to_history(const char *command) {
    if (!strlen(command)) return;
    
    int prev_index = (history_index - 1 + MAX_HISTORY_ENTRIES) % MAX_HISTORY_ENTRIES;
    if (strlen(history[prev_index]) > 0 && strcmp(history[prev_index], command) == 0) {
        history_current = history_index;
        return;
    }
    
    strcpy(history[history_index], command);
    history_index = (history_index + 1) % MAX_HISTORY_ENTRIES;
    history_current = history_index;
}

void shell_history_up(void) {
    int prev = (history_current - 1 + MAX_HISTORY_ENTRIES) % MAX_HISTORY_ENTRIES;
    if (strlen(history[prev]) > 0) {
        history_current = prev;
        strcpy(input_buffer, history[history_current]);
        input_index = cursor_pos = strlen(input_buffer);
        shell_draw_input_line();
    }
}

void shell_history_down(void) {
    int next = (history_current + 1) % MAX_HISTORY_ENTRIES;
    if (next != history_index) {
        history_current = next;
        strcpy(input_buffer, history[history_current]);
        input_index = cursor_pos = strlen(input_buffer);
        shell_draw_input_line();
    } else {
        shell_clear_input();
        history_current = history_index;
    }
}

void shell_cancel_input(void) {
    shell_clear_input();
    shell_newline();
    shell_print("^C\n");
    shell_print_prompt();
    shell_draw_input_line();
}

char *input(char received) {
    switch (received) {
        case '\n':
        case '\r':
            input_buffer[input_index] = '\0';
            strncpy(command_buffer, input_buffer, sizeof(command_buffer) - 1);
            command_buffer[sizeof(command_buffer) - 1] = '\0';
            if (strlen(command_buffer) > 0) {
                shell_add_to_history(command_buffer);
            }
            shell_newline();
            shell_clear_input();
            return command_buffer;
            
        case '\b':
        case 127:
            shell_backspace();
            return NULL;
            
        case 3:  // Ctrl+C
            shell_cancel_input();
            return NULL;
            
        case 4:  // Ctrl+D
            shell_delete();
            return NULL;
            
        case 1:  // Ctrl+A
            shell_move_cursor_home();
            return NULL;
            
        case 5:  // Ctrl+E
            shell_move_cursor_end();
            return NULL;
            
        case 12:  // Ctrl+L
            clear_screen(bg_color);
            move_cursor_to(0, 0);
            shell_print_prompt();
            shell_draw_input_line();
            return NULL;
            
        case 11:  // Ctrl+K
            input_buffer[cursor_pos] = '\0';
            input_index = cursor_pos;
            shell_draw_input_line();
            return NULL;
            
        case 21:  // Ctrl+U
            shell_clear_input();
            return NULL;
            
        case 23:  // Ctrl+W
            if (cursor_pos > 0) {
                int start = cursor_pos - 1;
                while (start > 0 && input_buffer[start] == ' ') start--;
                while (start > 0 && input_buffer[start] != ' ') start--;
                if (input_buffer[start] == ' ') start++;
                
                int chars_to_remove = cursor_pos - start;
                for (int i = start; i < input_index - chars_to_remove; i++) {
                    input_buffer[i] = input_buffer[i + chars_to_remove];
                }
                input_index -= chars_to_remove;
                cursor_pos = start;
                input_buffer[input_index] = '\0';
                shell_draw_input_line();
            }
            return NULL;
            
        case 27:  // Escape
            return NULL;
            
        default:
            if (received >= 32 && received <= 126 && input_index < MAX_INPUT_LENGTH - 1) {
                for (int i = input_index; i > cursor_pos; i--) {
                    input_buffer[i] = input_buffer[i - 1];
                }
                input_buffer[cursor_pos] = received;
                cursor_pos++;
                input_index++;
                input_buffer[input_index] = '\0';
                shell_draw_input_line();
            }
            return NULL;
    }
}

static void parse_args(const char *command, char args[][32], int *argc) {
    *argc = 0;
    int i = 0, j = 0;
    
    while (command[i] && *argc < MAX_ARGS) {
        while (command[i] == ' ' || command[i] == '\t') i++;
        if (!command[i]) break;
        
        j = 0;
        while (command[i] && command[i] != ' ' && command[i] != '\t' && j < MAX_ARG_LENGTH - 1) {
            args[*argc][j++] = command[i++];
        }
        args[*argc][j] = '\0';
        (*argc)++;
    }
}

void cmd_help(void) {
    shell_print("Available commands:\n"
               "  hello [name]  - Display greeting\n"
               "  help          - Show this help message\n"
               "  clear         - Clear the screen\n"
               "  echo <text>   - Echo text to screen\n"
               "  history       - Show command history\n"
               "  uptime        - Show system uptime\n"
               "  exit          - Exit the shell\n"
               "\nNavigation:\n"
               "  Ctrl+C        - Cancel current input\n"
               "  Ctrl+L        - Clear screen\n"
               "  Ctrl+A        - Move to beginning of line\n"
               "  Ctrl+E        - Move to end of line\n"
               "  Ctrl+K        - Kill to end of line\n"
               "  Ctrl+U        - Kill entire line\n"
               "  Ctrl+W        - Kill word backward\n"
               "  Left/Right    - Move cursor\n"
               "  Home/End      - Move to line start/end\n");
}

void cmd_hello(int argc, char args[][32]) {
    if (argc > 1) {
        shell_print("Hello, ");
        shell_print(args[1]);
        shell_print("!\n");
    } else {
        shell_print("Hello, World!\n");
    }
}

void cmd_clear(void) {
    clear_screen(bg_color);
    move_cursor_to(0, 0);
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

void cmd_history(void) {
    shell_print("Command history:\n");
    int count = 0;
    for (int i = 0; i < MAX_HISTORY_ENTRIES; i++) {
        int idx = (history_index + i) % MAX_HISTORY_ENTRIES;
        if (strlen(history[idx]) > 0) {
            shell_print("  ");
            shell_print(history[idx]);
            shell_print("\n");
            count++;
        }
    }
    if (count == 0) {
        shell_print("  (no commands in history)\n");
    }
}

void cmd_uptime(void) {
    shell_print("System uptime: Unknown (uptime not implemented)\n");
}

void cmd_exit(void) {
    shell_success("Goodbye!");
}

void parse_command(void) {
    if (!command_buffer[0]) {
        shell_print_prompt();
        shell_draw_input_line();
        return;
    }
    
    char args[MAX_ARGS][MAX_ARG_LENGTH];
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
        cmd_help();
    } else if (strcmp(args[0], "clear") == 0) {
        cmd_clear();
    } else if (strcmp(args[0], "echo") == 0) {
        cmd_echo(argc, args);
    } else if (strcmp(args[0], "history") == 0) {
        cmd_history();
    } else if (strcmp(args[0], "uptime") == 0) {
        cmd_uptime();
    } else if (strcmp(args[0], "exit") == 0) {
        cmd_exit();
        return;
    } else {
        shell_error("Unknown command");
        shell_print("Type 'help' for available commands.\n");
    }
    
    command_buffer[0] = '\0';
    shell_print_prompt();
    shell_draw_input_line();
}