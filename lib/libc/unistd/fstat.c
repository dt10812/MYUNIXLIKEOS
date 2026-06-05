#include "sys/stat.h"
#include "sys/syscall.h"

int fstat(int fildes, struct stat *buf) {
    return syscall(SYS_fstat, (long)fildes, (long)buf, 0, 0, 0);
}
