/* Non-standard program, should be deprecated eventually.
 * Should be later replaced "ls /bin" and man when ELF is done.
 */

#include "commands.h"
#include "string.h"
#include "stdio.h"

extern void terminal_write(const char*);

int cmd_help(int argc, char** argv) {
    if (argc < 2) {
        terminal_write("Available: cat, cd, echo, fs, grep, help, ls, mkdir, touch, clear\n");
        terminal_write("Usage: help [command]\n");
        return 0;
    }

    char* cmd = argv[1];

    if (!strcmp(cmd, "ls")) {
        terminal_write("ls [-a]: List files. -a shows . and ..\n");
    } else if (!strcmp(cmd, "cd")) {
        terminal_write("cd [dir]: Change directory.\n");
    } else if (!strcmp(cmd, "cat")) {
        terminal_write("cat [file]: Read file content.\n");
    } else if (!strcmp(cmd, "fs")) {
        terminal_write("fs [type]: Supports ntfs, exfat, fat32, ext4, apfs.\n");
    } else if (!strcmp(cmd, "echo")) {
        terminal_write("echo [text]: Print text to terminal.\n");
    } else if (!strcmp(cmd, "grep")) {
        terminal_write("grep [pattern] [file]: Search in file.\n");
    } else if (!strcmp(cmd, "mkdir")) {
        terminal_write("mkdir [name]: Create directory.\n");
    } else if (!strcmp(cmd, "touch")) {
        terminal_write("touch [file]: Create empty file.\n");
    } else if (!strcmp(cmd, "help")) {
        terminal_write("help [cmd]: Detailed info on commands.\n");
    } else {
        terminal_write("sh: command not found: ");
        terminal_write(cmd);
        terminal_write("\n");
    }
    return 0;
}
