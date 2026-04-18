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
#include "multiboot.h"

char fs_type_name[16] = "Ext2";

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
    serial_write("pmm_start");
    if (!mmap_tag) {
        serial_write("fallback");
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
        serial_write("fallback_done");
        return;
    }
    
    serial_write("parsing_mmap");
    struct multiboot_tag_mmap* mmap = (struct multiboot_tag_mmap*)mmap_tag;
    struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)mmap->entries;
    
    /* Find the largest available memory region */
    uintptr_t max_addr = 0;
    size_t max_len = 0;
    
    size_t num_entries = (mmap->size - sizeof(struct multiboot_tag_mmap)) / mmap->entry_size;
    serial_write("num_entries:");
    serial_write(num_entries < 10 ? (char[]){'0' + num_entries, '\0'} : "many");
    for (size_t i = 0; i < num_entries; i++) {
        if (entry[i].type == MULTIBOOT_MEMORY_AVAILABLE && 
            entry[i].len > max_len &&
            entry[i].addr >= 0x100000) { /* Skip low memory */
            max_addr = entry[i].addr;
            max_len = entry[i].len;
        }
    }
    
    serial_write("max_len_found");
    if (max_len == 0) {
        serial_write("no_mem_fallback");
        /* No suitable memory found, fallback */
        pmm_init(NULL);
        return;
    }
    
    serial_write("setting_pmm");
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
    serial_write("pmm_done");
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
    if (count == 0) return NULL;
    
    /* For now, just allocate single pages */
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
    return pmm_total_memory_pages;
}

size_t pmm_used_pages(void) {
    return pmm_total_memory_pages - pmm_available_pages;
}

size_t pmm_free_pages(void) {
    return pmm_available_pages;
}

/* Kernel heap allocator */
void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    /* For now, just allocate whole pages */
    return pmm_alloc_page();
}

void kfree(void* ptr) {
    pmm_free_page(ptr);
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
    
    /* Identity map first 4MB (kernel) */
    page_table_t* first_table = (page_table_t*)alloc_page_table();
    if (!first_table) {
        printf("Failed to allocate first page table\n");
        return;
    }
    
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        (*first_table)[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE;
    }
    
    (*kernel_page_directory)[0] = (uint32_t)first_table | PAGE_PRESENT | PAGE_WRITE;
    
    /* Map kernel heap */
    for (uintptr_t addr = KERNEL_HEAP_START; addr < KERNEL_HEAP_START + KERNEL_HEAP_SIZE; addr += PAGE_SIZE * PAGE_TABLE_SIZE) {
        page_table_t* table = (page_table_t*)alloc_page_table();
        if (!table) continue;
        
        (*kernel_page_directory)[addr >> 22] = (uint32_t)table | PAGE_PRESENT | PAGE_WRITE;
    }
}

void* alloc_page_table(void) {
    page_table_t* table = (page_table_t*)pmm_alloc_page();
    if (!table) return NULL;
    
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
        (*kernel_page_directory)[pd_index] = (uint32_t)table | PAGE_PRESENT | PAGE_WRITE;
        kernel_page_tables[pd_index] = table;
    }
    
    page_table_t* table = (page_table_t*)((*kernel_page_directory)[pd_index] & 0xFFFFF000);
    (*table)[pt_index] = (physical & 0xFFFFF000) | flags;
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
    /* Direct VGA debug - earliest possible output */
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    int vga_idx = 0;
    
    /* Init serial for debugging */
    serial_init();
    
    /* Write "K" to VGA and serial to confirm we're running */
    vga[vga_idx++] = 0x0F00 | 'K';
    serial_write("K");
    
    /* Validate multiboot */
    if (magic != MULTIBOOT2_INFO_MAGIC || !mbi) {
        vga[vga_idx++] = 0x0C00 | 'B';  /* Red 'B' for bad multiboot */
        serial_write("B");
        __asm__ volatile("cli; hlt");
    }
    
    /* Parse memory map */
    vga[vga_idx++] = 0x0200 | '1';  /* Green '1' */
    serial_write("1");
    struct multiboot_tag* tag = (struct multiboot_tag*)(mbi->tags);
    struct multiboot_tag_mmap* mmap_tag = NULL;
    
    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            mmap_tag = (struct multiboot_tag_mmap*)tag;
            break;
        }
        tag = (struct multiboot_tag*)((uint8_t*)tag + ((tag->size + 7) & ~7));
    }
    
    /* Initialize components incrementally */
    vga[vga_idx++] = 0x0200 | 'P';  /* Green 'P' - about to init PMM */
    serial_write("P");
    pmm_init(mmap_tag);
    vga[vga_idx++] = 0x0200 | 'p';  /* Green 'p' - PMM done */
    serial_write("p");
    
    vga[vga_idx++] = 0x0200 | 'F';  /* Green 'F' - about to init FS */
    serial_write("F");
    fs_init();
    vga[vga_idx++] = 0x0200 | 'f';  /* Green 'f' - FS done */
    serial_write("f");
    
    vga[vga_idx++] = 0x0200 | 'G';  /* Green 'G' - about to init GDT */
    serial_write("G");
    gdt_init();  /* <-- potential crash point */
    vga[vga_idx++] = 0x0200 | 'g';  /* Green 'g' - GDT done */
    serial_write("g");
    
    vga[vga_idx++] = 0x0200 | 'I';  /* Green 'I' - about to init IDT */
    serial_write("I");
    idt_init();  /* <-- potential crash point */
    vga[vga_idx++] = 0x0200 | 'i';  /* Green 'i' - IDT done */
    serial_write("i");
    
    vga[vga_idx++] = 0x0200 | 'T';  /* Green 'T' - about to set TSS */
    serial_write("T");
    tss_set_kernel_stack((uint32_t)&stack_top);
    vga[vga_idx++] = 0x0200 | 't';  /* Green 't' - TSS done */
    serial_write("t");
    
    vga[vga_idx++] = 0x0200 | 'U';  /* Green 'U' - about to install progs */
    serial_write("U");
    install_user_progs();
    vga[vga_idx++] = 0x0200 | 'u';  /* Green 'u' - progs done */
    serial_write("u");
    
    vga[vga_idx++] = 0x0E00 | ':';  /* Yellow ':' - about to printf */
    serial_write(":");
    
    /* Try printf */
    printf("*nix IA-32 Kernel booted\n");
    printf("Total memory: %u KB\n", (unsigned)(pmm_total_pages() * 4));
    printf("Free memory: %u KB\n", (unsigned)(pmm_free_pages() * 4));
    
    /* Get and display current time */
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
    
    /* Start the shell */
    sh();
}