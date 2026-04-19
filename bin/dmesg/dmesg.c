#include "commands.h"
#include "stdio.h"

extern const char* get_kernel_log(void);

int cmd_dmesg(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    const char *log = get_kernel_log();
    if (log && *log) {
        printf("%s", log);
    } else {
        printf("dmesg: kernel log buffer is empty\n");
    }

    return 0;
}