#include "stdio.h"
#include "pmm.h"
#include "commands.h"

int cmd_vmstat(int argc, char** argv) {
    (void)argc;
    (void)argv;
    size_t free_pages = pmm_free_pages();

    printf("procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----\n");
    printf(" r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st\n");
    printf(" 0  0      0 %6u      0      0    0    0     0     0    0    0  0  0 100  0  0\n",
           (unsigned)(free_pages * PAGE_SIZE / 1024));

    return 0;
}