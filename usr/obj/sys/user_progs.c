#include "vfs.h"
#include <stdint.h>
extern uint8_t hello_start[], hello_end[];
void install_user_progs(void) {
    k_install("hello", hello_start, (uint32_t)(hello_end - hello_start));
}
