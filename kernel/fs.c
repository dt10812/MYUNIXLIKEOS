#include "commands.h"
#include "vfs.h"
#include "string.h"
#include "pmm.h"
#include "stdio.h"
#include "elf.h"

vnode_t* current_dir = NULL;
vnode_t* vfs_root    = NULL;

static uint32_t vnodes_pool_used = 0;
static vnode_t  vnodes_pool[64];

static vnode_t* alloc_vnode(void) {
    if (vnodes_pool_used >= 64) return NULL;
    vnode_t* n = &vnodes_pool[vnodes_pool_used++];
    n->name[0]     = '\0';
    n->flags       = 0;
    n->parent      = NULL;
    n->child_count = 0;
    n->content     = NULL;
    for (int i = 0; i < 16; i++) n->children[i] = NULL;
    return n;
}

void fs_init(void) {
    vnodes_pool_used = 0;
    vfs_root = alloc_vnode();
    if (!vfs_root) return;
    strcpy(vfs_root->name, "/");
    vfs_root->flags       = VFS_DIRECTORY;
    vfs_root->parent      = NULL;
    vfs_root->child_count = 0;
    vfs_root->content     = NULL;
    current_dir = vfs_root;
}

vnode_t* vfs_lookup(const char* path) {
    if (!path || !*path) return NULL;
    if (strcmp(path, "/")  == 0) return vfs_root;
    if (strcmp(path, ".")  == 0) return current_dir;
    if (strcmp(path, "..") == 0)
        return current_dir->parent ? current_dir->parent : current_dir;

    const char* name = (*path == '/') ? path + 1 : path;
    for (uint32_t i = 0; i < current_dir->child_count; i++) {
        vnode_t* c = current_dir->children[i];
        if (strcmp(c->name, name) == 0) return c;
    }
    return NULL;
}

int k_mkdir(const char* path) {
    if (!path || !*path) { printf("mkdir: missing name\n"); return -1; }
    if (!current_dir) return -1;
    if (current_dir->child_count >= 16) { printf("mkdir: directory full\n"); return -1; }
    if (vfs_lookup(path)) { printf("mkdir: already exists\n"); return -1; }
    vnode_t* node = alloc_vnode();
    if (!node) { printf("mkdir: out of nodes\n"); return -1; }
    strcpy(node->name, path);
    node->flags       = VFS_DIRECTORY;
    node->parent      = current_dir;
    node->child_count = 0;
    node->content     = NULL;
    current_dir->children[current_dir->child_count++] = node;
    return 0;
}

int k_touch(const char* path) {
    if (!path || !*path) { printf("touch: missing name\n"); return -1; }
    if (!current_dir) return -1;
    if (current_dir->child_count >= 16) { printf("touch: directory full\n"); return -1; }
    if (vfs_lookup(path)) { printf("touch: already exists\n"); return -1; }
    vnode_t* node = alloc_vnode();
    if (!node) { printf("touch: out of nodes\n"); return -1; }
    strcpy(node->name, path);
    node->flags       = VFS_FILE;
    node->parent      = current_dir;
    node->child_count = 0;
    node->content     = (char*)pmm_alloc_z(64);
    if (node->content) node->content[0] = '\0';
    current_dir->children[current_dir->child_count++] = node;
    return 0;
}

int k_install(const char* name, const uint8_t* data, uint32_t size) {
    if (!name || !data || !size) return -1;
    if (vfs_root->child_count >= 16) return -1;
    vnode_t* node = alloc_vnode();
    if (!node) return -1;
    strcpy(node->name, name);
    node->flags   = VFS_FILE;
    node->parent  = vfs_root;
    node->content = (char*)data;
    node->size    = size;
    vfs_root->children[vfs_root->child_count++] = node;
    return 0;
}

int k_unlink(const char* path) {
    if (!path || !*path) return -1;
    vnode_t* node = vfs_lookup(path);
    if (!node || !(node->flags & VFS_FILE)) return -1;
    vnode_t* parent = node->parent;
    if (!parent) return -1;

    int found = -1;
    for (uint32_t i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == node) {
            found = (int)i;
            break;
        }
    }
    if (found < 0) return -1;

    for (uint32_t i = found; i + 1 < parent->child_count; i++) {
        parent->children[i] = parent->children[i + 1];
    }
    parent->child_count--;

    if (node->content) {
        node->content = NULL;
    }

    node->flags = 0;
    node->parent = NULL;
    node->child_count = 0;
    node->size = 0;
    return 0;
}

/* ── ELF execution ────────────────────────────────────────────────────────── */

#define USER_STACK_SIZE PAGE_SIZE

extern void jump_usermode(uint32_t entry, uint32_t user_stack);

/* k_exec(path, argv)
 *
 * argv is a NULL-terminated array of strings (may be NULL for no args).
 *
 * Stack layout seen by crt0 on entry to ring 3 (grows down, so built
 * from bottom up here):
 *
 *   user_esp   [ argc          ] top of built frame
 *              [ argv[0] ptr   ]
 *              [ argv[1] ptr   ]
 *              [ ...           ]
 *              [ NULL          ] end of argv ptr array
 *              [ "arg0\0"      ] string data
 *              [ "arg1\0"      ]
 *              [ ...           ]
 *
 * crt0 reads argc from [esp] and argv ptr from [esp+4].
 */
int k_exec(const char *path, const char **argv) {
    if (!path || !*path) { printf("exec: missing path\n"); return -1; }

    vnode_t *node = vfs_lookup(path);
    if (!node) {
        printf("exec: not found: %s\n", path);
        return -1;
    }
    if (node->flags != VFS_FILE) {
        printf("exec: not a file\n");
        return -1; }
    if (!node->content) {
        printf("exec: empty file\n");
        return -1;
    }

    uint32_t entry = elf_load((const uint8_t*)node->content, node->size);
    if (!entry) {
        printf("exec: ELF load failed\n");
        return -1;
    }

    /* allocate one page for the user stack and map it into user space */
    uintptr_t stack_page = (uintptr_t)pmm_alloc_page();
    if (!stack_page) {
        printf("exec: out of memory\n");
        return -1;
    }
    memset((void*)stack_page, 0, PAGE_SIZE);

    uintptr_t user_stack_virt = 0xBFFFFFFF - PAGE_SIZE + 1;
    map_page(user_stack_virt, stack_page, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);

    /* Build argc/argv on the user stack.
     * Use a pointer that walks DOWN from the top of the page.
     * We write string data first (at the high end), then the
     * pointer array + argc below that.
     */
    /* align down first so the frame we build is naturally aligned */
    uint8_t *stk_top = (uint8_t *)(user_stack_virt + USER_STACK_SIZE);
    uint8_t *str_ptr = (uint8_t *)((uint32_t)stk_top & ~0xFU);

    /* count args and copy string data onto stack */
    int argc = 0;
    uint32_t arg_ptrs[16];

    if (argv) {
        while (argv[argc]) {
            if (argc >= 15) {
                printf("exec: too many arguments\n");
                return -1;
            }
            size_t len = strlen(argv[argc]) + 1;
            if (str_ptr - len < (uint8_t*)user_stack_virt) {
                printf("exec: user stack overflow\n");
                return -1;
            }
            str_ptr -= len;
            memcpy(str_ptr, argv[argc], len);
            arg_ptrs[argc] = (uint32_t)str_ptr;
            argc++;
        }
    }

    /* Now build the pointer array + argc below the string data.
     * Layout (each cell is 4 bytes, stack grows down):
     *   argc
     *   argv[0]
     *   argv[1]
     *   ...
     *   argv[argc-1]
     *   NULL
     */
    uint32_t *frame = (uint32_t *)str_ptr;
    frame--;  *frame = 0;                          /* NULL terminator */
    for (int i = argc - 1; i >= 0; i--) {
        frame--;
        *frame = arg_ptrs[i];
    }
    frame--;  *frame = (uint32_t)argc;             /* argc */

    uint32_t user_esp = (uint32_t)frame;

    jump_usermode(entry, user_esp);

    return 0;
}

/* ── Enhanced VFS Operations ─────────────────────────────────────────────── */

/* vfs_stat - Get file/directory information */
int vfs_stat(const char* path, vnode_t** out) {
    if (!path || !*path || !out) return -1;
    
    vnode_t* node = vfs_lookup(path);
    if (!node) return -1;
    
    *out = node;
    return 0;
}

/* vfs_isdir - Check if node is a directory */
int vfs_isdir(const char* path) {
    vnode_t* node = vfs_lookup(path);
    if (!node) return 0;
    return (node->flags & VFS_DIRECTORY) != 0;
}

/* vfs_isfile - Check if node is a regular file */
int vfs_isfile(const char* path) {
    vnode_t* node = vfs_lookup(path);
    if (!node) return 0;
    return (node->flags & VFS_FILE) != 0;
}

/* vfs_filesize - Get file size in bytes */
uint32_t vfs_filesize(const char* path) {
    vnode_t* node = vfs_lookup(path);
    if (!node) return 0;
    if (!(node->flags & VFS_FILE)) return 0;
    
    if (node->content) {
        return strlen(node->content);
    }
    return 0;
}

/* vfs_readdir - List contents of a directory */
int vfs_readdir(const char* path, vnode_t*** entries, uint32_t* count) {
    if (!path || !*path || !entries || !count) return -1;
    
    vnode_t* dir = vfs_lookup(path);
    if (!dir || !(dir->flags & VFS_DIRECTORY)) return -1;
    
    if (dir->child_count == 0) {
        *count = 0;
        *entries = NULL;
        return 0;
    }
    
    *entries = dir->children;
    *count = dir->child_count;
    return 0;
}

/* vfs_read_file - Read entire file content */
const char* vfs_read_file(const char* path, uint32_t* size_out) {
    vnode_t* node = vfs_lookup(path);
    if (!node || !(node->flags & VFS_FILE)) {
        if (size_out) *size_out = 0;
        return NULL;
    }
    
    if (size_out) {
        if (node->content) {
            *size_out = strlen(node->content);
        } else {
            *size_out = 0;
        }
    }
    
    return node->content;
}

/* vfs_write_file - Write content to file */
int vfs_write_file(const char* path, const char* content, uint32_t size) {
    vnode_t* node = vfs_lookup(path);
    if (!node || !(node->flags & VFS_FILE)) return -1;
    
    if (!content || !size) {
        if (node->content) node->content[0] = '\0';
        node->size = 0;
        return 0;
    }
    
    /* For now, we have fixed 64-byte buffers. Copy what fits. */
    if (!node->content) {
        node->content = (char*)pmm_alloc_z(64);
        if (!node->content) return -1;
    }
    
    uint32_t copy_size = size < 63 ? size : 63;
    memcpy(node->content, content, copy_size);
    node->content[copy_size] = '\0';
    node->size = copy_size;
    
    return 0;
}

/* vfs_append_file - Append content to file */
int vfs_append_file(const char* path, const char* content, uint32_t size) {
    vnode_t* node = vfs_lookup(path);
    if (!node || !(node->flags & VFS_FILE)) return -1;
    
    if (!node->content) {
        return vfs_write_file(path, content, size);
    }
    
    uint32_t current_size = strlen(node->content);
    uint32_t available = 64 - current_size - 1;
    
    if (available <= 0) return -1;  /* File full */
    
    uint32_t append_size = size < available ? size : available;
    memcpy(node->content + current_size, content, append_size);
    node->content[current_size + append_size] = '\0';
    node->size = current_size + append_size;
    
    return 0;
}

/* vfs_find_in_path - Find a file within a directory */
vnode_t* vfs_find_in_path(vnode_t* dir, const char* name) {
    if (!dir || !name || !(dir->flags & VFS_DIRECTORY)) return NULL;
    
    for (uint32_t i = 0; i < dir->child_count; i++) {
        if (strcmp(dir->children[i]->name, name) == 0) {
            return dir->children[i];
        }
    }
    return NULL;
}

/* vfs_child_count - Get number of children in directory */
uint32_t vfs_child_count(const char* path) {
    vnode_t* node = vfs_lookup(path);
    if (!node || !(node->flags & VFS_DIRECTORY)) return 0;
    return node->child_count;
}

/* vfs_get_child - Get child vnode by index */
vnode_t* vfs_get_child(const char* path, uint32_t index) {
    vnode_t* node = vfs_lookup(path);
    if (!node || !(node->flags & VFS_DIRECTORY)) return NULL;
    if (index >= node->child_count) return NULL;
    return node->children[index];
}