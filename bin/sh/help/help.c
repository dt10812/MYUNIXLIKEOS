#include "commands.h"
#include "stdio.h"
#include "string.h"

extern void terminal_write(const char*);

int cmd_help(int argc, char** argv) {
    if (argc < 2) {
        terminal_write("Available: cat, cd, color, echo, fs, grep, help, ls, memory, mkdir, snake, touch, clear\n");
        terminal_write("Usage: help [command]\n");
        return 0;
    }

    char* cmd = argv[1];

    if (!strcmp(cmd, "ls")) {
        printf("ls [-a]: List files. -a shows . and ..\n");
    } else if (!strcmp(cmd, "cd")) {
        printf("cd [dir]: Change directory.\n");
    } else if (!strcmp(cmd, "cat")) {
        printf("cat [file]: Read file content.\n");
    } else if (!strcmp(cmd, "fs")) {
        printf("fs [type]: Supports ntfs, exfat, fat32, ext4, apfs.\n");
    } else if (!strcmp(cmd, "echo")) {
        printf("echo [text]: Print text to terminal.\n");
    } else if (!strcmp(cmd, "color")) {
        printf("color <foreground> [background]: Change text foreground and optional background.\n");
        printf("Colors are names or 0xN, and 0xNN can set both at once.\n");
        printf("Supported names: black, blue, green, cyan, red, magenta, brown, lightgray, darkgray, lightblue, lightgreen, lightcyan, lightred, lightmagenta, yellow, white.\n");
    } else if (!strcmp(cmd, "memory")) {
        printf("memory: Show OS memory usage for physical page allocation.\n");
        printf("Usage: memory\n");
    } else if (!strcmp(cmd, "snake")) {
        printf("snake: Start a simple snake game using W,A,S,D to move.\n");
        printf("Use: snake          # start game\n");
        printf("     snake color <snake|food|border|board|text> <color>\n");
        printf("     snake reset     # restore default game colors\n");
    } else if (!strcmp(cmd, "grep")) {
        printf("grep [pattern] [file]: Search in file.\n");
    } else if (!strcmp(cmd, "mkdir")) {
        printf("mkdir [name]: Create directory.\n");
    } else if (!strcmp(cmd, "touch")) {
        printf("touch [file]: Create empty file.\n");
    } else if (!strcmp(cmd, "help")) {
        printf("help [cmd]: Detailed info on commands.\n");
    } else {
        printf("Unknown command.\n");
    }

    return 0;
}