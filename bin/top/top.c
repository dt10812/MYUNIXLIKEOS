#include "stdio.h"
#include "pmm.h"
#include "commands.h"

int cmd_top(int argc, char** argv) {
    (void)argc;
    (void)argv;

    size_t total_pages = pmm_total_pages();
    size_t used_pages = pmm_used_pages();
    size_t free_pages = pmm_free_pages();
    size_t total_kb = (total_pages * PAGE_SIZE) / 1024;
    size_t used_kb = (used_pages * PAGE_SIZE) / 1024;
    size_t free_kb = (free_pages * PAGE_SIZE) / 1024;

    printf("top - %02u:%02u:%02u up\n", 0, 0, 0); /* TODO: actual uptime */
    printf("Tasks:   1 total,   1 running,   0 sleeping,   0 stopped,   0 zombie\n");
    printf("%%Cpu(s):  0.0 us,  0.0 sy,  0.0 ni,100.0 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st\n");
    printf("KiB Mem : %8u total, %8u free, %8u used\n", 
           (unsigned)total_kb, (unsigned)free_kb, (unsigned)used_kb);
    printf("KiB Swap: %8u total, %8u free, %8u used\n", 0, 0, 0);

    printf("\n  PID USER      PR  NI    VIRT    RES    SHR S  %%CPU %%MEM     TIME+ COMMAND\n");
    printf("    1 root      20   0    1024    512    256 S   0.0  0.0   0:00.00 init\n");

    return 0;
}