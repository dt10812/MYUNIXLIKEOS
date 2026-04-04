#include "stdio.h"
#include "pmm.h"
#include "commands.h"

int cmd_memory(int argc, char** argv) {
    (void)argc;
    (void)argv;

    size_t total_pages = pmm_total_pages();
    size_t used_pages = pmm_used_pages();
    size_t free_pages = pmm_free_pages();
    size_t page_bytes = PAGE_SIZE;
    size_t total_kb = (total_pages * page_bytes) / 1024;
    size_t used_kb = (used_pages * page_bytes) / 1024;
    size_t free_kb = (free_pages * page_bytes) / 1024;

    printf("Memory usage:\n");
    printf("  Total pages: %u\n", (unsigned)total_pages);
    printf("  Used pages : %u\n", (unsigned)used_pages);
    printf("  Free pages : %u\n", (unsigned)free_pages);
    printf("  Page size  : %u bytes\n", (unsigned)page_bytes);
    printf("  Total RAM  : %u KB\n", (unsigned)total_kb);
    printf("  Used RAM   : %u KB\n", (unsigned)used_kb);
    printf("  Free RAM   : %u KB\n", (unsigned)free_kb);

    return 0;
}
