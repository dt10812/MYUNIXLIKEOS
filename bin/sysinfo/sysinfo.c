#include <stdio.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>

/* Inline sysinfo wrapper */
static int sysinfo_call(struct sysinfo *info) {
    return syscall(SYS_sysinfo, (long)info, 0, 0, 0, 0);
}

int cmd_sysinfo(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    struct sysinfo info;

    if (sysinfo_call(&info) == -1) {
        printf("Error: Could not get system information\n");
        return 1;
    }

    /* ASCII Art Logo (CP437) */
    printf("\xDB\xDB\xDB\xBB   \xDB\xDB\xDB\xBB\xDB\xDB\xBB   \xDB\xDB\xBB\xDB\xDB\xBB   \xDB\xDB\xDB\xBB   \xDB\xDB\xBB\xDB\xDB  \xDB\xDB\xBB\n");
    printf("\xDB\xDB\xDB\xDB\xBB \xDB\xDB\xDB\xDB\xBA\xC8\xDB\xDB\xBB \xDB\xDB\xCD\xBB\xDB\xDB\xBA   \xDB\xDB\xBA\xDB\xDB\xDB\xDB\xBB  \xDB\xDB\xBA\xC8\xDB\xDB\xBB\xDB\xDB\xCD\xBB\n");
    printf("\xDB\xDB\xCD\xDB\xDB\xDB\xDB\xCD\xDB\xDB\xBA \xC8\xDB\xDB\xDB\xDB\xCD\xBB \xDB\xDB\xBA   \xDB\xDB\xBA\xDB\xDB\xCD\xDB\xDB\xBB \xDB\xDB\xBA \xC8\xDB\xDB\xDB\xCD\xBB \n");
    printf("\xDB\xDB\xBA\xC8\xDB\xDB\xCD\xBB\xDB\xDB\xBA  \xC8\xDB\xDB\xCD\xBB  \xDB\xDB\xBA   \xDB\xDB\xBA\xDB\xDB\xBA\xC8\xDB\xDB\xBB\xDB\xDB\xBA \xDB\xDB\xCD\xDB\xDB\xBB \n");
    printf("\xDB\xDB\xBA \xC8\xCD\xBB \xDB\xDB\xBA   \xDB\xDB\xBA   \xC8\xDB\xDB\xDB\xDB\xDB\xCD\xBB\xDB\xDB\xBA \xC8\xDB\xDB\xDB\xDB\xBA\xDB\xDB\xBA\xDB\xDB\xCD\xBB \xDB\xDB\xBB\n");
    printf("\xC8\xCD\xBB     \xC8\xCD\xBB   \xC8\xCD\xBB    \xC8\xCD\xCD\xCD\xCD\xBB \xC8\xCD\xBB  \xC8\xCD\xCD\xCD\xBB\xC8\xCD\xBB\xC8\xCD\xBB  \xC8\xCD\xBB\n");
    printf("\n");

    /* System Information */
    printf("%s@%s\n", "root", "dev_null");
    printf("---------------------------------\n");

    printf("OS: MYUNIXLIKEOS v1.0\n");
    printf("Kernel: Custom x86 Kernel\n");
    printf("Architecture: i386\n");
    printf("Uptime: %u seconds (%u minutes)\n",
           info.uptime, info.uptime / 60);
    printf("Processes: %u\n", info.procs);

    /* Memory Information */
    printf("\nMemory:\n");
    printf("  Total: %u KB\n", info.totalram / 1024);
    printf("  Free: %u KB\n", info.freeram / 1024);
    printf("  Used: %u KB\n", (info.totalram - info.freeram) / 1024);
    printf("  Unit: %u bytes\n", info.mem_unit);

    /* Calculate memory usage percentage */
    if (info.totalram > 0) {
        uint32_t used_percent = ((info.totalram - info.freeram) * 100) / info.totalram;
        printf("  Usage: %u%%\n", used_percent);
    }

    /* Swap Information */
    if (info.totalswap > 0) {
        printf("\nSwap:\n");
        printf("  Total: %u KB\n", info.totalswap / 1024);
        printf("  Free: %u KB\n", info.freeswap / 1024);
        printf("  Used: %u KB\n", (info.totalswap - info.freeswap) / 1024);

        uint32_t swap_used_percent = ((info.totalswap - info.freeswap) * 100) / info.totalswap;
        printf("  Usage: %u%%\n", swap_used_percent);
    }

    /* Additional system info */
    printf("\nSystem:\n");
    printf("  Shell: Custom Shell (sh)\n");
    printf("  Terminal: VGA Text Mode (80x25)\n");
    printf("  CPU: i386 Compatible\n");
    printf("  Graphics: VGA Text Mode\n");

    printf("\n");

    return 0;
}