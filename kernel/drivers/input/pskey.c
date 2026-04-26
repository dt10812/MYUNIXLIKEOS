/* ps2 keyboard driver */
#include "io.h"
#include "stdint.h"
#include "string.h"
#include "stddef.h"
#include "stdbool.h"
#include "keyboard.h"

/* Input buffer for improved keyboard throughput */
#define INPUT_BUFFER_SIZE 256
char input_buffer[INPUT_BUFFER_SIZE];
size_t buffer_write_idx = 0;
size_t buffer_read_idx = 0;

static keyboard_layout_t current_layout = KEYBOARD_LAYOUT_QWERTY;

static bool shift_down = false;
static bool ctrl_down = false;
static bool alt_down = false;
static bool capslock_on = false;
static bool extended_scancode = false;

/* Special key codes for arrow keys */
#define KEY_UP    0xE0
#define KEY_DOWN  0xE1
#define KEY_LEFT  0xE2
#define KEY_RIGHT 0xE3

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

static const char azerty_table[0x80] = {
    0, 0x1B, '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', ')', '=', '\b', '\t', 'a', 'z', 'e', 'r',
    't', 'y', 'u', 'i', 'o', 'p', '^', '$', '\n', 0,
    'q', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm',
    '%', '`', 0, '*', 'w', 'x', 'c', 'v', 'b', 'n',
    ',', ';', ':', '!', 0, '*', 0, ' ', 0,
    [0x53] = 0x7F
};

static const char azerty_table_shift[0x80] = {
    0, 0x1B, '&', 0xE9, '"', '\'', '(', '-', 0xE8, '_',
    0xE7, 0xE0, ')', '=', '\b', '\t', 'A', 'Z', 'E', 'R',
    'T', 'Y', 'U', 'I', 'O', 'P', 0xA8, 0xA3, '\n', 0,
    'Q', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'M',
    0xB5, '~', 0, '*', 'W', 'X', 'C', 'V', 'B', 'N',
    '?', '.', '/', '+', 0, '*', 0, ' ', 0,
    [0x53] = 0x7F
};

/* to later support other keyboard layouts */
static const char *scancode_table;
static const char *scancode_table_shift;

void keyboard_set_layout(keyboard_layout_t layout) {
    current_layout = layout;
    switch (layout) {
        case KEYBOARD_LAYOUT_QWERTY:
            scancode_table = qwerty_table;
            scancode_table_shift = qwerty_table_shift;
            break;
        case KEYBOARD_LAYOUT_AZERTY:
            scancode_table = azerty_table;
            scancode_table_shift = azerty_table_shift;
            break;
    }
}

void keyboard_init(void) {
    keyboard_set_layout(KEYBOARD_LAYOUT_QWERTY); /* Default to QWERTY */
}

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
        
        /* Handle extended scancodes (0xE0 prefix) */
        if (c == 0xE0) {
            extended_scancode = true;
            continue;
        }
        
        if (extended_scancode) {
            extended_scancode = false;
            switch (c) {
                case 0x48: return KEY_UP;
                case 0x50: return KEY_DOWN;
                case 0x4B: return KEY_LEFT;
                case 0x4D: return KEY_RIGHT;
                default: continue; /* Ignore other extended keys */
            }
        }
        
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
    
    /* Handle extended scancodes (0xE0 prefix) */
    if (c == 0xE0) {
        extended_scancode = true;
        return 0;
    }
    
    if (extended_scancode) {
        extended_scancode = false;
        switch (c) {
            case 0x48: return KEY_UP;
            case 0x50: return KEY_DOWN;
            case 0x4B: return KEY_LEFT;
            case 0x4D: return KEY_RIGHT;
            default: return 0; /* Ignore other extended keys */
        }
    }
    
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

/* Buffer input from keyboard for higher throughput */
static char buffer_getchar(void) {
    char c = keyboard_pollchar();
    if (c && buffer_write_idx < INPUT_BUFFER_SIZE) {
        input_buffer[buffer_write_idx++] = c;
    }
    return c;
}

/* Read from the buffer */
static char buffer_read(void) {
    if (buffer_read_idx < buffer_write_idx) {
        return input_buffer[buffer_read_idx++];
    }
    return 0;
}

/* Reset buffer state */
static void buffer_reset(void) {
    buffer_write_idx = 0;
    buffer_read_idx = 0;
}

void read_line(char* buf, size_t size) {
    size_t idx = 0;
    buffer_reset();
    
    while (true) {
        /* First try to read from the buffer */
        char c = buffer_read();
        
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
            buffer_reset();
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