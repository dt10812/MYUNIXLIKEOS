#include "sys/types.h"
#include "sys/syscall.h"

int brk(void *addr) {
    return (int)syscall(SYS_brk, (long)addr, 0, 0, 0, 0);
}
