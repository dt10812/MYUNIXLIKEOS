#include "commands.h"
#include "vfs.h"
#include <stdbool.h>

extern void terminal_write(const char*);
extern vnode_t* current_dir;

int cmd_ls(int argc, char** argv) {
    (void)argc; (void)argv;
    vnode_t* dir = current_dir ? current_dir : vfs_root;
    if (!dir) {
        terminal_write("ls: no directory\n");
        return -1;
    }
    
    bool a = (argv[1][0] == '-' && argv[1][1] == 'a' ); // proper parser down the line
    for (uint32_t i = 0; i < dir->child_count; i++) {
        vnode_t* n = dir->children[i];
        
        if(*n->name != '.' || a){
            terminal_write((n->flags & VFS_DIRECTORY) ? "[D] " : "[F] ");
            terminal_write(n->name);
            terminal_write("\n");
        }
    }
    return 0;
}
