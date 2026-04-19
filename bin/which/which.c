#include "commands.h"
#include "stdio.h"
#include "string.h"
#include "vfs.h"

extern vnode_t* current_dir;
vnode_t* vfs_lookup(const char* path);

int cmd_which(int argc, char *argv[]) {
    if (argc < 2) {
        printf("which: missing command name\n");
        printf("Usage: which <command>\n");
        return 1;
    }

    const char *cmd = argv[1];

    // First check if it's a built-in command by looking in /bin
    char path[256];
    sprintf(path, "/bin/%s/%s.c", cmd, cmd);

    vnode_t *node = vfs_lookup(path);
    if (node && (node->flags & VFS_FILE)) {
        printf("/bin/%s/%s\n", cmd, cmd);
        return 0;
    }

    // Check if it's in the root bin directory
    sprintf(path, "/bin/%s", cmd);
    node = vfs_lookup(path);
    if (node && (node->flags & VFS_FILE)) {
        printf("/bin/%s\n", cmd);
        return 0;
    }

    // Check if it's in /usr/bin (though we don't have this yet)
    sprintf(path, "/usr/bin/%s", cmd);
    node = vfs_lookup(path);
    if (node && (node->flags & VFS_FILE)) {
        printf("/usr/bin/%s\n", cmd);
        return 0;
    }

    // Check current directory
    node = vfs_lookup(cmd);
    if (node && (node->flags & VFS_FILE)) {
        printf("./%s\n", cmd);
        return 0;
    }

    printf("which: %s: command not found\n", cmd);
    return 1;
}