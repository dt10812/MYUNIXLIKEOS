#include "sys/types.h"
#include "sys/syscall.h"

pid_t fork(void) {
    return (pid_t)syscall(SYS_fork, 0, 0, 0, 0, 0);
}
