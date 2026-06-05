#include "unistd.h"
#include "sys/syscall.h"

int chdir(const char *path) {
    return syscall(SYS_chdir, (long)path, 0, 0, 0, 0);
}
