section .rodata
global hello_start, hello_end
hello_start: incbin "usr/obj/hello.elf"
hello_end:
