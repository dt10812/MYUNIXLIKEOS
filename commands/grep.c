#include <stddef.h>
#include "commands.h"
#include "vfs.h"

extern void terminal_write(const char*);
extern vnode_t* current_dir;
extern vnode_t* vfs_root;

char* kkstrstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        const char* h = haystack;
        const char* n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char*)haystack;
    }
    return NULL;
}

int cmd_grep(int argc, char** argv) {
    if (argc < 2) {
        terminal_write("Usage: grep <pattern>\n");
        return -1;
    }

    vnode_t* dir = current_dir ? current_dir : vfs_root;
    if (!dir) return -1;

    for (uint32_t i = 0; i < dir->child_count; i++) {
        vnode_t* c = dir->children[i];
        if ((c->flags & VFS_FILE) && c->content && kkstrstr(c->content, argv[1])) {
            terminal_write(c->name);
            terminal_write("\n");
        }
    }

    return 0;
}
