#include "commands.h"
#include "vfs.h"

extern void terminal_write(const char*);
extern int k_mkdir(const char* name);

int cmd_mkdir(int argc, char** argv) {
    if (argc < 2) {
        terminal_write("Usage: mkdir <name>\n");
        return -1;
    }
    return k_mkdir(argv[1]);
}
