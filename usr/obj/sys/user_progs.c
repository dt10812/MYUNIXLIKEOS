#include "vfs.h"
#include <stdint.h>
extern uint8_t hello_start[], hello_end[];
extern uint8_t simple_start[], simple_end[];
extern uint8_t echo_user_start[], echo_user_end[];
void install_user_progs(void) {
    k_install("hello", hello_start, (uint32_t)(hello_end - hello_start));
    k_install("simple", simple_start, (uint32_t)(simple_end - simple_start));
    k_install("echo_user", echo_user_start, (uint32_t)(echo_user_end - echo_user_start));
}
