section .text
global read_tsc

; Read Time Stamp Counter
read_tsc:
    rdtsc
    shl rdx, 32
    or rax, rdx
    ret