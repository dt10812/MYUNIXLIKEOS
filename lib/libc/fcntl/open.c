#include "sys/syscall.h"
#include "fcntl.h"
#include "stdarg.h"

int open(const char *path, int oflag, ...) {
    mode_t mode = 0;
    if (oflag & O_CREAT) {
        va_list ap;
        va_start(ap, oflag);
        mode = va_arg(ap, int);
        va_end(ap);
    }
    return syscall(SYS_open, (long)path, (long)oflag, (long)mode, 0, 0);
}