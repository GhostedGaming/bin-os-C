#ifndef SHELL_H
#define SHELL_H

// Main shell functions
void shell_init(void);
void shell_print_prompt(void);
void shell_draw_input_line(void);
void shell_update_cursor(void);
void shell_newline(void);
void shell_scroll_up(void);
void shell_print(const char *text);
void shell_backspace(void);
void shell_cancel_input(void);

// Input handling
char *input(char received);
void parse_command(void);

// Utility functions
int strcmp(const char *str1, const char *str2);

#endif // SHELL_H