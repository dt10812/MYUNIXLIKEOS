#include "dirent.h"
#include "sys/syscall.h"

int getdents(int fd, struct dirent *dirp, size_t count) {
    return syscall(SYS_getdents, (long)fd, (long)dirp, (long)count, 0, 0);
}
