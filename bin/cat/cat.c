/* TODO: move this out of kernel and into a userspace prog */
#include "commands.h"
#include "vfs.h"
#include "stdio.h"

extern void terminal_write(const char*);
extern vnode_t* current_dir;
vnode_t* vfs_lookup(const char* path);

int cmd_cat(int argc, char** argv) {
    if (argc < 2) {
        // POSIX complaint cat should read from stdin if file is not present
        printf("Usage: cat <file>\n");
        return -1;
    }

    vnode_t* file;

    char **pptr = argv; // path ptr
    for(++pptr; --argc; pptr++){
        file = vfs_lookup(*pptr);
        if (!file || !(file->flags & VFS_FILE)) {
            printf("cat: %s: No such file or directory\n", *pptr);
            return -1;
        }
        if (file->content) {
            printf("%s\n", file->content);
        }
    }
    return 0;
}