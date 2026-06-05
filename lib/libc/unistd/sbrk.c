#include "sys/types.h"
#include "sys/syscall.h"

void *sbrk(intptr_t increment) {
    return (void *)syscall(SYS_sbrk, (long)increment, 0, 0, 0, 0);
}
