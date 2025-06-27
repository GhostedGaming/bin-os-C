global check_for_cpuid
global get_vendor

section .bss
    vendor_string resb 13    ; 12 chars + null terminator

section .text

check_for_cpuid:
    pushfq                   ; Save EFLAGS
    pushfq                   ; Store EFLAGS
    xor dword [rsp], 0x00200000  ; Invert the ID bit in stored EFLAGS
    popfq                    ; Load stored EFLAGS (with ID bit inverted)
    pushfq                   ; Store EFLAGS again (ID bit may or may not be inverted)
    pop rax                  ; rax = modified EFLAGS (ID bit may or may not be inverted)
    xor rax, [rsp]          ; rax = whichever bits were changed
    popfq                    ; Restore original EFLAGS
    and rax, 0x00200000     ; rax = zero if ID bit can't be changed, else non-zero
    ret

get_vendor:
    push rbx                 ; Save rbx (callee-saved register)
    push rcx                 ; Save rcx
    push rdx                 ; Save rdx
    
    mov eax, 0              ; CPUID function 0 (get vendor string)
    cpuid                   ; Execute CPUID
    
    ; CPUID returns vendor string in EBX, EDX, ECX (in that order)
    ; Store the 12-character vendor string
    mov [vendor_string], ebx     ; First 4 characters
    mov [vendor_string + 4], edx ; Next 4 characters  
    mov [vendor_string + 8], ecx ; Last 4 characters
    mov byte [vendor_string + 12], 0  ; Null terminator
    
    mov rax, vendor_string   ; Return pointer to vendor string
    
    pop rdx                  ; Restore registers
    pop rcx
    pop rbx
    ret

; Alternative version that takes a buffer parameter
; Call with: get_vendor_to_buffer(char* buffer)
global get_vendor_to_buffer
get_vendor_to_buffer:
    push rbx                 ; Save rbx
    push rcx                 ; Save rcx  
    push rdx                 ; Save rdx
    
    mov r8, rdi             ; Save buffer pointer (first argument in rdi)
    
    mov eax, 0              ; CPUID function 0
    cpuid
    
    ; Store vendor string in provided buffer
    mov [r8], ebx           ; First 4 characters
    mov [r8 + 4], edx       ; Next 4 characters
    mov [r8 + 8], ecx       ; Last 4 characters
    mov byte [r8 + 12], 0   ; Null terminator
    
    mov rax, r8             ; Return buffer pointer
    
    pop rdx
    pop rcx
    pop rbx
    ret