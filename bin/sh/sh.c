/* TODO:
 * - Proper sh parser.
 * - move out of kernel space into a user prog
 */
#include "stdio.h"
#include "commands.h"
#include "string.h"
#include "limits.h"
#include <stddef.h>
#include "vfs.h"
#include "unistd.h"

void read_line(char *buf, size_t size);
extern void cmd_clear(void);

static void split_args(char *input, char **argv, int *argc) {
    *argc = 0;
    char in_token = 0;

    while (*input) {
        if (*input == ' ' || *input == '\t') {
            *input = '\0';
            in_token = 0;
        } else if (!in_token) {
            in_token = 1;
            argv[(*argc)++] = input;
            if (*argc >= 16)
                break;
        }
        input++;
    }
}

void sh(void) {
    char buf[ARG_MAX];
    char *argv[16];
    int argc;

    for (;;) {
        printf("root@dev_null# ");

        memset(buf,  0, sizeof(buf));
        memset(argv, 0, sizeof(argv));
        argc = 0;

        read_line(buf, ARG_MAX);
        split_args(buf, argv, &argc);

        if (argc == 0) continue;

        if (strcmp(argv[0], "ls")    == 0) { cmd_ls(argc, argv);    continue; }
        if (strcmp(argv[0], "mkdir") == 0) { cmd_mkdir(argc, argv); continue; }
        if (strcmp(argv[0], "touch") == 0) { cmd_touch(argc, argv); continue; }
        if (strcmp(argv[0], "cd")    == 0) { cmd_cd(argc, argv);    continue; }
        if (strcmp(argv[0], "cat")   == 0) { cmd_cat(argc, argv);   continue; }
        if (strcmp(argv[0], "grep")  == 0) { cmd_grep(argc, argv);  continue; }
        if (strcmp(argv[0], "help")  == 0) { cmd_help(argc, argv);  continue; }
        if (strcmp(argv[0], "clear") == 0) { cmd_clear();           continue; }
        if (strcmp(argv[0], "echo")  == 0) { cmd_echo(argc, argv);  continue; }
        if (strcmp(argv[0], "color") == 0) { cmd_color(argc, argv); continue; }
        if (strcmp(argv[0], "memory") == 0) { cmd_memory(argc, argv); continue; }
        if (strcmp(argv[0], "free") == 0) { cmd_free(argc, argv); continue; }
        if (strcmp(argv[0], "top") == 0) { cmd_top(argc, argv); continue; }
        if (strcmp(argv[0], "vmstat") == 0) { cmd_vmstat(argc, argv); continue; }
        if (strcmp(argv[0], "snake") == 0) { cmd_snake(argc, argv); continue; }
        if (strcmp(argv[0], "vi")    == 0) { cmd_vi(argc, argv);    continue; }
        if (strcmp(argv[0], "nano")  == 0) { cmd_nano(argc, argv);  continue; }
        if (strcmp(argv[0], "pwd")   == 0) { cmd_pwd(argc, argv);   continue; }
        if (strcmp(argv[0], "date")  == 0) { cmd_date(argc, argv);  continue; }
        if (strcmp(argv[0], "rm")    == 0) { cmd_rm(argc, argv);    continue; }
        if (strcmp(argv[0], "cp")    == 0) { cmd_cp(argc, argv);    continue; }
        if (strcmp(argv[0], "mv")    == 0) { cmd_mv(argc, argv);    continue; }
        if (strcmp(argv[0], "shutdown") == 0) { cmd_shutdown(argc, argv); continue; }
        if (strcmp(argv[0], "reboot")   == 0) { cmd_reboot(argc, argv);   continue; }
        if (strcmp(argv[0], "calc")    == 0) { cmd_calc(argc, argv);    continue; }
        if (strcmp(argv[0], "man")     == 0) { cmd_man(argc, argv);     continue; }
        if (strcmp(argv[0], "wc")      == 0) { cmd_wc(argc, argv);      continue; }
        if (strcmp(argv[0], "head")    == 0) { cmd_head(argc, argv);    continue; }
        if (strcmp(argv[0], "tail")    == 0) { cmd_tail(argc, argv);    continue; }
        if (strcmp(argv[0], "sort")    == 0) { cmd_sort(argc, argv);    continue; }
        if (strcmp(argv[0], "uniq")    == 0) { cmd_uniq(argc, argv);    continue; }
        if (strcmp(argv[0], "whoami")  == 0) { cmd_whoami(argc, argv);  continue; }
        if (strcmp(argv[0], "password") == 0) { cmd_password(argc, argv); continue; }

        if (strcmp(argv[0], "exec")  == 0) {
            if (argc < 2) { printf("exec: missing program name\n"); continue; }
            argv[argc] = NULL;
            execl(argv[1], argv[1], argv[2], argv[3], argv[4],
                  argv[5], argv[6], argv[7], argv[8], argv[9],
                  argv[10], argv[11], argv[12], argv[13], argv[14], NULL);
            continue;
        }

        printf("sh: command not found: %s\n", argv[0]);
    }
}