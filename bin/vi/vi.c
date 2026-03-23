/* WIP:
 * Not ideal, but it "works" for now.
 * Non-standard but working towards it.
 * Much of this needs to be abstracted.
 * Few bugs such as loading file adds extra \n.
 * 
 * TODO:
 * Short-term goals:
 *  - Fix minor load and save issue.
 *  - Add ability to remove lines.
 *  - Arrow key movement.
 *  - Make lines not wrap when too long.
 * Long-term goals:
 *  - Motion commands.
 *  - Use ANSI escape sequences when supported.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "vfs.h"
#include "pmm.h"
#include "io.h"
#include "commands.h"
#include "stdio.h"
#include "string.h"

extern void do_clear();
extern size_t terminal_col, terminal_row;

#define LINE_MAX 2048 // should be later defined in limits.h
#define VI_BUF  4096
#define VI_ROWS 24

char *inst_str = "-- INSERT --";
char *cmd_str = "-- COMMAND --";

static void vi_print_status(const char *msg) {
    size_t c = terminal_col;
    size_t r = terminal_row;

    terminal_col = 0;
    terminal_row = VI_ROWS;
    for (int i = 0; i < 80; i++)
        putchar(' '); // clear bottom line

    terminal_col = 0;
    terminal_row = VI_ROWS;
    for(size_t i = 0; msg[i]; i++)
        putchar(msg[i]);

    terminal_col = c;
    terminal_row = r;
}

static void vi_draw(const char *name, const char *buf, size_t len) {
    do_clear();
    terminal_col = 0;
    terminal_row = 0;

    size_t buf_pos = 0;
    size_t content_rows = 0;

    for (int row = 0; row < VI_ROWS; row++) {
        if (buf_pos < len) {
            while (buf_pos < len && buf[buf_pos] != '\n') {
                putchar(buf[buf_pos++]);
            }
            if (buf_pos < len)
                buf_pos++;
            content_rows++;
        } else {
            putchar('~');
        }
        putchar('\n');
    }

    terminal_col = 0;
    terminal_row = VI_ROWS;

    for(size_t i = 0; name[i]; i++)
        putchar(name[i]);

    terminal_col = 0;
    terminal_row = content_rows;
}

int cmd_vi(int argc, char **argv) {
    if (argc < 2) {
        char *err = "Usage: vi <filename>\n";
        for(size_t i = 0; err[i]; i++)
            putchar(err[i]);
        return -1;
    }

    vnode_t *file = vfs_lookup(argv[1]);
    if (!file) {
        if (k_touch(argv[1]) != 0) {
            char *err = "vi: cannot create file\n";
            for(size_t i = 0; err[i]; i++)
                putchar(err[i]);
            return -1;
        }
        file = vfs_lookup(argv[1]);
    }
    if (!file || !(file->flags & VFS_FILE)) {
        char *err = "vi: not a file\n";
        for(size_t i = 0; err[i]; i++)
            putchar(err[i]);
        return -1;
    }

    char buf[VI_BUF];
    size_t len = 0;
    if (file->content) {
        for (const char *p = file->content; *p && len < VI_BUF - 1;)
            buf[len++] = *p++;
    }
    buf[len] = '\0';

    bool cmd_mode = false;
    char input[LINE_MAX];
    size_t input_len = 0;

    vi_draw(argv[1], buf, len);
    vi_print_status(inst_str);

    for (;;) {
        char c = getchar();

        if (c == 0x1B) { // handle esc
            if (!cmd_mode && input_len > 0 && len + input_len + 1 < VI_BUF) {
                for (size_t i = 0; i < input_len; i++)
                    buf[len + i] = input[i];
                len += input_len;
                buf[len++] = '\n';
                buf[len] = '\0';
            }
            cmd_mode = !cmd_mode;
            input_len = 0;
            input[0] = '\0';
            if (cmd_mode) {
                vi_print_status(cmd_str);
                terminal_row = VI_ROWS;
                terminal_col = strlen(cmd_str) + 1;
            } else {
                vi_draw(argv[1], buf, len);
                vi_print_status(inst_str);
            }
            continue;
        }

        if (c == '\n') {
            input[input_len] = '\0';

            if (cmd_mode) {
                if (strcmp(input, ":wq") == 0) {
                    if (!file->content)
                        file->content = (char*)pmm_alloc_z(VI_BUF);
                    if (!file->content) {
                        char *err = "\nvi: out of memory\n";
                        for(size_t i = 0; err[i]; i++)
                            putchar(err[i]);
                        return -1;
                    }
                    
                    size_t save_len = (len > 0 && buf[len-1] == '\n') ? len - 1 : len;
                    size_t i;
                    for (i = 0; i < save_len && i < VI_BUF - 1; i++)
                        file->content[i] = buf[i];
                    file->content[i] = '\0';
                    do_clear();
                    return 0;
                }

                if (strcmp(input, ":q!") == 0) {
                    do_clear();
                    return 0;
                }

                char err[LINE_MAX]; // enough for output
                size_t ei = 0;
                for (const char *p = "Not an editor command: "; *p;)
                    err[ei++] = *p++;
                for (size_t j = 0; input[j] && ei < sizeof(err) - 1;)
                    err[ei++] = input[j++];
                err[ei] = '\0';
                vi_print_status(err);

            }
            else if (len + input_len + 1 < VI_BUF) {
                for (size_t i = 0; i < input_len; i++)
                    buf[len + i] = input[i];
                len += input_len;
                buf[len++] = '\n';
                buf[len]   = '\0';
                vi_draw(argv[1], buf, len);
                vi_print_status(inst_str);

            } else {
                vi_print_status("Buffer full");
            }

            // clear input
            input_len = 0;
            input[0] = '\0';
            continue;
        }

        if (c == '\b' || c == 0x7F) { // back space
            if (input_len > 0) {
                input[--input_len] = '\0';
                terminal_col--;
                putchar(' ');
                terminal_col--;
            }
            continue;
        }

        // output char and add null terminator
        if (input_len < LINE_MAX - 1) {
            input[input_len++] = c;
            input[input_len] = '\0';
            putchar(c);
        }
    }
}