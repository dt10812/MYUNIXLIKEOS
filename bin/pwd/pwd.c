#include "commands.h"
#include "stdio.h"

extern void terminal_write(const char*);

int cmd_pwd(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    /* Print the current working directory path */
    printf("/\n");
    return 0;
}
