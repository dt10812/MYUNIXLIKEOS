#include "commands.h"
#include "stdio.h"
#include "sys/sysinfo.h"
#include "sys/syscall.h"

/* Inline sysinfo wrapper */
static int sysinfo_call(struct sysinfo *info) {
    return syscall(SYS_sysinfo, (long)info, 0, 0, 0, 0);
}

int cmd_uptime(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    struct sysinfo info;

    if (sysinfo_call(&info) == -1) {
        printf("uptime: could not get system information\n");
        return 1;
    }

    // Calculate uptime components
    unsigned int uptime = info.uptime;
    unsigned int days = uptime / 86400;
    unsigned int hours = (uptime % 86400) / 3600;
    unsigned int minutes = (uptime % 3600) / 60;
    unsigned int seconds = uptime % 60;

    printf(" %02d:%02d:%02d up", hours, minutes, seconds);

    if (days > 0) {
        if (days == 1) {
            printf(" %u day", days);
        } else {
            printf(" %u days", days);
        }
    }

    printf(",  %u user,  load average: 0.00, 0.00, 0.00\n", info.procs);

    return 0;
}