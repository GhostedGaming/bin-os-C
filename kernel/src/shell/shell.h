#ifndef SHELL_H
#define SHELL_H

char *input(char received);

void shell_backspace(void);

// I should make a serperate file like helpers.h and put this into it but for now this will be here
int strcmp(const char *str1, const char *str2);

void parse_command();

#endif // SHELL_H