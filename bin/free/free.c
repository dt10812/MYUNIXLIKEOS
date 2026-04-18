#include "stdio.h"
#include "pmm.h"
#include "commands.h"

int cmd_free(int argc, char** argv) {
    (void)argc;
    (void)argv;

    size_t total_pages = pmm_total_pages();
    size_t used_pages = pmm_used_pages();
    size_t free_pages = pmm_free_pages();
    size_t total_kb = (total_pages * PAGE_SIZE) / 1024;
    size_t used_kb = (used_pages * PAGE_SIZE) / 1024;
    size_t free_kb = (free_pages * PAGE_SIZE) / 1024;

    printf("              total        used        free\n");
    printf("Mem:     %8u KB %8u KB %8u KB\n", 
           (unsigned)total_kb, (unsigned)used_kb, (unsigned)free_kb);

    return 0;
}