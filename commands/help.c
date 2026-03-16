#include "commands.h"

extern void terminal_write(const char*);

int cmd_help(int argc, char** argv) {
    (void)argc; (void)argv;
    terminal_write("Available commands:\n");
    terminal_write("  ls mkdir touch cd cat grep fs help\n");
    return 0;
}
