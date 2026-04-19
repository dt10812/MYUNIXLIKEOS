#include "commands.h"
#include "vfs.h"
#include "stdio.h"
#include "string.h"
#include "pmm.h"

extern void terminal_write(const char*);
extern vnode_t* current_dir;
extern vnode_t* vfs_lookup(const char* path);
extern int k_touch(const char* path);

int cmd_cp(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: cp <source> <destination>\n");
        return -1;
    }
    
    const char* src_name = argv[1];
    const char* dst_name = argv[2];
    
    /* Find source file */
    vnode_t* src = NULL;
    for (uint32_t i = 0; i < current_dir->child_count; i++) {
        if (strcmp(current_dir->children[i]->name, src_name) == 0) {
            src = current_dir->children[i];
            break;
        }
    }
    
    if (!src) {
        printf("cp: cannot stat '%s': No such file or directory\n", src_name);
        return -1;
    }
    
    if (src->flags & VFS_DIRECTORY) {
        printf("cp: '%s' is a directory\n", src_name);
        return -1;
    }
    
    /* Create destination file */
    k_touch(dst_name);
    
    /* Find destination file */
    vnode_t* dst = NULL;
    for (uint32_t i = 0; i < current_dir->child_count; i++) {
        if (strcmp(current_dir->children[i]->name, dst_name) == 0) {
            dst = current_dir->children[i];
            break;
        }
    }
    
    if (!dst) {
        printf("cp: failed to create destination file '%s'\n", dst_name);
        return -1;
    }
    
    /* Copy file content */
    if (src->content) {
        /* Allocate new memory for the content copy */
        dst->content = (char*)kmalloc(src->size + 1);
        if (dst->content) {
            memcpy(dst->content, src->content, src->size);
            dst->content[src->size] = '\0';  /* Null terminate */
            dst->size = src->size;
        } else {
            printf("cp: failed to allocate memory for copy\n");
            return -1;
        }
    }
    
    printf("'%s' -> '%s'\n", src_name, dst_name);
    return 0;
}
