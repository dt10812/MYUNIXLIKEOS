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
#include "io.h"

#define MAX_ALIASES 32
#define MAX_ALIAS_NAME 32
#define MAX_ALIAS_VALUE 128

static struct {
    char name[MAX_ALIAS_NAME];
    char value[MAX_ALIAS_VALUE];
} aliases[MAX_ALIASES];
static int alias_count = 0;

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

static int find_alias(const char *name) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static void add_alias(const char *name, const char *value) {
    if (alias_count >= MAX_ALIASES) {
        printf("alias: too many aliases\n");
        return;
    }

    int existing = find_alias(name);
    if (existing >= 0) {
        // Update existing alias
        strcpy(aliases[existing].value, value);
    } else {
        // Add new alias
        strcpy(aliases[alias_count].name, name);
        strcpy(aliases[alias_count].value, value);
        alias_count++;
    }
}

static void remove_alias(const char *name) {
    int idx = find_alias(name);
    if (idx >= 0) {
        // Shift remaining aliases
        for (int i = idx; i < alias_count - 1; i++) {
            strcpy(aliases[i].name, aliases[i + 1].name);
            strcpy(aliases[i].value, aliases[i + 1].value);
        }
        alias_count--;
    }
}

static void list_aliases(void) {
    if (alias_count == 0) {
        printf("No aliases defined\n");
        return;
    }

    for (int i = 0; i < alias_count; i++) {
        printf("alias %s='%s'\n", aliases[i].name, aliases[i].value);
    }
}

int cmd_alias(int argc, char *argv[]) {
    if (argc == 1) {
        // List all aliases
        list_aliases();
        return 0;
    }

    if (argc == 2) {
        // Show specific alias
        int idx = find_alias(argv[1]);
        if (idx >= 0) {
            printf("alias %s='%s'\n", aliases[idx].name, aliases[idx].value);
        } else {
            printf("alias: %s: not found\n", argv[1]);
        }
        return 0;
    }

    if (argc >= 3) {
        if (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--delete") == 0) {
            // Delete alias
            for (int i = 2; i < argc; i++) {
                remove_alias(argv[i]);
            }
            return 0;
        } else {
            // Set alias: alias name=value or alias name value
            char *name = argv[1];
            char *value = argv[2];

            // Handle name=value format
            char *equals = strchr(name, '=');
            if (equals) {
                *equals = '\0';
                value = equals + 1;
            }

            add_alias(name, value);
            return 0;
        }
    }

    printf("Usage: alias [name[=value] ...] or alias -d name ...\n");
    return 1;
}

static void expand_alias(char *input, size_t size) {
    char temp_buf[ARG_MAX];
    char *argv[16];
    int argc;

    // Make a copy of the input for parsing
    strcpy(temp_buf, input);
    split_args(temp_buf, argv, &argc);

    if (argc == 0) return;

    int alias_idx = find_alias(argv[0]);
    if (alias_idx >= 0) {
        // Replace the command with the alias value
        char expanded[ARG_MAX];
        strcpy(expanded, aliases[alias_idx].value);

        // Append remaining arguments if any
        for (int i = 1; i < argc; i++) {
            strcat(expanded, " ");
            strcat(expanded, argv[i]);
        }

        if (strlen(expanded) < size) {
            strcpy(input, expanded);
        }
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

        // Expand aliases
        expand_alias(buf, ARG_MAX);

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
        if (strcmp(argv[0], "gcc")     == 0) { cmd_gcc(argc, argv);     continue; }
        if (strcmp(argv[0], "man")     == 0) { cmd_man(argc, argv);     continue; }
        if (strcmp(argv[0], "wc")      == 0) { cmd_wc(argc, argv);      continue; }
        if (strcmp(argv[0], "head")    == 0) { cmd_head(argc, argv);    continue; }
        if (strcmp(argv[0], "tail")    == 0) { cmd_tail(argc, argv);    continue; }
        if (strcmp(argv[0], "sort")    == 0) { cmd_sort(argc, argv);    continue; }
        if (strcmp(argv[0], "uniq")    == 0) { cmd_uniq(argc, argv);    continue; }
        if (strcmp(argv[0], "whoami")  == 0) { cmd_whoami(argc, argv);  continue; }
        if (strcmp(argv[0], "password") == 0) { cmd_password(argc, argv); continue; }
        if (strcmp(argv[0], "sysinfo") == 0) { cmd_sysinfo(argc, argv); continue; }
        if (strcmp(argv[0], "alias")   == 0) { cmd_alias(argc, argv);   continue; }
        if (strcmp(argv[0], "uptime")  == 0) { cmd_uptime(argc, argv);  continue; }
        if (strcmp(argv[0], "dmesg")   == 0) { cmd_dmesg(argc, argv);   continue; }
        if (strcmp(argv[0], "which")   == 0) { cmd_which(argc, argv);   continue; }

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