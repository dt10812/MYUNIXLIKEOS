#include "vfs.h"
#include "stdio.h"
#include "commands.h"
#include "string.h"
#include "stddef.h"
#include "pmm.h"
#include "io.h"

extern void cmd_clear(void);
extern void terminal_write(const char* data);
extern void terminal_putc(char c);
extern void terminal_backspace(void);
extern char keyboard_getchar(void);
extern size_t terminal_row;
extern size_t terminal_col;

#define NANO_BUF 8192
#define NANO_ROWS 21
#define NANO_COLS 80

static char nano_cut_buffer[NANO_BUF];

void nano_help(void) {
    cmd_clear();
    terminal_write("Nano Help:\n\n");
    terminal_write("^G Help     ^O Save    ^R Read      ^X Exit\n");
    terminal_write("^K Delete   ^U Undo   ^W Search    ^Y PrevPage\n");
    terminal_write("^V NextPage ^A Home   ^E End       ^D Delete\n");
    terminal_write("^B Left     ^F Right  ^N Down      ^P Up\n");
    terminal_write("^C Position ^J Justify ^T Spell    ^L GoToLine\n\n");
    terminal_write("Type text normally. Use Ctrl+O to save and Ctrl+X to exit.\n");
    terminal_write("Arrow navigation: Use Ctrl+A/F (left/right), Ctrl+P/N (up/down)\n");
    terminal_write("Press any key to continue...\n");
    keyboard_getchar();
}

static size_t nano_line_start(const char* buffer, size_t pos) {
    while (pos > 0 && buffer[pos - 1] != '\n')
        pos--;
    return pos;
}

static size_t nano_line_end(const char* buffer, size_t pos, size_t len) {
    while (pos < len && buffer[pos] != '\n')
        pos++;
    return pos;
}

static size_t nano_row_for_pos(const char* buffer, size_t pos) {
    size_t row = 0;
    for (size_t i = 0; i < pos; i++)
        if (buffer[i] == '\n') row++;
    return row;
}

static size_t nano_col_for_pos(const char* buffer, size_t pos) {
    size_t start = nano_line_start(buffer, pos);
    return pos - start;
}

static void nano_draw(const char* filename, const char* buffer, size_t len, size_t view_line, size_t cursor_pos) {
    cmd_clear();
    size_t pos = 0;
    size_t line = 0;

    while (line < view_line && pos < len) {
        if (buffer[pos] == '\n')
            line++;
        pos++;
    }

    for (size_t row = 0; row < NANO_ROWS; row++) {
        if (pos < len) {
            size_t end = nano_line_end(buffer, pos, len);
            for (size_t i = pos; i < end && i < len; i++) {
                if (terminal_col < NANO_COLS)
                    terminal_putc(buffer[i]);
            }
            pos = end;
            if (pos < len && buffer[pos] == '\n') {
                terminal_putc('\n');
                pos++;
            } else {
                terminal_putc('\n');
            }
        } else {
            terminal_write("~\n");
        }
    }

    terminal_write("File: ");
    terminal_write(filename);
    terminal_write("  Ctrl+O Save  Ctrl+X Exit  Ctrl+G Help\n");

    size_t line_num = nano_row_for_pos(buffer, cursor_pos);
    size_t col = nano_col_for_pos(buffer, cursor_pos);
    printf("Ln %u, Col %u, Size %u\n", (unsigned)line_num + 1, (unsigned)col + 1, (unsigned)len);

    if (line_num >= view_line && line_num < view_line + NANO_ROWS) {
        size_t screen_row = line_num - view_line;
        if (screen_row < TERM_HEIGHT) {
            move_cursor((int)col, (int)screen_row);
        }
    }
}

static void nano_save(vnode_t* file, const char* buffer, size_t len) {
    if (!file->content) {
        file->content = (char*)pmm_alloc_z(NANO_BUF);
        if (!file->content) {
            terminal_write("nano: out of memory\n");
            return;
        }
    }
    for (size_t i = 0; i < len; i++)
        file->content[i] = buffer[i];
    file->content[len] = '\0';
    file->size = len;
    terminal_write("nano: written to disk\n");
}

static void nano_search(const char* buffer, size_t len, size_t* cursor_pos, size_t* view_line) {
    (void)len;
    char query[128];
    terminal_write("Search: ");
    read_line(query, sizeof(query));
    const char* found = strstr(buffer, query);
    if (!found) {
        terminal_write("Not found\n");
        return;
    }
    *cursor_pos = found - buffer;
    *view_line = nano_row_for_pos(buffer, *cursor_pos);
}

static void nano_justify_line(char* buffer, size_t* len, size_t cursor_pos) {
    size_t start = nano_line_start(buffer, cursor_pos);
    size_t end = nano_line_end(buffer, cursor_pos, *len);
    size_t write = start;
    bool in_space = false;
    for (size_t i = start; i < end; i++) {
        if (buffer[i] == ' ' || buffer[i] == '\t') {
            if (!in_space) {
                buffer[write++] = ' ';
                in_space = true;
            }
        } else {
            buffer[write++] = buffer[i];
            in_space = false;
        }
    }
    if (write > start && buffer[write - 1] == ' ') write--;
    size_t removed = end - write;
    if (removed) {
        memmove(buffer + write, buffer + end, *len - end);
        *len -= removed;
    }
}

static void nano_cut_line(char* buffer, size_t* len, size_t* cursor_pos) {
    size_t start = nano_line_start(buffer, *cursor_pos);
    size_t end = nano_line_end(buffer, *cursor_pos, *len);
    size_t line_len = end - start;
    if (line_len >= sizeof(nano_cut_buffer)) line_len = sizeof(nano_cut_buffer) - 1;
    memcpy(nano_cut_buffer, buffer + start, line_len);
    nano_cut_buffer[line_len] = '\0';
    if (end < *len && buffer[end] == '\n') end++;
    memmove(buffer + start, buffer + end, *len - end);
    *len -= end - start;
    *cursor_pos = start;
}

static void nano_delete_line(char* buffer, size_t* len, size_t* cursor_pos) {
    nano_cut_line(buffer, len, cursor_pos);
}

static void nano_goto_line(char* buffer, size_t len, size_t* cursor_pos, size_t* view_line) {
    (void)buffer;
    (void)len;
    char line_str[32];
    terminal_write("Enter line number: ");
    read_line(line_str, sizeof(line_str));
    size_t line_num = 0;
    for (const char* p = line_str; *p >= '0' && *p <= '9'; p++) {
        line_num = line_num * 10 + (*p - '0');
    }
    if (line_num < 1) line_num = 1;
    
    size_t current_line = 0;
    *cursor_pos = 0;
    for (size_t i = 0; i < len && current_line < line_num - 1; i++) {
        if (buffer[i] == '\n') {
            current_line++;
            *cursor_pos = i + 1;
        }
    }
    *view_line = current_line > NANO_ROWS / 2 ? current_line - NANO_ROWS / 2 : 0;
}

static void nano_paste(char* buffer, size_t* len, size_t* cursor_pos) {
    size_t cut_len = strlen(nano_cut_buffer);
    if (*len + cut_len >= NANO_BUF) {
        terminal_write("nano: paste too large\n");
        return;
    }
    memmove(buffer + *cursor_pos + cut_len, buffer + *cursor_pos, *len - *cursor_pos);
    memcpy(buffer + *cursor_pos, nano_cut_buffer, cut_len);
    *cursor_pos += cut_len;
    *len += cut_len;
}

static bool nano_spell_check(const char* buffer, size_t cursor_pos) {
    static const char* dict[] = {
        "the", "and", "to", "of", "a", "in", "is", "it", "you", "that",
        "this", "for", "with", "on", "as", "are", "be", "or", "not"
    };
    size_t start = cursor_pos;
    while (start > 0 && ((buffer[start - 1] >= 'a' && buffer[start - 1] <= 'z') ||
                        (buffer[start - 1] >= 'A' && buffer[start - 1] <= 'Z')))
        start--;
    size_t end = cursor_pos;
    while (end < strlen(buffer) && ((buffer[end] >= 'a' && buffer[end] <= 'z') ||
                                   (buffer[end] >= 'A' && buffer[end] <= 'Z')))
        end++;
    if (end <= start) return false;
    char word[64];
    size_t len = end - start;
    if (len >= sizeof(word)) return false;
    for (size_t i = 0; i < len; i++) {
        char c = buffer[start + i];
        word[i] = (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;
    }
    word[len] = '\0';
    for (size_t i = 0; i < sizeof(dict) / sizeof(dict[0]); i++) {
        if (strcmp(word, dict[i]) == 0) return true;
    }
    return false;
}

int cmd_nano(int argc, char** argv) {
    if (argc < 2) {
        terminal_write("Usage: nano <filename>\n");
        return -1;
    }

    const char* path = argv[1];
    vnode_t* file = vfs_lookup(path);
    if (!file) {
        if (k_touch(path) != 0) {
            terminal_write("nano: error creating file\n");
            return -1;
        }
        file = vfs_lookup(path);
        if (!file) {
            terminal_write("nano: unable to open file\n");
            return -1;
        }
    }

    static char buffer[NANO_BUF];
    size_t len = 0;
    size_t cursor_pos = 0;
    size_t view_line = 0;

    if (file->content && file->size > 0) {
        while (len < file->size && len + 1 < NANO_BUF) {
            buffer[len] = file->content[len];
            len++;
        }
    }
    buffer[len] = '\0';
    cursor_pos = len;

    while (1) {
        nano_draw(path, buffer, len, view_line, cursor_pos);
        char c = keyboard_getchar();
        if (!c) continue;

        if (c == 0x07) {
            nano_help();
            continue;
        }
        if (c == 0x0F) {
            nano_save(file, buffer, len);
            continue;
        }
        if (c == 0x12) {
            if (file->content) {
                len = 0;
                while (len < file->size && len + 1 < NANO_BUF) {
                    buffer[len] = file->content[len];
                    len++;
                }
                buffer[len] = '\0';
                cursor_pos = len;
                view_line = 0;
                terminal_write("nano: file reloaded\n");
            } else {
                terminal_write("nano: no file to read\n");
            }
            continue;
        }
        if (c == 0x19) {
            if (view_line > NANO_ROWS) view_line -= NANO_ROWS;
            else view_line = 0;
            continue;
        }
        if (c == 0x16) {
            view_line += NANO_ROWS;
            continue;
        }
        if (c == 0x0B) {
            nano_cut_line(buffer, &len, &cursor_pos);
            continue;
        }
        if (c == 0x15) {
            nano_paste(buffer, &len, &cursor_pos);
            continue;
        }
        if (c == 0x03) {
            size_t line = nano_row_for_pos(buffer, cursor_pos);
            size_t col = nano_col_for_pos(buffer, cursor_pos);
            printf("Ln %u, Col %u, Size %u\n", (unsigned)line + 1, (unsigned)col + 1, (unsigned)len);
            continue;
        }
        /* Ctrl+H - Delete line (Delete) */
        if (c == 0x08 || c == 0x7F) {
            if (cursor_pos > 0) {
                memmove(buffer + cursor_pos - 1, buffer + cursor_pos, len - cursor_pos);
                cursor_pos--;
                len--;
                buffer[len] = '\0';
            }
            continue;
        }
        if (c == 0x17) {
            nano_search(buffer, len, &cursor_pos, &view_line);
            continue;
        }
        if (c == 0x14) {
            if (nano_spell_check(buffer, cursor_pos))
                terminal_write("Spell: word looks okay\n");
            else
                terminal_write("Spell: unknown word\n");
            continue;
        }
        /* Ctrl+A - Home (go to start of line) */
        if (c == 0x01) {
            cursor_pos = nano_line_start(buffer, cursor_pos);
            continue;
        }
        /* Ctrl+B - Left */
        if (c == 0x02) {
            if (cursor_pos > 0) cursor_pos--;
            continue;
        }
        /* Ctrl+F - Right */
        if (c == 0x06) {
            if (cursor_pos < len) cursor_pos++;
            continue;
        }
        /* Ctrl+P - Previous line (Up) */
        if (c == 0x10) {
            size_t line_start = nano_line_start(buffer, cursor_pos);
            if (line_start > 0) {
                size_t prev_line_start = nano_line_start(buffer, line_start - 2);
                size_t col = cursor_pos - line_start;
                size_t prev_line_end = nano_line_end(buffer, prev_line_start, len);
                size_t prev_line_len = prev_line_end - prev_line_start;
                if (col > prev_line_len) col = prev_line_len;
                cursor_pos = prev_line_start + col;
            }
            continue;
        }
        /* Ctrl+Z - Next line (Down) instead of suspending */
        if (c == 0x1A) {
            size_t line_end = nano_line_end(buffer, cursor_pos, len);
            if (line_end < len && buffer[line_end] == '\n') {
                size_t next_line_start = line_end + 1;
                size_t col = cursor_pos - nano_line_start(buffer, cursor_pos);
                size_t next_line_end = nano_line_end(buffer, next_line_start, len);
                size_t next_line_len = next_line_end - next_line_start;
                if (col > next_line_len) col = next_line_len;
                cursor_pos = next_line_start + col;
            }
            continue;
        }
        /* Ctrl+L - Go to line */
        if (c == 0x0C) {
            nano_goto_line(buffer, len, &cursor_pos, &view_line);
            continue;
        }
        /* Ctrl+D - Delete character at cursor */
        if (c == 0x04) {
            if (cursor_pos < len) {
                memmove(buffer + cursor_pos, buffer + cursor_pos + 1, len - cursor_pos - 1);
                len--;
                buffer[len] = '\0';
            }
            continue;
        }
        if (c == 0x18) {
            terminal_write("Save changes? (y/n): ");
            char answer = keyboard_getchar();
            terminal_write("\n");
            if (answer == 'y' || answer == 'Y') {
                nano_save(file, buffer, len);
                return 0;
            }
            return 0;
        }
        if (c == 0x08 || c == 0x7F) {
            if (cursor_pos > 0) {
                memmove(buffer + cursor_pos - 1, buffer + cursor_pos, len - cursor_pos);
                cursor_pos--;
                len--;
                buffer[len] = '\0';
            }
            continue;
        }
        if ((unsigned char)c >= 32) {
            if (len + 1 < NANO_BUF) {
                memmove(buffer + cursor_pos + 1, buffer + cursor_pos, len - cursor_pos);
                buffer[cursor_pos++] = c;
                len++;
                buffer[len] = '\0';
            }
            continue;
        }

        size_t current_line = nano_row_for_pos(buffer, cursor_pos);
        if (current_line < view_line)
            view_line = current_line;
        else if (current_line >= view_line + NANO_ROWS)
            view_line = current_line - NANO_ROWS + 1;
    }
}
