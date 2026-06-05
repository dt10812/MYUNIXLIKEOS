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
#include "nvram.h"
#include "multiboot.h"
#include "keyboard.h"

char fs_type_name[16] = "Ext2";

/* Stack canary for buffer overflow protection */
uintptr_t __stack_chk_guard = 0xDEADBEEF;

/* Stack canary failure handler */
__attribute__((noreturn))
void __stack_chk_fail(void) {
    /* Stack corruption detected - panic */
    printf("\n*** STACK BUFFER OVERFLOW DETECTED ***\n");
    printf("System halting for security.\n");
    
    /* Disable interrupts and halt */
    asm volatile("cli");
    for (;;) {
        asm volatile("hlt");
    }
}

static uintptr_t stack_canary_seed(void) {
    uint8_t sec, min, hour, day, month, year, century;
    rtc_get_time(&sec, &min, &hour, &day, &month, &year, &century);

    uintptr_t seed = ((uintptr_t)sec << 24) | ((uintptr_t)min << 16) |
                     ((uintptr_t)hour << 8) | (uintptr_t)day;

    seed ^= ((uintptr_t)month << 24) | ((uintptr_t)year << 16) |
            ((uintptr_t)century << 8) | (uintptr_t)&stack_canary_seed;
    seed ^= (uintptr_t)&__stack_chk_guard;
    seed ^= 0xA5A5A5A5u;

    if (seed == 0)
        seed = 0xDEADBEEF ^ (uintptr_t)&stack_canary_seed;

    return seed;
}

/* Initialize stack canary with runtime entropy */
static void init_stack_canary(void) {
    __stack_chk_guard = stack_canary_seed();
    if (__stack_chk_guard == 0 || (__stack_chk_guard & 0xFF) == 0)
        __stack_chk_guard |= 0xA1A5A5A5u;
}

/* Kernel log buffer for dmesg */
#define KERNEL_LOG_SIZE 4096
static char kernel_log_buffer[KERNEL_LOG_SIZE];
static size_t kernel_log_pos = 0;

void kernel_log(const char *msg) {
    size_t len = strlen(msg);
    if (kernel_log_pos + len >= KERNEL_LOG_SIZE) {
        // Wrap around if buffer is full
        kernel_log_pos = 0;
    }
    
    for (size_t i = 0; i < len && kernel_log_pos < KERNEL_LOG_SIZE; i++) {
        kernel_log_buffer[kernel_log_pos++] = msg[i];
    }
    
    // Null terminate
    if (kernel_log_pos < KERNEL_LOG_SIZE) {
        kernel_log_buffer[kernel_log_pos] = '\0';
    }
}

const char* get_kernel_log(void) {
    return kernel_log_buffer;
}

/* Physical Memory Manager - Dynamic bitmap based on real memory map */
static uint8_t* pmm_bitmap = NULL;
static size_t pmm_bitmap_size = 0;
static uintptr_t pmm_memory_start = 0;
static size_t pmm_total_memory_pages = 0;
static size_t pmm_available_pages = 0;

/* Kernel heap (for paging_init use only) */
#define KERNEL_HEAP_START 0xC0000000  /* 3GB */
#define KERNEL_HEAP_SIZE  0x10000000  /* 256MB */

/* Paging */
static page_directory_t* kernel_page_directory = NULL;
static page_table_t* kernel_page_tables[PAGE_DIRECTORY_SIZE] = {0};

void bitmap_set(uint64_t bit) {
    if (bit / 8 >= pmm_bitmap_size) return;
    pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}

void bitmap_clear(uint64_t bit) {
    if (bit / 8 >= pmm_bitmap_size) return;
    pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

int bitmap_test(uint64_t bit) {
    if (bit / 8 >= pmm_bitmap_size) return 0;
    return (pmm_bitmap[bit / 8] >> (bit % 8)) & 1;
}

void pmm_init(void* mmap_tag) {
    if (!mmap_tag) {
        /* Fallback to minimal memory for testing */
        pmm_memory_start = 0x100000; /* 1MB */
        pmm_total_memory_pages = 1024; /* 4MB */
        pmm_bitmap_size = (pmm_total_memory_pages + 7) / 8;
        pmm_bitmap = (uint8_t*)0x200000; /* 2MB */
        
        /* Mark all pages as free initially */
        for (size_t i = 0; i < pmm_bitmap_size; i++) {
            pmm_bitmap[i] = 0;
        }
        
        /* Reserve the bitmap itself - bitmap is at 0x200000, page 512 */
        size_t bitmap_page_start = 0x200000 / PAGE_SIZE; /* 512 */
        size_t bitmap_pages = (pmm_bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;
        for (size_t i = 0; i < bitmap_pages; i++) {
            bitmap_set(bitmap_page_start + i);
        }
        pmm_available_pages = pmm_total_memory_pages - bitmap_pages;
        return;
    }
    
    struct multiboot_tag_mmap* mmap = (struct multiboot_tag_mmap*)mmap_tag;
    struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)mmap->entries;
    
    /* Find the largest available memory region */
    uintptr_t max_addr = 0;
    size_t max_len = 0;
    
    size_t num_entries = (mmap->size - sizeof(struct multiboot_tag_mmap)) / mmap->entry_size;
    for (size_t i = 0; i < num_entries; i++) {
        if (entry[i].type == MULTIBOOT_MEMORY_AVAILABLE && 
            entry[i].len > max_len &&
            entry[i].addr >= 0x100000) { /* Skip low memory */
            max_addr = entry[i].addr;
            max_len = entry[i].len;
        }
    }
    
    if (max_len == 0) {
        /* No suitable memory found, fallback */
        pmm_init(NULL);
        return;
    }
    
    pmm_memory_start = max_addr;
    pmm_total_memory_pages = max_len / PAGE_SIZE;
    
    /* Limit to reasonable size for bitmap */
    if (pmm_total_memory_pages > 32768) { /* 128MB */
        pmm_total_memory_pages = 32768;
    }
    
    pmm_bitmap_size = (pmm_total_memory_pages + 7) / 8;
    /* Place bitmap at fixed address to avoid overwriting kernel */
    pmm_bitmap = (uint8_t*)0x200000; /* 2MB */
    
    /* Initialize bitmap to all free */
    for (size_t i = 0; i < pmm_bitmap_size; i++) {
        pmm_bitmap[i] = 0;
    }
    
    /* Reserve the bitmap itself - if it's within managed memory */
    uintptr_t bitmap_start = 0x200000;
    uintptr_t bitmap_end = bitmap_start + pmm_bitmap_size;
    if (bitmap_start >= pmm_memory_start && bitmap_start < pmm_memory_start + pmm_total_memory_pages * PAGE_SIZE) {
        size_t start_page = (bitmap_start - pmm_memory_start) / PAGE_SIZE;
        size_t end_page = (bitmap_end - pmm_memory_start + PAGE_SIZE - 1) / PAGE_SIZE;
        for (size_t i = start_page; i < end_page && i < pmm_total_memory_pages; i++) {
            bitmap_set(i);
        }
    }
    
    /* Mark unavailable regions from memory map */
    for (size_t i = 0; i < num_entries; i++) {
        if (entry[i].type != MULTIBOOT_MEMORY_AVAILABLE) {
            uintptr_t start = entry[i].addr;
            uintptr_t end = start + entry[i].len;
            
            /* Convert to page indices relative to our memory region */
            if (start < pmm_memory_start) start = pmm_memory_start;
            if (end > pmm_memory_start + pmm_total_memory_pages * PAGE_SIZE) {
                end = pmm_memory_start + pmm_total_memory_pages * PAGE_SIZE;
            }
            
            if (start < end) {
                size_t start_page = (start - pmm_memory_start) / PAGE_SIZE;
                size_t end_page = (end - pmm_memory_start + PAGE_SIZE - 1) / PAGE_SIZE;
                
                for (size_t p = start_page; p < end_page && p < pmm_total_memory_pages; p++) {
                    bitmap_set(p);
                }
            }
        }
    }
    
    /* Count available pages */
    pmm_available_pages = 0;
    for (size_t i = 0; i < pmm_total_memory_pages; i++) {
        if (!bitmap_test(i)) pmm_available_pages++;
    }
    
    /* Reserve kernel and modules (assume first 8MB is reserved) */
    for (size_t i = 0; i < 2048 && i < pmm_total_memory_pages; i++) {
        bitmap_set(i);
        pmm_available_pages--;
    }
}

void* pmm_alloc_page(void) {
    for (size_t i = 0; i < pmm_total_memory_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            pmm_available_pages--;
            return (void*)(pmm_memory_start + i * PAGE_SIZE);
        }
    }
    return NULL;
}

void pmm_free_page(void* ptr) {
    if (!ptr) return;
    uintptr_t addr = (uintptr_t)ptr;
    if (addr < pmm_memory_start || addr >= pmm_memory_start + pmm_total_memory_pages * PAGE_SIZE) {
        return;
    }
    size_t page_idx = (addr - pmm_memory_start) / PAGE_SIZE;
    if (page_idx < pmm_total_memory_pages && bitmap_test(page_idx)) {
        bitmap_clear(page_idx);
        pmm_available_pages++;
    }
}

void* pmm_alloc_blocks(size_t count) {
    if (count == 0 || count > pmm_available_pages) return NULL;
    
    /* Find contiguous free pages */
    size_t start_page = 0;
    size_t consecutive_free = 0;
    
    for (size_t i = 0; i < pmm_total_memory_pages; i++) {
        if (!bitmap_test(i)) {
            if (consecutive_free == 0) {
                start_page = i;
            }
            consecutive_free++;
            if (consecutive_free >= count) {
                /* Found enough consecutive pages, mark them as used */
                for (size_t j = 0; j < count; j++) {
                    bitmap_set(start_page + j);
                }
                pmm_available_pages -= count;
                return (void*)(pmm_memory_start + start_page * PAGE_SIZE);
            }
        } else {
            consecutive_free = 0;
        }
    }
    
    return NULL; /* No contiguous block found */
}

void pmm_free_blocks(void* ptr, size_t count) {
    if (!ptr || count == 0) return;
    
    uintptr_t addr = (uintptr_t)ptr;
    if (addr < pmm_memory_start || addr >= pmm_memory_start + pmm_total_memory_pages * PAGE_SIZE) {
        return;
    }
    
    size_t start_page = (addr - pmm_memory_start) / PAGE_SIZE;
    if (start_page + count > pmm_total_memory_pages) {
        count = pmm_total_memory_pages - start_page;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (bitmap_test(start_page + i)) {
            bitmap_clear(start_page + i);
            pmm_available_pages++;
        }
    }
}

void* pmm_alloc_z(size_t size) {
    if (size == 0) return NULL;

    size_t page_count = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    void* p = pmm_alloc_blocks(page_count);
    if (!p) return NULL;

    memset(p, 0, page_count * PAGE_SIZE);
    return p;
}

size_t pmm_total_pages(void) {
    return pmm_total_memory_pages;
}

size_t pmm_used_pages(void) {
    return pmm_total_memory_pages - pmm_available_pages;
}

size_t pmm_free_pages(void) {
    return pmm_available_pages;
}

/* Kernel heap allocator - page-backed, node-coalescing heap */
#define HEAP_START KERNEL_HEAP_START
#define HEAP_SIZE  KERNEL_HEAP_SIZE
#define HEAP_ALIGN 8
#define HEAP_PAGE_MAGIC 0xC0FFEE42u
#define HEAP_BLOCK_MAGIC 0xA110CA7Eu
#define HEAP_MIN_SPLIT 16

typedef struct heap_block {
    uint32_t magic;
    size_t size;
    struct heap_block* next;
    struct heap_block* prev;
    int free;
    struct heap_page* page;
} heap_block_t;

typedef struct heap_page {
    uint32_t magic;
    uintptr_t phys_addr;
    uintptr_t virt_addr;
    struct heap_page* next;
    struct heap_page* prev;
    heap_block_t* first_block;
} heap_page_t;

static heap_page_t* heap_pages = NULL;
static uintptr_t heap_next_virtual = HEAP_START;

static inline uintptr_t align_up(uintptr_t value, uintptr_t alignment) {
    uintptr_t mask = alignment - 1;
    return (value + mask) & ~mask;
}

static inline uintptr_t align_down(uintptr_t value, uintptr_t alignment) {
    uintptr_t mask = alignment - 1;
    return value & ~mask;
}

static int heap_validate_block(heap_page_t* page, heap_block_t* block) {
    if (!page || !block) {
        return 0;
    }
    if (block->magic != HEAP_BLOCK_MAGIC) {
        return 0;
    }
    if (block->page != page) {
        return 0;
    }

    uintptr_t page_start = page->virt_addr;
    uintptr_t page_end = page_start + PAGE_SIZE;
    uintptr_t block_addr = (uintptr_t)block;

    if (block_addr < page_start || block_addr + sizeof(heap_block_t) > page_end) {
        return 0;
    }
    if ((block_addr & (HEAP_ALIGN - 1)) != 0) {
        return 0;
    }
    if (block->size == 0 || (block->size & (HEAP_ALIGN - 1)) != 0) {
        return 0;
    }
    uintptr_t block_end = block_addr + sizeof(heap_block_t);
    if (block_end > page_end || block->size > page_end - block_end) {
        return 0;
    }

    uintptr_t expected_next = block_end + block->size;
    if (block->next) {
        uintptr_t next_addr = (uintptr_t)block->next;
        if (next_addr < page_start || next_addr > page_end) {
            return 0;
        }
        if (next_addr + sizeof(heap_block_t) > page_end) {
            return 0;
        }
        if (next_addr != expected_next) {
            return 0;
        }
        heap_block_t* next_block = (heap_block_t*)next_addr;
        if (next_block->magic != HEAP_BLOCK_MAGIC) {
            return 0;
        }
        if (next_block->prev != block) {
            return 0;
        }
    }

    return 1;
}

static int heap_validate_page(heap_page_t* page) {
    if (!page || page->magic != HEAP_PAGE_MAGIC) {
        return 0;
    }
    if (page->virt_addr < HEAP_START || page->virt_addr + PAGE_SIZE > HEAP_START + HEAP_SIZE) {
        return 0;
    }
    if (page->first_block == NULL) {
        return 0;
    }
    if (page->first_block->page != page) {
        return 0;
    }

    heap_block_t* current = page->first_block;
    while (current) {
        if (!heap_validate_block(page, current)) {
            return 0;
        }
        current = current->next;
    }

    return 1;
}

static heap_page_t* heap_alloc_page(void) {
    if (heap_next_virtual + PAGE_SIZE > HEAP_START + HEAP_SIZE) {
        return NULL;
    }

    void* phys = pmm_alloc_page();
    if (!phys) {
        return NULL;
    }

    uintptr_t virt = heap_next_virtual;
    heap_next_virtual += PAGE_SIZE;

    map_page(virt, (uintptr_t)phys, PAGE_PRESENT | PAGE_WRITE);

    heap_page_t* page = (heap_page_t*)virt;
    memset(page, 0, sizeof(heap_page_t));
    page->magic = HEAP_PAGE_MAGIC;
    page->phys_addr = (uintptr_t)phys;
    page->virt_addr = virt;

    uintptr_t block_addr = align_up(virt + sizeof(heap_page_t), HEAP_ALIGN);
    size_t block_size = PAGE_SIZE - (block_addr - virt) - sizeof(heap_block_t);
    block_size = align_down(block_size, HEAP_ALIGN);

    heap_block_t* block = (heap_block_t*)block_addr;
    memset(block, 0, sizeof(heap_block_t));
    block->magic = HEAP_BLOCK_MAGIC;
    block->size = block_size;
    block->next = NULL;
    block->prev = NULL;
    block->free = 1;
    block->page = page;
    page->first_block = block;

    page->next = heap_pages;
    page->prev = NULL;
    if (heap_pages) {
        heap_pages->prev = page;
    }
    heap_pages = page;

    return page;
}

static heap_block_t* heap_find_free_block(heap_page_t* page, size_t size) {
    if (!heap_validate_page(page)) {
        return NULL;
    }

    heap_block_t* current = page->first_block;
    while (current) {
        if (!heap_validate_block(page, current)) {
            return NULL;
        }
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void heap_init(void) {
    heap_pages = NULL;
    heap_next_virtual = HEAP_START;

    heap_page_t* page = heap_alloc_page();
    if (!page) {
        printf("Failed to allocate heap page\n");
        return;
    }
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;

    if (size > (size_t)-1 - (HEAP_ALIGN - 1)) {
        return NULL;
    }
    size = align_up(size, HEAP_ALIGN);

    while (1) {
        heap_page_t* page = heap_pages;
        while (page) {
            if (!heap_validate_page(page)) {
                printf("heap: corrupted page detected\n");
                return NULL;
            }

            heap_block_t* block = heap_find_free_block(page, size);
            if (block) {
                uintptr_t block_addr = (uintptr_t)block;
                uintptr_t page_end = page->virt_addr + PAGE_SIZE;
                uintptr_t payload_end = block_addr + sizeof(heap_block_t);
                size_t remaining = 0;

                if (payload_end > page_end) {
                    printf("heap: corrupted block boundary\n");
                    return NULL;
                }

                if (size <= page_end - payload_end) {
                    uintptr_t remainder = payload_end + size;
                    remaining = page_end - remainder;

                    if (remaining >= sizeof(heap_block_t) + HEAP_MIN_SPLIT) {
                        heap_block_t* new_block = (heap_block_t*)remainder;
                        memset(new_block, 0, sizeof(heap_block_t));
                        new_block->magic = HEAP_BLOCK_MAGIC;
                        new_block->size = align_down(remaining - sizeof(heap_block_t), HEAP_ALIGN);
                        new_block->free = 1;
                        new_block->page = page;
                        new_block->prev = block;
                        new_block->next = block->next;
                        if (block->next) {
                            block->next->prev = new_block;
                        }
                        block->next = new_block;
                        block->size = size;
                    }
                }

                block->free = 0;
                return (void*)((char*)block + sizeof(heap_block_t));
            }
            page = page->next;
        }

        if (!heap_alloc_page()) {
            return NULL;
        }
    }
}

void kfree(void* ptr) {
    if (!ptr) return;

    uintptr_t ptr_addr = (uintptr_t)ptr;
    if (ptr_addr < HEAP_START || ptr_addr >= HEAP_START + HEAP_SIZE) {
        return;
    }

    heap_block_t* block = (heap_block_t*)((char*)ptr - sizeof(heap_block_t));
    if ((uintptr_t)block < HEAP_START || (uintptr_t)block >= HEAP_START + HEAP_SIZE) {
        return;
    }
    if (block->magic != HEAP_BLOCK_MAGIC) return;

    heap_page_t* page = block->page;
    if (!page || page->magic != HEAP_PAGE_MAGIC) return;
    if (!heap_validate_page(page)) {
        printf("heap: corrupted page detected during free\n");
        return;
    }

    uintptr_t page_start = page->virt_addr;
    uintptr_t page_end = page_start + PAGE_SIZE;
    uintptr_t block_addr = (uintptr_t)block;
    if (block_addr < page_start || block_addr + sizeof(heap_block_t) > page_end) return;
    if ((uintptr_t)block->next != 0 && (uintptr_t)block->next != block_addr + sizeof(heap_block_t) + block->size) {
        printf("heap: corrupted block links\n");
        return;
    }

    if (block->free) return;

    block->free = 1;

    if (block->next && block->next->free) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    if (block->prev && block->prev->free) {
        block->prev->size += sizeof(heap_block_t) + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

/* Paging implementation */
void paging_init(void) {
    /* Allocate page directory */
    kernel_page_directory = (page_directory_t*)pmm_alloc_page();
    if (!kernel_page_directory) {
        printf("Failed to allocate page directory\n");
        return;
    }
    
    /* Clear page directory */
    memset(kernel_page_directory, 0, sizeof(page_directory_t));
    
    /* Identity map the low physical region used during bootstrap. The PMM
       reserves the first 8MB, so map enough low memory for the page tables
       that will live there (16MB total, four 4MB tables). */
    for (int table_index = 0; table_index < 4; table_index++) {
        page_table_t* table = (page_table_t*)alloc_page_table();
        if (!table) {
            printf("Failed to allocate page table %d\n", table_index);
            return;
        }

        for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
            uintptr_t phys = ((uintptr_t)table_index * PAGE_TABLE_SIZE + i) * PAGE_SIZE;
            (*table)[i] = phys | PAGE_PRESENT | PAGE_WRITE;
        }

        (*kernel_page_directory)[table_index] = (uint32_t)table | PAGE_PRESENT | PAGE_WRITE;
        kernel_page_tables[table_index] = table;
    }
    
    /* Map kernel heap */
    for (uintptr_t addr = KERNEL_HEAP_START; addr < KERNEL_HEAP_START + KERNEL_HEAP_SIZE; addr += PAGE_SIZE * PAGE_TABLE_SIZE) {
        page_table_t* table = (page_table_t*)alloc_page_table();
        if (!table) continue;
        
        (*kernel_page_directory)[addr >> 22] = (uint32_t)table | PAGE_PRESENT | PAGE_WRITE;
    }
}

void* alloc_page_table(void) {
    page_table_t* table = (page_table_t*)pmm_alloc_page();
    if (!table) {
        return NULL;
    }

    /* Clear the table */
    memset(table, 0, sizeof(page_table_t));

    return table;
}

void map_page(uintptr_t virtual, uintptr_t physical, uint32_t flags) {
    uint32_t pd_index = virtual >> 22;
    uint32_t pt_index = (virtual >> 12) & 0x3FF;
    
    /* Allocate page table if needed */
    if (!((*kernel_page_directory)[pd_index] & PAGE_PRESENT)) {
        page_table_t* table = (page_table_t*)alloc_page_table();
        if (!table) return;

        uint32_t pd_flags = PAGE_PRESENT | PAGE_WRITE;
        if (flags & PAGE_USER)
            pd_flags |= PAGE_USER;

        (*kernel_page_directory)[pd_index] = (uint32_t)table | pd_flags;
        kernel_page_tables[pd_index] = table;
    }
    
    page_table_t* table = (page_table_t*)((*kernel_page_directory)[pd_index] & 0xFFFFF000);
    (*table)[pt_index] = (physical & 0xFFFFF000) | flags;

    __asm__ volatile("invlpg (%0)" : : "r"(virtual) : "memory");
}

void unmap_page(uintptr_t virtual) {
    uint32_t pd_index = virtual >> 22;
    uint32_t pt_index = (virtual >> 12) & 0x3FF;
    
    if (!((*kernel_page_directory)[pd_index] & PAGE_PRESENT)) return;
    
    page_table_t* table = (page_table_t*)((*kernel_page_directory)[pd_index] & 0xFFFFF000);
    (*table)[pt_index] = 0;
    
    /* Invalidate TLB entry */
    __asm__ volatile("invlpg (%0)" : : "r"(virtual) : "memory");
}

void enable_paging(void) {
    /* Load page directory */
    __asm__ volatile("mov %0, %%cr3" : : "r"(kernel_page_directory));
    
    /* Enable paging */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; /* Set PG bit */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

uintptr_t get_physical_address(uintptr_t virtual) {
    uint32_t pd_index = virtual >> 22;
    uint32_t pt_index = (virtual >> 12) & 0x3FF;
    
    if (!((*kernel_page_directory)[pd_index] & PAGE_PRESENT)) return 0;
    
    page_table_t* table = (page_table_t*)((*kernel_page_directory)[pd_index] & 0xFFFFF000);
    if (!((*table)[pt_index] & PAGE_PRESENT)) return 0;
    
    return ((*table)[pt_index] & 0xFFFFF000) + (virtual & 0xFFF);
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

void kernel_main(uint32_t magic, struct multiboot_info* mbi) {
    /* Init serial for debugging */
    serial_init();
    
    /* Validate multiboot */
    if (magic != MULTIBOOT2_INFO_MAGIC || !mbi) {
        __asm__ volatile("cli; hlt");
    }
    
    /* Parse memory map */
    struct multiboot_tag* tag = (struct multiboot_tag*)(mbi->tags);
    struct multiboot_tag_mmap* mmap_tag = NULL;
    
    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            mmap_tag = (struct multiboot_tag_mmap*)tag;
            break;
        }
        tag = (struct multiboot_tag*)((uint8_t*)tag + ((tag->size + 7) & ~7));
    }
    
    /* Initialize components */
    pmm_init(mmap_tag);
    paging_init();
    heap_init();
    enable_paging();
    fs_init();
    gdt_init();
    idt_init();
    keyboard_init();
    
    /* Initialize stack canary for buffer overflow protection */
    init_stack_canary();
    tss_set_kernel_stack((uint32_t)&stack_top);
    install_user_progs();

    unsigned total_kb = (unsigned)(pmm_total_pages() * 4);
    unsigned free_kb = (unsigned)(pmm_free_pages() * 4);

    /* Try printf */
    printf("*nix IA-32 Kernel booted\n");
    printf("Total memory: %u KB\n", total_kb);
    printf("Free memory: %u KB\n", free_kb);
    
    /* Get and display current time */
    time_t t;
    if (time(&t) != (time_t)-1) {
        struct tm bt;
        if (localtime_r(&t, &bt) != NULL &&
            bt.tm_wday >= 0 && bt.tm_wday < 7 &&
            bt.tm_mon >= 0 && bt.tm_mon < 12) {
            printf("TIME OK\n");
        } else {
            printf("UTC time unavailable\n");
        }
    } else {
        printf("UTC time unavailable\n");
    }
    
    /* Display splash screen */
    printf("\n");
    printf("\xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB\n");
    printf("\xBA                                                        \xBA\n");
    printf("\xBA           Welcome to MYUNIXLIKEOS v1.0.0              \xBA\n");
    printf("\xBA                                                        \xBA\n");
    printf("\xBA        A minimal Unix-like OS for x86 systems         \xBA\n");
    printf("\xBA                                                        \xBA\n");
    printf("\xBA  Type 'help' for available commands                   \xBA\n");
    printf("\xBA  Type 'man <command>' for command documentation       \xBA\n");
    printf("\xBA                                                        \xBA\n");
    printf("\xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC\n");
    printf("\n");
    

    /* Start the shell */
    sh();
}