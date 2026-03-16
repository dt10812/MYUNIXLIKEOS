[BITS 32]

extern kernel_main
global _start

_start:
    cli
    xor ebp, ebp
    mov ebp, 0
    mov esp, 0x90000        ; simple stack

    call kernel_main
.halt:
    hlt
    jmp .halt