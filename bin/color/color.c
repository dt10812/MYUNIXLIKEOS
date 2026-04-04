#include "commands.h"
#include "stdio.h"
#include "string.h"
#include "io.h"

static const struct {
    const char *name;
    uint8_t value;
} color_table[] = {
    {"black",        0x00},
    {"blue",         0x01},
    {"green",        0x02},
    {"cyan",         0x03},
    {"red",          0x04},
    {"magenta",      0x05},
    {"brown",        0x06},
    {"lightgray",    0x07},
    {"darkgray",     0x08},
    {"lightblue",    0x09},
    {"lightgreen",   0x0A},
    {"lightcyan",    0x0B},
    {"lightred",     0x0C},
    {"lightmagenta", 0x0D},
    {"yellow",       0x0E},
    {"white",        0x0F},
};

static int parse_color_nibble(const char *arg) {
    if (!arg) return -1;

    if (arg[0] == '0' && arg[1] == 'x' && arg[2] && !arg[3]) {
        char c = arg[2];
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    for (size_t i = 0; i < sizeof(color_table) / sizeof(color_table[0]); i++) {
        if (!strcmp(color_table[i].name, arg))
            return color_table[i].value;
    }

    return -1;
}

static int parse_color_attribute(const char *arg) {
    if (!arg) return -1;

    if (arg[0] == '0' && arg[1] == 'x') {
        int value = 0;
        const char *p = arg + 2;
        int digits = 0;
        for (; *p; p++) {
            char c = *p;
            int digit;
            if (c >= '0' && c <= '9')
                digit = c - '0';
            else if (c >= 'a' && c <= 'f')
                digit = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                digit = c - 'A' + 10;
            else
                return -1;
            value = (value << 4) | digit;
            digits++;
        }
        return (digits == 1 || digits == 2) ? value : -1;
    }

    return parse_color_nibble(arg);
}

int cmd_color(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: color <foreground> [background]\n");
        printf("Foreground and background may be names or 0xN.\n");
        printf("Supported colors: black blue green cyan red magenta brown lightgray darkgray lightblue lightgreen lightcyan lightred lightmagenta yellow white\n");
        printf("Example: color red blue or color 0xC 0x1\n");
        return 0;
    }

    if (!strcmp(argv[1], "reset") || !strcmp(argv[1], "default")) {
        terminal_set_color(0x0F);
        printf("Text color reset to default.\n");
        return 0;
    }

    uint8_t attr;
    if (argc == 2 && argv[1][0] == '0' && argv[1][1] == 'x' && argv[1][2] && argv[1][3] && !argv[1][4]) {
        int value = parse_color_attribute(argv[1]);
        if (value < 0) {
            printf("color: unknown color '%s'\n", argv[1]);
            return 1;
        }
        attr = (uint8_t)value;
        terminal_set_color(attr);
        printf("Color attribute set to %s.\n", argv[1]);
        return 0;
    }

    int fg = parse_color_nibble(argv[1]);
    if (fg < 0) {
        printf("color: unknown foreground '%s'\n", argv[1]);
        return 1;
    }

    if (argc == 2) {
        attr = (uint8_t)fg;
        terminal_set_color(attr);
        printf("Foreground color changed to %s.\n", argv[1]);
        return 0;
    }

    int bg = parse_color_nibble(argv[2]);
    if (bg < 0) {
        printf("color: unknown background '%s'\n", argv[2]);
        return 1;
    }

    attr = (uint8_t)((bg << 4) | fg);
    terminal_set_color(attr);
    printf("Foreground set to %s and background set to %s.\n", argv[1], argv[2]);
    return 0;
}
