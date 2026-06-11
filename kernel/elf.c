#include "elf.h"
#include "string.h"
#include "stdio.h"
#include "pmm.h"

extern void map_page(uintptr_t virtual, uintptr_t physical, uint32_t flags);
extern void unmap_page(uintptr_t virtual);
extern uintptr_t get_physical_address(uintptr_t virtual);

#define USER_BASE 0x08048000u
#define USER_TOP  0xBFFFFFFFu
#define USER_STACK_VIRT (0xBFFFFFFF - PAGE_SIZE + 1)
#define USER_HEAP_LIMIT USER_STACK_VIRT
#define ELF_MAX_MAPPED_PAGES 2048

static uintptr_t user_heap_base = 0;
static uintptr_t user_heap_brk = 0;
static int user_heap_initialized = 0;

static inline uintptr_t page_align_down(uintptr_t addr);
static inline uintptr_t page_align_up(uintptr_t addr);

void user_heap_init(uintptr_t base) {
    user_heap_base = page_align_up(base);
    if (user_heap_base < USER_BASE) {
        user_heap_base = USER_BASE;
    }
    user_heap_brk = user_heap_base;
    user_heap_initialized = 1;
}

void *user_sbrk(intptr_t increment) {
    if (!user_heap_initialized) return (void *)-1;
    uintptr_t old_brk = user_heap_brk;
    if (increment == 0) return (void *)old_brk;

    uintptr_t new_brk = (increment < 0)
        ? (uintptr_t)((intptr_t)old_brk + increment)
        : old_brk + increment;

    if (new_brk < user_heap_base || new_brk > USER_HEAP_LIMIT) {
        return (void *)-1;
    }

    if (increment > 0) {
        uintptr_t start = page_align_up(old_brk);
        uintptr_t end = page_align_up(new_brk);
        for (uintptr_t addr = start; addr < end; addr += PAGE_SIZE) {
            void *phys = pmm_alloc_page();
            if (!phys) {
                /* Roll back any newly mapped pages */
                for (uintptr_t rollback = start; rollback < addr; rollback += PAGE_SIZE) {
                    uintptr_t phys_old = get_physical_address(rollback);
                    if (phys_old) {
                        unmap_page(rollback);
                        pmm_free_page((void *)phys_old);
                    }
                }
                return (void *)-1;
            }
            map_page(addr, (uintptr_t)phys, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
        }
    } else if (increment < 0) {
        uintptr_t start = page_align_up(new_brk);
        uintptr_t end = page_align_up(old_brk);
        for (uintptr_t addr = start; addr < end; addr += PAGE_SIZE) {
            uintptr_t phys = get_physical_address(addr);
            if (phys) {
                unmap_page(addr);
                pmm_free_page((void *)phys);
            }
        }
    }

    user_heap_brk = new_brk;
    return (void *)old_brk;
}

int user_brk(void *addr) {
    if (!user_heap_initialized) return -1;
    uintptr_t target = (uintptr_t)addr;
    if (target == 0) {
        target = user_heap_brk;
    }
    if (target < user_heap_base || target > USER_HEAP_LIMIT) return -1;

    if (target > user_heap_brk) {
        uintptr_t diff = target - user_heap_brk;
        if (user_sbrk((intptr_t)diff) == (void *)-1) return -1;
    } else if (target < user_heap_brk) {
        if (user_sbrk((intptr_t)((intptr_t)target - (intptr_t)user_heap_brk)) == (void *)-1) return -1;
    }
    return 0;
}

typedef struct {
    uintptr_t virt;
    uintptr_t phys;
} mapped_page_t;

static inline uintptr_t page_align_down(uintptr_t addr) {
    return addr & ~(PAGE_SIZE - 1);
}

static inline uintptr_t page_align_up(uintptr_t addr) {
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

static void cleanup_mapped_pages(mapped_page_t *mapped_pages, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (mapped_pages[i].virt) {
            unmap_page(mapped_pages[i].virt);
            if (mapped_pages[i].phys) {
                pmm_free_page((void*)mapped_pages[i].phys);
            }
            mapped_pages[i].virt = 0;
            mapped_pages[i].phys = 0;
        }
    }
}

int elf_validate(const uint8_t *data, size_t size) {
    if (!data || size < sizeof(Elf32_Ehdr)) {
        printf("elf: too small\n");
        return -1;
    }

    const Elf32_Ehdr *ehdr = (const Elf32_Ehdr *)data;

    if (*((uint32_t *)ehdr->e_ident) != ELF_MAGIC) {
        printf("elf: bad magic\n");
        return -1;
    }
    if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) {
        printf("elf: not an executable or shared object\n");
        return -1;
    }
    if (ehdr->e_machine != EM_386) {
        printf("elf: not IA-32\n");
        return -1;
    }
    if (ehdr->e_version != 1) {
        printf("elf: unsupported ELF version\n");
        return -1;
    }
    if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0) {
        printf("elf: no program headers\n");
        return -1;
    }
    if (ehdr->e_phentsize < sizeof(Elf32_Phdr)) {
        printf("elf: invalid program header size\n");
        return -1;
    }
    if ((uint64_t)ehdr->e_phoff + (uint64_t)ehdr->e_phnum * ehdr->e_phentsize > size) {
        printf("elf: program header table out of bounds\n");
        return -1;
    }

    uintptr_t load_bias = (ehdr->e_type == ET_DYN) ? USER_BASE : 0;
    uint32_t entry = ehdr->e_entry;
    if (entry == 0 || entry + load_bias < USER_BASE || entry + load_bias > USER_TOP) {
        printf("elf: invalid entry point\n");
        return -1;
    }

    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        const Elf32_Phdr *phdr = (const Elf32_Phdr *)(data + ehdr->e_phoff + i * ehdr->e_phentsize);

        if (phdr->p_type != PT_LOAD)
            continue;

        if (phdr->p_memsz < phdr->p_filesz) {
            printf("elf: segment %u has memsz < filesz\n", i);
            return -1;
        }
        if ((phdr->p_flags & ~(PF_R | PF_W | PF_X)) != 0) {
            printf("elf: segment %u has unsupported flags\n", i);
            return -1;
        }
        if (phdr->p_offset > size) {
            printf("elf: segment %u file offset out of bounds\n", i);
            return -1;
        }
        if ((uint64_t)phdr->p_offset + phdr->p_filesz > size) {
            printf("elf: segment %u file bounds out of range\n", i);
            return -1;
        }

        uintptr_t virt_begin = phdr->p_vaddr + load_bias;
        uintptr_t virt_end = phdr->p_vaddr + load_bias + phdr->p_memsz;
        if (virt_end < virt_begin) {
            printf("elf: segment %u overflows virtual address\n", i);
            return -1;
        }
        if (virt_begin < USER_BASE) {
            printf("elf: segment %u maps below user base\n", i);
            return -1;
        }
        if (virt_end > USER_TOP + 1u) {
            printf("elf: segment %u exceeds user space\n", i);
            return -1;
        }

        if (phdr->p_align != 0 && (phdr->p_vaddr % phdr->p_align) != (phdr->p_offset % phdr->p_align)) {
            printf("elf: segment %u alignment mismatch\n", i);
            return -1;
        }
    }

    return 0;
}

uint32_t elf_load(const uint8_t *data, size_t size) {
    if (elf_validate(data, size) != 0)
        return 0;

    const Elf32_Ehdr *ehdr = (const Elf32_Ehdr *)data;
    uintptr_t load_bias = (ehdr->e_type == ET_DYN) ? USER_BASE : 0;
    mapped_page_t mapped_pages[ELF_MAX_MAPPED_PAGES];
    size_t mapped_count = 0;
    uintptr_t max_segment_end = USER_BASE;

    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        const Elf32_Phdr *phdr = (const Elf32_Phdr *)(
            data + ehdr->e_phoff + i * ehdr->e_phentsize
        );

        if (phdr->p_type != PT_LOAD)
            continue;

        uintptr_t segment_va = phdr->p_vaddr + load_bias;
        uintptr_t segment_start = page_align_down(segment_va);
        uintptr_t segment_end = page_align_up(segment_va + phdr->p_memsz);
        uintptr_t page_count = (segment_end - segment_start) / PAGE_SIZE;

        if (page_count == 0) {
            continue;
        }
        if (page_count > ELF_MAX_MAPPED_PAGES - mapped_count) {
            printf("elf: too many mapped pages\n");
            cleanup_mapped_pages(mapped_pages, mapped_count);
            return 0;
        }

        for (uintptr_t addr = segment_start; addr < segment_end; addr += PAGE_SIZE) {
            uintptr_t existing_phys = get_physical_address(addr);
            if (existing_phys != 0) {
                unmap_page(addr);
                pmm_free_page((void*)existing_phys);
            }

            void* phys = pmm_alloc_page();
            if (!phys) {
                printf("elf: out of memory for segment\n");
                cleanup_mapped_pages(mapped_pages, mapped_count);
                return 0;
            }

            uint32_t segment_flags = PAGE_PRESENT | PAGE_USER;
            if (phdr->p_flags & PF_W) {
                segment_flags |= PAGE_WRITE;
            }

            map_page(addr, (uintptr_t)phys, segment_flags);
            if (get_physical_address(addr) != (uintptr_t)phys) {
                pmm_free_page(phys);
                printf("elf: failed to map page\n");
                cleanup_mapped_pages(mapped_pages, mapped_count);
                return 0;
            }

            mapped_pages[mapped_count].virt = addr;
            mapped_pages[mapped_count].phys = (uintptr_t)phys;
            mapped_count++;

            memset((void*)addr, 0, PAGE_SIZE);
        }

        uint8_t *dest = (uint8_t *)segment_va;
        memcpy(dest, data + phdr->p_offset, phdr->p_filesz);
        if (phdr->p_memsz > phdr->p_filesz) {
            memset(dest + phdr->p_filesz, 0, phdr->p_memsz - phdr->p_filesz);
        }

        if (segment_end > max_segment_end) {
            max_segment_end = segment_end;
        }
    }

    user_heap_init(max_segment_end);
    return ehdr->e_entry + load_bias;
}

extern void jump_usermode(uint32_t entry, uint32_t user_stack);

int elf_exec(const uint8_t *data, size_t size) {
    uint32_t entry = elf_load(data, size);
    if (entry == 0) {
        printf("elf: load failed\n");
        return -1;
    }

    /* Allocate user stack */
    void* stack_phys = pmm_alloc_page();
    if (!stack_phys) {
        printf("elf: out of memory for user stack\n");
        return -1;
    }
    memset(stack_phys, 0, PAGE_SIZE);

    /* Map user stack */
    uintptr_t stack_virt = 0xBFFFFFFF - PAGE_SIZE + 1;  /* Top of user space */
    map_page(stack_virt, (uintptr_t)stack_phys, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    if (get_physical_address(stack_virt) != (uintptr_t)stack_phys) {
        pmm_free_page(stack_phys);
        printf("elf: failed to map user stack\n");
        return -1;
    }

    uintptr_t user_esp = (stack_virt + PAGE_SIZE) & ~0xFu;

    jump_usermode(entry, user_esp);
    pmm_free_page(stack_phys);
    unmap_page(stack_virt);
    return 0;
}