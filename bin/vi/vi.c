/* Improved vi-like editor with basic motion and command support. */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "vfs.h"
#include "pmm.h"
#include "io.h"
#include "commands.h"
#include "stdio.h"
#include "string.h"

extern void cmd_clear(void);
extern size_t terminal_col;
extern size_t terminal_row;

#define LINE_MAX 2048
#define VI_BUF  8192
#define VI_ROWS 23
#define VI_COLS 80

static const char *inst_str = "-- INSERT --";
static const char *cmd_str = "-- COMMAND --";

static void vi_print_status(const char *msg) {
    size_t saved_col = terminal_col;
    size_t saved_row = terminal_row;

    terminal_col = 0;
    terminal_row = VI_ROWS;
    for (int i = 0; i < VI_COLS; i++)
        putchar(' ');

    terminal_col = 0;
    terminal_row = VI_ROWS;
    for (size_t i = 0; msg[i]; i++)
        putchar(msg[i]);

    terminal_col = saved_col;
    terminal_row = saved_row;
}

static size_t vi_line_start(const char *buf, size_t pos) {
    while (pos > 0 && buf[pos - 1] != '\n')
        pos--;
    return pos;
}

static size_t vi_line_end(const char *buf, size_t pos, size_t len) {
    while (pos < len && buf[pos] != '\n')
        pos++;
    return pos;
}

static size_t vi_line_number(const char *buf, size_t pos) {
    size_t line = 0;
    for (size_t i = 0; i < pos && buf[i]; i++)
        if (buf[i] == '\n') line++;
    return line;
}

static size_t vi_column(const char *buf, size_t pos) {
    return pos - vi_line_start(buf, pos);
}

static void vi_draw(const char *name, const char *buf, size_t len, size_t cursor) {
    cmd_clear();
    size_t pos = 0;
    for (int row = 0; row < VI_ROWS; row++) {
        if (pos < len) {
            size_t end = vi_line_end(buf, pos, len);
            for (size_t i = pos; i < end && i < len; i++) {
                if (terminal_col < VI_COLS)
                    putchar(buf[i]);
            }
            pos = end;
            if (pos < len && buf[pos] == '\n') {
                putchar('\n');
                pos++;
            } else {
                putchar('\n');
            }
        } else {
            putchar('~');
            putchar('\n');
        }
    }

    terminal_col = 0;
    terminal_row = VI_ROWS;
    printf("%s  Ln %u, Col %u", name, (unsigned)vi_line_number(buf, cursor) + 1, (unsigned)vi_column(buf, cursor) + 1);
}

static void vi_move_up(const char *buf, size_t len, size_t *cursor) {
    (void)len;
    size_t col = vi_column(buf, *cursor);
    if (*cursor == 0) return;
    size_t prev_line_end = vi_line_start(buf, *cursor) - 1;
    size_t prev_line_start = vi_line_start(buf, prev_line_end);
    size_t target = prev_line_start + col;
    if (target > prev_line_end) target = prev_line_end;
    *cursor = target;
}

static void vi_move_down(const char *buf, size_t len, size_t *cursor) {
    size_t col = vi_column(buf, *cursor);
    size_t next_line_start = vi_line_end(buf, *cursor, len);
    if (next_line_start < len && buf[next_line_start] == '\n')
        next_line_start++;
    if (next_line_start >= len) return;
    size_t next_line_end = vi_line_end(buf, next_line_start, len);
    size_t target = next_line_start + col;
    if (target > next_line_end) target = next_line_end;
    *cursor = target;
}

static void vi_save(vnode_t *file, const char *buf, size_t len) {
    if (!file->content) {
        file->content = (char*)pmm_alloc_z(VI_BUF);
        if (!file->content) return;
    }
    for (size_t i = 0; i < len; i++)
        file->content[i] = buf[i];
    file->content[len] = '\0';
    file->size = len;
}

static void vi_delete_line(char *buf, size_t *len, size_t *cursor) {
    size_t start = vi_line_start(buf, *cursor);
    size_t end = vi_line_end(buf, *cursor, *len);
    if (end < *len && buf[end] == '\n') end++;
    memmove(buf + start, buf + end, *len - end);
    *len -= end - start;
    *cursor = start;
}

static void vi_insert_char(char *buf, size_t *len, size_t *cursor, char c) {
    if (*len + 1 >= VI_BUF) return;
    memmove(buf + *cursor + 1, buf + *cursor, *len - *cursor);
    buf[*cursor] = c;
    (*cursor)++;
    (*len)++;
}

static void vi_delete_char(char *buf, size_t *len, size_t *cursor) {
    if (*cursor >= *len) return;
    memmove(buf + *cursor, buf + *cursor + 1, *len - *cursor - 1);
    (*len)--;
}

int cmd_vi(int argc, char **argv) {
    if (argc < 2) {
        terminal_write("Usage: vi <filename>\n");
        return -1;
    }

    vnode_t *file = vfs_lookup(argv[1]);
    if (!file) {
        if (k_touch(argv[1]) != 0) {
            terminal_write("vi: cannot create file\n");
            return -1;
        }
        file = vfs_lookup(argv[1]);
    }
    if (!file || !(file->flags & VFS_FILE)) {
        terminal_write("vi: not a file\n");
        return -1;
    }

    static char buf[VI_BUF];
    size_t len = 0;
    if (file->content) {
        while (len < VI_BUF - 1 && file->content[len]) {
            buf[len] = file->content[len];
            len++;
        }
    }
    buf[len] = '\0';

    bool insert_mode = false;
    size_t cursor = len;
    char command_line[LINE_MAX];

    vi_draw(argv[1], buf, len, cursor);
    vi_print_status(cmd_str);

    for (;;) {
        char c = getchar();
        if (!c) continue;

        if (!insert_mode) {
            if (c == 'i') {
                insert_mode = true;
                vi_print_status(inst_str);
                continue;
            }
            if (c == 'a') {
                if (cursor < len) cursor++;
                insert_mode = true;
                vi_print_status(inst_str);
                continue;
            }
            if (c == 'o') {
                size_t line_end = vi_line_end(buf, cursor, len);
                if (len + 1 < VI_BUF) {
                    cursor = line_end;
                    vi_insert_char(buf, &len, &cursor, '\n');
                }
                insert_mode = true;
                vi_print_status(inst_str);
                vi_draw(argv[1], buf, len, cursor);
                continue;
            }
            if (c == 'h') {
                if (cursor > 0) cursor--;
                vi_draw(argv[1], buf, len, cursor);
                continue;
            }
            if (c == 'l') {
                if (cursor < len) cursor++;
                vi_draw(argv[1], buf, len, cursor);
                continue;
            }
            if (c == 'k') {
                vi_move_up(buf, len, &cursor);
                vi_draw(argv[1], buf, len, cursor);
                continue;
            }
            if (c == 'j') {
                vi_move_down(buf, len, &cursor);
                vi_draw(argv[1], buf, len, cursor);
                continue;
            }
            if (c == 'x') {
                vi_delete_char(buf, &len, &cursor);
                if (cursor > len) cursor = len;
                vi_draw(argv[1], buf, len, cursor);
                continue;
            }
            if (c == 'd') {
                char next = getchar();
                if (next == 'd') {
                    vi_delete_line(buf, &len, &cursor);
                    vi_draw(argv[1], buf, len, cursor);
                    continue;
                }
                continue;
            }
            if (c == ':') {
                terminal_write(":");
                read_line(command_line, sizeof(command_line));
                if (strcmp(command_line, "w") == 0 || strcmp(command_line, "wq") == 0) {
                    vi_save(file, buf, len);
                }
                if (strcmp(command_line, "q") == 0) {
                    return 0;
                }
                if (strcmp(command_line, "q!") == 0) {
                    return 0;
                }
                if (strcmp(command_line, "wq") == 0) {
                    return 0;
                }
                if (strncmp(command_line, "e ", 2) == 0) {
                    len = 0;
                    if (file->content) {
                        while (len < VI_BUF - 1 && file->content[len]) {
                            buf[len] = file->content[len];
                            len++;
                        }
                    }
                    buf[len] = '\0';
                    cursor = len;
                }
                vi_draw(argv[1], buf, len, cursor);
                vi_print_status(cmd_str);
                continue;
            }
            continue;
        }

        if (c == 0x1B) {
            insert_mode = false;
            vi_print_status(cmd_str);
            continue;
        }
        if (c == 0x08 || c == 0x7F) {
            if (cursor > 0) {
                cursor--;
                memmove(buf + cursor, buf + cursor + 1, len - cursor);
                len--;
                buf[len] = '\0';
            }
            vi_draw(argv[1], buf, len, cursor);
            continue;
        }
        if (c == '\r' || c == '\n') {
            if (len + 1 < VI_BUF) {
                vi_insert_char(buf, &len, &cursor, '\n');
            }
            vi_draw(argv[1], buf, len, cursor);
            continue;
        }
        if ((unsigned char)c >= 32) {
            vi_insert_char(buf, &len, &cursor, c);
            vi_draw(argv[1], buf, len, cursor);
            continue;
        }
    }
}
