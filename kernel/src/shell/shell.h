#ifndef SHELL_H
#define SHELL_H

void shell_init(void);
void shell_print_prompt(void);
void shell_draw_input_line(void);
void shell_update_cursor(void);
void shell_newline(void);
void shell_scroll_up(void);
void shell_print(const char *text);
void shell_backspace(void);
void shell_cancel_input(void);
void cmd_clear(void);

char *input(char received);
void parse_command(void);

#endif // SHELL_H