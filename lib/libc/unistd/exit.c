#include "sys/syscall.h"

void _exit(int status) {
    syscall(SYS_exit, (long)status, 0, 0, 0, 0);
    for (;;) { }
}
