global get_speed
global get_cpu_base_frequency_mhz
global get_cpu_max_frequency_mhz

get_speed:
    mov eax, 1
    cpuid 
    ret

; Get base frequency (requires CPUID leaf 0x16)
get_cpu_base_frequency_mhz:
    mov eax, 0x16
    cpuid
    ; Base frequency in MHz is in EAX
    ret

; Get max frequency (requires CPUID leaf 0x16)  
get_cpu_max_frequency_mhz:
    mov eax, 0x16
    cpuid
    mov eax, ebx    ; Max frequency in MHz is in EBX
    ret
