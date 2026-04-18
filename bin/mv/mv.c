#include "commands.h"
#include "vfs.h"
#include "stdio.h"
#include "string.h"

extern void terminal_write(const char*);
extern vnode_t* current_dir;

int cmd_mv(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: mv <source> <destination>\n");
        return -1;
    }
    
    const char* src_name = argv[1];
    const char* dst_name = argv[2];
    
    /* Find source file */
    vnode_t* src = NULL;
    uint32_t src_idx = 0xFFFFFFFF;
    for (uint32_t i = 0; i < current_dir->child_count; i++) {
        if (strcmp(current_dir->children[i]->name, src_name) == 0) {
            src = current_dir->children[i];
            src_idx = i;
            break;
        }
    }
    
    if (!src) {
        printf("mv: cannot stat '%s': No such file or directory\n", src_name);
        return -1;
    }
    
    /* Check if destination already exists */
    for (uint32_t i = 0; i < current_dir->child_count; i++) {
        if (strcmp(current_dir->children[i]->name, dst_name) == 0) {
            printf("mv: cannot move '%s' to '%s': File exists\n", src_name, dst_name);
            return -1;
        }
    }
    
    /* Rename the file */
    strcpy(src->name, dst_name);
    
    printf("'%s' -> '%s'\n", src_name, dst_name);
    return 0;
}
