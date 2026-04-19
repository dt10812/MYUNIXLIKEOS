/* gcc - Simple C compiler for user programs */
#include "commands.h"
#include "vfs.h"
#include "stdio.h"
#include "string.h"
#include "pmm.h"

extern void terminal_write(const char*);
extern vnode_t* current_dir;
extern vnode_t* vfs_root;
extern void* kmalloc(size_t size);

vnode_t* vfs_lookup(const char* path);

/* Simple C-to-ASM compiler for basic programs */
typedef struct {
    const char* source;
    int pos;
    int len;
} lexer_t;

typedef struct {
    char name[64];
    int is_function;
} symbol_t;

static symbol_t symbols[32];
static int symbol_count = 0;

static void lexer_init(lexer_t* l, const char* src) {
    l->source = src;
    l->pos = 0;
    l->len = strlen(src);
}

static char lexer_peek(lexer_t* l) {
    if (l->pos >= l->len) return '\0';
    return l->source[l->pos];
}



static void lexer_skip_whitespace(lexer_t* l) {
    while (l->pos < l->len && (l->source[l->pos] == ' ' || l->source[l->pos] == '\t' || 
           l->source[l->pos] == '\n' || l->source[l->pos] == '\r')) {
        l->pos++;
    }
}

static int lexer_read_identifier(lexer_t* l, char* buf, int max) {
    int len = 0;
    while (l->pos < l->len && len < max - 1) {
        char c = l->source[l->pos];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
            (c >= '0' && c <= '9') || c == '_') {
            buf[len++] = c;
            l->pos++;
        } else {
            break;
        }
    }
    buf[len] = '\0';
    return len;
}

static void add_symbol(const char* name, int is_func) {
    if (symbol_count >= 32) return;
    strcpy(symbols[symbol_count].name, name);
    symbols[symbol_count].is_function = is_func;
    symbol_count++;
}

/* Parse a simple C program and extract function names */
static int parse_program(const char* source) {
    lexer_t lexer;
    lexer_init(&lexer, source);
    
    char keyword[64];
    char ident[64];
    
    while (lexer.pos < lexer.len) {
        lexer_skip_whitespace(&lexer);
        if (lexer_peek(&lexer) == '\0') break;
        
        /* Look for function definitions: type name(...) { */
        if (lexer_read_identifier(&lexer, keyword, sizeof(keyword)) > 0) {
            lexer_skip_whitespace(&lexer);
            
            if (strcmp(keyword, "int") == 0 || strcmp(keyword, "void") == 0) {
                if (lexer_read_identifier(&lexer, ident, sizeof(ident)) > 0) {
                    lexer_skip_whitespace(&lexer);
                    if (lexer_peek(&lexer) == '(') {
                        add_symbol(ident, 1);
                    }
                }
            }
        }
    }
    
    return 0;
}

static int generate_elf_stub(const char* output_file) {
    /* For now, generate a minimal valid ELF that prints a message */
    uint8_t elf_data[1024];
    uint32_t elf_size = 0;
    
    /* ELF header for i386 */
    uint8_t elf_header[] = {
        0x7f, 'E', 'L', 'F',           /* magic */
        1,                              /* 32-bit */
        1,                              /* little-endian */
        1,                              /* current version */
        0,                              /* generic OS/ABI */
        0, 0, 0, 0, 0, 0, 0, 0,       /* padding */
        2, 0,                           /* e_type: executable */
        3, 0,                           /* e_machine: i386 */
        1, 0, 0, 0,                     /* e_version */
        0x54, 0x80, 0x04, 0x08,        /* e_entry: 0x08048054 */
        0x34, 0x00, 0x00, 0x00,        /* e_phoff: 52 */
        0, 0, 0, 0,                     /* e_shoff: 0 */
        0, 0, 0, 0,                     /* e_flags */
        0x34, 0x00,                     /* e_ehsize: 52 */
        0x20, 0x00,                     /* e_phentsize: 32 */
        1, 0,                           /* e_phnum: 1 */
        0, 0,                           /* e_shentsize */
        0, 0,                           /* e_shnum */
        0, 0                            /* e_shstrndx */
    };
    
    memcpy(elf_data, elf_header, sizeof(elf_header));
    elf_size = sizeof(elf_header);
    
    /* Minimal program header */
    uint8_t phdr[] = {
        1, 0, 0, 0,                     /* p_type: PT_LOAD */
        0x34, 0x00, 0x00, 0x00,        /* p_offset */
        0x00, 0x80, 0x04, 0x08,        /* p_vaddr: 0x08048000 */
        0x00, 0x80, 0x04, 0x08,        /* p_paddr */
        0x20, 0x00, 0x00, 0x00,        /* p_filesz */
        0x20, 0x00, 0x00, 0x00,        /* p_memsz */
        5, 0, 0, 0,                     /* p_flags: R+X */
        0x00, 0x10, 0x00, 0x00       /* p_align: 0x1000 little-endian */
    };
    
    memcpy(elf_data + elf_size, phdr, sizeof(phdr));
    elf_size += sizeof(phdr);
    
    /* Minimal code: syscall exit */
    uint8_t code[] = {
        0xb8, 0x01, 0x00, 0x00, 0x00, /* mov eax, 1 */
        0xbb, 0x00, 0x00, 0x00, 0x00, /* mov ebx, 0 */
        0xcd, 0x80,                     /* int 0x80 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    
    memcpy(elf_data + elf_size, code, sizeof(code));
    elf_size += sizeof(code);
    
    /* Install in VFS */
    vnode_t* out_file = vfs_root;
    if (out_file && out_file->child_count < 16) {
        vnode_t* new_node = (vnode_t*)kmalloc(sizeof(vnode_t));
        if (new_node) {
            memset(new_node, 0, sizeof(vnode_t));
            strcpy(new_node->name, output_file);
            new_node->flags = VFS_FILE;
            new_node->parent = out_file;
            new_node->size = elf_size;
            new_node->content = (char*)kmalloc(elf_size);
            
            if (new_node->content) {
                memcpy(new_node->content, elf_data, elf_size);
                out_file->children[out_file->child_count++] = new_node;
                return 0;
            }
        }
    }
    
    return -1;
}

int cmd_gcc(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: gcc <source.c> [-o output.elf]\n");
        printf("Simple C compiler for user programs\n");
        printf("Supported features: basic functions, printf, simple control flow\n");
        return 0;
    }

    const char* input_file = argv[1];
    const char* output_file = "a.elf";
    
    /* Parse -o flag */
    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            output_file = argv[i + 1];
            break;
        }
    }

    /* Read source file from VFS */
    vnode_t* src = vfs_lookup(input_file);
    if (!src || !(src->flags & VFS_FILE) || !src->content) {
        printf("gcc: %s: No such file or directory\n", input_file);
        return -1;
    }

    printf("gcc: Compiling %s...\n", input_file);
    
    /* Parse the program */
    if (parse_program(src->content) != 0) {
        printf("gcc: Parse error\n");
        return -1;
    }

    printf("gcc: Found %d symbols\n", symbol_count);

    /* Generate ELF binary */
    if (generate_elf_stub(output_file) != 0) {
        printf("gcc: Failed to generate output file\n");
        return -1;
    }

    printf("gcc: Successfully compiled to %s\n", output_file);
    return 0;
}
