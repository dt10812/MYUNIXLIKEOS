#include "sys/syscall.h"

int close(int fildes) {
    return syscall(SYS_close, (long)fildes, 0, 0, 0, 0);
}
