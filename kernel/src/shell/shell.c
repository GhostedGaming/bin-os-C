#include <stddef.h>
#include "shell.h"
#include "../serial/serial.h"
#include "../draw/draw.h"

static char command_buffer[255];
static char input_buffer[255];
static int input_index = 0;

void shell_backspace(void) {
    if (input_index > 0) {
        input_index--;
        input_buffer[input_index] = '\0';
    }
}

char *input(char received) {
    if (received == '\n' || received == '\r') {
        input_buffer[input_index] = '\0';
        
        int i = 0;
        while (input_buffer[i] != '\0' && i < 254) {
            command_buffer[i] = input_buffer[i];
            i++;
        }
        command_buffer[i] = '\0';
        
        input_index = 0;
        return command_buffer;
    }
    else if (received == '\b' || received == 127) {
        return NULL;
    }
    else if (input_index < 254) {
        input_buffer[input_index] = received;
        input_index++;
        return NULL;
    }
    return NULL;
}

int strcmp(const char *str1, const char *str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            return str1[i] - str2[i];
        }
        i++;
    }
    return str1[i] - str2[i];
}

void parse_command() {
    if (command_buffer[0] != '\0') {
        if (strcmp(command_buffer, "hello") == 0) {
            write_serial("Hello world!\r\n");
        } else if (strcmp(command_buffer, "help") == 0) {
            write_serial("Available commands: hello, help, clear\r\n");
        } else if (strcmp(command_buffer, "clear") == 0) {
            write_serial("Screen cleared\r\n");
            clear_screen(rgb_to_color(0, 0, 0));
        } else if (command_buffer[0] != '\0') {
            write_serial("Unknown command: ");
            write_serial(command_buffer);
            write_serial("\r\n");
        }
        command_buffer[0] = '\0';
    }
}