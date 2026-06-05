#include "sys/types.h"
#include "sys/syscall.h"

ssize_t read(int fildes, void *buf, size_t nbyte) {
    return syscall(SYS_read, (long)fildes, (long)buf, (long)nbyte, 0, 0);
}
