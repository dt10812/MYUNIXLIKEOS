#include "commands.h"
#include "vfs.h"

extern void terminal_write(const char*);
extern vnode_t* current_dir;
extern vnode_t* vfs_root;

vnode_t* vfs_lookup(const char* path);

int cmd_cd(int argc, char** argv) {
    if (argc < 2) {
        terminal_write("Usage: cd <path>\n");
        return -1;
    }

    vnode_t* target = vfs_lookup(argv[1]);
    if (!target || !(target->flags & VFS_DIRECTORY)) {
        terminal_write("cd: no such directory\n");
        return -1;
    }

    current_dir = target;
    return 0;
}
