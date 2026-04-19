#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define PAGE_DIRECTORY_SIZE 1024
#define PAGE_TABLE_SIZE 1024

/* Page directory/table entry flags */
#define PAGE_PRESENT    0x1
#define PAGE_WRITE      0x2
#define PAGE_USER       0x4
#define PAGE_PWT        0x8
#define PAGE_PCD        0x10
#define PAGE_ACCESSED   0x20
#define PAGE_DIRTY      0x40
#define PAGE_PS         0x80
#define PAGE_GLOBAL     0x100

/* Page directory and table structures */
typedef uint32_t page_directory_t[PAGE_DIRECTORY_SIZE];
typedef uint32_t page_table_t[PAGE_TABLE_SIZE];

/* Kernel heap allocator */
void* kmalloc(size_t size);
void kfree(void* ptr);

/* Virtual memory functions */
void paging_init(void);
void* alloc_page_table(void);
void map_page(uintptr_t virtual, uintptr_t physical, uint32_t flags);
void unmap_page(uintptr_t virtual);
void enable_paging(void);
uintptr_t get_physical_address(uintptr_t virtual);

void pmm_init(void* mmap_tag);
void* pmm_alloc_page();             /* Find one free 4KB page */
void  pmm_free_page(void* ptr);     /* Mark a page as free */
void* pmm_alloc_blocks(size_t count); /* Allocate contiguous pages */
void  pmm_free_blocks(void* ptr, size_t count); /* Free contiguous pages */
void* pmm_alloc_z(size_t size);     /* Allocate zeroed pages */

size_t pmm_total_pages(void);
size_t pmm_used_pages(void);
size_t pmm_free_pages(void);

/* Internal bit manipulation */
void bitmap_set(uint64_t bit);
void bitmap_clear(uint64_t bit);
int  bitmap_test(uint64_t bit);

#endif