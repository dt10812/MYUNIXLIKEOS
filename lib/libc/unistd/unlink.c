#include "unistd.h"

int unlink(const char *path) {
    return (int)syscall(SYS_unlink, (long)path, 0, 0, 0, 0);
}
