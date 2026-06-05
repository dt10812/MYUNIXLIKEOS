[BITS 32]

; Multiboot2 header
section .multiboot2_header
align 8
mb2_start:
    dd 0xE85250D6                                ; magic
    dd 0                                         ; arch: 32-bit protected mode
    dd mb2_end - mb2_start                       ; header length
    dd 0x17ADAF12                                ; checksum
    ; end tag
    dw 0
    dw 0
    dd 8
mb2_end:

; Stack
section .bss
align 16
stack_bottom:
    resb 0x4000        ; 16kb kernel stack
global stack_top
stack_top:

section .text

global _start
extern kernel_main

_start:
    ; Direct VGA output to confirm _start is executing
    mov word [0xB8000], 0x0F41  ; White 'A' at first VGA position
    
    mov esp, stack_top
    mov ebp, esp

    ; Multiboot2: eax = magic, ebx = info ptr
    push ebx            ; arg1: multiboot info ptr
    push eax            ; arg0: magic
    
    mov word [0xB8002], 0x0F42  ; White 'B' - about to call kernel_main
    
    call kernel_main

.halt:
    mov word [0xB8004], 0x0FCB  ; White 'Ë' (crashed/halted marker)
    cli
    hlt
    jmp .halt


; gdt/tss load helpers

global gdt_load
gdt_load:
    mov eax, [esp+4]    ; pointer to gdt_ptr struct
    lgdt [eax]
    mov ax, 0x10        ; kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush     ; far jump to reload cs
.flush:
    ret

global tss_load
tss_load:
    mov ax, 0x28        ; TSS selector
    ltr ax
    ret


; Unexpected IRQ/exception handlers
extern fault_handler

; Build a vector table so each exception carries its real vector number.
; For exceptions with an error code, the CPU already pushed err_code.
; For those without one, we synthesize a zero error code.

global isr_table

%macro ISR_NOERR 1
global isr%1
isr%1:
    push dword 0        ; dummy error code
    push dword %1       ; real trap vector
    pusha
    push esp            ; pointer to trapframe
    call fault_handler
    add esp, 4
    popa
    add esp, 8
    iret
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    push dword %1       ; real trap vector (error code already on stack)
    pusha
    push esp            ; pointer to trapframe
    call fault_handler
    add esp, 4
    popa
    add esp, 8
    iret
%endmacro

isr_table:
%assign i 0
%rep 32
    dd isr%+i
%assign i i+1
%endrep

%assign i 0
%rep 32
    %if i == 8 || i == 10 || i == 11 || i == 12 || i == 13 || i == 14 || i == 17
        ISR_ERR i
    %else
        ISR_NOERR i
    %endif
    %assign i i+1
%endrep

global isr_noerr
global isr_err
isr_noerr:
    jmp isr0
isr_err:
    jmp isr8

; Syscall handler

global _syscall
extern syscall_handler

_syscall:
    push dword 0        ; dummy err_code
    push dword 0x80     ; trapno
    pusha               ; edi, esi, ebp, esp, ebx, edx, ecx, eax
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10        ; kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; arg0: pointer to trapframe
    call syscall_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret


; void jump_usermode(uint32_t entry, uint32_t user_stack)
;
; Drops to ring 3 via iret.
; Selectors: user code = 0x1B (index 3, RPL=3)
;            user data = 0x23 (index 4, RPL=3)

global jump_usermode
jump_usermode:
    mov eax, [esp+4]    ; entry
    mov ecx, [esp+8]    ; user esp

    push dword 0x23     ; ss
    push ecx            ; user esp
    push dword 0x202    ; eflags: IF=1, IOPL=0
    push dword 0x1B     ; cs
    push eax            ; eip

    mov dx, 0x23
    mov ds, dx
    mov es, dx
    mov fs, dx
    mov gs, dx

    iret
