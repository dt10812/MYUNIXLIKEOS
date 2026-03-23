#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

void pmm_init(void* mmap_tag);
void* pmm_alloc_page();           // Find one free 4KB page
void  pmm_free_page(void* ptr);    // Mark a page as free
void* pmm_alloc_blocks(size_t count);
void* pmm_alloc_z(size_t size);    // Allocate zeroed pages

// Internal bit manipulation
void bitmap_set(uint64_t bit);
void bitmap_clear(uint64_t bit);
int  bitmap_test(uint64_t bit);

#endif