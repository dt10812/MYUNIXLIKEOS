/* ps2 keyboard driver */
#include "io.h"
#include "stdint.h"
#include "string.h"
#include "stddef.h"
#include "stdbool.h"

static bool shift_down = false;
static bool ctrl_down = false;
static bool alt_down = false;
static bool capslock_on = false;

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

static bool is_lower_letter(char c) {
    return c >= 'a' && c <= 'z';
}

static char translate_scancode(uint8_t code) {
    char normal = scancode_table[code];
    char ch = shift_down ? scancode_table_shift[code] : normal;

    if (capslock_on && is_lower_letter(normal)) {
        if (shift_down)
            ch = normal;
        else
            ch = normal - 'a' + 'A';
    }

    if (ctrl_down) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
            ch &= 0x1F;
        }
    }

    if (alt_down && ch) {
        ch = (char)(0x80 | (uint8_t)ch);
    }

    return ch;
}

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
        if (c == 0x1D) {
            ctrl_down = true;
            continue;
        }
        if (c == 0x9D) {
            ctrl_down = false;
            continue;
        }
        if (c == 0x38) {
            alt_down = true;
            continue;
        }
        if (c == 0xB8) {
            alt_down = false;
            continue;
        }
        if (c == 0x3A) {
            capslock_on = !capslock_on;
            continue;
        }
        if (c & 0x80)
            continue;
        return translate_scancode(c);
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
    if (c == 0x1D) {
        ctrl_down = true;
        return 0;
    }
    if (c == 0x9D) {
        ctrl_down = false;
        return 0;
    }
    if (c == 0x38) {
        alt_down = true;
        return 0;
    }
    if (c == 0xB8) {
        alt_down = false;
        return 0;
    }
    if (c == 0x3A) {
        capslock_on = !capslock_on;
        return 0;
    }
    if (c & 0x80)
        return 0;
    return translate_scancode(c);
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