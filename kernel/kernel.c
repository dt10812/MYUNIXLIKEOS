#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "vfs.h"
#include "pmm.h"
#include "io.h"
#include "commands.h"

// Multiboot2 header for GRUB
__attribute__((section(".multiboot2_header"), used, aligned(8)))
static const uint32_t multiboot2_header[] = {
    0xE85250D6,
    0x00000000,
    0x00000010,
    (uint32_t)(-(0xE85250D6 + 0 + 0x10)),
    0x00000000,
    0x00000008
};

// Simple keyboard input (PS/2 set 1 scancodes)
static const char scancode_table[0x80] = {
    0, 0x1B, '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n',
    'm', ',', '.', '/', 0, '*', 0, ' ', 0,
    [0x53] = 0x7F /* delete */
};


static volatile uint16_t* const VGA_BUFFER = (volatile uint16_t*)0xB8000;
static uint32_t terminal_row = 0;
static uint32_t terminal_col = 0;

vnode_t* vfs_root = NULL;
vnode_t* current_dir = NULL;
char fs_type_name[16] = "EXT4";

static void terminal_putc(char c) {
    if (c == '\n') {
        terminal_col = 0;
        terminal_row++;
        if (terminal_row >= 25) terminal_row = 24;
        return;
    }
    VGA_BUFFER[terminal_row * 80 + terminal_col] = (uint16_t)(' ' | 0x0F00);
    VGA_BUFFER[terminal_row * 80 + terminal_col] = ((uint16_t)0x0F << 8) | (uint8_t)c;
    terminal_col++;
    if (terminal_col >= 80) {
        terminal_col = 0;
        terminal_row++;
        if (terminal_row >= 25) terminal_row = 24;
    }
}

static void terminal_backspace(void) {
    if (terminal_col == 0) {
        if (terminal_row == 0) return;
        terminal_row--;
        terminal_col = 79;
    } else {
        terminal_col--;
    }
    VGA_BUFFER[terminal_row * 80 + terminal_col] = ((uint16_t)0x0F << 8) | ' ';
}

void terminal_write(const char* s) {
    while (*s) terminal_putc(*s++);
}

int compare_string(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (int)((unsigned char)*a) - (int)((unsigned char)*b);
}

void copy_string(char* dest, const char* src) {
    while ((*dest++ = *src++)) ;
}

static char keyboard_getchar(void) {
    while ((inb(0x64) & 1) == 0) ;
    uint8_t code = inb(0x60);
    if (code & 0x80) return 0;
    if (code < 0x80) return scancode_table[code];
    return 0;
}

static void read_line(char* buf, size_t size) {
    size_t idx = 0;
    while (true) {
        char c = keyboard_getchar();
        if (!c) continue;
        if (c == '\r' || c == '\n') {
            terminal_write("\n");
            buf[idx] = '\0';
            return;
        }
        if ((c == '\b' || c == 0x7F) && idx > 0) {
            idx--;
            terminal_backspace();
            continue;
        }
        if (idx + 1 < size) {
            buf[idx++] = c;
            terminal_putc(c);
        }
    }
}

static void split_args(char* input, char** argv, int* argc) {
    *argc = 0;
    bool in_token = false;
    while (*input) {
        if (*input == ' ' || *input == '\t') {
            *input = '\0';
            in_token = false;
        } else if (!in_token) {
            in_token = true;
            argv[(*argc)++] = input;
        }
        input++;
    }
}

#define MAX_PAGES 128
static uint8_t pmm_bitmap[MAX_PAGES / 8];
static uint8_t pmm_page_pool[MAX_PAGES][PAGE_SIZE];
static vnode_t vnodes_pool[64];
static uint32_t vnodes_pool_used = 0;

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
    uintptr_t cur = (uintptr_t)ptr;
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

static vnode_t* alloc_vnode(void) {
    if (vnodes_pool_used >= 64) return NULL;
    vnode_t* n = &vnodes_pool[vnodes_pool_used++];
    n->name[0] = '\0';
    n->flags = 0;
    n->parent = NULL;
    n->child_count = 0;
    n->content = NULL;
    for (int i = 0; i < 16; i++) n->children[i] = NULL;
    return n;
}

void vfs_init(void) {
    vnodes_pool_used = 0;
    vfs_root = alloc_vnode();
    if (!vfs_root) return;
    copy_string(vfs_root->name, "/");
    vfs_root->flags = VFS_DIRECTORY;
    vfs_root->parent = vfs_root;
    vfs_root->child_count = 0;
    vfs_root->content = NULL;
    current_dir = vfs_root;
}

vnode_t* vfs_lookup(const char* path) {
    if (!path || !*path) return NULL;
    if (compare_string(path, "/") == 0) return vfs_root;
    if (compare_string(path, ".") == 0) return current_dir;
    if (compare_string(path, "..") == 0) {
        return current_dir->parent ? current_dir->parent : current_dir;
    }

    if (*path == '/') path++;
    for (uint32_t i = 0; i < current_dir->child_count; i++) {
        vnode_t* c = current_dir->children[i];
        if (compare_string(c->name, path) == 0) return c;
    }
    return NULL;
}

int k_mkdir(const char* path) {
    if (!path || !*path) {
        terminal_write("mkdir: missing name\n");
        return -1;
    }
    if (!current_dir) return -1;
    if (current_dir->child_count >= 16) {
        terminal_write("mkdir: directory full\n");
        return -1;
    }
    if (vfs_lookup(path)) {
        terminal_write("mkdir: already exists\n");
        return -1;
    }
    vnode_t* node = alloc_vnode();
    if (!node) {
        terminal_write("mkdir: out of nodes\n");
        return -1;
    }
    copy_string(node->name, path);
    node->flags = VFS_DIRECTORY;
    node->parent = current_dir;
    node->child_count = 0;
    node->content = NULL;
    current_dir->children[current_dir->child_count++] = node;
    return 0;
}

int k_touch(const char* path) {
    if (!path || !*path) {
        terminal_write("touch: missing name\n");
        return -1;
    }
    if (!current_dir) return -1;
    if (current_dir->child_count >= 16) {
        terminal_write("touch: directory full\n");
        return -1;
    }
    if (vfs_lookup(path)) {
        terminal_write("touch: already exists\n");
        return -1;
    }
    vnode_t* node = alloc_vnode();
    if (!node) {
        terminal_write("touch: out of nodes\n");
        return -1;
    }
    copy_string(node->name, path);
    node->flags = VFS_FILE;
    node->parent = current_dir;
    node->child_count = 0;
    node->content = (char*)pmm_alloc_z(64);
    if (node->content) node->content[0] = '\0';
    current_dir->children[current_dir->child_count++] = node;
    return 0;
}

static int do_ls(int argc, char** argv) {
    return cmd_ls(argc, argv);
}

static int do_mkdir(int argc, char** argv) {
    return cmd_mkdir(argc, argv);
}

static int do_touch(int argc, char** argv) {
    return cmd_touch(argc, argv);
}

static int do_cd(int argc, char** argv) {
    return cmd_cd(argc, argv);
}

static int do_cat(int argc, char** argv) {
    return cmd_cat(argc, argv);
}

static int do_grep(int argc, char** argv) {
    return cmd_grep(argc, argv);
}

static int do_fs(int argc, char** argv) {
    return cmd_fs(argc, argv);
}

static int do_help(int argc, char** argv) {
    return cmd_help(argc, argv);
}

static size_t str_len(const char* s) {
    size_t l = 0;
    while (*s++) l++;
    return l;
}

int cmd_nano(int argc, char** argv) {
    if (argc < 2) {
        terminal_write("Usage: nano <filename>\n");
        return -1;
    }

    const char* path = argv[1];
    vnode_t* file = vfs_lookup(path);
    if (!file) {
        if (k_touch(path) != 0) {
            terminal_write("nano: cannot create file\n");
            return -1;
        }
        file = vfs_lookup(path);
    }
    if (!file || !(file->flags & VFS_FILE)) {
        terminal_write("nano: not a file\n");
        return -1;
    }

    char input[256];
    char buffer[4096];
    size_t len = 0;
    if (file->content) {
        char* p = file->content;
        while (*p && len < sizeof(buffer)-1) {
            buffer[len++] = *p++;
        }
    }
    buffer[len] = '\0';

    terminal_write("nano: edit mode (type :wq to save, :q! to cancel)\n");
    while (1) {
        terminal_write("nano> ");
        read_line(input, sizeof(input));

        if (compare_string(input, ":wq") == 0) {
            if (!file->content) {
                file->content = (char*)pmm_alloc_z(4096);
            }
            if (!file->content) {
                terminal_write("nano: out of memory\n");
                return -1;
            }
            size_t i;
            for (i = 0; i < len && i < 4095; i++) file->content[i] = buffer[i];
            file->content[i] = '\0';
            terminal_write("nano: saved\n");
            return 0;
        }
        if (compare_string(input, ":q!") == 0) {
            terminal_write("nano: abort\n");
            return 0;
        }

        size_t line_len = str_len(input);
        if (len + line_len + 1 < sizeof(buffer)) {
            for (size_t i = 0; i < line_len; i++) buffer[len + i] = input[i];
            len += line_len;
            buffer[len++] = '\n';
            buffer[len] = '\0';
        } else {
            terminal_write("nano: buffer full\n");
        }
    }
}

static int do_nano(int argc, char** argv) {
    return cmd_nano(argc, argv);
}

static void shell_loop(void) {
    char buf[256];
    while (1) {
        terminal_write("root@dev_null# ");
        read_line(buf, sizeof(buf));
        char* argv[16];
        int argc;
        split_args(buf, argv, &argc);
        if (argc == 0) continue;
        if (compare_string(argv[0], "ls") == 0) { do_ls(argc, argv); continue; }
        if (compare_string(argv[0], "mkdir") == 0) { do_mkdir(argc, argv); continue; }
        if (compare_string(argv[0], "touch") == 0) { do_touch(argc, argv); continue; }
        if (compare_string(argv[0], "cd") == 0) { do_cd(argc, argv); continue; }
        if (compare_string(argv[0], "cat") == 0) { do_cat(argc, argv); continue; }
        if (compare_string(argv[0], "grep") == 0) { do_grep(argc, argv); continue; }
        if (compare_string(argv[0], "fs") == 0) { do_fs(argc, argv); continue; }
        if (compare_string(argv[0], "nano") == 0) { do_nano(argc, argv); continue; }
        if (compare_string(argv[0], "help") == 0) { do_help(argc, argv); continue; }
        terminal_write("Unknown command\n");
    }
}

void kernel_main(void) {
    pmm_init(NULL);
    vfs_init();
    current_dir = vfs_root;

    terminal_write("Unix-32 Kernel Booted with ");
    terminal_write(fs_type_name);
    terminal_write("\n");
    terminal_write("Type help for commands\n");

    shell_loop();
}
