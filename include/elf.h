#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>

/* ELF magic */
#define ELF_MAGIC       0x464C457F  /* 0x7F 'E' 'L' 'F' */

/* e_type */
#define ET_EXEC         2
#define ET_DYN          3

/* e_machine */
#define EM_386          3

/* p_type */
#define PT_LOAD         1

/* p_flags */
#define PF_X            0x1
#define PF_W            0x2
#define PF_R            0x4

/* sh_type */
#define SHT_REL         9

/* r_info macros */
#define ELF32_R_SYM(i)  ((i) >> 8)
#define ELF32_R_TYPE(i) ((i) & 0xFF)

/* Relocation types */
#define R_386_32        1

typedef struct {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;       /* virtual entry point */
    uint32_t e_phoff;       /* program header table offset */
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;       /* number of program headers */
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) Elf32_Ehdr;

typedef struct {
    uint32_t p_type;    /* PT_LOAD etc. */
    uint32_t p_offset;  /* offset in file */
    uint32_t p_vaddr;   /* target virtual address */
    uint32_t p_paddr;
    uint32_t p_filesz;  /* bytes to copy from file */
    uint32_t p_memsz;   /* bytes in memory (zero-fill memsz - filesz for BSS) */
    uint32_t p_flags;
    uint32_t p_align;
} __attribute__((packed)) Elf32_Phdr;

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
} __attribute__((packed)) Elf32_Shdr;

typedef struct {
    uint32_t r_offset;
    uint32_t r_info;
} __attribute__((packed)) Elf32_Rel;

/* Returns 0 if valid ELF32 executable for IA-32, -1 otherwise */
int elf_validate(const uint8_t *data, size_t size);

/* Loads all PT_LOAD segments and returns entry point, or 0 on failure */
uint32_t elf_load(const uint8_t *data, size_t size);

#endif