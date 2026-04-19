section .rodata
global hello_start, hello_end
hello_start: incbin "usr/obj/hello.elf"
hello_end:
global simple_start, simple_end
simple_start: incbin "usr/obj/simple.elf"
simple_end:
global echo_user_start, echo_user_end
echo_user_start: incbin "usr/obj/echo_user.elf"
echo_user_end:
