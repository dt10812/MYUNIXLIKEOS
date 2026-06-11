#include "commands.h"
#include "string.h"
#include "stdbool.h"

extern void terminal_write(const char*);

int cmd_echo(int argc, char** argv) {
    if (argc < 2) {
        terminal_write("\n");
        return 0;
    }

    int start = 1;
    bool newline = true;
    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        newline = false;
        start = 2;
    }

    for (int i = start; i < argc; i++) {
        terminal_write(argv[i]);
        if (i < argc - 1) {
            terminal_write(" ");
        }
    }

    if (newline) {
        terminal_write("\n");
    }
    return 0;
}
