section .data
    vendor_string db 13 dup(0)     ; 12 chars + null terminator
    cpu_speed_mhz dd 0              ; Store CPU speed in MHz

section .text
    global check_cpu_and_get_vendor
    global get_cpu_vendor_string
    global check_cpu_and_call_c
    global get_cpu_vendor_info
    global get_cpu_speed_mhz        ; New function to get CPU speed
    global get_cpu_frequencies      ; New function to get detailed frequency info
    extern process_cpu_vendor       ; External C function declaration

; Function: check_cpu_and_get_vendor
; Returns: 1 if CPUID supported and vendor string retrieved, 0 otherwise
check_cpu_and_get_vendor:
    push rbp
    mov rbp, rsp
    push rbx                        ; Save callee-saved register

    ; CPUID support check (64-bit version)
    pushfq                          ; Save RFLAGS
    pushfq                          ; Store RFLAGS
    xor qword [rsp], 0x00200000     ; Invert the ID bit in stored RFLAGS
    popfq                           ; Load stored RFLAGS (with ID bit inverted)
    pushfq                          ; Store RFLAGS again (ID bit may or may not be inverted)
    pop rax                         ; rax = modified RFLAGS (ID bit may or may not be inverted)
    xor rax, [rsp]                  ; rax = whichever bits were changed
    popfq                           ; Restore original RFLAGS
    and rax, 0x00200000             ; rax = zero if ID bit can't be changed, else non-zero
    
    test rax, rax
    jz no_cpuid_check               ; Jump if CPUID not supported

    ; CPUID is supported, get vendor string
    xor eax, eax                    ; CPUID function 0 (vendor string)
    cpuid
    
    ; Store vendor string (EBX, EDX, ECX contain the 12-character vendor string)
    mov [vendor_string], ebx        ; First 4 characters
    mov [vendor_string + 4], edx    ; Middle 4 characters  
    mov [vendor_string + 8], ecx    ; Last 4 characters
    mov byte [vendor_string + 12], 0 ; Null terminator
    
    mov rax, 1                      ; Return 1 for success
    jmp check_cleanup

no_cpuid_check:
    mov rax, 0                      ; Return 0 for no CPUID support

check_cleanup:
    pop rbx                         ; Restore register
    pop rbp
    ret

; Function: get_cpu_vendor_string
; Returns: pointer to vendor string in RAX
get_cpu_vendor_string:
    push rbp
    mov rbp, rsp
    
    mov rax, vendor_string          ; Return pointer to vendor string
    
    pop rbp
    ret

; Function: get_cpu_speed_mhz
; Returns: CPU base frequency in MHz in EAX, 0 if not supported
get_cpu_speed_mhz:
    push rbp
    mov rbp, rsp
    push rbx
    push rcx
    push rdx

    ; First check if CPUID is supported
    call check_cpuid_support
    test rax, rax
    jz no_speed_support

    ; Check maximum CPUID function supported
    xor eax, eax
    cpuid
    cmp eax, 0x16                   ; Check if function 0x16 is supported
    jl try_alternative_method       ; If not, try alternative method

    ; Use CPUID function 0x16 (Processor Frequency Information)
    mov eax, 0x16
    cpuid
    
    ; EAX contains base frequency in MHz
    test eax, eax
    jz try_alternative_method       ; If 0, try alternative
    
    mov [cpu_speed_mhz], eax
    jmp speed_cleanup

try_alternative_method:
    ; Try CPUID function 0x15 (Time Stamp Counter and Processor Frequency Info)
    mov eax, 0x15
    cpuid
    
    ; If ECX (crystal clock frequency) is non-zero, we can calculate
    test ecx, ecx
    jz estimate_from_brand_string
    
    ; Calculate: (ECX * EBX) / EAX = TSC frequency
    ; This gives us a more accurate frequency
    test eax, eax
    jz estimate_from_brand_string
    
    ; Simple calculation (may need refinement for accuracy)
    mov eax, ecx
    mul ebx                         ; EDX:EAX = ECX * EBX
    ; For simplicity, we'll just use the lower 32 bits
    ; In practice, you'd want to handle the full 64-bit division
    mov ecx, [rsp + 24]            ; Restore original ECX from stack
    xor edx, edx
    div ecx                        ; EAX = (ECX * EBX) / original_EAX
    
    ; Convert from Hz to MHz (divide by 1,000,000)
    mov ecx, 1000000
    xor edx, edx
    div ecx
    
    mov [cpu_speed_mhz], eax
    jmp speed_cleanup

estimate_from_brand_string:
    ; Fallback: return 0 to indicate we couldn't determine speed
    xor eax, eax
    mov [cpu_speed_mhz], eax

speed_cleanup:
    mov eax, [cpu_speed_mhz]        ; Return the speed in EAX
    pop rdx
    pop rcx
    pop rbx
    pop rbp
    ret

no_speed_support:
    xor eax, eax                    ; Return 0 for no support
    mov [cpu_speed_mhz], eax
    jmp speed_cleanup

; Function: get_cpu_frequencies
; Returns detailed frequency information
; EAX = base frequency (MHz), EBX = max frequency (MHz), ECX = bus frequency (MHz)
get_cpu_frequencies:
    push rbp
    mov rbp, rsp
    push rdx

    ; Check if CPUID function 0x16 is supported
    xor eax, eax
    cpuid
    cmp eax, 0x16
    jl no_freq_info

    ; Get processor frequency information
    mov eax, 0x16
    cpuid
    ; EAX = Base frequency in MHz
    ; EBX = Maximum frequency in MHz  
    ; ECX = Bus (reference) frequency in MHz
    
    jmp freq_cleanup

no_freq_info:
    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx

freq_cleanup:
    pop rdx
    pop rbp
    ret

; Helper function: check_cpuid_support
; Returns: 1 if CPUID supported, 0 otherwise
check_cpuid_support:
    push rbp
    mov rbp, rsp

    pushfq
    pushfq
    xor qword [rsp], 0x00200000
    popfq
    pushfq
    pop rax
    xor rax, [rsp]
    popfq
    and rax, 0x00200000
    
    test rax, rax
    jz cpuid_not_supported
    
    mov rax, 1
    jmp cpuid_check_done

cpuid_not_supported:
    xor rax, rax

cpuid_check_done:
    pop rbp
    ret

; Function: check_cpu_and_call_c
; Calls C function with vendor string or NULL
check_cpu_and_call_c:
    push rbp
    mov rbp, rsp
    push rbx                        ; Save registers (callee-saved)
    push r12
    push r13
    push r14
    push r15

    ; CPUID support check (64-bit version)
    pushfq                          ; Save RFLAGS (64-bit)
    pushfq                          ; Store RFLAGS
    xor qword [rsp], 0x00200000     ; Invert the ID bit in stored RFLAGS
    popfq                           ; Load stored RFLAGS (with ID bit inverted)
    pushfq                          ; Store RFLAGS again (ID bit may or may not be inverted)
    pop rax                         ; rax = modified RFLAGS (ID bit may or may not be inverted)
    xor rax, [rsp]                  ; rax = whichever bits were changed
    popfq                           ; Restore original RFLAGS
    and rax, 0x00200000             ; rax = zero if ID bit can't be changed, else non-zero
    
    test rax, rax
    jz no_cpuid_support             ; Jump if CPUID not supported

    ; CPUID is supported, get vendor string
    xor eax, eax                    ; CPUID function 0 (vendor string)
    cpuid
    
    ; Store vendor string (EBX, EDX, ECX contain the 12-character vendor string)
    mov [vendor_string], ebx        ; First 4 characters
    mov [vendor_string + 4], edx    ; Middle 4 characters  
    mov [vendor_string + 8], ecx    ; Last 4 characters
    mov byte [vendor_string + 12], 0 ; Null terminator
    
    ; Call external C function with vendor string (System V AMD64 ABI)
    mov rdi, vendor_string          ; First argument in RDI
    call process_cpu_vendor         ; Call C function
    
    mov rax, 1                      ; Return 1 for success
    jmp cleanup

no_cpuid_support:
    ; Call C function with NULL to indicate no CPUID support
    xor rdi, rdi                    ; RDI = 0 (NULL pointer)
    call process_cpu_vendor         ; Call C function
    
    mov rax, 0                      ; Return 0 for no CPUID support

cleanup:
    pop r15                         ; Restore registers
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

; Function: get_cpu_vendor_info
; Alternative version that returns vendor info in registers
; Returns: RAX=1 if supported, 0 if not. EBX, ECX, EDX contain vendor string if supported
get_cpu_vendor_info:
    push rbp
    mov rbp, rsp
    push r12                        ; Save register we'll use

    ; Check CPUID support (64-bit version)
    pushfq
    pushfq
    xor qword [rsp], 0x00200000
    popfq
    pushfq
    pop rax
    xor rax, [rsp]
    popfq
    and rax, 0x00200000
    
    test rax, rax
    jz vendor_not_supported

    ; Get vendor string
    xor eax, eax
    cpuid
    ; EBX, EDX, ECX now contain vendor string
    ; EAX contains max supported function number
    
    ; Store original EBX value since we need to restore r12
    mov r12, rbx
    
    ; You can now check for specific vendors:
    ; Intel: "GenuineIntel" (EBX=756E6547h, EDX=49656E69h, ECX=6C65746Eh)
    ; AMD:   "AuthenticAMD" (EBX=68747541h, EDX=69746E65h, ECX=444D4163h)
    
    mov rbx, r12                    ; Restore EBX with vendor info
    mov rax, 1                      ; Success (use RAX in 64-bit)
    jmp vendor_cleanup

vendor_not_supported:
    xor rax, rax                    ; No CPUID support
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx

vendor_cleanup:
    pop r12
    pop rbp
    ret