#include "commands.h"
#include "stdio.h"

int cmd_whoami(int argc, char** argv) {
    (void)argc;
    (void)argv;
    printf("root\n");
    return 0;
}
