#include "sys/types.h"
#include "sys/syscall.h"

pid_t wait(int *status) {
    return (pid_t)syscall(SYS_wait, (long)status, 0, 0, 0, 0);
}
