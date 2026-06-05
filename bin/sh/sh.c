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
#include "keyboard.h"

/* Forward declarations for functions defined later */
static void add_to_history(const char *cmd);
static const char* get_history_prev(void);
static const char* get_history_next(void);
static void set_env_var(const char *name, const char *value);
static const char* get_env_var(const char *name);
static void init_default_env(void);
static void list_env_vars(void);
static void expand_env_vars(char *input, size_t size);

#define MAX_ALIASES 32
#define MAX_ALIAS_NAME 32
#define MAX_ALIAS_VALUE 128

/* Command history */
#define MAX_HISTORY 100
#define MAX_HISTORY_LEN 256
static char command_history[MAX_HISTORY][MAX_HISTORY_LEN];
static int history_count = 0;
static int history_pos = -1;

/* Environment variables */
#define MAX_ENV_VARS 64
#define MAX_ENV_NAME 64
#define MAX_ENV_VALUE 256
static struct {
    char name[MAX_ENV_NAME];
    char value[MAX_ENV_VALUE];
} env_vars[MAX_ENV_VARS];
static int env_count = 0;

/* Special key codes for arrow keys */
#define KEY_UP    0xE0
#define KEY_DOWN  0xE1
#define KEY_LEFT  0xE2
#define KEY_RIGHT 0xE3

static struct {
    char name[MAX_ALIAS_NAME];
    char value[MAX_ALIAS_VALUE];
} aliases[MAX_ALIASES];
static int alias_count = 0;

void read_line(char *buf, size_t size);
extern void cmd_clear(void);

/* Access keyboard buffer directly since we're in kernel space */

static void shell_buffer_reset(void) {
    memset(input_buffer, 0, INPUT_BUFFER_SIZE);
    buffer_read_idx = 0;
    buffer_write_idx = 0;
}

static char shell_buffer_read(void) {
    if (buffer_read_idx < buffer_write_idx) {
        return input_buffer[buffer_read_idx++];
    }
    return 0;
}

static void shell_read_line(char *buf, size_t size) {
    size_t idx = 0;
    char current_line[MAX_HISTORY_LEN] = {0};
    int history_navigating = 0;
    
    shell_buffer_reset();
    
    while (1) {
        /* First try to read from the buffer */
        char c = shell_buffer_read();
        
        /* If buffer is empty, collect characters from keyboard */
        if (!c) {
            c = keyboard_pollchar();
            if (!c) {
                /* No input available, try blocking getchar as fallback */
                c = keyboard_getchar();
            }
        }
        
        if (!c) continue;
        
        if (c == '\r' || c == '\n') {
            terminal_write("\n");
            buf[idx] = '\0';
            shell_buffer_reset();
            
            /* Add to history if not empty */
            if (idx > 0) {
                add_to_history(buf);
            }
            return;
        }
        
        /* Handle arrow keys for history navigation */
        if (c == KEY_UP) {
            if (!history_navigating) {
                /* Save current input */
                strcpy(current_line, buf);
                history_navigating = 1;
            }
            
            const char *prev_cmd = get_history_prev();
            if (prev_cmd) {
                /* Clear current line */
                while (idx > 0) {
                    terminal_backspace();
                    idx--;
                }
                
                /* Copy history command */
                strncpy(buf, prev_cmd, size - 1);
                buf[size - 1] = '\0';
                idx = strlen(buf);
                
                /* Redisplay */
                terminal_write(buf);
            }
            continue;
        }
        
        if (c == KEY_DOWN) {
            if (history_navigating) {
                const char *next_cmd = get_history_next();
                if (next_cmd) {
                    /* Clear current line */
                    while (idx > 0) {
                        terminal_backspace();
                        idx--;
                    }
                    
                    /* Copy history command */
                    strncpy(buf, next_cmd, size - 1);
                    buf[size - 1] = '\0';
                    idx = strlen(buf);
                    
                    /* Redisplay */
                    terminal_write(buf);
                } else {
                    /* Restore original input */
                    while (idx > 0) {
                        terminal_backspace();
                        idx--;
                    }
                    
                    strcpy(buf, current_line);
                    idx = strlen(buf);
                    terminal_write(buf);
                    history_navigating = 0;
                }
            }
            continue;
        }
        
        if ((c == '\b' || c == 0x7F) && idx > 0) {
            idx--;
            terminal_backspace();
            continue;
        }
        
        if (idx + 1 < size) {
            buf[idx++] = c;
            terminal_putc(c);
        }
    }
}

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

    // Validate name and value lengths
    if (strlen(name) >= MAX_ALIAS_NAME) {
        printf("alias: name too long\n");
        return;
    }
    if (strlen(value) >= MAX_ALIAS_VALUE) {
        printf("alias: value too long\n");
        return;
    }

    int existing = find_alias(name);
    if (existing >= 0) {
        // Update existing alias
        strncpy(aliases[existing].value, value, MAX_ALIAS_VALUE - 1);
        aliases[existing].value[MAX_ALIAS_VALUE - 1] = '\0';
    } else {
        // Add new alias
        strncpy(aliases[alias_count].name, name, MAX_ALIAS_NAME - 1);
        aliases[alias_count].name[MAX_ALIAS_NAME - 1] = '\0';
        strncpy(aliases[alias_count].value, value, MAX_ALIAS_VALUE - 1);
        aliases[alias_count].value[MAX_ALIAS_VALUE - 1] = '\0';
        alias_count++;
    }
}

static void remove_alias(const char *name) {
    int idx = find_alias(name);
    if (idx >= 0) {
        // Shift remaining aliases
        for (int i = idx; i < alias_count - 1; i++) {
            strncpy(aliases[i].name, aliases[i + 1].name, MAX_ALIAS_NAME - 1);
            aliases[i].name[MAX_ALIAS_NAME - 1] = '\0';
            strncpy(aliases[i].value, aliases[i + 1].value, MAX_ALIAS_VALUE - 1);
            aliases[i].value[MAX_ALIAS_VALUE - 1] = '\0';
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
    strncpy(temp_buf, input, ARG_MAX - 1);
    temp_buf[ARG_MAX - 1] = '\0';
    split_args(temp_buf, argv, &argc);

    if (argc == 0) return;

    int alias_idx = find_alias(argv[0]);
    if (alias_idx >= 0) {
        // Replace the command with the alias value
        char expanded[ARG_MAX];
        strncpy(expanded, aliases[alias_idx].value, ARG_MAX - 1);
        expanded[ARG_MAX - 1] = '\0';

        // Append remaining arguments if any
        for (int i = 1; i < argc; i++) {
            size_t current_len = strlen(expanded);
            if (current_len + 1 < ARG_MAX) {
                strncat(expanded, " ", ARG_MAX - current_len - 1);
            }
            current_len = strlen(expanded);
            if (current_len < ARG_MAX && argv[i]) {
                strncat(expanded, argv[i], ARG_MAX - current_len - 1);
            }
        }

        if (strlen(expanded) < size) {
            strncpy(input, expanded, size - 1);
            input[size - 1] = '\0';
        }
    }
}

void sh(void) {
    char buf[ARG_MAX];
    char *argv[16];
    int argc;

    /* Initialize default environment variables */
    init_default_env();

    for (;;) {
        terminal_write("root@dev_null# ");

        memset(buf,  0, sizeof(buf));
        memset(argv, 0, sizeof(argv));
        argc = 0;

        shell_read_line(buf, ARG_MAX);

        // Expand environment variables
        expand_env_vars(buf, ARG_MAX);

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
        if (strcmp(argv[0], "uname")   == 0) { cmd_uname(argc, argv);   continue; }
        if (strcmp(argv[0], "export")  == 0) {
            if (argc == 1) {
                list_env_vars();
            } else if (argc == 2) {
                /* Show specific variable */
                const char *value = get_env_var(argv[1]);
                if (value) {
                    printf("%s=%s\n", argv[1], value);
                } else {
                    printf("export: %s: not found\n", argv[1]);
                }
            } else if (argc >= 3) {
                /* Set variable: export NAME=VALUE or export NAME VALUE */
                char *name = argv[1];
                char *value = argv[2];
                
                /* Handle NAME=VALUE format */
                char *equals = strchr(name, '=');
                if (equals) {
                    *equals = '\0';
                    value = equals + 1;
                }
                
                set_env_var(name, value);
            }
            continue;
        }

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

/* History functions */
static void add_to_history(const char *cmd) {
    if (history_count < MAX_HISTORY) {
        strcpy(command_history[history_count], cmd);
        history_count++;
    } else {
        /* Shift history and add new command */
        for (int i = 1; i < MAX_HISTORY; i++) {
            strcpy(command_history[i-1], command_history[i]);
        }
        strcpy(command_history[MAX_HISTORY-1], cmd);
    }
    history_pos = history_count;
}

static const char* get_history_prev(void) {
    if (history_pos > 0) {
        history_pos--;
        return command_history[history_pos];
    }
    return NULL;
}

static const char* get_history_next(void) {
    if (history_pos < history_count - 1) {
        history_pos++;
        return command_history[history_pos];
    }
    return NULL;
}

/* Environment variable functions */
static void set_env_var(const char *name, const char *value) {
    if (!name || !value) return;
    
    /* Find existing variable */
    for (int i = 0; i < env_count; i++) {
        if (strcmp(env_vars[i].name, name) == 0) {
            strncpy(env_vars[i].value, value, MAX_ENV_VALUE - 1);
            env_vars[i].value[MAX_ENV_VALUE - 1] = '\0';
            return;
        }
    }
    
    /* Add new variable */
    if (env_count < MAX_ENV_VARS) {
        strncpy(env_vars[env_count].name, name, MAX_ENV_NAME - 1);
        env_vars[env_count].name[MAX_ENV_NAME - 1] = '\0';
        strncpy(env_vars[env_count].value, value, MAX_ENV_VALUE - 1);
        env_vars[env_count].value[MAX_ENV_VALUE - 1] = '\0';
        env_count++;
    }
}

static const char* get_env_var(const char *name) {
    if (!name) return NULL;
    
    for (int i = 0; i < env_count; i++) {
        if (strcmp(env_vars[i].name, name) == 0) {
            return env_vars[i].value;
        }
    }
    return NULL;
}

static void init_default_env(void) {
    set_env_var("PATH", "/bin:/usr/bin");
    set_env_var("HOME", "/");
    set_env_var("USER", "root");
    set_env_var("SHELL", "/bin/sh");
    set_env_var("PWD", "/");
}

static void list_env_vars(void) {
    for (int i = 0; i < env_count; i++) {
        printf("%s=%s\n", env_vars[i].name, env_vars[i].value);
    }
}

static void expand_env_vars(char *input, size_t size) {
    char output[MAX_HISTORY_LEN] = {0};
    size_t out_idx = 0;
    size_t in_idx = 0;
    
    while (input[in_idx] && out_idx < size - 1) {
        if (input[in_idx] == '$') {
            /* Find variable name */
            size_t var_start = ++in_idx;
            while (input[in_idx] && ((input[in_idx] >= 'a' && input[in_idx] <= 'z') || 
                                   (input[in_idx] >= 'A' && input[in_idx] <= 'Z') || 
                                   (input[in_idx] >= '0' && input[in_idx] <= '9') || 
                                   input[in_idx] == '_')) {
                in_idx++;
            }
            
            /* Extract variable name */
            char var_name[MAX_ENV_NAME];
            size_t var_len = in_idx - var_start;
            if (var_len >= MAX_ENV_NAME) var_len = MAX_ENV_NAME - 1;
            strncpy(var_name, &input[var_start], var_len);
            var_name[var_len] = '\0';
            
            /* Get variable value */
            const char *value = get_env_var(var_name);
            if (value) {
                /* Copy value to output */
                size_t value_len = strlen(value);
                if (out_idx + value_len >= size - 1) {
                    value_len = size - 1 - out_idx;
                }
                strncpy(&output[out_idx], value, value_len);
                out_idx += value_len;
            }
        } else {
            output[out_idx++] = input[in_idx++];
        }
    }
    
    output[out_idx] = '\0';
    strcpy(input, output);
}