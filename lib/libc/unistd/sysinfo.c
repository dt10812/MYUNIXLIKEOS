#include <unistd.h>
#include <sys/syscall.h>

int sysinfo(struct sysinfo *info) {
    return syscall(SYS_sysinfo, (long)info, 0, 0, 0, 0);
}