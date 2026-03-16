#include "commands.h"
#include "vfs.h"

extern void terminal_write(const char*);
extern vnode_t* current_dir;

int cmd_ls(int argc, char** argv) {
    (void)argc; (void)argv;
    vnode_t* dir = current_dir ? current_dir : vfs_root;
    if (!dir) {
        terminal_write("ls: no directory\n");
        return -1;
    }
    for (uint32_t i = 0; i < dir->child_count; i++) {
        vnode_t* n = dir->children[i];
        terminal_write((n->flags & VFS_DIRECTORY) ? "[D] " : "[F] ");
        terminal_write(n->name);
        terminal_write("\n");
    }
    return 0;
}
