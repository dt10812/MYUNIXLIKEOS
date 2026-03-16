#include "commands.h"
#include "vfs.h"

extern void terminal_write(const char*);
extern int k_touch(const char* name);

int cmd_touch(int argc, char** argv) {
    if (argc < 2) {
        terminal_write("Usage: touch <name>\n");
        return -1;
    }
    return k_touch(argv[1]);
}
