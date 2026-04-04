/* TODO:
 * - Move more things out of here.
 * - Move keyboard drivers to a seperate file.
 */
#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"
#include "vfs.h"
#include "pmm.h"
#include "io.h"
#include "commands.h"
#include "string.h"
#include "stdio.h"
#include "sys/syscall.h"
#include "idt.h"
#include "gdt.h"
#include "sys/time.h"

char fs_type_name[16] = "Ext2";

#define MAX_PAGES 128
static uint8_t pmm_bitmap[MAX_PAGES / 8];
static uint8_t pmm_page_pool[MAX_PAGES][PAGE_SIZE];

void bitmap_set(uint64_t bit) {
    if (bit >= MAX_PAGES) return;
    pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}

void bitmap_clear(uint64_t bit) {
    if (bit >= MAX_PAGES) return;
    pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

int bitmap_test(uint64_t bit) {
    if (bit >= MAX_PAGES) return 0;
    return (pmm_bitmap[bit / 8] >> (bit % 8)) & 1;
}

void pmm_init(void* mmap_tag) {
    (void)mmap_tag;
    for (size_t i = 0; i < sizeof(pmm_bitmap); i++) pmm_bitmap[i] = 0;
}

void* pmm_alloc_page(void) {
    for (uint64_t i = 0; i < MAX_PAGES; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            return pmm_page_pool[i];
        }
    }
    return NULL;
}

void pmm_free_page(void* ptr) {
    if (!ptr) return;
    uintptr_t base = (uintptr_t)pmm_page_pool;
    uintptr_t cur  = (uintptr_t)ptr;
    if (cur < base) return;
    uint64_t idx = (cur - base) / PAGE_SIZE;
    if (idx < MAX_PAGES) bitmap_clear(idx);
}

void* pmm_alloc_blocks(size_t count) {
    if (count == 0) return NULL;
    return pmm_alloc_page();
}

void* pmm_alloc_z(size_t size) {
    void* p = pmm_alloc_page();
    if (!p) return NULL;
    size_t n = size < PAGE_SIZE ? size : PAGE_SIZE;
    for (size_t i = 0; i < n; i++) ((uint8_t*)p)[i] = 0;
    return p;
}

size_t pmm_total_pages(void) {
    return MAX_PAGES;
}

size_t pmm_used_pages(void) {
    size_t total = 0;
    for (uint64_t i = 0; i < MAX_PAGES; i++) {
        if (bitmap_test(i))
            total++;
    }
    return total;
}

size_t pmm_free_pages(void) {
    return pmm_total_pages() - pmm_used_pages();
}

/* kernel space streams */

static unsigned char _stdout_buf[2048];
static unsigned char _stderr_buf[256];
static unsigned char _stdin_buf[256];

static FILE _stdout_f = {
    .flags  = 1,
    .fileno = 1,
    .buf    = _stdout_buf,
    .bptr   = _stdout_buf,
    .len    = sizeof(_stdout_buf),
    .free   = 0,
    .iobf   = _IOLBF,
    .eof    = 0,
};

static FILE _stderr_f = {
    .flags  = 1,
    .fileno = 2,
    .buf    = _stderr_buf,
    .bptr   = _stderr_buf,
    .len    = sizeof(_stderr_buf),
    .free   = 0,
    .iobf   = _IONBF,
    .eof    = 0,
};

static FILE _stdin_f = {
    .flags  = 0,
    .fileno = 0,
    .buf    = _stdin_buf,
    .bptr   = _stdin_buf,
    .len    = sizeof(_stdin_buf),
    .free   = 0,
    .iobf   = _IOLBF,
    .eof    = 0,
};

FILE *stdout = &_stdout_f;
FILE *stderr = &_stderr_f;
FILE *stdin  = &_stdin_f;

/* stack_top is defined in entry.asm */
extern uint32_t stack_top;
extern void install_user_progs();

void kernel_main(void) {
    pmm_init(NULL);
    fs_init();
    idt_init();
    gdt_init();

    /* set TSS esp0 to top of kernel stack — stable for all ring 3 syscalls */
    tss_set_kernel_stack((uint32_t)&stack_top);

    install_user_progs();

    printf("*nix IA-32 Kernel booted\n");

    /* TODO: strftime */
    time_t t;
    time(&t);
    struct tm bt;
    const char *days[]   = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    localtime_r(&t, &bt);
    printf("%s %s %i:%i %i UTC\n", days[bt.tm_wday],
        months[bt.tm_mon], bt.tm_hour, bt.tm_min, bt.tm_year);
    sh();
}