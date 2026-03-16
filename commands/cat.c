#include "commands.h"
#include "vfs.h"

extern void terminal_write(const char*);
extern vnode_t* current_dir;
vnode_t* vfs_lookup(const char* path);

int cmd_cat(int argc, char** argv) {
    if (argc < 2) {
        terminal_write("Usage: cat <file>\n");
        return -1;
    }

    vnode_t* file = vfs_lookup(argv[1]);
    if (!file || !(file->flags & VFS_FILE)) {
        terminal_write("cat: file not found\n");
        return -1;
    }

    if (file->content) {
        terminal_write(file->content);
        terminal_write("\n");
    }
    return 0;
}
