[bits 64]

extern isr_handler
extern irq_handler

; Macro for ISRs that don't push an error code
%macro isr_no_err 1
global isr%1
isr%1:
    cli                     ; Disable interrupts
    push 0                  ; Push dummy error code
    push %1                 ; Push interrupt number
    jmp isr_common_stub     ; Jump to common handler
%endmacro

; Macro for ISRs that push an error code
%macro isr_err 1
global isr%1
isr%1:
    cli                     ; Disable interrupts
    push %1                 ; Push interrupt number
    jmp isr_common_stub     ; Jump to common handler
%endmacro

; Macro for hardware interrupts (IRQs)
%macro irq_handler 2
global irq%1
irq%1:
    cli                     ; Disable interrupts
    push 0                  ; Push dummy error code
    push %2                 ; Push IRQ number
    jmp irq_common_stub     ; Jump to IRQ handler
%endmacro

; Define ISRs for the most important CPU exceptions
isr_no_err 0    ; Division by zero
isr_no_err 1    ; Debug
isr_no_err 2    ; Non-maskable interrupt
isr_no_err 3    ; Breakpoint
isr_no_err 4    ; Overflow
isr_no_err 5    ; Bound range exceeded
isr_no_err 6    ; Invalid opcode
isr_no_err 7    ; Device not available
isr_err    8    ; Double fault (has error code)
isr_no_err 9    ; Coprocessor segment overrun
isr_err    10   ; Invalid TSS (has error code)
isr_err    11   ; Segment not present (has error code)
isr_err    12   ; Stack-segment fault (has error code)
isr_err    13   ; General protection fault (has error code)
isr_err    14   ; Page fault (has error code)
isr_no_err 15   ; Reserved
isr_no_err 16   ; x87 floating-point exception
isr_err    17   ; Alignment check (has error code)
isr_no_err 18   ; Machine check
isr_no_err 19   ; SIMD floating-point exception
isr_no_err 20   ; Virtualization exception
isr_err    21   ; Control protection exception (has error code)
isr_no_err 22   ; Reserved
isr_no_err 23   ; Reserved
isr_no_err 24   ; Reserved
isr_no_err 25   ; Reserved
isr_no_err 26   ; Reserved
isr_no_err 27   ; Reserved
isr_no_err 28   ; Reserved
isr_no_err 29   ; Reserved
isr_err    30   ; Security exception (has error code)
isr_no_err 31   ; Reserved

; Define hardware interrupt handlers (IRQs remapped to 32-47)
irq_handler 0, 32   ; Timer (IRQ0 -> interrupt 32)
irq_handler 1, 33   ; Keyboard (IRQ1 -> interrupt 33)

; Common ISR stub that saves state and calls C handler
isr_common_stub:
    ; Save all registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    ; Call C exception handler
    ; The interrupt number is at [rsp + 120] (15 registers * 8 bytes)
    mov rdi, [rsp + 120]    ; Pass interrupt number as first argument
    call isr_handler
    
    ; Restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    ; Clean up error code and interrupt number
    add rsp, 16
    
    ; Return from interrupt
    iretq

; Common IRQ stub for hardware interrupts
irq_common_stub:
    ; Save all registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    ; Call C IRQ handler
    ; The IRQ number is at [rsp + 120] (15 registers * 8 bytes)
    mov rdi, [rsp + 120]    ; Pass IRQ number as first argument
    call irq_handler
    
    ; Restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    ; Clean up error code and IRQ number
    add rsp, 16
    
    ; Return from interrupt
    iretq

; Mark stack as non-executable (fixes GNU-stack warning)
section .note.GNU-stack noalloc noexec nowrite progbits