#include "commands.h"
#include "vfs.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern void terminal_write(const char*);
extern vnode_t* current_dir;

int cmd_ls(int argc, char** argv) {
    vnode_t* dir = current_dir ? current_dir : vfs_root;
    if (!dir) {
        terminal_write("ls: no directory\n");
        return -1;
    }
    
    bool show_all = false;
    bool long_format = false;
    
    /* Parse flags */
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strchr(argv[i], 'a')) show_all = true;
            if (strchr(argv[i], 'l')) long_format = true;
        }
    }
    
    if (long_format) {
        printf("%-20s %s %10s\n", "NAME", "TYPE", "SIZE");
        printf("%-20s %s %10s\n", "----", "----", "----");
        
        for (uint32_t i = 0; i < dir->child_count; i++) {
            vnode_t* n = dir->children[i];
            
            if (*n->name != '.' || show_all) {
                const char* type_str = (n->flags & VFS_DIRECTORY) ? "DIR" : "FILE";
                uint32_t size = (n->flags & VFS_FILE) ? (n->content ?strlen(n->content) : 0) : 0;
                printf("%-20s %s %10u\n", n->name, type_str, size);
            }
        }
    } else {
        for (uint32_t i = 0; i < dir->child_count; i++) {
            vnode_t* n = dir->children[i];
            
            if (*n->name != '.' || show_all) {
                terminal_write((n->flags & VFS_DIRECTORY) ? "[D] " : "[F] ");
                terminal_write(n->name);
                terminal_write("\n");
            }
        }
    }
    return 0;
}
