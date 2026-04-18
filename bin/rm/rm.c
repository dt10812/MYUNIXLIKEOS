#include "commands.h"
#include "vfs.h"
#include "stdio.h"
#include "string.h"

extern void terminal_write(const char*);
extern vnode_t* current_dir;
extern vnode_t* vfs_lookup(const char* path);

int cmd_rm(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: rm <file>\n");
        return -1;
    }
    
    const char* filename = argv[1];
    
    /* Find the file in current directory */
    uint32_t idx = 0xFFFFFFFF;
    for (uint32_t i = 0; i < current_dir->child_count; i++) {
        if (strcmp(current_dir->children[i]->name, filename) == 0) {
            idx = i;
            break;
        }
    }
    
    if (idx == 0xFFFFFFFF) {
        printf("rm: cannot remove '%s': No such file or directory\n", filename);
        return -1;
    }
    
    vnode_t* file = current_dir->children[idx];
    
    /* Prevent removal of directories */
    if (file->flags & VFS_DIRECTORY) {
        printf("rm: cannot remove '%s': Is a directory\n", filename);
        return -1;
    }
    
    /* Remove from children array */
    for (uint32_t i = idx; i < current_dir->child_count - 1; i++) {
        current_dir->children[i] = current_dir->children[i + 1];
    }
    current_dir->children[current_dir->child_count - 1] = NULL;
    current_dir->child_count--;
    
    printf("removed '%s'\n", filename);
    return 0;
}
