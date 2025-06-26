#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

// Structure for interrupt registers (if you're using it)
struct interrupt_registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, eflags, rsp, ss;
};

// Function declarations
void init_keyboard(void);
void keyboard_handler(struct interrupt_registers *regs);

#endif // KEYBOARD_H