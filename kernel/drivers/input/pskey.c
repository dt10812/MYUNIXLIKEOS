/* ps2 keyboard driver */
#include "io.h"
#include "stdint.h"
#include "string.h"
#include "stddef.h"
#include "stdbool.h"

static bool shift_down = false;

static const char qwerty_table[0x80] = {
    0, 0x1B, '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n',
    'm', ',', '.', '/', 0, '*', 0, ' ', 0,
    [0x53] = 0x7F
};

static const char qwerty_table_shift[0x80] = {
    0, 0x1B, '!', '@', '#', '$', '%', '^', '&', '*',
    '(', ')', '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R',
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    '"', '~', 0, '|', 'Z', 'X', 'C', 'V', 'B', 'N',
    'M', '<', '>', '?', 0, '*', 0, ' ', 0,
    [0x53] = 0x7F
};

/* to later support other keyboard layouts */
static const char *scancode_table = qwerty_table;
static const char *scancode_table_shift = qwerty_table_shift;

char keyboard_getchar(void) {
    for (;;) {
        while (!(inb(0x64) & 1));
        uint8_t c = inb(0x60);
        if (c == 0x2A || c == 0x36) {
            shift_down = true;
            continue;
        }
        if (c == 0xAA || c == 0xB6) {
            shift_down = false;
            continue;
        }
        if (c & 0x80)
            continue;
        return shift_down ? scancode_table_shift[c] : scancode_table[c];
    }
}

char keyboard_pollchar(void) {
    if (!(inb(0x64) & 1))
        return 0;

    uint8_t c = inb(0x60);
    if (c == 0x2A || c == 0x36) {
        shift_down = true;
        return 0;
    }
    if (c == 0xAA || c == 0xB6) {
        shift_down = false;
        return 0;
    }
    if (c & 0x80)
        return 0;
    return shift_down ? scancode_table_shift[c] : scancode_table[c];
}

void read_line(char* buf, size_t size) {
    size_t idx = 0;
    while (true) {
        char c = keyboard_getchar();
        if (!c) continue;
        if (c == '\r' || c == '\n') {
            terminal_write("\n");
            buf[idx] = '\0';
            return;
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